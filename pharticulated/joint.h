//
// pharticulated/joint.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHARTICULATED_JOINT_TYPE_H
#define PHARTICULATED_JOINT_TYPE_H

#include "bodypart.h"

#include "data/base.h"
#include "data/struct.h"
#include "data/safestruct.h"
#include "math/simplemath.h"
#include "vectormath/mathops.h"

#include "paging/base.h"
#include "paging/ref.h"

#define DEFAULT_MUSCLE_ANGLE_STRENGTH	200.0f
#define DEFAULT_MUSCLE_SPEED_STRENGTH	10.0f

#define STIFFNESS_MODULUS_ENABLED 1

namespace rage {

#define INVALID_AB_LINK_INDEX 255

	class fiAsciiTokenizer;
	class phArticulatedBody;
	class phArticulatedBodyPart;

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	class phJointType : public pgBase
	{
	public:

#if __DECLARESTRUCT
		virtual void DeclareStruct(datTypeStruct &s);
#endif
		DECLARE_PLACE(phJointType);

		phJointType();
		phJointType(datResource &rsc);
		virtual ~phJointType() {}

		static void VirtualConstructFromPtr (datResource & rsc, phJointType* Joint, u8 jointType);

		void SetJointType( u8 set ) { m_JointType = set; }

		virtual phJointType* Clone() { return NULL; } // Should be pure virtual, but IMPLEMENT_PLACE prevents that

		float m_DefaultStiffness;

		// PURPOSE: Tell whether or not to always enforce joint limits in the collision solver, even if they are not violated.
		// NOTES:	This is used to make sure collisions on connecting body parts do not cause single-frame motion large enough to violate the joint limits.
		bool m_EnforceAccededLimits;

		// PURPOSE: the type of joint, defined in the enum near the beginning of this file
		u8 m_JointType;

		u8 m_ParentLinkIndex;
		u8 m_ChildLinkIndex;

		// PURPOSE: orientation of the joint's rotation axis in the parent's coordinates
		// NOTES:	The z-axis of the orientation matrix is the rotation axis.
		Mat34V m_OrientParent;

		// PURPOSE: orientation of the joint's rotation axis in the child's coordinates
		Mat34V m_OrientChild;    

		// ---

#ifdef USE_SOFT_LIMITS
		float GetInvTimeToSeparateSoft () { return m_InvTimeToSeparateSoft; }
		void SetInvTimeToSeparateSoft (float invTime) { m_InvTimeToSeparateSoft = invTime; }

		// PURPOSE: Define the strength of soft joint limits. On each frame, a force is applied to restore each joint that exceeds a soft limit that will, if applied
		//			continuously, accelerate the joint to reach the limit in the time to separate that is inverted here.
		float m_InvTimeToSeparateSoft;
#endif
	};


	class phJoint
	{
		friend class phJointType;
		friend class phArticulatedBody;

	public:

		phJoint();
		virtual ~phJoint() {}

		enum JointType
		{
			JNT_1DOF,
			JNT_3DOF,
			PRISM_JNT,			// Prismatic joint (1-D)
			NUM_JOINT_TYPES,
			INVALID_TYPE = 255
		};

		phArticulatedBodyPart*		GetParentLink(const phArticulatedBody *body) const;
		phArticulatedBodyPart*		GetChildLink(const phArticulatedBody *body) const;

		u8		GetParentLinkIndex() const { return m_Type->m_ParentLinkIndex; };
		u8		GetChildLinkIndex() const { return m_Type->m_ChildLinkIndex; };

		// PURPOSE: Get the type of joint, defined in the enum near the biggining of this file.
		// RETURN: the type of joint, specifying the number of degrees of freedom, and whether it is a translating or rotating joint
		u8 GetJointType() const;

		phJointType& GetType()
		{
			return *m_Type;
		}

		void SetMinStiffness( float minStiffness );
		float GetMinStiffness() const { return m_MinStiffness; }

		// PURPOSE: Apply the given torque to the joint.
		// PARAMS:
		//	torque - the torque to apply to this joint
		// NOTES:
		//	The torque is applied to the child body part, and the negated torque is applied to the parent.
		void ApplyTorque (phArticulatedBody *body, Vec::V3Param128 torque, Vec::V3Param128 timeStep) const;

		// PURPOSE: Apply the given angular impulse to the joint.
		// PARAMS:
		//	angImpulse - the angular impulse to apply to this joint
		// NOTES:
		//	The angular impulse is applied to the child body part, and the negated angular impulse is applied to the parent.
		void ApplyAngImpulse (phArticulatedBody *body, Vec::V3Param128 angImpulse) const;

		inline void MatchChildToParentPositionAndVelocity(phArticulatedBody *body);

		// Get the angular velocity of the *joint angle*
		float GetJointAngularVelocityOnAxis (phArticulatedBody *body, Vec3V_In axis) const;

		// PURPOSE: Get the position of the joint.
		// RETURN:	the joint position in the object's instance coordinates
		inline Vector3 GetJointPosition (phArticulatedBody *body) const;

		virtual void ResetLimitAdjustments() {}

		// PURPOSE: Compute the response to the joint generating a unit torque.
		// PARAMS:
		//	unitAxis - the unit direction of the unit torque
		// NOTES:
		//	1.	If the torque is applied to the child link and the opposite torque applied to the parent link, then there is a change in
		//		joint angle.  The component of the change around the same axis the value returned.  
		//	2.	Usually, this function is not called directly.  Instead it is called by one of the "Reponse" methods of 1 DOF or 3 DOF joints.
		//		In those cases, the axis is one of the special joint axes (such as the twist or lean axis).
		//	3.	The axis vector must be a unit vector
		ScalarV_Out ResponseToUnitTorqueOnAxis (phArticulatedBody *body, Vec3V_In unitAxis) const;

		// PURPOSE: Compute the response to the joint generating a unit force.
		// PARAMS:
		//	unitAxis - the unit direction of the unit force
		// NOTES:
		//		This similar to ResponseToUnitTorqueOnAxis, for use by prismatic joints.
		ScalarV_Out ResponseToUnitForceOnAxis (phArticulatedBody *body, Vec3V_In unitAxis) const;

		static void GetOrtho( Vec3V_In u, Vec3V_InOut v, Vec3V_InOut w);
		static Vec3V_Out GetOrtho( Vec3V_In u );
		static void VrRotateAlign( Mat34V_InOut outMat, Vec3V_In fromVec, Vec3V_In toVec );
		static void VrRotate( Mat34V_InOut outMat, ScalarV_In c, ScalarV_In s, Vec3V_In u );
		static void RotationMapR3Set( Mat34V_InOut ret, Vec3V_In u, ScalarV_In s, ScalarV_In c );

		// PURPOSE: Get information on whether a joint has exceeded its limits, and in what direction.
		// RETURN: the number of joint limit degrees of freedom that are violated
		inline int ComputeNumHardJointLimitDofs(phArticulatedBody *body);

		// For 0 <= j < numJointsToEnforce, get relevant info about the joint limit status.
		//		the "jointLimitID" also called a "detailCode" identifies extra information
		//		about the joint limit in a way that the joint can use effectively.
		// Uses the pre-stored values calculated by CheckJointLimitStatus
		inline void GetJointLimitDetailedInfo (phArticulatedBody *body, int limitNum, int& jointLimitID, float& dofExcess, float& response);

		// Calculate the responses to a unit "torque" or "impulse" to the given joint limit
		inline void PrecalcResponsesToJointImpulse( phArticulatedBody* theBody, int jointNum, int jointLimitID );

		// Get response to the joint limits' "Rate of Change" in response to a previously
		//		Precalc's impulse/torque.  The returned "response" is the acceleration or
		//		the delta-velocity to the joint limit value.
		inline float GetPrecalcJointLimitResponse( phArticulatedBody* theBody, int jointNum, int jointLimitID );

		// Applies a impulse or torque to the axis corresponding to the joint limit.
		inline void ApplyJointImpulse( phArticulatedBody *body, int jointLimitID, ScalarV_In impulse );
		inline void ApplyJointPush( phArticulatedBody *body, int jointLimitID, ScalarV_In push );
		inline void ApplyJointImpulseAndPush( phArticulatedBody *body, int jointLimitID, ScalarV_In impulse, ScalarV_In push );
		inline void GetJointLimitAxis( phArticulatedBody *body, int jointLimitID, Vector3& axis, Vector3& position ) const;
		inline void GetJointLimitUnitImpulse( int jointLimitID, phPhaseSpaceVector& unitImpulseSpatial );
		// Handles the joint limits by applying torques (used to be phArticulatedBody::Handle(1 or 3)DofLimits)

		inline void GetPrecalcJointLimitAngleResponse( phArticulatedBody& theBody, 
			int jointNum, int jointLimitID, Vector3& threeReponses );

		// PURPOSE: Get the orientation of the joint in the parent's local coordinates.
		// RETURN:	the orientation of this joint in the parent body part's coordinates
		// NOTES:	The third column of the matrix is a unit vector in the joint axis direction.
		const Matrix34& GetOrientationParent () const;
		const Matrix34& GetOrientationChild () const;

		// PURPOSE: Get the position of this joint in the parent's local coordinate system.
		// RETURN:	the position of this joint in the parent body part's coordinate system
		const Vector3& GetPositionParent() const;
		const Vec3V& GetPositionParentV() const;

		void SetPositionParent(const Vector3& position);

		// PURPOSE: Get the position of this joint in the child's local coordinate system.
		// RETURN:	the position of this joint in the child body part's coordinate system
		const Vector3& GetPositionChild () const;
		const Vec3V& GetPositionChildV () const;

		inline void Freeze ();

		// The states for muscles controlling joints.
		enum driveState
		{
			DRIVE_STATE_FREE,				// no muscle control
			DRIVE_STATE_SPEED,				// muscles control joint angular speed to match target
			DRIVE_STATE_ANGLE,				// muscles control joint angle to match target
			DRIVE_STATE_ANGLE_AND_SPEED,	// muscles control joint angle and angular speed (or angle with speed damping)
			NUM_DRIVE_STATES
		};

		driveState GetDriveState () const;
		void SetDriveState (driveState state);
		bool MuscleDriveActive () const;
		bool MuscleDriveAngle () const;
		bool MuscleDriveSpeed () const;

		// PURPOSE: Set the flag to always enforce joint limits in the collision solver, even if they are not violated.
		// PARAMS:
		//	enforce - optional boolean to tell whether or not to always enforce joint limits
		// NOTES:	This is used to make sure collisions on connecting body parts do not cause single-frame motion large enough to violate the joint limits.
		void SetEnforceAccededLimitsFlag (bool enforce=true);

		// PURPOSE: Tell whether this joint is currently enforcing acceded limits.
		// RETURN: whether this joint is currently enforcing acceded limits
		// NOTES:	This is used to make sure collisions on connecting body parts do not cause single-frame motion large enough to violate the joint limits.
		bool EnforcingAccededLimits ();

		// ---

		static void VirtualConstructFromPtr (datResource & rsc, phJoint* Joint);

		// PURPOSE: Set the stiffness for this joint.
		// PARAMS:
		//	stiffness - the new stiffness for this joint
		// NOTES:	Stiffness can range from 0 (loose) to 1 (rigid). It is not the same as damping - it affects the joint's reaction to impulses but not its free motion.
		void SetStiffness (float stiffness);
		void SetStiffness (ScalarV_In stiffness);

		ScalarV_Out GetStiffness() const;

		// Computes the joint torque and force from the previous physics update step
		// This should be called after UpdateVelocitiesOnly.
		void ComputeJointImpulse( Vector3 *jointImpulse ) const;
		void ComputeJointImpulses( Vector3 *jointForce, Vector3 *jointTorque ) const;

		inline phPhaseSpaceVector & GetVelocityToPropDown() { return m_VelocityToPropDown; }

		// PURPOSE: Get the effective inverse angular inertia for the given joint limit direction.
		// PARAMS:
		//	limitDirectionIndex - index number for the direction of the limit (lean1, lean2 or twist for 3dof joints)
		// RETURN:	the response (scalar angular acceleration) to a unit torque in the given direction
		inline float GetJointLimitResponse (phArticulatedBody *body, int limitDirectionIndex);

		// Finds the total change in rotation in response to a unit torque.
		void TotalResponseToTorque (phArticulatedBody *body, Vec3V_In torque, Vec3V_InOut response) const;
#ifdef USE_SOFT_LIMITS
		// PURPOSE: Apply torques to restore the joint angle to the soft limits, if it exceeds a soft minimum or maximum angle.
		// PARAMS:
		//	timeStep - the update time interval
		inline void EnforceSoftJointLimits (float timeStep);
#endif
		inline void SetAxis (phArticulatedBody *body, const Vector3& jointPosition, const Vector3& axisDirection);
		inline void SetAngleLimits (float limit1, float limit2);

		// <COMBINE phArticulatedBody::ComputeAndApplyMuscleTorques>
		inline void ComputeAndApplyMuscleTorques (phArticulatedBody *body, float UNUSED_PARAM(timeStep));

#if PHARTICULATED_DEBUG_SERIALIZE
		void DebugSerialize(fiAsciiTokenizer& t);
#endif

#if STIFFNESS_MODULUS_ENABLED
		static float sm_StiffnessModulus;
#endif 

	protected:

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

		// ---

		// PURPOSE: Clamp the given angular acceleration to avoid overshooting the given target angle during the next update.
		// PARAMS:
		//	angAccel - the angular acceleration to clamp
		//	angVelocity - the current angular velocity
		//	timeStep - the time interval for the next update
		//	currentAngle - the current angle
		//	muscleTargetAngle - the target angle for muscle forces
		Vec3V_Out ClampAngAccelFromOvershoot (Vec3V_In angAccel, Vec3V_In angVelocity, ScalarV_In timeStep, Vec3V_In currentAngle, Vec3V_In muscleTargetAngle);
		ScalarV_Out ClampAngAccelFromOvershoot (ScalarV_In angAccel, ScalarV_In angVelocity, ScalarV_In timeStep, ScalarV_In currentAngle, ScalarV_In muscleTargetAngle);

		inline void PrecomputeAupdown();
		inline void PrecomputeAdownup(phArticulatedBody *body);

		inline void TransformInertiaByASdownup( phArticulatedBodyInertia& dest ) const;
		inline void TransformInertiaByASupdown( phArticulatedBodyInertia& dest ) const;


	public:

		pgRef<phJointType> m_Type;

	protected:

		phPhaseSpaceVector m_VelocityToPropDown;						// Velocity increase propagated down through the link

		driveState m_DriveState; 

		// ---

		// Internal values used for the physics simulation
		// The first four values are needed for post-simulation function calls (Responses, etc).
		phArticulatedBodyInertia m_UpInertia;		// Inertia of the subtree above the link
		phArticulatedBodyInertia m_DownInertia;		// Inertia fo the subtree below the link

		float m_Stiffness;

		// PURPOSE: Define a minimum joint stiffness, enforced in SetStiffness().
		float m_MinStiffness;
	};

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	inline u8 phJoint::GetJointType() const
	{
		return m_Type->m_JointType;
	}

	inline bool phJoint::EnforcingAccededLimits ()
	{
		return m_Type->m_EnforceAccededLimits;
	}

	inline void phJoint::SetEnforceAccededLimitsFlag (bool enforce)
	{
		if (m_Type->m_JointType==phJoint::JNT_1DOF)
		{
			m_Type->m_EnforceAccededLimits = enforce;
		}
	}

	__forceinline const Matrix34& phJoint::GetOrientationParent () const
	{
		return RCC_MATRIX34(m_Type->m_OrientParent);
	}

	__forceinline const Matrix34& phJoint::GetOrientationChild () const
	{
		return RCC_MATRIX34(m_Type->m_OrientChild);
	}

	__forceinline const Vector3& phJoint::GetPositionParent () const
	{
		return RCC_VECTOR3(m_Type->m_OrientParent.GetCol3ConstRef());
	}

	__forceinline const Vec3V& phJoint::GetPositionParentV () const
	{
		return m_Type->m_OrientParent.GetCol3ConstRef();
	}

	__forceinline void phJoint::SetPositionParent (const Vector3& position)
	{
		m_Type->m_OrientParent.SetCol3(RCC_VEC3V(position));
	}

	__forceinline const Vector3& phJoint::GetPositionChild () const
	{
		return RCC_VECTOR3(m_Type->m_OrientChild.GetCol3ConstRef());
	}

	__forceinline const Vec3V& phJoint::GetPositionChildV () const
	{
		return m_Type->m_OrientChild.GetCol3ConstRef();
	}

	inline phJoint::driveState phJoint::GetDriveState () const 
	{ 
		return m_DriveState; 
	}

	inline void phJoint::SetDriveState (driveState state) 
	{ 
		m_DriveState = state; 
	}

	inline bool phJoint::MuscleDriveActive () const 
	{ 
		return (m_DriveState!=DRIVE_STATE_FREE); 
	}

	inline bool phJoint::MuscleDriveAngle () const 
	{ 
		return (m_DriveState==DRIVE_STATE_ANGLE || m_DriveState==DRIVE_STATE_ANGLE_AND_SPEED); 
	}

	inline bool phJoint::MuscleDriveSpeed () const 
	{ 
		return (m_DriveState==DRIVE_STATE_SPEED || m_DriveState==DRIVE_STATE_ANGLE_AND_SPEED); 
	}


	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	inline Vector3 phJoint::GetJointPosition (phArticulatedBody *body) const
	{
		Vec3V jointPos = UnTransform3x3Ortho(GetChildLink(body)->GetMatrix(),GetPositionChildV());
		jointPos += RCC_VEC3V(GetChildLink(body)->GetPosition());
		return VEC3V_TO_VECTOR3(jointPos);
	}

	inline Vec3V_Out phJoint::ClampAngAccelFromOvershoot (Vec3V_In angAccel, Vec3V_In angVelocity, ScalarV_In timeStep, Vec3V_In currentAngle, Vec3V_In muscleTargetAngle)
	{
		return Vec3V(ClampAngAccelFromOvershoot(SplatX(angAccel),SplatX(angVelocity),timeStep,SplatX(currentAngle),SplatX(muscleTargetAngle)),
			ClampAngAccelFromOvershoot(SplatY(angAccel),SplatY(angVelocity),timeStep,SplatY(currentAngle),SplatY(muscleTargetAngle)),
			ClampAngAccelFromOvershoot(SplatZ(angAccel),SplatZ(angVelocity),timeStep,SplatZ(currentAngle),SplatZ(muscleTargetAngle)));
	}

	inline ScalarV_Out phJoint::ClampAngAccelFromOvershoot (ScalarV_In angAccel, ScalarV_In angVelocity, ScalarV_In timeStep, ScalarV_In currentAngle, ScalarV_In muscleTargetAngle)
	{
		ScalarV clampedAngAccel = angAccel;
		if (IsGreaterThanAll(Abs(angAccel),ScalarVFromF32(SMALL_FLOAT)))
		{
			ScalarV predictedAngleChange = Scale(Add(angVelocity,Scale(angAccel,timeStep)),timeStep);
			ScalarV angleFromTarget = Subtract(Add(currentAngle,predictedAngleChange),muscleTargetAngle);
			if (!SameSignAll(Subtract(currentAngle,muscleTargetAngle),angleFromTarget))
			{
				// The predicted angle is past the target from the current angle, so clamp the angular acceleration to not pass the target.
				//float newAngAccel = angAccel-angleFromTarget*square(InvertSafe(timeStep));
				clampedAngAccel = ScalarV(V_ZERO);//(SameSign(angAccel,newAngAccel) ? newAngAccel : 0.0f);
			}
		}

		return clampedAngAccel;
	}

	inline void phJoint::SetStiffness (float stiffness)
	{
		float newStiffness = FPClamp(stiffness, m_MinStiffness, 0.999f);

#if STIFFNESS_MODULUS_ENABLED
		// Compute a new stiffness which is between the given stiffness and one, linearly modulated by the stiffness modulus
		newStiffness = 1.0f - (1.0f - newStiffness) * (1.0f - sm_StiffnessModulus);
#endif // STIFFNESS_MODULUS_ENABLED

		m_Stiffness = newStiffness;
	}

	inline ScalarV_Out phJoint::GetStiffness() const
	{
		return ScalarVFromF32(m_Stiffness);
	}

} // namespace rage

#endif		// ARTICULATED_BODY_JOINT_TYPE_H
