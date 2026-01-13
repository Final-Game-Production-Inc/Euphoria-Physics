//
// system/timemgr.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_TIMEMGR_H
#define SYSTEM_TIMEMGR_H

#include "timer.h"
#include "vectormath/classes.h"

namespace rage {

/*
PURPOSE
A time management class for the entire simulation.  The normal interface is through
the TIME macro, which returns the global sysTimeManager instance.

NOTES
Use <c>TIME.GetSeconds()</c> in your <c>Update()</c> functions as your time step.  
The main two modes are:

* <c>REALTIME</c> - <c>GetSeconds()</c> returns the true amount of time between the 
last two calls to <c>Update()</c>.
* <c>FIXEDFRAME</c> - Runs the simulation with the same time step every frame 
(typically 1/30th or 1/60th second).

Your application is responsible for calling <c>TIME.Update()<c> at the top of the 
update pass.  
Please use <c>TIME.GetInvSeconds()</c> when possible, rather than dividing by 
<c>TIME.GetSeconds()</c>.  <c>TIME.GetSeconds()</c> is plenty fast because the 
system clock is only checked when <c>TIME.Update()</c> is called.
<FLAG Component>
*/


class sysTimeManager {
public:
	// PURPOSE: Create a new time manager. 
	sysTimeManager();

	virtual ~sysTimeManager();

	// PURPOSE: Resets all the time accumulators (ElapsedTime, FrameCount, etc.)
	virtual void Reset();

	// PURPOSE: Updates the timers. Call this once per frame.
	virtual void Update(float replayUnwarpedSeconds = -1.f, bool unpaused = true);

	//PURPOSE: Set the real time mode.
	//PARAMS
	//	minfps - clamp frametime to never go below minfps
	//	fps - used to set the sample step for the first frame and when IsFrameStep is true
	void SetRealTimeMode(float minfps=0.0f, float fps=0.0f);

	//PURPOSE: Set the fixed time mode.
	//PARAMS
	//	fps - frame per second to be assigned.
	//	over - used for testing.
	void SetFixedFrameMode(float fps,u32 over=1);

	// PURPOSE: Returns the global time manager instance
	// RETURNS: The global time manager instance
	static sysTimeManager&	GetGlobalManager();

	// PURPOSE: Returns the amount of time since the start (or the last reset)
	// RETURNS: The amount of time since the start
	float		GetElapsedTime() const;

	// PURPOSE: Returns the amount of unpaused time since the start (or the last reset)
	// RETURNS: The amount of unpaused time since the start
	float		GetUnpausedElapsedTime() const;

	// PURPOSE: Returns the amount of time since the start (or the last reset) as of our *previous* update
	// RETURNS: The amount of time since the start, as of our *previous* update
	float		GetPrevElapsedTime() const;

	// PURPOSE: Returns the amount of time between the last two updates (i.e. the elapsed frame time)
	// RETURNS: The amount of time between the last two updates (i.e. the elapsed frame time)
	// NOTES:	Returning a reference here so that we can load it in the vector pipeline (which is not a LHS if the float is already in memory).
	float			GetSeconds() const;
	const float&	GetSecondsConstRef() const;
	ScalarV_Out		GetSecondsV() const;

	// PURPOSE: Returns 1.0 / elapsed frame time. Use this when possible instead of dividing by GetSeconds()
	// RETURNS: 1.0 / elapsed frame time. Use this when possible instead of dividing by GetSeconds()
	// NOTES:	Returning a reference here so that we can load it in the vector pipeline (which is not a LHS if the float is already in memory).
	float			GetInvSeconds() const;
	const float&	GetInvSecondsConstRef() const;
	ScalarV_Out		GetInvSecondsV() const;

	// PURPOSE: Returns the number of frames per second
	// RETURNS: The number of frames per second
	float		GetFPS() const;

	// PURPOSE: Returns the actual frames per second
	// RETURNS: The actual frames per second
	// NOTES: This could be different from GetFPS because of warping or because the time manager is in FixedFrame mode
	float		GetFPSActual() const;	

	// PURPOSE: Returns the number of oversamples (where fixed framerate step = 1.0 / FPS * Oversamples)
	// RETURNS: The number of oversamples (where fixed framerate step = 1.0 / FPS * Oversamples)
	u32	GetOverSamples() const;

	// PURPOSE: Returns the temporary oversample amount.
	// RETURNS: The temporary oversample amount.
	u32			GetTempOverSamples() const;

	// PURPOSE: Gets the time warp factor (the about by which time is sped up or slowed down)
	// RETURNS: The time warp factor (the about by which time is sped up or slowed down)
	float		GetTimeWarp() const;

	// PURPOSE: Returns the number of frames since the beginning (or the last reset)
	// RETURNS: The number of frames since the beginning (or the last reset)
	u32			GetFrameCount() const;

	// PURPOSE: Returns the number of frames since the beginning (or the last reset) not counting paused frames
	// RETURNS: The number of frames since the beginning (or the last reset) not counting paused frames
	u32			GetUnpausedFrameCount() const;

	// PURPOSE: Gets the amount of time between the last to updates, regardless of time warping
	// RETURNS: The about of time between the last to updates, regardless of time warping
	float		GetUnwarpedSeconds() const;
	void		SetUnwarpedSeconds(float NewVal);

	// PURPOSE: Gets the amount of time between the last two updates, regardless of time warping or fixed frame mode
	// RETURNS: The about of time between the last two updates, regardless of time warping or fixed frame mode
	float		GetUnwarpedRealtimeSeconds() const;

	// PURPOSE: Similar to GetUnwarpedRealtimeSeconds, however during a replay, returns the recorded value.
	float		GetUnwarpedReplaySeconds() const;

	// PURPOSE: Gets the warped amount of time between the last two updates, regardless of fixed frame mode
	// RETURNS: The warped amount of time between the last two updates, regardless of fixed frame mode
	// NOTES:   Intended for audio that needs realtime but wants a pitch shift resulting from time warping
	float		GetWarpedRealtimeSeconds() const;

	// PURPOSE: Similar to GetWarpedRealtimeSeconds, however during a replay, returns the recorded value.
	float		GetWarpedReplaySeconds() const;

	// PURPOSE: Returns true before the first Update (or between a Reset and the next Update)
	// RETURNS: True before the first Update (or between a Reset and the next Update)
	bool			IsFirstFrame() const;

	// PURPOSE: Determine whether the time manager is using a constant step interval between updates
	// RETURNS: True if the time manager is using a constant step interval between updates
	// NOTES: 
	//	FrameStep is similar to FixedFrameMode, but the settings are independent.
	//  That is, if FrameStep is true <i>or</i> the time manager is in FixedFrameMode
	//	each update will report that a constant interval of time has elapsed.
	bool			IsFrameStep() const;

	// PURPOSE: Sets the amount of time elapsed since last restart
	// PARAMS: t - the amount of time elapsed since last restart
	// NOTES: Useful for replay
	void			SetElapsedTime(float t);

	// PURPOSE: Sets the amount of unpaused time elapsed since last restart
	// PARAMS: t - the amount of time elapsed since last restart
	// NOTES: Useful for replay
	void			SetUnpausedElapsedTime(float t);

	// PURPOSE: Sets the amount of previous time elapsed since last restart
	// PARAMS: t - the amount of previous time elapsed since last restart
	// NOTES: Useful for replay
	void			SetPrevElapsedTime(float t);

	// PURPOSE: Sets the amount of time between the last two updates
	// PARAMS: t - the amount of time between the last two updates
	//         invt - one over t, the first parameter
	// NOTES: Useful for replay
	void			SetSeconds(float t, float invt);

	// PURPOSE: Turns on or off frame step (each updating taking a constant time inverval)
	// PARAMS: trueOrFalse - Whether to tur frame step on or off
	void			SetFrameStep(bool trueOrFalse);

	// PURPOSE: Gets the min and max amount of time between each update.
	// PARAMS
	//   min - The minimum amount of time between each update
	//   max - The maximum amount of time between each update
	void			GetFrameTimeClamp (float& min, float& max) const;

	// PURPOSE: Sets the min and max amount of time between each update.
	// PARAMS
	//   min - The minimum amount of time between each update
	//   max - The maximum amount of time between each update
	void			SetClamp(float min,float max);

	// PURPOSE: Set this to true to print a frame label each frame
	// PARAMS: trueFalse - Whether to turn frame labelling on or off each frame
	void			ShowFrameOutput(bool trueFalse);

	// PURPOSE: Sets the number of seconds between updates
	// PARAMS: s - the number of seconds between updates
	// NOTES: Useful for replay
	void			SetSeconds(float s);

	// PURPOSE: Activates or deactivates temporary oversampling.
	// PARAMS:
	//   active - If true, activates temporary oversampling
	//   amount - When activating, the number of oversamples to perform.
	void			SetTempOverSampling(bool active, u32 amount);

	// PURPOSE: Sets the time warp factor. This will speed up or slow down time as reported by GetSeconds, GetElapsedSeconds, etc.
	// PARAMS: tw - The desired time warp factor
	void			SetTimeWarp(float tw);

	// PURPOSE: Sets the current frame count.
	// PARAMS: k - The frame count to become
	void			SetFrameCount(u32 k);

	// PURPOSE:	Reset internal timer, for when an application loses focus.
	void			ResetInternalTimer();

	// PURPOSE: To keep track of whether the system is in real time or fixed frame mode
	enum EnumTimeMode
	{
		REALTIME,  // The system is using the actual measured frame-to-frame time as a frame update time
		FIXEDFRAME // The system is inverting the frames per second passed to SetFixedFrameMode to get the frame update time
	};

	// RETURNS: the mode the system is currently in
	EnumTimeMode	GetMode() const;

	// PURPOSE: Clamps the internally accumulated elapsed time
	void			SetClampElapsedTime(bool bClampOn);
protected:

	float			m_Seconds;
	float			m_InvSeconds;
	float			m_PrevElapsedTime;
	float			m_ElapsedTime;
	float			m_UnpausedElapsedTime;
	float			m_FPS;
	float			m_SampleStep;
	float			m_ClampMax;
	float			m_ClampMin;
	u32				m_FrameCount;
	u32				m_UnpausedFrameCount;
	u32				m_OverSamples;	
	bool			m_FirstFrame;
	bool			m_ShowFrame;
	bool			m_FrameStep;
	EnumTimeMode	m_Mode;
	sysTimer		m_Time;
	bool			m_bClampElapsedTime;
	bool			m_TempOverSampling;
	u32				m_TempOverSampleAmount;
	float			m_TempSeconds;
	float           m_TimeWarp;				// we can play with how fast time goes by!
	float			m_UnwarpedSeconds;		// seconds elapsed this frame, unaffected by m_TimeWarp
	float			m_UnwarpedRealtimeSeconds;// actual time, unchanged by fixing framerate or warps.  Use with care
	float			m_WarpedRealtimeSeconds;// actual time, unchanged by fixing framerate but scaled by warps.  Use with care
	float			m_UnwarpedReplaySeconds;// replayed version of m_UnwarpedSeconds
	float			m_WarpedReplaySeconds;// replayed version of m_WarpedRealtimeSeconds

	static sysTimeManager	sm_GlobalManager;
};

// A convenience macro to access the global time manager
#define TIME (::rage::sysTimeManager::GetGlobalManager())

inline sysTimeManager&	sysTimeManager::GetGlobalManager() {
	return sm_GlobalManager;
}

inline float sysTimeManager::GetElapsedTime() const 
{
	return(m_ElapsedTime);
}

inline float sysTimeManager::GetUnpausedElapsedTime() const
{
	return(m_UnpausedElapsedTime);
}

inline float sysTimeManager::GetPrevElapsedTime() const 
{
	return(m_PrevElapsedTime);
}

inline float sysTimeManager::GetSeconds() const 
{
	return(m_Seconds);
}

inline const float& sysTimeManager::GetSecondsConstRef() const 
{
	return(m_Seconds);
}

inline ScalarV_Out sysTimeManager::GetSecondsV() const
{
	return ScalarVFromF32(m_Seconds);
}

inline float sysTimeManager::GetInvSeconds() const 
{
	return(m_InvSeconds);
}

inline const float& sysTimeManager::GetInvSecondsConstRef() const 
{
	return(m_InvSeconds);
}

inline ScalarV_Out sysTimeManager::GetInvSecondsV() const 
{
	return ScalarVFromF32(m_InvSeconds);
}

inline float sysTimeManager::GetFPS() const 
{
	return(m_InvSeconds);
}

inline u32 sysTimeManager::GetOverSamples() const
{
	return(m_OverSamples);
}

inline u32 sysTimeManager::GetTempOverSamples() const
{
	return(m_TempOverSampleAmount);
}

inline float sysTimeManager::GetTimeWarp() const
{
	return(m_TimeWarp);
}

inline u32 sysTimeManager::GetFrameCount() const
{
	return(m_FrameCount);
}

inline u32 sysTimeManager::GetUnpausedFrameCount() const
{
	return(m_UnpausedFrameCount);
}

inline float sysTimeManager::GetUnwarpedSeconds() const
{
	return(m_UnwarpedSeconds);
}	

inline void sysTimeManager::SetUnwarpedSeconds(float NewVal)
{
	m_UnwarpedSeconds = NewVal;
}

inline float sysTimeManager::GetUnwarpedRealtimeSeconds() const
{
	return(m_UnwarpedRealtimeSeconds);
}

inline float sysTimeManager::GetUnwarpedReplaySeconds() const
{
	return(m_UnwarpedReplaySeconds);
}

inline float sysTimeManager::GetWarpedRealtimeSeconds() const
{
	return(m_WarpedRealtimeSeconds);
}

inline float sysTimeManager::GetWarpedReplaySeconds() const
{
	return(m_WarpedReplaySeconds);
}

inline bool sysTimeManager::IsFirstFrame() const
{
	return(m_FirstFrame);
}

inline bool sysTimeManager::IsFrameStep() const
{
	return(m_FrameStep);
}

inline void sysTimeManager::SetElapsedTime(float t)
{
	m_ElapsedTime=t;
}

inline void sysTimeManager::SetUnpausedElapsedTime(float t)
{
	m_UnpausedElapsedTime=t;
}

inline void sysTimeManager::SetPrevElapsedTime(float t)
{
	m_PrevElapsedTime=t;
}

inline void sysTimeManager::SetSeconds(float t, float invt)
{
	m_Seconds=t; 
	m_InvSeconds=invt;
}

inline void sysTimeManager::SetFrameStep(bool trueOrFalse)
{
	m_FrameStep=trueOrFalse;
}

inline void sysTimeManager::SetClamp(float min,float max)
{
	m_ClampMin=min; 
	m_ClampMax=max;
}

inline void sysTimeManager::GetFrameTimeClamp(float& min, float& max) const
{
	min = m_ClampMin;
	max = m_ClampMax;
}

inline void sysTimeManager::ShowFrameOutput(bool trueFalse)
{
	m_ShowFrame=trueFalse;
}

inline void sysTimeManager::SetSeconds(float s)
{
	m_Seconds=s;
	m_InvSeconds=1.0f/s;
}

inline void sysTimeManager::SetTimeWarp(float tw)
{
	m_TimeWarp = tw;
}

inline void sysTimeManager::SetFrameCount(u32 k)
{
	m_FrameCount = k;
}

inline float sysTimeManager::GetFPSActual() const
{
	float diff=m_ElapsedTime-m_PrevElapsedTime;
	return ((diff==0)?60.0f:1.0f/diff);
}

inline void	sysTimeManager::SetClampElapsedTime(bool bClampOn)
{
	m_bClampElapsedTime = bClampOn;
}

inline void sysTimeManager::ResetInternalTimer()
{
	m_Time.Reset();
}

} // namespace rage
#endif // SYSTEM_TIMEMGR_H

