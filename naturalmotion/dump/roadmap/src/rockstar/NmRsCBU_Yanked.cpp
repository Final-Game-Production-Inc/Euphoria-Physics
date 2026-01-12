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
#include "NmRsCBU_Yanked.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_StaggerFall.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_BodyBalance.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_Catchfall.h"

namespace ART
{
  NmRsCBUYanked::NmRsCBUYanked(ART::MemoryManager* services) : CBUTaskBase(services, bvid_yanked)
  {
    initialiseCustomVariables();
  }

  NmRsCBUYanked::~NmRsCBUYanked()
  {
  }

  void NmRsCBUYanked::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
  }

  void NmRsCBUYanked::onActivate()
  {

    Assert(m_character);

    m_YankTimer = 0.f;

    //Yanked Entry
    NM_RS_DBG_LOGF(L"- Yanked Entry");
    m_characterIsFalling = false;

    m_body->resetEffectors(kResetCalibrations | kResetAngles);

    if (m_parameters.m_useHeadLook) 
    {         
      NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      Assert(headLookTask);
      headLookTask->updateBehaviourMessage(NULL);

      headLookTask->m_parameters.m_pos = (m_parameters.m_headLookPos);
      headLookTask->m_parameters.m_stiffness = (11.0f);
      headLookTask->m_parameters.m_damping = (1.0f);
      headLookTask->m_parameters.m_alwaysLook = (true);
      headLookTask->m_parameters.m_instanceIndex = (m_parameters.m_headLookInstanceIndex);
      // .. and activate it
      headLookTask->activate();
    }

    //Override bodyBalance
    getSpineInputData()->getUpperNeck()->setOpposeGravity(0.f);
    getSpineInputData()->getLowerNeck()->setOpposeGravity(0.f);

    getSpineInputData()->getSpine0()->setOpposeGravity(0.f); // just to keep the back from limboing too much
    getSpineInputData()->getSpine1()->setOpposeGravity(0.f);
    getSpineInputData()->getSpine2()->setOpposeGravity(0.f);

    // try to stop hunching shoulders
    getLeftArmInputData()->getClavicle()->setOpposeGravity(0.f);
    getRightArmInputData()->getClavicle()->setOpposeGravity(0.f);

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    dynamicBalancerTask->activate();
    if (dynamicBalancerTask->isActive())
    {
      dynamicBalancerTask->setOpposeGravityAnkles(1.f);
      dynamicBalancerTask->setOpposeGravityLegs(1.f);
      dynamicBalancerTask->setLowerBodyGravityOpposition(m_body);
    }

    //For Look at type randomness
    //m_parameters.m_headLookAtVelProb 0 = no look at vel. >1 = always look at vel
    //if negative (-1) m_parameters.m_headLookAtVelProb set to m_lookAtRandom i.e. randomizeed
    m_lookAtRandom = m_character->getRandom().GetRanged(0.0f, 1.0f); //Set here only        
    m_lookAtTimer = 0.f; //Force lookAt to be randomized on tick
    //For Turn type randomness
    m_turnLeft = m_character->getRandom().GetBool();
    m_turnThreshold = m_character->getRandom().GetRanged(m_parameters.m_turnThresholdMin,m_parameters.m_turnThresholdMax);

    m_rollOverTimer = -0.1f;
    m_hulaTimer = m_character->getRandom().GetRanged(-0.01f,m_parameters.m_hulaPeriod);
    m_rollOverPeriod  = 1.5f;
    m_reachTimer = 0.4f;
    m_hulaState = m_character->getRandom().GetRanged(0,3);
    m_hulaDirection = m_character->getRandom().GetBool()?  -1: 1; 
    m_stepsLeftThisCycle = m_character->getRandom().GetRanged(1,7);
    m_stepsThisCycle = m_stepsLeftThisCycle;
    m_numStepsAtStart = dynamicBalancerTask->getNumSteps();

    //for safety initialise stiffness/damping variables
    m_armStiffness = m_parameters.m_armStiffnessStart;
    m_armDamping = m_parameters.m_armDampingStart;
    m_spineStiffness = m_parameters.m_spineStiffnessStart;
    m_spineDamping = m_parameters.m_spineDampingStart;
    m_lastFootState = NmRsCBUDynBal_FootState::kNotStepping;
    m_spineBendDir = -1.f;
    if (m_character->getRandom().GetBool())
      m_spineBendDir *= -1.f;
    if (m_character->getRandom().GetBool())
      m_hipPitch = -m_parameters.m_hipPitchForward;
    else
      m_hipPitch = m_parameters.m_hipPitchBack;
    m_spineTwistDir = 1.f;
    if (m_character->getRandom().GetBool())
      m_spineTwistDir *= -1.f;
    m_turn = (m_character->getRandom().GetBool());
  }

  void NmRsCBUYanked::onDeactivate()
  {
    Assert(m_character);

    //De-activate subTasks
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    if (dynamicBalancerTask->isActive() && m_character->noBehavioursUsingDynBalance())
      dynamicBalancerTask->requestDeactivate();

    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    if (headLookTask->isActive())
      headLookTask->deactivate();

    setFrictionMultipliers(1.f);

  }

  CBUTaskReturn NmRsCBUYanked::onTick(float timeStep)
  {
    if (!m_character->getArticulatedBody())
      return eCBUTaskComplete;

    // check to see if the articulated body is asleep, in which case we are 'stable' and can't / needn't balance
    if (rage::phSleep *sleep = m_character->getArticulatedWrapper()->getArticulatedCollider()->GetSleep())
    {
      if (sleep->IsAsleep())
        return eCBUTaskComplete;
    }

    NM_RS_DBG_LOGF(L"- Yanked During")
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    if (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK)//mmmmtodo add in bcr? 
    {
      if (!m_characterIsFalling)
      {
        NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
        Assert(catchFallTask);
        catchFallTask->updateBehaviourMessage(NULL);// sets values to defaults

        //float defaultBodyStiffness = 9.f;  // for the bodyBalance
        catchFallTask->m_parameters.m_legsStiffness = 8.f;//defaultBodyStiffness * 5.5f/defaultBodyStiffness;
        catchFallTask->m_parameters.m_torsoStiffness = 10.f;//defaultBodyStiffness * 10.f/defaultBodyStiffness;
        catchFallTask->m_parameters.m_armsStiffness = 14.f;//defaultBodyStiffness * 15.f/defaultBodyStiffness;
        catchFallTask->m_comVelRDSThresh = m_parameters.m_comVelRDSThresh;
        catchFallTask->m_parameters.m_useHeadLook = false;
        catchFallTask->m_armReduceSpeed = 1.5f;
        catchFallTask->activate();

        m_characterIsFalling = true;
        setFrictionMultipliers(m_parameters.m_groundFriction);
      }

      Escapologist();//calls onGround and RollOver if active
      OnGround();//mmmmYanked put here or in main tick?          
      HeadLookOrAvoid(timeStep);
    }
    else
    {
      //Yanked Upright          
      NM_RS_DBG_LOGF(L"spine0 angle = %.4f, desired = %.4f", getSpine()->getSpine0()->getActualLean1(), getSpine()->getSpine0()->getDesiredLean1());
      NmRsCBUStaggerFall* staggerFallTask = (NmRsCBUStaggerFall*)m_cbuParent->m_tasks[bvid_staggerFall];
      Assert(staggerFallTask);
      float lowerBodyStiffness = m_parameters.m_lowerBodyStiffness;
      //Bend at hips
      dynamicBalancerTask->setHipPitch(m_hipPitch);
      getLeftLeg()->getFoot()->setFrictionMultiplier(m_parameters.m_footFriction);
      getRightLeg()->getFoot()->setFrictionMultiplier(m_parameters.m_footFriction);
      if (!staggerFallTask->isActive())
      {
        //legStiffnesses
        //ramp down leg stiffnesses over time
        if (m_YankTimer >= m_parameters.m_timeStartEnd + m_parameters.m_rampTimeToEndValues)
        {
          lowerBodyStiffness = m_parameters.m_lowerBodyStiffnessEnd;
        }
        else if (m_YankTimer >= m_parameters.m_timeStartEnd)
        {
          float t = (m_YankTimer - m_parameters.m_timeStartEnd)/m_parameters.m_rampTimeToEndValues;
          lowerBodyStiffness = m_parameters.m_lowerBodyStiffness + t*(m_parameters.m_lowerBodyStiffnessEnd - m_parameters.m_lowerBodyStiffness);
        }


        //Per step reduction
        int numSteps = dynamicBalancerTask->getNumSteps() - m_numStepsAtStart;
        float stepStiffnessReduction = 0.f;
        //Both legs balancing/stance or Swing or stiffness
        if (numSteps > m_parameters.m_stepsTillStartEnd) 
          stepStiffnessReduction = (numSteps - m_parameters.m_stepsTillStartEnd - 1)*m_parameters.m_perStepReduction;
        float leftLegStiffness = lowerBodyStiffness - stepStiffnessReduction;
        float rightLegStiffness = lowerBodyStiffness - stepStiffnessReduction;
        if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)
        {
          if (numSteps >= m_parameters.m_stepsTillStartEnd) 
            stepStiffnessReduction = (numSteps - m_parameters.m_stepsTillStartEnd)*m_parameters.m_perStepReduction;
          //Balancing/Stance leg stiffness
          rightLegStiffness = lowerBodyStiffness - stepStiffnessReduction;
        }
        if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
        {
          if (numSteps >= m_parameters.m_stepsTillStartEnd) 
            stepStiffnessReduction = (numSteps - m_parameters.m_stepsTillStartEnd)*m_parameters.m_perStepReduction;
          //Balancing/Stance leg stiffness
          leftLegStiffness = lowerBodyStiffness - stepStiffnessReduction;
        }


        //balancer maybe ramping down
        float balTimer = dynamicBalancerTask->getTimer();
        float balMaximumBalanceTime = dynamicBalancerTask->getMaximumBalanceTime();
        if (balTimer>balMaximumBalanceTime  && !dynamicBalancerTask->getBalanceIndefinitely())
        {
          float stiffnessReductionTime = 1.f/(0.5f*(balTimer - balMaximumBalanceTime)+1.f);
          float balLegStiffness = dynamicBalancerTask->getLeftLegStiffness();
          leftLegStiffness = rage::Min(leftLegStiffness,stiffnessReductionTime*balLegStiffness);

          balLegStiffness = dynamicBalancerTask->getRightLegStiffness();
          rightLegStiffness = rage::Min(rightLegStiffness,stiffnessReductionTime*balLegStiffness);
        }

        //don't go so unstiff as to be unstable
        rightLegStiffness = rage::Max(5.f,rightLegStiffness);
        leftLegStiffness = rage::Max(5.f,leftLegStiffness);

        dynamicBalancerTask->setLeftLegStiffness(leftLegStiffness);
        dynamicBalancerTask->setRightLegStiffness(rightLegStiffness);

        if (dynamicBalancerTask->isActive())
          dynamicBalancerTask->calibrateLowerBodyEffectors(m_body);

        //set stiffness/damping params 
        if (m_YankTimer <= m_parameters.m_timeAtStartValues)
        {
          m_armStiffness = m_parameters.m_armStiffnessStart;
          m_armDamping = m_parameters.m_armDampingStart;
          m_spineStiffness = m_parameters.m_spineStiffnessStart;
          m_spineDamping = m_parameters.m_spineDampingStart;
        }
        else if (m_YankTimer <= m_parameters.m_timeAtStartValues + m_parameters.m_rampTimeFromStartValues)
        {
          float t = (m_YankTimer - m_parameters.m_timeAtStartValues)/m_parameters.m_rampTimeFromStartValues;
          m_armStiffness = m_parameters.m_armStiffnessStart + t*(m_parameters.m_armStiffness - m_parameters.m_armStiffnessStart);
          m_armDamping = m_parameters.m_armDampingStart + t*(m_parameters.m_armDamping - m_parameters.m_armDampingStart);
          m_spineStiffness = m_parameters.m_spineStiffnessStart + t*(m_parameters.m_spineStiffness - m_parameters.m_spineStiffnessStart);
          m_spineDamping = m_parameters.m_spineDampingStart + t*(m_parameters.m_spineDamping - m_parameters.m_spineDampingStart);
        }
        else
        {
          m_armStiffness = m_parameters.m_armStiffness;
          m_armDamping = m_parameters.m_armDamping;
          m_spineStiffness = m_parameters.m_spineStiffness;
          m_spineDamping = m_parameters.m_spineDamping;
        }
        NM_RS_DBG_LOGF(L" Arm Stiffness is= %.4f", m_armStiffness);
        NM_RS_DBG_LOGF(L" Spine Stiffness is= %.4f", m_spineStiffness);

        getSpine()->setBodyStiffness(getSpineInput(), m_spineStiffness, m_spineDamping, bvmask_LowSpine);
      }//staggerFall not active

      if (m_reachTimer >0.f)// && (reachSpeed>1.f))
      {
        NM_RS_DBG_LOGF(L" BLIND BRACE*****************************");
        ReachForRope();
      }
      else
      {
        //relax arms
        m_body->setStiffness(6.f, 0.5f, bvmask_ArmLeft | bvmask_ArmRight);

        getLeftArmInputData()->getElbow()->setDesiredAngle(0.3f);
        getRightArmInputData()->getElbow()->setDesiredAngle(0.3f);
        getLeftArmInputData()->getWrist()->setDesiredAngles(0.0f,0.0f,0.0f);
        getRightArmInputData()->getWrist()->setDesiredAngles(0.0f,0.0f,0.0f);
      }
      m_reachTimer -= timeStep;
      if (m_reachTimer < -0.2f)
        m_reachTimer = m_character->getRandom().GetRanged(0.4f,0.9f);
      HeadLookOrAvoid(timeStep);
      ToggleTurnAndUpperBodyReaction();
    }//m_characterIsFalling
    m_YankTimer += timeStep;
    return eCBUTaskComplete;
  }


  void NmRsCBUYanked::ToggleTurnAndUpperBodyReaction()
  {
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    //If stepping with a different foot:
    //  maybe switch the turn direction, spineBend, spineTwist/Lean2, hipPitch 
    if ((dynamicBalancerTask->footState() != NmRsCBUDynBal_FootState::kNotStepping) 
      && (m_lastFootState != dynamicBalancerTask->footState()))
    {
      rage::Vector3 bodyBack = m_character->m_COMTM.c;
      m_character->levelVector(bodyBack);
      bodyBack.Normalize();
      float dot = bodyBack.Dot(m_turnTo);
      if (dot < -m_turnThreshold)//turns fully round if going backwards
      {
        m_turnLeft = !m_turnLeft;
        m_turnThreshold = m_character->getRandom().GetRanged(m_parameters.m_turnThresholdMin,m_parameters.m_turnThresholdMax);
      }
      m_lastFootState = dynamicBalancerTask->footState();
      dynamicBalancerTask->setForceStep(dynamicBalancerTask->footState(), 0.07f, true);
      m_turn = (m_character->getRandom().GetFloat() < 0.8f);
      if (m_character->getRandom().GetFloat() > 0.7f)
        m_spineBendDir *= -1.f;
      if (m_character->getRandom().GetFloat() > 0.7f)
      {
        if (m_character->getRandom().GetBool())
          m_hipPitch = -m_parameters.m_hipPitchForward;
        else
          m_hipPitch = m_parameters.m_hipPitchBack;
      }
      if (m_character->getRandom().GetFloat() > 0.7f)
        m_spineTwistDir *= -1.f;
    }
    //Maybe switch the turn direction, spineBend, spineTwist/Lean2, hipPitch 
    if (m_character->getRandom().GetFloat() > 0.9f)
      m_spineBendDir *= -1.f;
    if (m_character->getRandom().GetFloat() > 0.9f)
    {
      if (m_character->getRandom().GetBool())
        m_hipPitch = -m_parameters.m_hipPitchForward;
      else
        m_hipPitch = m_parameters.m_hipPitchBack;
    }
    if (m_character->getRandom().GetFloat() > 0.9f)
      m_spineTwistDir *= -1.f;

    bool turn = (m_turnThreshold > 0.01f && m_parameters.m_turnThresholdMin > 0.f && m_parameters.m_turnThresholdMax > 0.f);
    if (turn)
    {
      float spineTwist;
      if (m_turnLeft)
      {
        spineTwist = -m_parameters.m_spineBend*2.f;
      }
      else//turnRight
      {
        spineTwist = m_parameters.m_spineBend*2.f;
      }

      getSpineInputData()->setBackAngles(m_spineBendDir*m_parameters.m_spineBend*2.f,m_spineTwistDir*spineTwist,m_spineTwistDir*spineTwist);

      m_character->instanceToWorldSpace(&m_turnTo, m_parameters.m_headLookPos, m_parameters.m_headLookInstanceIndex);
      m_turnTo -= getSpine()->getSpine3Part()->getPosition();
      m_character->levelVector(m_turnTo);
      m_turnTo.Cross(m_character->m_gUp);
      m_turnTo.Normalize();
      if (!m_turnLeft)
        m_turnTo *=  -1.f;
      rage::Matrix34 tmCom;
      getSpine()->getPelvisPart()->getBoundMatrix(&tmCom);
      rage::Vector3 dirTarget = tmCom.b;
      if (m_turnLeft)
        dirTarget *= -1.f;
      dirTarget += 0.5f*tmCom.c;
      if (m_turn)
        dynamicBalancerTask->useCustomTurnDir(true, dirTarget);
      else
        dynamicBalancerTask->useCustomTurnDir(false, dirTarget);
#if ART_ENABLE_BSPY
      m_character->bspyDrawLine(getSpine()->getHeadPart()->getPosition(), getSpine()->getHeadPart()->getPosition()+dirTarget, rage::Vector3(1,0,0));
      m_turnTo = 2.f*m_turnTo;
      m_character->bspyDrawLine(getSpine()->getHeadPart()->getPosition(), getSpine()->getHeadPart()->getPosition()+dirTarget, rage::Vector3(1,1,1));
      m_character->bspyDrawLine(getSpine()->getSpine3Part()->getPosition(), m_parameters.m_headLookPos, rage::Vector3(1,1,1));
#endif
    }


  }

  void NmRsCBUYanked::HeadLookOrAvoid(float timeStep)
  {
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);

    if (m_characterIsFalling && (m_character->hasCollidedWithEnvironment(bvmask_CervicalSpine | bvmask_ClavicleRight | bvmask_ClavicleLeft)))
    {
      float stiff = 1.f;
      NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      Assert(headLookTask);
      if (headLookTask->isActive())
        headLookTask->deactivate();

      getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_groundSpineStiffness, 1.f, bvmask_HighSpine, &stiff);
      getSpine()->keepHeadAwayFromGround(getSpineInput(), 10.0f);

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "Yanked AVOID", stiff);
#endif
    }
    else if (m_parameters.m_useHeadLook) 
    {
      if (!headLookTask->isActive())//May not have been activated in bodyBalance::Activate
        headLookTask->updateBehaviourMessage(NULL); // initialise the parameters
      //Look at target
      headLookTask->m_parameters.m_pos = m_parameters.m_headLookPos;

      if (m_characterIsFalling)
      {
        headLookTask->m_parameters.m_stiffness = m_parameters.m_groundSpineStiffness;
        headLookTask->m_parameters.m_damping = 1.f;
      }
      else
      {
        headLookTask->m_parameters.m_stiffness = (m_spineStiffness+1.0f);
        headLookTask->m_parameters.m_damping = (m_spineDamping);
      }

      headLookTask->m_parameters.m_alwaysLook = (true);
      headLookTask->m_parameters.m_instanceIndex = (m_parameters.m_headLookInstanceIndex);


      //Maybe Look in velocity direction if:
      //   1)stepping
      //   2)On ground and moving
      //otherwise just use target set above
      if ((dynamicBalancerTask->footState() != NmRsCBUDynBal_FootState::kNotStepping) || (m_characterIsFalling && m_character->m_COMvelMag > 1.5f))
      {
        m_lookAtTimer -= timeStep;
        if (m_lookAtTimer < 0.f)
        {
          m_lookAtTimer = m_character->getRandom().GetRanged(0.2f, 0.40f);
          if (m_parameters.m_headLookAtVelProb < -0.001f)
            m_parameters.m_headLookAtVelProb = m_lookAtRandom;
          if (m_parameters.m_headLookAtVelProb > m_character->getRandom().GetRanged(0.0f, 1.0f))
          {
            m_lookInVelDir = true;
          }
          else
          {
            m_lookInVelDir = false;
          }
        }
        if (m_lookInVelDir)//Look in velocity direction
        {
          rage::Vector3 posTarget = m_character->m_COMvelRelative;
          posTarget.Normalize();
          posTarget += getSpine()->getHeadPart()->getPosition();
          rage::Vector3 targetVel = getSpine()->getSpine3Part()->getLinearVelocity();
          headLookTask->m_parameters.m_vel = targetVel;
          headLookTask->m_parameters.m_pos = (posTarget);
          headLookTask->m_parameters.m_instanceIndex = (-1);
        }
      }
      else
      {
        m_lookAtTimer = 0.f; //Force randomization of LookAt type when the character begins stepping again
      }

      if (!headLookTask->isActive())//May not have been activated in bodyBalance::Activate
        headLookTask->activate();
    }

  }

  void NmRsCBUYanked::ReachForRope()    
  {
    //This reachForRope is only for when the upper arms being constrained to the chest  
    NmRsCBUStaggerFall* staggerFallTask = (NmRsCBUStaggerFall*)m_cbuParent->m_tasks[bvid_staggerFall];
    Assert(staggerFallTask);

    rage::Vector3 braceTarget;
    float maxArmLength = m_character->getRandom().GetRanged(0.2f,0.5f);//0.55 quite bent, 0.6 Just bent, 0.7 straight
    rage::Vector3 vec;
    float armTwist = -0.5f;//arms in front or to side, 0 = arms a bit wider(1.0f for behind)
    rage::Vector3 headLookWorld;
    m_character->instanceToWorldSpace(&headLookWorld, m_parameters.m_headLookPos, m_parameters.m_headLookInstanceIndex);

    braceTarget = headLookWorld;
#if ART_ENABLE_BSPY
    m_character->bspyDrawLine(getSpine()->getSpine3Part()->getPosition(), braceTarget, rage::Vector3(0,1,0));
#endif
    rage::Vector3 comVel = m_character->m_COMvel;
    rage::Vector3 back = m_character->m_COMTM.c;
    m_character->levelVector(comVel);
    m_character->levelVector(back);
    comVel.Normalize();
    back.Normalize();
    bool movingBackwards = (comVel.Dot(back) > 0.5f);

    rage::Vector3 rightOffset;
    rage::Vector3 chestPos;

    NM_RS_DBG_LOGF(L"pos x: %.4f  y: %.4f z: %.4f", braceTarget.x, braceTarget.y, braceTarget.z)

      if (!staggerFallTask->isActive())
      {
        getLeftArm()->setBodyStiffness(getLeftArmInput(), m_armStiffness, m_armDamping);
        getLeftArmInputData()->getElbow()->setStiffness(m_armStiffness, 0.75f*m_armDamping);
        getLeftArmInputData()->getWrist()->setStiffness(m_armStiffness - 1.0f, 1.75f);
      }
      // clamp left arm not to reach too far
      vec = getSpine()->getSpine3Part()->getPosition();
      NM_RS_DBG_LOGF(L"leftshoulder x: %.4f  y: %.4f z: %.4f", vec.x, vec.y, vec.z)

        braceTarget -= vec;
      NM_RS_DBG_LOGF(L"leftHandPos2 x: %.4f  y: %.4f z: %.4f", braceTarget.x, braceTarget.y, braceTarget.z)

        float mag = braceTarget.Mag();
      braceTarget.Normalize();
      braceTarget *= rage::Min(mag , maxArmLength);
      NM_RS_DBG_LOGF(L"leftHandPos3 x: %.4f  y: %.4f z: %.4f", braceTarget.x, braceTarget.y, braceTarget.z)
        if (movingBackwards)//still have the arms come up infront of the body
          braceTarget *= -1.f;
      braceTarget += vec;
      NM_RS_DBG_LOGF(L"mag: %.4f  maxArmLength: %.4f",mag,maxArmLength)

        NM_RS_DBG_LOGF(L"pos2 x: %.4f  y: %.4f z: %.4f", braceTarget.x, braceTarget.y, braceTarget.z)
#if ART_ENABLE_BSPY
          m_character->bspyDrawPoint(braceTarget, 0.5f, rage::Vector3(0,1,0));
#endif

        float dragReduction = 0.f;
        rage::Vector3 targetVel = getLeftArm()->getClaviclePart()->getLinearVelocity();
        {
          NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
          NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
          ikInputData->setTarget(braceTarget);
          ikInputData->setTwist(armTwist);
          ikInputData->setDragReduction(dragReduction);
          ikInputData->setVelocity(targetVel);
          ikInputData->setMatchClavicle(kMatchClavicle);
          getLeftArm()->postInput(ikInput);
        }

        getLeftArmInputData()->getWrist()->setDesiredAngles(0.f, 0.4f, .0f);

        //Right arm
        braceTarget = headLookWorld;
        if (!staggerFallTask->isActive())
        {
          getRightArm()->setBodyStiffness(getRightArmInput(), m_armStiffness, m_armDamping);
          getRightArmInputData()->getElbow()->setStiffness(m_armStiffness, 0.75f*m_armDamping);
          getRightArmInputData()->getWrist()->setStiffness(m_armStiffness - 1.0f, 1.75f);
        }

        // clamp right arm not to reach too far
        vec = getSpine()->getSpine3Part()->getPosition();
        braceTarget -= vec;

        mag = braceTarget.Mag();
        braceTarget.Normalize();
        maxArmLength = m_character->getRandom().GetRanged(0.2f,0.5f);
        braceTarget *= rage::Min(mag, maxArmLength);
        if (movingBackwards)//still have the arms come up infront of the body
          braceTarget *= -1.f;
        braceTarget += vec;
#if ART_ENABLE_BSPY
        m_character->bspyDrawPoint(braceTarget, 0.5f, rage::Vector3(0,0,1));
#endif
        targetVel = getRightArm()->getClaviclePart()->getLinearVelocity();
        {
          NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
          NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
          ikInputData->setTarget(braceTarget);
          ikInputData->setTwist(armTwist);
          ikInputData->setDragReduction(dragReduction);
          ikInputData->setVelocity(targetVel);
          ikInputData->setMatchClavicle(kMatchClavicle);
          getRightArm()->postInput(ikInput);
        }

        getRightArmInputData()->getWrist()->setDesiredAngles(0.0f, 0.0f, 0.0f);
  }


  void NmRsCBUYanked::OnGround() 
  {
    //orientation of character
    rage::Matrix34 tm;
    getSpine()->getSpine0Part()->getBoundMatrix(&tm);
    rage::Vector3 bodyBack = tm.c;
    rage::Vector3 bodyRight = -tm.b;
    bool onLeft = (bodyRight.Dot(m_character->m_gUp) < -0.4f);
    bool onRight = (bodyRight.Dot(m_character->m_gUp) > 0.4f);

    // reduce limbStiffness if rolling over and in contact
    if (onLeft)
    {
      if (m_character->hasCollidedWithWorld(bvmask_ArmLeft)) 
        getLeftArm()->setBodyStiffness(getLeftArmInput(), 7.f,m_parameters.m_armDamping);
      if (m_character->hasCollidedWithWorld(bvmask_LegLeft)) 
        getLeftLeg()->setBodyStiffness(getLeftLegInput(), 7.f,m_parameters.m_armDamping);
    }

    if (onRight)
    {
      if (m_character->hasCollidedWithWorld(bvmask_ArmRight)) 
        getRightArm()->setBodyStiffness(getRightArmInput(), 7.f,m_parameters.m_armDamping);
      if (m_character->hasCollidedWithWorld(bvmask_LegRight)) 
        getRightLeg()->setBodyStiffness(getRightLegInput(), 7.f,m_parameters.m_armDamping);
    }

  }

  void NmRsCBUYanked::Escapologist()
  {
    m_body->setStiffness(m_parameters.m_groundLegStiffness,m_parameters.m_groundLegDamping, bvmask_LegLeft | bvmask_LegRight);
    m_body->setStiffness(m_parameters.m_groundArmStiffness,m_parameters.m_groundArmDamping, bvmask_ArmLeft | bvmask_ArmRight);
    m_body->setStiffness(m_parameters.m_groundSpineStiffness,m_parameters.m_groundSpineDamping, bvmask_LowSpine);

    //Taken from cowboy hip limits (except pike which is lower)
    float pike = 0.6f*m_parameters.m_hipAmplitude;
    float bannana = -0.36f*m_parameters.m_hipAmplitude;//-2.5f
    float sideOut = 0.35f*m_parameters.m_hipAmplitude;//2.5f
    float sideIn = -0.8f*m_parameters.m_hipAmplitude;//-2.5f
    float twistCharlie = -1.f*m_parameters.m_hipAmplitude;//-1
    float twistPinToed = 0.2f*m_parameters.m_hipAmplitude;//1

    float backLow = 1.2f*m_parameters.m_spineAmplitude;//5
    float backHigh = 3.2f*m_parameters.m_spineAmplitude;//16.5
    float backMedium = 1.6f*m_parameters.m_spineAmplitude;//6.5
    float backTwist = 4.f*m_parameters.m_spineAmplitude;//4

    //hip lean2 (-ve is straddle)
    //hip twist (-ve is chaplin)
    if (m_hulaTimer > 0.f && m_hulaTimer < m_parameters.m_hulaPeriod)
    {
      switch (m_hulaState)
      {
      case 3://right
        {
          getLeftLegInputData()->getHip()->setDesiredLean1(bannana);
          getLeftLegInputData()->getHip()->setDesiredLean2(sideOut);
          getRightLegInputData()->getHip()->setDesiredLean1(bannana);
          getRightLegInputData()->getHip()->setDesiredLean2(sideIn);
          getLeftLegInputData()->getHip()->setDesiredTwist(twistCharlie);
          getRightLegInputData()->getHip()->setDesiredTwist(twistPinToed);
          getSpineInputData()->setBackAngles(backLow,-backHigh,backTwist);

          NM_RS_DBG_LOGF(L"Writhe 4");
        }
        break;
      case 2://bannana
        {
          getLeftLegInputData()->getHip()->setDesiredLean1(bannana);
          getLeftLegInputData()->getHip()->setDesiredLean2(0.f);
          getRightLegInputData()->getHip()->setDesiredLean1(bannana);
          getRightLegInputData()->getHip()->setDesiredLean2(0.f);
          getLeftLegInputData()->getHip()->setDesiredTwist(0.f);
          getRightLegInputData()->getHip()->setDesiredTwist(0.f);
          getSpineInputData()->setBackAngles(-backHigh,0.f,0.f);

          NM_RS_DBG_LOGF(L"Writhe 3");
        }
        break;
      case 1://left
        {
          getLeftLegInputData()->getHip()->setDesiredLean1(bannana);
          getLeftLegInputData()->getHip()->setDesiredLean2(sideIn);
          getRightLegInputData()->getHip()->setDesiredLean1(bannana);
          getRightLegInputData()->getHip()->setDesiredLean2(sideOut);
          getLeftLegInputData()->getHip()->setDesiredTwist(twistPinToed);
          getRightLegInputData()->getHip()->setDesiredTwist(twistCharlie);
          getSpineInputData()->setBackAngles(backLow,backHigh,-backTwist);

          NM_RS_DBG_LOGF(L"Writhe 2");
        }
        break;
      case 0://pike
        {
          getLeftLegInputData()->getHip()->setDesiredLean1(pike);
          getLeftLegInputData()->getHip()->setDesiredLean2(0.f);
          getRightLegInputData()->getHip()->setDesiredLean1(pike);
          getRightLegInputData()->getHip()->setDesiredLean2(0.f);
          getLeftLegInputData()->getHip()->setDesiredTwist(0.f);
          getRightLegInputData()->getHip()->setDesiredTwist(0.f);
          getSpineInputData()->setBackAngles(backMedium,0.f,0.f);

          NM_RS_DBG_LOGF(L"Writhe 1");
        }
        break;
      }
      RollOver();

    }
    else//relax: lets catchFall takeOver
    {
      //catchFall doesn't set the hips twist/lean2
      getLeftLegInputData()->getHip()->setDesiredLean2(getLeftLeg()->getHip()->getActualLean2());
      getRightLegInputData()->getHip()->setDesiredLean2(getRightLeg()->getHip()->getActualLean2());
      getLeftLegInputData()->getHip()->setDesiredTwist(0.f);
      getRightLegInputData()->getHip()->setDesiredTwist(0.f);

      //mmmmtodo Don't always do below? 
      //Relax arms to get dragged effect (arms go above head when dragged by feet)
      getLeftArmInputData()->getClavicle()->setStiffness(4.f,0.5f);
      getLeftArmInputData()->getShoulder()->setStiffness(4.f,0.5f);
      getLeftArmInputData()->getElbow()->setStiffness(4.f,0.5f);

      getRightArmInputData()->getClavicle()->setStiffness(4.f,0.5f);
      getRightArmInputData()->getShoulder()->setStiffness(4.f,0.5f);
      getRightArmInputData()->getElbow()->setStiffness(4.f,0.5f);
    }

    if (m_hulaTimer < 0.f)
    {
      m_hulaTimer = m_parameters.m_hulaPeriod;
      m_stepsLeftThisCycle -= 1;
      if (m_stepsLeftThisCycle == 0)
      {
        if (m_parameters.m_minRelaxPeriod < 0.f)
          m_hulaTimer -= m_character->getRandom().GetRanged(m_parameters.m_minRelaxPeriod, m_parameters.m_maxRelaxPeriod) * m_stepsThisCycle * m_parameters.m_hulaPeriod;//mmmmHula make more likely as time increases or number of hulas increases
        else
          m_hulaTimer += m_character->getRandom().GetRanged(m_parameters.m_minRelaxPeriod, m_parameters.m_maxRelaxPeriod);//mmmmHula make more likely as time increases or number of hulas increases
        m_stepsLeftThisCycle = m_character->getRandom().GetRanged(3,7);
        m_stepsThisCycle = m_stepsLeftThisCycle;
      }
      if (m_character->getRandom().GetFloat() > 0.95f)
        m_hulaDirection *= -1;
      if ((m_character->getRandom().GetFloat() > 0.9f) && (m_hulaState == 3 || m_hulaState == 1))
        m_hulaState += m_hulaDirection;
      if (m_hulaState > 3)
        m_hulaState = 0;
      if (m_hulaState < 0)
        m_hulaState = 3;
      m_hulaState += m_hulaDirection;
      if (m_hulaState > 3)
        m_hulaState = 0;
      if (m_hulaState < 0)
        m_hulaState = 3;
    }
    else
      m_hulaTimer -= m_character->getLastKnownUpdateStep();        
  }

  void NmRsCBUYanked::setFrictionMultipliers(float frictionMult)
  {
    getLeftLeg()->getFoot()->setFrictionMultiplier(frictionMult);
    getRightLeg()->getFoot()->setFrictionMultiplier(frictionMult);
    getLeftLeg()->getShin()->setFrictionMultiplier(frictionMult);
    getRightLeg()->getShin()->setFrictionMultiplier(frictionMult);
    getLeftLeg()->getThigh()->setFrictionMultiplier(frictionMult);
    getRightLeg()->getThigh()->setFrictionMultiplier(frictionMult);

    getLeftArm()->getLowerArm()->setFrictionMultiplier(frictionMult);
    getRightArm()->getLowerArm()->setFrictionMultiplier(frictionMult);
    getLeftArm()->getUpperArm()->setFrictionMultiplier(frictionMult);
    getRightArm()->getUpperArm()->setFrictionMultiplier(frictionMult);
    getLeftArm()->getClaviclePart()->setFrictionMultiplier(frictionMult);
    getRightArm()->getClaviclePart()->setFrictionMultiplier(frictionMult);

    getSpine()->getPelvisPart()->setFrictionMultiplier(frictionMult);
    getSpine()->getSpine0Part()->setFrictionMultiplier(frictionMult);
    getSpine()->getSpine1Part()->setFrictionMultiplier(frictionMult);
    getSpine()->getSpine2Part()->setFrictionMultiplier(frictionMult);
    getSpine()->getSpine3Part()->setFrictionMultiplier(frictionMult);
  }

  void NmRsCBUYanked::RollOver()    
  {
    bool bodyOnGround = m_character->hasCollidedWithWorld(bvmask_LowSpine);
    rage::Vector3 torqueVector;
    rage::Vector3 spine0Pos;
    rage::Vector3 spine3Pos;
    spine0Pos = getSpine()->getSpine0()->getJointPosition();
    spine3Pos = getSpine()->getSpine3()->getJointPosition();
    torqueVector = spine3Pos - spine0Pos;
    torqueVector.Normalize(torqueVector);
    //float rotVelMag = m_character->m_COMrotvelMag;
    float rotVelMagCom = m_character->m_COMrotvel.Dot(torqueVector);
    NM_RS_DBG_LOGF(L"Writhe rotVelMagHip 0: %f", rotVelMagCom);
    float rotVelMagHip = getSpine()->getPelvisPart()->getAngularVelocity().Dot(torqueVector);
    NM_RS_DBG_LOGF(L"Writhe rotVelMagHip 0: %f", rotVelMagHip);
    float rotVelMag = getSpine()->getSpine3Part()->getAngularVelocity().Dot(torqueVector);
    float rollOverDirection = rage::Sign(rotVelMag);
    NM_RS_DBG_LOGF(L"Writhe rotVelMag 3: %f", rotVelMag);
    // apply some cheat forces
    //if ((rotVelMag<10.f) && bodyOnGround)
    rotVelMag = rage::Abs(rotVelMag);
    rotVelMagHip = rage::Abs(rotVelMagHip);
    rotVelMagCom = rage::Abs(rotVelMagCom);
    if (bodyOnGround && rotVelMag < 6.f && rotVelMagHip < 6.f && rotVelMagCom < 6.f && m_character->m_COMrotvelMag < 8.f)
    {
      float rollOverPhase = 0.5f*(rage::Sinf(m_rollOverTimer*m_rollOverPeriod)+1.0f);
      rollOverPhase = rage::Clamp((rollOverPhase-0.2f), 0.0f, 1.0f);
      torqueVector.Scale(torqueVector,rollOverDirection*rollOverPhase*25.f*m_parameters.m_rollHelp); 
      NM_RS_DBG_LOGF(L"Writhe rollover : %f", rollOverDirection*rollOverPhase*20.f*5.0f);
      NM_RS_DBG_LOGF(L"Writhe rollOverDirection : %f", rollOverDirection);
      NM_RS_DBG_LOGF(L"Writhe rollOverPhase : %f", rollOverPhase);
      //reduce twist cheat torque if sideon to velocity.
      if (m_character->m_COMvelMag > 1.f)
      {
        rage::Vector3 comVelNorm = m_character->m_COMvel;
        comVelNorm.Normalize();
        if (comVelNorm.Dot(m_character->m_COMTM.a) > 0.5f)//vel.side
          torqueVector *= 0.1f;
      }
      getSpine()->getSpine3Part()->applyTorque(torqueVector);
      getSpine()->getSpine2Part()->applyTorque(torqueVector);
      getSpine()->getSpine1Part()->applyTorque(torqueVector);
      getSpine()->getSpine0Part()->applyTorque(torqueVector);
    }

    if (m_rollOverTimer < 0.f)
    {
      m_rollOverTimer = m_character->getRandom().GetRanged(0.5f,3.f);
    }
    else
      m_rollOverTimer -= m_character->getLastKnownUpdateStep(); 
  }

#if ART_ENABLE_BSPY
  void NmRsCBUYanked::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_armStiffnessStart, true);
    bspyTaskVar(m_parameters.m_armDampingStart, true);
    bspyTaskVar(m_parameters.m_spineStiffnessStart, true);
    bspyTaskVar(m_parameters.m_spineDampingStart, true);
    bspyTaskVar(m_parameters.m_timeAtStartValues, true);
    bspyTaskVar(m_parameters.m_rampTimeFromStartValues, true);
    bspyTaskVar(m_parameters.m_armStiffness, true);
    bspyTaskVar(m_parameters.m_armDamping, true);
    bspyTaskVar(m_parameters.m_spineStiffness, true);
    bspyTaskVar(m_parameters.m_spineDamping, true);

    bspyTaskVar(m_parameters.m_stepsTillStartEnd, true);
    bspyTaskVar(m_parameters.m_perStepReduction, true);
    bspyTaskVar(m_parameters.m_timeStartEnd, true);
    bspyTaskVar(m_parameters.m_rampTimeToEndValues, true);
    bspyTaskVar(m_parameters.m_lowerBodyStiffness, true);
    bspyTaskVar(m_parameters.m_lowerBodyStiffnessEnd, true);
    bspyTaskVar(m_parameters.m_headLookPos, true);
    bspyTaskVar(m_parameters.m_headLookInstanceIndex, true);
    bspyTaskVar(m_parameters.m_headLookAtVelProb, true);
    bspyTaskVar(m_parameters.m_hipPitchForward, true);
    bspyTaskVar(m_parameters.m_hipPitchBack, true);
    bspyTaskVar(m_parameters.m_spineBend, true);
    bspyTaskVar(m_parameters.m_footFriction, true);
    bspyTaskVar(m_parameters.m_comVelRDSThresh, true);
    bspyTaskVar(m_parameters.m_useHeadLook, true);
    bspyTaskVar(m_parameters.m_turnThresholdMin, true);
    bspyTaskVar(m_parameters.m_turnThresholdMax, true);

    //Wriggle Params
    bspyTaskVar(m_parameters.m_hulaPeriod, true);
    bspyTaskVar(m_parameters.m_hipAmplitude, true);
    bspyTaskVar(m_parameters.m_spineAmplitude, true);
    bspyTaskVar(m_parameters.m_minRelaxPeriod, true);
    bspyTaskVar(m_parameters.m_maxRelaxPeriod, true);
    bspyTaskVar(m_parameters.m_rollHelp, true);
    bspyTaskVar(m_parameters.m_groundLegStiffness, true);
    bspyTaskVar(m_parameters.m_groundArmStiffness, true);
    bspyTaskVar(m_parameters.m_groundSpineStiffness, true);
    bspyTaskVar(m_parameters.m_groundLegDamping, true);
    bspyTaskVar(m_parameters.m_groundArmDamping, true);
    bspyTaskVar(m_parameters.m_groundSpineDamping, true);
    bspyTaskVar(m_parameters.m_groundFriction, true);

    bspyTaskVar(m_armStiffness, false);
    bspyTaskVar(m_armDamping, false);
    bspyTaskVar(m_spineStiffness, false);
    bspyTaskVar(m_spineDamping, false);
    bspyTaskVar(m_reachTimer, false);
    bspyTaskVar(m_hulaTimer, false);
    bspyTaskVar(m_rollOverPeriod, false);
    bspyTaskVar(m_rollOverTimer, false);
    bspyTaskVar(m_lastFootState, false);
    bspyTaskVar(m_hulaState, false);
    bspyTaskVar(m_hulaDirection, false);
    bspyTaskVar(m_stepsLeftThisCycle, false);
    bspyTaskVar(m_stepsThisCycle, false);
    bspyTaskVar(m_lookInVelDir, false);

    rage::Vector3 bodyBack = m_character->m_COMTM.c;
    m_character->levelVector(bodyBack);
    bodyBack.Normalize();
    float dot = bodyBack.Dot(m_turnTo);
    bspyTaskVar(dot, false);


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

