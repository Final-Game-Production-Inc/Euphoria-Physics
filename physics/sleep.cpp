//
// physics/sleep.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "sleep.h"

#include "colliderdispatch.h"

#include "bank/bank.h"
#include "data/callback.h"
#include "data/resourcehelpers.h"
#include "diag/output.h"
#include "math/simplemath.h"
#include "physics/simulator.h"
#include "physics/sleepmgr.h"
#include "grprofile/drawmanager.h"
#include "system/timemgr.h"
#include "vector/colors.h"
#include "vector/matrix33.h"

namespace rage {


	//=============================================================================
	// Debug drawing

	EXT_PFD_DECLARE_ITEM(PutToSleep);
	EXT_PFD_DECLARE_ITEM(Awakened);
	EXT_PFD_DECLARE_ITEM(SleepHUD);
	PH_SLEEP_DEBUG_ONLY(EXT_PFD_DECLARE_ITEM(SleepDebug);)


	//=============================================================================
	// phSleep

	int phSleep::sm_DefaultMaxMotionlessTicks = 15;
	int phSleep::sm_DefaultMaxAsleepTicks = 20;

	// Global parameters for the maximum velocity and angular velocity at 
	// which a collider can sleep.

	// Current default
	float phSleep::sm_DefaultVelTolerance2 = 0.005f;
	float phSleep::sm_DefaultAngVelTolerance2 = 0.01f;
	float phSleep::sm_DefaultInternalVelTolerance2 = 0.2f;

#if PH_SLEEP_DEBUG
#define ADD_DEBUG_RECORD(X) \
	if (PFD_SleepDebug.WillDraw() && m_Collider && m_Collider->GetInstance() && m_Collider->GetInstance()->IsInLevel()) \
	{ \
		PHSLEEP->AddDebugRecord(X, PHLEVEL->GetHandle(m_Collider->GetInstance())); \
	}
#else // PH_SLEEP_DEBUG
#define ADD_DEBUG_RECORD(X)
#endif // PH_SLEEP_DEBUG

	phSleep::phSleep (phCollider* collider)
	{
		Init(collider);
	}


	void phSleep::Init (phCollider* collider)
	{
		m_VelTolerance2 = sm_DefaultVelTolerance2;
		m_AngVelTolerance2 = sm_DefaultAngVelTolerance2;
		m_InternalVelTolerance2 = sm_DefaultInternalVelTolerance2;
		m_MaxMotionlessTicks = sm_DefaultMaxMotionlessTicks;
		m_MaxAsleepTicks = sm_DefaultMaxAsleepTicks;
		m_Collider = collider;
		if (m_Collider)
		{
			Reset();
		}
	}


	void phSleep::Reset (bool UNUSED_PARAM(fullReset))
	{
		m_MotionlessTicks = 0;
		m_AsleepTicks = 0;
		m_ActiveSleeper = false;
		WakeUp();

		ADD_DEBUG_RECORD(RESET);
	}


	void phSleep::WakeUp ()
	{
		ADD_DEBUG_RECORD(WAKE);

#if __PFDRAW
		Vector3 AWAKENED_DRAW_SIZE(0.2f,0.2f,0.2f);
		PFD_Awakened.DrawBox(RCC_MATRIX34(m_Collider->GetMatrix()),AWAKENED_DRAW_SIZE);
#endif

		// The counters (m_MotionlessTicks and m_AsleepTicks) are not reset to zero here so that their
		// accumulated values are still used to let the object immediately return to sleep or inactivity
		// if it is not moving.
		m_CurrentMode = AWAKE;
	}

	void phSleep::BlockSleepFinishing(int numUpdates)
	{
		ADD_DEBUG_RECORD(BLOCK);

		Assert(m_MaxAsleepTicks > numUpdates+1);
		if(IsAsleep())
		{
			m_AsleepTicks = Clamp(m_MaxAsleepTicks-(numUpdates+1),0,m_AsleepTicks);
			m_CurrentMode = ASLEEP;
		}
	}

	void phSleep::SendToSleep ()
	{
		// The m_AsleepTicks counter is not reset to zero here so that its accumulated value can still be
		// used to let the object immediately return to sleep again if it is awakened and not moving.
		m_CurrentMode = ASLEEP;
	}

	void phSleep::ComputeStillness (float timeStep, float timeStepForForce, float& effectiveAngVel2, float& effectiveVel2, float& nextAngVelDiff, float& nextVelDiff, float& internalVel) const
	{
		Vector3 effectiveAngVel(RCC_VECTOR3(m_Collider->GetTurn()));
		effectiveAngVel.Add(RCC_VECTOR3(m_Collider->GetAppliedTurn()));
		float invTimeStep = InvertSafe(timeStep);
		effectiveAngVel.Scale(invTimeStep);
		effectiveAngVel.Add(RCC_VECTOR3(m_Collider->GetAngVelocity()));
		effectiveAngVel.Subtract(RCC_VECTOR3(m_Collider->GetReferenceFrameAngularVelocity()));
		effectiveAngVel2 = effectiveAngVel.Mag2();

		Vector3 effectiveVelocity(RCC_VECTOR3(m_Collider->GetAppliedPush()));
		effectiveVelocity.Scale(invTimeStep);
		effectiveVelocity.Add(RCC_VECTOR3(m_Collider->GetVelocity()));
		effectiveVelocity.Subtract(RCC_VECTOR3(m_Collider->GetReferenceFrameVelocity()));
		effectiveVel2 = effectiveVelocity.Mag2();

		ScalarV timeStepForForceV = ScalarVFromF32(timeStepForForce);
		Vector3 nextAngVel = VEC3V_TO_VECTOR3(m_Collider->GetAngImpulse(timeStepForForceV.GetIntrin128()));	// use zero timeStep because we don't want to include forces
		Matrix34 invInertia;
		Matrix33 i33;
		m_Collider->GetInverseInertiaMatrix(RC_MAT33V(i33));
		invInertia = i33;
		invInertia.Transform3x3(nextAngVel);
		nextAngVel.Add(effectiveAngVel);
		nextAngVelDiff = nextAngVel.Mag2()-effectiveAngVel2;

		Vector3 nextVelocity = VEC3V_TO_VECTOR3(m_Collider->GetImpulse(timeStepForForceV.GetIntrin128()));	// use zero timeStep because we don't want to include forces
		nextVelocity.Scale(m_Collider->GetInvMass());
		nextVelocity.Add(effectiveVelocity);
		nextVelDiff = nextVelocity.Mag2()-effectiveVel2;

		internalVel = m_Collider->GetTotalInternalMotion();
	}

	bool phSleep::CheckNearlyStill (float timeStep, float timeStepForForce)
	{
		float effectiveAngVel2, effectiveVel2, nextAngVelDiff, nextVelDiff, internalVel;
		ComputeStillness(timeStep, timeStepForForce, effectiveAngVel2, effectiveVel2, nextAngVelDiff, nextVelDiff, internalVel);

		if (effectiveAngVel2>m_AngVelTolerance2)
		{
			// The effective angular velocity is too large, so return false for not nearly still.
			return false;
		}

		if (effectiveVel2>m_VelTolerance2)
		{
			// The effective velocity is too large, so return false for not nearly still.
			return false;
		}

		if (nextAngVelDiff>m_AngVelTolerance2)
		{
			// The change in angular velocity is too large, so return false for not nearly still.
			return false;
		}

		if (nextVelDiff>m_VelTolerance2)
		{
			// The change in velocity is too large, so return false for not nearly still.
			return false;
		}

		if (internalVel > m_InternalVelTolerance2)
		{
			// Too much internal motion, as in a ragdoll
			return false;
		}

		// All the motion tests were passed, so return true for nearly still.
		return true;
	}

#if __PFDRAW
	extern float g_LastPhysicsTimeStep;

	void phSleep::ProfileDraw() const
	{
		if (PFD_SleepHUD.Begin())
		{
			float effectiveAngVel2, effectiveVel2, nextAngVelDiff, nextVelDiff, internalVel;
			ComputeStillness(g_LastPhysicsTimeStep, g_LastPhysicsTimeStep, effectiveAngVel2, effectiveVel2, nextAngVelDiff, nextVelDiff, internalVel);

			const int TEXT_SIZE = 512;
			char text[TEXT_SIZE];
			formatf(text, TEXT_SIZE, "state: %s\nasleep: %d\nmotionless: %d\neffectiveAngVel2: %f\neffectiveVel2: %f\nnextAngVelDiff: %f\nnextVelDiff: %f\ninternalVel: %f",
				m_CurrentMode == AWAKE ? "AWAKE" : m_CurrentMode == ASLEEP ? "ASLEEP" : "DONESLEEPING", 
				m_AsleepTicks, m_MotionlessTicks, effectiveAngVel2, effectiveVel2, nextAngVelDiff, nextVelDiff, internalVel);

			grcDrawLabelf(RCC_VECTOR3(m_Collider->GetPosition()), text);

			PFD_SleepHUD.End();
		}

#if PH_SLEEP_DEBUG
		if (PFD_SleepDebug.WillDraw() && m_Collider && m_Collider->GetInstance() && m_Collider->GetInstance()->IsInLevel())
		{
			PHSLEEP->RenderDebugRecord(PHLEVEL->GetHandle(m_Collider->GetInstance()), m_Collider->GetPosition());
		}
#endif // PH_SLEEP_DEBUG
	}
#endif // __PFDRAW

#if __WIN32
#pragma warning(disable:4702)
#endif

	void phSleep::Update (float timeStep) 
	{
		//	return;
		Assert(m_Collider);
		if (CheckNearlyStill(timeStep, timeStep)) 
		{
			// The object is nearly still.
			if (m_CurrentMode==AWAKE)
			{
				// The object is nearly still and awake, so increment its motionless counter.
				m_MotionlessTicks++;
				if (m_MotionlessTicks>=m_MaxMotionlessTicks)
				{
					// The object has been nearly still and awake long enough, so put it to sleep.
					SendToSleep();

#if __PFDRAW
					Vector3 PUT_TO_SLEEP_SIZE(0.15f,0.15f,0.15f);
					PFD_PutToSleep.DrawBox(RCC_MATRIX34(m_Collider->GetMatrix()),PUT_TO_SLEEP_SIZE);
#endif

				}
			}

			if (m_CurrentMode!=AWAKE)
			{
				// The object is nearly still and sleeping. This check after the "if" above allows objects that were awakened and remain
				// nearly still to go back to sleep and update as asleep on the same frame.
				// Make the collider motionless and increment the sleep counter.
				m_AsleepTicks++;
				if (m_AsleepTicks>=m_MaxAsleepTicks)
				{
					// The object has been sleeping long enough, so change from asleep to done sleeping,
					// which is a signal for the simulator to make the object inactive.
					m_CurrentMode = DONE_SLEEPING;
				}
			}
		}
		else if (m_CurrentMode==AWAKE)
		{
			// The object is moving and awake, so reset its counters.
			m_MotionlessTicks = 0;
			m_AsleepTicks = 0;
		}
		else
		{
			// The object is moving and sleeping, so wake it up.
			WakeUp();
		}
	}


	/////////////////////////////////////////////////////////////////
	// debug

#if __BANK && !__SPU
	void phSleep::AddWidgets(bkBank & bank)
	{
		bank.AddSlider("MaxMotionlessTicks",&m_MaxMotionlessTicks,0,10000,1);
		bank.AddSlider("MaxAsleepTicks",&m_MaxAsleepTicks,0,10000,1);
		bank.AddSlider("VelTolerance2",&m_VelTolerance2,0.0f,1000.0f,0.0001f);
		bank.AddSlider("AngVelTolerance2",&m_AngVelTolerance2,0.0f,1000.0f,0.0001f);
		bank.AddSlider("InternalVelTolerance2",&m_InternalVelTolerance2,0.0f,1000.0f,0.0001f);
	}
#endif


#if __DEBUGLOG
	void phSleep::DebugReplay() const
	{
		diagDebugLog(diagDebugLogPhysics, 'pSVT', &m_VelTolerance2);
		diagDebugLog(diagDebugLogPhysics, 'pSAV', &m_AngVelTolerance2);
		diagDebugLog(diagDebugLogPhysics, 'pSIV', &m_InternalVelTolerance2);
		diagDebugLog(diagDebugLogPhysics, 'pSMM', &m_MaxMotionlessTicks);
		diagDebugLog(diagDebugLogPhysics, 'pSMA', &m_MaxAsleepTicks);
		diagDebugLog(diagDebugLogPhysics, 'pSCM', &m_CurrentMode);
		diagDebugLog(diagDebugLogPhysics, 'pSMT', &m_MotionlessTicks);
		diagDebugLog(diagDebugLogPhysics, 'pSAT', &m_AsleepTicks);
		diagDebugLog(diagDebugLogPhysics, 'pSAS', &m_ActiveSleeper);
	}
#endif

	/////////////////
	// RESOURCING
	////////////////


	phSleep::phSleep(class datResource &rsc)
	{
		rsc.PointerFixup(m_Collider);
	}

	/////////////////
	// static methods

	void phSleep::SetDefaultVelTolerance2 (float tolerance)
	{
		sm_DefaultVelTolerance2 = tolerance;
	}

	void phSleep::SetDefaultAngVelTolerance2 (float tolerance)
	{
		sm_DefaultAngVelTolerance2 = tolerance;
	}

	void phSleep::SetDefaultInternalVelTolerance2 (float tolerance)
	{
		sm_DefaultInternalVelTolerance2 = tolerance;
	}

	void phSleep::SetDefaultMaxMotionlessTicks (int maxTicks)
	{
		sm_DefaultMaxMotionlessTicks = maxTicks;
	}

	void phSleep::SetDefaultMaxAsleepTicks (int maxTicks)
	{
		sm_DefaultMaxAsleepTicks = maxTicks;
	}

} // namespace rage