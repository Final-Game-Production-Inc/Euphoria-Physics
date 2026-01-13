//
// system/timemgr.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "timemgr.h"

#include "diag/debuglog.h"
#include "diag/output.h"
#include "math/amath.h"

using namespace rage;

sysTimeManager sysTimeManager::sm_GlobalManager;

sysTimeManager::sysTimeManager()
: m_Seconds(1.0f / 60.0f)
, m_InvSeconds(60.f)
, m_PrevElapsedTime(0.0f)
, m_ElapsedTime(0.0f)
, m_UnpausedElapsedTime(0.0f)
, m_FPS(60.0f)
, m_SampleStep(1.0f / 60.0f)
, m_ClampMax(0.1f)
, m_ClampMin(0.0001f)
, m_FrameCount(0)
, m_UnpausedFrameCount(0)
, m_OverSamples(1)
, m_FirstFrame(true)
, m_ShowFrame(false)
, m_FrameStep(false)
, m_Mode(sysTimeManager::REALTIME)
, m_Time()
, m_bClampElapsedTime(false)
, m_TempOverSampling(false)
, m_TempOverSampleAmount(0)
, m_TempSeconds(0.0f)
, m_TimeWarp(1.0f)
, m_UnwarpedSeconds(1.0f / 60.0f)
, m_UnwarpedRealtimeSeconds(1.0f / 60.0f)
, m_WarpedRealtimeSeconds(1.0f / 60.0f)
, m_UnwarpedReplaySeconds(1.0f / 60.0f)
, m_WarpedReplaySeconds(1.0f / 60.0f)
{
}


sysTimeManager::~sysTimeManager()
{
}


void sysTimeManager::Reset()
{
	m_FirstFrame = true;
	m_Seconds = 1.0f / m_FPS;
	m_InvSeconds = m_FPS;
	m_PrevElapsedTime = m_ElapsedTime = m_UnpausedElapsedTime = 0.0f;
	m_FrameCount = 0;
	m_UnpausedFrameCount = 0;
	m_TimeWarp = 1.0f;
	m_UnwarpedSeconds = m_Seconds;
}

void sysTimeManager::Update(float replayUnwarpedSeconds, bool unpaused)
{
	m_UnwarpedRealtimeSeconds = m_Time.GetTimeAndReset();
	m_WarpedRealtimeSeconds = m_UnwarpedRealtimeSeconds * m_TimeWarp;

	m_UnwarpedReplaySeconds = Selectf(replayUnwarpedSeconds, replayUnwarpedSeconds, m_UnwarpedRealtimeSeconds);
	m_WarpedReplaySeconds = m_UnwarpedReplaySeconds * m_TimeWarp;

	m_UnwarpedSeconds = Selectf(replayUnwarpedSeconds, replayUnwarpedSeconds, m_UnwarpedRealtimeSeconds);
	m_Seconds = m_UnwarpedSeconds * m_TimeWarp;

	bool bApplyTimePostClamp = m_bClampElapsedTime;

	if (!IsFirstFrame())
	{
		m_PrevElapsedTime=m_ElapsedTime;

		if (!m_FrameStep && m_Mode==REALTIME)
		{
			if (!m_bClampElapsedTime)
			{
				m_ElapsedTime += m_Seconds;

				if (unpaused)
				{
					m_UnpausedElapsedTime += m_Seconds;
				}
			}

			// fixed real time...
			if (m_SampleStep) 
			{
				if (m_Seconds<m_SampleStep)
				{
					m_OverSamples=1;
				}
				else
				{
					// Removed by Sam
					//m_OverSamples=(unsigned int)ceil(timeSinceLastFrame/m_SampleStep+.001);
					//m_Seconds=timeSinceLastFrame/m_OverSamples;
					m_UnwarpedSeconds = m_SampleStep;
					m_Seconds = m_SampleStep * m_TimeWarp;
				}
			}
		}
		// fixed frame:
		else 
		{
			m_UnwarpedSeconds = m_SampleStep;
			m_Seconds = m_SampleStep * m_TimeWarp;	// apply timewarp to fixed-frame mode too
			m_ElapsedTime += m_Seconds;
			bApplyTimePostClamp = false;

			if (unpaused)
			{
				m_UnpausedElapsedTime += m_Seconds;
			}
		}
	}
	else 
	{
		if (m_Mode==REALTIME)
		{
			m_Seconds=m_SampleStep?m_SampleStep:1.0f/m_FPS;
		}
		else
		{
			m_Seconds=m_SampleStep;
		}

		if (!m_bClampElapsedTime)
		{
			m_ElapsedTime += m_Seconds;

			if (unpaused)
			{
				m_UnpausedElapsedTime += m_Seconds;
			}
		}

		m_UnwarpedSeconds = m_Seconds;
		m_FirstFrame=false;
	}


#if !__NO_OUTPUT && 0
	// print out current frame:
	if (m_ShowFrame && diagOutput::OutputThisFrame())
	{
		Displayf(">>>>>>>>>>>>>>>>>>>>>>>>>> FRAME: %u <<<<<<<<<<<<<<<<<<<<<<<<<<<<\n",m_FrameCount);
		diagOutput::SetOutputThisFrame(false);
	}
#endif

	++m_FrameCount;
	if (unpaused)
		++m_UnpausedFrameCount;

	m_Seconds = Clamp(m_Seconds, m_ClampMin, m_ClampMax);
	m_UnwarpedSeconds = Clamp(m_UnwarpedSeconds, m_ClampMin, m_ClampMax);

	if (bApplyTimePostClamp)
	{
		m_ElapsedTime += m_Seconds;

		if (unpaused)
		{
			m_UnpausedElapsedTime += m_Seconds;
		}
	}

	m_InvSeconds=1.0f/m_Seconds;

	if (unpaused)
	{
		diagDebugLog(diagDebugLogMisc, 'TMTW', &m_TimeWarp);
		diagDebugLog(diagDebugLogMisc, 'TMUT', &m_UnpausedElapsedTime);
		diagDebugLog(diagDebugLogMisc, 'TMSe', &m_Seconds);
		diagDebugLog(diagDebugLogMisc, 'TMUS', &m_UnwarpedSeconds);
	}
}


/*
PURPOSE
	Set the temp oversampling.
PARAMS
	active - true to activate the temp oversampling.
	amount - the amount to be set.
RETURNS
	none.
NOTES
*/
void sysTimeManager::SetTempOverSampling(bool active, u32 amount)
{
	if (active)
	{
		if (m_TempOverSampling)
		{
			m_Seconds = m_TempSeconds;
			m_InvSeconds = 1 / m_Seconds;
		}

		m_TempOverSampling = true;
		m_TempSeconds = m_Seconds;
		m_TempOverSampleAmount = amount;

		m_Seconds /= amount;
		m_InvSeconds = 1 / m_Seconds;
	}
	else if (active == false && m_TempOverSampling)
	{
		m_TempOverSampling = false;
		m_TempOverSampleAmount = 0;
		m_Seconds = m_TempSeconds;
		m_InvSeconds = 1 / m_Seconds;
	}
}


void sysTimeManager::SetRealTimeMode(float minfps, float fps)
{
	m_FPS=fps?fps:60.0f;
	m_Mode=REALTIME;
	m_SampleStep=minfps?1.0f/minfps:0.0f;
}

void sysTimeManager::SetFixedFrameMode(float fps, u32 over)
{
	Assert(over && fps);
	m_FPS=fps;
	m_OverSamples=over;
	m_Mode=FIXEDFRAME;
	m_SampleStep=1.0f/(m_FPS*(m_OverSamples));
	m_Seconds=m_SampleStep;
	m_InvSeconds=1.0f/m_Seconds;
	m_UnwarpedSeconds = m_Seconds;
}

sysTimeManager::EnumTimeMode sysTimeManager::GetMode() const
{
	return m_Mode;
}

