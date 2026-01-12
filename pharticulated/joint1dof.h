//
// pharticulated/joint1dof.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//
// Implements a 1-DOF rotational joint, an "elbow joint".

#ifndef PHARTICULATED_JOINT1DOF_TYPE_H
#define PHARTICULATED_JOINT1DOF_TYPE_H

#include "bodypart.h"
#include "joint.h"

#include "data/struct.h"
#include "data/safestruct.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

namespace rage {


	class phArticulatedBody;

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	class phJoint1DofType : public phJointType
	{
		friend class phJoint1Dof;
		friend class fragType;

	public:

#if __DECLARESTRUCT
		void DeclareStruct(datTypeStruct &s);
#endif

		phJoint1DofType();
		~phJoint1DofType() {}
		phJoint1DofType(datResource &rsc);

		virtual phJointType* Clone();

	protected:
		// PURPOSE: the minimum joint angle limit, beyond which motion is clamped
		float m_HardAngleMin;

		// PURPOSE: the maximum joint angle limit, beyond which motion is clamped
		float m_HardAngleMax;

		// ---

		// PURPOSE: the upper and lower limits on the muscle torque that will be applied, when muscle driving is active
		float m_MaxMuscleTorque,m_MinMuscleTorque;
#ifdef USE_SOFT_LIMITS
		// PURPOSE: the strength with which soft limits are enforced
		float m_SoftLimitStrength;
#endif
	};


	class phJoint1Dof : public phJoint 
	{
		friend class phArticulatedBody;
		friend class phJoint;

	public:

		phJoint1Dof();
		~phJoint1Dof() {}
		phJoint1Dof(phJoint1DofType *jointType);
		phJoint1Dof(datResource& ) {};

		// As presently implemented this always returns an angle in the
		//		[-PI, PI].  This might need to be improved.
		// GetComputedAngle should be called only if ComputeCurrentAngle has
		//		been invoked earlier to compute the value.
		ScalarV_Out ComputeCurrentAngle(phArticulatedBody *body);		// Finds the current angle, stores it in the 'CurrentAngle' member
		float GetComputedAngle() const { return CurrentAngle; }
		Vector3& GetRotationAxis() { return RC_VECTOR3(m_JointAxis.omega); }
		const Vector3& GetRotationAxis() const { return RCC_VECTOR3(m_JointAxis.omega); }
		// Rotational Velocity means the rate of change of joint angle.
		//	The sign of the rotational velocity is given by the righthand rule.
		//	The "Get" function only works if the "Compute" function has already been called.
		ScalarV_Out ComputeRotationalVelocity(phArticulatedBody *body) const;

		float GetAngle (phArticulatedBody *body) { ComputeCurrentAngle(body); return CurrentAngle; }
		float GetAngularSpeed (phArticulatedBody *body) const { return ComputeRotationalVelocity(body).Getf(); }

		float ComputeHardLimitExcess (phArticulatedBody *body);

		float ResponseToUnitTorque(phArticulatedBody *body) const;		// Change in rotation rate in response to a unit torque

		// Applies a torque around the joint axis
		void ApplyTorque(phArticulatedBody *body, const float& torque, Vec::V3Param128 timeStep) const;		

		// <COMBINE phJoint::ApplyAngImpulse>
		void ApplyAngImpulse (phArticulatedBody *body, ScalarV_In angImpulseMag) const;

		void MatchChildToParentPositionAndVelocity(phArticulatedBody *body);

		// PURPOSE:
		//   Set the child link to a given rotation around the rotation axis based on the parent link's matrix
		// PARAMS:
		//   body - articulated body this joint is a part of
		//   rotation - angle in radians the child link will be rotated about the rotation axis from the initial joint matrix
		// NOTES: This will also zero out the relative velocity between the child and parent links
		void MatchChildToParentPositionAndVelocity(phArticulatedBody* body, ScalarV_In rotation);

		// <COMBINE phJoint::GetJointLimitResponse>
		float GetJointLimitResponse (phArticulatedBody *body, int limitDirectionIndex);

		void GetAngleLimits (float& minAngle, float& maxAngle) const;

		// <COMBINE phJoint::ComputeNumHardJointLimitDofs>
		int ComputeNumHardJointLimitDofs (phArticulatedBody *body);

		// For 0 <= j < numJointsToEnforce, get relevant info about the joint limit status.
		//		the "jointLimitID" also called a "detailCode" identifies extra information
		//		about the joint limit in a way that the joint can use effectively.
		// Uses the prestored values calculated by CheckJointLimitStatus
		void GetJointLimitDetailedInfo (phArticulatedBody *body, int limitNum, int& jointLimitID, float& dofExcess, float& response);

		// Calculate the responses to a unit "torque" or "impulse" to the given joint limit
		void PrecalcResponsesToJointImpulse( phArticulatedBody* theBody, int jointNum, int jointLimitID );

		// Get response to the joint limits's "Rate of Change" in response to a previously
		//		Precalc's impulse/torque.  The returned "response" is the acceleration or
		//		the delta-velocity to the joint limit value.
		float GetPrecalcJointLimitResponse( phArticulatedBody* theBody, int jointNum, int jointLimitID );

		// Applies a impulse or torque to the axis corresponding to the joint limit.
		void ApplyJointImpulse( phArticulatedBody *body, int jointLimitID, ScalarV_In impulse );
		void ApplyJointPush( phArticulatedBody *body, int jointLimitID, ScalarV_In push );
		void ApplyJointImpulseAndPush( phArticulatedBody *body, int jointLimitID, ScalarV_In impulse, ScalarV_In push );
		void GetJointLimitAxis( phArticulatedBody *body, int jointLimitID, Vector3& axis, Vector3& position ) const;
		void GetJointLimitUnitImpulse( int jointLimitID, phPhaseSpaceVector& unitImpulseSpatial ) const;

		void GetPrecalcJointLimitAngleResponse( phArticulatedBody& theBody, int jointNum, int jointLimitID, Vector3& threeResponses );

		void Freeze();

		// PURPOSE: Set the joint's axis position and direction.
		// PARAMS:
		//	axisPosition - the position of the joint axis in world coordinates
		//	axisUnitDir - the unit direction of the joint axis in world coordinates
		//	parentLink - the parent (upper) body part connecting this joint
		//	childLink - the child (lower) body part connecting this joint
		// NOTES:	Before calling this, the connecting body parts should be in the desired rest position and orientation.
		void SetAxis (phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisUnitDir);

		// PURPOSE: Set the joint's axis position and direction.
		// PARAMS:
		//	axisPosition - the position of the joint axis in world coordinates
		//	axisUnitDir - the unit direction of the joint axis in world coordinates
		//	parentLink - the parent (upper) body part connecting this joint
		//	childLink - the child (lower) body part connecting this joint
		//	maxAngle - the maximum angle of the joint (thie minimum is set to the negative of the maximum)
		// NOTES:	Before calling this, the connecting body parts should be in the desired rest position and orientation.
		void SetAxis (phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisUnitDir, float maxAngle);

		// PURPOSE: Set the joint's axis position and direction.
		// PARAMS:
		//	axisPosition - the position of the joint axis in world coordinates
		//	axisUnitDir - the unit direction of the joint axis in world coordinates
		//	parentLink - the parent (upper) body part connecting this joint
		//	childLink - the child (lower) body part connecting this joint
		//	minAngle - the minimum angle of the joint
		//	maxAngle - the maximum angle of the joint
		// NOTES:	Before calling this, the connecting body parts should be in the desired rest position and orientation.
		void SetAxis (phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisUnitDir, float minAngle, float maxAngle);

		// Set the minimum and maximum angle limits for the joint
		void SetAngleLimits (float minAngle, float maxAngle, bool adjustOnly = false);

		void ResetLimitAdjustments() { m_HardAngleMinAdjust = m_HardAngleMinAdjust = 0.0f; }

#ifdef USE_SOFT_LIMITS
		void SetSoftAngleLimits (float softMinAngle, float softMaxAngle);

		// PURPOSE: Set the strength with which soft limits are enforced
		void SetSoftLimitStrength (float softLimitStrength);

		// <COMBINE phJoint::EnforceSoftJointLimits>
		void EnforceSoftJointLimits (float timeStep);
#endif
		// PURPOSE: Set the target angle for muscle torques to drive toward.
		// PARAMS:
		//	targetAngle - the target angle for muscle torques to drive toward
		void SetMuscleTargetAngle (ScalarV_In targetAngle);
		void SetMuscleTargetAngle (float targetAngle);

		// PURPOSE: Set the target angular speed for muscle torques to drive toward.
		// PARAMS:
		//	targetAngle - the target angular speed for muscle torques to drive toward
		void SetMuscleTargetSpeed (ScalarV_In targetSpeed);
		void SetMuscleTargetSpeed (float targetSpeed);

		// PURPOSE: Set the target angle strength for muscle torques.
		// PARAMS:
		//	angleStrength - linear spring constant for muscle torques to drive toward the target angle
		void SetMuscleAngleStrength (float angleStrength);

		// PURPOSE: Set the target speed strength for muscle torques.
		// PARAMS:
		//	speedStrength - linear spring constant for muscle torques to drive toward the target speed
		void SetMuscleSpeedStrength (float speedStrength);

		// PURPOSE: Set the maximum torque that muscles can apply.
		// PARAMS:
		//	maxTorque - the maximum torque that muscles can apply
		void SetMaxMuscleTorque (float maxTorque);

		// PURPOSE: Set the minimum torque that muscles can apply.
		// PARAMS:
		//	maxTorque - the minimum torque that muscles can apply
		void SetMinMuscleTorque (float minTorque);

		// PURPOSE: Set the maximum and minimum torques that muscles can apply.
		// PARAMS:
		//	minTorque - the minimum torque that muscles can apply
		//	maxTorque - the maximum torque that muscles can apply
		void SetMinAndMaxMuscleTorque (float minTorque, float maxTorque);

		// PURPOSE: Set the maximum and minimum torque that muscles can apply.
		// PARAMS:
		//	maxTorqueMag - the maximum magnitude of the torque that muscles can apply
		void SetMinAndMaxMuscleTorque (float maxTorqueMag);

		// PURPOSE: Get the muscle target angle.
		// RETURN: the muscle target angle
		float GetMuscleTargetAngle () const;

		// PURPOSE: Get the muscle target speed.
		// RETURN: the muscle target speed
		float GetMuscleTargetSpeed () const;

		// PURPOSE: Get the muscle angle strength.
		// RETURN: the linear spring constant for muscle torques to drive toward the target angle
		float GetMuscleAngleStrength () const;

		// PURPOSE: Get the muscle speed strength.
		// RETURN: the linear spring constant for muscle torques to drive toward the target speed
		float GetMuscleSpeedStrength () const;

		// PURPOSE: Get the maximum muscle torque.
		// RETURN: the maximum torque that the muscles can apply
		float GetMaxMuscleTorque ();

		// PURPOSE: Get the minimum muscle torque.
		// RETURN: the minimum torque that the muscles can apply
		float GetMinMuscleTorque ();

		// PURPOSE: Compute the torque on this joint from its muscles.
		// PARAMS:
		//	timeStep - the frame time interval
		// RETURN: the torque on this joint from its muscles
		ScalarV_Out ComputeMuscleTorque (phArticulatedBody *body, ScalarV_In timeStep);

		// <COMBINE phJoint::ComputeAndApplyMuscleTorques>
		void ComputeAndApplyMuscleTorques (phArticulatedBody *body ,float timeStep);

		phJoint1DofType & Get1DofType() { return static_cast<phJoint1DofType&>(*m_Type); }

#if PHARTICULATED_DEBUG_SERIALIZE
		void DebugSerialize(fiAsciiTokenizer& t);
#endif

	private:
		void PrecomputeAupdown();
		void PrecomputeAdownup(phArticulatedBody *body);

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

		ScalarV_Out GetsSIsInvStiffness_downupV() const;
		ScalarV_Out GetsSIsInvStiffness_updownV() const;
		void SetsSIsInvStiffness_downupV(ScalarV_In val);
		void SetsSIsInvStiffness_updownV(ScalarV_In val);

		// PURPOSE: copy the given joint into this joint (maintaining this joint's pointers)
		// PARAMS:
		//	original - the joint to copy here
		void Copy (const phJoint1Dof& original);


		void TransformInertiaByASdownup( phArticulatedBodyInertia& dest ) const;
		void TransformInertiaByASupdown( phArticulatedBodyInertia& dest ) const;

		ScalarV_Out GetsSIs_downupV() const;
		void SetsSIs_downupV(ScalarV_In val);

	protected:

		// PURPOSE: the axis of rotation of this joint in world coordinates
		phPhaseSpaceVector m_JointAxis;

		// Values used to help computation of As for articulated inertia
		phPhaseSpaceVector IAs_downup;		// I^A_{child} s
		phPhaseSpaceVector IAs_updown;		// I^A_{parent} s
		float sSIsInvStiffness_downup;			// (s^S I^A_{child} s + gamma)^{-1}
		float sSIsInvStiffness_updown;			// (s^S I^A_{parent} s + gamma)^{-1}

		// Values updated every frame
		float CurrentAngle;			// Set only if ComputeCurrentAngle has been called
		float Unused;

		// PURPOSE: the direction of any joint bending beyond the limits
		// NOTES:	This is 0 if the limit is not exceeded, +1 if the joint angle is too large, or -1 if it is too small.
		int m_JointLimitDir;

		// This allows for hard limit extent adjustments per inst
		float m_HardAngleMinAdjust;
		float m_HardAngleMaxAdjust;

		// ---

		// PURPOSE: the linear spring and damping constants for the muscle forces, when muscle driving is active
		float m_MuscleAngleStrength,m_MuscleSpeedStrength;

		// PURPOSE: the target angle and angular speed for this prismatic joint, when muscle driving is active
		float m_MuscleTargetAngle,m_MuscleTargetSpeed;

		float sSIs_downup;						// Uses for "Precompute" inertia calculuations
#ifdef USE_SOFT_LIMITS
		// PURPOSE: the minimum soft angle limit, beyond which there is a restoring force
		float m_SoftAngleMin;

		// PURPOSE: the maximum soft angle limit, beyond which there is a restoring force
		float m_SoftAngleMax;
#endif
	};


	// ****************************************************************************
	// Inlined member functions for phJoint1Dof
	// ****************************************************************************

	inline float phJoint1Dof::ResponseToUnitTorque(phArticulatedBody *body) const 
	{
		return ResponseToUnitTorqueOnAxis(body, m_JointAxis.omega).Getf();
	}

	inline void phJoint1Dof::GetAngleLimits (float& minAngle, float& maxAngle) const 
	{
		minAngle = static_cast<phJoint1DofType*>(m_Type.ptr)->m_HardAngleMin + m_HardAngleMinAdjust;
		maxAngle = static_cast<phJoint1DofType*>(m_Type.ptr)->m_HardAngleMax + m_HardAngleMaxAdjust;
	}

	inline ScalarV_Out phJoint1Dof::GetsSIsInvStiffness_downupV() const
	{
		return ScalarVFromF32(sSIsInvStiffness_downup);
	}

	inline ScalarV_Out phJoint1Dof::GetsSIsInvStiffness_updownV() const
	{
		return ScalarVFromF32(sSIsInvStiffness_updown);
	}

	inline void phJoint1Dof::SetsSIsInvStiffness_downupV(ScalarV_In val)
	{
		StoreScalar32FromScalarV( sSIsInvStiffness_downup, val );
	}

	inline void phJoint1Dof::SetsSIsInvStiffness_updownV(ScalarV_In val)
	{
		StoreScalar32FromScalarV( sSIsInvStiffness_updown, val );
	}

	__forceinline void phJoint1Dof::TransformByAdownup( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		Vec3V factor = Scale( VECTOR3_TO_VEC3V((IAs_downup^from)), Vec3V(GetsSIsInvStiffness_downupV()) );
		phPhaseSpaceVector result = from;
		result.AddScaled( m_JointAxis, VEC3V_TO_VECTOR3(-factor) );
		to = result;
	}

	__forceinline void phJoint1Dof::TransformByAdownupTwice( const phPhaseSpaceVector& from, phPhaseSpaceVector& to, const phPhaseSpaceVector& from2, phPhaseSpaceVector& to2 ) const
	{
		Vec3V sSIsInvStiffness_downup(GetsSIsInvStiffness_downupV());
		{
			Vec3V factor = Scale( VECTOR3_TO_VEC3V((IAs_downup^from)), sSIsInvStiffness_downup );
			phPhaseSpaceVector result = from;
			result.AddScaled( m_JointAxis, VEC3V_TO_VECTOR3(-factor) );
			to = result;
		}

		{
			Vec3V factor2 = Scale( VECTOR3_TO_VEC3V((IAs_downup^from2)), sSIsInvStiffness_downup );
			phPhaseSpaceVector result2 = from2;
			result2.AddScaled( m_JointAxis, VEC3V_TO_VECTOR3(-factor2) );
			to2 = result2;
		}
	}

	__forceinline void phJoint1Dof::TransformByAdownupAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		Vec3V factor = Scale( VECTOR3_TO_VEC3V((IAs_downup^from)), Vec3V(GetsSIsInvStiffness_downupV()) );
		phPhaseSpaceVector result = to;
		result += from;
		result.AddScaled( m_JointAxis, VEC3V_TO_VECTOR3(-factor) );
		to = result;
	}

	__forceinline void phJoint1Dof::TransformByAupdown( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		phPhaseSpaceVector result = from;
		Vec3V factor = Scale( VECTOR3_TO_VEC3V((IAs_updown^from)), Vec3V(GetsSIsInvStiffness_updownV()) );
		result.AddScaled( m_JointAxis, VEC3V_TO_VECTOR3(-factor) );
		to = result;
	}

	__forceinline void phJoint1Dof::TransformByAupdownTwice( const phPhaseSpaceVector& from, phPhaseSpaceVector& to, const phPhaseSpaceVector& from2, phPhaseSpaceVector& to2 ) const
	{
		Vec3V sSIsInvStiffness_updown (GetsSIsInvStiffness_updownV());
		{
			phPhaseSpaceVector result = from;
			Vec3V factor = Scale( VECTOR3_TO_VEC3V((IAs_updown^from)), sSIsInvStiffness_updown );
			result.AddScaled( m_JointAxis, VEC3V_TO_VECTOR3(-factor) );
			to = result;
		}
		{
			phPhaseSpaceVector result2 = from2;
			Vec3V factor2 = Scale( VECTOR3_TO_VEC3V((IAs_updown^from2)), sSIsInvStiffness_updown );
			result2.AddScaled( m_JointAxis, VEC3V_TO_VECTOR3(-factor2) );
			to2 = result2;
		}
	}

	__forceinline void phJoint1Dof::TransformByASdownup( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		Vec3V factor = Scale( VECTOR3_TO_VEC3V((m_JointAxis^from)), Vec3V(GetsSIsInvStiffness_downupV()) );
		phPhaseSpaceVector result = from;
		result.AddScaled( IAs_downup, VEC3V_TO_VECTOR3(-factor) );
		to = result;
	}

	__forceinline void phJoint1Dof::TransformByASdownupAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		Vec3V factor = Scale( VECTOR3_TO_VEC3V((m_JointAxis^from)), Vec3V(GetsSIsInvStiffness_downupV()) );
		phPhaseSpaceVector result = to;
		result += from;
		result.AddScaled( IAs_downup, VEC3V_TO_VECTOR3(-factor) );
		to = result;
	}

	__forceinline void phJoint1Dof::TransformByASupdown( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		Vec3V factor = Scale( VECTOR3_TO_VEC3V((m_JointAxis^from)), Vec3V(GetsSIsInvStiffness_updownV()) );
		phPhaseSpaceVector result = from;
		result.AddScaled( IAs_updown, VEC3V_TO_VECTOR3(-factor) );
		to = result;
	}

	__forceinline void phJoint1Dof::TransformByASupdownAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		Vec3V factor = Scale( VECTOR3_TO_VEC3V((m_JointAxis^from)), Vec3V(GetsSIsInvStiffness_updownV()) );
		phPhaseSpaceVector result = to;
		result += from;
		result.AddScaled( IAs_updown, VEC3V_TO_VECTOR3(-factor) );
		to = result;
	}

	__forceinline void phJoint1Dof::TransformByAupdownAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		Vec3V factor = Scale( VECTOR3_TO_VEC3V((IAs_updown^from)), Vec3V(GetsSIsInvStiffness_updownV()) );
		phPhaseSpaceVector result = to;
		result += from;
		result.AddScaled( m_JointAxis, VEC3V_TO_VECTOR3(-factor) );
		to = result;
	}


	// ****************************************************************************
	// Inlined member functions for phJoint1Dof
	// ****************************************************************************

	inline phJoint1Dof::phJoint1Dof ()
	{
		// Set the pointer to type data
		phJoint1DofType *jointType = rage_new phJoint1DofType;
		m_Type = static_cast<phJointType*>(jointType);
		m_Type->m_JointType = phJoint::JNT_1DOF;

		m_Stiffness = 0.0f;
		m_MuscleAngleStrength = DEFAULT_MUSCLE_ANGLE_STRENGTH;
		m_MuscleSpeedStrength = DEFAULT_MUSCLE_SPEED_STRENGTH;
		m_MuscleTargetAngle = 0.0f;
		m_MuscleTargetSpeed = 0.0f;
#ifdef USE_SOFT_LIMITS
		m_SoftAngleMin = -FLT_MAX;
		m_SoftAngleMax = FLT_MAX;
#endif

		CurrentAngle = 0.0f;
		m_HardAngleMinAdjust = 0.0f;
		m_HardAngleMaxAdjust = 0.0f;
	}

	inline phJoint1Dof::phJoint1Dof(phJoint1DofType *jointType)
	{
		// Set the pointer to type data
		m_Type = static_cast<phJointType*>(jointType);
		m_Type->m_JointType = phJoint::JNT_1DOF;

		m_Stiffness = 0.0f;
		m_MuscleAngleStrength = DEFAULT_MUSCLE_ANGLE_STRENGTH;
		m_MuscleSpeedStrength = DEFAULT_MUSCLE_SPEED_STRENGTH;
		m_MuscleTargetAngle = 0.0f;
		m_MuscleTargetSpeed = 0.0f;
#ifdef USE_SOFT_LIMITS
		// init soft angle lims
		m_SoftAngleMin = Get1DofType().m_HardAngleMin;
		m_SoftAngleMax = Get1DofType().m_HardAngleMax;
#endif

		CurrentAngle = 0.0f;
		m_HardAngleMinAdjust = 0.0f;
		m_HardAngleMaxAdjust = 0.0f;
	}

#ifdef USE_SOFT_LIMITS
	inline void phJoint1Dof::SetSoftLimitStrength(float softLimitStrength)
	{
		Get1DofType().m_SoftLimitStrength = softLimitStrength;
	}
#endif
	inline void phJoint1Dof::SetMuscleTargetAngle (ScalarV_In targetAngle)
	{
		// extra asserts to catch some bad inputs we were seeing
		// seems like these should probably go across to main rage branch
		// since this is called fairly rarely, and usually by game code or NM
		Assert(targetAngle.Getf()==targetAngle.Getf() && targetAngle.Getf() >= -2.0f*PI && targetAngle.Getf() <= 2.0f*PI);
		StoreScalar32FromScalarV(m_MuscleTargetAngle,targetAngle);
	}

	inline void phJoint1Dof::SetMuscleTargetAngle (float targetAngle)
	{
#if HACK_GTA4	// EUGENE_APPROVED
		// extra asserts to catch some bad inputs we were seeing
		// seems like these should probably go across to main rage branch
		// since this is called fairly rarely, and usually by game code or NM
		Assert(targetAngle==targetAngle && targetAngle >= -2.0f*PI && targetAngle <= 2.0f*PI);
#endif
		m_MuscleTargetAngle = targetAngle;
	}

	inline void phJoint1Dof::SetMuscleTargetSpeed (ScalarV_In targetSpeed)
	{
		StoreScalar32FromScalarV(m_MuscleTargetSpeed,targetSpeed);
	}

	inline void phJoint1Dof::SetMuscleTargetSpeed (float targetSpeed)
	{
		// extra asserts to catch some bad inputs we were seeing
		// seems like these should probably go across to main rage branch
		// since this is called fairly rarely, and usually by game code or NM
		Assert(targetSpeed==targetSpeed && targetSpeed >= -6.0f*PI && targetSpeed <= 6.0f*PI);
		m_MuscleTargetSpeed = targetSpeed;
	}

	inline void phJoint1Dof::SetMuscleAngleStrength (float angleStrength)
	{
		m_MuscleAngleStrength = angleStrength;
	}

	inline void phJoint1Dof::SetMuscleSpeedStrength (float speedStrength)
	{
		m_MuscleSpeedStrength = speedStrength;
	}

	inline void phJoint1Dof::SetMaxMuscleTorque (float maxTorque)
	{
		Get1DofType().m_MaxMuscleTorque = maxTorque;
	}

	inline void phJoint1Dof::SetMinMuscleTorque (float minTorque)
	{
		Get1DofType().m_MinMuscleTorque = minTorque;
	}

	inline void phJoint1Dof::SetMinAndMaxMuscleTorque (float minTorque, float maxTorque)
	{
		Get1DofType().m_MinMuscleTorque = minTorque;
		Get1DofType().m_MaxMuscleTorque = maxTorque;
	}

	inline void phJoint1Dof::SetMinAndMaxMuscleTorque (float maxTorqueMag)
	{
		Get1DofType().m_MinMuscleTorque = -maxTorqueMag;
		Get1DofType().m_MaxMuscleTorque = maxTorqueMag;
	}

	inline float phJoint1Dof::GetMuscleTargetAngle () const
	{
		return m_MuscleTargetAngle;
	}

	inline float phJoint1Dof::GetMuscleTargetSpeed () const
	{
		return m_MuscleTargetSpeed;
	}

	inline float phJoint1Dof::GetMuscleAngleStrength () const
	{
		return m_MuscleAngleStrength;
	}

	inline float phJoint1Dof::GetMuscleSpeedStrength () const
	{
		return m_MuscleSpeedStrength;
	}

	inline float phJoint1Dof::GetMaxMuscleTorque ()
	{
		return Get1DofType().m_MaxMuscleTorque;
	}

	inline float phJoint1Dof::GetMinMuscleTorque ()
	{
		return Get1DofType().m_MinMuscleTorque;
	}

	inline ScalarV_Out phJoint1Dof::GetsSIs_downupV() const
	{
		return ScalarVFromF32(sSIs_downup);
	}

	inline void phJoint1Dof::SetsSIs_downupV(ScalarV_In val)
	{
		StoreScalar32FromScalarV( sSIs_downup, val );
	}

	__forceinline void phJoint1Dof::TransformInertiaByASdownup( phArticulatedBodyInertia& dest ) const
	{
		dest.SetOuterProductScaled( IAs_downup, sSIsInvStiffness_downup );
		dest.SubtractFrom(m_DownInertia);
	}

	__forceinline void phJoint1Dof::TransformInertiaByASupdown( phArticulatedBodyInertia& dest ) const
	{
		dest.SetOuterProductScaled( IAs_updown, sSIsInvStiffness_updown );
		dest.SubtractFrom(m_UpInertia);
	}

} // namespace rage

#endif    // ARTICULATED_BODY_JOINT_1_DOF
