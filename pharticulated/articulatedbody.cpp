// 
// pharticulated/articulatedbody.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "articulatedbody.h"

#include "bodypart.h"

#include "data/resourcehelpers.h"
#include "math/simplemath.h"
#include "phbound/boundcapsule.h"
#include "phbound/boundcomposite.h"
#include "phcore/constants.h"
#include "phcore/conversion.h"
#include "phcore/phmath.h"
#include "system/threadtype.h"
#include "system/timemgr.h"

#if __PPU
#include "system/dmaplan.h"
#endif

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"
// #include "system/findsize.h"


namespace rage {

	float phArticulatedBody::sm_RagdollMaxLinearSpeed = 80.0f;
	float phArticulatedBody::sm_RagdollMaxAngularSpeed = 6.28f;
	float phArticulatedBody::sm_RagdollDampingLinC = 0.03f;
	float phArticulatedBody::sm_RagdollDampingLinV = 0.03f;
	float phArticulatedBody::sm_RagdollDampingLinV2 = 0.02f;
	float phArticulatedBody::sm_RagdollDampingAngC = 0.03f;
	float phArticulatedBody::sm_RagdollDampingAngV = 0.03f;
	float phArticulatedBody::sm_RagdollDampingAngV2 = 0.02f;
	float phArticulatedBody::sm_EmergencyMinStiffness = 0.5f;
	bool phArticulatedBody::sm_EffectorDrivingEnabled = true;

void phArticulatedBody::Reset ()
{
	// want to keep - see comment in articulatedBody.h
	m_ResetLastFrame = true;
}

void phArticulatedBody::ZeroLinkVelocities()
{
	for (int linkIndex=0; linkIndex<GetNumBodyParts(); linkIndex++)
	{
		ZeroLinkVelocity(linkIndex);
	}
}

void phArticulatedBody::ResetAllJointLimitAdjustments()
{
	for (int jointIndex=0; jointIndex<GetNumJoints(); jointIndex++)
	{
		GetJoint(jointIndex).ResetLimitAdjustments();
	}
}

Vec3V_Out phArticulatedBody::GetLocalVelocityNoProp (int partIndex, Vec::V3Param128 position) const
{
	const phPhaseSpaceVector& velocity = GetVelocityNoProp(partIndex);
	Vec3V v_returnedVal = velocity.trans;
	v_returnedVal = AddCrossed( v_returnedVal, velocity.omega, Vec3V(position) );
	return v_returnedVal;
}

Vec3V_Out phArticulatedBody::GetLocalVelocity (int partIndex, Vec::V3Param128 position) 
{
	m_IncrementalBase = PropagateIncrementalToLink(partIndex, m_IncrementalBase);

	const phPhaseSpaceVector& velocity = GetVelocityNoProp(partIndex);
	Vec3V v_returnedVal = velocity.trans;
	v_returnedVal = AddCrossed( v_returnedVal, velocity.omega, Vec3V(position) );
	return v_returnedVal;
}

void phArticulatedBody::GetLocalVelocityNoProp (int partIndex, Vec::V3Param128 position, Vector3* returnedVelocity) const
{
	RC_VEC3V(*returnedVelocity) = GetLocalVelocityNoProp(partIndex, position);
}

void phArticulatedBody::GetLocalVelocity (int partIndex, Vec::V3Param128 position, Vector3* returnedVelocity) 
{
	RC_VEC3V(*returnedVelocity) = GetLocalVelocity(partIndex, position);
}

const phPhaseSpaceVector& phArticulatedBody::GetVelocity(int partIndex )
{
	m_IncrementalBase = PropagateIncrementalToLink(partIndex, m_IncrementalBase);

	return GetVelocityNoProp(partIndex);
}

Vec3V_Out phArticulatedBody::GetAngularVelocity(int partIndex ) 
{
	m_IncrementalBase = PropagateIncrementalToLink(partIndex, m_IncrementalBase);

	return GetVelocityNoProp(partIndex).omega;
}

Vec3V_Out phArticulatedBody::GetLinearVelocity(int partIndex ) 
{
	m_IncrementalBase = PropagateIncrementalToLink(partIndex, m_IncrementalBase);

	const phPhaseSpaceVector& velocity = GetVelocityNoProp(partIndex);
	return AddCrossed(velocity.trans, velocity.omega, GetLink(partIndex).GetMatrix().GetCol3());
}

void phArticulatedBody::MoveLinkPosition (int partIndex, Vec3V_In offset)
{
	VALIDATE_PHYSICS_ASSERT(IsFiniteAll(GetVelocityNoProp(partIndex).omega));
	VALIDATE_PHYSICS_ASSERT(IsFiniteAll(GetVelocityNoProp(partIndex).trans));

	// TODO: We can improve the pass-by-reg of this function

	Vec3V localOffset = offset;

	// Move the body part's position by the given offset.
	Mat34V &linkMat = GetLink(partIndex).GetMatrix();
	linkMat.SetCol3( linkMat.GetCol3() + localOffset );

	// Change the translational velocity to match the rotational velocity about the new coordinate origin.
	m_PartVelocities[partIndex].trans = AddCrossed(m_PartVelocities[partIndex].trans, localOffset, m_PartVelocities[partIndex].omega);
	VALIDATE_PHYSICS_ASSERT(m_PartVelocities[partIndex]==m_PartVelocities[partIndex]);
}

void phArticulatedBody::ResetDownVel(int partIndex)
{
	for ( int i=0; i<GetNumBodyParts()-1; i++ ) {
		if (m_Type->m_JointParentIndex[i] == partIndex)
		{
			phJoint& theJoint = GetJoint(i);
			theJoint.m_VelocityToPropDown = GetVelocityNoProp(partIndex);
		}
	}
}

void phArticulatedBody::SetEffectorsToZeroPose()
{
	Assert(m_Type);
	Vector3 vZero(0.0f,0.0f,0.0f);
	for (int i = 0; i < GetNumJoints(); i++)
	{
		if (GetTypeOfJoint(i) == phJoint::JNT_1DOF)
		{
			GetJoint1Dof(i).SetMuscleTargetAngle(0.0f);
			GetJoint1Dof(i).SetMuscleTargetSpeed(0.0f);
			GetJoint1Dof(i).SetMuscleAngleStrength(DEFAULT_MUSCLE_ANGLE_STRENGTH);
			GetJoint1Dof(i).SetMuscleSpeedStrength(DEFAULT_MUSCLE_SPEED_STRENGTH);
		}
		else if (GetTypeOfJoint(i) == phJoint::JNT_3DOF)
		{
			GetJoint3Dof(i).SetMuscleTargetAngle(vZero);
			GetJoint3Dof(i).SetMuscleTargetSpeed(vZero);
			GetJoint3Dof(i).SetMuscleAngleStrength(DEFAULT_MUSCLE_ANGLE_STRENGTH);
			GetJoint3Dof(i).SetMuscleSpeedStrength(DEFAULT_MUSCLE_SPEED_STRENGTH);
		}
	}
}

void phArticulatedBody::SetEffectorsToCurrentPose(float strength, float damping)
{
	Assert(m_Type);
	Vector3 vZero(0.0f,0.0f,0.0f);
	for (int i = 0; i < GetNumJoints(); i++)
	{
		if (GetTypeOfJoint(i) == phJoint::JNT_1DOF)
		{
			GetJoint1Dof(i).SetMuscleTargetAngle(GetJoint1Dof(i).GetComputedAngle());
			GetJoint1Dof(i).SetMuscleTargetSpeed(0.0f);
			GetJoint1Dof(i).SetMuscleAngleStrength(strength);
			GetJoint1Dof(i).SetMuscleSpeedStrength(damping);
		}
		else if (GetTypeOfJoint(i) == phJoint::JNT_3DOF)
		{
			// Split the lean into two parts and get the lean1, lean2 and twist angles.
			rage::Vec3V leanAndTwistAngles,leanAndTwistRates;
			GetJoint3Dof(i).ComputeCurrentLeanAnglesAndRates(this, leanAndTwistAngles,leanAndTwistRates);
			GetJoint3Dof(i).SetMuscleTargetAngle(leanAndTwistAngles);
			GetJoint3Dof(i).SetMuscleTargetSpeed(vZero);
			GetJoint3Dof(i).SetMuscleAngleStrength(strength);
			GetJoint3Dof(i).SetMuscleSpeedStrength(damping);
		}
	}
}

// *************************************************************
//    The current velocities are used to update the positions and
//		orientation of the links.  The links may "drift" apart.
// *************************************************************
void phArticulatedBody::UpdatePositionsFromVelocities( Vec::V3Param128 time ) 
{
	Mat34V tempRot;
	Vec3V angVelocity, linearVelocity, position;
	ScalarV v_time(time);

	// Periodically clear out the velocity from a fixed root to enable it to sleep.  In order to stop velocity from entering the root
	// completely we would need to add branches into the propagation functions, but that's slow.  
	if (m_RootIsFixed)
	{
		ZeroLinkVelocity(0);
	}

	for (int partIndex= (m_RootIsFixed ? 1 : 0); partIndex<GetNumBodyParts(); partIndex++)
	{
		phArticulatedBodyPart& theLink = GetLink(partIndex);
		linearVelocity = GetLinearVelocityNoProp(partIndex);
		angVelocity = GetAngularVelocityNoProp(partIndex);

		// Freeze the body if any large or non-finite velocities or positions are detected here
		if (!IsFiniteAll(linearVelocity) || !IsFiniteAll(angVelocity) || !IsFiniteAll(theLink.GetPositionV()) ||
			!IsLessThanAll(linearVelocity,Vec3V(V_FLT_LARGE_6)) ||
			!IsLessThanAll(angVelocity,Vec3V(V_FLT_LARGE_6)) ||
			!IsLessThanAll(theLink.GetPositionV(),Vec3V(V_FLT_LARGE_6)))
		{
			Assertf(0, "phArticulatedBody::UpdatePositionsFromVelocities - bad velocities detected - resetting velocities: Lin velocity is %f %f %f. Ang velocity is %f %f %f. position is %f %f %f. link num id %d. numParts is %d",
				VEC3V_ARGS(linearVelocity), VEC3V_ARGS(angVelocity), 
				VEC3V_ARGS(theLink.GetPositionV()), partIndex, GetNumBodyParts());
			Freeze();
			RejuvenateMatrices();
			SetBodyMinimumStiffness(0.8f);
			SetStiffness(0.8f);
			return;
		}

		position = AddScaled(RCC_VEC3V(theLink.GetPosition()), linearVelocity, v_time);
		theLink.SetPosition(RCC_VECTOR3(position));
		SetLinearVelocityNoDownVelAdjust(partIndex, linearVelocity);

		angVelocity = GetAngularVelocityNoProp(partIndex);
		ScalarV angVelMag2 = MagSquared( angVelocity );
		Assertf(IsFiniteAll(angVelMag2), "phArticulatedBody::UpdatePositionsFromVelocities was about to call SetRotationMatrix with non-unit direction %f, %f, %f. Link velocity is %f %f %f.  angVelMag2 is %f. numParts is %d",
			VEC3V_ARGS(angVelocity), 
			VEC3V_ARGS(GetAngularVelocityNoProp(partIndex)),
			angVelMag2.Getf(), GetNumBodyParts());

		if ( IsGreaterThanAll( angVelMag2, ScalarV(V_FLT_SMALL_6) ) != 0 && IsFiniteAll(angVelMag2) )
		{
			ScalarV angVelMag = Sqrt(angVelMag2);
			angVelocity = InvScale(angVelocity, Vec3V(angVelMag));	// Make unit vector
			Assertf(fabs(RCC_VECTOR3(angVelocity).Mag2()-1.0f)<2.0e-6f,"phArticulatedBody::UpdatePositionsFromVelocities is about to call SetRotationMatrix with non-unit direction %f, %f, %f. Link velocity is %f %f %f.  angVelMag2 is %f.  angVelMag is %f. numParts is %d.",
				VEC3V_ARGS(angVelocity), 
				VEC3V_ARGS(GetAngularVelocityNoProp(partIndex)),
				angVelMag2.Getf(), angVelMag.Getf(), GetNumBodyParts());
			SetRotationMatrix(tempRot, angVelocity.GetIntrin128(), (angVelMag*v_time).GetIntrin128());	// Rotation matrix // TODO: Make the 3rd argument here a V3Param128, and replace the implementation with a vectorized one
			Mat34V zzz = theLink.GetMatrix();

			Mat34V linkOrientation;
			Transform3x3( linkOrientation, zzz, tempRot );
			theLink.SetOrientation(RCC_MATRIX34(linkOrientation));		// Multiply by tempRot on the left
		}
	}
	ResetPropagationVelocities();

	InvalidateJointLimits();
}

void phArticulatedBody::FinishPushesWithImpulses (Vec::V3Param128 pushTimeStep, Vec::V3Param128 dampingTimeStep)
{
	ScalarV maxPushDist = ScalarVFromF32(17.0f);		// 510 meters per second at 30 frames per second
	ScalarV maxPushAngle = ScalarVFromF32(1.414f);	// 81 degrees
	ScalarV timeInv = InvertSafe(ScalarV(dampingTimeStep));
	ScalarV maxPushSpeed = maxPushDist*timeInv;
	ScalarV maxPushAngSpeed = maxPushAngle*timeInv;
	ClampVelocities(maxPushSpeed.GetIntrin128(),maxPushAngSpeed.GetIntrin128());
	UpdatePositionsFromVelocities(pushTimeStep);
}

void phArticulatedBody::MovePosition (Vec::V3Param128 offset)
{
	EnsureVelocitiesFullyPropagated();

	// Move all the body parts. This is used to reset the all the body part positions to be relative to the root part.
	for (int linkIndex=0; linkIndex<GetNumBodyParts(); linkIndex++)
	{
		MoveLinkPosition(linkIndex, RCC_VEC3V(offset));
	}

	ResetPropagationVelocities();
}


void phArticulatedBody::RotateBody (Mat34V_In rotation)
{
	// Transpose the rotation because articulated body parts use left-handed matrices, unlike the rest of Rage.
	Matrix34 bodyRotation;
	bodyRotation.Transpose(RCC_MATRIX34(rotation));

	// Make sure the rotation matrix's position is zero. This is the same as using Dot3x3 for the root part, but not the same for the other body parts,
	// which have non-zero positions that will be rotated around the body's local origin.
	bodyRotation.d.Zero();

	// Rotate all the body parts, including their positions.
	for (int bodyPartIndex=0; bodyPartIndex<GetNumBodyParts(); bodyPartIndex++)
	{
		// Rotate the body part's matrix orientation and position.
		RC_MATRIX34(GetLink(bodyPartIndex).m_Matrix).Dot(bodyRotation);
	}
}


void phArticulatedBody::RotateBody (Vec::V3Param128 turn)
{
	// TODO: Refactor this function if we find that it ever gets executed!
	// NOTE 1: For this one, we can do a Select() to choose between results, since chances are this "if()" will
	// be taken often, since it's just an error-check. Will check to be sure.
	// NOTE 2: Maybe can prefetch m_LinkPtr[bodyPartIndex+1]->m_Matrix in these loops?


	// Find the squared angle of rotation.
	ScalarV angle2 = MagSquared(Vec3V(turn));
	if (IsTrue(angle2 > ScalarV(V_FLT_SMALL_12)))
	{
		// Find the angle and the unit direction of rotation.
		ScalarV angle = Sqrt(angle2);
		Vec3V unitAxis(turn);
		unitAxis /= angle;

		// Make a rotation matrix. It gets transposed because articulated body parts use left-handed matrices, unlike the rest of Rage.
		Mat34V rotation;
		Mat34VFromAxisAngle(rotation, unitAxis, angle);

		// Rotate all the body parts.
		RotateBody(rotation);
	}
}


void phArticulatedBody::RotateBodyTo (Mat34V_In toMatrix)
{
	Matrix34 originalMatrix(MAT34V_TO_MATRIX34(GetLink(0).GetMatrix()));
	originalMatrix.Transpose();

	Matrix34 targetMatrix(RCC_MATRIX34(toMatrix));
	targetMatrix.d.Set(Vector3(Vector3::ZeroType));

	// Rotate all the body parts, including their positions.
	for (int bodyPartIndex=1; bodyPartIndex<GetNumBodyParts(); bodyPartIndex++)
	{
		ASSERT_ONLY(float orthonormalityLinkInitial = RCC_MATRIX34(GetLink(bodyPartIndex).m_Matrix).MeasureNonOrthonormality();)
		Matrix34 matPart(MAT34V_TO_MATRIX34(GetLink(bodyPartIndex).GetMatrix()));
		matPart.Transpose();

		// get the matrix delta from old to new
		Matrix34 deltaPartMatrix;
		deltaPartMatrix.DotTranspose(matPart, originalMatrix);

		// Rotate the body part's matrix orientation by dot'ing with the transposed delta
		// Note: order is reversed because both matrices are transposed
		RC_MATRIX34(GetLink(bodyPartIndex).m_Matrix).Dot(deltaPartMatrix, targetMatrix);
		RC_MATRIX34(GetLink(bodyPartIndex).m_Matrix).Transpose();

		Assertf(RCC_MATRIX34(GetLink(bodyPartIndex).m_Matrix).IsOrthonormal(),	"phArticulatedBody::RotateBodyTo: Link matrix isn't orthonormal."
																				"\nLink Index: %i"
																				"\nOriginal Link Orthonormality: %f"
																				"\nFinal Link Orthonormality: %f"
																				"\nOriginal Root Orthonormality: %f"
																				"\nFinal Root Orthonormality: %f",
																				bodyPartIndex,
																				orthonormalityLinkInitial,
																				RCC_MATRIX34(GetLink(bodyPartIndex).m_Matrix).MeasureNonOrthonormality(),
																				RCC_MATRIX34(GetLink(0).m_Matrix).MeasureNonOrthonormality(),
																				targetMatrix.MeasureNonOrthonormality());
	}

	// just set the root to be the transpose of the target, that way we can easily avoid any drift on the root
	RC_MATRIX34(GetLink(0).m_Matrix).Transpose(targetMatrix);
	RC_MATRIX34(GetLink(0).m_Matrix).d.Set(Vector3(Vector3::ZeroType));
}

void phArticulatedBody::SetDriveState (phJoint::driveState state)
{
	int numJoints = GetNumJoints();
	for (int jointIndex=0; jointIndex<numJoints; jointIndex++)
	{
		GetJoint(jointIndex).SetDriveState(state);
	}
}


void phArticulatedBody::SetRotationMatrix (Mat34V_InOut rotationMtx, Vec::V3Param128 u, Vec::V3Param128 theta)
{
	// TODO: Let's try to get rid of this float LHS.... damn theta. At least make it "const float&".

	Assertf(fabs(MagSquared(Vec3V(u)).Getf()-1.0f)<2.0e-6f,"phArticulatedBody::SetRotationMatrix was called with non-unit direction %f, %f, %f",Vec3V(u).GetXf(),Vec3V(u).GetYf(),Vec3V(u).GetZf());

	Mat33V rotMat;
	Mat33VFromAxisAngle( rotMat, RCC_VEC3V(u), -ScalarV(theta) ); // Have to negate theta to stay compatible with this left-handed rotation.
	rotationMtx.Set3x3( rotMat );
	
	//float cos = cosf(theta);
	//float sin = sinf(theta);
	//float mc = 1.0f-cos;
	//float xmc = u.x*mc;
	//float xymc = xmc*u.y;
	//float xzmc = xmc*u.z;
	//float yzmc = u.y*u.z*mc;
	//float xs = u.x*sin;
	//float ys = u.y*sin;
	//float zs = u.z*sin;
	//rotationMtx.Set(u.x*u.x*mc+cos,xymc-zs,xzmc+ys, xymc+zs,u.y*u.y*mc+cos,yzmc-xs, xzmc-ys,yzmc+xs,u.z*u.z*mc+cos);
}

void phArticulatedBody::ApplyIncrementalJointLimitImpulse( int jointNum, int jointLimitID, ScalarV_In impulse )
{
	PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(CheckReadOnlyOnMainThread();)
	if (IsZeroAll(impulse))
		return;

	int childLinkNum = jointNum+1;
	int parentLinkNum = m_Type->m_JointParentIndex[jointNum];

	phJoint& theJoint = GetJoint(jointNum);

	phPhaseSpaceVector spatialImpulse;
	theJoint.GetJointLimitUnitImpulse( jointLimitID, spatialImpulse );

	spatialImpulse *= Vector3((-impulse).GetIntrin128());
	m_IncrementalBase = ApplyIncrementalImpulse(parentLinkNum, spatialImpulse.trans, spatialImpulse.omega, m_IncrementalBase);
	spatialImpulse.Negate();
	m_IncrementalBase = ApplyIncrementalImpulse(childLinkNum, spatialImpulse.trans, spatialImpulse.omega, m_IncrementalBase);
}

int phArticulatedBody::ApplyIncrementalImpulse (int link, Vec3V_In trans, Vec3V_In omega, int incrementalBase)
{
	PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(CheckReadOnlyOnMainThread();)
	// Convert impulse to delta velocity
	phPhaseSpaceVector deltaVelocity;
	phPhaseSpaceVector impulse;
	impulse.omega = omega;
	impulse.trans = trans;
	phArticulatedBodyPart& theLink = GetLink(link);
	theLink.m_LinkInertiaInverse.Transform( impulse, deltaVelocity );
	return AddIncrementalVelocityToLink(link,&deltaVelocity,incrementalBase);
}

int phArticulatedBody::AddIncrementalVelocityToLink (int linkNum, const phPhaseSpaceVector* RESTRICT deltaVel, int incrementalBase)
{
	PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(CheckReadOnlyOnMainThread();)
	VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(deltaVel->omega),Vec3V(V_FLT_LARGE_6)));
	VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(deltaVel->trans),Vec3V(V_FLT_LARGE_6)));

	if (m_RootIsFixed && linkNum == 0)
	{
		return incrementalBase;
	}

	if (incrementalBase!=-1)
	{
		PropagateIncrementalToLink(linkNum,incrementalBase);
	}

	incrementalBase = linkNum;		// This link is the new base link
	AddToVelocityNoProp(linkNum, *deltaVel);
	m_VelocitiesToPropUp[linkNum] += *deltaVel;

	return incrementalBase;
}


// Propagates the incremental velocities to linkNum so that
//   the DeltaVelocity field in linkNum is correct.
int phArticulatedBody::PropagateIncrementalToLink (int linkNum, int incrementalBase)
{

	// Strategy: Find the least common ancestor of incrementalBase
	//	and LinkNum.  While doing this, propagate incremental velocites
	//	up from incrementalBase.
	//  Then propagate incremental velocities down from the least common
	//  ancestor.  The LCA becomes the new incrementalBase

	if (linkNum >= GetNumBodyParts() || incrementalBase >= GetNumBodyParts())
	{
		Assertf(0, "phArticulatedBody::PropagateIncrementalToLink - bad indices provided.  link is %d, base is %d, numParts is %d", linkNum, incrementalBase, GetNumBodyParts());
		linkNum = Min(linkNum, GetNumBodyParts()-1);
		incrementalBase = Min(incrementalBase, GetNumBodyParts()-1);
	}

	if (incrementalBase==-1)
	{
		return -1;
	}

	// Stage 1: Propagate up from incrementalBase while seaching for
	//			the least common ancestor
	int linkB = linkNum;
	int treePathLength = 1;
	int treePath[phArticulatedBodyType::MAX_NUM_LINKS+1];
	int lastTreePathNode = 0;
	while (incrementalBase!=linkB ) 
	{
		if (incrementalBase>linkB)
		{
			Assert(incrementalBase>=1);
			int jointIndex = incrementalBase-1;
			int parent = m_Type->m_JointParentIndex[jointIndex];
			PropagateOnceUp(incrementalBase,parent );
			incrementalBase = parent;
		}
		else
		{
			Assert(linkB>=1);
			int jointIndex = linkB-1;
			int parent = m_Type->m_JointParentIndex[jointIndex];
			treePath[treePathLength++] = lastTreePathNode = linkB;
			linkB = parent;
		}
	}

	// Stage 2: Propagate down from the least common ancestor
	//			to the linkNum.
	int child = lastTreePathNode;
	while ( treePathLength>1 )
	{
		PropagateOnceDown( child, linkB );
		linkB = child;
		child = treePath[ (--treePathLength)-1 ];  
	}

	return incrementalBase;
}


void phArticulatedBody::PropagateOnceUp( int childLinkNum, int parentLinkNum )
{
	phJoint& theJoint = GetJoint(childLinkNum-1);

	// Upward propagated delta velocity (a) save in theJoint.m_VelocityToPropDown
	//		and (b) add into the parent's IncrementalDeltaVelocity
	phPhaseSpaceVector temp;
	theJoint.TransformByAupdown( m_VelocitiesToPropUp[childLinkNum], temp );
	VALIDATE_PHYSICS_ASSERT(temp==temp);
	VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(temp.omega),Vec3V(V_FLT_LARGE_6)));
	VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(temp.trans),Vec3V(V_FLT_LARGE_6)));
	theJoint.m_VelocityToPropDown += temp;
	AddToVelocityNoProp(parentLinkNum, temp);
	m_VelocitiesToPropUp[parentLinkNum] += temp;
	m_VelocitiesToPropUp[childLinkNum].SetZero();
}


void phArticulatedBody::PropagateOnceDown( int childLinkNum, int parentLinkNum )
{
	phJoint& theJoint = GetJoint(childLinkNum-1);
	phPhaseSpaceVector parentVelocity = GetVelocityNoProp(parentLinkNum);
	phPhaseSpaceVector velocityToPropDown = theJoint.m_VelocityToPropDown;
	phPhaseSpaceVector propDown = parentVelocity;
	propDown -= velocityToPropDown;
	phPhaseSpaceVector downDeltaVel;
	theJoint.TransformByAdownup( propDown, downDeltaVel );
	VALIDATE_PHYSICS_ASSERT(downDeltaVel==downDeltaVel);
	VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(downDeltaVel.omega),Vec3V(V_FLT_LARGE_6)));
	VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(downDeltaVel.trans),Vec3V(V_FLT_LARGE_6)));
	AddToVelocityNoProp(childLinkNum, downDeltaVel);
	theJoint.m_VelocityToPropDown = parentVelocity;
}


int phArticulatedBody::FindLeastCommonAncestor (int linkA, int linkB)
{
	while ( linkA!=linkB ) 
	{
		if ( linkA > linkB )
		{
			Assert(linkA>=1);
			int jointIndex = linkA-1;
			linkA = m_Type->m_JointParentIndex[jointIndex];
		}
		else
		{
			Assert(linkB>=1);
			int jointIndex = linkB-1;
			linkB = m_Type->m_JointParentIndex[jointIndex];
		}
	}

	return linkA;		// Now same as LinkB - the least common ancestor
}

void phArticulatedBody::GetInverseInertiaMatrix (Matrix33& outInvInertia, int bodyPartIndex) const
{
	const phArticulatedBodyInertia& inertiaInverse = GetLink(bodyPartIndex).GetLinkInertiaInverse();
	RC_MAT33V(outInvInertia) = inertiaInverse.m_Mass;
}


void phArticulatedBody::GetInverseMassMatrix (int linkNum, Vec::V3Param128 position, Matrix33& inverseMassMatrix) const
{
	Mat33V v_invMassMatrix = RCC_MAT33V( inverseMassMatrix );

	Mat33V cross;
	CrossProduct(cross, RCC_VEC3V(position));

	Assertf(linkNum>=0 && linkNum<GetNumBodyParts(),"phArticulatedBody::GetInverseMassMatrix has linkNum %i and there are %i links.",linkNum,GetNumBodyParts());
	// Guess we must have had a suspected null link pointer here in gta4
	Assert(m_LinkPtr[linkNum]);

	const phArticulatedBodyInertia& inertiaInverse = GetLink(linkNum).GetLinkInertiaInverse();
	Multiply( v_invMassMatrix, cross, inertiaInverse.m_Mass );
	Mat33V zzz(v_invMassMatrix);
	Transpose( v_invMassMatrix, cross );
	Multiply( v_invMassMatrix, zzz, v_invMassMatrix );
	zzz = cross;
	Multiply( cross, zzz, inertiaInverse.m_CrossInertia );
	v_invMassMatrix += cross;
	Transpose( cross, cross );
	v_invMassMatrix += cross;
	v_invMassMatrix += inertiaInverse.m_InvInertia;

	RC_MAT33V(inverseMassMatrix) = v_invMassMatrix;
}


void phArticulatedBody::GetInverseMassMatrix (int linkA, Vec::V3Param128 positionA, int linkB, Vec::V3Param128 positionB, Matrix33& inverseMassMatrix)
{
	Vec3V xaxis = Vec3V(V_X_AXIS_WZERO);
	Vec3V yaxis = Vec3V(V_Y_AXIS_WZERO);
	Vec3V zaxis = Vec3V(V_Z_AXIS_WZERO);

	Vec3V resultX = GetIMM_Helper(linkA, positionA, xaxis.GetIntrin128(), linkB, positionB);
	Vec3V resultY = GetIMM_Helper(linkA, positionA, yaxis.GetIntrin128(), linkB, positionB);
	Vec3V resultZ = GetIMM_Helper(linkA, positionA, zaxis.GetIntrin128(), linkB, positionB);

	Mat33V outMat(resultX, resultY, resultZ);
	Transpose( outMat, outMat );

	RC_MAT33V(inverseMassMatrix) = outMat;
}

Vec3V_Out phArticulatedBody::GetIMM_Helper (int linkA, Vec::V3Param128 positionA, Vec::V3Param128 forceA, int linkB, Vec::V3Param128 positionB)
{
	Vec3V v_forceA = RCC_VEC3V(forceA);

	// Convert applied force to spatial vector.
	phPhaseSpaceVector spatialForceA;
	spatialForceA.trans = Cross(RCC_VEC3V(positionA), v_forceA);
	spatialForceA.omega = v_forceA;

	// Further convert to spatial acceleration of linkA
	phPhaseSpaceVector spatialAccel;
	GetLink(linkA).GetLinkInertiaInverse().Transform( spatialForceA, spatialAccel );

	// Propagate accelerations up to least common ancestor of linkA and linkB
	phPhaseSpaceVector temp;
	int linkAancestor = linkA;
	int linkBancestor = linkB;
	int treePathLength = 0;
	int treePath[phArticulatedBodyType::MAX_NUM_LINKS];

	while ( linkAancestor!=linkBancestor )
	{
		if ( linkAancestor<linkBancestor )
		{
			treePath[treePathLength++] = linkBancestor;
			Assert(linkBancestor>=0);
			int jointIndex = linkBancestor-1;
			linkBancestor = m_Type->m_JointParentIndex[jointIndex];
		}
		else {
			phJoint& aJoint = GetJoint(linkAancestor-1);
			aJoint.TransformByAupdown( spatialAccel, temp );
			spatialAccel = temp;
			Assert(linkAancestor>=1);
			int jointIndex = linkAancestor-1;
			linkAancestor = m_Type->m_JointParentIndex[jointIndex];
		}
	}

	// Propagate accelerations back down to linkB
	while ( treePathLength>0 )
	{
		phJoint& aJoint = GetJoint(treePath[--treePathLength]-1);
		aJoint.TransformByAdownup( spatialAccel, temp );
		spatialAccel = temp;
	}

	// Convert spatial acceleration at position B on link B back to linear acceleration.
	Vec3V v_outAcc;
	v_outAcc = Cross(spatialAccel.omega, RCC_VEC3V(positionB));
	v_outAcc += spatialAccel.trans;

	return v_outAcc;
}


void phArticulatedBody::SetVelocity (Vec3V_In velocity)
{
	EnsureVelocitiesFullyPropagated();

	// Find the change in the linear velocity of the root part.
	Vec3V rootDelVelocity = Subtract(velocity, GetLinearVelocityNoProp(0));

	// Change the linear velocity of all the body parts by the same amount.
	Vec3V newPartVelocity;
	for (int bodyPartIndex=0; bodyPartIndex<GetNumBodyParts(); bodyPartIndex++)
	{
		newPartVelocity = Add(GetLinearVelocityNoProp(bodyPartIndex), rootDelVelocity);
		SetLinearVelocityNoDownVelAdjust(bodyPartIndex, newPartVelocity);
	}

	ResetPropagationVelocities();
}


float phArticulatedBody::GetGravityFactor()
{
	return m_Type->m_ReplaceUponReResource;
}


u16 phArticulatedBody::FindJointLimitDofs (int* jointIndices, int* dofIndices, float* dofExcessHard, float* dofResponse, float allowedAnglePenetration, u16& numDeepLimitDofsOut)
{
	// Want to keep this - see comment in articulatedBody.h
	if (m_ResetLastFrame)
	{
		m_ResetLastFrame = false;
		return 0;
	}

	// Reset counter of deeply exceeded joint dof lims
	m_NumVeryDeepLimitDofs=0;
	int numDeepLimitDofs = 0;

	ASSERT_ONLY(m_JointLimitsInvalid = false;)

	int totalNumLimitDofs=0,numJoints=GetNumBodyParts()-1,numLimitDofs;
	for (int jointIndex=0; jointIndex<numJoints; jointIndex++)
	{
		bool jointLimDeeplyExceeded = false;
		numLimitDofs = GetJoint(jointIndex).ComputeNumHardJointLimitDofs(this);
		for (int limitDofIndex=0; limitDofIndex<numLimitDofs; limitDofIndex++)
		{
			jointIndices[totalNumLimitDofs] = jointIndex;
			GetJoint(jointIndex).GetJointLimitDetailedInfo(this, limitDofIndex,dofIndices[totalNumLimitDofs],dofExcessHard[totalNumLimitDofs],dofResponse[totalNumLimitDofs]);

			float excess = Abs(dofExcessHard[totalNumLimitDofs]);

			if (excess > allowedAnglePenetration * 2.0f)
			{
				numDeepLimitDofs++;
			}
			// Track number of deeply exceeded joint dof lims for use in determining if a ragdoll has become unstable
			static float excessLim = 0.4f;
			if (!jointLimDeeplyExceeded && excess > excessLim)
			{
				m_NumVeryDeepLimitDofs++;
				jointLimDeeplyExceeded = true;
			}

			totalNumLimitDofs++;
		}
	}

	numDeepLimitDofsOut = (u16)numDeepLimitDofs;
	return (u16)totalNumLimitDofs;
}


void phArticulatedBody::RejuvenateMatrices ()
{
	for (int bodyPartIndex=0; bodyPartIndex<GetNumBodyParts(); bodyPartIndex++)
	{
		GetLink(bodyPartIndex).RejuvenateMatrix();
	}
}

#if __PPU
//void phArticulatedBodyType::GenerateDmaPlan(sysDmaPlan& dmaPlan)
//{
	//if (GetNumBodyParts() > 1)
	//{
	//	dmaPlan.AddArray(m_JointTypes, GetNumBodyParts() - 1, true);
	//	ADD_DMA_REF(m_JointTypes);
	//}
//}

void phArticulatedBody::GenerateDmaPlan(sysDmaPlan& dmaPlan)
{
	ADD_DMA_OWNER_READ_ONLY(m_Type.ptr);
	ADD_DMA_ARRAY_OWNER(m_PartVelocities.GetElements(), m_PartVelocities.GetCapacity());
	ADD_DMA_ARRAY_OWNER(m_VelocitiesToPropUp.GetElements(), m_VelocitiesToPropUp.GetCapacity());
	ADD_DMA_ARRAY_OWNER_READ_ONLY(m_AngInertiaXYZmassW.GetElements(), m_AngInertiaXYZmassW.GetCapacity());
// 	m_Type->GenerateDmaPlan(dmaPlan);

	for (int linkIndex = 0; linkIndex < GetNumBodyParts(); ++linkIndex)
	{
		phArticulatedBodyPart*& link = m_LinkPtr[linkIndex];
		ADD_DMA_OWNER_READ_ONLY(link);
	}

	for (int jointIndex = 0; jointIndex < GetNumJoints(); ++jointIndex)
	{
		switch (GetJoint(jointIndex).GetJointType())
		{
			case phJoint::JNT_1DOF:
			{
				phJoint1Dof*& joint1Dof = (phJoint1Dof*&)m_JointPtr[jointIndex];
 				ADD_DMA_OWNER_PARTIAL(joint1Dof, joint1Dof, &joint1Dof->m_MuscleAngleStrength);
				break;
			}
			case phJoint::JNT_3DOF:
			{
				phJoint3Dof*& joint3Dof = (phJoint3Dof*&)m_JointPtr[jointIndex];
				ADD_DMA_OWNER_PARTIAL(joint3Dof, joint3Dof, &joint3Dof->UpInertiaJC);
				break;
			}
			default:
			{
				Assert(GetJoint(jointIndex).GetJointType() == phJoint::PRISM_JNT);
				phPrismaticJoint*& prismaticJoint = (phPrismaticJoint*&)m_JointPtr[jointIndex];
				ADD_DMA_OWNER_PARTIAL(prismaticJoint, prismaticJoint, prismaticJoint->GetCoreEnd());
				break;
			}
		}

		//ADD_DMA_REF(m_JointPtr[jointIndex]->m_Type.ptr);
		ADD_DMA_OWNER_READ_ONLY(m_JointPtr[jointIndex]->m_Type.ptr);
	}
}
#endif

IMPLEMENT_PLACE(phArticulatedBodyType);

#if __DECLARESTRUCT
void phArticulatedBodyType::DeclareStruct(datTypeStruct &s)
{
	m_NumJoints = m_NumLinks-1;
	pgBase::DeclareStruct(s);
	SSTRUCT_BEGIN_BASE(phArticulatedBodyType, pgBase)
		SSTRUCT_CONTAINED_ARRAY_COUNT(phArticulatedBodyType, m_JointParentIndex, MAX_NUM_LINKS)
		SSTRUCT_FIELD(phArticulatedBodyType, m_ReplaceUponReResource)
		SSTRUCT_FIELD(phArticulatedBodyType, m_AngularDecayRate)  // Remove this the next time you rebuild ped ragdoll assets
		SSTRUCT_DYNAMIC_ARRAY(phArticulatedBodyType, m_phJointTypes, m_NumJoints)
		SSTRUCT_DYNAMIC_ARRAY(phArticulatedBodyType, m_ResourcedAngInertiaXYZmassW, m_NumLinks)
		SSTRUCT_FIELD(phArticulatedBodyType, m_NumLinks)
		SSTRUCT_FIELD(phArticulatedBodyType, m_NumJoints)
		SSTRUCT_CONTAINED_ARRAY_COUNT(phArticulatedBodyType, m_JointTypes, MAX_NUM_JOINTS)
		SSTRUCT_FIELD(phArticulatedBodyType, m_LocallyOwned)
		SSTRUCT_IGNORE(phArticulatedBodyType, m_Pad)
		SSTRUCT_END(phArticulatedBodyType);
}
#endif // __DECLARESTRUCT

phArticulatedBodyType::phArticulatedBodyType(datResource &rsc)
{
	rsc.PointerFixup(m_phJointTypes);

	for(int i=0; i<m_NumLinks-1;i++)
	{
		ObjectFixup(rsc, m_phJointTypes[i].ptr);
		phJointType::VirtualConstructFromPtr(rsc, m_phJointTypes[i], m_phJointTypes[i]->m_JointType);	
	}
}

phArticulatedBodyType::phArticulatedBodyType(bool locallyOwned)
{
	m_LocallyOwned = locallyOwned;
	m_NumLinks = 0;
	m_phJointTypes = NULL;
	m_ResourcedAngInertiaXYZmassW = NULL;
	m_ReplaceUponReResource = 1.0f;
}


phArticulatedBodyType::~phArticulatedBodyType()
{
	// If locally owned, the types were deleted in ~phArticulatedBody()
	if (!m_LocallyOwned)
	{
		// delete joint types
		for (int i=0; i<m_NumLinks-1; i++)
		{
			if (m_JointTypes[i] == 0)
			{
				phJoint1DofType *joint = static_cast<phJoint1DofType*>(m_phJointTypes[i].ptr);
				if (joint)
					delete joint;
			}
			else if (m_JointTypes[i] == 1)
			{
				phJoint3DofType *joint = static_cast<phJoint3DofType*>(m_phJointTypes[i].ptr);
				if (joint)
					delete joint;
			}
			else if (m_JointTypes[i] == 2)
			{
				phPrismaticJointType *joint = static_cast<phPrismaticJointType*>(m_phJointTypes[i].ptr);
				if (joint)
					delete joint;
			}
			else
			{
				AssertMsg(0, "~phArticulatedBodyType - Bad joint type");
			}
		}
	}
}

phArticulatedBodyType* phArticulatedBodyType::Clone()
{
	phArticulatedBodyType* clone = rage_new phArticulatedBodyType;
	*clone = *this;

	clone->m_phJointTypes = rage_new datOwner<phJointType>[m_NumLinks-1];

	for (int i = 0; i < m_NumLinks-1; ++i)
	{
		clone->m_phJointTypes[i] = m_phJointTypes[i]->Clone();
	}

	clone->m_ResourcedAngInertiaXYZmassW = rage_new Vec4V[m_NumLinks];
	sysMemCpy(clone->m_ResourcedAngInertiaXYZmassW, m_ResourcedAngInertiaXYZmassW, m_NumLinks * sizeof(Vec4V));

	clone->m_LocallyOwned = true;

	return clone;
}

phArticulatedBody::phArticulatedBody() 
{
	// want to keep - see comment in articulatedBody.h
	m_ResetLastFrame = true;
	m_NumVeryDeepLimitDofs = 0;
	m_IncrementalBase = -1;

	// Set pointer to the type data
	m_Type = rage_new phArticulatedBodyType(true);

	m_PartVelocities.Resize(1);
	m_VelocitiesToPropUp.Resize(1);
	m_AngInertiaXYZmassW.Resize(1);
	ZeroLinkVelocity(0); 
	m_AngInertiaXYZmassW[0].ZeroComponents();

	m_RootIsFixed = false;

	PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(m_ReadOnlyOnMainThread = false;)
}


phArticulatedBody::phArticulatedBody(phArticulatedBodyType *bodyType) 
{
	// want to keep - see comment in articulatedBody.h
	m_ResetLastFrame = true;
	m_NumVeryDeepLimitDofs = 0;
	m_IncrementalBase = -1;

	// Set pointer to the type data
	m_Type = bodyType;

	m_RootIsFixed = false;

	//_Type->m_NumLinks = 0;
	PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(m_ReadOnlyOnMainThread = false;)
}


phArticulatedBody::~phArticulatedBody()
{
	if (m_Type && m_Type->m_LocallyOwned)
	{
		// delete the joint types
		for(int i=0; i<GetNumBodyParts()-1;i++)
		{
			phJoint &jointBase = GetJoint(i);
			if (jointBase.GetJointType() == phJoint::JNT_1DOF)
			{
				phJoint1Dof *joint = static_cast<phJoint1Dof*>(&jointBase);
				if (joint->m_Type)
					delete &joint->Get1DofType();
			}
			else if (jointBase.GetJointType() == phJoint::JNT_3DOF)
			{
				phJoint3Dof *joint = static_cast<phJoint3Dof*>(&jointBase);
				if (joint->m_Type)
					delete &joint->Get3DofType();
			}
			else if (jointBase.GetJointType() == phJoint::PRISM_JNT)
			{
				phPrismaticJoint *joint = static_cast<phPrismaticJoint*>(&jointBase);
				if (joint->m_Type)
					delete &joint->GetPrismaticType();
			}
			else
			{
				AssertMsg(0, "~phArticulatedBody - Bad joint type");
			}
		}
	}

	for(int i=0; i<GetNumBodyParts()-1;i++)
	{
		delete &GetLink(i);
		delete &GetJoint(i);
	}
	if (GetNumBodyParts() > 0)
	{
		delete &GetLink(GetNumBodyParts() - 1);
	}

	if (m_Type && m_Type->m_LocallyOwned)
	{
		// delete the body type
		delete m_Type;
	}
}


void phArticulatedBody::NullOutTypeDataPointers()
{
	int numParts = GetNumBodyParts();

	// Null out the joint types
	for(int i=0; i<numParts-1;i++)
	{
		phJoint &jointBase = GetJoint(i);
		jointBase.m_Type = NULL;
		jointBase.m_Type = NULL;
	}

	// Null out the body type
	if (m_Type)
	{
		m_Type = NULL;
	}
}


void phArticulatedBody::AllocateJointsAndParts (int numParts, const phJoint::JointType* jointTypeList)
{
	for (int partIndex=0; partIndex<numParts; partIndex++)
	{
		SetBodyPart(partIndex, rage_new phArticulatedBodyPart());
	}

	int numJoints = numParts-1;
	for (int jointIndex=0; jointIndex<numJoints; jointIndex++)
	{
		if (jointTypeList[jointIndex]==phJoint::JNT_3DOF)
		{
			SetJoint(jointIndex, rage_new phJoint3Dof);
		}
		else if (jointTypeList[jointIndex]==phJoint::JNT_1DOF)
		{
			SetJoint(jointIndex, rage_new phJoint1Dof);
		}
		else
		{
			Assert(jointTypeList[jointIndex]==phJoint::PRISM_JNT);
			SetJoint(jointIndex, rage_new phPrismaticJoint);
		}
	}
}

void phArticulatedBody::PreInit(int numLinks)
{
	m_PartVelocities.Reserve(numLinks); 
	m_VelocitiesToPropUp.Reserve(numLinks); 
	m_AngInertiaXYZmassW.Reserve(numLinks); 
}

// Add the root link -- always returns zero
int phArticulatedBody::AddRoot( phArticulatedBodyPart& link )
{
	//FastAssert ( GetNumBodyParts() == 0 );
	SetBodyPart(0,&link);
	m_Type->SetNumBodyParts(1);

	InvalidateJointLimits();

	// Update the velocity array sizes
	m_PartVelocities.ResizeGrow(1); 
	m_VelocitiesToPropUp.ResizeGrow(1); 
	m_AngInertiaXYZmassW.ResizeGrow(1); 

	// Reset the handled flag for the link being added
	ZeroLinkVelocity(0);
	m_AngInertiaXYZmassW[0].ZeroComponents();

	return 0;			// The root link is link number zero
}


int phArticulatedBody::AddChild (int parentIndex, phJoint& joint, phArticulatedBodyPart& bodyPart)
{
	PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(CheckReadOnlyOnMainThread();)
	// Make sure there is a body part available and that the parent is already set.
	Assert ( GetNumBodyParts() < phArticulatedBodyType::MAX_NUM_LINKS );
	Assert ( parentIndex < GetNumBodyParts() );

	// Set the child body part pointer.
	SetBodyPart(GetNumBodyParts(), &bodyPart);

	// Reset the joint's down vel
	joint.m_VelocityToPropDown.SetZero();

	// Set the joint pointer.
	int numJoints = GetNumBodyParts()-1;
	SetJoint(numJoints, &joint);

	// Return the index of the child body part, and increment the number of body parts.
	int numLinks = GetNumBodyParts();
	m_Type->SetNumBodyParts(numLinks+1);

	InvalidateJointLimits();

	// Set the parent body part index for this joint.
	m_Type->m_JointParentIndex[numJoints] = parentIndex;
	joint.m_Type->m_ParentLinkIndex = (u8) parentIndex;
	joint.m_Type->m_ChildLinkIndex = (u8) numLinks;
	Assert(parentIndex >= 0 && numLinks >= 0);

	// Update the velocity array sizes
	m_PartVelocities.ResizeGrow(GetNumBodyParts()); 
	m_VelocitiesToPropUp.ResizeGrow(GetNumBodyParts()); 
	m_AngInertiaXYZmassW.ResizeGrow(GetNumBodyParts()); 

	// Reset the handled flag for the link being added
	ZeroLinkVelocity(GetNumBodyParts()-1);
	m_AngInertiaXYZmassW[GetNumBodyParts()-1].ZeroComponents();

	return numLinks;
}


void phArticulatedBody::InitChain (phBoundComposite& compositeBound, bool threeDofJoints, const Matrix34& bodyMatrix, bool interlinked)
{
	ScalarV v_half(V_HALF);

	int numParts = compositeBound.GetNumBounds();
	int partIndex;
	for (partIndex=0; partIndex<numParts; partIndex++)
	{
		SetBodyPart(partIndex, rage_new phArticulatedBodyPart());
	}

	int numJoints = numParts-1;
	int jointIndex;
	for (jointIndex=0; jointIndex<numJoints; jointIndex++)
	{
		if (threeDofJoints)
		{
			SetJoint(jointIndex, rage_new phJoint3Dof);
		}
		else
		{
			SetJoint(jointIndex, rage_new phJoint1Dof);
		}
	}

	// Save locals.
	Mat34V v_bodymatrix = RCC_MAT34V(bodyMatrix);

	Mat34V initialPartMtx;
	initialPartMtx.Set3x3(v_bodymatrix);
	Transpose3x3( initialPartMtx, initialPartMtx );
	initialPartMtx.SetCol3( Vec3V(V_ZERO) );
	ScalarV v_verticalOffset;
	const float firstPartMass = 70.0f;
	const float partMass = 70.0f;
	for (partIndex=0; partIndex<numParts; partIndex++)
	{
		GetLink(partIndex).SetMatrix(RCC_MATRIX34(initialPartMtx));
		if (partIndex<numParts-1)
		{
			// Move the position of the next part downward from the position of this part.
			if(!interlinked)
			{
				v_verticalOffset = SplatY( compositeBound.GetBound(partIndex)->GetBoundingBoxMin()-
					compositeBound.GetBound(partIndex+1)->GetBoundingBoxMax() );
			}
			else
			{
				ScalarV v_first = - static_cast<phBoundCapsule*>(compositeBound.GetBound(partIndex))->GetLengthV();
				ScalarV v_second = static_cast<phBoundCapsule*>(compositeBound.GetBound(partIndex+1))->GetLengthV();
				v_verticalOffset = Scale(v_first, v_half) - Scale(v_second, v_half);
			}
			initialPartMtx.SetCol3( AddScaled(initialPartMtx.GetCol3(), v_bodymatrix.GetCol1(), Vec3V(v_verticalOffset)) );
		}
	}

	// Position the body parts before joining them together.
	ZeroLinkVelocities();

	// Add the first part as the root of the body.
	Assert(&GetLink(0));
	AddRoot(GetLink(0));

	// Find the position of the first joint.
	Vec3V jointPosition = v_bodymatrix.GetCol1();
	if(!interlinked)
	{
		jointPosition = Scale( jointPosition, Vec3V(SplatY( compositeBound.GetBound(0)->GetBoundingBoxMin() )) );
	}
	else
	{
		jointPosition = SubtractScaled( Vec3V(V_ZERO), jointPosition, Scale(static_cast<phBoundCapsule*>(compositeBound.GetBound(0))->GetLengthV(), v_half));
	}

	// Set the rotation axis and angle limits.
	Vec3V jointAxis;
	const float leanLimit = PH_DEG2RAD(90.0f);
	const float twistLimit = PH_DEG2RAD(90.0f);
	float limit1,limit2;
	if (threeDofJoints)
	{
		jointAxis = v_bodymatrix.GetCol1();
		limit1 = leanLimit;
		limit2 = twistLimit;
	}
	else
	{
		jointAxis = v_bodymatrix.GetCol2();
		limit1 = -leanLimit;
		limit2 = leanLimit;
	}

	int parentPartIndex=0,childPartIndex=1;
	for (jointIndex=0; jointIndex<numJoints; jointIndex++)
	{
		AddChild(parentPartIndex,GetJoint(jointIndex),GetLink(childPartIndex));
		GetJoint(jointIndex).SetAxis(this, RCC_VECTOR3(jointPosition),RCC_VECTOR3(jointAxis));
		GetJoint(jointIndex).SetAngleLimits(limit1,limit2);
		if(jointIndex<numJoints-1) {
			if(!interlinked)
			{
				Vec3V inertiaBox = compositeBound.GetBound(jointIndex+1)->GetBoundingBoxSize();
				jointPosition = SubtractScaled(jointPosition, v_bodymatrix.GetCol1(), Vec3V(SplatY(inertiaBox)));

			}
			else
			{
				jointPosition = SubtractScaled(jointPosition, v_bodymatrix.GetCol1(), static_cast<phBoundCapsule*>(compositeBound.GetBound(jointIndex+1))->GetLengthV());
			}
		}
		parentPartIndex++;
		childPartIndex++;
	}

	// Set the inertias
	for (partIndex=0; partIndex<numParts; partIndex++)
	{
		float mass = (partIndex>0 ? partMass : firstPartMass);
		Assert(compositeBound.GetBound(partIndex));
		phBound* boundPart = compositeBound.GetBound(partIndex);
		Assert(boundPart);
		Vector3 angInertia(VEC3V_TO_VECTOR3(boundPart->GetComputeAngularInertia(mass)));
		SetMassAndAngInertia(partIndex,mass,angInertia);
	}

	SetStiffness(0.0f);
}


void phArticulatedBody::RemoveChild (int linkIndex)
{
	PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(CheckReadOnlyOnMainThread();)
	Assert ( linkIndex >= 0 && linkIndex < GetNumBodyParts() );

	// Reset the part velocities for the link being removed and slide the rest down
	if (linkIndex+1 < GetNumBodyParts())
	{
		memmove(&m_PartVelocities[linkIndex], &m_PartVelocities[linkIndex+1], (GetNumBodyParts()-linkIndex-1) * sizeof(phPhaseSpaceVector));
		memmove(&m_VelocitiesToPropUp[linkIndex], &m_VelocitiesToPropUp[linkIndex+1], (GetNumBodyParts()-linkIndex-1) * sizeof(phPhaseSpaceVector));
		memmove(&m_AngInertiaXYZmassW[linkIndex], &m_AngInertiaXYZmassW[linkIndex+1], (GetNumBodyParts()-linkIndex-1) * sizeof(Vec4V));
	}

	// Move all the pointers down, copying over the one that's being removed.
	memmove(&m_LinkPtr[linkIndex], &m_LinkPtr[linkIndex+1], (GetNumBodyParts()-linkIndex-1) * sizeof(phArticulatedBodyPart*));

	if (linkIndex != 0)
	{
		// Reset the joint's child index and downVel
		m_JointPtr[linkIndex-1]->m_Type->m_ChildLinkIndex = INVALID_AB_LINK_INDEX;
		m_JointPtr[linkIndex-1]->m_VelocityToPropDown.SetZero();

		memmove(&m_JointPtr[linkIndex-1],   &m_JointPtr[linkIndex],   (GetNumBodyParts()-linkIndex-1) * sizeof(phJoint*));
		memmove(&m_Type->m_JointTypes[linkIndex-1], &m_Type->m_JointTypes[linkIndex], (GetNumBodyParts()-linkIndex-1) * sizeof(u8));

		// Move all the parent link indices down, replacing anything that was using the
		// removed link as a parent with the removed link's parent.
		int parent = GetParentNum(linkIndex);
		for ( int j = linkIndex; j < GetNumBodyParts()-1; j++ )
		{
			int movedParent = m_Type->m_JointParentIndex[j];

			if (movedParent == linkIndex)
			{
				movedParent = parent;
			}
			else if (movedParent > linkIndex)
			{
				movedParent--;
			}

			m_Type->m_JointParentIndex[j-1] = movedParent;
			GetJoint(j-1).m_Type->m_ParentLinkIndex = (u8) movedParent;
			GetJoint(j-1).m_Type->m_ChildLinkIndex = (u8) j;
		}
	}

	// Zero out the last velocity entry
	ZeroLinkVelocity(GetNumBodyParts()-1);
	m_AngInertiaXYZmassW[GetNumBodyParts()-1].ZeroComponents();

	m_Type->SetNumBodyParts(GetNumBodyParts()-1);

	InvalidateJointLimits();

	// Update the velocity array sizes
	m_PartVelocities.ResizeGrow(GetNumBodyParts()); 
	m_VelocitiesToPropUp.ResizeGrow(GetNumBodyParts()); 
	m_AngInertiaXYZmassW.ResizeGrow(GetNumBodyParts()); 
}

void phArticulatedBody::Freeze ()
{
	PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(CheckReadOnlyOnMainThread();)
	ZeroLinkVelocities();

	int numJoints = GetNumBodyParts()-1;
	for (int jointIndex=0; jointIndex<numJoints; jointIndex++)
	{
		GetJoint(jointIndex).Freeze();
	}

	m_IncrementalBase = -1;
}


void phArticulatedBody::SetStiffness( float stiffness )
{
	int i;
	phJoint*const* jointPP = GetJointArray();		// Point to first joint
	for ( i=GetNumBodyParts()-1; i>0; i-- ) {
		(*jointPP)->SetStiffness( stiffness );
		jointPP++;
	}
}


void phArticulatedBody::SetBodyMinimumStiffness( float minStiffness )
{
	int i;
	phJoint*const* jointPP = GetJointArray();		// Point to first joint
	for ( i=GetNumBodyParts()-1; i>0; i-- ) {
		(*jointPP)->SetMinStiffness( minStiffness );
		jointPP++;
	}
}


// want to keep - see comment in articulatedBody.h
void phArticulatedBody::ApplyGravityToLinks (Vec::V3Param128 gravity, Vec::V3Param128 timestep, float gravityFactor, bool skipRoot)
{
#if !__SPU
	VALIDATE_PHYSICS_ASSERTF(FPIsFinite(Vec3V(gravity).GetXf()) && FPIsFinite(Vec3V(gravity).GetYf()) && FPIsFinite(Vec3V(gravity).GetZf()),"ApplyGravityToLinks has %f, %f, %f",Vec3V(gravity).GetXf(),Vec3V(gravity).GetYf(),Vec3V(gravity).GetZf());
#endif // !__SPU

	skipRoot = skipRoot || m_RootIsFixed;
	if (!skipRoot) 
	{
		EnsureVelocitiesFullyPropagated();

		Vec3V gravityImpulse = Scale(RCC_VEC3V(gravity), Vec3VFromF32(gravityFactor));
		gravityImpulse = Scale(gravityImpulse, ScalarV(timestep));
		skipRoot = skipRoot || m_RootIsFixed;
		// want to keep - see comment in articulatedBody.h
		for (int partIndex=0; partIndex<GetNumBodyParts(); partIndex++)
		{
			SetLinearVelocityNoDownVelAdjust(partIndex, Add(GetLinearVelocityNoProp(partIndex), gravityImpulse));
		}

		ResetPropagationVelocities();
	}
	else
	{
		Vec3V gravityForce(V_ZERO);
		Vec3V gravityAccel = RCC_VEC3V(gravity);
		gravityAccel = Scale(gravityAccel, Vec3VFromF32(gravityFactor));
		// want to keep - see comment in articulatedBody.h
		for (int partIndex=skipRoot ? 1 : 0; partIndex<GetNumBodyParts(); partIndex++)
		{
			gravityForce = Scale(gravityAccel, GetMass(partIndex));
			ApplyForce(partIndex, RCC_VECTOR3(gravityForce), GetLink(partIndex).GetPosition(), timestep);
		}
	}
}


// *******************************************************************
// ArticulatedBody::CalculateInertias()
//          This function is called when the links all have
//		the correct initial position and orientation.
//		It assumes the bodies are positioned correctly so that
//		they are joined properly by their links.
//			It computes quantities that depend only on the positions
//		and orientations, and that do not depend on the links' 
//		velocities.
//			This includes calculating all the articulated body inertias
//		and the local cross-accelerations A_{i,j} where links i and j
//		are connected by a link.
// ********************************************************************
void phArticulatedBody::CalculateInertias ()
{
	// prefetch some stuff so we avoid L2 cache misses
	for (int partIndex=0; partIndex<GetNumJoints(); partIndex++)
	{
		PrefetchDC( &(GetJoint(partIndex).m_DownInertia) );
	}

	phArticulatedBodyInertia* linkInertias = Alloca(phArticulatedBodyInertia, GetNumBodyParts());

	int partIndex = 0;
	if (m_RootIsFixed)
	{
		GetLink(0).CalcRBI(linkInertias[partIndex], GetMassAndAngInertia(0) * Vec4V(V_TEN) * Vec4V(V_TEN));
		partIndex = 1;
	}

	// Phase 0: Initialize
	for (; partIndex<GetNumBodyParts(); partIndex++)
	{
		phArticulatedBodyPart& bodyPart = GetLink(partIndex);
		bodyPart.CalcRBI(linkInertias[partIndex], GetMassAndAngInertia(partIndex));
	}

	if (GetNumBodyParts() > 1)
	{
		// Main phase 1

		// Starting from the root, calculate A_{i,j} where
		//		link j is the parent to link i
		int i;
		// Loop over all links from the leaves upwards
		phJoint*const* jointPP = GetJointArray();
		jointPP += GetNumBodyParts()-2;		// Point to last joint
		int* parentNumPtr = m_Type->m_JointParentIndex+(GetNumBodyParts()-2);				// Point to parent indices						
		for ( i=GetNumBodyParts() - 1; i>0; i-- ) {
			phJoint& theJoint = **jointPP;							// Its parent joint

			// linkInertias[i] presently holds the I^A ("down inertia") value.

			theJoint.m_DownInertia = linkInertias[i];			// Henceforth holds the "m_DownInertia"
			theJoint.PrecomputeAdownup(this);						// Current link's I^A is found in "m_DownInertia"		

			theJoint.TransformInertiaByASdownup( theJoint.m_UpInertia );	// Inertia transferred upwards
			// As of now, theJoint.m_UpInertia temporarily holds the inertia transmitted up to the parent link.
			linkInertias[*parentNumPtr] += theJoint.m_UpInertia;			// Accumulate in parent's inertia
			jointPP--;
			parentNumPtr--;
		}


		// prefetch phase 2 stuff so we avoid L2 cache misses
		/*	for (int partIndex=0; partIndex<GetNumJoints(); partIndex++)
		{
		PrefetchDC( &(((phJoint3Dof*)(m_JointPtr[ partIndex ]))->DownInertiaJC) );
		PrefetchDC( &(((phJoint3Dof*)(m_JointPtr[ partIndex ]))->DownIAstiffnessInvI) );
		}	
		*/
		// Main phase 2:
		// Loop over all links from the root downward
		phArticulatedBodyInertia temp;
		jointPP = GetJointArray();							// Point to first joint
		parentNumPtr = m_Type->m_JointParentIndex;				// Point to first parent index							
		for ( i=1; i<GetNumBodyParts(); i++ ) {
			phJoint& theJoint = **jointPP;							// Its parent joint

			// The parent link inertia is already the final articulated inertia I^B
			theJoint.m_UpInertia.SubtractFrom( linkInertias[*parentNumPtr] );
			theJoint.PrecomputeAupdown();					// Parent inertia I^A is in "Up Inertia"
			theJoint.TransformInertiaByASupdown( temp );	// Inertia transfered down to child link
			linkInertias[i] += temp;
			jointPP++;
			parentNumPtr++;
			PrefetchDC( &GetLink( GetNumBodyParts() - i ).m_LinkInertiaInverse );
			PrefetchDC( &GetLink( GetNumBodyParts() - i ).m_LinkInertiaInverse.m_InvInertia );
		}
	}

	// Phase 3: Invert all link inertia matrices.
	for (int partIndex=0; partIndex<GetNumBodyParts(); partIndex++)
	{
		phArticulatedBodyPart& bodPart = GetLink(partIndex);
		linkInertias[partIndex].Inverse(bodPart.m_LinkInertiaInverse);
	}

	if (m_RootIsFixed)
	{
		GetLink(0).m_LinkInertiaInverse.SetM(Matrix33(Matrix33::ZeroType));
		GetLink(0).m_LinkInertiaInverse.SetH(Matrix33(Matrix33::ZeroType));
		GetLink(0).m_LinkInertiaInverse.SetI(Matrix33(Matrix33::ZeroType));
	}

	InvalidateJointLimits();
}


void phArticulatedBody::ClampVelocities (Vec::V3Param128 maxSpeed, Vec::V3Param128 maxAngSpeed)
{
	for (int bodyPartIndex=0; bodyPartIndex<GetNumBodyParts(); bodyPartIndex++)
	{
		// Clamp the angular velocity.
		Vec3V angularVelocity = ClampMag(GetAngularVelocityNoProp(bodyPartIndex), ScalarV(V_ZERO), ScalarV(maxAngSpeed));

		// Clamp the linear velocity.
		Vec3V linearVelocity = ClampMag(GetLinearVelocityNoProp(bodyPartIndex), ScalarV(V_ZERO), ScalarV(maxSpeed));

		// Recompute the 6D velocity from the linear and angular velocities.
		SetVelocitiesNoDownVelAdjust(bodyPartIndex, linearVelocity, angularVelocity);
	}
	ResetPropagationVelocities();
}


void phArticulatedBody::UpdateVelocities (Vec::V3Param128 timeStep)
{
	// Get the root link linear velocity, to remove it from all the body part linear velocities.
	Vector3 removedVelocity(VEC3V_TO_VECTOR3(GetLinearVelocityNoProp(0)));
	Vector3 reducedVelocity;

	// Get all the body part inertias, and adjust all the body part velocities.
	for (int partIndex=0; partIndex<GetNumBodyParts(); partIndex++)
	{
		phArticulatedBodyPart &bodyPart = GetLink(partIndex);
		PrefetchDC( &(bodyPart.m_LinkInertiaInverse) );
		PrefetchDC( &(bodyPart.m_LinkInertiaInverse.m_InvInertia) );

		// Subtract the root link linear velocity from this body part linear velocity.
		reducedVelocity.Subtract(VEC3V_TO_VECTOR3(GetLinearVelocityNoProp(partIndex)),removedVelocity);
		VALIDATE_PHYSICS_ASSERT(reducedVelocity==reducedVelocity);
		SetLinearVelocityNoDownVelAdjust(partIndex, RCC_VEC3V(reducedVelocity));
	}
	ResetPropagationVelocities();

	// Calculate bias forces for each link. Bias forces compensate for the articulated body simulating in its own moving reference frame.
	phPhaseSpaceVector* biasForces = Alloca(phPhaseSpaceVector, GetNumBodyParts());
	CalculateBiasForces(biasForces);

	// Update all velocities from bias forces.
	Vector3 negTimeStep = timeStep;
	negTimeStep.Negate();
	phPhaseSpaceVector deltaVelocity;
	for (int partIndex=0; partIndex<GetNumBodyParts(); partIndex++)
	{
		// Update velocity based on bias force.
		phArticulatedBodyPart& bodyPart = GetLink(partIndex);
		bodyPart.m_LinkInertiaInverse.Transform(biasForces[partIndex],deltaVelocity);
		deltaVelocity *= negTimeStep;
		AddToVelocityNoProp(partIndex, deltaVelocity);
	}

	// Restore the root part velocity back into all the body parts.
	Vector3 restoredVelocity;
	for (int partIndex=0; partIndex<GetNumBodyParts(); partIndex++)
	{
		restoredVelocity.Add(VEC3V_TO_VECTOR3(GetLinearVelocityNoProp(partIndex)),removedVelocity);
		SetLinearVelocityNoDownVelAdjust(partIndex, RCC_VEC3V(restoredVelocity));
	}
	ResetPropagationVelocities();

	InvalidateJointLimits();
}

// **************************************************************************
// "Incremental Delta Velocity" - intended for use by "accumulator" LCP solver
// **************************************************************************

// Initialize for incremental delta velocity calculations.
//		Clear the handled flag information.  
//		Reset "top" and "base"
//		Clear the incremental delta velocities
void phArticulatedBody::ResetPropagationVelocities()
{
	for ( int i=0; i<GetNumBodyParts(); i++ ) 
	{
		m_VelocitiesToPropUp[i].SetZero();
	}

	for ( int i=0; i<GetNumBodyParts()-1; i++ ) {
		phJoint& theJoint = GetJoint(i);
		theJoint.m_VelocityToPropDown = GetVelocityNoProp(theJoint.GetParentLinkIndex());
	}
	m_IncrementalBase = -1;
}

void phArticulatedBody::EnsureVelocitiesFullyPropagated()
{
	PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(CheckReadOnlyOnMainThread();)
	if (m_IncrementalBase != -1)
	{
		// Propagate up
		int numBodyParts = GetNumBodyParts();
		int childLinkIndex = 0, parentLinkIndex = 0, jointIndex = 0;
		while (m_IncrementalBase != 0) 
		{
			jointIndex = m_IncrementalBase-1;
			parentLinkIndex = m_Type->GetJointParentIndex(jointIndex);
			PropagateOnceUp(m_IncrementalBase, parentLinkIndex);
			m_IncrementalBase = parentLinkIndex;
		}

		// Propagate down
		for (jointIndex = 0; jointIndex < numBodyParts-1; jointIndex++)
		{
			childLinkIndex = jointIndex + 1;
			parentLinkIndex = m_Type->GetJointParentIndex(jointIndex);
			PropagateOnceDown(childLinkIndex, parentLinkIndex);
		}

		// Reset handled flags and the incremental base
		m_IncrementalBase = -1;
	}

#if __ASSERT
	//CheckVelocitiesFullyPropagated();
#endif
}

#if __ASSERT
void phArticulatedBody::CheckVelocitiesFullyPropagated()
{
	Assert(m_IncrementalBase == -1);

	for ( int i=0; i<GetNumJoints(); i++ ) 
		Assert(GetJoint(i).m_VelocityToPropDown == GetVelocityNoProp(GetJoint(i).GetParentLinkIndex()));

	for (int i=1; i<GetNumBodyParts(); i++)
	{
		Assert(m_VelocitiesToPropUp[i].IsZero());
	}
}
#endif

#if PHARTICULATEDBODY_MULTITHREADED_VALDIATION
void phArticulatedBody::SetReadOnlyOnMainThread(bool readOnly)
{
#if __DEV
	if(readOnly)
	{
		CheckVelocitiesFullyPropagated();
	}
#endif // __DEV
	m_ReadOnlyOnMainThread = readOnly;
}
void phArticulatedBody::CheckReadOnlyOnMainThread() const
{
#if !__SPU && !__TOOL
	if(!Verifyf(!m_ReadOnlyOnMainThread || !sysThreadType::IsUpdateThread(), "An articulated body that should be read only on the main thread has been modified."))
	{
		//__debugbreak();
	}
#endif
}
#endif // PHARTICULATEDBODY_MULTITHREADED_VALDIATION

void phArticulatedBody::ComputeAndApplyMuscleTorques (float timeStep)
{
	if (!sm_EffectorDrivingEnabled)
		return;

	int numJoints = GetNumJoints();
	for (int jointIndex=0; jointIndex<numJoints; jointIndex++)
	{
		PrefetchDC( &GetJoint(jointIndex).m_DriveState );
	}

	for (int jointIndex=0; jointIndex<numJoints; jointIndex++)
	{	// 560, 1680, 520
		//Displayf( "%d, %d, %d\n", sizeof( phJoint ), sizeof( phJoint3Dof ), sizeof( phArticulatedBody ) );
		phJoint& joint = GetJoint(jointIndex);
		if (joint.MuscleDriveActive())
		{
			joint.ComputeAndApplyMuscleTorques(this, timeStep);
		}
	}
}

void phArticulatedBody::SaveVelocities(Vec3V *savedLinearVelocities, Vec3V *savedAngularVelocities)
{
	for (int bodyPartIndex=0; bodyPartIndex<GetNumBodyParts(); bodyPartIndex++)
	{
		savedLinearVelocities[bodyPartIndex] = GetLinearVelocityNoProp(bodyPartIndex);
		savedAngularVelocities[bodyPartIndex] = GetAngularVelocityNoProp(bodyPartIndex);
	}
}

void phArticulatedBody::SaveAndZeroVelocities(Vec3V * RESTRICT savedLinearVelocities, Vec3V * RESTRICT savedAngularVelocities)
{
	const int numBodyParts = GetNumBodyParts();
	const int numJoints = numBodyParts-1;
	int nextBodyPartIndex = numJoints;

	PrefetchDC(&m_PartVelocities[nextBodyPartIndex]);
	PrefetchDC(&savedLinearVelocities[nextBodyPartIndex]);
	PrefetchDC(&savedAngularVelocities[nextBodyPartIndex]);
	PrefetchDC(&m_VelocitiesToPropUp[nextBodyPartIndex]);

	phJoint*const* jointPP = GetJointArray();
	m_IncrementalBase = -1;
	for (int bodyPartIndex = nextBodyPartIndex; bodyPartIndex >= 0; bodyPartIndex=nextBodyPartIndex)
	{
		--nextBodyPartIndex;
		if(nextBodyPartIndex >= 0)
		{
			// Prefetch the next set of values we're going to use. 
			PrefetchDC(&m_PartVelocities[nextBodyPartIndex]);
			PrefetchDC(&savedLinearVelocities[nextBodyPartIndex]);
			PrefetchDC(&savedAngularVelocities[nextBodyPartIndex]);
			PrefetchDC(&jointPP[nextBodyPartIndex]->m_VelocityToPropDown);
			PrefetchDC(&m_VelocitiesToPropUp[nextBodyPartIndex]);
		}
		savedLinearVelocities[bodyPartIndex] = GetLinearVelocityNoProp(bodyPartIndex);
		savedAngularVelocities[bodyPartIndex] = GetAngularVelocityNoProp(bodyPartIndex);
		ZeroLinkVelocity(bodyPartIndex);
		if(bodyPartIndex < numJoints)
		{
			jointPP[bodyPartIndex]->m_VelocityToPropDown.SetZero();
		}
	}

}

void phArticulatedBody::RestoreVelocities(Vec3V *savedLinearVelocities, Vec3V *savedAngularVelocities)
{
	int linkIndex;
	for (linkIndex=0; linkIndex<GetNumBodyParts(); linkIndex++)
	{
		SetVelocitiesNoDownVelAdjust(linkIndex, savedLinearVelocities[linkIndex], savedAngularVelocities[linkIndex]);
	}
	ResetPropagationVelocities();
}
#ifdef USE_SOFT_LIMITS
// Apply torques judiciously to try to enforce joint limits
// Each joint limit that is exceeded is handled separately, so 
//	interactions between different joint limits are not considered.
// The articulated body inertias, as computed by the Featherstone
//	algorithm, are used to determine appropriate torques to overcome
//	joint limit violations.
void phArticulatedBody::EnforceSoftJointLimits (float timeStep)
{
	int numJoints = GetNumJoints();
	for (int jointIndex=0; jointIndex<numJoints; jointIndex++)
	{
		GetJoint(jointIndex).EnforceSoftJointLimits(timeStep);
	}
}
#endif
// Bias forces depend on Link Inertias and Velocities,
//		but not on extenally applied forces or impulses
inline void phArticulatedBody::CalculateBiasForces(phPhaseSpaceVector* biasForces)
{
	// Phase 1: Calculate the base bias force for each link independently.
	phPhaseSpaceVector biasForce;
	phPhaseSpaceVector temp;
	phPhaseSpaceVector& linkMomentum = temp;				// Temporary variable
	phArticulatedBodyPart*const* linkPP = GetLinkArray();
	phJoint*const* jointPP = GetJointArray();

	int numJoints = GetNumJoints();
	int numBodyParts = numJoints + 1;

	// Loop over each link
	for (int i = 0; i < numBodyParts; ++i) {
		phArticulatedBodyPart& theLink = **linkPP;
		phArticulatedBodyInertia localInertia;
		theLink.CalcRBI(localInertia, (i == 0 && m_RootIsFixed) ? GetMassAndAngInertia(i) * Vec4V(V_TEN) * Vec4V(V_TEN) : GetMassAndAngInertia(i));
		const phPhaseSpaceVector& velocity = GetVelocityNoProp(i);
		localInertia.Transform( velocity, linkMomentum );
		biasForce = velocity;
		biasForce *= linkMomentum;
		biasForces[i] = biasForce;
		linkPP++;
		PrefetchDC( &((*jointPP)->m_DownInertia) );
		PrefetchDC( &((*jointPP)->m_DownInertia.m_InvInertia) );
		jointPP++;
	}

	PrefetchDC( m_Type->m_JointParentIndex );
	PrefetchDC( &(m_Type->m_JointParentIndex[numJoints-1])  );

	// Phase 2:
	//    propagate bias forces values upward in the tree

	// Loop over links and joints from leaves upward
	jointPP = GetJointArray() + (numJoints - 1);		// Point to last joint
	phPhaseSpaceVector* upBiasForces = Alloca(phPhaseSpaceVector, numJoints);
	phPhaseSpaceVector* upBiasForcePP = upBiasForces + (numJoints - 1);
	int* parentNumPtr = m_Type->m_JointParentIndex + (numJoints - 1);				// Point to parent indices
	for ( int i = numJoints; i > 0; i-- ) {
		phJoint& theJoint = **jointPP;							// Its parent joint

		phPhaseSpaceVector crossVelocity = GetVelocityNoProp(*parentNumPtr);
		crossVelocity *= GetVelocityNoProp(i);			// v_{parent} cross v_{current}
		theJoint.m_DownInertia.Transform( crossVelocity, temp );		// Use the "A" Inertia (downward only)
		temp += biasForces[i];
		phPhaseSpaceVector upBiasForce;
		theJoint.TransformByASdownup( temp, upBiasForce );
		biasForces[*parentNumPtr] += upBiasForce;
		*upBiasForcePP = upBiasForce;
		PrefetchDC( &((*jointPP)->m_UpInertia) );
		jointPP--;
		upBiasForcePP--;
		parentNumPtr--;
	}

	// Phase 3:
	//	propagate bias forces downward

	// Loop over links and joints from the root downward
	jointPP = GetJointArray();							// Point to first joint
	upBiasForcePP = upBiasForces;
	parentNumPtr = m_Type->m_JointParentIndex;				// Point to first parent index	
	for ( int i=1; i<numBodyParts; ++i ) {
		phJoint& theJoint = **jointPP;							// Its parent joint

		phPhaseSpaceVector crossVelocity = GetVelocityNoProp(*parentNumPtr);
		crossVelocity *= GetVelocityNoProp(i);			// v_{parent} cross v_{current}
		theJoint.m_UpInertia.Transform( crossVelocity, temp );		// Use the "A" inertia (upward only)
		temp.SubtractFrom( biasForces[*parentNumPtr] );
		temp -= *upBiasForcePP;
		biasForce = biasForces[i];
		theJoint.TransformByASupdownAndAdd( temp, biasForce );	
		biasForces[i] = biasForce;
		jointPP++;
		upBiasForcePP++;
		parentNumPtr++;
	}
}



// **************************************************************************
// Nonphysical routines that correct for "drift"
// **************************************************************************

// Starting at root, match up positions and velocities of links
void phArticulatedBody::JoinBackPositionsAndVelocitiesToRoot()
{
	int i;
	for ( i=GetNumJoints()-1; i>0; i-- ) 
	{
		PrefetchDC( &GetJoint(i) );
	}

	phArticulatedBodyPart*const* linkPP = GetLinkArray()+1;							// Point to first non-root
	phJoint*const* jointPP = GetJointArray();							// Point to first joint
	for ( i=GetNumBodyParts()-1; i>0; i-- ) {
		phJoint& theJoint = **jointPP;							// Its parent joint

		theJoint.MatchChildToParentPositionAndVelocity(this);

		jointPP++;
		linkPP++;
	}

	ResetPropagationVelocities();
}

// Starting at root, match up positions and velocities of links
// Also, preserve center of mass and linear momentum
void phArticulatedBody::JoinBackPositionsAndVelocities()
{
	if (m_RootIsFixed)
	{
		JoinBackPositionsAndVelocitiesToRoot();
	}
	else
	{
		phArticulatedBodyPart& rootBodyPart = GetLink(0);
		ScalarV totalMass = GetMass(0);
		Vec3V centerMass = RCC_VEC3V(rootBodyPart.GetPosition());
		centerMass = Scale( centerMass, totalMass );
		Vec3V momentum = GetLinearVelocityNoProp(0);
		momentum = Scale(momentum, totalMass);								// Root does not change position or velocity (yet)
		Vec3V newCenterMass(centerMass);
		Vec3V newMomentum(momentum);
		phArticulatedBodyPart*const* linkPP = GetLinkArray()+1;					// Point to first non-root
		phJoint*const* jointPP = GetJointArray();					// Point to first joint
		int i, linkIndex;
		for ( i=GetNumBodyParts()-1; i>0; i-- ) {
			phArticulatedBodyPart& theLink = **linkPP;							// The current link
			phJoint& theJoint = **jointPP;							// Its parent joint
			linkIndex = GetNumBodyParts() - i;

			ScalarV mass = GetMass(linkIndex);
			centerMass = AddScaled(centerMass, RCC_VEC3V(theLink.GetPosition()), mass);
			momentum = AddScaled(momentum, GetLinearVelocityNoProp(linkIndex), mass);
			totalMass += mass;
			theJoint.MatchChildToParentPositionAndVelocity(this);
			newCenterMass = AddScaled(newCenterMass, RCC_VEC3V(theLink.GetPosition()), mass);
			newMomentum = AddScaled(newMomentum, GetLinearVelocityNoProp(linkIndex), mass);
			jointPP++;
			linkPP++;
		}

		Vec3V adjustVelocity = momentum;
		adjustVelocity = Subtract(adjustVelocity, newMomentum);
		adjustVelocity = InvScale(adjustVelocity, totalMass);
		Vec3V adjustPosition = centerMass;			// Old center of mass times mass
		adjustPosition = Subtract(adjustPosition, newCenterMass);			// Minus new center of mass times mass
		adjustPosition = InvScale(adjustPosition, totalMass);				// Negative change in center of mass position

		linkPP = GetLinkArray();										// Point to root
		Vec3V position;
		for ( i=GetNumBodyParts(); i>0; i-- ) {
			phArticulatedBodyPart& theLink = **linkPP;			// The current link
			linkIndex = GetNumBodyParts() - i;
			Vec3V linearVelocity = GetLinearVelocityNoProp(linkIndex) + adjustVelocity;
			position = Add(RCC_VEC3V(theLink.GetPosition()), adjustPosition);
			theLink.SetPosition(RCC_VECTOR3(position));
			SetLinearVelocityNoDownVelAdjust(linkIndex, linearVelocity);
			linkPP++;
		}
		ResetPropagationVelocities();
	}
}

void phArticulatedBody::GetInertiaMatrix (Matrix33& outInertia, int bodyPartIndex) const
{
	const phArticulatedBodyInertia& inertiaInv = GetLink(bodyPartIndex).m_LinkInertiaInverse;
	phArticulatedBodyInertia linkInertia;
	inertiaInv.Inverse(linkInertia);
	RC_MAT33V(outInertia) = linkInertia.m_InvInertia;
}

void phArticulatedBody::SetAngVelocity (Vec3V_In angVelocity)
{
	EnsureVelocitiesFullyPropagated();

	// Find the change in the angular velocity of the root part.
	Vec3V rootDelAngVelocity = Subtract(angVelocity, GetAngularVelocityNoProp(0));

	// Change the angular velocity of all the body parts by the same amount, and change all of their linear velocities due to their
	// offsets from the root part and the change in the angular velocity of the root part.
	for (int bodyPartIndex=0; bodyPartIndex<GetNumBodyParts(); bodyPartIndex++)
	{
		// Change the linear velocity of this body part.
		phArticulatedBodyPart& bodyPart = GetLink(bodyPartIndex);
		Vec3V newPartVelocity = AddCrossed(GetLinearVelocityNoProp(bodyPartIndex), rootDelAngVelocity, RCC_VEC3V(bodyPart.GetPosition()));

		// Change the angular velocity of this body part.
		Vec3V newPartAngVelocity = Add(GetAngularVelocityNoProp(bodyPartIndex), rootDelAngVelocity);
		SetVelocitiesNoDownVelAdjust(bodyPartIndex, newPartVelocity, newPartAngVelocity);
	}
	ResetPropagationVelocities();
}

void phArticulatedBody::Copy (const phArticulatedBody& original)
{
	// Adding this assert at a time that this function isn't used in game
	Assertf(0, "phArticulatedBody::Copy - This function isn't currently copying over velocities and inertias.");

	// Save the body part pointers.
	int numBodyParts = original.GetNumBodyParts();
	phArticulatedBodyPart* linkPtr[phArticulatedBodyType::MAX_NUM_LINKS];
	for (int bodyPartIndex=0; bodyPartIndex<numBodyParts; bodyPartIndex++)
	{
		linkPtr[bodyPartIndex] = &GetLink(bodyPartIndex);
	}

	// Save the joint pointers.
	int numJoints = original.GetNumJoints();
	phJoint* jointPtr[phArticulatedBodyType::MAX_NUM_JOINTS];
	for (int jointIndex=0; jointIndex<numJoints; jointIndex++)
	{
		jointPtr[jointIndex] = &GetJoint(jointIndex);
	}

	// Copy the original.
	*this = original;

	// Restore the body part pointers and copy the body parts.
	for (int bodyPartIndex=0; bodyPartIndex<numBodyParts; bodyPartIndex++)
	{
		linkPtr[bodyPartIndex]->Copy(original.GetLink(bodyPartIndex)); 
		SetBodyPart(bodyPartIndex, linkPtr[bodyPartIndex]);
	}

	// Restore the joint pointers and copy the joints.
	for (int jointIndex=0; jointIndex<numJoints; jointIndex++)
	{
		switch (jointPtr[jointIndex]->GetJointType())
		{
		case phJoint::JNT_1DOF:
			{
				static_cast<phJoint1Dof*>(jointPtr[jointIndex])->Copy(original.GetJoint1Dof(jointIndex));
				break;
			}

		case phJoint::JNT_3DOF:
			{
				static_cast<phJoint3Dof*>(jointPtr[jointIndex])->Copy(original.GetJoint3Dof(jointIndex)); 
				break;
			}

		case phJoint::PRISM_JNT:
			{
				static_cast<phPrismaticJoint*>(jointPtr[jointIndex])->Copy(original.GetPrismaticJoint(jointIndex));
				break;
			}
		}
		SetJoint(jointIndex, jointPtr[jointIndex]);
	}
}

#if PHARTICULATED_DEBUG_SERIALIZE
void phArticulatedBody::DebugSerialize()
{
	fiSafeStream s = ASSET.Create("body1", "dat", false);
	Assert(s);
	fiAsciiTokenizer t;
	char buff[256];
	t.Init(buff, s);
	t.PutDelimiter("articulated body\n");
	t.PutDelimiter("num links: "); t.Put(GetNumBodyParts()); t.PutDelimiter("num links: ");

	t.PutDelimiter("handled flags:\n");

	t.PutDelimiter("incremental base: "); t.Put(m_IncrementalBase); t.PutDelimiter("\n");
	t.PutDelimiter("gravity factor: "); t.Put(m_ReplaceUponReResource); t.PutDelimiter("\n");

	for (int i = 0; i < GetNumBodyParts(); i++)
	{
		t.PutDelimiter("link "); t.Put(i); t.PutDelimiter(":\n");

		t.StartBlock();

		GetLink(i).DebugSerialize(t);

		t.EndBlock();
	}

	for (int i = 0; i < GetNumBodyParts() - 1; i++)
	{
		t.PutDelimiter("joint "); t.Put(i); t.PutDelimiter(":\n");

		t.StartBlock();

		GetJoint(i).DebugSerialize(t);

		t.EndBlock();
	}
}
#endif // PHARTICULATED_DEBUG_SERIALIZE

} // namespace rage
