//
// pharticulated/joint3dof.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

// Implements a s-DOF rotational joint, a "ball-and-socket joint".

#include "joint3dof.h"

#include "articulatedbody.h"
#include "bodyinertia.h"
#include "bodypart.h"

#include "math/angmath.h"
#include "physics/simulator.h"
#include "grprofile/drawmanager.h"

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

namespace rage {


	EXT_PFD_DECLARE_ITEM(ArticulatedMuscleSpeedTorque);
	EXT_PFD_DECLARE_ITEM(ArticulatedMuscleAngleTorque);

#if __DECLARESTRUCT
	void phJoint3DofType::DeclareStruct(datTypeStruct &s)
	{
		phJointType::DeclareStruct(s);
		SSTRUCT_BEGIN_BASE(phJoint3DofType, phJointType)
			SSTRUCT_FIELD(phJoint3DofType, m_HardFirstLeanAngleMax)
			SSTRUCT_FIELD(phJoint3DofType, m_HardSecondLeanAngleMax)
			SSTRUCT_FIELD(phJoint3DofType, m_HardTwistAngleMax)
#ifdef USE_SOFT_LIMITS
			SSTRUCT_FIELD(phJoint3DofType, m_SoftLimitRatio)
#endif
			SSTRUCT_FIELD(phJoint3DofType, m_TwistOffset)
			SSTRUCT_FIELD(phJoint3DofType, m_UseChildForTwistAxis)
			SSTRUCT_IGNORE(phJoint3DofType, m_Pad)
			SSTRUCT_FIELD(phJoint3DofType, m_MaxMuscleTorque)
			SSTRUCT_FIELD(phJoint3DofType, m_MinMuscleTorque)
#ifdef USE_SOFT_LIMITS
			SSTRUCT_FIELD(phJoint3DofType, m_SoftLimitLeanStrength)
			SSTRUCT_FIELD(phJoint3DofType, m_SoftLimitTwistStrength)
#endif
			SSTRUCT_IGNORE(phJoint3DofType, m_Pad1)
			SSTRUCT_END(phJoint3DofType)
	}
#endif // __DECLARESTRUCT

	phJoint3DofType::phJoint3DofType (datResource &rsc) : phJointType(rsc)
	{
	}

	phJoint3DofType::phJoint3DofType()
	{
#ifdef USE_SOFT_LIMITS
		m_SoftLimitRatio = 1.0f;					// 1.0 corresponds to soft limits that are equal to hard limits, and are therefore disabled
#endif
		m_HardFirstLeanAngleMax = FLT_MAX;
		m_HardSecondLeanAngleMax = FLT_MAX;
		m_HardTwistAngleMax = FLT_MAX;
		m_UseChildForTwistAxis = false;

		m_MaxMuscleTorque = Vec3V(1.0e8f,1.0e8f,1.0e8f);
		m_MinMuscleTorque = Vec3V(-1.0e8f,-1.0e8f,-1.0e8f);
#ifdef USE_SOFT_LIMITS
		m_SoftLimitLeanStrength = 1.0f;
		m_SoftLimitTwistStrength = 1.0f;
#endif
	}

	phJointType* phJoint3DofType::Clone()
	{
		phJoint3DofType* clone = rage_new phJoint3DofType;
		*clone = *this;
		return clone;
	}

	float phJoint3Dof::ComputeCurrentLean (phArticulatedBody *body)
	{
		// Get the joint orientation for the parent in global coordinates.
		m_JointPoseWorld = GetParentLink(body)->GetMatrix();
		RC_MATRIX34(m_JointPoseWorld).Dot3x3(RCC_MATRIX34(m_Type->m_OrientParent));

		// Parent's central axis in global coordinates
		Vector3 axisP(m_JointPoseWorld.GetM20f(), m_JointPoseWorld.GetM21f(), m_JointPoseWorld.GetM22f());

		Vector3 axisC(
			Get3DofType().m_OrientChild.GetM20f(),
			Get3DofType().m_OrientChild.GetM21f(),
			Get3DofType().m_OrientChild.GetM22f());
		MAT34V_TO_MATRIX34(GetChildLink(body)->GetMatrix()).UnTransform3x3( axisC );				// Child's central axis in global coordinates

		RC_VECTOR3(m_LeanAxis) = axisP;
		RC_VECTOR3(m_LeanAxis).Cross(axisC);			// Axis of rotation that causes the lean

		float normSq = RC_VECTOR3(m_LeanAxis).Mag2();
		if ( normSq>=VERY_SMALL_FLOAT )
		{
			float norm = sqrtf(normSq);
			RC_VECTOR3(m_LeanAxis) /= norm;			// Normalize the axis of rotation for the lean
			CurrentLeanAngle = atan2f( norm, axisP.Dot(axisC) );
			// Axis for undoing twist is midway from axisP to axisC;
			RC_VECTOR3(m_TwistAxis) = axisP;
			RC_VECTOR3(m_TwistAxis) += axisC;
			m_TwistFactor = RCC_VECTOR3(m_TwistAxis).Mag();
			if ( FPAbs(m_TwistFactor)<VERY_SMALL_FLOAT )
			{
				// In this case, the lean is 180 degrees, a degenerate case
				// Only happens in cases of roundoff error, since normSq should have been zero above.
				RC_VECTOR3(m_TwistAxis) = axisP;
				m_TwistFactor = 1.0f;
			}
			else
			{
				// This is the usual case (general position)
				m_TwistFactor = 2.0f/m_TwistFactor;		// Now equals 1.0/cos(CurrentLeanAngle/2.0).
				RC_VECTOR3(m_TwistAxis) *= 0.5f*m_TwistFactor;		// Normalize the TwistAxis
			}
		}
		else
		{
			CurrentLeanAngle = 0.0f;
			m_LeanAxis = GetOrtho( RCC_VEC3V(axisP) );	// Use arbitrary orthogonal axis for the lean axis
			m_TwistAxis = RCC_VEC3V(axisP);
			m_TwistFactor = 1.0f;
		}

		if (Get3DofType().m_UseChildForTwistAxis)
		{
			// Change the twist axis to match the child body part's axis.
			m_TwistAxis = VECTOR3_TO_VEC3V(axisC);
		}

		m_ThirdAxis = Cross(m_TwistAxis,m_LeanAxis);

		return CurrentLeanAngle;
	}


	void phJoint3Dof::ComputeCurrentLeanAndTwist (phArticulatedBody *body)
	{
		// Transform the joint orientation from the parent's coordinates to global coordinates.
		Mat34V jointWorld;
		Transform3x3(jointWorld,m_Type->m_OrientParent,GetParentLink(body)->GetMatrix());
		Mat44V axesAndAngles = geomVectors::ComputeLeanAndTwistAngles(GetParentLink(body)->GetMatrix(),GetChildLink(body)->GetMatrix(),jointWorld,
			Get3DofType().m_OrientChild,Get3DofType().m_UseChildForTwistAxis);
		m_LeanAxis = axesAndAngles.GetCol0().GetXYZ();
		m_ThirdAxis = axesAndAngles.GetCol1().GetXYZ();
		m_TwistAxis = axesAndAngles.GetCol2().GetXYZ();
		CurrentLeanAngle = axesAndAngles.GetCol3().GetXf();
		m_TwistAngle = axesAndAngles.GetCol3().GetZf();
		m_TwistFactor = axesAndAngles.GetCol3().GetWf();
		Transform3x3(m_JointPoseWorld,m_Type->m_OrientParent,GetParentLink(body)->GetMatrix());
	}

	bool phJoint3Dof::CheckHardLeanLimitStatus (phArticulatedBody *body)
	{
		if (!m_DifferentMaxLeanAngles)
		{
			float t = CurrentLeanAngle-(Get3DofType().m_HardFirstLeanAngleMax + m_Lean1Adjustment);
			if (t>=0.0f)
			{
				m_CurrentHardLeanLimit = Get3DofType().m_HardFirstLeanAngleMax + m_Lean1Adjustment;
				ScalarV rateOfLean;
				rateOfLean	=	Dot( m_LeanAxis, body->GetAngularVelocity(GetChildLinkIndex() ));
				rateOfLean	-=	Dot( m_LeanAxis, body->GetAngularVelocity(GetParentLinkIndex() ));
				StoreScalar32FromScalarV( CurrentRateOfLean, rateOfLean );
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			// Dot product of lean direction, in x and y directions, respectively.
			float leanDotParentX = m_LeanAxis.GetXf()*m_JointPoseWorld.GetM10f() + m_LeanAxis.GetYf()*m_JointPoseWorld.GetM11f() + m_LeanAxis.GetZf()*m_JointPoseWorld.GetM12f();
			float leanDotParentY = m_LeanAxis.GetXf()*m_JointPoseWorld.GetM00f() + m_LeanAxis.GetYf()*m_JointPoseWorld.GetM01f() + m_LeanAxis.GetZf()*m_JointPoseWorld.GetM02f();
			float hardLimitFractionSq = square(CurrentLeanAngle)*
				(square(leanDotParentX/(Get3DofType().m_HardFirstLeanAngleMax + m_Lean1Adjustment)) + 
				square(leanDotParentY/(Get3DofType().m_HardSecondLeanAngleMax + m_Lean2Adjustment)));
			if (hardLimitFractionSq<1.0f)
			{
				return false;
			}

			m_CurrentHardLeanLimit = CurrentLeanAngle * FPInvSqrt(hardLimitFractionSq);
			ScalarV rateOfLean;
			rateOfLean	=	Dot( m_LeanAxis, body->GetAngularVelocity(GetChildLinkIndex() ));
			rateOfLean	-=	Dot( m_LeanAxis, body->GetAngularVelocity(GetParentLinkIndex() ));
			StoreScalar32FromScalarV( CurrentRateOfLean, rateOfLean );
			return true;
		}
	}

	// ***************************************************************************
	// Based on the Parent's position and velocity,
	// Modify the Child link's position and velocity to match it properly
	void phJoint3Dof::MatchChildToParentPositionAndVelocity(phArticulatedBody *body)
	{
		// Save locals.
		Mat34V v_ParentLinkMatrix = GetParentLink(body)->GetMatrix();
		Mat34V v_ChildLinkMatrix = GetChildLink(body)->GetMatrix();

		// Match positions
		Vec3V jointPosP;
		jointPosP = UnTransform3x3Ortho( v_ParentLinkMatrix, GetPositionParentV() );
		jointPosP += v_ParentLinkMatrix.GetCol3();									// Position of joint, global coords

		Vec3V jointPosC;
		jointPosC = UnTransform3x3Ortho( v_ChildLinkMatrix, GetPositionChildV() );		// Position relative to child CM, global coords

		Vec3V childLinkPos = jointPosP;
		childLinkPos -= jointPosC;
		v_ChildLinkMatrix.SetCol3( childLinkPos );
		GetChildLink(body)->SetMatrix( RCC_MATRIX34(v_ChildLinkMatrix) );

		// Match linear velocity
		Vec3V velP = jointPosP;
		velP -= v_ParentLinkMatrix.GetCol3();
		velP = Cross(body->GetAngularVelocityNoProp(GetParentLinkIndex()), velP);		// Velocity	of joint (rel to parent CM)
		velP += body->GetLinearVelocityNoProp(GetParentLinkIndex());					// Actual velocity
		Vec3V velC = jointPosC;																// Position of joint rel to CM (global coords)
		velC = Cross(body->GetAngularVelocityNoProp(GetChildLinkIndex()), velC);		// Velocity of joint rel to child CM
		Vec3V childLinearVel = velP;
		childLinearVel -= velC;
		body->SetLinearVelocityNoDownVelAdjust(GetChildLinkIndex(), childLinearVel);
	}

	int phJoint3Dof::ComputeNumHardJointLimitDofs (phArticulatedBody *body)
	{
		if (!m_HasTwistLimit)
		{
			// Check only lean limits
			if (NeedToKnowTwistValues())
			{
				ComputeCurrentLeanAndTwist(body);
				m_LeanLimitExceeded = CheckHardLeanLimitStatus(body);
				m_TwistRate = m_TwistFactor*Subtract(Dot(m_TwistAxis,body->GetAngularVelocity(GetChildLinkIndex())),Dot(m_TwistAxis,body->GetAngularVelocity(GetParentLinkIndex()))).Getf();
			}
			else
			{
				ComputeCurrentLean(body);
				m_LeanLimitExceeded = CheckHardLeanLimitStatus(body);
			}
			// There are no twist limits, so return that they are not exceeded
			m_TwistLimitExceeded = false;
			return (m_LeanLimitExceeded ? 2 : 0);
		}

		ComputeCurrentLeanAndTwist(body);
		m_LeanLimitExceeded = CheckHardLeanLimitStatus(body);
		int numJointLimitDofs = m_LeanLimitExceeded ? 2 : 0;

		// float t;
		m_TwistRate = m_TwistFactor*Subtract(Dot(m_TwistAxis,body->GetAngularVelocity(GetChildLinkIndex())),Dot(m_TwistAxis,body->GetAngularVelocity(GetParentLinkIndex()))).Getf();
		if ( (/*t =*/ (m_TwistAngle-(Get3DofType().m_HardTwistAngleMax + m_TwistAdjustment))) >= 0.0f )
		{
			// if exceeds limit in positive direction
			m_CurrentHardTwistLimit = Get3DofType().m_HardTwistAngleMax + m_TwistAdjustment;
			m_TwistLimitExceeded = true;
			numJointLimitDofs++;
		}
		else if ( (/*t =*/ (m_TwistAngle+Get3DofType().m_HardTwistAngleMax + m_TwistAdjustment)) <= 0.0f )
		{
			// if exceeds limit in negative direction
			m_CurrentHardTwistLimit = -(Get3DofType().m_HardTwistAngleMax + m_TwistAdjustment);
			m_TwistLimitExceeded = true;
			numJointLimitDofs++;
		}
		else
		{
			m_TwistLimitExceeded = false;
		}

		return numJointLimitDofs;
	}

	extern float g_JointLimitElasticity;

	void phJoint3Dof::GetJointLimitDetailedInfo(phArticulatedBody *body, int limitNum, int& jointLimitID, float& dofExcess, float& response)
	{
		int leanDelta = m_LeanLimitExceeded ? 2 : 0;
		if (limitNum==leanDelta)
		{
			// Its the twist axis
			if (m_CurrentHardTwistLimit>0.0f)
			{
				// Twist exceeded in positive direction
				jointLimitID = 2;
				dofExcess = m_TwistAngle - m_CurrentHardTwistLimit;
			}
			else
			{
				// Twist exceeded in positive direction
				jointLimitID = 3;
				dofExcess = m_CurrentHardTwistLimit - m_TwistAngle;
			}
		}
		else if (limitNum==0)
		{
			// Lean limit exceeded
			jointLimitID = 0;
			dofExcess = CurrentLeanAngle - m_CurrentHardLeanLimit;
		}
		else
		{
			// The "third axis" direction
			jointLimitID = 1;
			dofExcess = 0.0f;
			float jointAngVel = GetJointAngularVelocityOnAxis(body, m_ThirdAxis) * (0.5f + g_JointLimitElasticity * 0.5f);
			if (jointAngVel<0.0f)
			{
				m_ThirdAxis = Negate(m_ThirdAxis);
			}
		}

		response = phJoint3Dof::GetJointLimitResponse(body, jointLimitID);
	}

	float phJoint3Dof::GetJointLimitResponse (phArticulatedBody *body, int limitDirectionIndex)
	{
		switch (limitDirectionIndex)
		{
		case 0:
			{
				// Lean axis
				return ResponseToUnitTorqueOnAxis(body, m_LeanAxis).Getf();
			}
		case 1:
			{
				// Third axis
				return ResponseToUnitTorqueOnAxis(body, m_ThirdAxis).Getf();
			}
		case 2:
		case 3:
			{
				// Positive or negative twist
				return m_TwistFactor*ResponseToUnitTorqueOnAxis(body, m_TwistAxis).Getf();
			}
		}

		return 0.0f;
	}


	void phJoint3Dof::ApplyJointImpulse (phArticulatedBody *body, int jointLimitID, ScalarV_In impulse)
	{
		Vec3V angImpulse;
		switch (jointLimitID)
		{
		case 0:		// Lean axis
			angImpulse = Scale(m_LeanAxis, impulse);
			ApplyAngImpulse(body, angImpulse.GetIntrin128());
			break;
		case 1:		// Third axis
			angImpulse = Scale(m_ThirdAxis, impulse);
			ApplyAngImpulse(body, angImpulse.GetIntrin128());
			break;
		case 2:		// Positive twist
			angImpulse = Scale(m_TwistAxis, impulse);
			ApplyAngImpulse(body, angImpulse.GetIntrin128());
			break;
		case 3:		// Negative twist
			angImpulse = Scale(m_TwistAxis, -impulse);
			ApplyAngImpulse(body, angImpulse.GetIntrin128());
			break;
		}
	}

	void phJoint3Dof::GetJointLimitAxis( phArticulatedBody *body, int jointLimitID, Vector3& axis, Vector3& position ) const
	{
		position = GetJointPosition(body);
		switch ( jointLimitID )
		{
		case 0:		// Lean axis}
			{
				axis = RCC_VECTOR3(m_LeanAxis);
				break;
			}

		case 1:
			{
				axis = RCC_VECTOR3(m_ThirdAxis);
				break;
			}

		case 2:
			{
				axis = RCC_VECTOR3(m_TwistAxis);
				break;
			}

		case 3:
			{
				axis = RCC_VECTOR3(m_TwistAxis);
				axis.Negate();
			}
		}
	}

	void phJoint3Dof::GetJointLimitUnitImpulse( int jointLimitID, phPhaseSpaceVector& unitImpulseSpatial )
	{
		unitImpulseSpatial.omega = Vec3V(V_ZERO);
		switch ( jointLimitID )
		{
		case 0:		// Lean axis}
			{
				unitImpulseSpatial.trans = m_LeanAxis;
				break;
			}

		case 1:
			{
				unitImpulseSpatial.trans = m_ThirdAxis;
				break;
			}

		case 2:
			{
				unitImpulseSpatial.trans = m_TwistAxis;
				break;
			}

		case 3:
			{
				unitImpulseSpatial.trans = m_TwistAxis;
				unitImpulseSpatial.trans = Negate(unitImpulseSpatial.trans);
			}
		}
	}


	Vec3V_Out phJoint3Dof::ComputeCurrentLeanAngles (phArticulatedBody *body)
	{
		// Find the parent orientation. 
#ifndef USE_SOFT_LIMITS
		ComputeCurrentLeanAndTwist(body);  // If soft limits are used, these values were computed earlier
#endif
		m_JointPoseWorld = GetParentLink(body)->GetMatrix();
		RC_MATRIX34(m_JointPoseWorld).Dot3x3(RCC_MATRIX34(m_Type->m_OrientParent));
		return geomVectors::ComputeLeanAngles(m_JointPoseWorld,m_LeanAxis,ScalarVFromF32(CurrentLeanAngle),ScalarVFromF32(m_TwistAngle));
	}


	void phJoint3Dof::ComputeCurrentLeanAnglesAndRates (phArticulatedBody *body, Vec3V_InOut lean12Twist, Vec3V_InOut lean12TwistRate)
	{
		// Find the joint orientation in world coordinates.
#ifndef USE_SOFT_LIMITS
		ComputeCurrentLeanAndTwist(body);  // If soft limits are used, these values were computed earlier
#endif
		m_JointPoseWorld = GetParentLink(body)->GetMatrix();
		RC_MATRIX34(m_JointPoseWorld).Dot3x3(RCC_MATRIX34(m_Type->m_OrientParent));

		Mat34V jointTranspose;
		Transpose3x3(jointTranspose,m_JointPoseWorld);

		// Get the two lean axes and angles.
		Vec3V lean1Axis = jointTranspose.GetCol1();
		Vec3V lean2Axis = -jointTranspose.GetCol0();
		ScalarV leanMtx22a = Dot(m_LeanAxis,lean1Axis);
		ScalarV leanMtx22c = Dot(m_LeanAxis,lean2Axis);

		// Put the two lean angles and the twist angle in a vector.
		lean12Twist = Vec3V(Scale(ScalarVFromF32(CurrentLeanAngle),leanMtx22a),Scale(ScalarVFromF32(CurrentLeanAngle),leanMtx22c),ScalarVFromF32(m_TwistAngle));

		CurrentRateOfLean = GetJointAngularVelocityOnAxis(body, m_LeanAxis);
		m_TwistRate = GetJointAngularVelocityOnAxis(body, m_TwistAxis)*m_TwistFactor;

		ScalarV rateLean = ScalarVFromF32(CurrentRateOfLean);
		ScalarV rateTwist = ScalarVFromF32(m_TwistRate);
		ScalarV rateThird = Scale(ScalarVFromF32(GetJointAngularVelocityOnAxis(body, m_ThirdAxis)),Invert(ScalarVFromF32(m_TwistFactor)));
		lean12TwistRate = Vec3V(Subtract(Scale(rateLean,leanMtx22a),Scale(rateThird,leanMtx22c)),Add(Scale(rateLean,leanMtx22c),Scale(rateThird,leanMtx22a)),rateTwist);
		VALIDATE_PHYSICS_ASSERT(IsEqualAll(lean12TwistRate,lean12TwistRate));
	}

	void phJoint3Dof::Freeze ()
	{
		CurrentRateOfLean = 0.0f;
		m_TwistRate = 0.0f;
	}


	void phJoint3Dof::SetAxis (phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir)
	{
		Vec3V xDir,yDir;
		GetOrtho(RCC_VEC3V(axisDir),xDir,yDir);
		SetAxis(body, axisPosition,axisDir,RCC_VECTOR3(xDir),RCC_VECTOR3(yDir));
	}


	// Angles measured from the current configuration
	void phJoint3Dof::SetAxis (phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir, 
		float maxLeanAngle, float maxTwistAngle )
	{
		Vec3V xDir,yDir;
		GetOrtho(RCC_VEC3V(axisDir),xDir,yDir);
		SetAxis(body, axisPosition,axisDir,RCC_VECTOR3(xDir),RCC_VECTOR3(yDir));
		SetAngleLimits(maxLeanAngle,maxTwistAngle);
	}


	void phJoint3Dof::SetAngleLimits (float maxLeanAngle, float maxTwistAngle)
	{
		// Take into account the allowed penetration when setting the joint limits, so the users' chosen limits will appear exactly correct
		Get3DofType().m_HardFirstLeanAngleMax = FPMax(FLT_EPSILON, maxLeanAngle - phSimulator::GetAllowedAnglePenetration());
		Get3DofType().m_HardSecondLeanAngleMax = FPMax(FLT_EPSILON, maxLeanAngle - phSimulator::GetAllowedAnglePenetration());
		m_DifferentMaxLeanAngles = false;
		Get3DofType().m_HardTwistAngleMax = FPMax(FLT_EPSILON, maxTwistAngle - phSimulator::GetAllowedAnglePenetration());
		m_HasTwistLimit = (maxTwistAngle!=FLT_MAX);

		// Make sure that if there is a twist limit, the lean and twist limits are all less than PI.  
		Assert(!m_HasTwistLimit || (maxLeanAngle<PI && maxLeanAngle<PI && 
			Get3DofType().m_HardTwistAngleMax<PI));

		m_Lean1Adjustment = 0.0f;
		m_Lean2Adjustment = 0.0f;
		m_TwistAdjustment = 0.0f;
	}


	void phJoint3Dof::SetThreeAngleLimits (float maxLean1Angle, float maxLean2Angle, float maxTwistAngle, bool adjustOnly)
	{
		if (m_Type)
		{
			if (adjustOnly)
			{
				float newLean1 = FPMax(FLT_EPSILON, maxLean1Angle - phSimulator::GetAllowedAnglePenetration());
				float newLean2 = FPMax(FLT_EPSILON, maxLean2Angle - phSimulator::GetAllowedAnglePenetration());
				m_DifferentMaxLeanAngles = (newLean1!=newLean2);
				float newTwist = FPMax(FLT_EPSILON, maxTwistAngle - phSimulator::GetAllowedAnglePenetration());

				m_Lean1Adjustment = newLean1 - Get3DofType().m_HardFirstLeanAngleMax;
				m_Lean2Adjustment = newLean2 - Get3DofType().m_HardSecondLeanAngleMax;
				m_TwistAdjustment = newTwist - Get3DofType().m_HardTwistAngleMax;
			}
			else
			{
				// Take into account the allowed penetration when setting the joint limits, so the users' chosen limits will appear exactly correct
				Get3DofType().m_HardFirstLeanAngleMax = FPMax(FLT_EPSILON, maxLean1Angle - phSimulator::GetAllowedAnglePenetration());
				Get3DofType().m_HardSecondLeanAngleMax = FPMax(FLT_EPSILON, maxLean2Angle - phSimulator::GetAllowedAnglePenetration());
				m_DifferentMaxLeanAngles = (maxLean1Angle!=maxLean2Angle);
				Get3DofType().m_HardTwistAngleMax = FPMax(FLT_EPSILON, maxTwistAngle - phSimulator::GetAllowedAnglePenetration());

				m_Lean1Adjustment = 0.0f;
				m_Lean2Adjustment = 0.0f;
				m_TwistAdjustment = 0.0f;
			}
			m_HasTwistLimit = ((Get3DofType().m_HardTwistAngleMax + m_TwistAdjustment) !=FLT_MAX);

			// Make sure that if there is a twist limit, the lean and twist limits are all less than PI.  
			Assert(!m_HasTwistLimit || (maxLean1Angle<PI && maxLean1Angle<PI));
			Assertf(!m_HasTwistLimit || (Get3DofType().m_HardTwistAngleMax + m_TwistAdjustment)<PI, "m_HardTwistAngleMax is %f, m_TwistAdjustment is %f",
				Get3DofType().m_HardTwistAngleMax, m_TwistAdjustment);
		}
	}
#ifdef USE_SOFT_LIMITS
	void phJoint3Dof::SetSoftLimitStrengths (float leanStrength, float twistStrength )
	{
		static_cast<phJoint3DofType*>(m_Type.ptr)->m_SoftLimitLeanStrength = leanStrength;
		static_cast<phJoint3DofType*>(m_Type.ptr)->m_SoftLimitTwistStrength = twistStrength;
	}
#endif
	void phJoint3Dof::GetThreeAngleLimits (float& maxLean1Angle, float& maxLean2Angle, float& maxTwistAngle) const
	{
		maxLean1Angle = static_cast<phJoint3DofType*>(m_Type.ptr)->m_HardFirstLeanAngleMax + m_Lean1Adjustment;
		maxLean2Angle = static_cast<phJoint3DofType*>(m_Type.ptr)->m_HardSecondLeanAngleMax + m_Lean2Adjustment;
		maxTwistAngle = static_cast<phJoint3DofType*>(m_Type.ptr)->m_HardTwistAngleMax + m_TwistAdjustment;
	}


	void phJoint3Dof::SetAxis(phArticulatedBody* body, const Vector3& axisPosition, const Vector3& axisDir,
		const Vector3& firstLeanDir, float firstMaxLeanAngle, float secondMaxLeanAngle, float maxTwistAngle)
	{
		// Use cross product to get third axis direction.
		Vec3V thirdAxisDir = Cross(RCC_VEC3V(axisDir),RCC_VEC3V(firstLeanDir));
		SetAxis(body, axisPosition,axisDir,firstLeanDir,RCC_VECTOR3(thirdAxisDir));
		SetThreeAngleLimits(firstMaxLeanAngle,secondMaxLeanAngle,maxTwistAngle);
	}


	void phJoint3Dof::SetAxis( phArticulatedBody* body, const Vector3& axisPosition, const Vector3& axisDir,
		const Vector3& firstLeanDir, const Vector3& secondLeanDir)
	{
		AssertMsg(axisDir.Mag2()>square(0.99f) && axisDir.Mag2()<square(1.01f),"SetAxis was called with a non-unit axis.");
		AssertMsg(firstLeanDir.Mag2()>square(0.99f) && firstLeanDir.Mag2()<square(1.01f),"SetAxis was called with a non-unit axis.");
		AssertMsg(secondLeanDir.Mag2()>square(0.99f) && secondLeanDir.Mag2()<square(1.01f),"SetAxis was called with a non-unit axis.");

		// Save variables locally.
		Vec3V v_axisPosition = RCC_VEC3V(axisPosition);
		Mat34V v_parentLinkMatrix = GetParentLink(body)->GetMatrix();
		Mat34V v_childLinkMatrix = GetChildLink(body)->GetMatrix();
		Mat34V v_outOrientParent;
		Mat34V v_outOrientChild;

		Vec3V v_firstLeanDir = RCC_VEC3V(firstLeanDir);
		Vec3V v_secondLeanDir = RCC_VEC3V(secondLeanDir);
		Vec3V v_axisDir = RCC_VEC3V(axisDir);
		Mat34V axisCoords( v_firstLeanDir, v_secondLeanDir, v_axisDir, v_axisDir ); // (don't care about last col)
		Transpose3x3( axisCoords, axisCoords );

		Vec3V position = Subtract(v_axisPosition,v_parentLinkMatrix.GetCol3());
		v_outOrientParent = v_parentLinkMatrix;
		Transpose3x3( v_outOrientParent, v_outOrientParent );
		Transform3x3( v_outOrientParent, axisCoords, v_outOrientParent );
		m_Type->m_OrientParent = v_outOrientParent;
		m_Type->m_OrientParent.SetCol3(Transform3x3( v_parentLinkMatrix, position ));

		position = Subtract(v_axisPosition,v_childLinkMatrix.GetCol3());
		v_outOrientChild = v_childLinkMatrix;
		Transpose3x3( v_outOrientChild, v_outOrientChild );
		Transform3x3( v_outOrientChild, axisCoords, v_outOrientChild );
		Get3DofType().m_OrientChild = v_outOrientChild;
		m_Type->m_OrientChild.SetCol3(Transform3x3( v_childLinkMatrix, position ));
	}


	void phJoint3Dof::SetAxis(phArticulatedBody* body, const Vector3& axisPosition, const Vector3& axisDir, const Vector3& firstLeanDir, float firstMaxLeanAngle, 
		float secondMaxLeanAngle, float maxTwistAngle, const Quaternion& parentRotation, const Quaternion& childRotation)
	{
		SetAxis(body, axisPosition,axisDir,firstLeanDir,firstMaxLeanAngle,secondMaxLeanAngle,maxTwistAngle);

		Matrix34 mat;
		mat.FromQuaternion(parentRotation);
		RC_MATRIX34(m_Type->m_OrientParent).Dot3x3(mat);
		mat.FromQuaternion(childRotation);
		RC_MATRIX34(Get3DofType().m_OrientChild).Dot3x3(mat);
	}
#ifdef USE_SOFT_LIMITS
	// Return false if joint limit is *not* exceeded.
	// Return true if lean joint limit is exceeeded.
	// If true is returned, then also excessLean is the amount (always nonnegative)
	//		by which the joint limit is exceeded.  Also, in the case, 
	//		rotationalVelocity is returned as well. 
	// The excessLean value could be returned as zero.
	// The rotAxisLean is always set, even if joint limits are not exceeded.
	// This routine always calls ComputeCurrentLean. 
	bool phJoint3Dof::CheckSoftLeanLimitStatus (float* excessLeanSoft, Vector3* rotAxisLean, float* rotVelLean, Vector3* rotAxisTwist)
	{
		ComputeCurrentLean();
		bool limitExceeded = CheckSoftLeanLimitStatus();
		if (limitExceeded)
		{
			*excessLeanSoft = m_CurrentSoftLeanLimit - CurrentLeanAngle;
			*rotAxisLean = RCC_VECTOR3(m_LeanAxis);
			*rotVelLean = CurrentRateOfLean;
			*rotAxisTwist = RCC_VECTOR3(m_TwistAxis);
		}

		return limitExceeded;
	}

	bool phJoint3Dof::CheckSoftLeanLimitStatus (phArticulatedBody *body)
	{
		float softFirstLeanAngleMax = Get3DofType().m_SoftLimitRatio * (Get3DofType().m_HardFirstLeanAngleMax + m_Lean1Adjustment);
		float softSecondLeanAngleMax = Get3DofType().m_SoftLimitRatio * (Get3DofType().m_HardSecondLeanAngleMax + m_Lean2Adjustment);

		if (!m_DifferentMaxLeanAngles)
		{
			float t = CurrentLeanAngle-softFirstLeanAngleMax;
			if (t>=0.0f)
			{
				m_CurrentSoftLeanLimit = softFirstLeanAngleMax;
				CurrentRateOfLean = Subtract(Dot(m_LeanAxis,GetChildLink(body)->GetAngularVelocity()),
					Dot(m_LeanAxis,GetParentLink(body)->GetAngularVelocity())).Getf();
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			// Dot product of lean direction, in x and y directions, respectively.
			Vec3V& lean = m_LeanAxis;
			Mat34V& jntPoseWld = m_JointPoseWorld;
			float leanDotParentX = lean.GetXf()*jntPoseWld.GetM10f() + lean.GetYf()*jntPoseWld.GetM11f() + lean.GetZf()*jntPoseWld.GetM12f();
			float leanDotParentY = lean.GetXf()*jntPoseWld.GetM00f() + lean.GetYf()*jntPoseWld.GetM01f() + lean.GetZf()*jntPoseWld.GetM02f();
			float softLimitFractionSq = square(CurrentLeanAngle)*(square(leanDotParentX/softFirstLeanAngleMax) + square(leanDotParentY/softSecondLeanAngleMax));
			if (softLimitFractionSq<1.0f)
			{
				return false;
			}

			m_CurrentSoftLeanLimit = CurrentLeanAngle/sqrt(softLimitFractionSq);
			CurrentRateOfLean = Subtract(Dot(lean,GetChildLink(body)->GetAngularVelocity()),Dot(lean,GetParentLink(body)->GetAngularVelocity())).Getf();
			return true;		
		}
	}

	// Return false if no joint limit is exceeded.
	// Return true if either lean or twist joint limit is exceeeded.
	// If true is returned, then also excessLean is the amount (always nonnegative)
	//		by which the joint limit is exceeded.  Also, in the case, 
	//		rotationalVelocity is returned as well. 
	// The excessLean value could be zero.
	// This routine always calls ComputeCurrentLean or ComputeCurrentLeanAndTwist,
	//			via calling GetNumJointLimitDofs();
	bool phJoint3Dof::CheckSoftLeanAndTwistLimitStatus (bool* leanExceededFlag, float* excessLeanSoft,
		Vector3* rotAxisLean, float* rotVelLean, bool* twistExceededFlag,
		int *twistDir, float* excessTwistSoft,
		Vector3* rotAxisTwist, float* rotVelTwist)
	{
		int numLimitsToEnforce;
		numLimitsToEnforce = ComputeNumSoftJointLimitDofs();
		*leanExceededFlag = m_LeanLimitExceeded;
		*rotAxisLean = RCC_VECTOR3(m_LeanAxis);
		*rotAxisTwist = RCC_VECTOR3(m_TwistAxis);
		if ( m_LeanLimitExceeded )
		{
			*excessLeanSoft = CurrentLeanAngle - m_CurrentSoftLeanLimit;
			*rotVelLean = CurrentRateOfLean;
		}
		*twistExceededFlag = m_TwistLimitExceeded;
		if ( m_TwistLimitExceeded )
		{
			*excessTwistSoft = m_TwistAngle - m_CurrentSoftTwistLimit;	// Could be positive or negative
			*twistDir = (m_CurrentSoftTwistLimit<0.0f) ? -1 : 1;
			*rotVelTwist = m_TwistRate;
		}
		else
		{
			*twistDir = 0;
		}
		return (numLimitsToEnforce>0);	
	}

	int phJoint3Dof::ComputeNumSoftJointLimitDofs (phArticulatedBody *body)
	{
		float softTwistAngleMax = Get3DofType().m_SoftLimitRatio * (Get3DofType().m_HardTwistAngleMax + m_TwistAdjustment);

		if (!m_HasTwistLimit)
		{
			// Check only lean limits
			if (NeedToKnowTwistValues())
			{
				ComputeCurrentLeanAndTwist();
				m_LeanLimitExceeded = CheckSoftLeanLimitStatus();
				m_TwistRate = m_TwistFactor*Subtract(Dot(m_TwistAxis,GetChildLink(body)->GetAngularVelocity()),Dot(m_TwistAxis,GetParentLink(body)->GetAngularVelocity())).Getf();
			}
			else
			{
				ComputeCurrentLean();
				m_LeanLimitExceeded = CheckSoftLeanLimitStatus();
			}

			// There are no twist limits, so return that they are not exceeded
			m_TwistLimitExceeded = false;
			return (m_LeanLimitExceeded ? 2 : 0);
		}

		ComputeCurrentLeanAndTwist();
		m_LeanLimitExceeded = CheckSoftLeanLimitStatus();
		int numJointLimitDofs = m_LeanLimitExceeded ? 2 : 0;

		// float t;
		m_TwistRate = m_TwistFactor*Subtract(Dot(m_TwistAxis,GetChildLink(body)->GetAngularVelocity()),Dot(m_TwistAxis,GetParentLink(body)->GetAngularVelocity())).Getf();
		if ( (/*t =*/ (m_TwistAngle-softTwistAngleMax)) >= 0.0f )
		{
			// if exceeds limit in positive direction
			m_CurrentSoftTwistLimit = softTwistAngleMax;
			m_TwistLimitExceeded = true;
			numJointLimitDofs++;
		}
		else if ( (/*t =*/ (m_TwistAngle+softTwistAngleMax)) <= 0.0f )
		{
			// if exceeds limit in negative direction
			m_CurrentSoftTwistLimit = -softTwistAngleMax;
			m_TwistLimitExceeded = true;
			numJointLimitDofs++;
		}
		else
		{
			m_TwistLimitExceeded = false;
		}

		return numJointLimitDofs;
	}

	void phJoint3Dof::EnforceSoftJointLimits (float timeStep)
	{
		if (Get3DofType().m_SoftLimitRatio == 1.0f)
			return;

		float fadeThird = 1.0f;
		int exceedsMode=0;			// ==0 no limit exceed, ==1 twist only, ==2 lean only, ==3 twist and lean
		// Lean values
		float excessLeanSoft=0.0f;			// Amount by which the lean limit is exceeded
		float rotVelLean=0.0f;			// Rate at which lean is changing (radians/second)
		Vector3 rotAxisLean(Vector3::ZeroType);		// Axis around which a roation affects the excess lean
		// Twist values
		int twistDir = 0;				// +1, 0, or -1 value: indicates sign of excess twist
		float excessTwistSoft = 0.0f;			// Amount by which the soft twist limit is exceeded
		float rotVelTwist = 0.0f;			// Rate at which the twist is changing (radians/second)
		Vector3 rotAxisTwist(Vector3::ZeroType);		// Axis around which a roation causes an increase the twist
		//   -- A torque in direction rotAxisLean increases the lean
		if ( !NeedToKnowTwistValues() )
		{
			// If only need to check lean limits
			if (CheckSoftLeanLimitStatus(&excessLeanSoft,&rotAxisLean,&rotVelLean,&rotAxisTwist))
			{
				exceedsMode = 1;
			}
		}
		else
		{
			bool excessLeanFlag;	// Is true if the lean limit is exceeded
			bool excessTwistFlag;	// Is true if the twist limit is exceeded
			bool limitFlag = CheckSoftLeanAndTwistLimitStatus(&excessLeanFlag,&excessLeanSoft,&rotAxisLean,&rotVelLean,&excessTwistFlag,&twistDir,&excessTwistSoft,&rotAxisTwist,&rotVelTwist);
			if (limitFlag)
			{
				exceedsMode = excessLeanFlag ? 2 : 0;
				if (excessTwistFlag)
				{
					exceedsMode += 1;
				}
			}
		}

		if (exceedsMode==0)
		{
			return;
		}

		Vector3 thirdAxis;			// Axis orthogonal to rotAxisLean and retAxisTwist
		float rotVelThird = 0.0f;
		float deltaRotVelLean = 0.0f;			// Will hold the desired change in rotVelLean
		if (exceedsMode & 0x02)
		{
			// If lean exceeded
			thirdAxis = rotAxisLean;
			thirdAxis.Cross(rotAxisTwist);			// Third axis
			rotVelThird = GetJointAngularVelocityOnAxis(VECTOR3_TO_VEC3V(thirdAxis));  // Joint angular velocity around third axis

			float angleChange = Min(rotVelLean,0.0f)*timeStep;
			float expectedExcessLeanSoft = excessLeanSoft+angleChange;
			deltaRotVelLean -= expectedExcessLeanSoft*m_Type->m_InvTimeToSeparateSoft*static_cast<phJoint3DofType*>(m_Type.ptr)->m_SoftLimitLeanStrength;
			if (expectedExcessLeanSoft<=0.0f)
			{
				expectedExcessLeanSoft = 0.0f;
				if (rotVelLean<=0.0f)
				{
					exceedsMode &= 0x01;
				}
			}
		}

		float deltaRotVelTwist = 0.0f;
		if ( (exceedsMode&0x01)!=0 )
		{		// If twist limit exceeded
			if ( twistDir == -1 )
			{					// If twist exceeded in negative direction,
				excessTwistSoft = -excessTwistSoft;
				rotAxisTwist.Negate();
				rotVelTwist = -rotVelTwist;
			}

			float angleChange = Min(rotVelTwist,0.0f)*timeStep;
			float expectedExcessTwistSoft = Max(excessTwistSoft,0.0f)+angleChange;
			deltaRotVelTwist -= expectedExcessTwistSoft*m_Type->m_InvTimeToSeparateSoft*static_cast<phJoint3DofType*>(m_Type.ptr)->m_SoftLimitTwistStrength;
			deltaRotVelTwist *= InvertSafe(m_TwistFactor,LARGE_FLOAT);
			if (expectedExcessTwistSoft<=0.0f)
			{
				expectedExcessTwistSoft = 0.0f;
				if (rotVelTwist<0.0f)
				{
					exceedsMode &= 0x02;
				}
			}
		}

		Vector3 torque;
		switch (exceedsMode) 
		{
		case 0:
			torque.Zero();
			break;
		case 1:				// Only twist is exceeded
			{
				torque.Scale(rotAxisTwist,deltaRotVelTwist*InvertSafe(ResponseToUnitTorqueOnAxis(VECTOR3_TO_VEC3V(rotAxisTwist)).Getf(),LARGE_FLOAT));
			}
			break;
		case 2:				// Only lean is exceeded
			{
				// Get 2x2 response matrix for rotAxisLean and thirdAxis torques
				//  ( dw_L, dw_3 ) = ( ( a b ) (c d) ) ( torque_L, torque_3 ).
				Vector3& temp = torque;							// Reuse this vector as temporary storage
				TotalResponseToTorque( RCC_VEC3V(rotAxisLean), RC_VEC3V(temp) );
				float a = rotAxisLean.Dot(temp);
				float c = thirdAxis.Dot(temp);
				TotalResponseToTorque( RCC_VEC3V(thirdAxis), RC_VEC3V(temp) );
				float b = rotAxisLean.Dot(temp);
				float d = thirdAxis.Dot(temp);
				float detInv = InvertSafe(a*d-b*c,LARGE_FLOAT);					// Invert the matrix (implicitly)
				float deltaRotVelThird = -fadeThird*rotVelThird;				// Undo lateral lean rotation
				float torqueToApplyLeanAxis = detInv*(d*deltaRotVelLean - b*deltaRotVelThird);
				float torqueToApplyThirdAxis = detInv*(a*deltaRotVelThird - c*deltaRotVelLean);
				torque.Scale(rotAxisLean,torqueToApplyLeanAxis);
				torque.AddScaled(thirdAxis,torqueToApplyThirdAxis);
			}
			break;
		case 3:				// Both twist and lean are exceeded
			{
				// Get 3x3 response matrix for rotAxisLean and thirdAxis torques
				//  ( dw_L, dw_3, dw_T ) = M ( torque_L, torque_3, torque_T ).
				Matrix34 M;
				Vector3 temp;							
				TotalResponseToTorque( RCC_VEC3V(rotAxisLean), RC_VEC3V(temp) );
				M.a.x = (rotAxisLean.Dot(temp));
				M.b.x = (thirdAxis.Dot(temp));
				M.c.x = (rotAxisTwist.Dot(temp));
				TotalResponseToTorque( RCC_VEC3V(thirdAxis), RC_VEC3V(temp) );
				M.a.y = (rotAxisLean.Dot(temp));
				M.b.y = (thirdAxis.Dot(temp));
				M.c.y = (rotAxisTwist.Dot(temp));
				TotalResponseToTorque( RCC_VEC3V(rotAxisTwist), RC_VEC3V(temp) );
				M.a.z = (rotAxisLean.Dot(temp));
				M.b.z = (thirdAxis.Dot(temp));
				M.c.z = (rotAxisTwist.Dot(temp));

				Vector3 temp2;
				temp2.Set( deltaRotVelLean, -fadeThird*rotVelThird, deltaRotVelTwist );
				temp = M.SolveSVD( temp2 );
				float& torqueToApplyLeanAxis = temp.x;
				float& torqueToApplyThirdAxis = temp.y;
				float& torqueToApplyTwistAxis = temp.z;
				torque.Scale(rotAxisLean,torqueToApplyLeanAxis);
				torque.AddScaled(thirdAxis,torqueToApplyThirdAxis);
				torque.AddScaled(rotAxisTwist,torqueToApplyTwistAxis);
			}
			break;
		default:
			Assert(0);
		}

		if (torque.IsNonZero())
		{
			ApplyTorque(VECTOR3_TO_INTRIN(torque));
		}
	}
#endif // USE_SOFT_LIMITS
	// The next routine re-calculates items already computed in the above functions.
	// At the least, the duplicate call to ComputeCurrentLeanAndTwist should be avoided (ONLY IF SOFT LIMITS ARE USED!!).
	Vec3V_Out phJoint3Dof::ComputeMuscleTorque (phArticulatedBody *body, ScalarV_In timeStep)
	{
		// Find the joint angles.
		ComputeCurrentLeanAndTwist(body);

		// Get the two lean axes and angles.
		Mat34V& jntPoseWld = m_JointPoseWorld;
		Vec3V lean1Axis =  Vec3V(SplatY(jntPoseWld.GetCol0()),SplatY(jntPoseWld.GetCol1()),SplatY(jntPoseWld.GetCol2()));
		Vec3V lean2Axis = -Vec3V(SplatX(jntPoseWld.GetCol0()),SplatX(jntPoseWld.GetCol1()),SplatX(jntPoseWld.GetCol2()));

		ScalarV leanMtx22a = Dot(m_LeanAxis,lean1Axis);
		ScalarV leanMtx22c = Dot(m_LeanAxis,lean2Axis);
		// Also: leanMtx22b = -leanMtx22c;
		// Also: leanMtx22d = leanMtx22a;
		// Careful: m_LeanAxis and m_ThirdAxis form left-handed coordinate system.

		ScalarV currentRateOfThird = Scale(ScalarVFromF32(GetJointAngularVelocityOnAxis(body, m_ThirdAxis)),Invert(ScalarVFromF32(m_TwistFactor)));
		ScalarV leanRate = ScalarVFromF32(GetJointAngularVelocityOnAxis(body, m_LeanAxis));
		ScalarV twistRate = Scale(ScalarVFromF32(GetJointAngularVelocityOnAxis(body, m_TwistAxis)),ScalarVFromF32(m_TwistFactor));
		CurrentRateOfLean = leanRate.Getf();
		m_TwistRate = twistRate.Getf();

		ScalarV leanAngle = ScalarVFromF32(CurrentLeanAngle);
		ScalarV currentLeanAngle1 = Scale(leanAngle,leanMtx22a);
		ScalarV currentLeanAngle2 = Scale(leanAngle,leanMtx22c);
		ScalarV currentRateOfLean1 = Add(Scale(leanRate,leanMtx22a),Scale(currentRateOfThird,leanMtx22c));
		ScalarV currentRateOfLean2 = Subtract(Scale(leanRate,leanMtx22c),Scale(currentRateOfThird,leanMtx22a));

		// Find the torque scale factor for angle control.
		Vec3V angleAngAccel = Vec3V(V_ZERO);
		if (MuscleDriveAngle())
		{
			angleAngAccel = Subtract(m_MuscleTargetAngle,Vec3V(currentLeanAngle1,currentLeanAngle2,ScalarVFromF32(m_TwistAngle)));
			if (!m_HasTwistLimit)
			{
				// Canonicalize the z component of the angular acceleration.
				angleAngAccel = Vec3V(SplatX(angleAngAccel),SplatY(angleAngAccel),CanonicalizeAngle(SplatZ(angleAngAccel)));
			}

			angleAngAccel = Scale(angleAngAccel,m_MuscleAngleStrength);
		}

		// Find the torque scale factor for speed control.
		Vec3V speedAngAccel = Vec3V(V_ZERO);
		if (MuscleDriveSpeed())
		{
			speedAngAccel = Subtract(m_MuscleTargetSpeed,Vec3V(currentRateOfLean1,currentRateOfLean2,twistRate));
			speedAngAccel = Scale(speedAngAccel,m_MuscleSpeedStrength);
		}

		// Find the total angular acceleration, and clamp it to avoid overshooting the target on the next frame.
		Vec3V angAccel = Add(speedAngAccel,angleAngAccel);
		angAccel = ClampAngAccelFromOvershoot(angAccel,Vec3V(currentRateOfLean1,currentRateOfLean2,twistRate),timeStep,Vec3V(currentLeanAngle1,currentLeanAngle2,ScalarVFromF32(m_TwistAngle)),m_MuscleTargetAngle);
		//angAccel.x = ClampAngAccelFromOvershoot(angAccel.x,currentRateOfLean1.Getf(),timeStep.Getf(),currentLeanAngle1.Getf(),m_MuscleTargetAngle.GetXf());
		//angAccel.y = ClampAngAccelFromOvershoot(angAccel.y,currentRateOfLean2.Getf(),timeStep.Getf(),currentLeanAngle2.Getf(),m_MuscleTargetAngle.GetYf());
		//angAccel.z = ClampAngAccelFromOvershoot(angAccel.z,m_TwistRate,timeStep.Getf(),m_TwistAngle,m_MuscleTargetAngle.GetZf());
		VALIDATE_PHYSICS_ASSERT(IsEqualAll(angAccel,angAccel));

		// Find the apply the torque.
		Vector3 torque;
		ComputeMuscleTorqueFromAccel(body, VEC3V_TO_VECTOR3(angAccel), leanMtx22a.Getf(), leanMtx22c.Getf(), torque);

#if __PFDRAW
		if (PFD_ArticulatedMuscleAngleTorque.Begin())
		{
			Vector3 position = GetJointPosition(body);

			Vector3 angleTorque;
			ComputeMuscleTorqueFromAccel(body, VEC3V_TO_VECTOR3(angleAngAccel), leanMtx22a.Getf(), leanMtx22c.Getf(), angleTorque);
			float torqueMag2 = angleTorque.Mag2();

			if (torqueMag2 > VERY_SMALL_FLOAT)
			{
				float torqueMag = sqrtf(torqueMag2);
				float logTorque = log10(torqueMag + 1.0f) * 0.25f;
				Vector3 end(angleTorque);
				end.Scale(logTorque / torqueMag);
				end.Add(position);

				bool oldLighting = grcLighting(false);
				grcDrawSpiral(position, end, 0.0f, logTorque * 0.125f, 10.0f, 0.25f, logTorque * 0.05f);
				grcLighting(oldLighting);
			}

			PFD_ArticulatedMuscleAngleTorque.End();
		}

		if (PFD_ArticulatedMuscleSpeedTorque.Begin())
		{
			Vector3 position = GetJointPosition(body);

			Vector3 speedTorque;
			ComputeMuscleTorqueFromAccel(body, VEC3V_TO_VECTOR3(speedAngAccel), leanMtx22a.Getf(), leanMtx22c.Getf(), speedTorque);
			float torqueMag2 = speedTorque.Mag2();

			if (torqueMag2 > SMALL_FLOAT)
			{
				float torqueMag = sqrtf(torqueMag2);
				float logTorque = log10(torqueMag + 1.0f) * 0.25f;
				Vector3 end(speedTorque);
				end.Scale(logTorque / torqueMag);
				end.Add(position);

				bool oldLighting = grcLighting(false);
				grcDrawSpiral(position, end, 0.0f, logTorque * 0.125f, 10.0f, 0.0f, logTorque * 0.05f);
				grcLighting(oldLighting);
			}

			PFD_ArticulatedMuscleSpeedTorque.End();
		}
#endif // __PFDRAW

		return VECTOR3_TO_VEC3V(torque);
	}


	void phJoint3Dof::ComputeAndApplyMuscleTorques (phArticulatedBody *body, float timeStep)
	{
		Vec3V torque = ComputeMuscleTorque(body, ScalarVFromF32(timeStep));
		ApplyTorque(body, VEC3V_TO_VECTOR3(torque), ScalarVFromF32(timeStep).GetIntrin128ConstRef());
	}


	void phJoint3Dof::ComputeMuscleTorqueFromAccel(phArticulatedBody *body, const Vector3& angAccel, float leanMtx22a, float leanMtx22c, Vector3& torque)
	{
		const float& targetLean1Accel = angAccel.x;
		const float& targetLean2AxisAccel = angAccel.y;
		const float& targetTwistAccel = angAccel.z;

		Vector3 targetAngAccel;
		float& targetLeanAxisAccel = targetAngAccel.x;
		float& targetThirdAxisAccel = targetAngAccel.y;
		float& targetTwistAxisAccel = targetAngAccel.z;
		targetLeanAxisAccel = targetLean1Accel*leanMtx22a+targetLean2AxisAccel*leanMtx22c;
		targetThirdAxisAccel = (targetLean1Accel*leanMtx22c-targetLean2AxisAccel*leanMtx22a)*m_TwistFactor;
		targetTwistAxisAccel = targetTwistAccel/m_TwistFactor;

		//	// Find the torque.
#if 1
		// Use 3x3 solver (more sophisticated)
		Vector3 targetAccelGC;
		targetAccelGC.SetScaled(RCC_VECTOR3(m_LeanAxis),targetLeanAxisAccel);
		targetAccelGC.AddScaled(RCC_VECTOR3(m_ThirdAxis),targetThirdAxisAccel);
		targetAccelGC.AddScaled(RCC_VECTOR3(m_TwistAxis),targetTwistAxisAccel);
		Mat33V ident(V_IDENTITY);
		Mat34V M;
		TotalResponseToTorque(body, ident.GetCol0(), M.GetCol0Ref());
		TotalResponseToTorque(body, ident.GetCol1(), M.GetCol1Ref());
		TotalResponseToTorque(body, ident.GetCol2(), M.GetCol2Ref());
		Vector3 targetTorque;
		RCC_MATRIX34(M).SolveSVD(targetAccelGC,targetTorque);
		float leanAxisTorque = targetTorque.Dot(RCC_VECTOR3(m_LeanAxis));
		float thirdAxisTorque = targetTorque.Dot(RCC_VECTOR3(m_ThirdAxis));
		float twistAxisTorque = targetTorque.Dot(RCC_VECTOR3(m_TwistAxis));
#else
		// This caused large negative joint responses in GTA4, don't know if its inaccuracy was the cause or if it was just catching a problem caused somewhere else (NC 11/22/06)
		// Use the less accurate method of solving independently
		float response = ResponseToUnitTorqueOnAxis(m_LeanAxis);
		float leanAxisTorque = (response>VERY_SMALL_FLOAT ? targetLeanAxisAccel/response : 0.0f);
		response = ResponseToUnitTorqueOnAxis(m_ThirdAxis);
		float thirdAxisTorque = (response>VERY_SMALL_FLOAT ? targetThirdAxisAccel/response : 0.0f);
		response = ResponseToUnitTorqueOnAxis(m_TwistAxis);
		float twistAxisTorque = (response>VERY_SMALL_FLOAT ? targetTwistAxisAccel/response : 0.0f);
#endif

		// Clamp the torque to the allowed range along the lean1 axis.
		float lean1Torque = leanAxisTorque*leanMtx22a+thirdAxisTorque*leanMtx22c;
		lean1Torque = Clamp( lean1Torque, static_cast<phJoint3DofType*>(m_Type.ptr)->m_MinMuscleTorque.GetXf(), static_cast<phJoint3DofType*>(m_Type.ptr)->m_MaxMuscleTorque.GetXf());

		// Clamp the torque to the allowed range along the lean2 axis.
		float lean2Torque = leanAxisTorque*leanMtx22c-thirdAxisTorque*leanMtx22a;
		lean2Torque = Clamp( lean2Torque, static_cast<phJoint3DofType*>(m_Type.ptr)->m_MinMuscleTorque.GetYf(), static_cast<phJoint3DofType*>(m_Type.ptr)->m_MaxMuscleTorque.GetYf());

		// Clamp the torque to the allowed range along the twist axis.
		twistAxisTorque = Clamp( twistAxisTorque, static_cast<phJoint3DofType*>(m_Type.ptr)->m_MinMuscleTorque.GetZf(), static_cast<phJoint3DofType*>(m_Type.ptr)->m_MaxMuscleTorque.GetZf());

		leanAxisTorque = lean1Torque*leanMtx22a+lean2Torque*leanMtx22c;
		thirdAxisTorque = lean1Torque*leanMtx22c-lean2Torque*leanMtx22a;

		torque.SetScaled(RCC_VECTOR3(m_LeanAxis),leanAxisTorque);
		torque.AddScaled(RCC_VECTOR3(m_ThirdAxis),thirdAxisTorque);
		torque.AddScaled(RCC_VECTOR3(m_TwistAxis),twistAxisTorque);
		VALIDATE_PHYSICS_ASSERT(torque==torque);
	}


	// **********************************************************************************
	// The "A" matrix is what Featherstone calls the cross inertia matrix.
	// The main job for joints during the physics simulation is to transform
	//    quantities by multiplying them by A.
	// **********************************************************************************

	// The "PrecomputeA" routines are called before any of the "TransformByA"
	// routines are.  The purpose of the "Precompute" routines is to allow
	// the joint to precompute any helper values, etc.
	// When the "Precompute" routine is called, the I^A matrix is already
	// set as the appropriate LinkInertia.
	//
	// PrecomputeAdownup will called *before* PrecomputeAupdown

	// "Adownup" uses the I^A for the child link (i.e., the "down" link)
	//		This can be found in the joint's "m_DownInertia" field.
	//
	// "Aupdown" uses the I^A for the parent link (i.e., the "up" link)
	//		This can be found in the joint's "m_UpInertia" field.

	void phJoint3Dof::PrecomputeAdownup(phArticulatedBody *body)
	{
		// Since the S matrix is so simple in joint coordinates, we
		//	convert all the conversion to "joint coordinates".  This
		//	is merely a translation of coordinates.

		PrefetchDC( &DownInertiaJC );
		PrefetchDC( &(DownInertiaJC.m_InvInertia) );
		PrefetchDC( &DownIAstiffnessInvI );

		// Save/compute variables locally.
		Mat34V v_childLinkMatrix = GetChildLink(body)->GetMatrix();

		// Get the joint position
		Vec3V v_JointPosition;
		v_JointPosition = GetPositionChildV();
		v_JointPosition = UnTransform3x3Ortho( v_childLinkMatrix, v_JointPosition );
		v_JointPosition += RCC_VEC3V(GetChildLink(body)->GetPosition())/*v_childLinkMatrix.GetCol3()*/;
		JointPosition = v_JointPosition;

		TranslateTensor ( m_DownInertia, &DownInertiaJC, RCC_VECTOR3(v_JointPosition) );

		// Calculate DownIAstiffnessInvH := (I+\gamma)^{-1} H 
		//      and  DownIAstiffnessInvI := (I+\gamma)^{-1} I
		//      where H, I = submatrices of I^A (of "down inertia").  
		// All in joint coordinates.
		if ( FloatEqualsFloatAsInt(&m_Stiffness, U32_ONE) ) {	// The 'else' is almost always taken. Could conditionally select value for 
			DownIAstiffnessInvI = Mat33V(V_ZERO);				// 'DownIAstiffnessInvI' and 'DownIAstiffnessInvH' for minimal cost.
			DownIAstiffnessInvH = Mat33V(V_ZERO);
		}
		else {
			// Save/compute variables locally.
			Mat33V v_DownIAstiffnessInvI;
			Mat33V v_DownIAstiffnessInvH;
			Mat33V v_DownInertiaJCCrossInertia = DownInertiaJC.m_CrossInertia;
			Mat33V v_DownInertiaJCInvInertia = DownInertiaJC.m_InvInertia;
			ScalarV v_gammaStiffness;
			ScalarV v_stiffness = GetStiffness();

			ScalarV trace = SplatX( v_DownInertiaJCInvInertia.GetCol0() ) + SplatY( v_DownInertiaJCInvInertia.GetCol1() ) + SplatZ( v_DownInertiaJCInvInertia.GetCol2() );
			v_gammaStiffness = Scale( ScalarV(V_THIRD), trace );
			v_gammaStiffness = Scale( v_gammaStiffness, InvScale(v_stiffness, Subtract(ScalarV(V_ONE), v_stiffness) ) );
			Mat33V temp;
			temp = v_DownInertiaJCInvInertia;
			temp.GetCol0Ref().SetX( SplatX(temp.GetCol0()) + v_gammaStiffness );
			temp.GetCol1Ref().SetY( SplatY(temp.GetCol1()) + v_gammaStiffness );
			temp.GetCol2Ref().SetZ( SplatZ(temp.GetCol2()) + v_gammaStiffness );
			phArticulatedBodyInertia::InverseSym( temp, v_DownIAstiffnessInvI );
			v_DownIAstiffnessInvH = v_DownIAstiffnessInvI;

			Multiply( v_DownIAstiffnessInvI, v_DownInertiaJCInvInertia, v_DownIAstiffnessInvI );
			Multiply( v_DownIAstiffnessInvH, v_DownInertiaJCCrossInertia, v_DownIAstiffnessInvH );

			// Store.
			StoreScalar32FromScalarV( m_gammaStiffness, v_gammaStiffness );
			DownIAstiffnessInvI = v_DownIAstiffnessInvI;
			DownIAstiffnessInvH = v_DownIAstiffnessInvH;
		}
	}

	void phJoint3Dof::PrecomputeAupdown()
	{
		TranslateTensor ( m_UpInertia, &UpInertiaJC, RCC_VECTOR3(JointPosition) );

		// Calculate UpIAstiffnessInvH := (I+\gamma)^{-1} m_CrossInertia 
		//      and  UpIAstiffnessInvI := (I+\gamma)^{-1} m_InvInertia
		//      where H, I = submatrices of I^A (of "down inertia").  
		// All in joint coordinates.
		if ( FloatEqualsFloatAsInt(&m_Stiffness, U32_ONE) ) {
			UpIAstiffnessInvI = Mat33V(V_ZERO);
			UpIAstiffnessInvH = Mat33V(V_ZERO);
		}
		else {
			ScalarV v_gammaStiffness = ScalarVFromF32(m_gammaStiffness);
			Mat33V v_UpIAstiffnessInvI = UpIAstiffnessInvI;
			Mat33V v_UpIAstiffnessInvH = UpIAstiffnessInvH;
			Mat33V v_UpInertiaJCInvInertia = UpInertiaJC.m_InvInertia;
			Mat33V v_UpInertiaJCCrossInertia = UpInertiaJC.m_CrossInertia;

			Mat33V temp = v_UpInertiaJCInvInertia;	// Use as temporary storage! (Be careful!)
			temp.GetCol0Ref().SetX( SplatX(temp.GetCol0()) + v_gammaStiffness );
			temp.GetCol1Ref().SetY( SplatY(temp.GetCol1()) + v_gammaStiffness );
			temp.GetCol2Ref().SetZ( SplatZ(temp.GetCol2()) + v_gammaStiffness );
			phArticulatedBodyInertia::InverseSym( temp, v_UpIAstiffnessInvI );
			v_UpIAstiffnessInvH = v_UpIAstiffnessInvI;
			Multiply( v_UpIAstiffnessInvI, v_UpInertiaJCInvInertia, v_UpIAstiffnessInvI );
			Multiply( v_UpIAstiffnessInvH, v_UpInertiaJCCrossInertia, v_UpIAstiffnessInvH );

			UpIAstiffnessInvI = v_UpIAstiffnessInvI;
			UpIAstiffnessInvH = v_UpIAstiffnessInvH;
		}
	}

#if PHARTICULATED_DEBUG_SERIALIZE
	void phJoint3Dof::DebugSerialize(fiAsciiTokenizer& t)
	{
		t.PutDelimiter("m_Type->m_OrientChild: "); t.Put(m_Type->m_OrientChild); t.PutDelimiter("\n");
		t.PutDelimiter("m_Type->m_HardFirstLeanAngleMax: "); t.Put(m_Type->m_HardFirstLeanAngleMax); t.PutDelimiter("\n");
		t.PutDelimiter("m_Type->m_HardSecondLeanAngleMax: "); t.Put(m_Type->m_HardSecondLeanAngleMax); t.PutDelimiter("\n");
		t.PutDelimiter("m_Type->m_HardTwistAngleMax: "); t.Put(m_Type->m_HardTwistAngleMax); t.PutDelimiter("\n");
		t.PutDelimiter("m_Type->m_DifferentMaxLeanAngles: "); t.Put(m_Type->m_DifferentMaxLeanAngles); t.PutDelimiter("\n");
		t.PutDelimiter("m_Type->m_HasTwistLimit: "); t.Put(m_Type->m_HasTwistLimit); t.PutDelimiter("\n");

		t.PutDelimiter("m_LeanLimitExceeded: "); t.Put(m_LeanLimitExceeded); t.PutDelimiter("\n");
		t.PutDelimiter("m_TwistLimitExceeded: "); t.Put(m_TwistLimitExceeded); t.PutDelimiter("\n");
		t.PutDelimiter("m_CurrentHardLeanLimit: "); t.Put(m_CurrentHardLeanLimit); t.PutDelimiter("\n");
		t.PutDelimiter("m_CurrentHardTwistLimit: "); t.Put(m_CurrentHardTwistLimit); t.PutDelimiter("\n");
		t.PutDelimiter("CurrentRateOfLean: "); t.Put(CurrentRateOfLean); t.PutDelimiter("\n");
		t.PutDelimiter("m_TwistRate: "); t.Put(m_TwistRate); t.PutDelimiter("\n");
		t.PutDelimiter("CurrentLeanAngle: "); t.Put(CurrentLeanAngle); t.PutDelimiter("\n");
		t.PutDelimiter("m_TwistAngle: "); t.Put(m_TwistAngle); t.PutDelimiter("\n");
#ifdef USE_SOFT_LIMITS
		t.PutDelimiter("m_Type->m_SoftFirstLeanAngleMax: "); t.Put(m_Type->m_SoftFirstLeanAngleMax); t.PutDelimiter("\n");
		t.PutDelimiter("m_Type->m_SoftSecondLeanAngleMax: "); t.Put(m_Type->m_SoftSecondLeanAngleMax); t.PutDelimiter("\n");
		t.PutDelimiter("m_Type->m_SoftTwistAngleMax: "); t.Put(m_Type->m_SoftTwistAngleMax); t.PutDelimiter("\n");
		t.PutDelimiter("m_CurrentSoftLeanLimit: "); t.Put(m_CurrentSoftLeanLimit); t.PutDelimiter("\n");
		t.PutDelimiter("m_CurrentSoftTwistLimit: "); t.Put(m_CurrentSoftTwistLimit); t.PutDelimiter("\n");
#endif
		t.PutDelimiter("m_MuscleAngleStrength: "); t.Put(m_MuscleAngleStrength); t.PutDelimiter("\n");
		t.PutDelimiter("m_MuscleSpeedStrength: "); t.Put(m_MuscleSpeedStrength); t.PutDelimiter("\n");
		t.PutDelimiter("m_MuscleTargetAngle: "); t.Put(m_MuscleTargetAngle); t.PutDelimiter("\n");
		t.PutDelimiter("m_MuscleTargetSpeed: "); t.Put(m_MuscleTargetSpeed); t.PutDelimiter("\n");
		t.PutDelimiter("m_MaxMuscleTorque: "); t.Put(m_MaxMuscleTorque); t.PutDelimiter("\n");
		t.PutDelimiter("m_MinMuscleTorque: "); t.Put(m_MinMuscleTorque); t.PutDelimiter("\n");

		t.PutDelimiter("m_LeanAxis: "); t.Put(m_LeanAxis); t.PutDelimiter("\n");
		t.PutDelimiter("m_TwistAxis: "); t.Put(m_TwistAxis); t.PutDelimiter("\n");
		t.PutDelimiter("m_ThirdAxis: "); t.Put(m_ThirdAxis); t.PutDelimiter("\n");
		t.PutDelimiter("m_TwistFactor: "); t.Put(m_TwistFactor); t.PutDelimiter("\n");
		t.PutDelimiter("JointPoseWorld: "); t.Put(m_JointPoseWorld); t.PutDelimiter("\n");
		t.PutDelimiter("JointPosition: "); t.Put(JointPosition); t.PutDelimiter("\n");

		t.PutDelimiter("UpInertiaJC.m_Mass: "); t.Put(UpInertiaJC.m_Mass); t.PutDelimiter("\n");
		t.PutDelimiter("UpInertiaJC.m_CrossInertia: "); t.Put(UpInertiaJC.m_CrossInertia); t.PutDelimiter("\n");
		t.PutDelimiter("UpInertiaJC.m_InvInertia: "); t.Put(UpInertiaJC.m_InvInertia); t.PutDelimiter("\n");

		t.PutDelimiter("DownInertiaJC.m_Mass: "); t.Put(DownInertiaJC.m_Mass); t.PutDelimiter("\n");
		t.PutDelimiter("DownInertiaJC.m_CrossInertia: "); t.Put(DownInertiaJC.m_CrossInertia); t.PutDelimiter("\n");
		t.PutDelimiter("DownInertiaJC.m_InvInertia: "); t.Put(DownInertiaJC.m_InvInertia); t.PutDelimiter("\n");

		t.PutDelimiter("UpIAstiffnessInvH: "); t.Put(UpIAstiffnessInvH); t.PutDelimiter("\n");
		t.PutDelimiter("DownIAstiffnessInvH: "); t.Put(DownIAstiffnessInvH); t.PutDelimiter("\n");
		t.PutDelimiter("UpIAstiffnessInvI: "); t.Put(UpIAstiffnessInvI); t.PutDelimiter("\n");
		t.PutDelimiter("DownIAstiffnessInvI: "); t.Put(DownIAstiffnessInvI); t.PutDelimiter("\n");
		t.PutDelimiter("m_gammaStiffness: "); t.Put(m_gammaStiffness); t.PutDelimiter("\n");
	}
#endif // PHARTICULATED_DEBUG_SERIALIZE


	void phJoint3Dof::Copy (const phJoint3Dof& original)
	{ 
		// Copy the original.
		*this = original;
	}

} // namespace rage

