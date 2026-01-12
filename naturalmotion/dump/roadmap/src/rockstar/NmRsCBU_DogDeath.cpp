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
 *
 * 
 * Spine Twist. System for twisting the spine of the character towards a point
 * which is possibly moving.
 * 
 */


#include "NmRsInclude.h"
#include "NmRsCBU_DogDeath.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "ART/ARTFeedback.h"

namespace ART
{
  NmRsCBUDogDeath::NmRsCBUDogDeath(ART::MemoryManager* services) : CBUTaskBase(services, bvid_quadDeath),
    m_behaviourTime(0.0f),
    m_leftLeg(0),
    m_rightLeg(0),
    m_leftArm(0),
    m_rightArm(0),
    m_spine(0)
  {

    initialiseCustomVariables();

  }

  NmRsCBUDogDeath::~NmRsCBUDogDeath()
  {
  }

  void NmRsCBUDogDeath::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_spineStiffness = 3.0f;
    m_behaviourTime = 0.0f;
    m_collided = false;
    m_fallLeft = false;
    m_pelvisHeight = -1.f;
    m_shoveMag = 0.f;
  }

  void NmRsCBUDogDeath::onActivate()
  {
    Assert(m_character);
    Assert( m_character->getBodyIdentifier() == mp3Dog ||
      m_character->getBodyIdentifier() == rdrHorse );

    if (m_character->getBodyIdentifier() != mp3Dog &&
      m_character->getBodyIdentifier() != rdrHorse )
      return;

    // locally cache the limb definitions
    m_leftLeg = static_cast<const HindLegSetup*>(m_character->getLimbIKSetup(LimbIKSetup::kLeftLeg));
    m_rightLeg = static_cast<const HindLegSetup*>(m_character->getLimbIKSetup(LimbIKSetup::kRightLeg));
    m_leftArm = static_cast<const ForeLegSetup*>(m_character->getLimbIKSetup(LimbIKSetup::kLeftArm));
    m_rightArm = static_cast<const ForeLegSetup*>(m_character->getLimbIKSetup(LimbIKSetup::kRightArm));
    m_spine = m_character->getSpineSetup();

    //Scale incoming angVel
    //We know its incoming from animation as there are no other behaviours running on the horse
    //If it does get activated from say a ragdoll horse then call with no scale parameters
    for (int i = 0; i<m_character->getNumberOfParts(); i++)
    {
      if(m_character->isPartInMask(m_parameters.angVelScaleMask, i))
      {
        NmRsGenericPart *part = m_character->getGenericPartByIndex(i);
        rage::Vector3 rotVel = part->getAngularVelocity();
        rotVel *= m_parameters.angVelScale;
        part->setAngularVelocity(rotVel);
      }
    }

    // cache limb flex

    // todo: pick the fall direction more sensibly
    const float flexVariance = 0.f; //0.1f;
    float sideOffset = 0.f; //0.3f;
    m_fallLeft = m_character->getRandom().GetBool();
    if(m_fallLeft)
    {
      m_legFlexLengths[LimbIKSetup::kLeftArm] = m_parameters.legsFlexAmount+sideOffset+2.f*(m_character->getRandom().GetFloat()-0.5f)*flexVariance;
      m_legFlexLengths[LimbIKSetup::kLeftLeg] = m_parameters.legsFlexAmount+sideOffset+2.f*(m_character->getRandom().GetFloat()-0.5f)*flexVariance;
      m_legFlexLengths[LimbIKSetup::kRightArm] = m_parameters.legsFlexAmount-sideOffset+2.f*(m_character->getRandom().GetFloat()-0.5f)*flexVariance;
      m_legFlexLengths[LimbIKSetup::kRightLeg] = m_parameters.legsFlexAmount-sideOffset+2.f*(m_character->getRandom().GetFloat()-0.5f)*flexVariance;
    }
    else
    {
      m_legFlexLengths[LimbIKSetup::kLeftArm] = m_parameters.legsFlexAmount-sideOffset+2.f*(m_character->getRandom().GetFloat()-0.5f)*flexVariance;
      m_legFlexLengths[LimbIKSetup::kLeftLeg] = m_parameters.legsFlexAmount-sideOffset+2.f*(m_character->getRandom().GetFloat()-0.5f)*flexVariance;
      m_legFlexLengths[LimbIKSetup::kRightArm] = m_parameters.legsFlexAmount+sideOffset+2.f*(m_character->getRandom().GetFloat()-0.5f)*flexVariance;
      m_legFlexLengths[LimbIKSetup::kRightLeg] = m_parameters.legsFlexAmount+sideOffset+2.f*(m_character->getRandom().GetFloat()-0.5f)*flexVariance;
    }

    m_character->holdPoseAllEffectors();

  }

  void NmRsCBUDogDeath::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUDogDeath::onTick(float timeStep)
  {
    Assert(m_parameters.bodyRampDuration > timeStep);
    Assert(m_parameters.bodyRampDuration > timeStep);
    Assert(m_parameters.bodyRampDuration > timeStep);
    Assert(m_parameters.endStiffness >= 3.0f);
    Assert(m_parameters.startStiffness >= 3.0f);
    Assert(m_parameters.startStiffness > m_parameters.endStiffness);

    // calculate body ramp value
    float bodyRamp = 0.f;
    if(m_behaviourTime < m_parameters.bodyRampDuration)
      bodyRamp = 1.f-(m_behaviourTime/m_parameters.bodyRampDuration);

    // set body stiffness/damping/strength
    const float startMuscleStiffness = 1.0f;
    float muscleStiffness = m_parameters.muscleStiffness+(startMuscleStiffness-m_parameters.muscleStiffness)*bodyRamp;
    m_spineStiffness = m_parameters.endStiffness+(m_parameters.startStiffness-m_parameters.endStiffness)*bodyRamp;
    m_character->setBodyStiffness(m_spineStiffness,
      m_parameters.dampingScale, "us", 
      &muscleStiffness);

    // calculate neck ramp value
    float neckRamp = 0.f;
    if(m_behaviourTime < m_parameters.neckRampDuration)
      neckRamp = 1.f-(m_behaviourTime/m_parameters.neckRampDuration);

    // set neck stiffness/damping/strength
    muscleStiffness = m_parameters.muscleStiffness+(startMuscleStiffness-m_parameters.muscleStiffness)*neckRamp;
    m_neckStiffness = m_parameters.endStiffness+(m_parameters.startStiffness-m_parameters.endStiffness)*neckRamp;
    m_character->setBodyStiffness(m_neckStiffness,
      m_parameters.dampingScale, "un", 
      &muscleStiffness);

    // calculate legs ramp value
    float legsRamp = 0.f;
    if(m_behaviourTime < m_parameters.legsRampDuration)
      legsRamp = 1.f-(m_behaviourTime/m_parameters.legsRampDuration);

    // set legs stiffness/damping/strength
    muscleStiffness = m_parameters.muscleStiffness+(startMuscleStiffness-m_parameters.muscleStiffness)*legsRamp;
    m_legStiffness = m_parameters.legEndStiffness+(m_parameters.legStartStiffness-m_parameters.legEndStiffness)*legsRamp;
    m_character->setBodyStiffness(m_legStiffness, m_parameters.dampingScale, "lb", &muscleStiffness);

    /* 
    *  Control leg collapse
    */

    float poseStep = timeStep/m_parameters.legsFlexDuration;
    poseLimb(m_leftArm, m_legFlexLengths[LimbIKSetup::kLeftArm], poseStep);
    poseLimb(m_rightArm, m_legFlexLengths[LimbIKSetup::kRightArm], poseStep);
    poseLimb(m_leftLeg, m_legFlexLengths[LimbIKSetup::kLeftLeg], poseStep);
    poseLimb(m_rightLeg, m_legFlexLengths[LimbIKSetup::kRightLeg], poseStep);

    if(m_character->getBodyIdentifier() == rdrHorse)
    {
      solveTendon(m_leftArm, m_parameters.tendonOffset);
      solveTendon(m_rightArm, m_parameters.tendonOffset);
      solveTendon(m_leftLeg, m_parameters.tendonOffset);
      solveTendon(m_rightLeg, m_parameters.tendonOffset);
    }

    // Check if we've hit something
    if(!m_collided) 
    {

      m_collided |= m_spine->getHeadPart()->collidedWithNotOwnCharacter();
      m_collided |= m_spine->getSpine0Part()->collidedWithNotOwnCharacter();
      m_collided |= m_spine->getSpine1Part()->collidedWithNotOwnCharacter();
      m_collided |= m_spine->getSpine2Part()->collidedWithNotOwnCharacter();
      m_collided |= m_spine->getSpine3Part()->collidedWithNotOwnCharacter();
      m_collided |= m_spine->getNeckPart()->collidedWithNotOwnCharacter();
      m_collided |= m_spine->getHeadPart()->collidedWithNotOwnCharacter();
      m_collided |= m_leftArm->getClaviclePart()->collidedWithNotOwnCharacter();
      m_collided |= m_rightArm->getClaviclePart()->collidedWithNotOwnCharacter();

    }

    if(!m_collided) 
    {
      // get a direction to try to point the head
      rage::Vector3 pointHeadDirection;
      pointHeadDirection = m_spine->getSpine2Part()->getLinearVelocity();   // start with direction of fall
      pointHeadDirection *= -1.f;                                           // invert fall direction
      pointHeadDirection.Normalize();
      pointHeadDirection += m_character->m_gUp;                             // add a bit of up to keep head off ground
      pointHeadDirection.Normalize();

      // map point head direction into the space of each of our spine
      // joints and drive there
      rage::Vector3 boneDirection(0.0f, 0.0f, 1.0f);
      pointSpineJoint(m_spine->getSpine0(), boneDirection, pointHeadDirection);
      pointSpineJoint(m_spine->getSpine1(), boneDirection, pointHeadDirection);
      pointSpineJoint(m_spine->getSpine2(), boneDirection, pointHeadDirection);
      pointSpineJoint(m_spine->getSpine3(), boneDirection, pointHeadDirection);
      pointSpineJoint(m_spine->getLowerNeck(), boneDirection, pointHeadDirection);
      boneDirection.Set(0.0f, -0.37139070f, 0.92847675f);
      pointSpineJoint(m_spine->getUpperNeck(), boneDirection, pointHeadDirection);

    }

    /* 
    *  Give him a little shove if his arse is in the air towards the end of the body collapse
    */
    if(m_parameters.helperImpulse > 0.1f && m_behaviourTime > m_parameters.bodyRampDuration)
    {
      const float shoveThreshold = 0.5f;
      const float shoveDuration = 0.5f;
      const rage::Vector3 backFootCenter = (m_leftLeg->getToePart()->getPosition() + m_rightLeg->getToePart()->getPosition())/2.f;
      const rage::Vector3 pelvisCenter = m_spine->getPelvisPart()->getPosition();
      if(pelvisCenter.y - backFootCenter.y > shoveThreshold && m_behaviourTime - m_parameters.legsRampDuration < shoveDuration)
      {
        const float maxShove = m_parameters.helperImpulse;
        m_shoveMag = (m_behaviourTime - m_parameters.legsRampDuration)/shoveDuration;
        m_shoveMag = rage::Clamp(m_shoveMag, 0.f, 1.f);//Note this clamp wasn't being applied until 12/05/09
        m_shoveMag *= maxShove;
        rage::Matrix34 pelvisTm;
        m_spine->getPelvisPart()->getBoundMatrix(&pelvisTm);
        if(m_fallLeft)
        {
          m_spine->getPelvisPart()->applyImpulse(pelvisTm.c * m_shoveMag, rage::Vector3(0, 0, 0));
        }
        else
        {
          m_spine->getPelvisPart()->applyImpulse(pelvisTm.c * -m_shoveMag, rage::Vector3(0, 0, 0));
        }
      }
    }

    m_behaviourTime += timeStep;

    return eCBUTaskComplete;

  }

  void NmRsCBUDogDeath::poseLimb(const ForeLegSetup* limb, const float angle, const float step)
  {
    float localAngle = rage::Clamp(angle, 0.f, 1.f);
    if(m_character->getBodyIdentifier() == rdrHorse)
    {
      poseJoint(limb->getClavicle(),  -0.17f*localAngle, step);
      poseJoint(limb->getShoulder(),  -0.70f*localAngle, step);
      poseJoint(limb->getElbow(),     1.74f*localAngle,  step);
      poseJoint(limb->getWrist(),     -2.44f*localAngle, step);
      poseJoint(limb->getFinger(),    -1.05f*localAngle, step);
      poseJoint(limb->getNail(),      1.31f*localAngle,  step);
    }
    else if(m_character->getBodyIdentifier() == mp3Dog)
    {
      poseJoint(limb->getClavicle(),  -0.2f*localAngle, step);
      poseJoint(limb->getShoulder(),  -0.4f*localAngle, step);
      poseJoint(limb->getElbow(),     0.4f*localAngle,  step);
      poseJoint(limb->getWrist(),     -1.f*localAngle, step);
      poseJoint(limb->getNail(),      -1.f*localAngle,  step);
    }
    else
      Assert(false);

  }

  void NmRsCBUDogDeath::poseLimb(const HindLegSetup* limb, const float angle, const float step)
  {
    float localAngle = rage::Clamp(angle, 0.f, 1.f);
    if(m_character->getBodyIdentifier() == rdrHorse)
    {
      poseJoint(limb->getHip(),   0.87f*localAngle,  step);
      poseJoint(limb->getKnee(),  -1.92f*localAngle, step);
      poseJoint(limb->getAnkle(), 1.92f*localAngle,  step);
      poseJoint(limb->getFoot(),  -0.96f*localAngle, step);
      poseJoint(limb->getToe(),   1.31f*localAngle,  step);
    }
    else if(m_character->getBodyIdentifier() == mp3Dog)
    {
      poseJoint(limb->getHip(),   0.7f*localAngle,  step);
      poseJoint(limb->getKnee(),  1.f*localAngle, step);
      poseJoint(limb->getAnkle(), -1.f*localAngle,  step);
      poseJoint(limb->getToe(),   -1.3f*localAngle,  step);
    }
    else
      Assert(false);

  }

  void NmRsCBUDogDeath::poseJoint(NmRs1DofEffector* joint, float angle, const float step)
  {
    if(!joint)
      return;

    float desired = joint->getDesiredAngle();
    float error = angle - desired;
    if(error > step)
      desired += step;
    else if(error < -step)
      desired -= step;
    else
      desired = angle;
    joint->setDesiredAngle(desired);
  }

  void NmRsCBUDogDeath::poseJoint(NmRs3DofEffector* joint, float angle, const float step)
  {
    if(!joint)
      return;

    float desired = joint->getDesiredLean1();
    float error = angle - desired;
    if(error > step)
      desired += step;
    else if(error < -step)
      desired -= step;
    else
      desired = angle;
    joint->setDesiredLean1(desired);
  }

  /*
  *  solveTendon - Apply torque to ankle, foot and toe joints based on the position of the knee
  *                joint to keep correct leg posture independent of effector control.
  */
  void NmRsCBUDogDeath::solveTendon(const HindLegSetup* limb, float offset)
  {
    float angle = nmrsGetActualAngle(limb->getKnee())/-1.92f; // angle as 0..1 across limit space
    float angle1 = angle - offset;
    float angle2 = angle + offset;
    if(limb == m_leftLeg)
    {
      m_leftAnkleError = enforceSoftLimit(limb->getAnkle(), 1.92f*angle2, 1.92f*angle1, 11.0f, m_legStiffness);
      m_leftFootError  = enforceSoftLimit(limb->getFoot(), -0.96f*angle1, -0.96f*angle2, 11.0f, m_legStiffness);
      m_leftToeError   = enforceSoftLimit(limb->getToe(), 1.31f*angle2, 1.31f*angle1, 11.0f, m_legStiffness);
    }
    else
    {
      m_rightAnkleError = enforceSoftLimit(limb->getAnkle(), 1.92f*angle2, 1.92f*angle1, 11.0f, m_legStiffness);
      m_rightFootError  = enforceSoftLimit(limb->getFoot(), -0.96f*angle1, -0.96f*angle2, 11.0f, m_legStiffness);
      m_rightToeError   = enforceSoftLimit(limb->getToe(), 1.31f*angle2, 1.31f*angle1, 11.0f, m_legStiffness);
    }
  }

  /*
  *  solveTendon - Apply torque to wrist, finger and nail joints based on the position of the elbow
  *                joint to keep correct leg posture independent of effector control.
  */
  void NmRsCBUDogDeath::solveTendon(const ForeLegSetup* limb, float offset)
  {
    float angle = nmrsGetActualAngle(limb->getElbow())/1.74f;    // angle as 0..1 across limit space
    float angle1 = angle - offset;
    float angle2 = angle + offset;
    if(limb == m_leftArm)
    {
      m_leftWristError  = enforceSoftLimit(limb->getWrist(), -2.44f*angle1, -2.44f*angle2, 11.0f, m_legStiffness);
      m_leftFingerError = enforceSoftLimit(limb->getFinger(), -1.05f*angle1, -1.05f*angle2, 11.0f, m_legStiffness);
      m_leftNailError   = enforceSoftLimit(limb->getNail(), 1.31f*angle2, 1.31f*angle1, 11.0f, m_legStiffness);
    }
    else
    {
      m_rightWristError  = enforceSoftLimit(limb->getWrist(), -2.44f*angle1, -2.44f*angle2, 11.0f, m_legStiffness);
      m_rightFingerError = enforceSoftLimit(limb->getFinger(), -1.05f*angle1, -1.05f*angle2, 11.0f, m_legStiffness);
      m_rightNailError   = enforceSoftLimit(limb->getNail(), 1.31f*angle2, 1.31f*angle1, 11.0f, m_legStiffness);
    }
  }

  float NmRsCBUDogDeath::enforceSoftLimit(NmRs1DofEffector* effector, float uLimit, float lLimit, float limitStiff, float motorStiff)
  {
    if(!effector)
      return 0;

    float actualAngle = nmrsGetActualAngle(effector);
    if(actualAngle > uLimit)
    {
      effector->setDesiredAngle(uLimit);
      float newStiff = rage::Min(m_parameters.legStartStiffness, motorStiff+limitStiff*rage::Abs(actualAngle-uLimit));
      Assert(newStiff >= 3.0f);
      effector->setStiffness(newStiff, 1.0f);
      return actualAngle - uLimit;
    }
    else if(actualAngle < lLimit)
    {
      effector->setDesiredAngle(lLimit);
      float newStiff = rage::Min(m_parameters.legStartStiffness, motorStiff+limitStiff*rage::Abs(lLimit-actualAngle));
      Assert(newStiff >= 3.0f);
      effector->setStiffness(newStiff, 1.0f);
      return actualAngle - lLimit;  
    }
    return 0; 
  }

  float NmRsCBUDogDeath::enforceSoftLimit(NmRs3DofEffector* effector, float uLimit, float lLimit, float limitStiff, float motorStiff)
  {
    if(!effector)
      return 0;

    float actualAngle = nmrsGetActualLean1(effector);
    if(actualAngle > uLimit)
    {
      float newStiff = rage::Min(m_parameters.legStartStiffness, motorStiff+limitStiff*rage::Abs(actualAngle-uLimit));
      float newAngle = uLimit; 
      effector->setDesiredLean1(newAngle);
      Assert(newStiff >= 3.0f);
      effector->setStiffness(newStiff, 1.0f);
      return actualAngle - uLimit;
    }
    else if(actualAngle < lLimit)
    {
      float newStiff = rage::Min(m_parameters.legStartStiffness, motorStiff+limitStiff*rage::Abs(actualAngle-lLimit));
      float newAngle = lLimit;
      effector->setDesiredLean1(newAngle);
      Assert(newStiff >= 3.0f);
      effector->setStiffness(newStiff, 1.0f);
      return actualAngle - lLimit;
    }
    return 0;
  }

  void NmRsCBUDogDeath::pointSpineJoint(NmRs3DofEffector* joint, const rage::Vector3& boneDirection, rage::Vector3& direction)
  {
    if(!joint)
    {
      Assert(false);
      return;
    }

    rage::Matrix34 jtTM;
    rage::Vector3 axis, ts;
    rage::Quaternion q;
    rage::Vector3 boneDirectionWorld;
    joint->getMatrix1(jtTM);
    jtTM.Transform3x3(boneDirection, boneDirectionWorld);
    axis.Cross(boneDirectionWorld, direction);
    float angle = rage::AcosfSafe(boneDirectionWorld.Dot(direction));
    jtTM.UnTransform3x3(axis);
    q.FromRotation(axis, angle);
    ts = rsQuatToRageDriveTwistSwing(q);
    ts *= m_parameters.headLift;
    joint->setDesiredTwist(ts.x);
    joint->setDesiredLean1(ts.y);
    joint->setDesiredLean2(ts.z);
    // debug lines
    {
      const float scale = 0.05f;
      rage::Vector3 start, end;
      start = jtTM.d;
      end = start;
      end.AddScaled(direction, scale);
      end = start;
      end.AddScaled(boneDirectionWorld, scale);
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUDogDeath::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.startStiffness, true);
    bspyTaskVar(m_parameters.endStiffness, true);
    bspyTaskVar(m_parameters.legStartStiffness, true);
    bspyTaskVar(m_parameters.legEndStiffness, true);
    bspyTaskVar(m_parameters.bodyRampDuration, true);
    bspyTaskVar(m_parameters.neckRampDuration, true);
    bspyTaskVar(m_parameters.legsRampDuration, true);
    bspyTaskVar(m_parameters.dampingScale, true);
    bspyTaskVar(m_parameters.headLift, true);
    bspyTaskVar(m_parameters.muscleStiffness, true);
    bspyTaskVar(m_parameters.legsFlexAmount, true);
    bspyTaskVar(m_parameters.legsFlexDuration, true);
    bspyTaskVar(m_parameters.tendonOffset, true);
    bspyTaskVar(m_parameters.helperImpulse, true);
    bspyTaskVar(m_parameters.angVelScale, true);
    bspyTaskVar_Bitfield32(m_parameters.angVelScaleMask, true);


    bspyTaskVar(m_spineStiffness, false);
    bspyTaskVar(m_neckStiffness, false);
    bspyTaskVar(m_legStiffness, false);
    bspyTaskVar(m_behaviourTime, false);
    bspyTaskVar(m_shoveMag, false);

    bspyTaskVar(m_leftAnkleError, false);
    bspyTaskVar(m_leftFootError, false);
    bspyTaskVar(m_leftToeError, false);
    bspyTaskVar(m_rightAnkleError, false);
    bspyTaskVar(m_rightFootError, false);
    bspyTaskVar(m_rightToeError, false);
    bspyTaskVar(m_leftWristError, false);
    bspyTaskVar(m_leftFingerError, false);
    bspyTaskVar(m_leftNailError, false);
    bspyTaskVar(m_rightWristError, false);
    bspyTaskVar(m_rightFingerError, false);
    bspyTaskVar(m_rightNailError, false);
    bspyTaskVar(m_fallLeft, false);
    bspyTaskVar(m_pelvisHeight, false);
  }
#endif
}

