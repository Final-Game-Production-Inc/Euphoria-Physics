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
#include "NmRsCBU_SDCatchfall.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_SpineTwist.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_BodyFoetal.h"

namespace ART
{
  NmRsCBUSDCatchFall::NmRsCBUSDCatchFall(ART::MemoryManager* services) : CBUTaskBase(services, bvid_sDCatchFall),
    m_leftArm(0), 
    m_rightArm(0), 
    m_leftLeg(0), 
    m_rightLeg(0), 
    m_spine(0)
  {
    initialiseCustomVariables();
  }

  NmRsCBUSDCatchFall::~NmRsCBUSDCatchFall()
  {
  }

  void NmRsCBUSDCatchFall::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_timer = 0.f;
    m_groundNormal.Zero();
    m_workOutNormal = true;
  }

  void NmRsCBUSDCatchFall::onActivate()
  {

    Assert(m_character);

    // locally cache the limb definitions
    m_leftArm = m_character->getLeftArmSetup();
    m_rightArm = m_character->getRightArmSetup();
    m_leftLeg = m_character->getLeftLegSetup();
    m_rightLeg = m_character->getRightLegSetup();
    m_spine = m_character->getSpineSetup();

    m_fallDirection.Set(0,0,0);
    m_forwardsAmount = 0;
    m_leftRight = 1.f;

    m_character->resetEffectorsToDefaults();
    m_character->setBodyStiffness(m_parameters.m_torsoStiffness, 0.5f, "uk");
    m_character->setBodyStiffness(m_parameters.m_armsStiffness, 1.f, "ua");
    m_character->setBodyStiffness(m_parameters.m_legsStiffness, 0.5f, "lb");

    m_leftArm->getWrist()->setMuscleStrength(10.f);
    m_rightArm->getWrist()->setMuscleStrength(10.f);

    m_kneeBendL = m_character->getRandom().GetRanged(0.4f, 0.8f);
    m_kneeBendR = m_character->getRandom().GetRanged(0.4f, 0.8f);

    m_leftArmState.init(m_character, this);
    m_leftArmState.enter(m_leftArm, true, "ul");
    m_rightArmState.init(m_character, this);
    m_rightArmState.enter(m_rightArm, false, "ur");
  }

  void NmRsCBUSDCatchFall::onDeactivate()
  {
    Assert(m_character);

    m_cbuParent->m_tasks[bvid_headLook]->deactivate();
    m_cbuParent->m_tasks[bvid_spineTwist]->deactivate();

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUSDCatchFall::onTick(float timeStep)
  {
    // On the first time of the tick work out the normal. Cant't do this in the activate because it hasn't got the normal yet. 
    // On activate need to decide which way to turn 

    if (m_workOutNormal)
    {
      // Dot of the side Vec with the normal
      rage::Matrix34 comTm = m_character->m_COMTM;
      rage::Vector3 rightSide = comTm.a;
      if (rightSide.Dot(m_groundNormal)>0.f)
        m_leftRight = -1.f;
      m_workOutNormal = false;
    }

    m_timer += timeStep;
    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist];      
    if (spineTwistTask->isActive())
      spineTwistTask->deactivate();
    NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)m_cbuParent->m_tasks[bvid_headLook];      
    if (headLookTask->isActive())
      headLookTask->deactivate();

    float armSt = 0.6f;
    m_character->setBodyStiffness(m_parameters.m_torsoStiffness, 0.75f, "uk",&armSt);
    m_character->setBodyStiffness(m_parameters.m_armsStiffness, 1.5f, "ul",&armSt);
    m_character->setBodyStiffness(m_parameters.m_armsStiffness, 1.5f, "ur",&armSt);
    m_character->setBodyStiffness(m_parameters.m_legsStiffness, 0.5f, "lb");

    // set head look target
    m_fallDirection = m_spine->getSpine3Part()->getLinearVelocity() - m_floorVel;
    if (m_character->hasCollidedWithWorld("fb"))
    {
      m_character->setBodyStiffness(m_parameters.m_torsoStiffness*0.5f, 0.75f, "uk");
    }
    m_fallDirection.Normalize();

    m_rightArm->getWrist()->setStiffness(m_parameters.m_torsoStiffness,1.f);
    m_leftArm->getWrist()->setStiffness(m_parameters.m_torsoStiffness,1.f);

    m_forwardsAmount = -m_fallDirection.Dot(m_character->m_COMTM.c);

    float extraSit = 1.f;
    float localm_randomSpineL2 = 1.5f*m_leftRight;
    float localm_randomTwist = 3.f*m_leftRight;
    float localheadLean1Dir = -2.f*m_leftRight;

    // if we are on our back
    if (m_parameters.m_backSideFront==kBack)
    {
      // So work out the back bend!!!
      rage::Vector3 backLine = m_spine->getSpine3Part()->getPosition()- m_spine->getSpine0Part()->getPosition();
      backLine.Normalize();
      // project onto the normal of the hit point. 
      float projBackOntoNormal = rage::Abs(backLine.Dot(m_groundNormal));

      extraSit = 2.f*projBackOntoNormal;
      localheadLean1Dir = 2.f;
      localm_randomSpineL2 = 0.f;
    }
    else if (m_parameters.m_backSideFront==kFront)// on the front
    {
      extraSit = -2.f;
      localheadLean1Dir = -2.f;
    }

    m_spine->getSpine0()->setDesiredLean1(extraSit*0.2f);
    m_spine->getSpine0()->setDesiredLean2(localm_randomSpineL2*0.2f);
    m_spine->getSpine0()->setDesiredTwist(localm_randomTwist*0.2f);

    m_spine->getSpine1()->setDesiredLean1(extraSit*0.2f);
    m_spine->getSpine1()->setDesiredLean2(localm_randomSpineL2*0.2f);
    m_spine->getSpine1()->setDesiredTwist(localm_randomTwist*0.2f);

    m_spine->getSpine2()->setDesiredLean1(extraSit*0.2f);
    m_spine->getSpine2()->setDesiredLean2(localm_randomSpineL2*0.2f);
    m_spine->getSpine2()->setDesiredTwist(localm_randomTwist*0.2f);

    m_spine->getSpine3()->setDesiredLean1(extraSit*0.2f);
    m_spine->getSpine3()->setDesiredLean2(localm_randomSpineL2*0.2f);
    m_spine->getSpine3()->setDesiredTwist(localm_randomTwist*0.2f);

    m_spine->getLowerNeck()->setDesiredLean1(localheadLean1Dir);
    m_spine->getLowerNeck()->setDesiredLean2(localm_randomSpineL2);
    m_spine->getUpperNeck()->setDesiredLean1(localheadLean1Dir);
    m_spine->getUpperNeck()->setDesiredLean2(localm_randomSpineL2);

    if (m_parameters.m_useLeft)
    {
      if (m_parameters.m_backSideFront==kRightSide)
      {

        nmrsSetLean1(m_leftArm->getClavicle(),.35f);
        nmrsSetLean2(m_leftArm->getClavicle(),-0.05f);
        nmrsSetTwist(m_leftArm->getClavicle(),-0.1f);
        nmrsSetLean1(m_leftArm->getShoulder(),2.1f);
        nmrsSetLean2(m_leftArm->getShoulder(),1.7f);
        nmrsSetTwist(m_leftArm->getShoulder(),0.45f);
        nmrsSetAngle(m_leftArm->getElbow(),1.32f);
        nmrsSetLean1(m_leftArm->getWrist(),0.55f);
        nmrsSetLean2(m_leftArm->getWrist(),-.65f);
        nmrsSetTwist(m_leftArm->getWrist(),0.9f);

      }
      else if (m_parameters.m_backSideFront==kLeftSide)
      {
        nmrsSetLean1(m_leftArm->getClavicle(),.02f);
        nmrsSetLean2(m_leftArm->getClavicle(),-0.9f);
        nmrsSetTwist(m_leftArm->getClavicle(),0.2f);
        nmrsSetLean1(m_leftArm->getShoulder(),-2.03f);
        nmrsSetLean2(m_leftArm->getShoulder(),-0.5f);
        nmrsSetTwist(m_leftArm->getShoulder(),0.4f);
        nmrsSetAngle(m_leftArm->getElbow(),1.5f);
        nmrsSetLean1(m_leftArm->getWrist(),0.1f);
        nmrsSetLean2(m_leftArm->getWrist(),-.1f);
        nmrsSetTwist(m_leftArm->getWrist(),1.f);
      }
      else
      {
        m_leftArmState.tick(timeStep);
        m_leftArm->getWrist()->setDesiredLean2(-3.f);
      }
    }

    if (m_parameters.m_useRight)
    {
      if (m_parameters.m_backSideFront==kLeftSide)
      {
        nmrsSetLean1(m_rightArm->getClavicle(),.35f);
        nmrsSetLean2(m_rightArm->getClavicle(),-0.05f);
        nmrsSetTwist(m_rightArm->getClavicle(),-0.1f);
        nmrsSetLean1(m_rightArm->getShoulder(),2.1f);
        nmrsSetLean2(m_rightArm->getShoulder(),1.7f);
        nmrsSetTwist(m_rightArm->getShoulder(),0.45f);
        nmrsSetAngle(m_rightArm->getElbow(),1.32f);
        nmrsSetLean1(m_rightArm->getWrist(),0.55f);
        nmrsSetLean2(m_rightArm->getWrist(),-.65f);
        nmrsSetTwist(m_rightArm->getWrist(),0.9f);

      }
      else if(m_parameters.m_backSideFront==kRightSide)
      {
        nmrsSetLean1(m_rightArm->getClavicle(),.02f);
        nmrsSetLean2(m_rightArm->getClavicle(),-0.9f);
        nmrsSetTwist(m_rightArm->getClavicle(),0.2f);
        nmrsSetLean1(m_rightArm->getShoulder(),-2.f);
        nmrsSetLean2(m_rightArm->getShoulder(),-0.5f);
        nmrsSetTwist(m_rightArm->getShoulder(),0.4f);
        nmrsSetAngle(m_rightArm->getElbow(),1.5f);
        nmrsSetLean1(m_rightArm->getWrist(),0.1f);
        nmrsSetLean2(m_rightArm->getWrist(),-.1f);
        nmrsSetTwist(m_rightArm->getWrist(),1.f);
      }
      else
      {
        m_rightArmState.tick(timeStep);
        m_rightArm->getWrist()->setDesiredLean2(-3.f);
      }
    }
    m_leftLeg->getHip()->setDesiredLean2(-0.15f*localm_randomSpineL2-0.2f);//(m_kneeBendL+ 0.7f*extraSit)*strength);
    m_rightLeg->getHip()->setDesiredLean2(0.15f*localm_randomSpineL2-0.2f);//(m_kneeBendR + 0.7f*extraSit)*strength);
    m_leftLeg->getHip()->setDesiredLean1(0.6f*m_kneeBendL);//(m_kneeBendL+ 0.7f*extraSit)*strength);
    m_rightLeg->getHip()->setDesiredLean1(1.f*m_kneeBendR);//(m_kneeBendR + 0.7f*extraSit)*strength);
    m_leftLeg->getHip()->setDesiredTwist(0.15f*localm_randomTwist);//(m_kneeBendL+ 0.7f*extraSit)*strength);
    m_rightLeg->getHip()->setDesiredTwist(-0.35f*localm_randomTwist);//(m_kneeBendR + 0.7f*extraSit)*strength);
    m_leftLeg->getKnee()->setDesiredAngle(-1.f*m_kneeBendL);      
    m_rightLeg->getKnee()->setDesiredAngle(-1.f*m_kneeBendR);   
    m_leftLeg->getAnkle()->setDesiredLean1(-0.5f);
    m_rightLeg->getAnkle()->setDesiredLean1(-0.55f);

    return eCBUTaskComplete;
  }

  void NmRsCBUSDCatchFall::ArmState::armIK(const rage::Vector3 &target, float armTwist, float dragReduction, const rage::Vector3 *vel)
  {
    if (m_leftArmBool)
    {
      m_character->C_LimbIK(m_parent->m_leftArm, 1.f, 1.f, false, &target, &armTwist, &dragReduction, vel);// NULL, &straightness, &maxSpeed);       
    }
    else
    {
      m_character->C_LimbIK(m_parent->m_rightArm, -1.f, 1.f, false, &target, &armTwist, &dragReduction, vel);// NULL, &straightness, &maxSpeed);       
    }
  }

  void NmRsCBUSDCatchFall::ArmState::wristIK(const rage::Vector3 &target, const rage::Vector3 &normal)
  {
    if (m_leftArmBool)
    {
      m_character->leftWristIK(target, normal);
    }
    else
    {
      m_character->rightWristIK(target, normal);
    }
  }

  void NmRsCBUSDCatchFall::ArmState::enter(const ArmSetup *armSetup, bool leftArm, char *armMask)
  {
    m_armSetup = armSetup;
    m_leftArmBool = leftArm;
    m_armMask[0] = armMask[0]; m_armMask[1] = armMask[1];
    m_onBackRandomL1 = m_character->getRandom().GetRanged(-0.4f, 0.f);
    m_onBackRandomL2 = m_character->getRandom().GetRanged(0.f, 0.5f);
    m_maxElbowAngleRandom = m_character->getRandom().GetRanged(0.7f, 1.3f);
  }

  void NmRsCBUSDCatchFall::ArmState::tick(float /*timeStep*/)
  {
    float armTwist = 0.5f - m_parent->m_forwardsAmount; // rotating elbows in on forwards and out on backwards
    rage::Vector3 shoulderPos = m_armSetup->getClavicle()->getJointPosition();
    rage::Vector3 target = m_parent->m_probeEnd;
    rage::Vector3 backLine = m_character->m_COMvel;//m_parent->m_spine->getSpine3Part()->getPosition()- m_parent->m_spine->getSpine0Part()->getPosition();
    m_character->levelVector(backLine);
    backLine.Normalize();

    // If it the [leftArm and the rightSide] or [the rightArm and the Leftside].
    if((m_leftArmBool&&(m_parent->m_leftRight<0.f))||(!m_leftArmBool&&(m_parent->m_leftRight>0.f)))
    {
      target = shoulderPos + backLine*0.3f;//+ upTemp;//-m_parent->m_groundNormal*1.f + upTemp + backLine*6.f;
    }
    else
    {
      // The arm furthest away from the wall.
      target = shoulderPos + backLine*0.5;// - upTemp;//-m_parent->m_groundNormal*1.f + upTemp + backLine*6.f;
    }
    rage::Vector3 velo = m_character->m_COMvel;//m_armSetup->getClaviclePart()->getLinearVelocity();
    armIK(target, armTwist, 1.f, &velo);
    m_character->matchClavicleToShoulder(m_armSetup->getClavicle(), m_armSetup->getShoulder());
    wristIK(target, m_parent->m_groundNormal);
  }

#if ART_ENABLE_BSPY
  void NmRsCBUSDCatchFall::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_legsStiffness, true);
    bspyTaskVar(m_parameters.m_torsoStiffness, true);
    bspyTaskVar(m_parameters.m_armsStiffness, true);
    bspyTaskVar(m_parameters.m_backSideFront,true);
    bspyTaskVar(m_fallDirection, false);
    bspyTaskVar(m_floorVel, false);
    bspyTaskVar(m_probeEnd, false);
    bspyTaskVar(m_groundNormal, false);
    bspyTaskVar(m_forwardsAmount,false);
    bspyTaskVar(m_kneeBendL,false);
    bspyTaskVar(m_kneeBendR,false);
    bspyTaskVar(m_timer,false);
    bspyTaskVar(m_leftRight,false);
    bspyTaskVar(m_leftArmState.m_leftArmBool, false);
    bspyTaskVar(m_rightArmState.m_leftArmBool, false);
  }
#endif // NM_RS_ENABLE_SPY
}
