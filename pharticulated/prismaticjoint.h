//
// pharticulated/prismaticjoint.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//
// Implements a sliding joint

#ifndef PHARTICULATED_PRISMATICJOINT_TYPE_H
#define PHARTICULATED_PRISMATICJOINT_TYPE_H

#include "data/struct.h"
#include "data/safestruct.h"
#include "bodypart.h"
#include "joint.h"


namespace rage {

	class phArticulatedBody;

	class phPrismaticJointType : public phJointType
	{
	public:

#if __DECLARESTRUCT
		void DeclareStruct(datTypeStruct &s);
#endif

		phPrismaticJointType();
		~phPrismaticJointType() {}
		phPrismaticJointType(datResource &rsc);

		virtual phJointType* Clone();

		float m_HardLimitMin,m_HardLimitMax;	// joint value limits, beyond which motion is clamped
#ifdef USE_SOFT_LIMITS
		float m_SoftLimitMin,m_SoftLimitMax;	// soft limits, beyond which there is a restoring force
#else
		datPadding<8> m_Pad;
#endif

		// PURPOSE: the upper and lower limits on the muscle force that will be applied, when muscle driving is active
		float m_MaxMuscleForce,m_MinMuscleForce;

		datPadding<8> m_Pad0;
	};

	class phPrismaticJoint : public phJoint
	{
		friend class phJoint;

	public:
		phPrismaticJoint();
		~phPrismaticJoint() {}
		phPrismaticJoint(phPrismaticJointType *jointType);
		phPrismaticJoint(datResource& ) {};

		void GetAngleLimits (float& minTranslation, float& maxTranslation);

		float ComputeCurrentTranslation(phArticulatedBody *body);		// Returns the value of the prismatic joint (distance extended)
		float GetComputedTranslation() const;
		Vector3& GetTranslationAxis();
		const Vector3 &GetTranslationAxis() const;
		float ComputeVelocity(phArticulatedBody *body);				// Returns rate of change of joint value.
		float GetComputedVelocity() const;

		void ApplyForce( phArticulatedBody *body, float force, Vec::V3Param128 timeStep ) const;
		void ApplyImpulse( phArticulatedBody *body, ScalarV_In impulseMag ) const;

		// CheckJointLimitStatus calls ComputeCurrentAngle.  If joint limit
		//	is exceeded, it also calls ComputeVelocity().  It returns
		//  the direction in which the joint limit is exceeded and 
		//	data on whether and how much the joint limit is exceeded.
		bool CheckJointLimitStatus (phArticulatedBody *body, int *dir=NULL, float* excessAngleSoft=NULL, float* translationalVelocity=NULL);

		void MatchChildToParentPositionAndVelocity(phArticulatedBody *body);

		// <COMBINE phJoint::GetJointLimitResponse>
		float GetJointLimitResponse (phArticulatedBody *body, int limitDirectionIndex);

		// <COMBINE phJoint::ComputeNumHardJointLimitDofs>
		int ComputeNumHardJointLimitDofs (phArticulatedBody *body);

		// For 0 <= j < numJointsToEnforce, get relevant info about the joint limit status.
		//		the "jointLimitID" also called a "detailCode" identifies extra information
		//		about the joint limit in a way that the joint can use effectively.
		// Uses the prestored values calculated by CheckJointLimitStatus
		void GetJointLimitDetailedInfo (phArticulatedBody *body, int limitNum, int& jointLimitID, float& dofExcess, float& response);

		// Applies a impulse or torque to the axis corresponding to the joint limit.
		void ApplyJointImpulse(phArticulatedBody *body, int jointLimitID, ScalarV_In impulse );
		void ApplyJointPush( phArticulatedBody *body, int jointLimitID, ScalarV_In push );
		void ApplyJointImpulseAndPush( phArticulatedBody *body, int jointLimitID, ScalarV_In impulse, ScalarV_In push );
		void GetJointLimitAxis( phArticulatedBody *body, int jointLimitID, Vector3& axis, Vector3& position ) const;
		void GetJointLimitUnitImpulse( int jointLimitID, phPhaseSpaceVector& unitImpulseSpatial ) const;

		void Freeze ();

		phPrismaticJointType & GetPrismaticType() { return static_cast<phPrismaticJointType&>(*m_Type); }

		// Set the axis position (translation direction) using global coordinates.
		// First place the links in the desired (rest) position, then call SetAxis
		// In this rest position, the joint value is ZERO.
		void SetAxis( phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir);
		// MinAngle and MaxAngle measured from the current configuration
		void SetAxis( phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir, float MinAngle, float MaxAngle );
		// The third version of SetAxis sets MinAngle = -MaxAngle.
		void SetAxis( phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir, float MaxAngle );

		// Set the minimum and maximum angle limits for the joint
		void SetAngleLimits (float minTranslation, float maxTranslation);
#ifdef USE_SOFT_LIMITS
		void SetSoftAngleLimits (float softMinTranslation, float softMaxTranslation);
#endif
		void GetAngleLimits (float& minTranslation, float& maxTranslation) const;

#ifdef USE_SOFT_LIMITS
		// <COMBINE phJoint::EnforceSoftJointLimits>
		void EnforceSoftJointLimits (float timeStep);
#endif
		// PURPOSE: Set the target position for muscle forces to drive toward.
		// PARAMS:
		void SetMuscleTargetPosition (float targetPosition);

		// PURPOSE: Set the target speed for muscle forces to drive toward.
		// PARAMS:
		//	targetAngle - the target speed for muscle forces to drive toward
		void SetMuscleTargetSpeed (float targetSpeed);

		// PURPOSE: Set the strength for muscle forces.
		// PARAMS:
		//	strength - linear spring constant for muscle forces to drive toward the target position
		void SetMusclePositionStrength (float strength);

		// PURPOSE: Set the target speed strength for muscle forces.
		// PARAMS:
		//	speedStrength - linear spring constant for muscle forces to drive toward the target speed
		void SetMuscleSpeedStrength (float speedStrength);

		// PURPOSE: Set the maximum force that muscles can apply.
		// PARAMS:
		//	maxTorque - the maximum force that muscles can apply
		void SetMaxMuscleForce (float maxForce);

		// PURPOSE: Set the minimum force that muscles can apply.
		// PARAMS:
		//	maxTorque - the minimum force that muscles can apply
		void SetMinMuscleForce (float minForce);

		// PURPOSE: Set the maximum and minimum forces that muscles can apply.
		// PARAMS:
		//	minForce - the minimum force that muscles can apply
		//	maxForce - the maximum force that muscles can apply
		void SetMinAndMaxMuscleForce (float minForce, float maxForce);

		// PURPOSE: Set the maximum and minimum force that muscles can apply.
		// PARAMS:
		//	maxForceMag - the maximum magnitude of the force that muscles can apply
		void SetMinAndMaxMuscleForce (float maxForceMag);

		// PURPOSE: Get the muscle target position.
		// RETURN: the muscle target position
		float GetMuscleTargetPosition () const;

		// PURPOSE: Get the muscle target speed.
		// RETURN: the muscle target speed
		float GetMuscleTargetSpeed () const;

		// PURPOSE: Get the muscle position strength.
		// RETURN: the linear spring constant for muscle forces to drive toward the target position
		float GetMusclePositionStrength () const;

		// PURPOSE: Get the muscle speed strength.
		// RETURN: the linear spring constant for muscle forces to drive toward the target speed
		float GetMuscleSpeedStrength () const;

		// PURPOSE: Get the maximum muscle force.
		// RETURN: the maximum force that the muscles can apply
		float GetMaxMuscleForce ();

		// PURPOSE: Get the minimum muscle force.
		// RETURN: the minimum force that the muscles can apply
		float GetMinMuscleForce ();

		// <COMBINE phJoint::ComputeAndApplyMuscleTorques>
		void ComputeAndApplyMuscleTorques (phArticulatedBody *body, float timeStep);

		// <COMBINE phJoint1Dof::Copy>
		void Copy (const phPrismaticJoint& original);

		void* GetCoreEnd() { return &m_MusclePositionStrength; }

	private:

		inline void TransformByASupdown( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const;
		inline void TransformByASdownup( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const;
		inline void TransformByASdownupAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const;
		inline void TransformByASupdownAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const;
		inline void TransformByAupdown( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const;
		inline void TransformByAupdownTwice( const phPhaseSpaceVector& from, phPhaseSpaceVector& to, const phPhaseSpaceVector& from2, phPhaseSpaceVector& to2 ) const;
		inline void TransformByAdownup( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const;
		inline void TransformByAdownupTwice( const phPhaseSpaceVector& from, phPhaseSpaceVector& to, const phPhaseSpaceVector& from2, phPhaseSpaceVector& to2 ) const;
		inline void TransformByAdownupAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const;
		inline void TransformByAupdownAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const;

		void PrecomputeAupdown();
		void PrecomputeAdownup(phArticulatedBody *body);

		void TransformInertiaByASdownup( phArticulatedBodyInertia& dest ) const;
		void TransformInertiaByASupdown( phArticulatedBodyInertia& dest ) const;

		Vector3 GetsSIs_downupV() const;
		Vector3 GetsSIsInvStiffness_downupV() const;
		Vector3 GetsSIsInvStiffness_updownV() const;
		void SetsSIs_downupV(Vector3::Param val);
		void SetsSIsInvStiffness_downupV(Vector3::Param val);
		void SetsSIsInvStiffness_updownV(Vector3::Param val);

	protected:

		phPhaseSpaceVector JointAxis;

		// Values used to help computation of As for articulated inertia
		phPhaseSpaceVector IAs_downup;		// I^A_{child} s
		phPhaseSpaceVector IAs_updown;		// I^A_{parent} s
		float sSIsInvStiffness_downup;			// (s^S I^A_{child} s + gamma)^{-1}
		float sSIsInvStiffness_updown;			// (s^S I^A_{parent} s + gamma)^{-1}

		// Values updated every frame
		float CurrentTranslation;			// Set only if ComputeCurrentAngle has been called
		float TranslationalVelocity;		// In radians per second, around the joint axis
		int JointLimitDir;					// == 0 if limit not exceeded, +1 if too large, -1 if too small.

		// ---

		// PURPOSE: the linear spring and damping constants for the muscle forces, when muscle driving is active
		float m_MusclePositionStrength,m_MuscleSpeedStrength;

		// PURPOSE: the target position and speed for this prismatic joint, when muscle driving is active
		float m_MuscleTargetPosition,m_MuscleTargetSpeed;

		// Values used to help computation of As for articulated inertia
		float sSIs_downup;						// Uses for "Precompute" inertia calculuations
	};

	// ****************************************************************************
	// Inlined member functions for phPrismaticJoint
	// ****************************************************************************


	inline void phPrismaticJoint::GetAngleLimits (float& minAngle, float& maxAngle) 
	{
		minAngle = GetPrismaticType().m_HardLimitMin;
		maxAngle = GetPrismaticType().m_HardLimitMax;
	}

	__forceinline void phPrismaticJoint::TransformByAdownup( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		ScalarV factor = VECTOR3_TO_SCALARV(IAs_downup^from);
		factor = Scale( factor, ScalarVFromF32(sSIsInvStiffness_downup) );
		to = from;
		to.trans = AddScaled( to.trans, JointAxis.trans, Vec3V(-factor) );
	}

	__forceinline void phPrismaticJoint::TransformByAdownupTwice( const phPhaseSpaceVector& from, phPhaseSpaceVector& to, const phPhaseSpaceVector& from2, phPhaseSpaceVector& to2 ) const
	{
		{
			ScalarV factor = VECTOR3_TO_SCALARV(IAs_downup^from);
			factor = Scale( factor, ScalarVFromF32(sSIsInvStiffness_downup) );
			to = from;
			to.trans = AddScaled( to.trans, JointAxis.trans, Vec3V(-factor) );
		}
		{
			ScalarV factor2 = VECTOR3_TO_SCALARV(IAs_downup^from2);
			factor2 = Scale( factor2, ScalarVFromF32(sSIsInvStiffness_downup) );
			to2 = from2;
			to2.trans = AddScaled( to2.trans, JointAxis.trans, Vec3V(-factor2) );
		}
	}

	__forceinline void phPrismaticJoint::TransformByAdownupAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		ScalarV factor = VECTOR3_TO_SCALARV(IAs_downup^from);
		factor = Scale( factor, ScalarVFromF32(sSIsInvStiffness_downup) );
		to += from;
		to.trans = AddScaled( to.trans, JointAxis.trans, Vec3V(-factor) );
	}

	__forceinline void phPrismaticJoint::TransformByAupdown( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		ScalarV factor = VECTOR3_TO_SCALARV(IAs_updown^from);
		factor = Scale( factor, ScalarVFromF32(sSIsInvStiffness_updown) );
		to = from;
		to.trans = AddScaled( to.trans, JointAxis.trans, Vec3V(-factor) );
	}

	__forceinline void phPrismaticJoint::TransformByAupdownTwice( const phPhaseSpaceVector& from, phPhaseSpaceVector& to, const phPhaseSpaceVector& from2, phPhaseSpaceVector& to2 ) const
	{
		{
			ScalarV factor = VECTOR3_TO_SCALARV(IAs_updown^from);
			factor = Scale( factor, ScalarVFromF32(sSIsInvStiffness_updown) );
			to = from;
			to.trans = AddScaled( to.trans, JointAxis.trans, Vec3V(-factor) );
		}
		{
			ScalarV factor2 = VECTOR3_TO_SCALARV(IAs_updown^from2);
			factor2 = Scale( factor2, ScalarVFromF32(sSIsInvStiffness_updown) );
			to2 = from2;
			to2.trans = AddScaled( to2.trans, JointAxis.trans, Vec3V(-factor2) );
		}
	}

	__forceinline void phPrismaticJoint::TransformByASdownup( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		ScalarV factor = Scale( Dot(JointAxis.trans, from.omega), ScalarVFromF32(sSIsInvStiffness_downup) );
		to.AddScaled( from, IAs_downup, Vector3((-factor).GetIntrin128()) );
	}

	__forceinline void phPrismaticJoint::TransformByASdownupAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		ScalarV factor = Scale( Dot(JointAxis.trans, from.omega), ScalarVFromF32(sSIsInvStiffness_downup) );
		to.Add(from);
		to.AddScaled( IAs_downup, Vector3((-factor).GetIntrin128()) );
	}

	__forceinline void phPrismaticJoint::TransformByASupdown( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		ScalarV factor = Scale( Dot(JointAxis.trans, from.omega), ScalarVFromF32(sSIsInvStiffness_updown) );
		to.AddScaled( from, IAs_updown, Vector3((-factor).GetIntrin128()) );
	}

	__forceinline void phPrismaticJoint::TransformByASupdownAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		ScalarV factor = Scale( Dot(JointAxis.trans, from.omega), ScalarVFromF32(sSIsInvStiffness_updown) );
		to.Add(from);
		to.AddScaled( IAs_updown, Vector3((-factor).GetIntrin128()) );
	}

	__forceinline void phPrismaticJoint::TransformByAupdownAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		ScalarV factor = VECTOR3_TO_SCALARV(IAs_updown^from);
		factor = Scale( factor, ScalarVFromF32(sSIsInvStiffness_updown) );
		to += from;
		to.trans = AddScaled( to.trans, JointAxis.trans, Vec3V(-factor) );
	}

	inline float phPrismaticJoint::GetComputedTranslation() const 
	{
		return CurrentTranslation; 
	}

	inline Vector3& phPrismaticJoint::GetTranslationAxis() 
	{ 
		return RC_VECTOR3(JointAxis.trans); 
	}

	inline const Vector3& phPrismaticJoint::GetTranslationAxis() const 
	{ 
		return RCC_VECTOR3(JointAxis.trans); 
	}

	inline float phPrismaticJoint::GetComputedVelocity() const 
	{ 
		return TranslationalVelocity; 
	}


	// ****************************************************************************
	// Inlined member functions for phPrismaticJoint
	// ****************************************************************************

	inline phPrismaticJointType::phPrismaticJointType (datResource &rsc) : phJointType(rsc)
	{
	}

	inline phPrismaticJoint::phPrismaticJoint ()
	{
		// Set the pointer to type data
		phPrismaticJointType *jointType = rage_new phPrismaticJointType;
		m_Type = static_cast<phJointType*>(jointType);
#ifdef USE_SOFT_LIMITS
		m_Type->m_InvTimeToSeparateSoft = 4.0f;
#endif

		m_Type->m_JointType = phJoint::PRISM_JNT;

		m_Stiffness = 0.0f;
		m_MusclePositionStrength = DEFAULT_MUSCLE_ANGLE_STRENGTH;
		m_MuscleSpeedStrength = DEFAULT_MUSCLE_SPEED_STRENGTH;
	}

	inline phPrismaticJoint::phPrismaticJoint (phPrismaticJointType *jointType)
	{
		// Set the pointer to type data
		m_Type = static_cast<phJointType*>(jointType);
		m_Stiffness = 0.0f;
		m_MusclePositionStrength = DEFAULT_MUSCLE_ANGLE_STRENGTH;
		m_MuscleSpeedStrength = DEFAULT_MUSCLE_SPEED_STRENGTH;
	}

	inline void phPrismaticJoint::SetAxis( phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir, 
		float maxAngle )
	{
		SetAxis( body, axisPosition, axisDir, -maxAngle, maxAngle );
	}

	inline void phPrismaticJoint::SetMuscleTargetPosition (float targetPosition)
	{
		m_MuscleTargetPosition = targetPosition;
	}

	inline void phPrismaticJoint::SetMuscleTargetSpeed (float targetSpeed)
	{
		m_MuscleTargetSpeed = targetSpeed;
	}

	inline void phPrismaticJoint::SetMusclePositionStrength (float strength)
	{
		m_MusclePositionStrength = strength;
	}

	inline void phPrismaticJoint::SetMuscleSpeedStrength (float speedStrength)
	{
		m_MuscleSpeedStrength = speedStrength;
	}

	inline void phPrismaticJoint::SetMaxMuscleForce (float maxForce)
	{
		GetPrismaticType().m_MaxMuscleForce = maxForce;
	}

	inline void phPrismaticJoint::SetMinMuscleForce (float minForce)
	{
		GetPrismaticType().m_MinMuscleForce = minForce;
	}

	inline void phPrismaticJoint::SetMinAndMaxMuscleForce (float minForce, float maxForce)
	{
		GetPrismaticType().m_MinMuscleForce = minForce;
		GetPrismaticType().m_MaxMuscleForce = maxForce;
	}

	inline void phPrismaticJoint::SetMinAndMaxMuscleForce (float maxForceMag)
	{
		GetPrismaticType().m_MinMuscleForce = -maxForceMag;
		GetPrismaticType().m_MaxMuscleForce = maxForceMag;
	}

	inline float phPrismaticJoint::GetMuscleTargetPosition () const
	{
		return m_MuscleTargetPosition;
	}

	inline float phPrismaticJoint::GetMuscleTargetSpeed () const
	{
		return m_MuscleTargetSpeed;
	}

	inline float phPrismaticJoint::GetMusclePositionStrength () const
	{
		return m_MusclePositionStrength;
	}

	inline float phPrismaticJoint::GetMuscleSpeedStrength () const
	{
		return m_MuscleSpeedStrength;
	}

	inline float phPrismaticJoint::GetMaxMuscleForce ()
	{
		return GetPrismaticType().m_MaxMuscleForce;
	}

	inline float phPrismaticJoint::GetMinMuscleForce ()
	{
		return GetPrismaticType().m_MinMuscleForce;
	}

	inline Vector3 phPrismaticJoint::GetsSIs_downupV() const
	{
		Vector3 splatted;
		splatted.SplatX(*(Vector3*)&sSIs_downup);
		return splatted;
	}

	inline Vector3 phPrismaticJoint::GetsSIsInvStiffness_downupV() const
	{
		Vector3 splatted;
		splatted.SplatY(*(Vector3*)&sSIs_downup);
		return splatted;
	}

	inline Vector3 phPrismaticJoint::GetsSIsInvStiffness_updownV() const
	{
		Vector3 splatted;
		splatted.SplatZ(*(Vector3*)&sSIs_downup);
		return splatted;
	}

	inline void phPrismaticJoint::SetsSIs_downupV(Vector3::Param val)
	{
		Vector3 splatted = *(Vector3*)&sSIs_downup;
		splatted.And(VEC3_ANDX);
		Vector3 valUnSplat = val;
		valUnSplat.And(VEC3_MASKX);
		splatted.Or(valUnSplat);
		*(Vector3*)&sSIs_downup = splatted;
	}

	inline void phPrismaticJoint::SetsSIsInvStiffness_downupV(Vector3::Param val)
	{
		Vector3 splatted = *(Vector3*)&sSIs_downup;
		splatted.And(VEC3_ANDY);
		Vector3 valUnSplat = val;
		valUnSplat.And(VEC3_MASKY);
		splatted.Or(valUnSplat);
		*(Vector3*)&sSIs_downup = splatted;
	}

	inline void phPrismaticJoint::SetsSIsInvStiffness_updownV(Vector3::Param val)
	{
		Vector3 splatted = *(Vector3*)&sSIs_downup;
		splatted.And(VEC3_ANDZ);
		Vector3 valUnSplat = val;
		valUnSplat.And(VEC3_MASKZ);
		splatted.Or(valUnSplat);
		*(Vector3*)&sSIs_downup = splatted;
	}

	__forceinline void phPrismaticJoint::TransformInertiaByASdownup( phArticulatedBodyInertia& dest ) const
	{
		dest.SetOuterProductScaled( IAs_downup, sSIsInvStiffness_downup );
		dest.SubtractFrom(m_DownInertia);
	}

	__forceinline void phPrismaticJoint::TransformInertiaByASupdown( phArticulatedBodyInertia& dest ) const
	{
		dest.SetOuterProductScaled( IAs_updown, sSIsInvStiffness_updown );
		dest.SubtractFrom(m_UpInertia);
	}

} // namespace rage

#endif    // PHARTICULATED_PRISMATICJOINT_TYPE_H
