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
 * Flinch: Brace the arms and upper body against a force coming from an specified point
 * 
 */


#include "NmRsInclude.h"
#include "NmRsCBU_Flinch.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "NmRsEngine.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_SpineTwist.h" 
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_DynamicBalancer.h"


namespace ART
{
  NmRsCBUFlinch::NmRsCBUFlinch(ART::MemoryManager* services) : CBUTaskBase(services, bvid_upperBodyFlinch)
  {
    initialiseCustomVariables();
  }

  NmRsCBUFlinch::~NmRsCBUFlinch()
  {
  }

  void NmRsCBUFlinch::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_braceHead = false;
    m_angleFront = 0.f;
    m_braceHeadTimer = 0.f;
    m_braceFrontTimer = 0.f;
    m_armCollisionTimer = 0.0f;
    m_armCollisionDisabled = false;
    m_relaxTime = 0.f;
    m_relaxPeriod = 0.4f;//mmmmbcr 0.0001f;
    m_controlStiffnessStrengthScale = 0.01f;//mmmmbcr 1.f;
    m_turnTowardsMultiplier = 1;
    m_lookBackAtDoomTimer = 0.f;
    m_wasFromBackBehind = false;
    m_wasFromFrontBehind = false;
    m_armsBracingHead = false;
  }

  void NmRsCBUFlinch::onActivate()
  {
    Assert(m_character);

    m_rightSide = true;

    m_legStraightnessMod = m_character->getRandom().GetRanged(-0.1f,0.0f);
    m_noiseSeed = m_character->getRandom().GetRanged(0.0f, 4000.0f);
    m_turnTowardsMultiplier = m_character->getRandom().GetRanged(0,1);

    // gravity comp
    m_body->setOpposeGravity(1.0f, bvmask_UpperBody);

    NmRsCBUSpineTwist* spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
    Assert(spineTwistTask);
    spineTwistTask->setSpineTwistVelX(0.f);
    spineTwistTask->setSpineTwistVelY(0.f);
    spineTwistTask->setSpineTwistVelZ(0.f);
    spineTwistTask->activate();

    if (m_parameters.m_useHeadLook)
    {
      NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      Assert(headLookTask);
      headLookTask->updateBehaviourMessage(NULL);
      headLookTask->activate();
    }

    chooseFlinchType();
    if (m_parameters.m_applyStiffness)
      controlStiffness_entry();
  }

  void NmRsCBUFlinch::onDeactivate()
  {
    Assert(m_character);

    // gravity comp
    m_body->setOpposeGravity(0.0f, bvmask_UpperBody);

    //set all the effectors back to normal 
    m_body->resetEffectors(kResetCalibrations | kResetAngles, bvmask_UpperBody);

    NM_RS_DBG_LOGF(L"Flinch Exit");

    if (m_armCollisionDisabled)
      setLowerArmCollision(true);

    // Set head friction multiplier to its default value since it could have been modified by braceHead().
    m_character->setFrictionPreScale(1.0f, bvmask_Head);

    initialiseCustomVariables();

    //Deactivate Child behaviours activated in er ::activate
    m_cbuParent->m_tasks[bvid_spineTwist]->deactivate();
    m_cbuParent->m_tasks[bvid_headLook]->deactivate();
  }

  CBUTaskReturn NmRsCBUFlinch::onTick(float timeStep)
  {
    NM_RS_DBG_LOGF(L"Flinch During %.4f", m_parameters.m_pos.x);

    if (m_armCollisionDisabled)
    {
      m_armCollisionTimer -= timeStep;
      if (m_armCollisionTimer <= 0.0f)
      {
        setLowerArmCollision(true);
      }
    }

    chooseFlinchType();

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    if (dynamicBalancerTask->isActive())
    {
      rage::Vector3 direction = m_parameters.m_pos;
      direction = direction - m_character->m_COM;

      // turn to the side first. to avoid the walking backwards away from the target behaviour.
      if (rage::Abs(m_angleFront) > 2.8f)
      {
        rage::Matrix34 comMt = m_character->m_COMTM;
        direction = comMt.a;
        direction.Scale(rage::Sign(m_angleFront));
      }

      direction.Normalize();
      if ((m_turnTowardsMultiplier)&& rage::Abs(m_parameters.m_turnTowards))
      {
        direction.Scale((float)m_parameters.m_turnTowards);
        direction.Normalize();
        dynamicBalancerTask->useCustomTurnDir(true, direction);
      }
      else
      {
        dynamicBalancerTask->useCustomTurnDir(false, direction);
      }

      if (m_braceHead)
      {
        NM_RS_DBG_LOGF(L"random leg straightness = %.2f", m_legStraightnessMod);
        dynamicBalancerTask->setLegStraightnessModifier(m_legStraightnessMod);
      }
      else
      {
        dynamicBalancerTask->setLegStraightnessModifier(0.f);
      }
    }

    NM_RS_DBG_LOGF(L"Body Stiffness = %.4f", m_parameters.m_bodyStiffness);

    //m_character->setBodyStiffness(m_bodyStiffness, m_bodyDamping,"ub");

    // do the new Hit once.
    if (m_parameters.m_newHit)
    {
      if (m_parameters.m_applyStiffness)
        controlStiffness_entry();
      m_parameters.m_newHit = false;
    }
    else
    {
      if (m_parameters.m_applyStiffness)
        controlStiffness_tick(timeStep);
    }

    // Call Standalone behaviours in order of transitions
    if (m_braceHead)//Head
    {
      m_braceHeadTimer += m_character->getLastKnownUpdateStep();
      m_braceFrontTimer = 0.f;
      if (!m_parameters.m_dontBraceHead)
        braceHead();
    }
    else //Front
    {
      m_braceHeadTimer = 0.f;
      m_braceFrontTimer += m_character->getLastKnownUpdateStep();
      braceFront();
    }

    return eCBUTaskComplete;
  }


  void NmRsCBUFlinch::chooseFlinchType()
  {
    // based on doomdirection choose the places for the hands to defend
    // want: crossed hands, nearest arm closest to doom, at different radius

    m_doompos = m_parameters.m_pos;
    m_doomdirection = m_doompos - m_character->m_COM;

    //to handle cases where object from totally behind him : was always switching between modes
    rage::Vector3 upVector = m_character->getUpVector();
    rage::Vector3 direction =m_doomdirection;
    direction.Normalize();
    bool fromFrontBehind = false;
    bool fromBackBehind = false;
    bool wasFromBehind =  m_wasFromBackBehind || m_wasFromFrontBehind;//used later to keep and initialize m_rightSide
    if (direction.Dot(upVector)<-0.96f && m_character->m_COMTM.b.Dot(upVector)>0.7f)//mostly from behind
    {
      rage::Vector3 charDir = m_character->m_COMTM.c;
      m_character->levelVector(charDir);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "flinch", direction.Dot(charDir));
      bspyScratchpad(m_character->getBSpyID(), "flinch", direction);
      bspyScratchpad(m_character->getBSpyID(), "flinch", charDir);
#endif
      fromFrontBehind = direction.Dot(charDir)<0.f || m_wasFromFrontBehind;
      fromBackBehind = !fromFrontBehind;
      if(!(m_wasFromBackBehind || m_wasFromFrontBehind))
      {//initialize
        m_wasFromBackBehind = fromBackBehind;
        m_wasFromFrontBehind = fromFrontBehind;
      }
      else
      {//overwrite to keep history
        fromFrontBehind = m_wasFromFrontBehind;
        fromBackBehind = m_wasFromBackBehind;
      }
    }
    else
    {
      m_wasFromBackBehind  = false;
      m_wasFromFrontBehind = false;
    }

#if ART_ENABLE_BSPY
    if (m_wasFromBackBehind || m_wasFromFrontBehind || fromBackBehind || fromFrontBehind)
    {    
      bspyScratchpad(m_character->getBSpyID(), "flinch", rage::AcosfSafe(direction.Dot(upVector)));
      bspyScratchpad(m_character->getBSpyID(), "flinch", fromFrontBehind);
      bspyScratchpad(m_character->getBSpyID(), "flinch", fromBackBehind);
      bspyScratchpad(m_character->getBSpyID(), "flinch", fromFrontBehind);
      bspyScratchpad(m_character->getBSpyID(), "flinch", m_wasFromBackBehind);
      bspyScratchpad(m_character->getBSpyID(), "flinch", m_wasFromFrontBehind);
    }
#endif
    m_character->levelVector(m_doomdirection);
    m_doomdirection.Normalize();

    rage::Matrix34 comM = m_character->m_COMTM;

    // NM_RS_CBU_DRAWCOORDINATEFRAME(5,comM);

    comM.Inverse();
    rage::Vector3 flinP = m_doompos;
    flinP.Dot(comM);

    //NM_RS_CBU_DRAWPOINT(flinP,2,rage::Vector3(1.0,0.0,0.0));

    NM_RS_DBG_LOGF(L" flinP.x = %.5f", flinP.x);

    m_angleFront = rage::ArcTangent(flinP.x,-flinP.z);

    NM_RS_DBG_LOGF(L" angleFront.x = %.5f", m_angleFront);

    // decide Flinch Type
    if (m_parameters.m_protectHeadToggle)
    {
      m_braceHead = true;
    }
    else
    {
      if ((rage::Abs(m_angleFront) < 1.5f && !fromBackBehind) || fromFrontBehind)
      {
        m_braceHead = false;
      }
      else if (rage::Abs(m_angleFront) > 1.6f || fromBackBehind)
      {
        m_braceHead = true;
      }
    }


    if (!wasFromBehind || !(fromBackBehind || fromFrontBehind))
    {//object was not fromBehind or is not from behind
      if (m_angleFront > 0.4f) //(rightdist<leftdist) 
      {
        if (m_rightSide == false)
        {
          setLowerArmCollision(false);
          m_armCollisionTimer = 1.5f;
        }
        m_rightSide = true;
      }
      else if (m_angleFront < -0.4f)
      {
        if (m_rightSide == true)
        {
          setLowerArmCollision(false);
          m_armCollisionTimer = 1.5f;
        }
        m_rightSide = false;
      }
    }
    else
    {
      //will keep former value of m_rightSide
      //to avoid switch of sign : used below to bend spine
      m_angleFront = rage::Abs(m_angleFront);
      if(m_rightSide == false){m_angleFront = -m_angleFront;}
    }

    NM_RS_DBG_LOGF(L"rightSide = %s", m_rightSide?L"true":L"false");

    float offsetSway = 0.75f * (m_character->getEngine()->perlin3((float)m_character->getID(),(m_braceHeadTimer + m_braceFrontTimer) * 0.75f, m_noiseSeed * 0.5f)-0.5f);

    rage::Vector3 doomSide(m_doomdirection);
    doomSide.Cross(m_character->m_gUp);
    doomSide *= offsetSway;

    NmRsCBUSpineTwist* spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
    Assert(spineTwistTask);
    rage::Vector3 twistPos = m_doompos;
    if (m_braceHead)
    {
      rage::Vector3 twistTo = getSpine()->getSpine3Part()->getPosition();
      twistTo -= m_doomdirection;
      twistPos = twistTo;
    }
    spineTwistTask->setSpineTwistPos(twistPos + doomSide);
    //spineTwistTask->setSpineTwistStiffness(m_parameters.m_bodyStiffness);//will act different in LUA if studio not 60fps

    //we can turn it on but if this parameter changes on the fly we don't turn it off
    //as other behaviours may be using it - mmmmmtodo needs a request deactivate like balancer.
    if (m_parameters.m_useHeadLook)
    {
      NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      Assert(headLookTask);
      if (!headLookTask->isActive())
      {
        headLookTask->updateBehaviourMessage(NULL);
        headLookTask->activate();
      }

#if 0//Clearer implementation of headlook //mmmmtodo put this code in after testing
      rage::Vector3 lookPos = m_doompos - (0.75f*doomSide);//m_doomPos + [0m:0.279375m] left/right (looking at target).    doomside = [0m:0.75m]*left/right of m_doom
      if (m_braceHead) // look at center of feet
      {
        lookPos = getLeftLeg()->getFoot()->getPosition();
        lookPos += getRightLeg()->getFoot()->getPosition();
        lookPos.Scale(0.5f);
        lookPos -= (0.75f*doomSide);//centreOfFeet + [0m:0.279375m] left/right (looking at target).    doomside = [0m:0.75m]*left/right of m_doom
      }
      else if (m_parameters.m_headLookAwayFromTarget)
      {
        doomSide.Normalize();
        doomSide.Scale(10.0f);
        lookPos = m_doompos + (0.25f*doomSide);//m_doomPos + 2.5m left/right (looking at target).

        m_lookBackAtDoomTimer -= m_character->getLastKnownUpdateStep();

        // Actually look to the other side of doom
        if (m_lookBackAtDoomTimer<0.f)
        {
          lookPos = m_doompos - (0.75f*doomSide);//m_doomPos + 7.5m left/right (looking at target).
          if(m_lookBackAtDoomTimer<-0.2f)
          {
            //restart timer
            m_lookBackAtDoomTimer = m_character->getRandom().GetRanged(1.5f,4.0f);
          }
        }
      }

      headLookTask->m_parameters.m_pos = lookPos;
#else
      rage::Vector3 lookPos = m_doompos;
      if (m_braceHead)
      {
        // Make the head look target relative to the chest so that the Agent looks straight down at all times.
        // This is to make the arms pass the head clearly on it's way towards the neck.
        // Once both hands have successfully passed the head, m_armsBracingHead is set to true by the braceHead() function and
        // the Agent is allowed to look elsewhere.
        if (!m_armsBracingHead && (m_parameters.m_useLeft || m_parameters.m_useRight))
        {
          doomSide.Zero();
          rage::Matrix34 spine3Mat;
          getSpine()->getSpine3Part()->getMatrix(spine3Mat);
          const rage::Vector3 spine3FrontDir = -spine3Mat.c;
          const float forwardGain = 0.25f;
          lookPos = getSpine()->getSpine3Part()->getPosition();
          lookPos.AddScaled(spine3FrontDir, forwardGain);

          // Prevent Agent from looking sideways when it's looking and facing downwards
          // i.e. when horizontal line of eyes is indefinite relative to the horizon.
          headLookTask->m_parameters.m_eyesHorizontal = false;
        }
        else // Look at center of feet.
        {
          lookPos = getLeftLeg()->getFoot()->getPosition();
          lookPos += getRightLeg()->getFoot()->getPosition();
          lookPos.Scale(0.5f);

          // Enable look at parameter that is disabled when Agent braces its head.
          headLookTask->m_parameters.m_eyesHorizontal = true;
        }
      }
      else if (m_parameters.m_headLookAwayFromTarget)
      {
        doomSide.Normalize();
        doomSide.Scale(10.f);
        lookPos = m_doompos + doomSide;

        m_lookBackAtDoomTimer -= m_character->getLastKnownUpdateStep();

        // look back at the doom
        if (m_lookBackAtDoomTimer<0.f)
        {
          lookPos = m_doompos;
          if(m_lookBackAtDoomTimer<-0.2f)
          {
            //restart timer
            m_lookBackAtDoomTimer = m_character->getRandom().GetRanged(1.5f,4.0f);
          }
        }
      }

      doomSide *= 0.75f;

      headLookTask->m_parameters.m_pos = lookPos - doomSide;
#endif
      headLookTask->m_parameters.m_stiffness = m_parameters.m_bodyStiffness;
      headLookTask->m_parameters.m_damping = m_parameters.m_bodyDamping;
    }

  }


  /**
  * Other States used by Flinch
  */

  // Hunch shoulders, bend back, twistaway(Spine Twist), hands above head, head away from DOOM (Head Avoid Direcion)
  void NmRsCBUFlinch::braceHead()
  {
    NM_RS_DBG_LOGF(L"Brace Head");

    // sway away from the IMPENDING DOOM

    rage::Matrix34 comTM = m_character->m_COMTM;
    //NM_RS_CBU_DRAWCOORDINATEFRAME(4,comTM);
    rage::Vector3 sideV = comTM.a;
    rage::Vector3 toDoom = m_doompos - m_character->m_COM;
    toDoom.Normalize();
    float sideVdotDoom = sideV.Dot(toDoom);   
    float backLean2 = rage::Sinf(sideVdotDoom);
    NM_RS_DBG_LOGF(L"backLean2 = %.3f",backLean2);

    float bendamount = 1.2f;

    getSpineInputData()->getSpine0()->setDesiredLean1(0.1f*bendamount);
    getSpineInputData()->getSpine1()->setDesiredLean1(0.2f*bendamount);
    getSpineInputData()->getSpine2()->setDesiredLean1(0.3f*bendamount);
    getSpineInputData()->getSpine3()->setDesiredLean1(0.4f*bendamount);

    getSpineInputData()->getSpine0()->setDesiredLean2(0.2f*backLean2);
    getSpineInputData()->getSpine1()->setDesiredLean2(0.3f*backLean2);
    getSpineInputData()->getSpine2()->setDesiredLean2(0.3f*backLean2);
    getSpineInputData()->getSpine3()->setDesiredLean2(0.2f*backLean2);

    // Is there at least one arm to brace the head.
    if (m_parameters.m_useLeft || m_parameters.m_useRight)
    {
      // Make Agent successfully achieve tricky head brace arm pose with its hands behind the head, resting on neck.

      // The following direction and position are used to test whether the arm passed clearly by the head so that the wrist can be clamped on the neck and
      // to stop look at behaviour from looking at the target in front of Agent's chest.
      rage::Vector3 spine3ToHeadDir = getSpine()->getUpperNeck()->getJointPosition() - getSpine()->getSpine3()->getJointPosition();
      spine3ToHeadDir.Normalize();
      const rage::Vector3 spine3JointPos = getSpine()->getSpine3()->getJointPosition();

      // Friction multiplier for head is decreased when either hand is in contact with it.
      // As a result hand should nicely slide on head's surface rather than stick in weird orientation.
      const bool isEitherHandCollidedWithOwnCharacter = (getLeftArm()->getHand()->collidedWithOwnCharacter()) || (getRightArm()->getHand()->collidedWithOwnCharacter());
      const float headFrictionMltp = (isEitherHandCollidedWithOwnCharacter) ? (0.001f) : (1.0f);
      m_character->setFrictionPreScale(headFrictionMltp, bvmask_Head);

      bool leftHandPassedHead  = true;
      bool rightHandPassedHead = true;

      if (m_parameters.m_useLeft)
      {
        leftHandPassedHead = armBracingHeadPose(getLeftArm(), getLeftArmInputData(), spine3ToHeadDir, spine3JointPos);
      }

      if (m_parameters.m_useRight)
      {
        rightHandPassedHead = armBracingHeadPose(getRightArm(), getRightArmInputData(), spine3ToHeadDir, spine3JointPos);
      }

      // Both hands passed the head on its way towards the neck?
      m_armsBracingHead = leftHandPassedHead && rightHandPassedHead;

    } // End of if (m_parameters.m_useLeft || m_parameters.m_useRight).

    // if the balance has failed assume we are on the ground -> then we want to do a foetal with the legs. 
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    if ((dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK))
    {
      getLeftLegInputData()->getHip()->setDesiredLean1(1.5f);
      getLeftLegInputData()->getKnee()->setDesiredAngle(-2.f);

      getRightLegInputData()->getHip()->setDesiredLean1(1.5f);
      getRightLegInputData()->getKnee()->setDesiredAngle(-2.f);
    }
  }

  bool NmRsCBUFlinch::armBracingHeadPose(NmRsHumanArm *limb, NmRsArmInputWrapper* input, const rage::Vector3 &spine3ToHeadDir, const rage::Vector3 &spine3JointPos)
  {
    Assert(limb);

    // Test whether the hand has clearly passed the head on its way towards the neck, then modify effector angles for the wrist to clamp it to the neck.
    const rage::Vector3 wristJointPos = limb->getWrist()->getJointPosition();
    rage::Vector3 spine3ToWristDir = wristJointPos - spine3JointPos;
    spine3ToWristDir.Normalize();
    const bool handPassedHead = (spine3ToWristDir.Dot(spine3ToHeadDir) > 0.85f) ? true : false; // Tunable parameter.

    // Setup limb effector angles.
    const float wristLean1 = 1.0f; // Tunable parameter.
    const float wristLean2 = (handPassedHead) ? (-0.5f) : (-1.7f); // Tunable parameters.

    input->getWrist()->setDesiredLean1(wristLean1);
    input->getWrist()->setDesiredLean2(wristLean2);

    input->getClavicle()->setDesiredTwist(0.035f);
    input->getClavicle()->setDesiredLean1(-0.375f);
    input->getClavicle()->setDesiredLean2(-0.07f);

    input->getShoulder()->setDesiredTwist(-0.495f);
    input->getShoulder()->setDesiredLean1(1.514f);
    input->getShoulder()->setDesiredLean2(-0.396f);

    input->getElbow()->setDesiredAngle(2.4f);

    return handPassedHead;
  }

  void NmRsCBUFlinch::braceFront()
  {
    rage::Vector3 vertv; 
    rage::Vector3 offsetnorm;

    float boundHeight =  rage::Clamp(m_character->m_gUp.Dot(m_doompos) - m_character->m_gUp.Dot(m_character->m_COM),-.5f,2.0f);

    float timerScaled = m_braceFrontTimer * 0.75f;

    float pFB = m_parameters.m_noiseScale*(m_character->getEngine()->perlin3(timerScaled, m_noiseSeed, (float)m_character->getID())-0.5f);
    float pFS = m_parameters.m_noiseScale*(m_character->getEngine()->perlin3(timerScaled, (float)m_character->getID(),m_noiseSeed)-0.5f);
    float pSB = m_parameters.m_noiseScale*(m_character->getEngine()->perlin3((float)m_character->getID(),timerScaled, m_noiseSeed)-0.5f);

    //float bSway = m_noiseScale*(m_character->getEngine()->perlin3((float)m_character->getID(),timerScaled * 0.5f, m_noiseSeed * 2.0f)-0.5f);

    NM_RS_DBG_LOGF(L"plnA = %.3f",pFB);
    NM_RS_DBG_LOGF(L"plnA = %.3f",pFS);
    NM_RS_DBG_LOGF(L"plnA = %.3f",pSB);

    NM_RS_DBG_LOGF(L"boundHeight = %.3f",boundHeight);

    float rad = 0.38f; //32+.08*(1-1/(boundHeight+.2)) //[.32 .4)
    vertv = m_character->m_gUp;

    offsetnorm.Cross(m_doomdirection,vertv);
    offsetnorm.Scale(m_parameters.m_hand_dist_lr+pFB);

    // create a vector in the direction of the incoming doom
    //rage::Vector3  todoom;
    //todoom.Set(m_xd,m_yd,m_zd);
    //m_character->levelVector(todoom);
    //todoom -= m_character->m_COM;
    //m_character->levelVector(todoom);
    //todoom.Normalize();

    // adjust for the height of the incoming Impending Doom from the end of the universe - spongy
    vertv.Scale(boundHeight);
    rage::Vector3 todoom = m_doomdirection + vertv;
    todoom.Normalize();
    vertv.Normalize();

    // initial point on a sphere around the head to defend
    // scale the radius depending on the height
    // The position to defend
    rage::Vector3 defpos = getSpine()->getSpine3Part()->getPosition();
    todoom.Scale(rad);
    defpos += todoom;

    // and the distance apart for the hands
    vertv.Scale(m_parameters.m_hand_dist_vert + pFS);
    todoom.Scale(-m_parameters.m_hand_dist_fb+pSB);

    rage::Vector3 righttarget= defpos - offsetnorm;
    rage::Vector3 lefttarget= defpos + offsetnorm;
    // change things depending on what side it is. 

    if (m_rightSide) 
    {
      vertv.Scale(-1.f);
      todoom.Scale(-1.f);
    }

    righttarget -= todoom;
    righttarget -= vertv;

    lefttarget += todoom;
    lefttarget += vertv;

    todoom.Normalize();
    vertv.Normalize();
    offsetnorm.Normalize();

    // NM_RS_CBU_DRAWVECTOR(lefttarget,m_doomdirection);
    // NM_RS_CBU_DRAWVECTOR(lefttarget,vertv);
    // NM_RS_CBU_DRAWVECTOR(lefttarget,todoom);
    // NM_RS_CBU_DRAWVECTOR(lefttarget,vertv);
    //NM_RS_CBU_DRAWVECTOR(lefttarget,offsetnorm);


    // The Back Lean1: away from DOOM
    //      m_character->applySpineLean(m_backBendAmount,0.0f);
    //m_backBendAmount = 0.f;

    // HD
    // changed this so that the back angles are determined by where the threat is
    // eg. he doesn't lean backwards if he's trying to twist and guard to the side
    float angleVal = rage::Clamp(m_angleFront, -1.0f, 1.0f);
    float blendBackBendL1 = (1.0f - rage::Abs(angleVal)) * m_parameters.m_backBendAmount;
    float blendBackBendL2 = (angleVal) * m_parameters.m_backBendAmount * -0.5f;

    getSpineInputData()->getSpine0()->setDesiredLean1(0.1f * blendBackBendL1);
    getSpineInputData()->getSpine1()->setDesiredLean1(0.2f * blendBackBendL1);
    getSpineInputData()->getSpine2()->setDesiredLean1(0.3f * blendBackBendL1);
    getSpineInputData()->getSpine3()->setDesiredLean1(0.4f * blendBackBendL1);

    getSpineInputData()->getSpine0()->setDesiredLean2(0.2f * blendBackBendL2);
    getSpineInputData()->getSpine1()->setDesiredLean2(0.3f * blendBackBendL2);
    getSpineInputData()->getSpine2()->setDesiredLean2(0.3f * blendBackBendL2);
    getSpineInputData()->getSpine3()->setDesiredLean2(0.2f * blendBackBendL2);

    if ( m_braceFrontTimer < 0.1  && !m_parameters.m_dontBraceHead)
    {
      if (m_parameters.m_useRight)
      {
        getRightArmInputData()->getShoulder()->setDesiredTwist(-1.5f);
        getRightArmInputData()->getShoulder()->setDesiredLean1(0.85f);
        getRightArmInputData()->getShoulder()->setDesiredLean2(-.5f);
        getRightArmInputData()->getElbow()->setDesiredAngle(0.3f);
        getRightArmInputData()->getWrist()->setDesiredLean2(0.f);
        getRightArmInputData()->getWrist()->setDesiredLean1(.1f);

        getRightArmInputData()->getWrist()->blendToZeroPose((NmRsEffectorBase*)getRightArm()->getWrist(), 0.35f);
      }
      if (m_parameters.m_useLeft)
      {
        getLeftArmInputData()->getShoulder()->setDesiredTwist(-1.5f);
        getLeftArmInputData()->getShoulder()->setDesiredLean1(0.995f);
        getLeftArmInputData()->getShoulder()->setDesiredLean2(-.5f);
        getLeftArmInputData()->getElbow()->setDesiredAngle(.6f);
        getLeftArmInputData()->getWrist()->setDesiredLean2(0.f);
        getLeftArmInputData()->getWrist()->setDesiredLean1(.2f);

        getLeftArmInputData()->getWrist()->blendToZeroPose((NmRsEffectorBase*)getLeftArm()->getWrist(), 0.35f);
      }
    }
    else
    {
      //The Arms+Wrists: IK
      float twist = 0.05f;
      float clavAdjust = -0.5f;
      float dragReduction = 0.f;
      bool useActualAngles = false;

      rage::Vector3 dPos = getSpine()->getSpine3Part()->getPosition();
      dPos = dPos - m_doompos;
      dPos.NormalizeSafe();

      if ((m_character->getCharacterConfiguration().m_leftHandState <= CharacterConfiguration::eHS_Pistol)&&(m_parameters.m_useLeft))
      {
        float ikTwist = twist;
        float clav = clavAdjust;
        if (m_rightSide)
        {
          twist = -0.1f;
          clav = -0.3f;
        }

        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(-1);
		NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
        ikInputData->setTarget(lefttarget);
        ikInputData->setTwist(ikTwist);
        ikInputData->setDragReduction(dragReduction);
        ikInputData->setWristTarget(lefttarget);
        ikInputData->setWristNormal(dPos);
        ikInputData->setWristUseActualAngles(useActualAngles);
        ikInputData->setMatchClavicle(kMatchClavicle);
		getLeftArm()->postInput(ikInput);

		getLeftArmInputData()->getClavicle()->setDesiredLean2(clav);

        getLeftArmInputData()->getWrist()->setDesiredLean1(0.2f);
        getLeftArmInputData()->getWrist()->setDesiredLean2(0.0f);
        getLeftArmInputData()->getWrist()->blendToZeroPose((NmRsEffectorBase*)getLeftArm()->getWrist(), 0.25f);
	  }

      if ((m_character->getCharacterConfiguration().m_rightHandState <= CharacterConfiguration::eHS_Pistol)&&(m_parameters.m_useRight))
      {
        float ikTwist = twist;
        float clav = clavAdjust;
        if (!m_rightSide)
        {
          ikTwist = -0.1f;
          clav = -0.3f;
        }

        // depressing priority a bit to allow wrist leans to overwrite later on.
        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(-1);
        NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
        ikInputData->setTarget(righttarget);
        ikInputData->setTwist(ikTwist);
        ikInputData->setDragReduction(dragReduction);
        ikInputData->setWristTarget(righttarget);
        ikInputData->setWristNormal(dPos);
        ikInputData->setWristUseActualAngles(useActualAngles);
		ikInputData->setMatchClavicle(kMatchClavicle);
        getRightArm()->postInput(ikInput);

     	getRightArmInputData()->getClavicle()->setDesiredLean2(clav);

        getRightArmInputData()->getWrist()->setDesiredLean1(0.2f);
        getRightArmInputData()->getWrist()->setDesiredLean2(0.0f);
        getRightArmInputData()->getWrist()->blendToZeroPose((NmRsEffectorBase*)getRightArm()->getWrist(), 0.25f);
      }
    }
  }

  void NmRsCBUFlinch::setLowerArmCollision(bool enable)
  {
	  rage::phArticulatedCollider *collider = m_character->getArticulatedWrapper()->getArticulatedCollider();
	  if (enable)
	  {
			m_character->getEngine()->AssignSelfCollisionSet(collider, &m_character->getEngine()->m_DefaultSelfCollisionSet);
	  }
	  else
	  {
			m_character->getEngine()->AssignSelfCollisionSet(collider, &m_character->getEngine()->m_FlinchSelfCollisionSet);	 
	  }
	  m_armCollisionDisabled = !enable;
  }

  void NmRsCBUFlinch::controlStiffness_entry()
  {
    m_relaxTime = 0.f;
    NM_RS_DBG_LOGF(L"WeakenStart");
    float stiffness = 0.01f;
    if (m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl)
    {
      stiffness = 0.5f;
    }

    getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_bodyStiffness*0.5f, 0.5f, bvmask_LowSpine, &stiffness);
    getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_bodyStiffness*0.5f, 0.3f, bvmask_CervicalSpine, &stiffness);

    m_body->setStiffness(m_parameters.m_bodyStiffness*0.5f, 0.5f, bvmask_ArmLeft | bvmask_ArmRight, &stiffness);

    getLeftArmInputData()->getWrist()->setStiffness(m_parameters.m_bodyStiffness*0.5f, 0.3f, &stiffness);
    getRightArmInputData()->getWrist()->setStiffness(m_parameters.m_bodyStiffness*0.5f, 0.3f, &stiffness);
  }

  void NmRsCBUFlinch::controlStiffness_tick(float timeStep)
  {
    m_relaxTime = rage::Min(m_relaxTime + timeStep, m_relaxPeriod);

    float stiffness = rage::Clamp(1.f - ((m_relaxTime + NM_RS_FLOATEPS) / (m_relaxPeriod + NM_RS_FLOATEPS)),0.f,1.f);
    NM_RS_DBG_LOGF(L"Control Stiffness During %.5f",stiffness);

    m_body->setOpposeGravity(1.0f + stiffness, bvmask_LowSpine);
    m_body->setOpposeGravity(1.0f, bvmask_CervicalSpine | (bvmask_ArmLeft & ~ bvmask_HandLeft) | (bvmask_ArmRight & ~ bvmask_HandRight));

    float stiffnessScale = 1.f - 0.5f*stiffness;
    // upper body
    float rampDuration = 0.6f;
    float maxStiffness = 10.f + 10.f/(60.f*timeStep); // 15 up to 20
    NM_RS_DBG_LOGF(L"maxStiffness: %.3f", maxStiffness);

    float loosenessAmount = 0.5f;
    float scale = 1.f*(1.f-loosenessAmount) + m_controlStiffnessStrengthScale*loosenessAmount;
    NM_RS_DBG_LOGF(L"scale: %.5f ", scale);
    float halfWay = 0.5f*(1.f-scale) + 1.f*scale;
    float stiff = 1.2f*scale;

    NM_RS_DBG_LOGF(L"m_bodyStiffness*stiffnessScale: %.5f ", m_parameters.m_bodyStiffness*stiffnessScale);
    NM_RS_DBG_LOGF(L"halfWay: %.5f  ", halfWay);
    NM_RS_DBG_LOGF(L"stiff: %.5f ", stiff);

    getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_bodyStiffness*stiffnessScale, 2*halfWay, bvmask_Full, &stiff);
    stiff = 0.5f*scale;
    m_body->setStiffness(m_parameters.m_bodyStiffness*stiffnessScale, 2*halfWay, bvmask_ArmLeft | bvmask_ArmRight, &stiff);
    stiff = 1.2f*halfWay;
    getLeftArmInputData()->getClavicle()->setStiffness(m_parameters.m_bodyStiffness*stiffnessScale, 2*halfWay, &stiff);
    getRightArmInputData()->getClavicle()->setStiffness(m_parameters.m_bodyStiffness*stiffnessScale, 2*halfWay, &stiff);
    stiff = 0.5f*halfWay;
    getLeftArmInputData()->getWrist()->setStiffness(rage::Min(m_parameters.m_bodyStiffness*stiffnessScale, maxStiffness), 2, &stiff);
    getRightArmInputData()->getWrist()->setStiffness(rage::Min(m_parameters.m_bodyStiffness*stiffnessScale, maxStiffness), 2, &stiff);
    stiff = 0.5f*scale;
    getSpineInputData()->getLowerNeck()->setStiffness(rage::Min(m_parameters.m_bodyStiffness*stiffnessScale, maxStiffness), 2*scale, &stiff);
    getSpineInputData()->getUpperNeck()->setStiffness(rage::Min(m_parameters.m_bodyStiffness*stiffnessScale, maxStiffness), 2*scale, &stiff);

    m_controlStiffnessStrengthScale = rage::Min(1.f, m_controlStiffnessStrengthScale + timeStep/rampDuration);
    NM_RS_DBG_LOGF(L"strengthSCale: ", m_controlStiffnessStrengthScale);
  }


#if ART_ENABLE_BSPY
  void NmRsCBUFlinch::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_pos, true);

    bspyTaskVar(m_parameters.m_hand_dist_vert, true);
    bspyTaskVar(m_parameters.m_hand_dist_lr, true);
    bspyTaskVar(m_parameters.m_hand_dist_fb, true);
    bspyTaskVar(m_parameters.m_bodyStiffness, true);
    bspyTaskVar(m_parameters.m_bodyDamping, true);
    bspyTaskVar(m_parameters.m_backBendAmount, true);
    bspyTaskVar(m_parameters.m_useRight, true);
    bspyTaskVar(m_parameters.m_useLeft, true);
    bspyTaskVar(m_parameters.m_newHit, true);
    bspyTaskVar(m_parameters.m_noiseScale, true);
    bspyTaskVar(m_parameters.m_turnTowards, true);
    bspyTaskVar(m_parameters.m_useHeadLook, true);
    bspyTaskVar(m_parameters.m_headLookAwayFromTarget, true);



    bspyTaskVar(m_doomdirection, false);
    bspyTaskVar(m_doompos, false);

    bspyTaskVar(m_braceHeadTimer, false);
    bspyTaskVar(m_braceFrontTimer, false);
    bspyTaskVar(m_legStraightnessMod, false);
    bspyTaskVar(m_angleFront, false);
    bspyTaskVar(m_noiseSeed, false);
    bspyTaskVar(m_armCollisionTimer, false);
    bspyTaskVar(m_relaxTime, false);
    bspyTaskVar(m_relaxPeriod, false);
    bspyTaskVar(m_controlStiffnessStrengthScale, false);

    bspyTaskVar(m_turnTowardsMultiplier, false);

    bspyTaskVar(m_armCollisionDisabled, false);
    bspyTaskVar(m_braceHead, false);
    bspyTaskVar(m_rightSide, false);

    static const char* failTypeStrings[] =
    {
#define BAL_NAME_ACTION(_name) #_name ,
      BAL_STATES(BAL_NAME_ACTION)
#undef BAL_NAME_ACTION
    };

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    bspyTaskVar(dynamicBalancerTask->m_distKnee, false);
    bspyTaskVar(dynamicBalancerTask->m_heightKnee, false);
    bspyTaskVar(dynamicBalancerTask->m_distHeightKneeRatio, false);
    bspyTaskVar(dynamicBalancerTask->m_dist, false);
    bspyTaskVar(dynamicBalancerTask->m_height, false);
    bspyTaskVar(dynamicBalancerTask->m_distHeightRatio, false);
    bspyTaskVar_StringEnum(dynamicBalancerTask->m_failType, failTypeStrings, false);  
  }
#endif // ART_ENABLE_BSPY
}
