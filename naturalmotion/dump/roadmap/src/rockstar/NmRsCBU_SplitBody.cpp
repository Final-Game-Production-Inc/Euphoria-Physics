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
*/

#include "NmRsInclude.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "NmRsCBU_SplitBody.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_SoftKeyframe.h"

#define NMRSPLITBODY_DISABLE_LIMITS 0
#define NMRSPLITBODY_ADJUST_LIMITS 0
#define NMRSPLITBODY_PD_SPINE_ORIENTATION 1
#define NMRSPLITBODY_BREATHING 0
#define SPLITBODY_SIDEARM_AVOIDANCE 0
#define SPLITBODY_CONSTRAIN_HANDS 1
#define SPLITBODY_CONTROL_HAND_ORIENTATION 1

namespace ART
{
  NmRsCBUSplitBody::NmRsCBUSplitBody(ART::MemoryManager* services) : CBUTaskBase(services, bvid_splitBody),
    m_primaryArm(0),
    m_supportArm(0),
    m_constraintActive(0)
  {
    initialiseCustomVariables();
  }

  NmRsCBUSplitBody::~NmRsCBUSplitBody()
  {
  }

  void NmRsCBUSplitBody::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_supportHandError = 1.f;
    m_primaryHandError = 1.f;
    m_constraintActive = false;

    int i;
    for(i = 0; i < 2; ++i)
    {
      m_armData[i].relaxTimer = 0;
      m_armData[i].relaxScale = 1;
    }
  }

  void NmRsCBUSplitBody::onActivate()
  {
    Assert(m_character);

    // if m_splitBodyActive is not set, interpret this as a 'start' condition
    readParameters(!isActive());

    // disable collisions according to mask
    // fix me to actually work - perhaps write a generic part 
    for (int i = 0; i < m_character->getNumberOfParts(); i++)
    {
      if(m_character->isPartInMask(m_parameters.collisionMask, i))
      {
        NmRsGenericPart *part = m_character->getGenericPartByIndex(i);
        part->setCollisionEnabled(false);
      }
    }

#if NMRSPLITBODY_DISABLE_LIMITS
    // disable all limits
    if(m_parameters.adjustJointLimits)
    {
      for(int i = 0; i < TotalKnownHumanEffectors; ++i)
      {
        NmRsEffectorBase* effector = m_character->getEffectorDirect(i);
        Assert(effector);
        effector->disableLimits();
      }
    }
#endif

    // locally cache the limb definitions
    m_leftLeg = m_character->getLeftLegSetup();
    m_rightLeg = m_character->getRightLegSetup();
    m_leftArm = m_character->getLeftArmSetup();
    m_rightArm = m_character->getRightArmSetup();
    m_spine = m_character->getSpineSetup();

    // set up various cheat forces
    m_character->enableFootSlipCompensationActive(false);
    m_character->enableZMPPostureControlActive(true);
    m_character->m_posture.useZMP = false;

  }

  void NmRsCBUSplitBody::onDeactivate()
  {
    Assert(m_character);

    // disable hard key framing
    m_character->setIncomingTransformApplyMode(kDisabling);

    // disable soft key framing
    NmRsCBUSoftKeyframe *softKeyTask = (NmRsCBUSoftKeyframe *)m_cbuParent->m_tasks[bvid_softKeyframe];
    if (softKeyTask->isActive())
      softKeyTask->deactivate();

    // disable oppose gravity
    m_body->setOpposeGravity(0.0f);

    // re-enable various cheat forces
    m_character->enableFootSlipCompensationActive(true);
    m_character->enableZMPPostureControlActive(true);

    // re-enable collisions
    for (int i = 0; i < m_character->getNumberOfParts(); i++)
    {
      NmRsGenericPart *part = m_character->getGenericPartByIndex(i);
      part->setCollisionEnabled(true);
    }

#if NM_RUNTIME_LIMITS
    // restore any cached limits we may have altered.
    // warning: this may cause pops and should be 
    // replaced with character-wide system
    m_character->restoreAllLimits();
#endif

    // re-enable ZMP posture control
    m_character->m_posture.useZMP = true;

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUSplitBody::onTick(float timeStep)
  {
    // set stiffness as modulated for recoils
    m_character->setBodyStiffness(m_armData[NmRsCharacter::kLeftHand].relaxScale * m_parameters.stiffness, m_parameters.damping, "ul");
    m_character->setBodyStiffness(m_armData[NmRsCharacter::kRightHand].relaxScale * m_parameters.stiffness, m_parameters.damping, "ur");

    // active pose
    callMaskedEffectorFunctionNoArgs(
      m_character, 
      (m_parameters.activePoseMask | m_parameters.hardConstraintMask), 
      &NmRs1DofEffector::activePose, 
      &NmRs3DofEffector::activePose);

#if NM_RUNTIME_LIMITS
    if(m_parameters.adjustJointLimits)
      m_character->openAllLimitsToDesired();
#endif

#if SPLITBODY_CONSTRAIN_HANDS
    if(m_character->isConstraintActive() && m_character->weaponModeChanged())
      m_character->releaseHands();

    // deal with hand constraint
    if(m_parameters.constrainHands && (m_character->getWeaponMode() == kPistol || m_character->getWeaponMode() == kRifle))
    {

      // compute the desired constraint position
      rage::Vector3 constraintOffset(0.f, 0.f, 0.f);
      rage::Quaternion constraintOrientation;
      m_character->getWeaponSupportOffset(constraintOffset, constraintOrientation);
      m_character->computeConstraintPosition(m_constraintPos, m_primaryArm, m_supportArm, constraintOffset, false);

#if ART_ENABLE_BSPY && 0
      m_character->bspyDrawPoint(m_constraintPos, 0.05f, rage::Vector3(1.f,0.f,1.f));
#endif

      // measure error between primary hand and it's corresponding tm
      int incomingComponentCount = 0;
      NMutils::NMMatrix4 *itPtr = 0;
      ART::IncomingTransformStatus itmStatusFlags = ART::kITSNone;
      m_character->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, ART::kITSourceCurrent);
      if (incomingComponentCount != 0 && itPtr != 0)
      {
        int indexPrimaryHand = m_primaryArm->getHand()->getPartIndex();
        rage::Matrix34 primaryHandITM;
        NMutils::NMMatrix4 &tmPH = itPtr[indexPrimaryHand];
        primaryHandITM.Set(tmPH[0][0], tmPH[0][1], tmPH[0][2], 
          tmPH[1][0], tmPH[1][1], tmPH[1][2],
          tmPH[2][0], tmPH[2][1], tmPH[2][2], 
          tmPH[3][0], tmPH[3][1], tmPH[3][2]);
        rage::Vector3 primaryHandPos = m_primaryArm->getHand()->getPosition();

#if ART_ENABLE_BSPY && 0
        m_character->bspyDrawPoint(primaryHandPos, 0.025f, rage::Vector3(1.f,0.f,0.f));
#endif

        primaryHandPos.Subtract(primaryHandITM.d);
        m_primaryHandError = primaryHandPos.Mag();
      }
      else
      {
        Assert(false);
      }

      // measure error between support and and the constraint target
      rage::Vector3 supportHandPos = m_supportArm->getHand()->getPosition();
      supportHandPos -= m_constraintPos;
      m_supportHandError = supportHandPos.Mag();

      const float errorThreshold = 0.25f;
      if(m_supportHandError < errorThreshold)
      {
        m_character->constrainHands(m_primaryArm, m_supportArm, &m_constraintPos, &m_supportHandError, 0.f, 0.7f);
      }
      else
      {
        if(m_character->isConstraintActive())
          m_character->releaseHands();
      }

#if ART_ENABLE_BSPY
      debugPartPositions();
#endif

#if ART_ENABLE_BSPY
      if(m_character->isConstraintActive())
        m_character->bspyDrawPoint(m_constraintPos, 0.05f, rage::Vector3(0.f,1.f,0.f));
      else
        m_character->bspyDrawPoint(m_constraintPos, 0.05f, rage::Vector3(1.f,1.f,0.f));
#endif

    }
    else
    {
      if(m_character->isConstraintActive())
        m_character->releaseHands();
    }

#endif // SPLITBODY_CONSTRAIN_HANDS

    m_constraintActive = m_character->isConstraintActive();

#if NMRSPLITBODY_PD_SPINE_ORIENTATION
    {
      float relaxScale = 1.f;
      if(m_parameters.recoilRelax)
      {
        relaxScale = rage::Min(m_armData[NmRsCharacter::kLeftHand].relaxScale, m_armData[NmRsCharacter::kRightHand].relaxScale);
      }
      m_character->pdPartOrientationToITM(m_spine->getSpine3Part(), relaxScale*  m_parameters.controllerStiffness, m_parameters.controllerDamping);
    }
#endif

#if NMRSPLITBODY_BREATHING
    if(m_parameters.breathingScale > 0.f)
    {
      // Do some crazy Breathing over the top of the current desired pose.
      float breathDiff = m_parameters.breathingScale*0.025f*(10.f*rage::Sinf(4.f*m_behaviourTime)); // simple period of .75 sec, reducing over 30 seconds. 
      nmrsSetLean1(m_leftArm->getClavicle(),NMutils::clampValue(nmrsGetDesiredLean1(m_leftArm->getClavicle()) - 0.3f*breathDiff,-9.9f,9.9f));
      nmrsSetLean1(m_rightArm->getClavicle(),NMutils::clampValue(nmrsGetDesiredLean1(m_rightArm->getClavicle()) - 0.3f*breathDiff,-9.9f,9.9f));
      nmrsSetLean1(m_spine->getSpine3(),nmrsGetDesiredLean1(m_spine->getSpine3())-0.16f*breathDiff);
      nmrsSetLean1(m_spine->getSpine2(),nmrsGetDesiredLean1(m_spine->getSpine2())-0.12f*breathDiff);
      nmrsSetLean1(m_spine->getSpine1(),nmrsGetDesiredLean1(m_spine->getSpine1())-0.08f*breathDiff);
      nmrsSetLean1(m_spine->getSpine0(),nmrsGetDesiredLean1(m_spine->getSpine0())-0.04f*breathDiff);
    }
#endif

#if SPLITBODY_CONTROL_HAND_ORIENTATION
    if(m_parameters.handStable)
    {
      if(m_character->getWeaponMode() == kDual ||
        m_character->getWeaponMode() == kPistolLeft ||
        // m_character->getWeaponMode() == kPistol || // MP3 does not call out pistol support hand correctly
        m_character->getWeaponMode() == kRifle)
      {
        m_character->pdPartOrientationToITM(m_leftArm->getHand(), m_armData[NmRsCharacter::kLeftHand].relaxScale*m_parameters.handStableStiff, m_parameters.handStableDamp);
        m_character->pdPartPositionToITM(m_leftArm->getHand(), m_armData[NmRsCharacter::kLeftHand].relaxScale*m_parameters.handStableStiff*3.f, m_parameters.handStableDamp);
      }
      if(m_character->getWeaponMode() != kPistolLeft)
      {
        m_character->pdPartOrientationToITM(m_rightArm->getHand(), m_armData[NmRsCharacter::kRightHand].relaxScale*m_parameters.handStableStiff, m_parameters.handStableDamp);
        m_character->pdPartPositionToITM(m_rightArm->getHand(), m_armData[NmRsCharacter::kRightHand].relaxScale*m_parameters.handStableStiff*3.f, m_parameters.handStableDamp);
      }
    }
#endif

#if SPLITBODY_SIDEARM_AVOIDANCE
    // gently keep dangling sidearm from banging against legs
    if(m_character->getWeaponMode() == kSidearm)
    {
      // get sidearm position
      sideArmPos.Zero();
      rage::phInst* pInst = NULL;
      rage::phLevelNew* level = m_character->getLevel();
      int levelIndex = m_character->getIsInLevel(m_character->m_rightHandWeapon.levelIndex);
      if(levelIndex != -1)
      {
        Assert(level);
        pInst = level->GetInstance(levelIndex);
        if(pInst)
          sideArmPos.Set(RCC_VECTOR3(pInst->GetPosition()));
      }
      else
      {
        Assert(false);
      }

      // character vertical axis is gUp
      // careful here.. check that this is not being adjusted for floor velocity
      rage::Matrix34 tm;
      m_spine->getPelvisPart()->getMatrix(tm)
#if ART_ENABLE_BSPY
        m_character->bspyDrawLine(tm.d + m_gUp, tm.d - m_gUp, rage::Vector3(1.f,1.f,0.f));
      m_character->m_gUp
#endif

        // 
    }
#endif

    updateFireWeaponRelax(timeStep);

    m_behaviourTime += timeStep;

    return eCBUTaskComplete;
  }

  void NmRsCBUSplitBody::readParameters(bool start)
  {

    // set up hard key framing
    m_character->setIncomingTransformMask(m_parameters.hardConstraintMask);
    if(start)
      m_character->setIncomingTransformApplyMode(kEnabling);

    // set up soft key framing
    NmRsCBUSoftKeyframe *softKeyTask = (NmRsCBUSoftKeyframe *)m_cbuParent->m_tasks[bvid_softKeyframe];
    bool maskIsNotEmpty = m_parameters.softConstraintMask != 0;
    if(start)
      softKeyTask->updateBehaviourMessage(NULL);
    softKeyTask->m_parameters.followMultiplier = 0.5f;
    softKeyTask->m_parameters.goSlowerWhenWeaker = false;
    softKeyTask->m_parameters.mask = m_parameters.softConstraintMask;
    softKeyTask->m_parameters.maxAcceleration = 200.f;
    softKeyTask->m_parameters.targetPosition = rage::Vector3(0,0,0);
    softKeyTask->m_parameters.yawOffset = 0.f;
    if(start && maskIsNotEmpty)
      softKeyTask->activate();

    // set up effectors
    if(start)
    {
      m_character->recalibrateAllEffectors();
      m_character->setBodyStiffness(m_parameters.stiffness, m_parameters.damping, m_parameters.activePoseMask);
    }

    // configure oppose gravity
    m_character->pushMaskCode("fb");
    m_character->processEffectorMaskCode("fb");
    m_body->setOpposeGravity(0.0f);
    m_body->setOpposeGravity(1.0f, m_parameters.gravityCompensationMask);

    // disable ZMP, as it passes bad data to gravity opposition
    // when used with hard-keyed parts
    if(start && m_parameters.gravityCompensationMask != 0)
      m_character->m_posture.useZMP = false;

#if SPLITBODY_CONSTRAIN_HANDS
    // configure limb setup for the purposes of hand constraint
    if(m_parameters.leftHandStatus == 1)
    {
      m_primaryArm = (ArmSetup*)m_character->getLimbIKSetup(LimbIKSetup::kLeftArm);
      m_supportArm = (ArmSetup*)m_character->getLimbIKSetup(LimbIKSetup::kRightArm);
    }
    else
    {
      m_primaryArm = (ArmSetup*)m_character->getLimbIKSetup(LimbIKSetup::kRightArm);
      m_supportArm = (ArmSetup*)m_character->getLimbIKSetup(LimbIKSetup::kLeftArm);
    }
#endif

  }

  void NmRsCBUSplitBody::fireWeapon(int hand /* = NmRsCharacter::kRightHand */)
  {
    m_armData[hand].relaxTimer = m_parameters.recoilRelaxTime;      
    m_armData[hand].relaxScale = 1 - m_parameters.recoilRelaxAmount;

    // set stiffness here as well to get faster response than waiting for the tick
    m_character->setBodyStiffness(m_armData[NmRsCharacter::kLeftHand].relaxScale * m_parameters.stiffness, m_parameters.damping, "ul");
    m_character->setBodyStiffness(m_armData[NmRsCharacter::kRightHand].relaxScale * m_parameters.stiffness, m_parameters.damping, "ur");
  }

  void NmRsCBUSplitBody::updateFireWeaponRelax(float timeStep)
  {
    int i;
    float relaxTime = m_parameters.recoilRelaxTime;
    float relaxAmount = m_parameters.recoilRelaxAmount;
    float amount;
    for(i = 0; i < 2; ++i)
    {
      amount = 1 - relaxAmount;
      m_armData[i].relaxScale = amount + (1 - m_armData[i].relaxTimer/relaxTime) * (1 - amount);
      m_armData[i].relaxScale = amount + (1 - m_armData[i].relaxTimer/relaxTime) * (1 - amount);
      m_armData[i].relaxTimer -= timeStep;
      m_armData[i].relaxTimer = rage::Clamp(m_armData[i].relaxTimer, 0.f, 5.f);
    }
  }

#if ART_ENABLE_BSPY

  void NmRsCBUSplitBody::debugPartPositions()
  {
    // debug incoming transforms vs actual part positions
    int incomingComponentCount = 0;
    NMutils::NMMatrix4 *itPtr = 0;
    ART::IncomingTransformStatus itmStatusFlags = ART::kITSNone;
    m_character->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, ART::kITSourceCurrent);
    if (incomingComponentCount != 0 && itPtr != 0)
    {
      for(int index = 0; index < incomingComponentCount; ++index)
      {
        NmRsGenericPart* part = m_character->getGenericPartByIndex(index);
        if(part && m_character->isPartInMask(m_parameters.hardConstraintMask, part->getPartIndex()))
        {
          rage::Matrix34 partTM;
          NMutils::NMMatrix4 &tmPH = itPtr[index];
          partTM.Set(tmPH[0][0], tmPH[0][1], tmPH[0][2],
            tmPH[1][0], tmPH[1][1], tmPH[1][2],
            tmPH[2][0], tmPH[2][1], tmPH[2][2],
            tmPH[3][0], tmPH[3][1], tmPH[3][2]);
          // draw the itm position in yellow
          m_character->bspyDrawPoint(partTM.d, 0.01f, rage::Vector3(1.f,1.f,0.f));
          part->getMatrix(partTM);
          // draw the actual part position in orange
          m_character->bspyDrawPoint(partTM.d, 0.01f, rage::Vector3(1.f,0.5f,0.f));
        }
      }
    }
  }

  void NmRsCBUSplitBody::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar_Bitfield32(m_parameters.hardConstraintMask, true);
    bspyTaskVar_Bitfield32(m_parameters.softConstraintMask, true);
    bspyTaskVar_Bitfield32(m_parameters.activePoseMask, true);
    bspyTaskVar_Bitfield32(m_parameters.gravityCompensationMask, true);
    bspyTaskVar_Bitfield32(m_parameters.collisionMask, true);
    bspyTaskVar(m_parameters.stiffness, true);
    bspyTaskVar(m_parameters.damping, true);
    bspyTaskVar(m_parameters.leftHandWeaponIndex, true);
    bspyTaskVar(m_parameters.rightHandWeaponIndex, true);
    bspyTaskVar(m_parameters.constrainHands, true);
    bspyTaskVar(m_parameters.adjustJointLimits, true);
    bspyTaskVar(m_parameters.leftHandStatus, true);
    bspyTaskVar(m_parameters.rightHandStatus, true);
    bspyTaskVar(m_parameters.controlHandOrientation, true);
    bspyTaskVar(m_parameters.controllerStiffness, true);
    bspyTaskVar(m_parameters.controllerDamping, true);
    bspyTaskVar(m_parameters.handStable, true);
    bspyTaskVar(m_parameters.handStableStiff, true);
    bspyTaskVar(m_parameters.handStableDamp, true);
    bspyTaskVar(m_parameters.recoilRelax, true);
    bspyTaskVar(m_parameters.recoilRelaxAmount, true);
    bspyTaskVar(m_parameters.recoilRelaxTime, true);

    bspyTaskVar(m_primaryHandError, false);
    bspyTaskVar(m_supportHandError, false);
    bspyTaskVar(m_constraintActive, false);
    bspyTaskVar(m_character->m_constraintDistance, false);

    bspyTaskVar(m_character->weaponModeChanged(), false);
  }
#endif // ART_ENABLE_BSPY

}

