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
#include "NmRsCBU_RollUp.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_BraceForImpact.h"
#include "NmRsCBU_RollDownStairs.h"

namespace ART
{
  NmRsCBURollUp::NmRsCBURollUp(ART::MemoryManager* services) : CBUTaskBase(services, bvid_bodyRollUp)
  {
    initialiseCustomVariables();
  }

  NmRsCBURollUp::~NmRsCBURollUp()
  {
  }

  void NmRsCBURollUp::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_rollUpTimer       = 0.0f;
    m_fromShot          = false;
    m_armL              = 0.6f; //set in RollDownStairs (0.4)
    m_pedalSpeed = 0.f;
    m_phase = 0.f;
  }

  void NmRsCBURollUp::onActivate()
  {
    Assert(m_character);

    m_mask = m_parameters.m_effectorMask;

    m_rollUpTimer = 0;

    // Parameters.
    m_rotCOMSmoothed = 0.0f;
    m_damping = 0.5f;
    m_timeNotRolling = 0.f;

    // limbs todo not sure how to deal with this logic. need to add scale(whatever) method?
    // it is also a strange thing to be doing... setting 75% of the current (unknown!)
    // muscle stiffness.
    m_body->resetEffectors(kResetCalibrations);
#if 0
    for (int i = 0; i<m_character->getNumberOfEffectors(); i++)
    {
      NmRsEffectorBase *effector = m_character->getEffector(i);
      effector->setMuscleStiffness(0.75f*effector->getMuscleStiffness());
    }
#endif

    // mr math says "10 for stability!"
    getLeftLegInputData()->getAnkle()->setMuscleStiffness(10.0f);
    getRightLegInputData()->getAnkle()->setMuscleStiffness(10.0f);
    getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_stiffness, m_damping);
    m_body->setStiffness(m_parameters.m_stiffness, m_damping, bvmask_LegLeft | bvmask_LegRight);
    m_body->setStiffness(m_parameters.m_stiffness+2.f, 1.f, bvmask_ArmLeft | bvmask_ArmRight);
    getLeftArmInputData()->getClavicle()->setStiffness(m_parameters.m_stiffness+2.f,1.f);
    getRightArmInputData()->getClavicle()->setStiffness(m_parameters.m_stiffness+2.f,1.f);

    m_blendL = 0.f;
    m_blendR = 0.f;

    float randNo = m_character->getRandom().GetRanged(-1.0f, 1.0f);
    m_asymm = m_parameters.m_asymmetricalLegs*(rage::Abs(randNo)/randNo - 0.3f*randNo); 
    m_pedalSpeed = m_character->getRandom().GetRanged(1.f*6.2f, 3.f*6.2f);
    m_phase = m_character->getRandom().GetRanged(0.f, 6.2f);

    if (m_character->getBodyIdentifier() == gtaFred || m_character->getBodyIdentifier() == gtaFredLarge)
      m_extraSpread = m_character->getRandom().GetRanged(-1.f, 0.f);
    else
      m_extraSpread = 0.f; 
  }

  void NmRsCBURollUp::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();
    m_character->m_applyMinMaxFriction = false; 

    m_character->m_applyMinMaxFriction = false; 

  }

  CBUTaskReturn NmRsCBURollUp::onTick(float timeStep)
  {
    m_mask = m_parameters.m_effectorMask;

    m_rollUpTimer += timeStep;

    m_character->m_applyMinMaxFriction = true; 

    m_character->m_applyMinMaxFriction = true; 

    float comVelMag;
    NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
    Assert(rdsTask);
    if (rdsTask->m_parameters.m_useRelativeVelocity && rdsTask->isActive())
      comVelMag = m_character->m_COMvelRelativeMag;
    else
      comVelMag = m_character->m_COMvelMag;

    float amountOfRoll = m_character->m_COMrotvelMag;
    amountOfRoll += comVelMag * m_parameters.m_rollVelLinearContribution; // 1.0

    // if we want elbow motion (and a bit of leg motion) for roll slowdown
    rage::Vector3 bodyBack = m_character->m_COMTM.c;

    rage::Vector3 rotVel = m_character->m_COMrotvel;
    float roll = rage::Clamp(rotVel.Dot(m_character->m_COMTM.b)/8.f, -1.f, 1.f);      

    float upwardsFacing = bodyBack.Dot(m_character->m_gUp);

    float useArmsToSlowDown = rage::Abs(m_parameters.m_useArmToSlowDown);
    upwardsFacing = rage::Clamp(useArmsToSlowDown-upwardsFacing-1.f, 0.f, 10.f);

    rage::Vector3 spine3Vel;
    spine3Vel = getSpine()->getSpine3Part()->getLinearVelocity();
    if (rdsTask->m_parameters.m_useRelativeVelocity && rdsTask->isActive())
      spine3Vel -= m_character->getFloorVelocity();


    float forwardFall = -spine3Vel.Dot(bodyBack);
    float predictTime = 0.1f;
    rotVel *= predictTime;

    rage::Quaternion rot;
    rot.FromRotation(rotVel);
    rot.Transform(bodyBack,rotVel);

    float legPush = -0.5f*m_character->m_gUp.Dot(rotVel)*m_parameters.m_legPush;
    NM_RS_DBG_LOGF(L"legPush: %.4f", legPush);

    float lLean = 1.f;
    float rLean = 1.f;
    if (m_parameters.m_useArmToSlowDown*roll > 0)
      lLean = 1.f - rage::Abs(roll)*upwardsFacing;
    else
      rLean = 1.f -rage::Abs(roll)*upwardsFacing;

    amountOfRoll = amountOfRoll/2.f;
    float x = rage::Clamp(amountOfRoll, 0.f, 1.f);
    m_rotCOMSmoothed = (m_rotCOMSmoothed*2.f+x)/3.f;
    x = rage::Clamp(m_rotCOMSmoothed, 0.f, 1.f);
    Assert(x==x);

    NM_RS_DBG_LOGF(L"rotCOMSmoothed =  %.4f", m_rotCOMSmoothed);
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "RollUp", amountOfRoll);
#endif

    // success feedback when roll velocity below threshold for certain time
    if (amountOfRoll < m_parameters.m_rollVelForSuccess) //0.2
    {
      m_timeNotRolling += timeStep;
      if (m_timeNotRolling > m_parameters.m_noRollTimeBeforeSuccess) //0.5
      {
        ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
        if (feedback)
        {
          feedback->m_agentID = m_character->getID();
          feedback->m_argsCount = 0;
          strcpy(feedback->m_behaviourName, NMRollUpFeedbackName);
          feedback->onBehaviourSuccess();
        }
      }
    }

    float linVelClampedScalled = 1.f;

    NmRsCBUBraceForImpact* bfiTask = (NmRsCBUBraceForImpact*)m_cbuParent->m_tasks[bvid_braceForImpact];
    Assert(bfiTask);

    if (bfiTask->isActive())
      linVelClampedScalled = ((rage::Clamp(comVelMag,.2f,2.2f)-.2f)/2.0f);


    NM_RS_DBG_LOGF(L" SO THE SCALE FACOR IS + %.5f",linVelClampedScalled);

    float xNew = x*linVelClampedScalled;

    // interpolate between foetal and relaxed based on roll speed
    float localStiff = rage::Clamp(m_parameters.m_stiffness - 5.f*(1.f-xNew),1.0f,16.0f);

    m_body->setStiffness(localStiff, m_damping);

    getLeftLegInputData()->getAnkle()->setStiffness(localStiff, m_damping+1.f);
    getRightLegInputData()->getAnkle()->setStiffness(localStiff, m_damping+1.f);

    //MMMM Feet go unstable when stationary and in contact with m_parameters.m_stiffness = 7.f unless
    //Will put code in below after more testing
    //if (m_character->m_COMvelMag < 0.2f)
    //{
    //  getLeftLeg()->getAnkle()->setStiffness(8.f,1.f);
    //  getRightLeg()->getAnkle()->setStiffness(8.f,1.f);
    //  getLeftLeg()->getKnee()->setStiffness(7.f,1.f);
    //  getRightLeg()->getKnee()->setStiffness(7.f,1.f);
    //}


    float upDot = m_character->m_gUp.Dot(rotVel);
    float magRV  = m_character->m_COMrotvelMag;

    NM_RS_DBG_LOGF(L"magRV magRV: %.4f", magRV);

    if (((upDot>0.f) && (upDot<0.1f))&&(magRV>2.7f))
    {
      //randomize each turn
      float randNo = m_character->getRandom().GetRanged(-1.0f, 1.0f);
      m_asymm = m_parameters.m_asymmetricalLegs*(rage::Abs(randNo)/randNo - 0.3f*randNo); //(-or+)m_asymmetricalLegs -upto30%
    }

    NM_RS_DBG_LOGF(L"Leg asymmetry: %.4f", m_asymm);


    float hipMax = 1.6f;
    if (m_fromShot)
      hipMax = rage::Clamp(0.3f - forwardFall, 0.f, 1.f);

    getLeftLegInputData()->getHip()->setDesiredLean1(m_asymm + interpolate(0.125f, hipMax - legPush, x));
    getRightLegInputData()->getHip()->setDesiredLean1(-m_asymm + interpolate(0.125f, hipMax - legPush, x));

    float kneeBend = 1.25f - 3.f*legPush;
    float sinL = 0.f;//sinf(m_phase + m_rollUpTimer*m_pedalSpeed);
    float sinR = 0.f;//sinf(m_phase + m_rollUpTimer*m_pedalSpeed*1.13f + 3.f);
    getLeftLegInputData()->getKnee()->setDesiredAngle(-m_asymm + interpolate(-0.5f, -kneeBend-0.75f*sinL, x));
    getRightLegInputData()->getKnee()->setDesiredAngle(m_asymm + interpolate(-0.5f, -kneeBend-0.75f*sinR, x));

    float spreadScale = 0.35f;
    if (m_character->getBodyIdentifier() == gtaWilma)
      spreadScale = 0.1f;

    getLeftLegInputData()->getHip()->setDesiredLean2(-0.5f*m_asymm + (1.3f-lLean)*-0.3f   - (1 + sinf(m_phase + m_rollUpTimer * 0.4f* m_pedalSpeed))*0.5f*spreadScale);
    getRightLegInputData()->getHip()->setDesiredLean2(+0.5f*m_asymm + (1.4f-rLean)*-0.3f  - (1 + sinf(m_phase + m_rollUpTimer * 0.3f* m_pedalSpeed))*0.5f*spreadScale);

    getSpineInputData()->setBackAngles(interpolate(0.5f, 1.5f, x), 0.f, m_asymm);

    getSpineInputData()->getSpine0()->setDesiredLean1(interpolate(0, 0.35f, x));
    getSpineInputData()->getUpperNeck()->setDesiredLean1(interpolate(0.2f, 0.5f, x));
    getSpineInputData()->getLowerNeck()->setDesiredLean1(interpolate(0.2f, 0.5f, x));

    getLeftArmInputData()->getShoulder()->setDesiredTwist(interpolate(-1.f, -1.f, x));
    getLeftArmInputData()->getShoulder()->setDesiredLean1(lLean * 1.7f + m_extraSpread);
    getLeftArmInputData()->getShoulder()->setDesiredLean2(interpolate(-0.5f, -0.5f, x));
    getLeftArmInputData()->getElbow()->setDesiredAngle(interpolate(1.5f, 1.7f, x));

    getRightArmInputData()->getShoulder()->setDesiredTwist(interpolate(-1.f, -1.f, x));
    getRightArmInputData()->getShoulder()->setDesiredLean1(rLean * 1.7f + m_extraSpread);
    getRightArmInputData()->getShoulder()->setDesiredLean2(interpolate(-0.5f, -0.5f, x));
    getRightArmInputData()->getElbow()->setDesiredAngle(interpolate(1.5f, 1.7f, x));

    getLeftLegInputData()->getAnkle()->setDesiredLean1(-m_asymm + interpolate(-1.5f, -1.5f, x));
    getRightLegInputData()->getAnkle()->setDesiredLean1(m_asymm + interpolate(-1.5f, -1.5f, x));;

    // if not spinning too fast, keep head away from ground (rather than tucked in)
    if (amountOfRoll < 5.f)
      getSpine()->keepHeadAwayFromGround(getSpineInput(), 1.0f);

    // this is prototype hand placement code
    float relevance;
    // this scales stiffness with relevance (if relevance is high (doing arm plant) keep the stiffness high)
    // also scales stiffness with amount of motion (if motion is low, relax)
    m_blendL = plantArm(getLeftArm()->getShoulder(), getLeftArm()->getClaviclePart(), getLeftArmInputData(), 1.f, m_blendL,m_armL, relevance);
    getLeftArm()->setBodyStiffness(getLeftArmInput(), rage::Clamp(m_parameters.m_stiffness + 3.f*relevance + (4.f - 2.f*relevance)*(x - 1.f),0.5f,20.f),1.f);
    m_blendR = plantArm(getRightArm()->getShoulder(), getRightArm()->getClaviclePart(), getRightArmInputData(), -1.f, m_blendR,m_armL, relevance);
    getRightArm()->setBodyStiffness(getRightArmInput(), rage::Clamp(m_parameters.m_stiffness + 3.f*relevance + (4.f - 2.f*relevance)*(x - 1.f),0.5f,20.f),1.f);

    NM_RS_DBG_LOGF(L"m_blendL: %.4f", m_blendL);
    NM_RS_DBG_LOGF(L"m_blendR: %.4f", m_blendR);

    return eCBUTaskComplete;
  }

  float NmRsCBURollUp::plantArmGetBlend(float min, float max, float blendWidth, float x, float y)
  {
    float min2 = max-blendWidth;
    float bottom = rage::Min((x-min2)/blendWidth, 1.f);
    float top = rage::Max((x-min)/blendWidth, 0.f);
    y = rage::Clamp(y, bottom, top);

    NM_RS_DBG_LOGF(L"x: %.4f y: %.4f top: %.4f bottom: %.4f", x, y, top, bottom);
    return y;

  }

  float NmRsCBURollUp::plantArm(const NmRs3DofEffector* shoulderJoint, NmRsGenericPart* velocityPart, NmRsArmInputWrapper* input, float direction, float blend, float armL, float &relevance)
  {
    rage::Matrix34 shoulderTM;
    rage::Vector3 pos;
    rage::Vector3 vel;
    rage::Vector3 target;
    rage::Vector3 rotvel;
    relevance = 1.f;

    shoulderJoint->getMatrix1(shoulderTM);
    pos = shoulderTM.d;

    NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
    Assert(rdsTask);

    vel = getSpine()->getSpine3Part()->getLinearVelocity();
    if (rdsTask->m_parameters.m_useRelativeVelocity && rdsTask->isActive())
      vel -= m_character->getFloorVelocity();

    float upVel = vel.Dot(m_character->m_gUp);
    if (upVel > 0.f) // ramp down relevance if towards +v vertical velocity
      relevance = rage::Clamp(1.f-0.5f*upVel, 0.f, 1.f);

    float predictionTime = 0.5f;
    vel.AddScaled(vel, m_character->m_gUp, -0.5f*9.8f*predictionTime); //    -4.f);
    vel += (m_character->m_COMTM.b - m_character->m_COMTM.c) * 1.f; // TDL move the arm targets a bit further up and out
    vel.Normalize();
    rotvel = getSpine()->getSpine3Part()->getAngularVelocity();
    target.AddScaled(pos, vel, 0.5f);

    vel.Cross(rotvel, m_character->m_gUp);

    float rotationCompensation = 0.0125f;
    target.AddScaled(target, vel, rotationCompensation);
    target.Subtract(target, pos);
    target.Normalize();
    vel = target;

    target *= armL;

    target.Add(target, pos);

    // attempt to find mid range of arm movement vector, to give relevancy index
    rage::Matrix34 spine3TM;
    rage::Vector3 midRangeVector;
    rage::Vector3 axis;
    getSpine()->getSpine3Part()->getBoundMatrix(&spine3TM);
    midRangeVector = spine3TM.b;
    midRangeVector *= direction;
    axis = spine3TM.c;
    midRangeVector.AddScaled(midRangeVector, axis, -2.f);
    midRangeVector.Normalize();

    float blah = midRangeVector.Dot(vel);
    float armReach = m_parameters.m_armReachAmount;
    blend = plantArmGetBlend(0.7f - armReach, 1.3f - armReach, 0.3f, blah, blend);
    relevance = relevance * blend;

    vel = velocityPart->getLinearVelocity();

    if (relevance > 0.f)
    {
      float twist = -0.5f;
      float dragReduction = 1.f;

      NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
      NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
      ikInputData->setTarget(target);
      ikInputData->setTwist(twist);
      ikInputData->setDragReduction(dragReduction);
      ikInputData->setVelocity(vel);
      ikInputData->setBlend(relevance);
      ikInputData->setMatchClavicle(kMatchClavicle);

      if (direction > 0.f)
      {
        getLeftArm()->postInput(ikInput);
      }
      else
      {
        getRightArm()->postInput(ikInput);
      }

      input->getWrist()->setDesiredLean2(relevance * -0.8f);
    }
    return blend;
  }

#if ART_ENABLE_BSPY
  void NmRsCBURollUp::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_stiffness, true);
    bspyTaskVar(m_parameters.m_useArmToSlowDown, true);
    bspyTaskVar(m_parameters.m_armReachAmount, true);
    bspyTaskVar(m_parameters.m_legPush, true);
    bspyTaskVar(m_parameters.m_asymmetricalLegs, true);
    bspyTaskVar(m_parameters.m_rollVelForSuccess, true);
    bspyTaskVar(m_parameters.m_rollVelLinearContribution, true);
    bspyTaskVar(m_parameters.m_noRollTimeBeforeSuccess, true);

    bspyTaskVar(m_rollUpTimer, false);
    bspyTaskVar(m_damping, false);
    bspyTaskVar(m_armL, false);
    bspyTaskVar(m_blendL, false);
    bspyTaskVar(m_blendR, false);
    bspyTaskVar(m_timeNotRolling, false);
    bspyTaskVar(m_rotCOMSmoothed, false);
    bspyTaskVar(m_asymm, false);
    bspyTaskVar(m_pedalSpeed, false);
    bspyTaskVar(m_extraSpread, false);
    bspyTaskVar(m_phase, false);
    bspyTaskVar_Bitfield32(m_parameters.m_effectorMask, true);
    bspyTaskVar(m_fromShot, false);
  }
#endif // ART_ENABLE_BSPY
}
