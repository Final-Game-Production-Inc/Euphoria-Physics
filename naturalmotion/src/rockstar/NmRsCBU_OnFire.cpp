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
 */


#include "NmRsInclude.h"
#include "NmRsCBU_OnFire.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"
#include "NmRsCBU_TaskManager.h"

#include "NmRsCBU_ArmsWindmill.h"
#include "NmRsCBU_BodyWrithe.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_BodyBalance.h"
#include "NmRsCBU_DynamicBalancer.h"

namespace ART
{
  NmRsCBUOnFire::NmRsCBUOnFire(ART::MemoryManager* services) : CBUTaskBase(services, bvid_onFire)
  {
    initialiseCustomVariables();
  }

  NmRsCBUOnFire::~NmRsCBUOnFire()
  {}

  void NmRsCBUOnFire::initialiseCustomVariables()
  {
    m_timer = 0.0f;
    m_orientationState = NmRsCharacter::OS_Up;
  }

  void NmRsCBUOnFire::onActivate()
  {
    setActivateBehaviours();
    //Stumble
    float minHP = -m_parameters.stumbleMaxLeanForward;
    float maxHP = m_parameters.stumbleMaxLeanBack;
    m_hipPitchAim = m_character->getRandom().GetRanged(minHP,maxHP);

    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    Assert(bodyBalanceTask);
    bodyBalanceTask->updateBehaviourMessage(NULL);// sets values to defaults
    bodyBalanceTask->m_parameters.m_useBodyTurn = false;
    bodyBalanceTask->m_parameters.m_useHeadLook = false;
    bodyBalanceTask->m_parameters.m_backwardsAutoTurn = true;
    bodyBalanceTask->m_parameters.m_backwardsArms = true;
    bodyBalanceTask->m_parameters.m_spineStiffness = 10.0f;
    bodyBalanceTask->m_parameters.m_spineDamping = 0.9f;
    bodyBalanceTask->m_parameters.m_armStiffness = 10.0f;
    bodyBalanceTask->m_parameters.m_elbow = 1.2f;
    bodyBalanceTask->m_parameters.m_shoulder = 0.8f;

    bodyBalanceTask->activate();


    dynamicBalancerTask->setAnkleEquilibrium(-0.00f);
    if (dynamicBalancerTask->isActive())
    {
      if (!balColReactTask->isActive() || balColReactTask->m_balancerState == bal_Normal || balColReactTask->m_balancerState == bal_Rebound)
      {
        dynamicBalancerTask->setLeftLegStiffness(10.f);
        dynamicBalancerTask->setRightLegStiffness(10.f);
      }
      dynamicBalancerTask->setOpposeGravityLegs(1.f);
      dynamicBalancerTask->setOpposeGravityAnkles(1.f);

      dynamicBalancerTask->setLowerBodyGravityOpposition(m_body);
      dynamicBalancerTask->calibrateLowerBodyEffectors(m_body);
    }
  }

  void NmRsCBUOnFire::onDeactivate()
  {
    initialiseCustomVariables();
    m_character->deactivateTask(bvid_armsWindmill);
    m_character->deactivateTask(bvid_bodyBalance);
    m_character->deactivateTask(bvid_bodyWrithe);
    m_character->deactivateTask(bvid_catchFall);
  }

  //m_orientationState must be set before calling
  void NmRsCBUOnFire::setActivateBehaviours()
  {
    // Update stumble parameters.
    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    Assert(bodyBalanceTask);
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    if (m_timer > m_parameters.staggerTime || dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK)
      bodyBalanceTask->deactivate();

    // Activate armsWindmill.
    NmRsCBUArmsWindmill* armsWindmillTask = (NmRsCBUArmsWindmill*)m_cbuParent->m_tasks[bvid_armsWindmill];
    Assert(armsWindmillTask);

    if (!armsWindmillTask->isActive() && m_orientationState == NmRsCharacter::OS_Up)
    {
      armsWindmillTask->updateBehaviourMessage(NULL); // Set parameters to defaults.
      armsWindmillTask->m_parameters.m_leftCircleDesc.speed = 1.2f;
      armsWindmillTask->m_parameters.m_rightCircleDesc.speed = 1.2f;
      armsWindmillTask->m_parameters.m_leftElbowMin = 0.9f;
      armsWindmillTask->m_parameters.m_rightElbowMin = 0.9f;
      armsWindmillTask->m_parameters.m_leftCircleDesc.radius1 = 0.8f;
      armsWindmillTask->m_parameters.m_leftCircleDesc.radius2 = 0.4f;
      armsWindmillTask->m_parameters.m_rightCircleDesc.radius1 = 0.8f;
      armsWindmillTask->m_parameters.m_rightCircleDesc.radius2 = 0.4f;
      armsWindmillTask->m_parameters.m_phaseOffset = 90.0f;
      armsWindmillTask->m_parameters.m_adaptiveMode = 1;
      armsWindmillTask->activate();
    }

    // Activate bodyWrithe.
    NmRsCBUBodyWrithe* writheTask = (NmRsCBUBodyWrithe*)m_cbuParent->m_tasks[bvid_bodyWrithe];
    Assert(writheTask);

    if (!writheTask->isActive())
    {
      writheTask->updateBehaviourMessage(NULL); // Set parameters to defaults.
      writheTask->m_parameters.m_backStiffness = 14.0f;
      writheTask->m_parameters.m_legStiffness = 10.0f;
      writheTask->m_parameters.m_armDamping = 0.2f;
      writheTask->m_parameters.m_backDamping = 0.5f;
      writheTask->m_parameters.m_legDamping = 0.3f;
      writheTask->m_parameters.m_backPeriod = 1.2f;
      writheTask->m_parameters.m_legPeriod = 0.7f;
      writheTask->m_parameters.m_effectorMask = bvmask_Full;
      writheTask->m_parameters.m_backAmplitude = 3.2f;
      writheTask->m_parameters.m_legAmplitude = 1.1f;
      writheTask->m_parameters.m_kneeAmplitude = 1.0f;
      writheTask->m_parameters.m_applyStiffness = true;
      writheTask->m_parameters.m_shoulderLean1 = 0.7f;
      writheTask->m_parameters.m_shoulderLean2 = 0.4f;
      writheTask->m_parameters.m_lean1BlendFactor = 0.0f;
      writheTask->m_parameters.m_lean2BlendFactor = 0.0f;
      //Tantrum parameters
      //writheTask->m_parameters.m_lean1BlendFactor = 0.4f;
      //writheTask->m_parameters.m_lean2BlendFactor = 0.8f;
      writheTask->activate();
    }

    // Update bodyWrithe parameters.
    writheTask->m_parameters.m_blendArms = m_parameters.armsPoseWritheBlend;
    writheTask->m_parameters.m_blendBack = m_parameters.spinePoseWritheBlend;
    writheTask->m_parameters.m_blendLegs = m_parameters.legsPoseWritheBlend;
    writheTask->m_parameters.m_rollOverFlag = m_parameters.rollOverFlag;
    writheTask->m_parameters.m_rollTorqueScale = m_parameters.rollTorqueScale;
    writheTask->m_parameters.m_maxRollOverTime = m_parameters.maxRollOverTime;
    writheTask->m_parameters.m_rollOverRadius = m_parameters.rollOverRadius;

    if (m_orientationState != NmRsCharacter::OS_Up)
    {
      m_character->deactivateTask(bvid_armsWindmill);
      m_character->deactivateTask(bvid_bodyBalance);
      m_character->deactivateTask(bvid_catchFall);
    }

  }

  CBUTaskReturn NmRsCBUOnFire::onTick(float timeStep)
  {
    // Calculate character predicted facing direction.
    rage::Matrix34 predCOMTM;
    m_character->getPredictedCOMOrientation(m_parameters.predictTime, &predCOMTM);
    m_orientationState = m_character->getFacingDirectionFromCOMOrientation(predCOMTM);

    setActivateBehaviours();//m_orientationState must be set before calling this

    // Update bodyWrithe parameters.
    NmRsCBUBodyWrithe* writheTask = (NmRsCBUBodyWrithe*)m_cbuParent->m_tasks[bvid_bodyWrithe];
    Assert(writheTask);

    // The following values get modified dynamically depending on character facing direction.
    writheTask->m_parameters.m_elbowAmplitude = 0.5f;
    writheTask->m_parameters.m_armPeriod = 0.5f;
    writheTask->m_parameters.m_armAmplitude = 2.3f;
    writheTask->m_parameters.m_armStiffness = 13.0f;
    writheTask->m_parameters.m_onFire = true;
    writheTask->m_parameters.m_legPeriod = 0.7f;
    writheTask->m_parameters.m_legAmplitude = 1.1f;
    m_character->setFrictionPreScale(1.0f, bvmask_HandLeft | bvmask_HandRight);

    // Based on character predicted orientation during a roll choose a pose
    // that mostly supports character natural rolling; pose output gets blended with bodyWrithe.
    switch (m_orientationState)
      {
      case NmRsCharacter::OS_Front:
        {
          duringOnFireFront();

          // Set max elbow amplitude for bodyWrithe to increase the chance of rolling by pushing off the ground.
          writheTask->m_parameters.m_elbowAmplitude = 1.0f;
          // Increase arm stiffness for more pushing off the ground.
          writheTask->m_parameters.m_armStiffness = 16.0f;
          writheTask->m_parameters.m_armPeriod = 1.6f;
          writheTask->m_parameters.m_legPeriod = 1.3f;
          writheTask->m_parameters.m_legAmplitude = 1.4f;
          m_character->setFrictionPreScale(0.0f, bvmask_ArmLeft | bvmask_ArmRight);
        }
        break;

      case NmRsCharacter::OS_Back:
        {
          duringOnFireBack();
        }
        break;

      case NmRsCharacter::OS_Left:
        {
          duringOnFireLeft();
        }
        break;

      case NmRsCharacter::OS_Right:
        {
          duringOnFireRight();
        }
        break;

      case NmRsCharacter::OS_Up:
        {
          writheTask->m_parameters.m_elbowAmplitude = 0.0f;
          writheTask->m_parameters.m_armPeriod = 1.0f;
          writheTask->m_parameters.m_armAmplitude = 4.0f;
          writheTask->m_parameters.m_armStiffness = 12.0f;
          writheTask->m_parameters.m_onFire = false;
          writheTask->m_parameters.m_blendArms = m_parameters.armsWindmillWritheBlend;
          writheTask->m_parameters.m_blendBack = m_parameters.spineStumbleWritheBlend;
          writheTask->m_parameters.m_blendLegs = m_parameters.legsStumbleWritheBlend;
          duringOnFireUp(timeStep);
        }
        break;

      case NmRsCharacter::OS_Down:
      default:
        break;

      } // End of switch.

    m_timer += timeStep;

    return eCBUTaskComplete;
  }

  void NmRsCBUOnFire::duringOnFireUp(float timeStep)
  {
    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    Assert(bodyBalanceTask);
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    if (bodyBalanceTask->isActive())
    {
      if (m_timer > m_parameters.staggerTime - 0.3f)
      {
        rage::Matrix34 tmCom;
        tmCom.Set(m_character->m_COMTM);
        //make angvel from COM or Spine2
        float angComp = rage::Clamp(m_character->m_COMrotvel.Dot(tmCom.a), -3.f, 3.f);//side
        float linComp = rage::Clamp(m_character->m_COMvel.Dot(tmCom.c), -3.f, 3.f);//back
        rage::Vector3 hip2head = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
        hip2head.Normalize();
        float posComp = hip2head.Dot(tmCom.c);//back

        float hipPitch = rage::Clamp(-0.7f*posComp - 0.25f*linComp - 0.25f*angComp, -0.7f, 0.5f);
        dynamicBalancerTask->setHipPitch(hipPitch);
        dynamicBalancerTask->autoLeanHipsCancel();
      }
      else
      {
        float inc = m_parameters.staggerLeanRate*timeStep*60.f;
        float minHP = -m_parameters.stumbleMaxLeanForward;
        float maxHP = m_parameters.stumbleMaxLeanBack;
        float hipPitch;
        if ( dynamicBalancerTask->getHipPitch() < m_hipPitchAim)
        {
          hipPitch = rage::Clamp(dynamicBalancerTask->getHipPitch() + inc, minHP, maxHP);
          if (hipPitch > m_hipPitchAim)
            m_hipPitchAim = m_character->getRandom().GetRanged(minHP,maxHP);
        }
        else
        {
          hipPitch = rage::Clamp(dynamicBalancerTask->getHipPitch() - inc, minHP, maxHP);
          if (hipPitch < m_hipPitchAim)
            m_hipPitchAim = m_character->getRandom().GetRanged(minHP,maxHP);
        }
        dynamicBalancerTask->setHipPitch(hipPitch);
      }

      static bool bendLegs = true;
      if (bendLegs)
      {
      //bend legs to match hipPitch
      float straightness = rage::Max(-rage::Abs(dynamicBalancerTask->getHipPitch())*.25f,-0.4f);
      if (dynamicBalancerTask->footState() != NmRsCBUDynBal_FootState::kNotStepping)
        straightness = 0.f;
      dynamicBalancerTask->setLegStraightnessModifier(straightness);

      }
    }
  }
  void NmRsCBUOnFire::duringOnFireLeft()
  {
    // Pose legs.
    const rage::Vector3 lltHipLeft(0.0f, 0.0f, 0.0f);
    const rage::Vector3 lltHipRight(1.0f, 0.0f, 0.0f);
    const float twistKneeLeft = 0.0f;
    const float twistKneeRight = -1.0f;

    getLeftLegInputData()->getHip()->setDesiredAngles(lltHipLeft.x, lltHipLeft.y, lltHipLeft.z);
    getLeftLegInputData()->getKnee()->setDesiredAngle(twistKneeLeft);

    getRightLegInputData()->getHip()->setDesiredAngles(lltHipRight.x, lltHipRight.y, lltHipRight.z);
    getRightLegInputData()->getKnee()->setDesiredAngle(twistKneeRight);

    // Pose spine.
    const rage::Vector3 lltSpine(0.0f, 0.0f, 0.0f);

    getSpineInputData()->setBackAngles(lltSpine.x, lltSpine.y, lltSpine.z);

    // Pose head and neck.
    const rage::Vector3 lltHeadAndNeck(0.0f, 0.0f, 0.0f);

    getSpineInputData()->getLowerNeck()->setDesiredAngles(lltHeadAndNeck.x, lltHeadAndNeck.y, lltHeadAndNeck.z);
    getSpineInputData()->getUpperNeck()->setDesiredAngles(lltHeadAndNeck.x, lltHeadAndNeck.y, lltHeadAndNeck.z);
    const rage::Vector3 lltShoulders(1.5f, 1.0f, 0.5f);
    const rage::Vector3 lltClavicles(0.0f, -1.0f, -1.0f);
    const float elbowAng = 1.5f;
    const bool leftArm = true;
    if (leftArm)
    {
      getLeftArmInputData()->getShoulder()->setDesiredAngles(lltShoulders.x, lltShoulders.y, lltShoulders.z);
      getLeftArmInputData()->getClavicle()->setDesiredAngles(lltClavicles.x, lltClavicles.y, lltClavicles.z);
      getLeftArmInputData()->getElbow()->setDesiredAngle(elbowAng);
      m_character->setFrictionPreScale(0.0f, bvmask_HandLeft);
    }
  }

  void NmRsCBUOnFire::duringOnFireRight()
  {
    // Pose legs.
    const rage::Vector3 lltHipLeft(1.0f, 0.0f, 0.0f);
    const rage::Vector3 lltHipRight(0.0f, 0.0f, 0.0f);
    const float twistKneeLeft = -1.0f;
    const float twistKneeRight = 0.0f;

    getLeftLegInputData()->getHip()->setDesiredAngles(lltHipLeft.x, lltHipLeft.y, lltHipLeft.z);
    getLeftLegInputData()->getKnee()->setDesiredAngle(twistKneeLeft);

    getRightLegInputData()->getHip()->setDesiredAngles(lltHipRight.x, lltHipRight.y, lltHipRight.z);
    getRightLegInputData()->getKnee()->setDesiredAngle(twistKneeRight);

    // Pose spine.
    const rage::Vector3 lltSpine(0.0f, 0.0f, 0.0f);

    getSpineInputData()->setBackAngles(lltSpine.x, lltSpine.y, lltSpine.z);

    // Pose head and neck.
    const rage::Vector3 lltHeadAndNeck(0.0f, 0.0f, 0.0f);

    getSpineInputData()->getLowerNeck()->setDesiredAngles(lltHeadAndNeck.x, lltHeadAndNeck.y, lltHeadAndNeck.z);
    getSpineInputData()->getUpperNeck()->setDesiredAngles(lltHeadAndNeck.x, lltHeadAndNeck.y, lltHeadAndNeck.z);
    const rage::Vector3 lltShoulders(1.5f, 1.0f, 0.5f);
    const rage::Vector3 lltClavicles(0.0f, -1.0f, -1.0f);
    const float elbowAng = 1.5f;
    const bool rightArm = true;
    if (rightArm)
    {
      m_character->setFrictionPreScale(0.0f, bvmask_HandRight);
      getRightArmInputData()->getShoulder()->setDesiredAngles(lltShoulders.x, lltShoulders.y, lltShoulders.z);
      getRightArmInputData()->getClavicle()->setDesiredAngles(lltClavicles.x, lltClavicles.y, lltClavicles.z);
      getRightArmInputData()->getElbow()->setDesiredAngle(elbowAng);
    }

  }

   void NmRsCBUOnFire::duringOnFireBack()
   {
     // Pose legs.
     const rage::Vector3 lltHip(1.0f, 0.0f, 0.0f);
     const float twistKnee = -1.0f;

     getLeftLegInputData()->getHip()->setDesiredAngles(lltHip.x, lltHip.y, lltHip.z);
     getLeftLegInputData()->getKnee()->setDesiredAngle(twistKnee);

     getRightLegInputData()->getHip()->setDesiredAngles(lltHip.x, lltHip.y, lltHip.z);
     getRightLegInputData()->getKnee()->setDesiredAngle(twistKnee);

     // Pose spine.
     const rage::Vector3 lltSpine(2.0f, 0.0f, 0.0f);

     getSpineInputData()->setBackAngles(lltSpine.x, lltSpine.y, lltSpine.z);

     // Pose head and neck.
     const rage::Vector3 lltHeadAndNeck(1.0f, 0.0f, 0.0f);

     getSpineInputData()->getLowerNeck()->setDesiredAngles(lltHeadAndNeck.x, lltHeadAndNeck.y, lltHeadAndNeck.z);
     getSpineInputData()->getUpperNeck()->setDesiredAngles(lltHeadAndNeck.x, lltHeadAndNeck.y, lltHeadAndNeck.z);
   }

   void NmRsCBUOnFire::duringOnFireFront()
   {
     // Pose spine.
     const rage::Vector3 lltSpine(-2.0f, 0.0f, 0.0f);

     getSpineInputData()->setBackAngles(lltSpine.x, lltSpine.y, lltSpine.z);

     // Pose head and neck.
     const rage::Vector3 lltHeadAndNeck(-1.0f, 0.0f, 0.0f);

     getSpineInputData()->getLowerNeck()->setDesiredAngles(lltHeadAndNeck.x, lltHeadAndNeck.y, lltHeadAndNeck.z);
     getSpineInputData()->getUpperNeck()->setDesiredAngles(lltHeadAndNeck.x, lltHeadAndNeck.y, lltHeadAndNeck.z);

     // Pose arms.
     const rage::Vector3 lltShoulders(1.5f, 1.0f, 0.5f);
     const rage::Vector3 lltClavicles(0.0f, -1.0f, -1.0f);
     const float elbowAng = 1.5f;

     getLeftArmInputData()->getShoulder()->setDesiredAngles(lltShoulders.x, lltShoulders.y, lltShoulders.z);
     getLeftArmInputData()->getClavicle()->setDesiredAngles(lltClavicles.x, lltClavicles.y, lltClavicles.z);
     getLeftArmInputData()->getElbow()->setDesiredAngle(elbowAng);

     getRightArmInputData()->getShoulder()->setDesiredAngles(lltShoulders.x, lltShoulders.y, lltShoulders.z);
     getRightArmInputData()->getClavicle()->setDesiredAngles(lltClavicles.x, lltClavicles.y, lltClavicles.z);
     getRightArmInputData()->getElbow()->setDesiredAngle(elbowAng);
   }

#if ART_ENABLE_BSPY
  void NmRsCBUOnFire::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.staggerTime, true);
    bspyTaskVar(m_parameters.staggerLeanRate, true);
    bspyTaskVar(m_parameters.stumbleMaxLeanBack, true);
    bspyTaskVar(m_parameters.stumbleMaxLeanForward, true);
    bspyTaskVar(m_parameters.armsWindmillWritheBlend, true);
    bspyTaskVar(m_parameters.spineStumbleWritheBlend, true);
    bspyTaskVar(m_parameters.legsStumbleWritheBlend, true);
    bspyTaskVar(m_parameters.armsPoseWritheBlend, true);
    bspyTaskVar(m_parameters.spinePoseWritheBlend, true);
    bspyTaskVar(m_parameters.legsPoseWritheBlend, true);
    bspyTaskVar(m_parameters.rollTorqueScale, true);
    bspyTaskVar(m_parameters.predictTime, true);
    bspyTaskVar(m_parameters.rollOverFlag, true);
    bspyTaskVar(m_parameters.maxRollOverTime, true);
    bspyTaskVar(m_parameters.rollOverRadius, true);

    static const char* orientationState_names[] =
    {
#define ORIENTATION_STATE_NAME_ACTION(_name) #_name ,
      ORIENTATION_STATES(ORIENTATION_STATE_NAME_ACTION)
#undef ORIENTATION_STATE_NAME_ACTION
    };
    bspyTaskVar_StringEnum(m_orientationState, orientationState_names, false);

    bspyTaskVar(m_timer, false);
    bspyTaskVar(m_hipPitchAim, false);

  }
#endif // ART_ENABLE_BSPY

} // nms Art
