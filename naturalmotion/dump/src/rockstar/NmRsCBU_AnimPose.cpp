/*
* Copyright (c) 2005-2012 NaturalMotion Ltd. All rights reserved. 
*
* Not to be copied, adapted, modified, used, distributed, sold,
* licensed or commercially exploited in any manner without the
* written consent of NaturalMotion. 
*
* All non public elements of this software are the confidential
* information of NaturalMotion and may not be disclosed to any
* person nor used for any purpose not expressly approved by
* NaturalMotion in writing.
*
*
* This will take any animation currently set as the incoming transforms of the agent and drive the 
* character to the pose defined therein.
*
* You can choose to have the animPose override the Headlook/PointArm/PointGun behaviours if the animPose
* mask includes (all) neck and head/ua,ul,ur/ua,ul,ur respectively.
*
* This is also the (temporary) place for setting up gravity compensation on the character.
*
* Masking Behaviour:
*
* As with all behaviours, m_mask (base member) will always be enforced.
*
* Things that respect the parameter mask are:
*   1. Initial set stiffness.
*   2. Active pose.
*   3. Initial gravity compensation.
*
* Things that do not respect the parameter mask
*   1. Per-limb stiffness settings.
*   2. Wrist and ankle stabilization.
*   3. Per-limb gravity compensation.
*/

#include "NmRsInclude.h"
#include "NmRsCBU_AnimPose.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

namespace ART
{
  NmRsCBUAnimPose::NmRsCBUAnimPose(ART::MemoryManager* services) : CBUTaskBase(services, bvid_animPose)
  {
    initialiseCustomVariables();
  }

  NmRsCBUAnimPose::~NmRsCBUAnimPose()
  {
  }

  void NmRsCBUAnimPose::initialiseCustomVariables()
  {
	  bDampedPlaneInitialized = false;
	  vDampenPlanePoint.Zero();
	  vDampenPlaneNormal.Zero();
  }

  void NmRsCBUAnimPose::onActivate()
  {
    Assert(m_character);
  }

  void NmRsCBUAnimPose::onDeactivate()
  {
    Assert(m_character);
    m_character->m_posture.useZMP = true;

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUAnimPose::onTick(float /*timeStep*/)
  {
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    if (m_character->overideAnimPose)
    {
      updateBehaviourMessage(NULL);
    }
#endif // NM_SCRIPTING && NM_SCRIPTING_VARIABLES

	// Handle side damping if requested
	if (PHLEVEL->IsInLevel(m_parameters.dampenSideMotionInstanceIndex))
	{
		rage::phInst *pInst = PHLEVEL->GetInstance(m_parameters.dampenSideMotionInstanceIndex);
		rage::Matrix34 intMat = MAT34V_TO_MATRIX34(pInst->GetMatrix());

		// Initialize the dampen plane to the character's sagital plane
		if (!bDampedPlaneInitialized)
		{
			// Grab the side vector and COM
			vDampenPlaneNormal = m_character->m_COMTM.a;
			vDampenPlanePoint = m_character->m_COM;

			// Store in dampenSideMotionInstanceIndex's local space 
			intMat.UnTransform3x3(vDampenPlaneNormal);
			vDampenPlaneNormal.Normalize();
			intMat.UnTransform(vDampenPlanePoint);

			bDampedPlaneInitialized = true;
		}

		// Bring the damping plane into world space
		rage::Vector3 vWorldPlanePoint, vWorldPlaneNormal;
		intMat.Transform3x3(vDampenPlaneNormal, vWorldPlaneNormal);
		vDampenPlaneNormal.Normalize();
		intMat.Transform(vDampenPlanePoint, vWorldPlanePoint);

		// Apply extra damping forces to restrict side-swinging
		static float minDistFromPlane = 0.1f;
		static float dampingMult = 300.0f;
		rage::Vector3 vPlaneToCOM = m_character->m_COM - vWorldPlanePoint;
		float fDistFromPlane = vPlaneToCOM.Dot(vWorldPlaneNormal);
		if (rage::Abs(fDistFromPlane) > minDistFromPlane)
		{
			rage::Vector3 vCorrectiveForce = -vWorldPlaneNormal * fDistFromPlane * dampingMult;
			getSpine()->getPelvisPart()->applyForce(vCorrectiveForce);
			getSpine()->getSpine0Part()->applyForce(vCorrectiveForce);
			getSpine()->getSpine1Part()->applyForce(vCorrectiveForce);
			getSpine()->getSpine2Part()->applyForce(vCorrectiveForce);
			getSpine()->getSpine3Part()->applyForce(vCorrectiveForce);
			getSpine()->getNeckPart()->applyForce(vCorrectiveForce);
			getSpine()->getHeadPart()->applyForce(vCorrectiveForce);
		}
	}

    NM_RS_DBG_LOGF(L"ANIM POSE");
    Assert(m_parameters.stiffness >= 2.f || m_parameters.stiffness < 0.f );
    //Don't set stiffness, damping or muscle stiffness if m_parameters.stiffness is negative
    //Don't set muscle stiffness if m_parameters.muscleStiffness is negative
    if (m_parameters.muscleStiffness < 0.f && m_parameters.stiffness >= 0.f)
      m_body->setStiffness(m_parameters.stiffness, m_parameters.damping, m_parameters.effectorMask);
    else if (m_parameters.stiffness >= 0.f)
      m_body->setStiffness(m_parameters.stiffness, m_parameters.damping, m_parameters.effectorMask, &m_parameters.muscleStiffness);

    m_body->activePose(m_parameters.animSource, m_parameters.effectorMask);

    if (m_parameters.gravityCompensation >= -0.00001f)
    {
      //mmmmmtodo add per effector for useZMP?
      m_character->m_posture.useZMP = m_parameters.useZMPGravityCompensation;
      callMaskedEffectorFunctionFloatArg(m_character, m_parameters.effectorMask, m_parameters.gravityCompensation, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
    }

    if (m_parameters.muscleStiffnessLeftArm < 0.f && m_parameters.stiffnessLeftArm >= 0.f)
      m_body->setStiffness(m_parameters.stiffnessLeftArm, m_parameters.dampingLeftArm, bvmask_ArmLeft);
    else if (m_parameters.stiffnessLeftArm >= 0.f)
      m_body->setStiffness(m_parameters.stiffnessLeftArm, m_parameters.dampingLeftArm, bvmask_ArmLeft, &m_parameters.muscleStiffnessLeftArm);

    if (m_parameters.muscleStiffnessRightArm < 0.f && m_parameters.stiffnessRightArm >= 0.f)
      m_body->setStiffness(m_parameters.stiffnessRightArm, m_parameters.dampingRightArm, bvmask_ArmRight);
    else if (m_parameters.stiffnessRightArm >= 0.f)
      m_body->setStiffness(m_parameters.stiffnessRightArm, m_parameters.dampingRightArm, bvmask_ArmRight, &m_parameters.muscleStiffnessRightArm);

    if (m_parameters.muscleStiffnessLeftLeg < 0.f && m_parameters.stiffnessLeftLeg >= 0.f)
      m_body->setStiffness(m_parameters.stiffnessLeftLeg, m_parameters.dampingLeftLeg, bvmask_LegLeft);
    else if (m_parameters.stiffnessLeftLeg >= 0.f)
      m_body->setStiffness(m_parameters.stiffnessLeftLeg, m_parameters.dampingLeftLeg, bvmask_LegLeft, &m_parameters.muscleStiffnessLeftLeg);

    if (m_parameters.muscleStiffnessRightLeg < 0.f && m_parameters.stiffnessRightLeg >= 0.f)
      m_body->setStiffness(m_parameters.stiffnessRightLeg, m_parameters.dampingRightLeg, bvmask_LegRight);
    else if (m_parameters.stiffnessRightLeg >= 0.f)
      m_body->setStiffness(m_parameters.stiffnessRightLeg, m_parameters.dampingRightLeg, bvmask_LegRight, &m_parameters.muscleStiffnessRightLeg);

    if (m_parameters.muscleStiffnessSpine < 0.f && m_parameters.stiffnessSpine >= 0.f)
    {
      m_body->setStiffness(m_parameters.stiffnessSpine, m_parameters.dampingSpine, bvmask_Spine);
    }
    else if (m_parameters.stiffnessSpine >= 0.f)
    {
      m_body->setStiffness(m_parameters.stiffnessSpine, m_parameters.dampingSpine, bvmask_Spine, &m_parameters.muscleStiffnessSpine);
    }

    //Stabilize wrists and ankles if connected
    float muscleStiffness = 12.f; 
    if (m_parameters.connectedLeftHand == 1 || (m_parameters.connectedLeftHand == -1 &&  getLeftArm()->getHand()->collidedWithNotOwnCharacter()))
      getLeftArmInputData()->getWrist()->setStiffness(16.f, 1.f, &muscleStiffness);
    if (m_parameters.connectedRightHand == 1 || (m_parameters.connectedRightHand == -1 &&  getRightArm()->getHand()->collidedWithNotOwnCharacter()))
      getRightArmInputData()->getWrist()->setStiffness(16.f, 1.f, &muscleStiffness);
    if (m_parameters.connectedLeftFoot == 1 || (m_parameters.connectedLeftFoot == -1 &&  getLeftLeg()->getFoot()->collidedWithNotOwnCharacter()))
      getLeftLegInputData()->getAnkle()->setStiffness(16.f, 1.f, &muscleStiffness);
    if (m_parameters.connectedRightFoot == 1 || (m_parameters.connectedRightFoot == -1 &&  getRightLeg()->getFoot()->collidedWithNotOwnCharacter()))
      getRightLegInputData()->getAnkle()->setStiffness(16.f, 1.f, &muscleStiffness);

    if (m_parameters.gravCompLeftArm >= -0.0001f ||
      m_parameters.gravCompRightArm >= -0.0001f ||
      m_parameters.gravCompLeftLeg >= -0.0001f ||
      m_parameters.gravCompRightLeg >= -0.0001f ||
      m_parameters.gravCompSpine >= -0.0001f)
      m_character->m_posture.useZMP = m_parameters.useZMPGravityCompensation;

    if (m_parameters.connectedLeftHand == -1)
      m_character->m_posture.leftArmAutoSet = true;
    else if (m_parameters.connectedLeftHand > 0)
      m_character->setLeftHandConnected(true);
    else if (m_parameters.connectedLeftHand == 0)
      m_character->setLeftHandConnected(false);

    if (m_parameters.connectedRightHand == -1)
      m_character->m_posture.rightArmAutoSet = true;
    else if (m_parameters.connectedRightHand > 0)
      m_character->setRightHandConnected(true);
    else if (m_parameters.connectedRightHand == 0)
      m_character->setRightHandConnected(false);

    if (m_parameters.connectedLeftFoot == -1)
      m_character->m_posture.leftLegAutoSet = true;
    else if (m_parameters.connectedLeftFoot > 0)
      m_character->setLeftFootConnected(true);
    else if (m_parameters.connectedLeftFoot == 0)
      m_character->setLeftFootConnected(false);

    if (m_parameters.connectedRightFoot == -1)
      m_character->m_posture.rightLegAutoSet = true;
    else if (m_parameters.connectedRightFoot > 0)
      m_character->setRightFootConnected(true);
    else if (m_parameters.connectedRightFoot == 0)
      m_character->setRightFootConnected(false);

    if (m_parameters.gravCompLeftArm >= -0.00001f)
      callMaskedEffectorFunctionFloatArg(m_character, bvmask_ArmLeft, m_parameters.gravCompLeftArm, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
    if (m_parameters.gravCompRightArm >= -0.00001f)
      callMaskedEffectorFunctionFloatArg(m_character, bvmask_ArmRight, m_parameters.gravCompRightArm, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
    if (m_parameters.gravCompLeftLeg >= -0.00001f)
      callMaskedEffectorFunctionFloatArg(m_character, bvmask_LegLeft, m_parameters.gravCompLeftLeg, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
    if (m_parameters.gravCompRightLeg >= -0.00001f)
      callMaskedEffectorFunctionFloatArg(m_character, bvmask_LegRight, m_parameters.gravCompRightLeg, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
    if (m_parameters.gravCompSpine >= -0.00001f)
      callMaskedEffectorFunctionFloatArg(m_character, bvmask_LowSpine, m_parameters.gravCompSpine, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);

    return eCBUTaskComplete;
  }

#if ART_ENABLE_BSPY
  void NmRsCBUAnimPose::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.muscleStiffness, true);
    bspyTaskVar(m_parameters.stiffness, true);
    bspyTaskVar(m_parameters.damping, true);
    bspyTaskVar_Bitfield32(m_parameters.effectorMask, true);
    bspyTaskVar(m_parameters.overideHeadlook, true);
    bspyTaskVar(m_parameters.overidePointArm, true);
    bspyTaskVar(m_parameters.overidePointGun, true);

    bspyTaskVar(m_parameters.useZMPGravityCompensation, true);
    bspyTaskVar(m_parameters.gravityCompensation, true);

    bspyTaskVar(m_parameters.muscleStiffnessLeftArm, true);
    bspyTaskVar(m_parameters.muscleStiffnessRightArm, true);
    bspyTaskVar(m_parameters.muscleStiffnessSpine, true);
    bspyTaskVar(m_parameters.muscleStiffnessLeftLeg, true);
    bspyTaskVar(m_parameters.muscleStiffnessRightLeg, true);

    bspyTaskVar(m_parameters.stiffnessLeftArm, true);
    bspyTaskVar(m_parameters.stiffnessRightArm, true);
    bspyTaskVar(m_parameters.stiffnessSpine, true);
    bspyTaskVar(m_parameters.stiffnessLeftLeg, true);
    bspyTaskVar(m_parameters.stiffnessRightLeg, true);

    bspyTaskVar(m_parameters.dampingLeftArm, true);
    bspyTaskVar(m_parameters.dampingRightArm, true);
    bspyTaskVar(m_parameters.dampingSpine, true);
    bspyTaskVar(m_parameters.dampingLeftLeg, true);
    bspyTaskVar(m_parameters.dampingRightLeg, true);

    bspyTaskVar(m_parameters.gravCompLeftArm, true);
    bspyTaskVar(m_parameters.gravCompRightArm, true);
    bspyTaskVar(m_parameters.gravCompLeftLeg, true);
    bspyTaskVar(m_parameters.gravCompRightLeg, true);
    bspyTaskVar(m_parameters.gravCompSpine, true);
    static const char* connectedTypeStrings[] = 
    {
      "balanceDecides",
      "AutoByImpacts",
      "Unconnected",
      "Fully",
      "Point",
      "Line",
    };  
    bspyTaskVar_StringEnum(m_parameters.connectedLeftHand+2, connectedTypeStrings, true);  
    bspyTaskVar_StringEnum(m_parameters.connectedRightHand+2, connectedTypeStrings, true);  
    bspyTaskVar_StringEnum(m_parameters.connectedLeftFoot+2, connectedTypeStrings, true);  
    bspyTaskVar_StringEnum(m_parameters.connectedRightFoot+2, connectedTypeStrings, true);  
    static const char* animSourceStrings[] = 
    {
      "kITSourceCurrent", /**< Transforms are for the current frame */
      "kITSourcePrevious",    /**< Transforms are for the previous frame */
#if NM_ANIM_MATRICES
      "kITSourceAnimation",
#endif
      "kITSourceUnknown",    /**< Transforms are for the previous frame */
      /* how many transform sources will be prepared */
    };
    int animSource = m_parameters.animSource;
    if (animSource<0 || animSource>=KITSourceCount)
      animSource = KITSourceCount;
    bspyTaskVar_StringEnum(animSource, animSourceStrings, true);  

  }
#endif // ART_ENABLE_BSPY
}