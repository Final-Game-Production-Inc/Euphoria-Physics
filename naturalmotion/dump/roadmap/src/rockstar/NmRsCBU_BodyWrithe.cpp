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
#include "NmRsCBU_BodyWrithe.h"
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

namespace ART
{
  NmRsCBUBodyWrithe::NmRsCBUBodyWrithe(ART::MemoryManager* services) : CBUTaskBase(services, bvid_bodyWrithe)
  {
    initialiseCustomVariables();
  }

  NmRsCBUBodyWrithe::~NmRsCBUBodyWrithe()
  {
  }

  void NmRsCBUBodyWrithe::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_writheTimer     = 0.0f;
    m_subTimer        = 0.0f;
    m_rollOverPeriod  = 1.5f;
    m_rollOverDirection = 1.f;
    m_rollOverTimer   = 1.5f;

    if (m_character)
    {
      m_noiseSeed     = m_character->getRandom().GetRanged(0.0f, 4000.0f);
    }
    else
    {
      m_noiseSeed     = 123.0f;
    }
  }

  void NmRsCBUBodyWrithe::onActivate()
  {
    Assert(m_character);

    m_writheTimer   = 0.0f;
    m_subTimer      = m_character->getRandom().GetRanged(0.0f, 4000.0f);

    if (m_parameters.m_applyStiffness)
    {
      m_body->resetEffectors(kResetCalibrations, m_parameters.m_effectorMask);

      getSpine()->callMaskedEffectorFunctionFloatArg(
        getSpineInput(),
        m_parameters.m_effectorMask,
        0.75f,
        &NmRs1DofEffectorInputWrapper::setMuscleStiffness,
        &NmRs3DofEffectorInputWrapper::setMuscleStiffness);

      m_body->setStiffness(m_parameters.m_armStiffness, m_parameters.m_armDamping, m_parameters.m_effectorMask & (bvmask_ArmLeft | bvmask_ArmRight), NULL, true);
      m_body->setStiffness(m_parameters.m_legStiffness, m_parameters.m_legDamping, m_parameters.m_effectorMask & (bvmask_LegLeft | bvmask_LegRight), NULL, true);
      m_body->setStiffness(m_parameters.m_backStiffness, m_parameters.m_backDamping, m_parameters.m_effectorMask & bvmask_Spine, NULL, true);
    }

  }

  void NmRsCBUBodyWrithe::onDeactivate()
  {
    Assert(m_character);

    m_writheTimer = 0.0f;
    m_subTimer = 0.0f;
    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUBodyWrithe::onTick(float timeStep)
  {
    m_mask = m_parameters.m_effectorMask;

    if (m_parameters.m_applyStiffness)
    {
      m_body->setStiffness(m_parameters.m_armStiffness, m_parameters.m_armDamping, m_parameters.m_effectorMask & (bvmask_ArmLeft | bvmask_ArmRight), NULL, true);
      m_body->setStiffness(m_parameters.m_legStiffness, m_parameters.m_legDamping, m_parameters.m_effectorMask & (bvmask_LegLeft | bvmask_LegRight), NULL, true);
      m_body->setStiffness(m_parameters.m_backStiffness, m_parameters.m_backDamping, m_parameters.m_effectorMask & bvmask_Spine, NULL, true);
    }

    writeBackToEffectors();

    // make the main timer value non-linear
    m_subTimer += timeStep * 0.7f;
    float variableTS = (0.75f + (m_character->getEngine()->perlin3(m_subTimer, m_noiseSeed, 16.0f))) * timeStep;
    m_writheTimer = m_writheTimer + variableTS;

    return eCBUTaskComplete;
  }

  void NmRsCBUBodyWrithe::writeBackToEffectors()
  {
    float characterID = (float)m_character->getID();
    float kneeL, kneeR;
    float legL, legR;

    // generate a bunch of different noise values to desynchronise the sin/cos waves
    float plnMult = 2.0f * PI;
    float distr = m_writheTimer * 0.3f;

    float plnA = -PI + (m_character->getEngine()->perlin3(distr, m_noiseSeed, characterID) * plnMult);
    float plnB = -PI + (m_character->getEngine()->perlin3(distr, distr, characterID) * plnMult);
    float plnC = -PI + (m_character->getEngine()->perlin3(distr, m_noiseSeed, distr) * plnMult);
    float plnD = -PI + (m_character->getEngine()->perlin3(distr, distr, m_noiseSeed) * plnMult);
    float plnE = -PI + (m_character->getEngine()->perlin3(distr, characterID, distr) * plnMult);

    float spinTime  = 2.0f*PI*m_writheTimer/m_parameters.m_armPeriod;
    float armAmp = m_parameters.m_armAmplitude * (m_character->getEngine()->perlin3(m_noiseSeed, spinTime, spinTime));

    // the arms
    float leanone   = 0.8f*armAmp*rage::Sinf(spinTime + plnA);
    float leantwo   = 2.0f*armAmp*rage::Cosf(spinTime + plnB);
    float twist     = -plnC;
    float elbow     = m_parameters.m_elbowAmplitude*armAmp*rage::Sinf(spinTime + plnE);

    leanone = rage::Clamp(leanone, -1.0f, 1.0f);
    leantwo = rage::Clamp(leantwo, -1.0f, 1.0f);
    twist = rage::Clamp(twist, -1.0f, 1.0f);
    elbow = rage::Clamp(elbow, -1.0f, 1.0f);

    // get an independent arm input because we want to blend these values in-limb.
    NmRsLimbInput leftArmInput = createNmRsLimbInput<NmRsArmInputWrapper>(0, m_parameters.m_blendArms);
    NmRsArmInputWrapper* leftArmInputData = leftArmInput.getData<NmRsArmInputWrapper>();

    leftArmInputData->getClavicle()->setDesiredAngles(
      getLeftArm()->getClavicle()->getDesiredLean1FromRelative(leanone * 0.25f),
      getLeftArm()->getClavicle()->getDesiredLean2FromRelative(leantwo * 0.25f),
      getLeftArm()->getClavicle()->getDesiredTwistFromRelative(twist * 0.25f));
    getLeftArm()->blendToZeroPose(leftArmInput, 0.5f);
    leftArmInputData->getShoulder()->setDesiredAngles(
      getLeftArm()->getShoulder()->getDesiredLean1FromRelative(leanone),
      getLeftArm()->getShoulder()->getDesiredLean2FromRelative(leantwo),
      getLeftArm()->getShoulder()->getDesiredTwistFromRelative(twist));
    leftArmInputData->getElbow()->setDesiredAngle(getLeftArm()->getElbow()->getDesiredAngleFromRelative(elbow));

    getLeftArm()->postInput(leftArmInput);

    leanone         = 0.8f*armAmp*rage::Sinf(spinTime + plnC);
    leantwo         = 2.0f*armAmp*rage::Cosf(spinTime + plnD);
    twist           = -plnE;
    elbow           = m_parameters.m_elbowAmplitude*armAmp*rage::Cosf(spinTime + plnD);

    leanone = rage::Clamp(leanone, -1.0f, 1.0f);
    leantwo = rage::Clamp(leantwo, -1.0f, 1.0f);
    twist = rage::Clamp(twist, -1.0f, 1.0f);
    elbow = rage::Clamp(elbow, -1.0f, 1.0f);

    // get an independent arm input because we want to blend these values in-limb.
    NmRsLimbInput rightArmInput = createNmRsLimbInput<NmRsArmInputWrapper>(0, m_parameters.m_blendArms);
    NmRsArmInputWrapper* rightArmInputData = rightArmInput.getData<NmRsArmInputWrapper>();

    rightArmInputData->getClavicle()->setDesiredAngles(
      getRightArm()->getClavicle()->getDesiredLean1FromRelative(leanone * 0.25f),
      getRightArm()->getClavicle()->getDesiredLean2FromRelative(leantwo * 0.25f),
      getRightArm()->getClavicle()->getDesiredTwistFromRelative(twist * 0.25f));
    getRightArm()->blendToZeroPose(rightArmInput, 0.5f);
    rightArmInputData->getShoulder()->setDesiredAngles(
      getRightArm()->getShoulder()->getDesiredLean1FromRelative(leanone),
      getRightArm()->getShoulder()->getDesiredLean2FromRelative(leantwo),
      getRightArm()->getShoulder()->getDesiredTwistFromRelative(twist));
    rightArmInputData->getElbow()->setDesiredAngle(getRightArm()->getElbow()->getDesiredAngleFromRelative(elbow));

    getRightArm()->postInput(rightArmInput);

    // the head
    leanone = rage::Sinf(spinTime + plnA);
    leantwo = rage::Cosf(spinTime + plnD);
    twist   = leanone;

    //the back
    spinTime  = 2.0f*PI*m_writheTimer/m_parameters.m_backPeriod;

    // get an independent arm input because we want to blend these values in-limb.
    NmRsLimbInput spineInput = createNmRsLimbInput<NmRsSpineInputWrapper>(0, m_parameters.m_blendArms);
    NmRsSpineInputWrapper* spineInputData = spineInput.getData<NmRsSpineInputWrapper>();

    spineInputData->getLowerNeck()->setDesiredAngles(
      getSpine()->getLowerNeck()->getDesiredLean1FromRelative(-leanone),
      getSpine()->getLowerNeck()->getDesiredLean2FromRelative(-leantwo),
      getSpine()->getLowerNeck()->getDesiredTwistFromRelative(-twist));
    spineInputData->getUpperNeck()->setDesiredAngles(
      getSpine()->getUpperNeck()->getDesiredLean1FromRelative(leanone),
      getSpine()->getUpperNeck()->getDesiredLean2FromRelative(leantwo),
      getSpine()->getUpperNeck()->getDesiredTwistFromRelative(twist));
    spineInputData->setBackAngles(0.25f*m_parameters.m_backAmplitude*rage::Sinf((spinTime + plnD) *0.8f), 0.15f * plnC, 0.45f*0.9f*m_parameters.m_backAmplitude*rage::Sinf(spinTime + plnA));

    getSpine()->postInput(spineInput);

    // legs
    spinTime  = 1.2f*2.0f*PI*m_writheTimer/m_parameters.m_legPeriod;
    legL = m_parameters.m_legAmplitude*rage::Sinf(2.f + spinTime + plnE);
    legR = m_parameters.m_legAmplitude*rage::Cosf(spinTime + plnD);
    kneeL = m_parameters.m_kneeAmplitude*rage::Cosf(spinTime + plnB);
    kneeR = m_parameters.m_kneeAmplitude*rage::Sinf(spinTime + plnC);

    // get independent inputs because we want to blend these values in-limb.
    NmRsLimbInput leftLegInput = createNmRsLimbInput<NmRsLegInputWrapper>(0, m_parameters.m_blendLegs);
    NmRsLegInputWrapper* leftLegInputData = leftLegInput.getData<NmRsLegInputWrapper>();

    leftLegInputData->getHip()->setDesiredAngles(0.5f*legL, -0.1f-0.2f*m_parameters.m_legAmplitude*rage::Sinf(0.6f*spinTime),0.3f*m_parameters.m_legAmplitude*rage::Sinf(1.f + 0.72f*spinTime));
    leftLegInputData->getKnee()->setDesiredAngle(-0.5f+0.9f*kneeL);

    getLeftLeg()->postInput(leftLegInput);

    NmRsLimbInput rightLegInput = createNmRsLimbInput<NmRsLegInputWrapper>(0, m_parameters.m_blendLegs);
    NmRsLegInputWrapper* rightLegInputData = rightLegInput.getData<NmRsLegInputWrapper>();

    rightLegInputData->getHip()->setDesiredAngles(-0.5f*legR,-0.1f-0.2f*m_parameters.m_legAmplitude*rage::Cosf(0.6f*spinTime),0.3f*m_parameters.m_legAmplitude*rage::Sinf(-1.f + 0.52f*spinTime));
    rightLegInputData->getKnee()->setDesiredAngle(-0.5f-0.9f*kneeR);

    getRightLeg()->postInput(rightLegInput);

    if (m_parameters.m_rollOverFlag)
    {
      // at the feet on the ground
      bool bodyOnGround = m_character->hasCollidedWithWorld(bvmask_LowSpine);
      rage::Vector3 torqueVector;
      rage::Vector3 spine0Pos;
      rage::Vector3 spine3Pos;
      spine0Pos = getSpine()->getSpine0()->getJointPosition();
      spine3Pos = getSpine()->getSpine3()->getJointPosition();
      torqueVector = spine3Pos - spine0Pos;
      torqueVector.Normalize(torqueVector);
      float rotVelMagCom = m_character->m_COMrotvel.Dot(torqueVector);
      NM_RS_DBG_LOGF(L"Writhe rotVelMagHip 0: %f", rotVelMagCom);
      float rotVelMagHip = getSpine()->getPelvisPart()->getAngularVelocity().Dot(torqueVector);
      NM_RS_DBG_LOGF(L"Writhe rotVelMagHip 0: %f", rotVelMagHip);
      float rotVelMag = getSpine()->getSpine3Part()->getAngularVelocity().Dot(torqueVector);
      m_rollOverDirection = rage::Sign(rotVelMag);
      NM_RS_DBG_LOGF(L"Writhe rotVelMag 3: %f", rotVelMag);
      rotVelMag = rage::Abs(rotVelMag);
      rotVelMagHip = rage::Abs(rotVelMagHip);
      rotVelMagCom = rage::Abs(rotVelMagCom);
      // apply some cheat forces
      if (bodyOnGround && rotVelMag < 10.f && rotVelMagHip < 10.f && rotVelMagCom < 10.f)
      {
        float rollOverPhase = 0.5f*(rage::Sinf(m_writheTimer*m_rollOverPeriod)+1.0f);
        rollOverPhase = rage::Clamp((rollOverPhase-0.7f), 0.0f, 1.0f);
        torqueVector.Scale(torqueVector,m_rollOverDirection*rollOverPhase*30.f*5.0f); // maybe need to vary the direction it tries to roll
        NM_RS_DBG_LOGF(L"Writhe rollover : %f", m_rollOverDirection*rollOverPhase*30.f*5.0f);
        NM_RS_DBG_LOGF(L"Writhe m_rollOverDirection : %f", m_rollOverDirection);
        NM_RS_DBG_LOGF(L"Writhe rollOverPhase : %f", rollOverPhase);

        // todo should we implement some kind of masking on parts as well?
        getSpine()->getSpine3Part()->applyTorque(torqueVector);
        getSpine()->getSpine2Part()->applyTorque(torqueVector);
        getSpine()->getSpine1Part()->applyTorque(torqueVector);
        getSpine()->getSpine0Part()->applyTorque(torqueVector);

        // reduce arm stiffness
        //mmmmWon't rollover without reducing stiffnesses if (m_parameters.m_applyStiffness)
        {
          if (m_character->hasCollidedWithWorld(bvmask_ArmLeft))
            getLeftArmInputData()->setStiffness(7.f,m_parameters.m_armDamping);
          if (m_character->hasCollidedWithWorld(bvmask_ArmRight)) 
            getRightArmInputData()->setStiffness(7.f,m_parameters.m_armDamping);
          if (m_character->hasCollidedWithWorld(bvmask_LegLeft)) 
            getLeftLegInputData()->setStiffness(7.f,m_parameters.m_armDamping);
          if (m_character->hasCollidedWithWorld(bvmask_LegRight)) 
            getRightLegInputData()->setStiffness(7.f,m_parameters.m_armDamping);      
        }
      }
      if ( m_rollOverTimer<0.f)
      {
        m_rollOverDirection = -m_rollOverDirection;//(float)(2*m_character->getRandom().GetRanged(0,1)-1);
        m_rollOverTimer   = m_character->getRandom().GetRanged(0.5f,3.f);
      }
      else
        m_rollOverTimer-=m_character->getLastKnownUpdateStep();
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUBodyWrithe::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar_Bitfield32(m_parameters.m_effectorMask, true);
    bspyTaskVar(m_writheTimer, false);
    bspyTaskVar(m_subTimer, false);
    bspyTaskVar(m_noiseSeed, false);

    bspyTaskVar(m_parameters.m_armStiffness, true);
    bspyTaskVar(m_parameters.m_backStiffness, true);
    bspyTaskVar(m_parameters.m_legStiffness, true);

    bspyTaskVar(m_parameters.m_armDamping, true);
    bspyTaskVar(m_parameters.m_backDamping, true);
    bspyTaskVar(m_parameters.m_legDamping, true);

    bspyTaskVar(m_parameters.m_armPeriod, true);
    bspyTaskVar(m_parameters.m_backPeriod, true);
    bspyTaskVar(m_parameters.m_legPeriod, true);

    bspyTaskVar(m_parameters.m_armAmplitude, true);
    bspyTaskVar(m_parameters.m_backAmplitude, true);
    bspyTaskVar(m_parameters.m_legAmplitude, true);

    bspyTaskVar(m_parameters.m_elbowAmplitude, true);
    bspyTaskVar(m_parameters.m_kneeAmplitude, true);

    bspyTaskVar(m_parameters.m_blendArms, true);
    bspyTaskVar(m_parameters.m_blendLegs, true);
    bspyTaskVar(m_parameters.m_blendBack, true);
    bspyTaskVar(m_parameters.m_applyStiffness, true);

    bspyTaskVar(m_rollOverPeriod, true);
    bspyTaskVar(m_rollOverDirection, false);
    bspyTaskVar(m_rollOverTimer, false);
    bspyTaskVar(m_parameters.m_rollOverFlag, true);
  }
#endif // ART_ENABLE_BSPY
}
