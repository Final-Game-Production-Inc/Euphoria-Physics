//
// pharticulated/joint3dof.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

// Implements a 3-DOF rotational joint, a "ball and socket joint".
//
// Joint limits are enforced as
//	(a) a lean angle limit - limits the amount of lean of the
//		central axis.   Ellipsoidal bounds apply, so different lean
//		limits can apply to two orthogonal lean directions.  
//  (b) a twist angle limit - limits the amount of rotation
//		around the central axis.  This limit is applied separately
//		from the lean angle limit.


#ifndef PHARTICULATED_JOINT3DOF_TYPE_H
#define PHARTICULATED_JOINT3DOF_TYPE_H

#include "joint.h"
#include "data/struct.h"
#include "data/safestruct.h"
#include "system/cache.h"
#include "vector/quaternion.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

namespace rage {

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	class phJoint3DofType : public phJointType
	{
	public:

#if __DECLARESTRUCT
		void DeclareStruct(datTypeStruct &s);
#endif

		phJoint3DofType();
		~phJoint3DofType() {}
		phJoint3DofType(datResource &rsc);

		virtual phJointType* Clone();

		float m_HardFirstLeanAngleMax;                	// Lean angle limit in first direction
		float m_HardSecondLeanAngleMax;	                // Lean angle limit in second direction
		float m_HardTwistAngleMax;	                	// hard twist limit
#ifdef USE_SOFT_LIMITS
		// Ratio of soft to hard limits
		float m_SoftLimitRatio;
#endif
		// Used for inclusion/exclusion cones
		float m_TwistOffset;

		// PURPOSE: Tell whether to use the child body part's z-axis as the axis about which to compute the twist angle.
		// NOTES:	Default is false. This can be used to make twist calculations the same as in Natural Motion.
		//  ---- NOT FINISHED, THIS DOESN'T WORK YET ----
		bool m_UseChildForTwistAxis;      
#ifdef USE_SOFT_LIMITS
		datPadding<11> m_Pad;
#else
		datPadding<15> m_Pad;
#endif

		// ---

		// PURPOSE: the upper and lower limits on the muscle torque that will be applied, when muscle driving is active
		Vec3V m_MaxMuscleTorque;
		Vec3V m_MinMuscleTorque;
#ifdef USE_SOFT_LIMITS
		float m_SoftLimitLeanStrength;
		float m_SoftLimitTwistStrength;
		datPadding<8> m_Pad1;
#else
		datPadding<16> m_Pad1;
#endif
	} ;


	class phJoint3Dof : public phJoint
	{
		friend class phJoint;

	public:
		phJoint3Dof();
		~phJoint3Dof() {}
		phJoint3Dof(phJoint3DofType *jointType);
		phJoint3Dof(datResource& ) {};

		// The lean value is always between 0 and PI.
		// The twist angle is always between -PI and PI. 
		// The "Get" routines should be used only if the corresponding "Compute"
		//		function has already been called.
		float ComputeCurrentLean(phArticulatedBody *body);			// Find the lean angle

		// PURPOSE: Find the lean and twist angles for this joint from the parent and child body part orientations.
		void ComputeCurrentLeanAndTwist(phArticulatedBody *body);

		// The "NoCompute" version is the same, but does not call ComputeCurrentLean.
		//		Either ComputeCurrentLean or ComputeCurrentLeanAndTwist should be called first.
		//		The "NoCompute" version is mostly for internal use.
		bool CheckHardLeanLimitStatus (phArticulatedBody *body);

		// <COMBINE phJoint::GetJointLimitResponse>
		float GetJointLimitResponse (phArticulatedBody *body, int limitDirectionIndex);

		void MatchChildToParentPositionAndVelocity(phArticulatedBody *body);

		// <COMBINE phJoint::ComputeNumHardJointLimitDofs>
		int ComputeNumHardJointLimitDofs (phArticulatedBody *body);

		// For 0 <= j < numJointsToEnforce, get relevant info about the joint limit status.
		//		the "jointLimitID" also called a "detailCode" identifies extra information
		//		about the joint limit in a way that the joint can use effectively.
		// Uses the prestored values calculated by CheckJointLimitStatus
		void GetJointLimitDetailedInfo (phArticulatedBody *body, int limitNum, int& jointLimitID, float& dofExcess, float& response);

		// Applies a impulse or torque to the axis corresponding to the joint limit.
		void ApplyJointImpulse( phArticulatedBody *body, int jointLimitID, ScalarV_In impulse );
		void ApplyJointPush( phArticulatedBody *body, int jointLimitID, ScalarV_In push );
		void ApplyJointImpulseAndPush( phArticulatedBody *body, int jointLimitID, ScalarV_In impulse, ScalarV_In turn );
		void GetJointLimitAxis( phArticulatedBody *body, int jointLimitID, Vector3& axis, Vector3& position ) const;
		void GetJointLimitUnitImpulse( int jointLimitID, phPhaseSpaceVector& unitImpulseSpatial );

		Vec3V_Out ComputeCurrentLeanAngles (phArticulatedBody *body);
		void ComputeCurrentLeanAnglesAndRates (phArticulatedBody *body, Vec3V_InOut lean12Twist, Vec3V_InOut lean12TwistRate);

		bool HasTwistLimit () { return m_HasTwistLimit; }			// Is "true" if there is an effective twist limit

		void UseChildForTwistAxis (bool enable=true);

		void Freeze ();

		// <COMBINE phJoint1Dof::Copy>
		void Copy (const phJoint3Dof& original);

#ifdef USE_SOFT_LIMITS
		void SetSoftLimitRatio(float set) { Get3DofType().m_SoftLimitRatio = set; }
#endif

		// PURPOSE: Set the axis position using global coordinates.
		// PARAMS:
		//	axisPosition - the joint's axis position
		//	axisDir - the joint's unit axis direction
		//	parentLink - the joint's parent body part
		//	childLink - the joint's child body part
		// NOTES:
		//	1. First place the links in the desired (rest) position, then call SetAxis
		//	2. The two local coordinate systems coincide in the rest position.
		//	3. The z axis is the central (twist) axis.
		void SetAxis (phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir);

		// PURPOSE: Set the axis position using global coordinates.
		// PARAMS:
		//	axisPosition - the joint's axis position
		//	axisDir - the joint's unit axis direction
		//	parentLink - the joint's parent body part
		//	childLink - the joint's child body part
		//	maxLeanAngle - the lean angle limit
		//	maxTwistAngle - the twist angle limit
		void SetAxis (phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir,
			float maxLeanAngle, float maxTwistAngle);

		// PURPOSE: Set the axis position using global coordinates.
		// PARAMS:
		//	axisPosition - the joint's axis position
		//	axisDir - the joint's unit axis direction
		//	parentLink - the joint's parent body part
		//	childLink - the joint's child body part
		//	firstLeanDir - the lean1 direction
		//	firstMaxLeanAngle - the lean1 angle limit
		//	secondMaxLeanAngle - the lean2 angle limit
		//	maxTwistAngle - the twist angle limit
		// NOTES:
		//	This is the most general form of SetAxis - it allows different lean limits in different orthogonal directions.
		void SetAxis(phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir, 
			const Vector3& firstLeanDir, float firstMaxLeanAngle, float secondMaxLeanAngle, float maxTwistAngle);

		// Intended mostly for internal use:
		void SetAxis( phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir,
			const Vector3& firstLeanDir, const Vector3& secondLeanDir);

		void SetAxis(phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir, const Vector3& firstLeanDir, float firstMaxLeanAngle, 
			float secondMaxLeanAngle, float maxTwistAngle, const Quaternion& parentRotation, const Quaternion& childRotation=Quaternion::sm_I);

		void SetAngleLimits (float maxLeanAngle, float maxTwistAngle);
		void SetThreeAngleLimits (float maxLean1Angle, float maxLean2Angle, float maxTwistAngle, bool adjustOnly = false);
		void GetThreeAngleLimits (float& maxLean1Angle, float& maxLean2Angle, float& maxTwistAngle) const;

		void ResetLimitAdjustments() { m_Lean1Adjustment = m_Lean2Adjustment = m_TwistAdjustment = 0.0f; }

#ifdef USE_SOFT_LIMITS
		// Use to find out whether and by how much the lean limit is exceeded.
		// CheckLeanLimitStatus calls ComputeCurrentLean.
		bool CheckSoftLeanLimitStatus (float* excessLeanSoft, Vector3* rotAxisLean, float* rotVelLean, Vector3* rotAxisTwist);
		bool CheckSoftLeanLimitStatus (phArticulatedBody *body);
		// Use to find out whether and by how much the lean and twist limits are exceeded.
		bool CheckSoftLeanAndTwistLimitStatus (bool* leanExceededFlag, float* excessLeanSoft,
			Vector3* rotAxisLean, float* rotVelLean, bool* twistExceededFlag,
			int* twistDir, float* excessTwistSoft,
			Vector3* rotAxisTwist, float* rotVelTwist);

		int ComputeNumSoftJointLimitDofs (phArticulatedBody *body);

		// <COMBINE phJoint::EnforceSoftJointLimits>
		void EnforceSoftJointLimits (float timeStep);
#endif
		void SetLean1TargetAngle (float leanTarget);   
		void SetLean2TargetAngle (float leanTarget);   
		void SetTwistTargetAngle (float twistTarget);  
		void SetLean1TargetSpeed (float leanTarget);   
		void SetLean2TargetSpeed (float leanTarget);   
		void SetTwistTargetSpeed (float twistTarget);  
		void SetMaxLean1Torque (float maxTorque);      
		void SetMaxLean2Torque (float maxTorque);      
		void SetMaxTwistTorque (float maxTorque);      
		void SetMinLean1Torque (float minTorque);      
		void SetMinLean2Torque (float minTorque);      
		void SetMinTwistTorque (float minTorque);

		// PURPOSE: Set the target orientation for muscle torques to drive toward.
		// PARAMS:
		//	targetAngle - the target orientation for muscle torques to drive toward
		void SetMuscleTargetAngle (Vec3V_In targetAngle);
		void SetMuscleTargetAngle (const Vector3& targetAngle);

		// PURPOSE: Set the target angular velocity for muscle torques to drive toward.
		// PARAMS:
		//	targetAngle - the target angular velocity for muscle torques to drive toward
		void SetMuscleTargetSpeed (Vec3V_In targetSpeed);
		void SetMuscleTargetSpeed (const Vector3& targetSpeed);

		// PURPOSE: Set the target angle strength for muscle torques.
		// PARAMS:
		//	angleStrength - linear spring constant for muscle torques to drive toward the target angle
		void SetMuscleAngleStrength (const Vector3& angleStrength);

		// PURPOSE: Set the target angle strength for muscle torques.
		// PARAMS:
		//	angleStrength - linear spring constant for muscle torques to drive toward the target angle
		void SetMuscleAngleStrength (const float& angleStrength);

		// PURPOSE: Set the target speed strength for muscle torques.
		// PARAMS:
		//	speedStrength - linear spring constant for muscle torques to drive toward the target speed
		void SetMuscleSpeedStrength (const Vector3& speedStrength);

		// PURPOSE: Set the target speed strength for muscle torques.
		// PARAMS:
		//	speedStrength - linear spring constant for muscle torques to drive toward the target speed
		void SetMuscleSpeedStrength (const float& speedStrength);

		// PURPOSE: Set the maximum torque that muscles can apply.
		// PARAMS:
		//	maxTorque - the maximum torque that muscles can apply
		void SetMaxMuscleTorque (const Vector3& maxTorque);

		// PURPOSE: Set the minimum torque that muscles can apply.
		// PARAMS:
		//	maxTorque - the minimum torque that muscles can apply
		void SetMinMuscleTorque (const Vector3& minTorque);

		// PURPOSE: Set the maximum and minimum torques that muscles can apply.
		// PARAMS:
		//	minTorque - the minimum torque that muscles can apply
		//	maxTorque - the maximum torque that muscles can apply
		void SetMinAndMaxMuscleTorque (const Vector3& minTorque, const Vector3& maxTorque);

		// PURPOSE: Set the maximum and minimum torque that muscles can apply.
		// PARAMS:
		//	maxTorqueMag - the maximum magnitude of the torque that muscles can apply
		void SetMinAndMaxMuscleTorque (const Vector3& maxTorqueMag);
#ifdef USE_SOFT_LIMITS
		// PURPOSE: Set the strength with which each dof maintains soft limits.
		// PARAMS:
		//	leanStrength - linear spring constant for muscle torques of lean to drive away from exceeding soft limits
		//	twistStrength - linear spring constant for muscle torques of twist to drive away from exceeding soft limits
		void SetSoftLimitStrengths (float leanStrength, float twistStrength );
#endif
		// PURPOSE: Get the muscle target angle.
		// RETURN: the muscle target angle
		const Vector3& GetMuscleTargetAngle () const;

		// PURPOSE: Get the muscle target speed.
		// RETURN: the muscle target speed
		const Vector3& GetMuscleTargetSpeed () const;

		// PURPOSE: Get the muscle angle strength.
		// RETURN: the linear spring constant for muscle torques to drive toward the target angle
		const Vector3& GetMuscleAngleStrength () const;

		// PURPOSE: Get the muscle speed strength.
		// RETURN: the linear spring constant for muscle torques to drive toward the target speed
		const Vector3& GetMuscleSpeedStrength () const;

		// PURPOSE: Get the maximum muscle torque.
		// RETURN: the maximum torque that the muscles can apply
		const Vector3& GetMaxMuscleTorque () const;

		// PURPOSE: Get the minimum muscle torque.
		// RETURN: the minimum torque that the muscles can apply
		const Vector3& GetMinMuscleTorque () const;

		// PURPOSE: Compute the torque on this joint from its muscles.
		// PARAMS:
		//	timeStep - the frame time interval
		// RETURN: the torque on this joint from its muscles
		Vec3V_Out ComputeMuscleTorque (phArticulatedBody *body, ScalarV_In timeStep);

		// <COMBINE phJoint::ComputeAndApplyMuscleTorques>
		void ComputeAndApplyMuscleTorques (phArticulatedBody *body, float timeStep);

		void ComputeMuscleTorqueFromAccel (phArticulatedBody *body, const Vector3& angAccel, float leanMtx22a, float leanMtx22c, Vector3& torque);

		phJoint3DofType & Get3DofType() { return static_cast<phJoint3DofType&>(*m_Type); }

#if PHARTICULATED_DEBUG_SERIALIZE
		void DebugSerialize(fiAsciiTokenizer& t);
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

		void TranslateTensorUpperTriangular( phArticulatedBodyInertia& t ) const;

		bool NeedToKnowTwistValues();

		void PrecomputeAupdown();
		void PrecomputeAdownup(phArticulatedBody *body);

		void TransformInertiaByASdownup( phArticulatedBodyInertia& dest ) const;
		void TransformInertiaByASupdown( phArticulatedBodyInertia& accum ) const;

		static void TranslateTensor( const phArticulatedBodyInertia& src, phArticulatedBodyInertia *dest, Vector3::Vector3Param pos );

		//protected:
	public:

		// PURPOSE: the joint's pose in world coordinates
		Mat34V m_JointPoseWorld;      

		// Values used to help computation of A's
		Mat33V UpIAstiffnessInvH;			// (I^A + stiffness)^{-1}H in joint coordinates
		Mat33V DownIAstiffnessInvH;			// (I^A + stiffness)^{-1}H in joint coordinates
		Mat33V UpIAstiffnessInvI;			// (I^A + stiffness)^{-1}I^A in joint coordinates
		Mat33V DownIAstiffnessInvI;			// (I^A + stiffness)^{-1}I^A in joint coordinates

		Vec3V JointPosition;				// Position of joint (in global coords)

		Vec3V m_LeanAxis;					// Axis of rotation causing the current lean.

		// PURPOSE: axis of rotation for the current twist
		// NOTES:	This is arbitrary if the lean angle is zero.
		Vec3V m_TwistAxis;

		// PURPOSE: axis orthogonal to both lean and twist axes
		// NOTES:	This is set only if lean limit exceeded, and it is arbitrary if the lean angle is zero.
		Vec3V m_ThirdAxis;

		// This allows for hard limit extent adjustments per inst
		float m_Lean1Adjustment;
		float m_Lean2Adjustment;
		float m_TwistAdjustment;

		// Values needed for joint limits (not used during physics simulation)
		float m_CurrentHardLeanLimit;                	// Max lean permitted

		// The current max or min twist permitted.
		float m_CurrentHardTwistLimit;                	// Max or min twist permitted

		float CurrentRateOfLean;	                	// Rate at which lean is increasing

		// PURPOSE: the rate of change of the rotation angle about the twist axis
		float m_TwistRate;                 

		// stores the current lean
		float CurrentLeanAngle;                 		// Updated by ComputeCurrentLean and etc.

		// PURPOSE: The angle of rotation about the twist axis
		// NOTES:	This is updated by ComputeCurrentLeanAndTwist.
		float m_TwistAngle;                             

		// PURPOSE: 1.0/cosine(0.5f*CurrentLeanAngle)
		// NOTES:	This is used to convert the rotation around the twist axis to the change in twist
		float m_TwistFactor;                                       

		// Values needed for joint limits (not used during physics simulation)
		bool m_LeanLimitExceeded;	                		// true if lean limit exceeded
		bool m_TwistLimitExceeded;                		// true of twist limit exceeded

		bool m_DifferentMaxLeanAngles;                	// Is "true" if the two max lean angles are different

		bool m_HasTwistLimit;    

		// ---

		// Values used to help computation of A's
		phArticulatedBodyInertia UpInertiaJC;		// "Up Inertia" in joint coordinates
		phArticulatedBodyInertia DownInertiaJC;	// "Down Inertia" in joint coordinates

		// PURPOSE: the linear spring and damping constants for the muscle forces, when muscle driving is active
		Vec3V m_MuscleAngleStrength,m_MuscleSpeedStrength;

		// PURPOSE: the target angle and angular speed for this prismatic joint, when muscle driving is active
		Vec3V m_MuscleTargetAngle,m_MuscleTargetSpeed;
#ifdef USE_SOFT_LIMITS
		// Values needed for joint limits (not used during physics simulation)
		float m_CurrentSoftLeanLimit;		// Max lean permitted
		float m_CurrentSoftTwistLimit;
#endif
		float m_gammaStiffness;				// Gamma (stiffness) value for above items
	};

	// ****************************************************************************
	// Inlined member functions for phJoint1Dof
	// ****************************************************************************

	__forceinline void phJoint3Dof::TransformByAdownup( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		Vec3V v_JointPosition = JointPosition;
		phPhaseSpaceVector toVec;
		phPhaseSpaceVector fromVec = from;

		toVec.trans = fromVec.trans;
		toVec.trans = AddCrossed( toVec.trans, fromVec.omega, v_JointPosition );			// b_\ell

		Vec3V temp;
		temp = UnTransformOrtho( DownIAstiffnessInvI, fromVec.omega );
		toVec.omega = UnTransformOrtho( DownIAstiffnessInvH, toVec.trans );

		toVec.omega += temp;
		toVec.omega = fromVec.omega - toVec.omega;

		toVec.trans = AddCrossed( toVec.trans, v_JointPosition, toVec.omega ); // Convert to global coordinates

		to = toVec;
	}

	__forceinline void phJoint3Dof::TransformByAdownupTwice( const phPhaseSpaceVector& from, phPhaseSpaceVector& to, const phPhaseSpaceVector& from2, phPhaseSpaceVector& to2 ) const
	{
		Vec3V v_JointPosition = JointPosition;

		{
			phPhaseSpaceVector toVec;
			phPhaseSpaceVector fromVec = from;

			toVec.trans = fromVec.trans;
			toVec.trans = AddCrossed( toVec.trans, fromVec.omega, v_JointPosition );			// b_\ell

			Vec3V temp;
			temp = UnTransformOrtho( DownIAstiffnessInvI, fromVec.omega );
			toVec.omega = UnTransformOrtho( DownIAstiffnessInvH, toVec.trans );

			toVec.omega += temp;
			toVec.omega = fromVec.omega - toVec.omega;

			toVec.trans = AddCrossed( toVec.trans, v_JointPosition, toVec.omega ); // Convert to global coordinates

			to = toVec;
		}

		{
			phPhaseSpaceVector toVec2;
			phPhaseSpaceVector fromVec2 = from2;

			toVec2.trans = fromVec2.trans;
			toVec2.trans = AddCrossed( toVec2.trans, fromVec2.omega, v_JointPosition );			// b_\ell

			Vec3V temp;
			temp = UnTransformOrtho( DownIAstiffnessInvI, fromVec2.omega );
			toVec2.omega = UnTransformOrtho( DownIAstiffnessInvH, toVec2.trans );

			toVec2.omega += temp;
			toVec2.omega = fromVec2.omega - toVec2.omega;

			toVec2.trans = AddCrossed( toVec2.trans, v_JointPosition, toVec2.omega ); // Convert to global coordinates

			to2 = toVec2;
		}
	}

	__forceinline void phJoint3Dof::TransformByAupdown( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		Vec3V v_JointPosition = JointPosition;
		phPhaseSpaceVector toVec;
		phPhaseSpaceVector fromVec = from;

		toVec.trans = fromVec.trans;
		toVec.trans = AddCrossed( toVec.trans, fromVec.omega, v_JointPosition );			// b_\ell

		Vec3V temp;
		temp = UnTransformOrtho( UpIAstiffnessInvI, fromVec.omega );
		toVec.omega = UnTransformOrtho( UpIAstiffnessInvH, toVec.trans );
		toVec.omega += temp;
		toVec.omega = fromVec.omega - toVec.omega;

		toVec.trans = AddCrossed( toVec.trans, v_JointPosition, toVec.omega ); // Convert to global coordinates

		to = toVec;
	}

	__forceinline void phJoint3Dof::TransformByAupdownTwice( const phPhaseSpaceVector& from, phPhaseSpaceVector& to, const phPhaseSpaceVector& from2, phPhaseSpaceVector& to2 ) const
	{
		Vec3V v_JointPosition = JointPosition;

		{
			phPhaseSpaceVector toVec;
			phPhaseSpaceVector fromVec = from;

			toVec.trans = fromVec.trans;
			toVec.trans = AddCrossed( toVec.trans, fromVec.omega, v_JointPosition );			// b_\ell

			Vec3V temp;
			temp = UnTransformOrtho( UpIAstiffnessInvI, fromVec.omega );
			toVec.omega = UnTransformOrtho( UpIAstiffnessInvH, toVec.trans );
			toVec.omega += temp;
			toVec.omega = fromVec.omega - toVec.omega;

			toVec.trans = AddCrossed( toVec.trans, v_JointPosition, toVec.omega ); // Convert to global coordinates

			to = toVec;
		}

		{
			phPhaseSpaceVector toVec2;
			phPhaseSpaceVector fromVec2 = from2;

			toVec2.trans = fromVec2.trans;
			toVec2.trans = AddCrossed( toVec2.trans, fromVec2.omega, v_JointPosition );			// b_\ell

			Vec3V temp2;
			temp2 = UnTransformOrtho( UpIAstiffnessInvI, fromVec2.omega );
			toVec2.omega = UnTransformOrtho( UpIAstiffnessInvH, toVec2.trans );
			toVec2.omega += temp2;
			toVec2.omega = fromVec2.omega - toVec2.omega;

			toVec2.trans = AddCrossed( toVec2.trans, v_JointPosition, toVec2.omega ); // Convert to global coordinates

			to2 = toVec2;
		}
	}

	__forceinline void phJoint3Dof::TransformByAdownupAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		phPhaseSpaceVector xformFrom;
		TransformByAdownup( from, xformFrom );
		to += xformFrom;
	}

	__forceinline void phJoint3Dof::TransformByASdownup( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		Vec3V v_JointPosition = JointPosition;
		phPhaseSpaceVector tempIn = from;
		phPhaseSpaceVector tempOut;

		tempOut.omega = tempIn.trans;
		tempOut.omega = AddCrossed( tempOut.omega, tempIn.omega, v_JointPosition );		// ( from.omega, to.omega ) is "from" in local coords

		tempOut.trans = Multiply( DownIAstiffnessInvI, tempOut.omega );
		tempOut.trans = tempOut.omega - tempOut.trans;
		tempOut.omega = Multiply( DownIAstiffnessInvH, tempOut.omega );
		tempOut.omega = tempIn.omega - tempOut.omega;

		tempOut.trans = AddCrossed(tempOut.trans, v_JointPosition, tempOut.omega);	// result in global coordinates

		to = tempOut;
	}

	__forceinline void phJoint3Dof::TransformByASupdown( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		Vec3V v_JointPosition = JointPosition;
		phPhaseSpaceVector tempIn = from;
		phPhaseSpaceVector tempOut;

		tempOut.omega = tempIn.trans;
		tempOut.omega = AddCrossed( tempOut.omega, tempIn.omega, v_JointPosition );		// ( from.omega, to.omega ) is "from" in local coords

		tempOut.trans = Multiply( UpIAstiffnessInvI, tempOut.omega );
		tempOut.trans = tempOut.omega - tempOut.trans;
		tempOut.omega = Multiply( UpIAstiffnessInvH, tempOut.omega );
		tempOut.omega = tempIn.omega - tempOut.omega;

		tempOut.trans = AddCrossed(tempOut.trans, v_JointPosition, tempOut.omega);	// result in global coordinates

		to = tempOut;
	}

	__forceinline void phJoint3Dof::TransformByASdownupAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		phPhaseSpaceVector xformFrom;
		TransformByASdownup( from, xformFrom );
		to += xformFrom;
	}

	__forceinline void phJoint3Dof::TransformByASupdownAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		phPhaseSpaceVector xformFrom;
		TransformByASupdown( from, xformFrom );
		to += xformFrom;
	}

	__forceinline void phJoint3Dof::TransformByAupdownAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
	{
		phPhaseSpaceVector xformFrom;
		TransformByAupdown( from, xformFrom );
		to += xformFrom;
	}

	__forceinline void phJoint3Dof::TranslateTensorUpperTriangular( phArticulatedBodyInertia& t ) const
	{
		// Save locals.
		// INs.
		Mat33V v_tMass					= t.m_Mass;
		// OUTs.
		Mat33V v_tCrossInertia			= t.m_CrossInertia;
		Mat33V v_tInvInertia			= t.m_InvInertia;

		// Transform the tensor  t  by a translation of  -JointPosition
		//	Purpose: transform tensor in local coordinates to a tensor in
		//		in global coordinates
		//  It is assumed that t.H and t.I are zero and only t.M is non-zero.
		//  The old values of t.H and t.I are ignored and are overwritten.

		RC_MATRIX33(v_tInvInertia).CrossProduct( RCC_VECTOR3(JointPosition) );		// Use I as temporary memory (cute!)

		v_tCrossInertia = v_tInvInertia;
		Multiply( v_tCrossInertia, v_tMass, v_tCrossInertia );

		Mat33V zzz = v_tInvInertia;
		Multiply( v_tInvInertia, zzz, v_tCrossInertia );
		v_tInvInertia = -v_tInvInertia;

		t.m_InvInertia = v_tInvInertia;
		t.m_CrossInertia = v_tCrossInertia;
	}

	inline void phJoint3Dof::UseChildForTwistAxis (bool enable)
	{
		Get3DofType().m_UseChildForTwistAxis = enable;
	}

	inline bool phJoint3Dof::NeedToKnowTwistValues() 
	{ 
		return (m_HasTwistLimit || MuscleDriveActive()); 
	}

	// ****************************************************************************
	// Inlined member functions for phJoint1Dof
	// ****************************************************************************

	inline phJoint3Dof::phJoint3Dof() 
		: m_CurrentHardLeanLimit(FLT_MAX)		// Max lean permitted
		, m_CurrentHardTwistLimit(FLT_MAX)		// Max or min twist permitted
		, CurrentRateOfLean(0.0f)			// Rate at which lean is increasing
		, m_TwistRate(0.0f)			// Rate at which twist is increasing
		, CurrentLeanAngle(0.0f)				// Updated by ComputeCurrentLean and etc.
		, m_TwistAngle(0.0f)
		, m_Lean1Adjustment(0.0f)
		, m_Lean2Adjustment(0.0f)
		, m_TwistAdjustment(0.0f)
		, m_TwistFactor(0.0f)					// This equals 1.0/cos(CurrentLeanAngle/2.0).
		, m_LeanLimitExceeded(false)				// true if lean limit exceeded
		, m_DifferentMaxLeanAngles(false)	
		, m_HasTwistLimit(false)	
		, m_TwistLimitExceeded(false)			// true of twist limit exceeded
#ifdef USE_SOFT_LIMITS
		, m_CurrentSoftLeanLimit(FLT_MAX),		// Max lean permitted
		m_CurrentSoftTwistLimit(FLT_MAX)
#endif
	{
		// Set the pointer to type data
		phJoint3DofType *jointType = rage_new phJoint3DofType;
		m_Type = static_cast<phJointType*>(jointType);
		m_Type->m_JointType = phJoint::JNT_3DOF;

		m_Stiffness = 0.0f;

		m_MuscleAngleStrength = Vec3V(DEFAULT_MUSCLE_ANGLE_STRENGTH,DEFAULT_MUSCLE_ANGLE_STRENGTH,DEFAULT_MUSCLE_ANGLE_STRENGTH);
		m_MuscleSpeedStrength = Vec3V(DEFAULT_MUSCLE_SPEED_STRENGTH,DEFAULT_MUSCLE_SPEED_STRENGTH,DEFAULT_MUSCLE_SPEED_STRENGTH);
		m_MuscleTargetAngle = Vec3V(V_ZERO);
		m_MuscleTargetSpeed = Vec3V(V_ZERO);
	}

	inline phJoint3Dof::phJoint3Dof(phJoint3DofType *jointType) 
		: m_CurrentHardLeanLimit(FLT_MAX)		// Max lean permitted
		, m_CurrentHardTwistLimit(FLT_MAX)		// Max or min twist permitted
		, CurrentRateOfLean(0.0f)			// Rate at which lean is increasing
		, m_TwistRate(0.0f)			// Rate at which twist is increasing
		, CurrentLeanAngle(0.0f)				// Updated by ComputeCurrentLean and etc.
		, m_TwistAngle(0.0f)
		, m_Lean1Adjustment(0.0f)
		, m_Lean2Adjustment(0.0f)
		, m_TwistAdjustment(0.0f)
		, m_TwistFactor(0.0f)					// This equals 1.0/cos(CurrentLeanAngle/2.0).
		, m_LeanLimitExceeded(false)				// true if lean limit exceeded
		, m_DifferentMaxLeanAngles(false)	
		, m_HasTwistLimit(false)	
		, m_TwistLimitExceeded(false)			// true of twist limit exceeded
#ifdef USE_SOFT_LIMITS
		, m_CurrentSoftLeanLimit(FLT_MAX),		// Max lean permitted
		m_CurrentSoftTwistLimit(FLT_MAX)
#endif
	{
		// Set the pointer to type data
		m_Type = static_cast<phJointType*>(jointType);

		m_Type->m_JointType = phJoint::JNT_3DOF;

		m_Stiffness = 0.0f;

		m_MuscleAngleStrength = Vec3V(DEFAULT_MUSCLE_ANGLE_STRENGTH,DEFAULT_MUSCLE_ANGLE_STRENGTH,DEFAULT_MUSCLE_ANGLE_STRENGTH);
		m_MuscleSpeedStrength = Vec3V(DEFAULT_MUSCLE_SPEED_STRENGTH,DEFAULT_MUSCLE_SPEED_STRENGTH,DEFAULT_MUSCLE_SPEED_STRENGTH);
		m_MuscleTargetAngle = Vec3V(V_ZERO);
		m_MuscleTargetSpeed = Vec3V(V_ZERO);

		m_DifferentMaxLeanAngles = (Get3DofType().m_HardFirstLeanAngleMax!=Get3DofType().m_HardSecondLeanAngleMax);
		m_HasTwistLimit = (Get3DofType().m_HardTwistAngleMax !=FLT_MAX);
	}

	inline void phJoint3Dof::SetMuscleTargetAngle (Vec3V_In targetAngle) 
	{
		Vec3f s_MuscleTargetAngle;
		Vec3f s_targetAngle;
		LoadAsScalar( s_MuscleTargetAngle, m_MuscleTargetAngle );
		LoadAsScalar( s_targetAngle, targetAngle );

		Vec3f lower( -(Get3DofType().m_HardFirstLeanAngleMax+m_Lean1Adjustment), -(Get3DofType().m_HardSecondLeanAngleMax+m_Lean2Adjustment), -(Get3DofType().m_HardTwistAngleMax + m_TwistAdjustment) );
		Vec3f upper( Get3DofType().m_HardFirstLeanAngleMax+m_Lean1Adjustment, Get3DofType().m_HardSecondLeanAngleMax+m_Lean2Adjustment, Get3DofType().m_HardTwistAngleMax + m_TwistAdjustment );
		s_MuscleTargetAngle = Clamp( s_targetAngle, lower, upper );
		StoreAsScalar( m_MuscleTargetAngle, s_MuscleTargetAngle );
	}

	inline void phJoint3Dof::SetMuscleTargetAngle (const Vector3& targetAngle) 
	{
		Vec3f s_MuscleTargetAngle;
		Vec3f s_targetAngle;
		LoadAsScalar( s_MuscleTargetAngle, m_MuscleTargetAngle );
		LoadAsScalar( s_targetAngle, RCC_VEC3V(targetAngle) );

		Vec3f lower( -(Get3DofType().m_HardFirstLeanAngleMax+m_Lean1Adjustment), -(Get3DofType().m_HardSecondLeanAngleMax+m_Lean2Adjustment), -(Get3DofType().m_HardTwistAngleMax + m_TwistAdjustment) );
		Vec3f upper( Get3DofType().m_HardFirstLeanAngleMax+m_Lean1Adjustment, Get3DofType().m_HardSecondLeanAngleMax+m_Lean2Adjustment, Get3DofType().m_HardTwistAngleMax + m_TwistAdjustment );
		s_MuscleTargetAngle = Clamp( s_targetAngle, lower, upper );
		StoreAsScalar( m_MuscleTargetAngle, s_MuscleTargetAngle );
	}

	__forceinline void phJoint3Dof::SetMuscleTargetSpeed (Vec3V_In targetSpeed)
	{
		m_MuscleTargetSpeed = targetSpeed;
	}

	__forceinline void phJoint3Dof::SetMuscleTargetSpeed (const Vector3& targetSpeed)
	{
		m_MuscleTargetSpeed = RCC_VEC3V(targetSpeed);
	}

	__forceinline void phJoint3Dof::SetMuscleAngleStrength (const Vector3& angleStrength)
	{
		m_MuscleAngleStrength = RCC_VEC3V(angleStrength);
	}

	__forceinline void phJoint3Dof::SetMuscleAngleStrength (const float& angleStrength)
	{
		SetMuscleAngleStrength( VEC3V_TO_VECTOR3(Vec3VFromF32(angleStrength)) );
	}

	__forceinline void phJoint3Dof::SetMuscleSpeedStrength (const Vector3& speedStrength)
	{
		m_MuscleSpeedStrength = RCC_VEC3V(speedStrength);
	}

	__forceinline void phJoint3Dof::SetMuscleSpeedStrength (const float& speedStrength)
	{
		SetMuscleSpeedStrength( VEC3V_TO_VECTOR3(Vec3VFromF32(speedStrength)) );
	}

	__forceinline void phJoint3Dof::SetMaxMuscleTorque (const Vector3& maxTorque)
	{
		static_cast<phJoint3DofType*>(m_Type.ptr)->m_MaxMuscleTorque = RCC_VEC3V(maxTorque);
	}

	__forceinline void phJoint3Dof::SetMinMuscleTorque (const Vector3& minTorque)
	{
		static_cast<phJoint3DofType*>(m_Type.ptr)->m_MinMuscleTorque = RCC_VEC3V(minTorque);
	}

	inline void phJoint3Dof::SetMinAndMaxMuscleTorque (const Vector3& minTorque, const Vector3& maxTorque)
	{
		static_cast<phJoint3DofType*>(m_Type.ptr)->m_MinMuscleTorque = RCC_VEC3V(minTorque);
		static_cast<phJoint3DofType*>(m_Type.ptr)->m_MaxMuscleTorque = RCC_VEC3V(maxTorque);
	}

	inline void phJoint3Dof::SetMinAndMaxMuscleTorque (const Vector3& maxTorqueMag)
	{
		Vec3V v_maxTorqueMag = RCC_VEC3V(maxTorqueMag);

		static_cast<phJoint3DofType*>(m_Type.ptr)->m_MinMuscleTorque = Negate( v_maxTorqueMag );
		static_cast<phJoint3DofType*>(m_Type.ptr)->m_MaxMuscleTorque = v_maxTorqueMag;
	}

	inline const Vector3& phJoint3Dof::GetMuscleTargetAngle () const
	{
		return RCC_VECTOR3(m_MuscleTargetAngle);
	}

	inline const Vector3& phJoint3Dof::GetMuscleTargetSpeed () const
	{
		return RCC_VECTOR3(m_MuscleTargetSpeed);
	}

	inline const Vector3& phJoint3Dof::GetMuscleAngleStrength () const
	{
		return RCC_VECTOR3(m_MuscleAngleStrength);
	}

	inline const Vector3& phJoint3Dof::GetMuscleSpeedStrength () const
	{
		return RCC_VECTOR3(m_MuscleSpeedStrength);
	}

	inline const Vector3& phJoint3Dof::GetMaxMuscleTorque () const
	{
		return RCC_VECTOR3(static_cast<phJoint3DofType*>(m_Type.ptr)->m_MaxMuscleTorque);
	}

	inline const Vector3& phJoint3Dof::GetMinMuscleTorque () const
	{
		return RCC_VECTOR3(static_cast<phJoint3DofType*>(m_Type.ptr)->m_MinMuscleTorque);
	}


	VEC3_INLINE void phJoint3Dof::TranslateTensor ( const phArticulatedBodyInertia& src, phArticulatedBodyInertia *dest, Vector3::Vector3Param pos )
	{
		Mat33V v_crossProdMat;
		CrossProduct( v_crossProdMat, RCC_VEC3V(pos) );

		// Saving/accumulating variables locally helps reduce extra loads/stores.
		Mat33V v_srcCrossInertia = src.m_CrossInertia;
		Mat33V v_srcInvInertia = src.m_InvInertia;
		Mat33V v_srcMass = src.m_Mass;
		Mat33V v_destCrossInertia;
		Mat33V v_destInvInertia;

		// Change the inertia matrix by translating origin by "pos"
		dest->m_Mass = v_srcMass;

		v_destCrossInertia = v_crossProdMat;
		v_destCrossInertia = -v_destCrossInertia;							// Cross Product Matrix for -pos
		Multiply( v_destCrossInertia, v_srcMass, v_destCrossInertia );		// -(pos \cross)(src.M)
		v_destCrossInertia += v_srcCrossInertia;

		v_destInvInertia = v_destCrossInertia;
		Multiply( v_destInvInertia, v_crossProdMat, v_destInvInertia );
		UnTransformOrtho( v_crossProdMat, v_srcCrossInertia, v_crossProdMat );	// (JointPosition \cross)((src.H)^T).
		v_destInvInertia -= v_crossProdMat;
		v_destInvInertia += v_srcInvInertia;

		dest->m_CrossInertia = v_destCrossInertia;
		dest->m_InvInertia = v_destInvInertia;
	}

	__forceinline void phJoint3Dof::TransformInertiaByASdownup( phArticulatedBodyInertia& dest ) const
	{
		PrefetchDC( &dest );
		PrefetchDC( &(dest.m_InvInertia) );

		if ( FloatEqualsZero(&m_Stiffness) && !MuscleDriveSpeed() )
		{
			Mat33V v_destMass;
			Mat33V zzz = DownIAstiffnessInvH;
			Transpose( v_destMass, DownInertiaJC.m_CrossInertia );
			Multiply( v_destMass, zzz, v_destMass );
			v_destMass = DownInertiaJC.m_Mass - v_destMass;
			dest.m_Mass = v_destMass;
			TranslateTensorUpperTriangular( dest );
		}
		else
		{
			// Save locals. By doing this, 26 vector loads were reduced to 17.

			// INs.
			Mat33V v_DownInertiaJCCrossInertia		= DownInertiaJC.m_CrossInertia;
			Mat33V v_DownInertiaJCMass				= DownInertiaJC.m_Mass;
			Mat33V v_DownInertiaJCInvInertia		= DownInertiaJC.m_InvInertia;
			Mat33V v_DownIAstiffnessInvH			= DownIAstiffnessInvH;
			Mat33V v_DownIAstiffnessInvI			= DownIAstiffnessInvI;
			// OUTs.
			Mat33V v_Mass;
			Mat33V v_InvInertia;
			Mat33V v_CrossInertia;

			Transpose( v_Mass, v_DownInertiaJCCrossInertia );
			Multiply( v_Mass, v_DownIAstiffnessInvH, v_Mass );
			v_Mass = -v_Mass;
			v_Mass += v_DownInertiaJCMass;

			v_InvInertia = v_DownInertiaJCInvInertia;
			Multiply( v_InvInertia, v_DownIAstiffnessInvI, v_InvInertia );
			v_InvInertia = -v_InvInertia;
			v_InvInertia += v_DownInertiaJCInvInertia;

			v_CrossInertia = v_DownInertiaJCInvInertia;
			Multiply( v_CrossInertia, v_DownIAstiffnessInvH, v_CrossInertia );
			v_CrossInertia = -v_CrossInertia;
			v_CrossInertia += v_DownInertiaJCCrossInertia;

			phArticulatedBodyInertia output;
			output.m_Mass = v_Mass;
			output.m_InvInertia = v_InvInertia;
			output.m_CrossInertia = v_CrossInertia;
			TranslateTensor( output, &dest, -RCC_VECTOR3(JointPosition) );
		}
	}

	__forceinline void phJoint3Dof::TransformInertiaByASupdown( phArticulatedBodyInertia& dest ) const
	{
		if ( FloatEqualsZero(&m_Stiffness) && !MuscleDriveSpeed() )
		{
			// Save locals.
			// INs.
			Mat33V v_UpInertiaJCCrossInertia		= UpInertiaJC.m_CrossInertia;
			Mat33V v_UpInertiaJCMass				= UpInertiaJC.m_Mass;
			Mat33V v_UpIAstiffnessInvH				= UpIAstiffnessInvH;
			// OUTs.
			Mat33V v_Mass;

			Mat33V zzz = v_UpIAstiffnessInvH;
			Transpose( v_Mass, v_UpInertiaJCCrossInertia );
			Multiply( v_Mass, zzz, v_Mass );
			v_Mass = v_UpInertiaJCMass - v_Mass;
			dest.m_Mass = v_Mass;
			TranslateTensorUpperTriangular( dest );
		}
		else
		{
			// Save locals.
			// INs.
			Mat33V v_UpInertiaJCCrossInertia		= UpInertiaJC.m_CrossInertia;
			Mat33V v_UpInertiaJCMass				= UpInertiaJC.m_Mass;
			Mat33V v_UpInertiaJCInvInertia			= UpInertiaJC.m_InvInertia;
			Mat33V v_UpIAstiffnessInvH				= UpIAstiffnessInvH;
			Mat33V v_UpIAstiffnessInvI				= UpIAstiffnessInvI;
			// OUTs.
			Mat33V v_Mass;
			Mat33V v_InvInertia;
			Mat33V v_CrossInertia;

			Transpose( v_Mass, v_UpInertiaJCCrossInertia );
			Multiply( v_Mass, v_UpIAstiffnessInvH, v_Mass );
			v_Mass = v_UpInertiaJCMass - v_Mass;
			v_InvInertia = v_UpInertiaJCInvInertia;
			Multiply( v_InvInertia, v_UpIAstiffnessInvI, v_InvInertia );
			v_InvInertia = v_UpInertiaJCInvInertia - v_InvInertia;
			v_CrossInertia = v_UpInertiaJCInvInertia;
			Multiply( v_CrossInertia, v_UpIAstiffnessInvH, v_CrossInertia );
			v_CrossInertia = v_UpInertiaJCCrossInertia - v_CrossInertia;

			phArticulatedBodyInertia output;
			output.m_Mass = v_Mass;
			output.m_InvInertia = v_InvInertia;
			output.m_CrossInertia = v_CrossInertia;
			TranslateTensor( output, &dest, -RCC_VECTOR3(JointPosition) );
		}
	}

	inline void phJoint3Dof::SetLean1TargetAngle (float leanTarget) { m_MuscleTargetAngle.SetXf(leanTarget); }
	inline void phJoint3Dof::SetLean2TargetAngle (float leanTarget) { m_MuscleTargetAngle.SetYf(leanTarget); }
	inline void phJoint3Dof::SetTwistTargetAngle (float twistTarget) { m_MuscleTargetAngle.SetZf(twistTarget); }
	inline void phJoint3Dof::SetLean1TargetSpeed (float leanTarget) { m_MuscleTargetSpeed.SetXf(leanTarget); }
	inline void phJoint3Dof::SetLean2TargetSpeed (float leanTarget) { m_MuscleTargetSpeed.SetYf(leanTarget); }
	inline void phJoint3Dof::SetTwistTargetSpeed (float twistTarget) { m_MuscleTargetSpeed.SetZf(twistTarget); }
	inline void phJoint3Dof::SetMaxLean1Torque (float maxTorque) { static_cast<phJoint3DofType*>(m_Type.ptr)->m_MaxMuscleTorque.SetXf(maxTorque); }
	inline void phJoint3Dof::SetMaxLean2Torque (float maxTorque) { static_cast<phJoint3DofType*>(m_Type.ptr)->m_MaxMuscleTorque.SetYf(maxTorque); }
	inline void phJoint3Dof::SetMaxTwistTorque (float maxTorque) { static_cast<phJoint3DofType*>(m_Type.ptr)->m_MaxMuscleTorque.SetZf(maxTorque); }
	inline void phJoint3Dof::SetMinLean1Torque (float minTorque) { static_cast<phJoint3DofType*>(m_Type.ptr)->m_MinMuscleTorque.SetXf(minTorque); }
	inline void phJoint3Dof::SetMinLean2Torque (float minTorque) { static_cast<phJoint3DofType*>(m_Type.ptr)->m_MinMuscleTorque.SetYf(minTorque); }
	inline void phJoint3Dof::SetMinTwistTorque (float minTorque) { static_cast<phJoint3DofType*>(m_Type.ptr)->m_MinMuscleTorque.SetZf(minTorque); }

} // namespace rage

#endif    // ARTICULATED_BODY_JOINT_3_DOF
