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
 */


#include "NmRsInclude.h"
#include "NmRsBodyLayout.h"
#include "NmRsCBU_PointArm.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_AnimPose.h"

namespace ART
{
  NmRsCBUPointArm::NmRsCBUPointArm(ART::MemoryManager* services) : CBUTaskBase(services, bvid_pointArm)
  {
    initialiseCustomVariables();
  }

  NmRsCBUPointArm::~NmRsCBUPointArm()
  {
  }

  void NmRsCBUPointArm::initialiseCustomVariables()
  {
    m_mask = bvmask_Arms;

    m_parameters_Right.useLeftArm = false;
    m_parameters_Left.useLeftArm = false;
    m_parameters_Right.useRightArm = false;
    m_parameters_Left.useRightArm = false;

    m_parameters_Right.reachLength = 0.7f;
    m_parameters_Right.pointing = true;
    m_parameters_Right.useBodyVelocity = false;
    m_parameters_Right.advancedStaightness = 0.f;
    m_parameters_Right.advancedMaxSpeed = 0.f;

    m_parameters_Left.reachLength = 0.7f;
    m_parameters_Left.pointing = true;
    m_parameters_Left.useBodyVelocity = false;
    m_parameters_Left.advancedStaightness = 0.f;
    m_parameters_Left.advancedMaxSpeed = 0.f;
  }

  void NmRsCBUPointArm::onActivate()
  {
    Assert(m_character);
  }

  void NmRsCBUPointArm::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUPointArm::onTick(float )
  { 
    NmRsCBUAnimPose* animPoseTask = (NmRsCBUAnimPose*)m_cbuParent->m_tasks[bvid_animPose];
    Assert(animPoseTask);
    //Don't pointArm if animPose is active on that arm
    if ((m_parameters_Right.useRightArm) && (! (animPoseTask->isActive() && animPoseTask->m_parameters.overidePointArm && (animPoseTask->m_parameters.effectorMask & bvmask_ArmRight))))
      pointTheArm(m_parameters_Right);

    if ((m_parameters_Left.useLeftArm)  && (! (animPoseTask->isActive() && animPoseTask->m_parameters.overidePointArm && (animPoseTask->m_parameters.effectorMask & bvmask_ArmLeft))))
      pointTheArm(m_parameters_Left);

    return eCBUTaskComplete;
  }

  void NmRsCBUPointArm::pointTheArm(Parameters &parameters)
  {
    NmRsHumanArm *arm = (parameters.useLeftArm)?(getLeftArm()):(getRightArm());

    rage::Vector3 shoulderPos = arm->getShoulder()->getJointPosition();
    rage::Vector3 target, realTarget;
    m_character->instanceToWorldSpace(&realTarget, parameters.target, parameters.instanceIndex);
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "pointArm", realTarget);
#endif // ART_ENABLE_BSPY

    target.Normalize(realTarget - shoulderPos);
    target = shoulderPos + target*parameters.reachLength*parameters.armStraightness;

    float clavL1 = arm->getClavicle()->getDesiredLean1();
    float clavL2 = arm->getClavicle()->getDesiredLean2();
    float clavTw = arm->getClavicle()->getDesiredTwist();
    float shoulderL1 = arm->getShoulder()->getDesiredLean1();
    float shoulderL2 = arm->getShoulder()->getDesiredLean2();
    float shoulderTw = arm->getShoulder()->getDesiredTwist();

    rage::Vector3 comV = m_character->m_COMvel;

    NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
    NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
    ikInputData->setTarget(target);
    ikInputData->setTwist(parameters.twist);
    ikInputData->setMatchClavicle(kMatchClavicleBetter);
    if (parameters.useBodyVelocity)
    {
      ikInputData->setVelocity(comV);
      if ((parameters.advancedStaightness>0.f) && (parameters.advancedMaxSpeed>0.f))
      {
        ikInputData->setAdvancedMaxSpeed(parameters.advancedMaxSpeed);
        ikInputData->setAdvancedStaightness(parameters.advancedStaightness);
        ikInputData->setUseAdvancedIk(true);
      }
    }
    
    NmRsArmInputWrapper* armInputData = (parameters.useLeftArm) ? getLeftArmInputData() : getRightArmInputData();
    NmRsLimbInput& armInput = (parameters.useLeftArm) ? getLeftArmInput() : getRightArmInput();

    arm->setBodyStiffness(armInput, parameters.armStiffness, parameters.armDamping);

    arm->postInput(ikInput);

    // TDL hysteresis thing below
    if (!parameters.pointing && arm->getClavicle()->extentToLimit() < parameters.pointSwingLimit-0.1f)
      parameters.pointing = true;
    if (parameters.pointing && arm->getClavicle()->extentToLimit() > parameters.pointSwingLimit+0.1f)
      parameters.pointing = false;
    if (parameters.pointing)
    {
      armInputData->getClavicle()->setOpposeGravity(1.f);
      armInputData->getShoulder()->setOpposeGravity(1.f);
      armInputData->getElbow()->setOpposeGravity(1.f);
      armInputData->getWrist()->setOpposeGravity(1.f);

      armInputData->getClavicle()->setMuscleStiffness(1.5f);
      armInputData->getShoulder()->setMuscleStiffness(0.5f);
      armInputData->getElbow()->setMuscleStiffness(0.5f);
      armInputData->getWrist()->setDesiredLean1(0.f);
      armInputData->getWrist()->setDesiredLean2(0.f);
      armInputData->getWrist()->setDesiredTwist(0.f);
      float elbowAngle = arm->getElbow()->getDesiredAngle();
      armInputData->getWrist()->setDesiredLean1(elbowAngle * -0.5f);
    }
    else
    {
      if (parameters.useZeroPoseWhenNotPointing)
      {
        armInputData->getClavicle()->setDesiredLean1(0.f);
        armInputData->getClavicle()->setDesiredLean2(0.f);
        armInputData->getClavicle()->setDesiredTwist(0.f);
        armInputData->getShoulder()->setDesiredLean1(0.f);
        armInputData->getShoulder()->setDesiredLean2(0.5f);
        armInputData->getShoulder()->setDesiredTwist(0.f);
        armInputData->getElbow()->setDesiredAngle(0.5f);
        // TDL if there is no zero pose, we need a valid backup, hence the lines above.
        arm->blendToZeroPose(armInput, 1.f);
        arm->setBodyStiffness(armInput, 8.f, 1.f);
      }
      else
      {
        // reset arm to before the IK was applied
        armInputData->getClavicle()->setDesiredLean1(clavL1);
        armInputData->getClavicle()->setDesiredLean2(clavL2);
        armInputData->getClavicle()->setDesiredTwist(clavTw);
        armInputData->getShoulder()->setDesiredLean1(shoulderL1);
        armInputData->getShoulder()->setDesiredLean2(shoulderL2);
        armInputData->getShoulder()->setDesiredTwist(shoulderTw);
        //mmmmtodo BUG: elbow bend not reset - Kirk is taking advantage of this bug though  
      }
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUPointArm::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters_Left.useLeftArm, true);
    bspyTaskVar(m_parameters_Left.useZeroPoseWhenNotPointing, true);
    bspyTaskVar(m_parameters_Left.target, true);
    bspyTaskVar(m_parameters_Left.twist, true);
    bspyTaskVar(m_parameters_Left.armStraightness, true);
    bspyTaskVar(m_parameters_Left.armStiffness, true);
    bspyTaskVar(m_parameters_Left.armDamping, true);
    bspyTaskVar(m_parameters_Left.instanceIndex, true);
    bspyTaskVar(m_parameters_Left.pointSwingLimit, true);
    bspyTaskVar(m_parameters_Left.reachLength, true);

    bspyTaskVar(m_parameters_Right.useRightArm, true);
    bspyTaskVar(m_parameters_Right.useZeroPoseWhenNotPointing, true);
    bspyTaskVar(m_parameters_Right.target, true);
    bspyTaskVar(m_parameters_Right.twist, true);
    bspyTaskVar(m_parameters_Right.armStraightness, true);
    bspyTaskVar(m_parameters_Right.armStiffness, true);
    bspyTaskVar(m_parameters_Right.armDamping, true);
    bspyTaskVar(m_parameters_Right.instanceIndex, true);
    bspyTaskVar(m_parameters_Right.pointSwingLimit, true);
    bspyTaskVar(m_parameters_Right.reachLength, true);

    bspyTaskVar(m_parameters_Left.pointing, false);
    bspyTaskVar(m_parameters_Right.pointing, false);
  }
#endif // ART_ENABLE_BSPY
}
