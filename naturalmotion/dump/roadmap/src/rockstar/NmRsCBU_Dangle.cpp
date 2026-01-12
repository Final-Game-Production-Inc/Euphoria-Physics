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
 * Rolling motion. Apply when the character is to be tumbling, either down a hill,
 * after being blown by an explosion, or hit by a car.
 * The character is in a rough foetal position, he puts his arms out to brace against
 * collisions with the ground, and he will relax after he stops tumbling.
 *
 * Roll Up behaviour. This is a pre-condition type behaviour which has to occur
 * before the character starts rolling or tumbling. Rolling and tumbling are
 * largely dependent on the shape of the character before it hits the ground/terrain
 * and the slope etc. of the ground/terrain. Part of the post-ground impact suite of behaviours.
 *
 * TDL current implementation simple curls the character up in proportion to how fast he's rotating
 * This behaviour is designed to transition in from a fall behaviour as the character approaches the ground
 */


#include "NmRsInclude.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"
#include "NmRsCBU_Dangle.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_Dangle.h"
#include "NmRsCBU_Pedal.h"

namespace ART
{
  NmRsCBUDangle::NmRsCBUDangle(ART::MemoryManager* services) : CBUTaskBase(services, bvid_dangle),
    m_reachType(kNone),
    m_hangType(kRelaxing)
  {
    initialiseCustomVariables();
  }

  NmRsCBUDangle::~NmRsCBUDangle()
  {
  }

  void NmRsCBUDangle::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_DangleTimer = m_TimeToSwitchPedalLegs = m_TimeToSwitchHangingState = 0.0f;
    m_ReachTimerStart = m_ReachTimer = 5.0f;
  }

  void NmRsCBUDangle::onActivate()
  {
    Assert(m_character);

    // decide which hands to grab with
    m_reachType = kNone;

    initialiseCustomVariables();
  }

  void NmRsCBUDangle::onDeactivate()
  {
    Assert(m_character);

    getLeftLeg()->getFoot()->setCollisionEnabled(true);
    getRightLeg()->getFoot()->setCollisionEnabled(true);
    getLeftLeg()->getShin()->setCollisionEnabled(true);
    getRightLeg()->getShin()->setCollisionEnabled(true);

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUDangle::onTick(float timeStep)
  {
    NM_RS_DBG_LOGF(L"during");

    static float clavicle1 = 0.4935f; // 0.4935
    static float clavicle2 = 0.5126f; // 0.5126
    static float clavicle3 = 0.1445f; // 0.1445
    static float shoulder1 = 1.20737910f; // 1.20737910
    static float shoulder2 = 1.2f; // 0.833587527
    static float shoulder3 = 0.920000017f; // 0.920000017

    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtClav_Jnt_Left)))->setLimits(clavicle1, clavicle2, clavicle3);
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtClav_Jnt_Right)))->setLimits(clavicle1, clavicle2, clavicle3);
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtShoulder_Left)))->setLimits(shoulder1, shoulder2, shoulder3);
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtShoulder_Right)))->setLimits(shoulder1, shoulder2, shoulder3);

    getLeftLeg()->getFoot()->setCollisionEnabled(false);
    getRightLeg()->getFoot()->setCollisionEnabled(false);
    getLeftLeg()->getShin()->setCollisionEnabled(false);
    getRightLeg()->getShin()->setCollisionEnabled(false);

    // Update the timer
    m_DangleTimer += timeStep;

    m_body->setStiffness(3.0f, 1.0f);

    GatherStateData();

    Hanging(timeStep);

    ArmControl(timeStep);

    HeadControl();

    return eCBUTaskComplete;
  } 

  void NmRsCBUDangle::ArmControl(float timeStep)
  {
    // Handle arms
    static float clavicleStiffness = 8.0f;
    static float shoulderStiffness = 7.0f;
    static float elbowStiffness = 9.0f; 
    static float wristStiffness = 8.0f; 
    static float clavicle1 = 0.0f; // 0.0
    static float clavicle2 = -0.8f; // -0.8
    static float clavicle3 = -1.3f; // -1.3
    static float shoulder1 = 0.6f; // 0.3
    static float shoulder2 = -0.5f; // -1.5
    static float shoulder3 = -1.0f; // -1.0
    static float elbow1 = 0.5f;    // 0.5

    m_body->setStiffness(clavicleStiffness, clavicleStiffness/10.0f, bvmask_ClavicleLeft | bvmask_ClavicleRight);
    m_body->setStiffness(shoulderStiffness, shoulderStiffness/10.0f, bvmask_UpperArmLeft | bvmask_UpperArmRight);
    m_body->setStiffness(elbowStiffness, elbowStiffness/10.0f, bvmask_ForearmLeft | bvmask_ForearmRight);
    m_body->setStiffness(wristStiffness, wristStiffness/10.0f, bvmask_HandLeft | bvmask_HandRight);

    getLeftArmInputData()->getClavicle()->setDesiredAngles(clavicle1, clavicle2, clavicle3);
    getLeftArmInputData()->getShoulder()->setDesiredAngles(shoulder1, shoulder2, shoulder3);
    getLeftArmInputData()->getElbow()->setDesiredAngle(elbow1);

    getRightArmInputData()->getClavicle()->setDesiredAngles(clavicle1, clavicle2, clavicle3);
    getRightArmInputData()->getShoulder()->setDesiredAngles(shoulder1, shoulder2, shoulder3);
    getRightArmInputData()->getElbow()->setDesiredAngle(elbow1);

    if (!m_parameters.m_doGrab)
    {
      // disable headlook in was currently grabbing
      if (m_reachType != kNone)
      {
        NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
        if (headLookTask->isActive())
          headLookTask->deactivate();
      }
      m_reachType = kNone;
      m_ReachTimer = 0.5f;
      return;
    }

    // Update the timer
    m_ReachTimer -= timeStep;
    if (m_ReachTimer < 0.0f)
    {
      if (m_reachType == kNone)
      {
        m_reachType = kReaching;
        static float min = 6.0f;
        static float max = 12.0f;
        m_ReachTimerStart = m_ReachTimer = m_character->getRandom().GetRanged(min, max);
      }
      else if (m_reachType == kReaching)
      {
        m_reachType = kAborting;
        static float min = 0.5f;
        static float max = 1.0f;
        m_ReachTimer = m_character->getRandom().GetRanged(min, max);
        m_ReachTimerStart = m_ReachTimer;
      }
      else if (m_reachType == kAborting)
      {
        m_reachType = kNone;
        static float smin = 4.0f;
        static float smax = 8.0f;
        float laziness = 1.0f - m_parameters.m_grabFrequency;
        float min = smin + laziness * 10.0f * smin;
        float max = smax + laziness * 10.0f * smax;
        m_ReachTimerStart = m_ReachTimer = m_character->getRandom().GetRanged(min, max);
      }
    }

    if (m_reachType != kNone)
    {
      // Curl the legs
      static float kneeSeverity = -3.0f;
      getLeftLegInputData()->getKnee()->setDesiredAngle(kneeSeverity);
      getRightLegInputData()->getKnee()->setDesiredAngle(kneeSeverity);
      static float hipSeverity = 3.0f;
      getLeftLegInputData()->getHip()->setDesiredLean1(hipSeverity);
      getRightLegInputData()->getHip()->setDesiredLean1(hipSeverity);

      // Curl the spine
      static float spineSeverity = 5.0f;
      getSpineInputData()->getSpine3()->setDesiredLean1(spineSeverity);
      getSpineInputData()->getSpine2()->setDesiredLean1(spineSeverity);
      getSpineInputData()->getSpine1()->setDesiredLean1(spineSeverity);
      getSpineInputData()->getSpine0()->setDesiredLean1(spineSeverity);

      // Reach with arms
      static float front = -0.0f;
      rage::Vector3 reachTarget = getLeftLeg()->getFoot()->getPosition() + m_bodyBack * front;
      static float gravComp = 1.0f;
      getLeftArmInputData()->setOpposeGravity(gravComp);
      getRightLegInputData()->setOpposeGravity(gravComp);

      // set up ik priority to be a little less than overall behaviour priority
      // so we can overwrite the shoulder twist later.
      NmRsLimbInput leftArmIkInput = createNmRsLimbInput<NmRsIKInputWrapper>(-1);
      NmRsIKInputWrapper* leftArmIkInputData = leftArmIkInput.getData<NmRsIKInputWrapper>();
      leftArmIkInputData->setTarget(reachTarget);
      leftArmIkInputData->setTwist(0.0f);

      NmRsLimbInput rightArmIkInput = createNmRsLimbInput<NmRsIKInputWrapper>(-1);
      NmRsIKInputWrapper* rightArmIkInputData = rightArmIkInput.getData<NmRsIKInputWrapper>();
      rightArmIkInputData->setTarget(reachTarget);
      rightArmIkInputData->setTwist(0.0f);

      if (m_reachType == kReaching && m_ReachTimer > 1.5f)
      {
        leftArmIkInputData->setWristTarget(reachTarget);
        leftArmIkInputData->setWristNormal(-m_bodySide);

        rightArmIkInputData->setWristTarget(reachTarget);
        rightArmIkInputData->setWristNormal(m_bodySide);
      }

      getLeftArm()->postInput(leftArmIkInput);
      getRightArm()->postInput(rightArmIkInput);

      getLeftArmInputData()->getShoulder()->setDesiredTwist(0.0f);
      getRightArmInputData()->getShoulder()->setDesiredTwist(0.0f);

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(),"Dangle",reachTarget);
#endif

      // Set the stiffness
      float timeInState = m_ReachTimerStart - m_ReachTimer;
      static float rateIn = 7.0f;
      static float rateOut = 10.0f;
      float rate = m_reachType == kReaching ? rateIn : rateOut;
      static float max = 10.0f;
      float stiffness = rage::Clamp(timeInState * rate, m_reachType == kReaching ? 3.0f : 0.0f, max);
      if (m_reachType == kAborting)
        stiffness = rage::Clamp(max - stiffness, 3.0f, max);

      m_body->setStiffness(stiffness, stiffness/10.0f, bvmask_Trunk | bvmask_LegLeft | bvmask_LegRight);

      // Cheat forces from hand to target
      static float timeToStartHandPull = 2.0f;
      if (m_reachType == kReaching && timeInState > timeToStartHandPull)
      {
        rage::Vector3 toTargetLeft = reachTarget - getLeftArm()->getHand()->getPosition();
        rage::Vector3 toTargetRight = reachTarget - getRightArm()->getHand()->getPosition();
        toTargetLeft.Normalize();
        toTargetRight.Normalize();
        static float cheatMax = 50.0f;
        static float cheatRate = 100.0f;
        float cheatStr = rage::Clamp(timeInState * cheatRate, 0.0f, cheatMax);
        getRightArm()->getHand()->applyForce(cheatStr*toTargetRight);
        getLeftArm()->getHand()->applyForce(cheatStr*toTargetLeft);
      }
    }

#if ART_ENABLE_BSPY
    m_character->setSkeletonVizRoot(11);//parent is leftclavicle 15 for rightClavicle
    m_character->setSkeletonVizMode(NmRsCharacter::kSV_DesiredAngles);
    BehaviourMask mask = bvmask_ArmLeft;//”ur” for right arm
    m_character->setSkeletonVizMask(mask);
    m_character->drawSkeleton(m_character->getSkeletonVizMode() == NmRsCharacter::kSV_DesiredAngles);

    m_character->setSkeletonVizRoot(15);//parent is leftclavicle 15 for rightClavicle
    m_character->setSkeletonVizMode(NmRsCharacter::kSV_DesiredAngles);
    mask = bvmask_ArmRight;//”ur” for right arm
    m_character->setSkeletonVizMask(mask);
    m_character->drawSkeleton(m_character->getSkeletonVizMode() == NmRsCharacter::kSV_DesiredAngles);

    m_character->setSkeletonVizMode(NmRsCharacter::kSV_None);
    m_character->setSkeletonVizMask(bvmask_None);
#endif
  }

  void NmRsCBUDangle::HeadControl()
  {
    static float frontReach = -0.2f;
    static float frontHang = -0.4f;
    float front = m_reachType != kNone ? frontReach : frontHang;
    rage::Vector3 target = getLeftLeg()->getFoot()->getPosition() + m_bodyBack * front;
    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    if (m_reachType != kNone || m_hangType == kStruggling)
    {
      if (!headLookTask->isActive())
      {
        headLookTask->updateBehaviourMessage(NULL); // sets values to defaults
        headLookTask->m_parameters.m_pos = target;
        headLookTask->m_parameters.m_alwaysEyesHorizontal = false;
        headLookTask->m_parameters.twistSpine = false;
        headLookTask->m_parameters.m_alwaysLook = true;
        headLookTask->activate();
      }
      else 
      {
        // If already active, just update the look position
        headLookTask->m_parameters.m_pos = target;
      }
    }
  }

  void NmRsCBUDangle::Hanging(float timeStep)
  {
    // This function assumes not-reaching
    if (m_reachType != kNone)
      return;

    static bool pullingLeft = true;

    // Update the hanging timer
    m_TimeToSwitchHangingState -= timeStep;
    if (m_hangType == kRelaxing)
    {
      // Handle relaxing
      if (m_TimeToSwitchHangingState < 0.0f)
      {
        static float struggleMin = 0.5f;
        static float struggleMax = 4.0f;
        m_TimeToSwitchHangingState = m_character->getRandom().GetRanged(struggleMin, struggleMax);
        m_hangType = kStruggling;
      }

      // Handle spine
      static float spineStiff = 6.0f;
      getSpine()->setBodyStiffness(getSpineInput(), spineStiff, spineStiff/1.0f);
    }
    else
    {
      // Handle struggling
      if (m_TimeToSwitchHangingState < 0.0f)
      {
        static float relaxMin = 1.0f;
        static float relaxMax = 3.0f;
        m_TimeToSwitchHangingState = m_character->getRandom().GetRanged(relaxMin, relaxMax);
        m_hangType = kRelaxing;
      }
      else
      {
        // Handle spine
        static float spine3ang = 0.0f;
        static float spine2ang = 0.0f;
        static float spine1ang = 0.0f;
        static float spine0ang = 0.0f;
        static float spine3Sideang = 0.15f;
        static float spine2Sideang = 0.0f;
        static float spine1Sideang = 0.0f;
        static float spine0Sideang = 0.0f;

        getSpineInputData()->getSpine3()->setDesiredLean1(spine3ang);
        getSpineInputData()->getSpine2()->setDesiredLean1(spine2ang);
        getSpineInputData()->getSpine1()->setDesiredLean1(spine1ang);
        getSpineInputData()->getSpine0()->setDesiredLean1(spine0ang);

        float mult = pullingLeft? -1.0f : 1.0f;
        mult = m_hangType == kStruggling ? 1.0f : 0.0f;

        getSpineInputData()->getSpine3()->setDesiredLean2(mult * spine3Sideang);
        getSpineInputData()->getSpine2()->setDesiredLean2(mult * spine2Sideang);
        getSpineInputData()->getSpine1()->setDesiredLean2(mult * spine1Sideang);
        getSpineInputData()->getSpine0()->setDesiredLean2(mult * spine0Sideang);

        static float spine3stiff = 8.0f;
        static float spine2stiff = 8.0f;
        static float spine1stiff = 8.0f;
        static float spine0stiff = 8.0f;

        getSpineInputData()->getSpine3()->setStiffness(spine3stiff, spine3stiff/10.0f);
        getSpineInputData()->getSpine2()->setStiffness(spine2stiff, spine2stiff/10.0f);
        getSpineInputData()->getSpine1()->setStiffness(spine1stiff, spine1stiff/10.0f);
        getSpineInputData()->getSpine0()->setStiffness(spine0stiff, spine0stiff/10.0f);

        // Handle Legs
        static float hipPullAngle = 2.0f;
        static float hipPushAngle = -2.0f;
        static float kneePullAngle = -2.0f;
        static float kneePushAngle = 0.0f;
        static float stiff = 7.0f;
        static float minSwitchPushLeg = 0.5f;
        static float maxSwitchPushLeg = 1.5f;

        getLeftLegInputData()->getHip()->setStiffness(stiff, stiff/10.0f);
        getLeftLegInputData()->getKnee()->setStiffness(stiff, stiff/10.0f);
        getRightLegInputData()->getHip()->setStiffness(stiff, stiff/10.0f);
        getRightLegInputData()->getKnee()->setStiffness(stiff, stiff/10.0f);

        if (m_TimeToSwitchPedalLegs <= 0.0f)
        {
          pullingLeft = !pullingLeft;
          if (pullingLeft)
          {
            getLeftLegInputData()->getHip()->setDesiredLean1(hipPullAngle);
            getLeftLegInputData()->getKnee()->setDesiredAngle(kneePullAngle);
            getRightLegInputData()->getHip()->setDesiredLean1(hipPushAngle);
            getRightLegInputData()->getKnee()->setDesiredAngle(kneePushAngle);
          }
          else
          {
            getLeftLegInputData()->getHip()->setDesiredLean1(hipPushAngle);
            getLeftLegInputData()->getKnee()->setDesiredAngle(kneePushAngle);
            getRightLegInputData()->getHip()->setDesiredLean1(hipPullAngle);
            getRightLegInputData()->getKnee()->setDesiredAngle(kneePullAngle);
          }
          m_TimeToSwitchPedalLegs = m_character->getRandom().GetRanged(minSwitchPushLeg, maxSwitchPushLeg);
        }
        m_TimeToSwitchPedalLegs -= timeStep;
      }
    }
  }

  void NmRsCBUDangle::GatherStateData()
  {
    // determine character orientation
    rage::Matrix34 tmCom;
    static bool useCOMorient = true;
    if (useCOMorient) 
    {
      tmCom.Set(m_character->m_COMTM);
      m_bodySide = tmCom.a;  // faces right
      m_bodyUp = tmCom.b;
      m_bodyBack = tmCom.c;
    }
    else 
    {
      getSpine()->getSpine1Part()->getBoundMatrix(&tmCom); 
      m_bodyUp = tmCom.a;
      m_bodySide = -tmCom.b;
      m_bodyBack = tmCom.c;
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUDangle::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_doGrab, true);
    bspyTaskVar(m_parameters.m_grabFrequency, true);

    bspyTaskVar(m_DangleTimer, false);
    bspyTaskVar(m_ReachTimer, false);
    bspyTaskVar(m_ReachTimerStart, false);
    bspyTaskVar(m_TimeToSwitchPedalLegs, false);
    bspyTaskVar(m_TimeToSwitchHangingState, false);
    
    bspyTaskVar(m_bodyUp, false);
    bspyTaskVar(m_bodySide, false);
    bspyTaskVar(m_bodyBack, false);

    static const char*  reachTypeStrings[] =
    {
      "kNone",
      "kReaching",
      "kAborting"
    };
    bspyTaskVar_StringEnum(m_reachType, reachTypeStrings, false); 

    static const char*  hangTypeStrings[] =
    {
      "kRelaxing",
      "kStruggling"
    };
    bspyTaskVar_StringEnum(m_hangType, hangTypeStrings, false); 
  }
#endif // ART_ENABLE_BSPY
}
