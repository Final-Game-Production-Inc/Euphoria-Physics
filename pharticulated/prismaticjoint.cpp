//
// pharticulated/prismaticjoint.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

// Implements a sliding joint


#include "prismaticjoint.h"

#include "articulatedbody.h"

#include "grprofile/drawmanager.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#if __XENON
#include "system/timer.h"
#include "system/xtl.h"
#include <tracerecording.h>
#pragma comment (lib, "tracerecording.lib")
#endif

namespace rage {


	EXT_PFD_DECLARE_ITEM(ArticulatedMuscleSpeedTorque);
	EXT_PFD_DECLARE_ITEM(ArticulatedMuscleAngleTorque);

#if __DECLARESTRUCT
	void phPrismaticJointType::DeclareStruct(datTypeStruct &s)
	{
		phJointType::DeclareStruct(s);
		SSTRUCT_BEGIN_BASE(phPrismaticJointType, phJointType)
			SSTRUCT_FIELD(phPrismaticJointType, m_HardLimitMin)
			SSTRUCT_FIELD(phPrismaticJointType, m_HardLimitMax)
#ifdef USE_SOFT_LIMITS
			SSTRUCT_FIELD(phPrismaticJointType, m_SoftLimitMin)
			SSTRUCT_FIELD(phPrismaticJointType, m_SoftLimitMax)
#else
			SSTRUCT_IGNORE(phPrismaticJointType, m_Pad)
#endif
			SSTRUCT_FIELD(phPrismaticJointType, m_MaxMuscleForce)
			SSTRUCT_FIELD(phPrismaticJointType, m_MinMuscleForce)
			SSTRUCT_IGNORE(phPrismaticJointType, m_Pad0)
			SSTRUCT_END(phPrismaticJointType)
	}
#endif // __DECLARESTRUCT

	phPrismaticJointType::phPrismaticJointType()
	{
		m_HardLimitMin = -FLT_MAX;
		m_HardLimitMax = FLT_MAX;
#ifdef USE_SOFT_LIMITS
		m_SoftLimitMin = -FLT_MAX;
		m_SoftLimitMax = FLT_MAX;
#endif
		m_MaxMuscleForce = 1.0e8f;
		m_MinMuscleForce = -1.0e8f;
	}

	phJointType* phPrismaticJointType::Clone()
	{
		phPrismaticJointType* clone = rage_new phPrismaticJointType;
		*clone = *this;
		return clone;
	}

	float phPrismaticJoint::ComputeCurrentTranslation(phArticulatedBody *body)
	{
		// Child and Parent positions, in world coordinates
		Vector3 childPos;
		MAT34V_TO_MATRIX34(GetChildLink(body)->GetMatrix()).UnTransform3x3(RCC_VECTOR3(GetPositionChildV()), childPos);
		childPos.Add(GetChildLink(body)->GetPosition());
		Vector3 parentPos;
		MAT34V_TO_MATRIX34(GetParentLink(body)->GetMatrix()).UnTransform3x3(RCC_VECTOR3(GetPositionParentV()), parentPos);
		parentPos.Add(GetParentLink(body)->GetPosition());
		childPos -= parentPos;				// Vector from parent position to child position (in world coord's)
		CurrentTranslation = Dot(JointAxis.trans, VECTOR3_TO_VEC3V(childPos)).Getf(); // Explicit load-hit-store. Sucks but hey, gotta store a float.

		return CurrentTranslation;
	}

	// Applies a joint force to both the child and parent links.
	// The direction of the force is in the direction of the translation axis.
	// Positive forces tend to increase the prismatic joint value.
	void phPrismaticJoint::ApplyForce( phArticulatedBody *body, float force, Vec::V3Param128 timeStep ) const
	{
		Vector3 forceVec = VEC3V_TO_VECTOR3( JointAxis.trans );
		forceVec *= force;
		body->ApplyForce( GetChildLinkIndex(), forceVec, ORIGIN, timeStep );
		forceVec.Negate();
		body->ApplyForce( GetParentLinkIndex(), forceVec, ORIGIN, timeStep );
	}

	// Applies a joint impulse to both the child and parent links.
	// The direction of the impulse is in the direction of the translation axis.
	// Positive impulses tend to increase the prismatic joint value.
	void phPrismaticJoint::ApplyImpulse (phArticulatedBody *body, ScalarV_In impulseMag) const
	{

		Vec3V impulseVec = JointAxis.trans;
		impulseVec = Scale( impulseVec, impulseMag );
		//Vector3 position;
		//GetJointPosition( position );
		body->ApplyImpulse( GetChildLinkIndex(), impulseVec, Vec3V(V_ZERO) );
		impulseVec = Negate(impulseVec);
		body->ApplyImpulse( GetParentLinkIndex(), impulseVec, Vec3V(V_ZERO) );
	}


	// Assumes the JointAxis value is still set from the last physics update computation.
	// Uses JointAxis.trans and the m_Velocity value to get the rate of change of
	//    the joint angle.
	float phPrismaticJoint::ComputeVelocity(phArticulatedBody *body)
	{
		Vec3f s_jointAxisTrans;
		Vec3f s_childLinkVelocTrans;
		Vec3f s_parentLinkVelocTrans;
		LoadAsScalar(s_jointAxisTrans, JointAxis.trans);
		LoadAsScalar(s_childLinkVelocTrans, body->GetVelocity(GetChildLinkIndex()).trans);
		LoadAsScalar(s_parentLinkVelocTrans, body->GetVelocity(GetParentLinkIndex()).trans);

		TranslationalVelocity = ((Dot(s_jointAxisTrans, s_childLinkVelocTrans)) - (Dot(s_jointAxisTrans, s_parentLinkVelocTrans)));
		return TranslationalVelocity;
	}

	// Return false if joint limit is *not* exceeded.
	// Return true if joint limit is exceeeded.
	// If true is returned, then also excessValueHard is the signed amount
	//		by which the joint limit is exceeded.  Also, in the case, 
	//		rotationalVelocity is returned as well. 
	// In addition *dir is 1 for joint value too large, is -1 for
	//		joint value too small and is 0 if limits not exceeded.
	// The excessValueHand/Sort values could be zero.
	// This routine always calls ComputeCurrentTranslation. 
	// If it returns true, then it also calls ComputeVelocity.
	bool phPrismaticJoint::CheckJointLimitStatus (phArticulatedBody *body, int* dir, float* excessValueSoft, float* translationalVelocity)
	{
		// Find the current joint value.
		ComputeCurrentTranslation(body);

		// See if the current joint angle is within its limits.
#ifdef USE_SOFT_LIMITS
		float excess1 = CurrentTranslation-GetPrismaticType().m_SoftLimitMin;
		float excess2 = CurrentTranslation-GetPrismaticType().m_SoftLimitMax;
#else
		float excess1 = CurrentTranslation-GetPrismaticType().m_HardLimitMin;
		float excess2 = CurrentTranslation-GetPrismaticType().m_HardLimitMax;
#endif
		float excessSoft = excess2;
		if (excessSoft>=0.0f)
		{
			// This joint's angle is at or beyond its upper limit.
			JointLimitDir = 1;
		}
		else
		{
			excessSoft = excess1;
			if (excessSoft<=0.0f)
			{
				// This joint's angle is at or beyond its lower limit.
				JointLimitDir = -1;
			}
			else
			{
				// This joint's angle is within its limits.
				JointLimitDir = 0;
				return false;
			}
		}

		// This joint's angle is outside its limits. Find the angular speed.
		float speed = ComputeVelocity(body);
		if (dir)
		{
			Assert(excessValueSoft && translationalVelocity);
			(*dir) = JointLimitDir;
			(*excessValueSoft) = excessSoft;
			(*translationalVelocity) = speed;
		}

		return true;
	}

	// Based on the Parent's position and velocity,
	// Modify the Child link's position and velocity to match it properly
	void phPrismaticJoint::MatchChildToParentPositionAndVelocity(phArticulatedBody *body)
	{
		// Match orientations exactly
		Matrix34 cMat(MAT34V_TO_MATRIX34(GetChildLink(body)->GetMatrix()));
		cMat.Dot3x3( MAT34V_TO_MATRIX34(GetParentLink(body)->GetMatrix()), MAT34V_TO_MATRIX34(m_Type->m_OrientParent) );
		cMat.Dot3x3Transpose( GetOrientationChild() );
		GetChildLink(body)->SetOrientation( cMat );

		// Get prismatic axis
		Vector3 jointAxis = VEC3V_TO_VECTOR3( Vec3V( SplatZ(m_Type->m_OrientParent.GetCol0()), SplatZ(m_Type->m_OrientParent.GetCol1()), SplatZ(m_Type->m_OrientParent.GetCol2()) ) );
		MAT34V_TO_MATRIX34(GetParentLink(body)->GetMatrix()).UnTransform3x3(jointAxis);						// Joint axis, global coords

		// Match positions
		Vector3 jointPosP;
		MAT34V_TO_MATRIX34(GetParentLink(body)->GetMatrix()).UnTransform3x3( RCC_VECTOR3(GetPositionParentV()), jointPosP );
		jointPosP += GetParentLink(body)->GetPosition();									// Position of joint, global coords
		Vector3 jointPosC;
		MAT34V_TO_MATRIX34(GetChildLink(body)->GetMatrix()).UnTransform3x3( RCC_VECTOR3(GetPositionChildV()), jointPosC );		
		jointPosC += GetChildLink(body)->GetPosition();									// Position on child, global coords
		Vector3 shiftPos(jointPosP);
		shiftPos.Subtract(jointPosC);
		shiftPos.AddScaled( jointAxis, -shiftPos.Dot(jointAxis) );					// Negative of component of shiftPos orthogonal to joint axis
		shiftPos.Add( GetChildLink(body)->GetPosition() );
		GetChildLink(body)->SetPosition(shiftPos);

		// Match angular velocities exactly
		Vec3V angularVelocity = body->GetAngularVelocityNoProp(GetParentLinkIndex());

		// Match linear velocity except in JointAxis direction
		Vec3V updateVel = body->GetLinearVelocityNoProp(GetParentLinkIndex());
		updateVel = Subtract( updateVel, body->GetLinearVelocityNoProp(GetChildLinkIndex()) );
		updateVel = AddScaled( updateVel, VECTOR3_TO_VEC3V(jointAxis), -Dot(updateVel, VECTOR3_TO_VEC3V(jointAxis)) );
		Vec3V linearVelocity = body->GetLinearVelocityNoProp(GetChildLinkIndex()) + updateVel;

		body->SetVelocitiesNoDownVelAdjust(GetChildLinkIndex(), linearVelocity, angularVelocity);
	}

	int phPrismaticJoint::ComputeNumHardJointLimitDofs (phArticulatedBody *body)
	{
		return (CheckJointLimitStatus(body) ? 1 : 0);
	}


	void phPrismaticJoint::GetJointLimitDetailedInfo (phArticulatedBody *body, int limitNum, int& jointLimitID, float& dofExcess, float& response)
	{
		jointLimitID = 0;
		if (JointLimitDir==-1)
		{
			dofExcess = GetPrismaticType().m_HardLimitMin - CurrentTranslation;
		}
		else
		{
			Assert(JointLimitDir==1);
			dofExcess = CurrentTranslation - GetPrismaticType().m_HardLimitMax;
		}

		response = phPrismaticJoint::GetJointLimitResponse(body, limitNum);
	}

	float phPrismaticJoint::GetJointLimitResponse (phArticulatedBody *body, int UNUSED_PARAM(limitDirectionIndex))
	{
		Assert(JointLimitDir == 1 || JointLimitDir == -1);
		return ResponseToUnitForceOnAxis(body, JointAxis.trans).Getf();
	}

	void phPrismaticJoint::ApplyJointImpulse( phArticulatedBody *body, int UNUSED_PARAM(jointLimitID), ScalarV_In impulse )
	{
		Assert(JointLimitDir==1 || JointLimitDir==-1);
		ScalarV factor = (JointLimitDir==1) ? impulse : -impulse;
		ApplyImpulse( body, factor );
	}

	void phPrismaticJoint::GetJointLimitAxis( phArticulatedBody *body, int UNUSED_PARAM(jointLimitID),  Vector3& axis, Vector3& position ) const
	{
		axis = VEC3V_TO_VECTOR3(JointAxis.trans);
		position = GetJointPosition(body);
		if ( JointLimitDir == -1 ) {
			axis.Negate();
		}
	}

	void phPrismaticJoint::GetJointLimitUnitImpulse( int UNUSED_PARAM(jointLimitID), 
		phPhaseSpaceVector& unitImpulseSpatial ) const
	{
		// It is not necessary to include a torque value (in .trans) no matter what is the position
		//   of the joint (at least as long as the unitImpulseSpatial is to be applied equally to
		//   to both the child link and the parent link).
		unitImpulseSpatial.trans = Vec3V(V_ZERO);
		unitImpulseSpatial.omega = JointAxis.trans;
		if ( JointLimitDir == -1 ) {
			unitImpulseSpatial.omega = Negate(unitImpulseSpatial.omega);
		}
	}

	void phPrismaticJoint::Freeze ()
	{
		TranslationalVelocity = 0.0f;
	}


	void phPrismaticJoint::Copy (const phPrismaticJoint& original)
	{
		// Copy the original.
		*this = original;
	}

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	void phPrismaticJoint::SetAxis( phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir, float minAngle, float maxAngle )
	{
		SetAxis( body, axisPosition, axisDir );
		SetAngleLimits( minAngle, maxAngle );
	}

	void phPrismaticJoint::SetAxis( phArticulatedBody *body, const Vector3& axisPosition, const Vector3& axisDir)
	{
		AssertMsg(axisDir.Mag2()>square(0.99f) && axisDir.Mag2()<square(1.01f),"SetAxis was called with a non-unit axis.");

		Vec3V v_axisDir = RCC_VEC3V(axisDir);
		Vec3V v_axisPos = RCC_VEC3V(axisPosition);
		Mat34V v_parentLinkMatrix = GetParentLink(body)->GetMatrix();
		Mat34V v_childLinkMatrix = GetChildLink(body)->GetMatrix();
		Mat34V v_outOrientChild;

		// Setup a local coordinate systems for the joint: one for
		//		each link.
		// The two local coordinate systems coincide in the rest position.
		// The z axis is the rotation axis.
		Vec3V xDir, yDir;
		GetOrtho( v_axisDir, xDir, yDir );

		Mat34V axisCoords( xDir, yDir, v_axisDir, v_axisDir ); // (don't care about last column)
		Transpose3x3( axisCoords, axisCoords );

		Vec3V pos = v_axisPos;
		pos -= v_parentLinkMatrix.GetCol3();
		m_Type->m_OrientParent = v_parentLinkMatrix;
		Transpose3x3( m_Type->m_OrientParent, m_Type->m_OrientParent );
		Transform3x3( m_Type->m_OrientParent, axisCoords, m_Type->m_OrientParent );
		m_Type->m_OrientParent.SetCol3(Transform3x3( v_parentLinkMatrix, pos ));

		pos = v_axisPos;
		pos -= v_childLinkMatrix.GetCol3();
		v_outOrientChild = v_childLinkMatrix;
		Transpose3x3( v_outOrientChild, v_outOrientChild );
		Transform3x3( v_outOrientChild, axisCoords, v_outOrientChild );
		m_Type->m_OrientChild = v_outOrientChild;
		m_Type->m_OrientChild.SetCol3(Transform3x3( v_childLinkMatrix, pos ));
	}

	void phPrismaticJoint::SetAngleLimits (float minAngle, float maxAngle)
	{
		if (m_Type)
		{
			Assert(minAngle<maxAngle);
			GetPrismaticType().m_HardLimitMin = minAngle;
			GetPrismaticType().m_HardLimitMax = maxAngle;
#ifdef USE_SOFT_LIMITS
			GetPrismaticType().m_SoftLimitMin = minAngle;
			GetPrismaticType().m_SoftLimitMax = maxAngle;
#endif
		}
	}
#ifdef USE_SOFT_LIMITS
	void phPrismaticJoint::SetSoftAngleLimits (float softMinAngle, float softMaxAngle)
	{
		Assert(softMinAngle<=softMaxAngle);
		Assert(softMinAngle>=GetPrismaticType().m_HardLimitMin);
		Assert(softMaxAngle<=GetPrismaticType().m_HardLimitMax);
		GetPrismaticType().m_SoftLimitMin = softMinAngle;
		GetPrismaticType().m_SoftLimitMax = softMaxAngle;
	}

	void phPrismaticJoint::EnforceSoftJointLimits (float timeStep)
	{
		float excessAmtSoft=0.0f, vel=0.0f;
		int dir = 0;
		if ( CheckJointLimitStatus(&dir, &excessAmtSoft, &vel) )
		{
			if ((dir==1)!=(vel>0.0f))   
			{   
				// The angular velocity is away from the joint limit.
				excessAmtSoft += vel*timeStep;
			} 

			// Find the expected violations of joint limits after this update.
			if ((dir==1)==(excessAmtSoft>0.0f))
			{
				// The soft joint limit is expected to be exceeded after this update, so apply enough torque to restore it in the specified time.
				float force = -excessAmtSoft*m_Type->m_InvTimeToSeparateSoft/ResponseToUnitForceOnAxis(JointAxis.trans).Getf();
				ApplyForce(force);
			}
		}
	}
#endif
	void phPrismaticJoint::ComputeAndApplyMuscleTorques (phArticulatedBody *body, float timeStep)
	{
		// Find the torque scale factor for angle control.
		float positionForce = 0.0f;
		if (MuscleDriveAngle())
		{
			ComputeCurrentTranslation(body);
			positionForce = m_MusclePositionStrength*(m_MuscleTargetPosition-CurrentTranslation);
		}

		// Find the torque scale factor for speed control.
		float speedForce = 0.0f;
		if (MuscleDriveSpeed())
		{
			ComputeVelocity(body);
			speedForce = m_MuscleSpeedStrength*(m_MuscleTargetSpeed-TranslationalVelocity);
		}

		// Find the torque, clamp it to the allowed range and apply it to the joint.
		float force = positionForce + speedForce;
		float response = ResponseToUnitForceOnAxis(body, JointAxis.trans).Getf();
		if (response>VERY_SMALL_FLOAT)
		{
			force /= response;
			force = Clamp(force,GetPrismaticType().m_MinMuscleForce,GetPrismaticType().m_MaxMuscleForce);
			ApplyForce(body, force, ScalarVFromF32(timeStep).GetIntrin128ConstRef());

#if __PFDRAW
			if (PFD_ArticulatedMuscleAngleTorque.Begin())
			{
				positionForce /= response;
				positionForce = Clamp(positionForce,GetPrismaticType().m_MinMuscleForce,GetPrismaticType().m_MaxMuscleForce);

				if (positionForce > SMALL_FLOAT || positionForce < -SMALL_FLOAT)
				{
					positionForce = log10(fabs(force) + 1.0f) * 0.25f;
					Vector3 position = GetJointPosition(body);

					Vector3 end = VEC3V_TO_VECTOR3(JointAxis.omega);
					end.Scale(positionForce);
					end.Add(position);

					bool oldLighting = grcLighting(false);
					grcDrawSpiral(position, end, 0.0f, positionForce * 0.125f, 10.0f, 0.25f, positionForce * 0.05f);
					grcLighting(oldLighting);
				}

				PFD_ArticulatedMuscleAngleTorque.End();
			}

			if (PFD_ArticulatedMuscleSpeedTorque.Begin())
			{
				speedForce /= response;
				speedForce = Clamp(speedForce,GetPrismaticType().m_MinMuscleForce,GetPrismaticType().m_MaxMuscleForce);

				if (speedForce > SMALL_FLOAT || speedForce < -SMALL_FLOAT)
				{
					speedForce = log10(fabs(speedForce) + 1.0f) * 0.25f;
					Vector3 position = GetJointPosition(body);

					Vector3 end = VEC3V_TO_VECTOR3(JointAxis.omega);
					end.Scale(speedForce);
					end.Add(position);

					bool oldLighting = grcLighting(false);
					grcDrawSpiral(position, end, 0.0f, speedForce * 0.125f, 10.0f, 0.0f, speedForce * 0.05f);
					grcLighting(oldLighting);
				}

				PFD_ArticulatedMuscleSpeedTorque.End();
			}
#endif // __PFDRAW
		}
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

	void phPrismaticJoint::PrecomputeAdownup(phArticulatedBody *body)
	{
		// Calculate the joint axis as a spatial vector
		JointAxis.trans = Vec3V( 
			SplatZ(m_Type->m_OrientChild.GetCol0()), 
			SplatZ(m_Type->m_OrientChild.GetCol1()), 
			SplatZ(m_Type->m_OrientChild.GetCol2()) );
		JointAxis.trans = UnTransform3x3Ortho(GetChildLink(body)->GetMatrix(),JointAxis.trans);
		JointAxis.omega = Vec3V(V_ZERO);

		// Set IAs_downup = I^A_{child} s.
		m_DownInertia.TransformTrans(RCC_VECTOR3(JointAxis.trans), IAs_downup);

		// We might as well stay in the vector pipeline below.

		ScalarV v_sSIs_downup = Dot(JointAxis.trans, IAs_downup.omega);
		StoreScalar32FromScalarV( sSIs_downup, v_sSIs_downup );

		ScalarV v_Stiffness = GetStiffness();
		ScalarV v_sSIsInv_downup = Invert( v_sSIs_downup );
		ScalarV v_sSIsInvStiffness_downup = Scale( (ScalarV(V_ONE)-v_Stiffness), v_sSIsInv_downup );
		StoreScalar32FromScalarV( sSIsInvStiffness_downup, v_sSIsInvStiffness_downup );
	}

	void phPrismaticJoint::PrecomputeAupdown()
	{
		ScalarV v_one(V_ONE);

		// Set IAs_updown = I^A_{parent} s.
		m_UpInertia.TransformTrans(RCC_VECTOR3(JointAxis.trans), IAs_updown);
		ScalarV sSIs_updown = Dot(JointAxis.trans, IAs_updown.omega);

		// We might as well stay in the vector pipeline below.

		ScalarV v_sSIs_downup = ScalarVFromF32(sSIs_downup);
		ScalarV v_Stiffness = GetStiffness();
		ScalarV v_sSIsInvStiffness_updown = (v_one-v_Stiffness);
		v_sSIsInvStiffness_updown = InvScale( v_sSIsInvStiffness_updown, AddScaled(Scale(v_sSIsInvStiffness_updown,sSIs_updown), v_Stiffness, v_sSIs_downup ) );
		StoreScalar32FromScalarV( sSIsInvStiffness_updown, v_sSIsInvStiffness_updown );
	}

} // namespace rage


