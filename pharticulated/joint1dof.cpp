//
// pharticulated/joint1dof.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

// Implements a 1-DOF rotational joint, an "elbow joint".


#include "joint1dof.h"

#include "articulatedbody.h"

#include "math/angmath.h"
#include "physics/simulator.h"
#include "grprofile/drawmanager.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

namespace rage {


	EXT_PFD_DECLARE_ITEM(ArticulatedMuscleSpeedTorque);
	EXT_PFD_DECLARE_ITEM(ArticulatedMuscleAngleTorque);

#if __DECLARESTRUCT
	void phJoint1DofType::DeclareStruct(datTypeStruct &s)
	{
		phJointType::DeclareStruct(s);
		SSTRUCT_BEGIN_BASE(phJoint1DofType, phJointType)
			SSTRUCT_FIELD(phJoint1DofType, m_HardAngleMin)
			SSTRUCT_FIELD(phJoint1DofType, m_HardAngleMax)
			SSTRUCT_FIELD(phJoint1DofType, m_MaxMuscleTorque)
			SSTRUCT_FIELD(phJoint1DofType, m_MinMuscleTorque)
#ifdef USE_SOFT_LIMITS
			SSTRUCT_FIELD(phJoint1DofType, m_SoftLimitStrength)
#endif
			SSTRUCT_END(phJoint1DofType)
	}
#endif // __DECLARESTRUCT

	phJoint1DofType::phJoint1DofType (datResource &rsc) : phJointType(rsc)
	{

	}

	phJoint1DofType::phJoint1DofType()
	{
		m_MaxMuscleTorque = 1.0e8f;
		m_MinMuscleTorque = -1.0e8f;
#ifdef USE_SOFT_LIMITS
		m_SoftLimitStrength = 1.0f;
#endif
		m_HardAngleMin = -FLT_MAX;
		m_HardAngleMax = FLT_MAX;
	}

	phJointType* phJoint1DofType::Clone()
	{
		phJoint1DofType* clone = rage_new phJoint1DofType;
		*clone = *this;
		return clone;
	}

	// Applies a joint torque to both the child and parent links.
	// The direction of the torque is given by the righthand rule: use
	//		positive torque values to increase the joint angle.
	void phJoint1Dof::ApplyTorque( phArticulatedBody *body, const float& torque, Vec::V3Param128 timeStep ) const
	{
		Vec3V torqueVec = m_JointAxis.omega;
		torqueVec = Scale(torqueVec, Vec3VFromF32(torque));
		body->ApplyTorque( GetChildLinkIndex(), torqueVec.GetIntrin128(), timeStep );
		torqueVec = Negate(torqueVec);
		body->ApplyTorque( GetParentLinkIndex(), torqueVec.GetIntrin128(), timeStep );
	}

	void phJoint1Dof::ApplyAngImpulse (phArticulatedBody *body, ScalarV_In angImpulseMag) const
	{
		Vec3V angImpulse = m_JointAxis.omega;
		angImpulse = Scale( angImpulse, angImpulseMag );
		body->ApplyAngImpulse(GetChildLinkIndex(), angImpulse.GetIntrin128());
		angImpulse = Negate(angImpulse);
		body->ApplyAngImpulse(GetParentLinkIndex(), angImpulse.GetIntrin128());
	}

	// This version of ComputeCurrentAngle always returns a value between -PI and PI.
	// Sometime we probably want a fancier version that uses 
	//  the previous value of the angle and maybe the rotation rate
	//	to make a more informed estimate of the angle, even outside the range -PI to PI.
	ScalarV_Out phJoint1Dof::ComputeCurrentAngle(phArticulatedBody *body)
	{
		Mat34V v_OrientChild = m_Type->m_OrientChild;
		Mat34V v_OrientParent = m_Type->m_OrientParent;
		Vec3V v_outParentAxis;

		// Take two dot products with axes in global coordinates
		Vec3V ChildAxisX( SplatX(v_OrientChild.GetCol0()), SplatX(v_OrientChild.GetCol1()), SplatX(v_OrientChild.GetCol2()) );
		ChildAxisX = UnTransform3x3Ortho( GetChildLink(body)->GetMatrix(), ChildAxisX );	// Child x-axis, global coords
		Vec3V ParentAxis( SplatX(v_OrientParent.GetCol0()), SplatX(v_OrientParent.GetCol1()), SplatX(v_OrientParent.GetCol2()) );
		ParentAxis = UnTransform3x3Ortho( GetParentLink(body)->GetMatrix(), ParentAxis );	// Parent x-axis, global coords

		ScalarV v_x = Dot( ChildAxisX, ParentAxis );
		ParentAxis = Cross( m_JointAxis.omega, ParentAxis );	// Parent Y-axis, global coords
		ScalarV v_y = Dot( ChildAxisX, ParentAxis );

		ScalarV v_CurrentAngle = Arctan2( v_y, v_x ); // TODO: Comfirm this vectorized version doesn't cause any problems. (The old one was scalar, and "safe"!)

		StoreScalar32FromScalarV( CurrentAngle, v_CurrentAngle );

		return v_CurrentAngle;
	}

	ScalarV_Out phJoint1Dof::ComputeRotationalVelocity(phArticulatedBody *body) const
	{
		// We're storing the result to a float, so might as well use the scalar pipeline the whole way.
		Vec3V v_jointAxisOmega = m_JointAxis.omega;
		Vec3V v_childLinkAngVeloc = body->GetAngularVelocity(GetChildLinkIndex());
		Vec3V v_parentLinkAngVeloc = body->GetAngularVelocity(GetParentLinkIndex());

		ScalarV v_RotationalVelocity = ((Dot(v_jointAxisOmega, v_childLinkAngVeloc)) - (Dot(v_jointAxisOmega, v_parentLinkAngVeloc)));

		return v_RotationalVelocity;
	}

	float phJoint1Dof::ComputeHardLimitExcess (phArticulatedBody *body)
	{
		// Find the current joint angle.
		ComputeCurrentAngle(body);

		// Return the hard angle limit excess (positive for over the max, negative for under the min, otherwise zero).
		float overMax = CurrentAngle-Get1DofType().m_HardAngleMax + m_HardAngleMaxAdjust;
		float overMin = CurrentAngle-(Get1DofType().m_HardAngleMin + m_HardAngleMinAdjust);
		return FPIfGteZeroThenElse(overMax, overMax, FPIfGteZeroThenElse(overMin, 0.0f, overMin));
	}

	void phJoint1Dof::MatchChildToParentPositionAndVelocity(phArticulatedBody* body, ScalarV_In rotation)
	{
		// NOTE: When I say world space I mean relative to the collider's position
		// Compute the joint's matrix in world space based on where the parent is
		Mat34V parentFromJoint = m_Type->m_OrientParent; 
		Transpose3x3(parentFromJoint,parentFromJoint);
		Mat34V worldFromParent = GetParentLink(body)->GetMatrix();
		Transpose3x3(worldFromParent,worldFromParent);
		Mat34V worldFromJointIdeal;
		Transform(worldFromJointIdeal,worldFromParent,parentFromJoint);

		// Rotate the joint matrix around its constraint axis (always the local Z)
		Mat34VRotateLocalZ(worldFromJointIdeal,rotation);

		// Use the unchanging child-to-joint matrix to get a final child matrix
		Mat34V childFromJoint = m_Type->m_OrientChild; 
		Transpose3x3(childFromJoint,childFromJoint);
		Mat34V jointFromChild;
		InvertTransformOrtho(jointFromChild,childFromJoint);
		Mat34V worldFromChildIdeal;
		Transform(worldFromChildIdeal,worldFromJointIdeal,jointFromChild);
		Transpose3x3(worldFromChildIdeal,worldFromChildIdeal);
		GetChildLink(body)->SetMatrix(RCC_MATRIX34(worldFromChildIdeal));

		// Zero out the relative velocity of the child
		body->EnsureVelocitiesFullyPropagated();
		const Vec3V parentAngularVelocity = body->GetAngularVelocityNoProp(GetParentLinkIndex());
		const Vec3V parentVelocity = body->GetAngularVelocityNoProp(GetParentLinkIndex());
		const Vec3V newAngularVelocity = parentAngularVelocity;
		const Vec3V newVelocity = AddCrossed(parentVelocity,parentAngularVelocity, Subtract(worldFromChildIdeal.GetCol3(),worldFromParent.GetCol3()));
		body->SetVelocitiesNoDownVelAdjust(GetChildLinkIndex(),newAngularVelocity,newVelocity);
		body->ResetPropagationVelocities();
	}

	// ***************************************************************************
	// Based on the Parent's position and velocity,
	// Modify the Child link's position and velocity to match it properly
	void phJoint1Dof::MatchChildToParentPositionAndVelocity(phArticulatedBody *body)
	{
		Mat34V v_childLinkMatrix = GetChildLink(body)->GetMatrix();
		Mat34V v_parentLinkMatrix = GetParentLink(body)->GetMatrix();

		// Match orientation properly
		Vec3V axisP = Vec3V( SplatZ(m_Type->m_OrientParent.GetCol0()), SplatZ(m_Type->m_OrientParent.GetCol1()), SplatZ(m_Type->m_OrientParent.GetCol2()) );
		axisP = UnTransform3x3Ortho( v_parentLinkMatrix, axisP );
		Vec3V axisC = Vec3V( 
			SplatZ(m_Type->m_OrientChild.GetCol0()), 
			SplatZ(m_Type->m_OrientChild.GetCol1()), 
			SplatZ(m_Type->m_OrientChild.GetCol2()) );
		axisC = UnTransform3x3Ortho( v_childLinkMatrix, axisC );

		Mat34V childOrientation;
		VrRotateAlign(childOrientation, axisC, axisP);
		Mat34V zzz = v_childLinkMatrix;
		Transform3x3( childOrientation, zzz, childOrientation );
		GetChildLink(body)->SetOrientation(RCC_MATRIX34(childOrientation));

		// Match positions
		Vec3V jointPosP;
		jointPosP = UnTransform3x3Ortho( v_parentLinkMatrix, GetPositionParentV() );
		jointPosP += v_parentLinkMatrix.GetCol3();								// Position of joint, global coords
		Vec3V jointPosC;
		jointPosC = UnTransform3x3Ortho( v_childLinkMatrix, GetPositionChildV() );		// Position relative to child CM, global coords
		Vec3V newPosition = jointPosP;
		newPosition = Subtract(newPosition, jointPosC);
		GetChildLink(body)->SetPosition(RCC_VECTOR3(newPosition));

		// Match angular velocities
		ScalarV dotC = Dot(axisP, body->GetAngularVelocityNoProp(GetChildLinkIndex()));
		ScalarV dotP = Dot(axisP, body->GetAngularVelocityNoProp(GetParentLinkIndex()));
		Vec3V angularVelocity = AddScaled( body->GetAngularVelocityNoProp(GetParentLinkIndex()), axisP, Vec3V(dotC-dotP) );

		// Match linear velocity 
		Vec3V velP = jointPosP;
		velP -= v_parentLinkMatrix.GetCol3();
		velP = Cross(body->GetAngularVelocityNoProp(GetParentLinkIndex()), velP);		// Velocity	of joint (rel to parent CM)
		velP += body->GetLinearVelocityNoProp(GetParentLinkIndex());						// Actual velocity
		Vec3V velC = jointPosC;									// Position of joint rel to CM (global coords)
		velC = Cross(angularVelocity, velC);			// Velocity of joint rel to child CM
		Vec3V linearVelocity = velP;
		linearVelocity -= velC;

		body->SetVelocitiesNoDownVelAdjust(GetChildLinkIndex(), linearVelocity, angularVelocity);
	}

#define VERIFY_NEW_1DOF_JOINT_LIMIT_COMPUTATION 0

	int phJoint1Dof::ComputeNumHardJointLimitDofs (phArticulatedBody *body)
	{
		// Find the current joint angle.
		ScalarV currentAngle = ComputeCurrentAngle(body);

		const ScalarV angleMax = ScalarV(Get1DofType().m_HardAngleMax) + ScalarV(m_HardAngleMaxAdjust);
		const ScalarV angleMin = ScalarV(Get1DofType().m_HardAngleMin) + ScalarV(m_HardAngleMinAdjust);

		int result;

		if (IsTrue(angleMax - angleMin > ScalarV(V_TWO_PI)))
		{
			// Limit is more than a complete circle - so don't limit
			m_JointLimitDir = 0;
			result = 0;
		}
		else
		{
			ScalarV fromMax = CanonicalizeAngle(currentAngle - angleMax);
			ScalarV fromMin = CanonicalizeAngle(angleMin - currentAngle);

			if (EnforcingAccededLimits())
			{
				ScalarV jointLimitDir = SelectFT(fromMax < fromMin, ScalarV(Vec::V4VConstant(V_MASKXYZW)), ScalarV(Vec::V4VConstant(V_INT_1)));

				StoreScalar32FromScalarV(m_JointLimitDir, jointLimitDir);

#if VERIFY_NEW_1DOF_JOINT_LIMIT_COMPUTATION && __ASSERT
				int otherJointLimitDir = (Get1DofType().m_HardAngleMax + m_HardAngleMaxAdjust - currentAngle.Getf()<currentAngle.Getf()-(Get1DofType().m_HardAngleMin + m_HardAngleMinAdjust) ? 1 : -1);
				Assert(otherJointLimitDir == m_JointLimitDir);
#endif

				result = 1;
			}
			else
			{
				BoolV closerToMax = IsLessThan(Abs(fromMax - ScalarV(V_FLT_SMALL_6)), Abs(fromMin - ScalarV(V_FLT_SMALL_6)));

				BoolV overMax = IsGreaterThan(fromMax, ScalarV(V_ZERO));
				BoolV underMin = IsGreaterThan(fromMin, ScalarV(V_ZERO));

				ScalarV maxResult = SelectFT(overMax, ScalarV(V_ZERO), ScalarV(Vec::V4VConstant(V_INT_1)));
				ScalarV minResult = SelectFT(underMin, ScalarV(V_ZERO), ScalarV(Vec::V4VConstant(V_MASKXYZW)));

				ScalarV jointLimitDir = SelectFT(closerToMax, minResult, maxResult);

				StoreScalar32FromScalarV(m_JointLimitDir, jointLimitDir);

				if (IsEqualIntAll(jointLimitDir, ScalarV(V_ZERO)))
				{
					result = 0;
				}
				else
				{
					result = 1;
				}

#if VERIFY_NEW_1DOF_JOINT_LIMIT_COMPUTATION && __ASSERT
				// See if the current joint angle is within its limits.
				float excessHardAngle = CurrentAngle-(Get1DofType().m_HardAngleMax + m_HardAngleMaxAdjust);
				int otherJointLimitDir;
				if (excessHardAngle>=0.0f)
				{
					// This joint's angle is at or beyond its upper limit.
					otherJointLimitDir = 1;
				}
				else
				{
					excessHardAngle = CurrentAngle-(Get1DofType().m_HardAngleMin + m_HardAngleMinAdjust);
					if (excessHardAngle<=0.0f)
					{
						// This joint's angle is at or beyond its lower limit.
						otherJointLimitDir = -1;
					}
					else
					{
						// This joint's angle is within its limits.
						otherJointLimitDir = 0;
					}
				}

				// Note that this SHOULD fire in the case I was trying to fix...a joint with > 180 degree limits. Shouldn't
				// fire much with normal ragdolls though.
				Assert(otherJointLimitDir == m_JointLimitDir);
#endif // VERIFY_NEW_1DOF_JOINT_LIMIT_COMPUTATION && __ASSERT
			}
		}

		// This joint's angle is outside its limits, or it will enforce acceded limits.
		return result;
	}


	void phJoint1Dof::GetJointLimitDetailedInfo (phArticulatedBody *body, int limitNum, int& jointLimitID, float& dofExcess, float& response)
	{
		jointLimitID = 0;
		if (m_JointLimitDir==-1)
		{
			dofExcess = CanonicalizeAngle(Get1DofType().m_HardAngleMin + m_HardAngleMinAdjust - CurrentAngle);
		}
		else
		{
			Assert(m_JointLimitDir==1);
			dofExcess = CanonicalizeAngle(CurrentAngle - (Get1DofType().m_HardAngleMax + m_HardAngleMaxAdjust));
		}

		response = phJoint1Dof::GetJointLimitResponse(body, limitNum);
	}

	float phJoint1Dof::GetJointLimitResponse (phArticulatedBody *body, int UNUSED_PARAM(limitDirectionIndex))
	{
		Assert(m_JointLimitDir==1 || m_JointLimitDir==-1);
		return ResponseToUnitTorqueOnAxis(body, m_JointAxis.omega).Getf();
	}

	void phJoint1Dof::ApplyJointImpulse( phArticulatedBody *body, int UNUSED_PARAM(jointLimitID), ScalarV_In impulse )
	{
		Assert(m_JointLimitDir==1 || m_JointLimitDir==-1);
		ScalarV factor = (m_JointLimitDir==1 ? impulse : -impulse);
		ApplyAngImpulse( body, factor );
	}

	void phJoint1Dof::GetJointLimitAxis(phArticulatedBody *body,  int UNUSED_PARAM(jointLimitID),  Vector3& axis, Vector3& position ) const
	{
		Assert(m_JointLimitDir!=0);
		Vec3V v_axis = m_JointAxis.omega;
		position = GetJointPosition(body);
		if (m_JointLimitDir==-1)
		{
			v_axis = Negate(v_axis);
		}
		RC_VEC3V(axis) = v_axis;
	}

	void phJoint1Dof::GetJointLimitUnitImpulse( int UNUSED_PARAM(jointLimitID), 
		phPhaseSpaceVector& unitImpulseSpatial ) const
	{
		Assert(m_JointLimitDir!=0);
		unitImpulseSpatial.omega = Vec3V(V_ZERO);
		unitImpulseSpatial.trans = m_JointAxis.omega;
		if (m_JointLimitDir==-1)
		{
			unitImpulseSpatial.trans = Negate(unitImpulseSpatial.trans);
		}
	}

	void phJoint1Dof::Freeze ()
	{
	}

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	void phJoint1Dof::SetAxis (phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisUnitDir)
	{
		Assertf(axisUnitDir.Mag2()>0.998f && axisUnitDir.Mag2()<1.002f,"phJoint1Dof::SetAxis was called with an unnormalized axis %f, %f, %f",axisUnitDir.x,axisUnitDir.y,axisUnitDir.z);

		Mat34V v_parentLinkMatrix = GetParentLink(body)->GetMatrix();
		Mat34V v_childLinkMatrix = GetChildLink(body)->GetMatrix();
		Vec3V v_axisPos = RCC_VEC3V(axisPosition);

		Mat34V v_outOrientParent;
		Mat34V v_outOrientChild;

		// Setup a local coordinate systems for the joint: one for
		//		each link.
		// The two local coordinate systems coincide in the rest position.
		// The z axis is the rotation axis.
		Vec3V xDir, yDir;
		GetOrtho(RCC_VEC3V(axisUnitDir), xDir, yDir);
		Mat34V axisCoords( xDir, yDir, RCC_VEC3V(axisUnitDir), xDir ); // (don't care about last col)
		Transpose3x3( axisCoords, axisCoords );

		Vec3V pos = v_axisPos;
		pos -= v_parentLinkMatrix.GetCol3();
		v_outOrientParent = v_parentLinkMatrix;
		Transpose3x3( v_outOrientParent, v_outOrientParent );
		Transform3x3( v_outOrientParent, axisCoords, v_outOrientParent );
		m_Type->m_OrientParent = v_outOrientParent;
		m_Type->m_OrientParent.SetCol3(Transform3x3( v_parentLinkMatrix, pos ));

		pos = v_axisPos;
		pos -= v_childLinkMatrix.GetCol3();
		v_outOrientChild = v_childLinkMatrix;
		Transpose3x3( v_outOrientChild, v_outOrientChild );
		Transform3x3( v_outOrientChild, axisCoords, v_outOrientChild );
		m_Type->m_OrientChild = v_outOrientChild;
		m_Type->m_OrientChild.SetCol3(Transform3x3( v_childLinkMatrix, pos ));
#ifdef USE_SOFT_LIMITS
		Get1DofType().m_SoftLimitStrength = 1.0f;
#endif
	}

	void phJoint1Dof::SetAxis(phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisUnitDir, float maxAngle )
	{
		SetAxis(body, axisPosition,axisUnitDir,-maxAngle,maxAngle );
	}

	void phJoint1Dof::SetAxis(phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisUnitDir, float minAngle, float maxAngle)
	{
		SetAxis(body, axisPosition,axisUnitDir);
		SetAngleLimits(minAngle,maxAngle);
	}

	void phJoint1Dof::SetAngleLimits (float minAngle, float maxAngle, bool adjustOnly)
	{
		if (m_Type)
		{
			// See if the angle range is greater than the full circle.

			// Seems that if the artist has made a joint this close to 2PI we should assume 
			// the intention is for it to wrap around
			if (maxAngle-minAngle>1.99f*PI)
			{
				// The angle range is greater than the full circle, so make the limits outside of (-PI,PI) to be sure they won't limit the rotations.
				minAngle = -2.0f*PI;
				maxAngle = 2.0f*PI;
			}

			if (adjustOnly)
			{
				float newHardMin = minAngle + phSimulator::GetAllowedAnglePenetration();
				float newHardMax = maxAngle - phSimulator::GetAllowedAnglePenetration();
#ifdef USE_SOFT_LIMITS
				m_SoftAngleMin = newHardMin;
				m_SoftAngleMax = newHardMax;
#endif
				m_HardAngleMinAdjust = newHardMin - Get1DofType().m_HardAngleMin;
				m_HardAngleMaxAdjust = newHardMax - Get1DofType().m_HardAngleMax;
			}
			else
			{
				// Take into account the allowed penetration when setting the joint limits, so the users' chosen limits will appear exactly correct
				Get1DofType().m_HardAngleMin = minAngle + phSimulator::GetAllowedAnglePenetration();
				Get1DofType().m_HardAngleMax = maxAngle - phSimulator::GetAllowedAnglePenetration();
#ifdef USE_SOFT_LIMITS
				m_SoftAngleMin = Get1DofType().m_HardAngleMin;
				m_SoftAngleMax = Get1DofType().m_HardAngleMax;
#endif
				m_HardAngleMinAdjust = 0.0f;
				m_HardAngleMaxAdjust = 0.0f;

				if (Get1DofType().m_HardAngleMin > Get1DofType().m_HardAngleMax)
				{
					float avgAngle = (minAngle + maxAngle) * 0.5f;
#ifdef USE_SOFT_LIMITS
					m_SoftAngleMin = m_SoftAngleMax = avgAngle;
#endif
					Get1DofType().m_HardAngleMin = Get1DofType().m_HardAngleMax = avgAngle;
				}
			}
		}
	}
#ifdef USE_SOFT_LIMITS
	void phJoint1Dof::SetSoftAngleLimits (float softMinAngle, float softMaxAngle)
	{
		m_HardAngleMinAdjust = 0.0f;
		m_HardAngleMaxAdjust = 0.0f;

		Assert(softMinAngle<=softMaxAngle);
		// The cast below is due to the comparison being done as a double, on PC, which fails sometimes when it should not.
		Assert(softMinAngle<-PI || softMinAngle >= (float)(Get1DofType().m_HardAngleMin - phSimulator::GetAllowedAnglePenetration()));
		Assert(softMaxAngle>PI || softMaxAngle<=(float)(Get1DofType().m_HardAngleMax + phSimulator::GetAllowedAnglePenetration()));
		m_SoftAngleMin = softMinAngle + phSimulator::GetAllowedAnglePenetration();
		m_SoftAngleMax = softMaxAngle - phSimulator::GetAllowedAnglePenetration();

		if (m_SoftAngleMin > m_SoftAngleMax)
		{
			float avgAngle = (softMinAngle + softMaxAngle) * 0.5f;
			m_SoftAngleMin = m_SoftAngleMax = avgAngle;
		}
	}

	void phJoint1Dof::EnforceSoftJointLimits (float timeStep)
	{
		// Reset the enforce acceded limits flag.
		m_Type->m_EnforceAccededLimits = false;

		// Find the current joint angle.
		ComputeCurrentAngle();

		// See if the current joint angle is within its limits.
		float angleOverMax = CurrentAngle-m_SoftAngleMax;
		float angleOverMin = CurrentAngle-m_SoftAngleMin;
		if (angleOverMax>0.0f || angleOverMin<0.0f)
		{
			// This joint's angle is outside its soft limits.
			float excessSoftAngle = FPIfGteZeroThenElse(angleOverMax,angleOverMax,angleOverMin);

			// Find the angular speed.
			float angSpeed = ComputeRotationalVelocity().Getf();

			if (!SameSign(excessSoftAngle,angSpeed))   
			{   
				// The angular velocity is away from the joint limit, so reduce it by the expected motion next frame.
				float predictedMotion = angSpeed*timeStep;
				excessSoftAngle += predictedMotion;
				if (SameSign(excessSoftAngle,predictedMotion))
				{
					// The angle change from the speed is enough to eliminate the excess.
					return;
				}
			} 

			// The soft joint limit is expected to be exceeded after this update, so apply enough torque to restore it in the specified time.
			float torque = -excessSoftAngle *
				Get1DofType().m_SoftLimitStrength *
				m_Type->m_InvTimeToSeparateSoft *
				InvertSafe(ResponseToUnitTorque(),LARGE_FLOAT);

			ApplyTorque(torque);
		}
	}
#endif
	ScalarV_Out phJoint1Dof::ComputeMuscleTorque (phArticulatedBody *body, ScalarV_In timeStep)
	{
		// Find the torque scale factor for angle control.
		ScalarV angleAngAccel = ScalarV(V_ZERO);
		if (MuscleDriveAngle())
		{
			ComputeCurrentAngle(body);

#if 1	// Vectorised version DOESN'T seem to be working
			// Turn this on if problems show up, but it guarantees LHSs
			float angleDiff = m_MuscleTargetAngle-CurrentAngle;
			if(Get1DofType().m_HardAngleMax + m_HardAngleMaxAdjust - (Get1DofType().m_HardAngleMin + m_HardAngleMinAdjust) > 2.0f * PI)
			{
				// Wrap around joint can go the opposite way
				angleDiff = CanonicalizeAngle(angleDiff);
			}		
			angleAngAccel = ScalarVFromF32(m_MuscleAngleStrength*(angleDiff));
#else
			// Vectorized version seems to work
			ScalarV angleDiff = ScalarVFromF32(m_MuscleTargetAngle)-ScalarVFromF32(CurrentAngle);
			if(IsGreaterThanAll(ScalarVFromF32(Get1DofType().m_HardAngleMax + m_HardAngleMaxAdjust) -
				ScalarVFromF32(Get1DofType().m_HardAngleMin + m_HardAngleMinAdjust), ScalarV(V_TWO_PI)))
			{
				// Wrap around joint can go the opposite way
				const ScalarV piV = ScalarV(V_TWO_PI) * ScalarV(V_HALF);
				VecBoolV lessThanZero = IsLessThan(angleDiff, ScalarV(V_ZERO));
				ScalarV shift = SelectFT(lessThanZero, ScalarV(V_TWO_PI) * ScalarV(V_NEGONE), piV);

				angleDiff += shift;

				//angle = fmodf(angle, 2.0f * PI);
				angleDiff /= ScalarV(V_TWO_PI);
				angleDiff -= RoundToNearestIntZero(angleDiff);
				angleDiff *= ScalarV(V_TWO_PI);

				angleDiff -= shift;

				VecBoolV equalNegPi = IsEqual(angleDiff, piV * ScalarV(V_NEGONE));
				angleDiff = SelectFT(equalNegPi, angleDiff, piV);
			}		
			angleAngAccel = ScalarVFromF32(m_MuscleAngleStrength)*angleDiff;
#endif
		}

		// Find the torque scale factor for speed control.
		ScalarV speedAngAccel = ScalarV(V_ZERO);
		ScalarV rotationalVelocity = ComputeRotationalVelocity(body);
		if (MuscleDriveSpeed())
		{
			speedAngAccel = ScalarVFromF32(m_MuscleSpeedStrength)*(ScalarVFromF32(m_MuscleTargetSpeed)-rotationalVelocity);
		}

		// Find the total angular acceleration, and clamp it to avoid overshooting the target on the next frame.
		ScalarV angAccel = Add(angleAngAccel,speedAngAccel);
		angAccel = ClampAngAccelFromOvershoot(angAccel,rotationalVelocity,timeStep,ScalarVFromF32(CurrentAngle),ScalarVFromF32(m_MuscleTargetAngle));

		// Find the torque, clamp it to the allowed range and apply it to the joint.
		ScalarV torque = ScalarV(V_ZERO);
		ScalarV invAngInertia = ScalarVFromF32(ResponseToUnitTorque(body));
		if (IsGreaterThanAll(invAngInertia,ScalarVFromF32(VERY_SMALL_FLOAT)))
		{
			torque = ScalarVFromF32(FPClamp(Scale(angAccel,Invert(invAngInertia)).Getf(),Get1DofType().m_MinMuscleTorque,Get1DofType().m_MaxMuscleTorque));
		}

#if __PFDRAW
		if (PFD_ArticulatedMuscleAngleTorque.Begin())
		{
			float angleTorque = Clamp(angleAngAccel.Getf()/invAngInertia.Getf(),Get1DofType().m_MinMuscleTorque,Get1DofType().m_MaxMuscleTorque);
			if (angleTorque > SMALL_FLOAT || angleTorque < -SMALL_FLOAT)
			{
				angleTorque = log10(fabs(angleTorque) + 1.0f) * 0.25f;
				Vector3 position = GetJointPosition(body);

				Vector3 end = VEC3V_TO_VECTOR3(m_JointAxis.omega);
				end.Scale(angleTorque);
				end.Add(position);

				bool oldLighting = grcLighting(false);
				grcDrawSpiral(position, end, 0.0f, angleTorque * 0.125f, 10.0f, 0.25f, angleTorque * 0.05f);
				grcLighting(oldLighting);
			}

			PFD_ArticulatedMuscleAngleTorque.End();
		}

		if (PFD_ArticulatedMuscleSpeedTorque.Begin())
		{
			float speedTorque = Clamp(speedAngAccel.Getf()/invAngInertia.Getf(),Get1DofType().m_MinMuscleTorque,Get1DofType().m_MaxMuscleTorque);
			if (speedTorque > SMALL_FLOAT || speedTorque < -SMALL_FLOAT)
			{
				speedTorque = log10(fabs(speedTorque) + 1.0f) * 0.25f;
				Vector3 position = GetJointPosition(body);

				Vector3 end = VEC3V_TO_VECTOR3(m_JointAxis.omega);
				end.Scale(speedTorque);
				end.Add(position);

				bool oldLighting = grcLighting(false);
				grcDrawSpiral(position, end, 0.0f, speedTorque * 0.125f, 10.0f, 0.0f, speedTorque * 0.05f);
				grcLighting(oldLighting);
			}

			PFD_ArticulatedMuscleSpeedTorque.End();
		}
#endif // __PFDRAW

		VALIDATE_PHYSICS_ASSERT(IsEqualAll(torque,torque));
		return torque;
	}


	void phJoint1Dof::ComputeAndApplyMuscleTorques (phArticulatedBody *body, float timeStep)
	{
		float torque = ComputeMuscleTorque(body, ScalarVFromF32(timeStep)).Getf();
		ApplyTorque(body, torque, ScalarVFromF32(timeStep).GetIntrin128ConstRef());
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

	void phJoint1Dof::PrecomputeAdownup(phArticulatedBody *body)
	{
		Vec3V v_one(V_ONE);
		Mat34V v_childLinkMatrix = GetChildLink(body)->GetMatrix();

		// Calculate the joint axis as a spatial vector
		m_JointAxis.omega = Vec3V( 
			SplatZ(m_Type->m_OrientChild.GetCol0()), 
			SplatZ(m_Type->m_OrientChild.GetCol1()), 
			SplatZ(m_Type->m_OrientChild.GetCol2()) );
		m_JointAxis.trans = GetPositionChildV();
		m_JointAxis.omega = UnTransform3x3Ortho( v_childLinkMatrix, m_JointAxis.omega );
		m_JointAxis.trans = UnTransform3x3Ortho( v_childLinkMatrix, m_JointAxis.trans );
		m_JointAxis.trans += v_childLinkMatrix.GetCol3();
		m_JointAxis.trans = Cross(m_JointAxis.trans, m_JointAxis.omega);

		// Set IAs_downup = I^A_{child} s.
		m_DownInertia.Transform(m_JointAxis, IAs_downup);
		Vector3 du = m_JointAxis^IAs_downup;
		ScalarV downup(du.xyzw);
		SetsSIs_downupV( downup );
		ScalarV sSIsInv_downup = InvertSafe( downup, ScalarV(V_ZERO) );
		SetsSIsInvStiffness_downupV( Scale( (v_one.AsScalarV()-GetStiffness()), sSIsInv_downup ) );
	}

	void phJoint1Dof::PrecomputeAupdown()
	{
		Vec3V v_one(V_ONE);

		// Set IAs_updown = I^A_{parent} s.
		m_UpInertia.Transform(m_JointAxis, IAs_updown);
		Vec3V sSIs_updown = VECTOR3_TO_VEC3V((m_JointAxis^IAs_updown));
		ScalarV stiffness = GetStiffness();
		Vec3V denom = AddScaled( Vec3V( Scale( stiffness, GetsSIs_downupV() ) ), (v_one-Vec3V(stiffness)), Vec3V(sSIs_updown) );
		Vec3V updown = InvScale( (v_one - Vec3V(stiffness)), denom);
		SetsSIsInvStiffness_updownV( updown.AsScalarV() );
	}

#if PHARTICULATED_DEBUG_SERIALIZE
	void phJoint1Dof::DebugSerialize(fiAsciiTokenizer& t)
	{
		t.PutDelimiter("OrientChild: "); t.Put(OrientChild); t.PutDelimiter("\n");
		t.PutDelimiter("m_HardAngleMin: "); t.Put(m_Type->m_HardAngleMin); t.PutDelimiter("\n");
		t.PutDelimiter("m_HardAngleMax: "); t.Put(m_Type->m_HardAngleMax); t.PutDelimiter("\n");
#ifdef USE_SOFT_LIMITS
		t.PutDelimiter("m_SoftAngleMin: "); t.Put(m_Type->m_SoftAngleMin); t.PutDelimiter("\n");
#endif
		t.PutDelimiter("CurrentAngle: "); t.Put(CurrentAngle); t.PutDelimiter("\n");
		t.PutDelimiter("m_JointLimitDir: "); t.Put(m_JointLimitDir); t.PutDelimiter("\n");

		t.PutDelimiter("m_JointAxis.omega: "); t.Put(m_JointAxis.omega); t.PutDelimiter("\n");
		t.PutDelimiter("m_JointAxis.trans: "); t.Put(m_JointAxis.trans); t.PutDelimiter("\n");

		t.PutDelimiter("m_MuscleAngleStrength: "); t.Put(m_MuscleAngleStrength); t.PutDelimiter("\n");
		t.PutDelimiter("m_MuscleSpeedStrength: "); t.Put(m_MuscleSpeedStrength); t.PutDelimiter("\n");
		t.PutDelimiter("m_MuscleTargetAngle: "); t.Put(m_MuscleTargetAngle); t.PutDelimiter("\n");
		t.PutDelimiter("m_MuscleTargetSpeed: "); t.Put(m_MuscleTargetSpeed); t.PutDelimiter("\n");
		t.PutDelimiter("m_MaxMuscleTorque: "); t.Put(m_MaxMuscleTorque); t.PutDelimiter("\n");
		t.PutDelimiter("m_MinMuscleTorque: "); t.Put(m_MinMuscleTorque); t.PutDelimiter("\n");

		t.PutDelimiter("IAs_downup.omega: "); t.Put(IAs_downup.omega); t.PutDelimiter("\n");
		t.PutDelimiter("IAs_downup.trans: "); t.Put(IAs_downup.trans); t.PutDelimiter("\n");

		t.PutDelimiter("IAs_updown.omega: "); t.Put(IAs_updown.omega); t.PutDelimiter("\n");
		t.PutDelimiter("IAs_updown.trans: "); t.Put(IAs_updown.trans); t.PutDelimiter("\n");

		t.PutDelimiter("sSIs_downup: "); t.Put(sSIs_downup); t.PutDelimiter("\n");
		t.PutDelimiter("sSIsInvStiffness_downup: "); t.Put(sSIsInvStiffness_downup); t.PutDelimiter("\n");
		t.PutDelimiter("sSIsInvStiffness_updown: "); t.Put(sSIsInvStiffness_updown); t.PutDelimiter("\n");
	}
#endif // PHARTICULATED_DEBUG_SERIALIZE


	void phJoint1Dof::Copy (const phJoint1Dof& original)
	{ 
		// Copy the original.
		*this = original;
	}

} // namespace rage
