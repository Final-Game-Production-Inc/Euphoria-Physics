//
// physics/sleep.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_SLEEP_H
#define PHYSICS_SLEEP_H

#include "diag/debuglog.h"
#include "grprofile/drawcore.h"

namespace rage {

	class bkBank;
	class phCollider;


	//=============================================================================
	// PURPOSE
	//   Control the awake/sleeping state of a phCollider by watching the motion of its collider
	// NOTES 
	//   - Any physics collider that does not have a phSleep will never be put to sleep by the physics simulator.
	//   - phSleep has multiple ways of checking for small motion over certain periods of time to decide whether
	//     a physics collider should be asleep.
	//   - Sleeping colliders are assumed by the physics simulator to be not moving.
	//   - When the SleepState is DONE_SLEEPING, that is a signal for the physics simulator to deactivate the object.
	//   <FLAG Component>
	class phSleep
	{
	public:
		// PURPOSE: sleep modes - asleep, awake or done sleeping (ready to be deactivated)
		enum SleepState {ASLEEP, AWAKE, DONE_SLEEPING};

		// PURPOSE: construct and initialize the sleep with the given collider
		// PARAMS:
		//	collider - the collider of the object for the sleep to control
		phSleep (phCollider* collider=NULL);

		// PURPOSE: resource constructor
		// PARAMS:
		//	rsc - the data resource for this sleep
		phSleep (class datResource &rsc);

		// PURPOSE: destructor
		~phSleep ()						{ }

		// PURPOSE: Set the collider pointer, set the sleep tolerances, and make this phSleep awake.
		// PARAMS:
		//	collider - phCollider for the phSleep to detect motion.
		void Init (phCollider* collider);

		// PURPOSE: Make the phSleep awake and reset the sleep test parameters.
		// For gta4 added the option to fully reset ticks counters to zero
		// Want to get this changed moved across to rage\dev
		void Reset (bool fullReset=false);

		// PURPOSE: Make the phSleep awake and reset the sleep test parameters.
		void WakeUp ();

		// PURPOSE: Ensures the phSleep will not sleep for the given number of updates
		void BlockSleepFinishing(int numUpdates);

		//PURPOSE: Make the phSleep asleep, and stop the collider from moving.
		void SendToSleep ();

		// PURPOSE: See if the collider connected with this phSleep is moving with a velocity or angular velocity beyond the tolerance limits.
		// PARAMS:
		//	timeStep - the frame time interval
		// RETURN: true if the collider is nearly still, false if it is not.
		bool CheckNearlyStill (float timeStep, float timeStepForForce);

		// PURPOSE: Advance timing parameters and see if the sleep mode should change.
		void Update (float timeStep);

		// PURPOSE: Get the sleep mode (awake, asleep or done sleeping).
		// RETURN: the current sleep mode
		SleepState GetMode () const;

		// PURPOSE: See if this object is awake.
		// RETURN: true if this object is awake, false if it is asleep or done sleeping
		bool IsAwake () const;

		// PURPOSE: See if this object is not awake (if it is asleep or done sleeping)
		// RETURN: true if this object is asleep or done sleeping, false if it is awake
		bool IsAsleep () const;

		// PURPOSE: Return the fraction, indicating how far the sleep process has completed
		// RETURN: A value between zero and one, with zero meaning awake, and one meaning asleep
		float GetPercentAsleep() const;

		// PURPOSE: Return the number of ticks indicating how far the sleep process has completed
		int GetTicksAsleep() const;

		// PURPOSE: Return the fraction, indicating how far the sleep process has completed
		// RETURN: A value between zero and one, with zero meaning just fallen asleep, and one meaning done sleeping
		float GetPercentDoneSleeping() const;

		// PURPOSE: Return the fraction, indicating how far the sleep process has completed
		// RETURN: A value between zero and one, with zero meaning awake, and one meaning done sleeping
		float GetPercentTotal() const;

		// PURPOSE: Return the number of ticks indicating how far the sleep process has completed
		int GetTicksDoneSleeping() const;

		// PURPOSE: See if this object is done sleeping
		// RETURN: true if this object is done sleeping, false if it is asleep or awake
		// NOTES: done sleeping is a signal for the simulator to deactivate the object
		bool IsDoneSleeping () const;

		// PURPOSE: See if this object is an active sleeper.
		// RETURN: true if this object is an active sleeper, false if it is not
		// NOTES: Active sleepers are not deactivated by the simulator.
		bool IsActiveSleeper () const;

		// PURPOSE: Make this object an active sleeper, or stop it from being an active sleeper.
		// PARAMS:
		//	active - whether or not this object will be an active sleeper
		// NOTES: Active sleepers are not deactivated by the simulator.
		void SetActiveSleeper (bool active=true);

		// PURPOSE: Set the maximum squared velocity magnitude for the object to be considered sleeping.
		// PARAMS:
		//	tolerance - the maximum squared velocity magnitude for the object to be considered sleeping
		void SetVelTolerance2 (float tolerance);

		// PURPOSE: Set the maximum squared angular velocity magnitude for the object to be considered sleeping.
		// PARAMS:
		//	tolerance - the maximum squared angular velocity magnitude for the object to be considered sleeping
		void SetAngVelTolerance2 (float tolerance);

		// PURPOSE: Set the maximum squared total internal motion for the object to be considered sleeping.
		// PARAMS:
		//	tolerance - the maximum squared total internal motion for the object to be considered sleeping
		// NOTES:	Internal motion is used for sleep on articulated bodies.
		void SetInternalMotionTolerance2Sum (float tolerance);

		// PURPOSE:	Set the maximum number of frames of motionlessness for the object to stay awake.
		// PARAMS:
		//	maxTicks - the maximum number of frames of motionlessness for the object to stay awake
		void SetMaxMotionlessTicks (int maxTicks);

		// PURPOSE:	Set the maximum number of frames of sleep before the object is done sleeping.
		// PARAMS:
		//	maxTicks - the maximum number of frames of sleep before the object is done sleeping
		void SetMaxAsleepTicks (int maxTicks);

#if __SPU
		// PURPOSE:	Allow spu code to fix up the collider pointer to ls and back to mm. 
		// PARAMS:  Pointer to collider in LS or MM.
		void SetColliderPtr(phCollider* collider);
#endif

#if __BANK
		void AddWidgets (bkBank& bank);				// add tunable parameters to widgets
#endif

		// PURPOSE: Set the default maximum squared velocity magnitude for objects to be considered sleeping.
		// PARAMS:
		//	tolerance - the default maximum squared velocity magnitude for objects to be considered sleeping
		static void SetDefaultVelTolerance2 (float tolerance);
		static float GetDefaultVelTolerance2 () {return sm_DefaultVelTolerance2;}

		// PURPOSE: Set the default maximum squared angular velocity magnitude for objects to be considered sleeping.
		// PARAMS:
		//	tolerance - the default maximum squared angular velocity magnitude for objects to be considered sleeping
		static void SetDefaultAngVelTolerance2 (float tolerance);
		static float GetDefaultAngVelTolerance2 () {return sm_DefaultAngVelTolerance2;}

		// PURPOSE: Set the default maximum squared total internal motion for objects to be considered sleeping.
		// PARAMS:
		//	tolerance - the default maximum squared total internal motion for objects to be considered sleeping
		// NOTES:	Internal motion is used for sleep on articulated bodies.
		static void SetDefaultInternalVelTolerance2 (float tolerance);
		static float GetDefaultInternalVelTolerance2 () {return sm_DefaultInternalVelTolerance2;}

		// PURPOSE:	Set the default maximum number of frames of motionlessness for objects to stay awake.
		// PARAMS:
		//	maxTicks - the default maximum number of frames of motionlessness for objects to stay awake
		static void SetDefaultMaxMotionlessTicks (int maxTicks);
		static int GetDefaultMaxMotionlessTicks () {return sm_DefaultMaxMotionlessTicks;}

		// PURPOSE:	Set the default maximum number of frames of sleep before objects are done sleeping.
		// PARAMS:
		//	maxTicks - the default maximum number of frames of sleep before objects are done sleeping
		static void SetDefaultMaxAsleepTicks (int maxTicks);
		static int GetDefaultMaxAsleepTicks () {return sm_DefaultMaxAsleepTicks;}

#if __PFDRAW
		void ProfileDraw() const;
#endif

#if __DEBUGLOG
		void DebugReplay() const;
#endif

	private:
		void ComputeStillness (float timeStep, float timeStepForForce, float& effectiveAngVel2, float& effectiveVel2, float& nextAngVelDiff, float& nextVelDiff, float& internalVel) const;

		// PURPOSE: the default maximum squared velocity magnitude for objects to be considered sleeping
		static float sm_DefaultVelTolerance2;

		// PURPOSE: the default maximum squared angular velocity magnitude for objects to be considered sleeping
		static float sm_DefaultAngVelTolerance2;

		// PURPOSE: the default maximum squared total internal motion for objects to be considered sleeping
		static float sm_DefaultInternalVelTolerance2;

		// PURPOSE: the default maximum number of frames of motionlessness for objects to stay awake
		static int sm_DefaultMaxMotionlessTicks;

		// PURPOSE: the default maximum number of frames of sleep before objects are done sleeping
		static int sm_DefaultMaxAsleepTicks;

		// PURPOSE:	the maximum squared velocity magnitude for the object to be considered sleeping
		float m_VelTolerance2;

		// PURPOSE:	the maximum squared angular velocity magnitude for the object to be considered sleeping
		float m_AngVelTolerance2;

		// PURPOSE:	the maximum squared total internal motion for the object to be considered sleeping
		float m_InternalVelTolerance2;

		// PURPOSE:	the maximum number of frames of motionlessness for the object to stay awake
		int m_MaxMotionlessTicks;

		// PURPOSE:	the maximum number of frames of sleep before the object is done sleeping
		int m_MaxAsleepTicks;

		// PURPOSE:	the sleep mode (awake, asleep or done sleeping)
		SleepState m_CurrentMode;

		// PURPOSE:	the collider for this sleep
		phCollider* m_Collider;

		// PURPOSE:	the number of continuous frames that this object has been nearly motionless
		int m_MotionlessTicks;

		// PURPOSE:	the number of continuous frames that this object has been asleep
		int m_AsleepTicks;

		// PURPOSE:	whether or not the simulator will deactivate this object when it is done sleeping
		bool m_ActiveSleeper;
	} ;


	//=============================================================================
	// Implementations

	inline phSleep::SleepState phSleep::GetMode () const
	{
		return m_CurrentMode;
	}

	inline void phSleep::SetActiveSleeper (bool active)
	{
		m_ActiveSleeper = active;
	}

	inline bool phSleep::IsAwake () const
	{
		return m_CurrentMode==AWAKE;
	}

	inline bool phSleep::IsAsleep () const
	{
		return (m_CurrentMode==ASLEEP || m_CurrentMode==DONE_SLEEPING);
	}

	inline bool phSleep::IsDoneSleeping () const
	{
		return m_CurrentMode==DONE_SLEEPING;
	}

	inline float phSleep::GetPercentAsleep() const
	{
		return float(m_MotionlessTicks) / float(m_MaxMotionlessTicks);
	}

	inline int phSleep::GetTicksAsleep() const
	{
		return m_MotionlessTicks;
	}

	inline float phSleep::GetPercentDoneSleeping() const
	{
		return float(m_AsleepTicks) / float(m_MaxAsleepTicks);
	}

	inline float phSleep::GetPercentTotal() const
	{
		return float(m_AsleepTicks + m_MotionlessTicks) / float(m_MaxAsleepTicks + m_MaxMotionlessTicks);
	}

	inline int phSleep::GetTicksDoneSleeping() const
	{
		return m_AsleepTicks;
	}

	inline bool phSleep::IsActiveSleeper () const
	{
		return m_ActiveSleeper;
	}

	inline void phSleep::SetVelTolerance2 (float tolerance)
	{
		m_VelTolerance2 = tolerance;
	}

	inline void phSleep::SetAngVelTolerance2 (float tolerance)
	{
		m_AngVelTolerance2 = tolerance;
	}

	inline void phSleep::SetInternalMotionTolerance2Sum (float tolerance)
	{
		m_InternalVelTolerance2 = tolerance;
	}

	inline void phSleep::SetMaxMotionlessTicks (int maxTicks)
	{
		m_MaxMotionlessTicks = maxTicks;
	}

	inline void phSleep::SetMaxAsleepTicks (int maxTicks)
	{
		m_MaxAsleepTicks = maxTicks;
	}

#if __SPU
	inline void phSleep::SetColliderPtr(phCollider* collider)
	{
		m_Collider = collider;
	}
#endif

} // namespace rage

#endif