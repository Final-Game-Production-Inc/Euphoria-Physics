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
#include "NmRsCBU_StaggerFall.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_Catchfall.h"
#include "NmRsCBU_ArmsWindmillAdaptive.h" 
#include "NmRsCBU_RollDownStairs.h" 
#include "NmRsCBU_Yanked.h" 
#include "NmRsCBU_Shot.h" 

namespace ART
{
  NmRsCBUStaggerFall::NmRsCBUStaggerFall(ART::MemoryManager* services) : CBUTaskBase(services, bvid_staggerFall)
  {
    initialiseCustomVariables();
  }

  NmRsCBUStaggerFall::~NmRsCBUStaggerFall()
  {
  }

  void NmRsCBUStaggerFall::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
  }

  void NmRsCBUStaggerFall::onActivate()
  {
    Assert(m_character);

    m_balanceFailed = false;
    m_reacted = false;

    m_staggerTimer = 0.f;
    m_catchFallTimer = 0.f;
    m_leftToeTimer = 0.f;
    m_rightToeTimer = 0.f;

    //StaggerFall Entry
    NM_RS_DBG_LOGF(L"- StaggerFall Entry");

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

    dynamicBalancerTask->setStagger(true);
    //  if (dynamicBalancerTask->isActive())
    //{
    //	dynamicBalancerTask->setOpposeGravityAnkles(0.75f);
    //	dynamicBalancerTask->setOpposeGravityLegs(0.75f);
    //	dynamicBalancerTask->setLowerBodyGravityOpposition();
    //}
    //dynamicBalancerTask->setAnkleEquilibrium(-0.22f);
    //if (m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl)
    //  dynamicBalancerTask->setAnkleEquilibrium(-0.00f);

    //For Look at type randomness
    //m_parameters.m_headLookAtVelProb 0 = no look at vel. >1 = always look at vel
    //if negative (-1) m_parameters.m_headLookAtVelProb set to m_lookAtRandom i.e. randomizeed
    m_lookAtRandom = m_character->getRandom().GetRanged(0.0f, 1.0f); //Set here only        
    m_lookAtTimer = 0.f; //Force lookAt to be randomized on tick
    //For Turn type randomness
    m_randomTurn = m_character->getRandom().GetFloat();
    m_turnTimer = 0.0f;//Force bodyTurn to be randomized on tick 
    m_lastFootState = NmRsCBUDynBal_FootState::kNotStepping;//Force armsOut to be randomized on next foot step
    m_fscLeftPos.Zero();
    m_fscRightPos.Zero();
    m_numSteps = 0;

    //getLeftLeg()->getFoot()->setFrictionMultiplier((enable ? 2.5f : 0.1f)/(part->getBound()->GetMaterial(0).GetFriction()));
    getLeftLeg()->getFoot()->setFrictionMultiplier(4.f);
    getRightLeg()->getFoot()->setFrictionMultiplier(4.f);
    //bSpy only
    spyLeftLegState = ls_SwingLeg;
    spyRightLegState = ls_SwingLeg;
  }

  void NmRsCBUStaggerFall::onDeactivate()
  {
    Assert(m_character);

    //De-activate subTasks
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    dynamicBalancerTask->setStagger(false);
    dynamicBalancerTask->autoLeanHipsCancel();
    dynamicBalancerTask->setBalanceTime(0.2f);
    //

    //apply balancer settings that were overwitten by staggerFall
    if (dynamicBalancerTask->isActive())
    {
      dynamicBalancerTask->setOpposeGravityAnkles(1.f);
      dynamicBalancerTask->setOpposeGravityLegs(1.f);
      dynamicBalancerTask->calibrateLowerBodyEffectors(m_body);
      dynamicBalancerTask->setLowerBodyGravityOpposition(m_body);
    }
    dynamicBalancerTask->setHipPitch(0.f);

    Assert(dynamicBalancerTask);
    if (dynamicBalancerTask->isActive() && m_character->noBehavioursUsingDynBalance())
      dynamicBalancerTask->requestDeactivate();

    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    if (headLookTask->isActive())
      headLookTask->deactivate();

    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    if (catchFallTask->isActive())
      catchFallTask->deactivate();

    NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
    Assert(rdsTask);
    rdsTask->deactivate();

    NmRsCBUArmsWindmillAdaptive* armsWindmillAdaptiveTask = (NmRsCBUArmsWindmillAdaptive*)m_cbuParent->m_tasks[bvid_armsWindmillAdaptive];
    Assert(armsWindmillAdaptiveTask);
    if (armsWindmillAdaptiveTask->isActive())
      armsWindmillAdaptiveTask->deactivate();

    getLeftLeg()->getFoot()->setFrictionMultiplier(1.f);
    getRightLeg()->getFoot()->setFrictionMultiplier(1.f);
  }

  CBUTaskReturn NmRsCBUStaggerFall::onTick(float timeStep)
  {
    if (!m_character->getArticulatedBody())
      return eCBUTaskComplete;

    // check to see if the articulated body is asleep, in which case we are 'stable' and can't / needn't balance
    if (rage::phSleep *sleep = m_character->getArticulatedWrapper()->getArticulatedCollider()->GetSleep())
    {
      if (sleep->IsAsleep())
        return eCBUTaskComplete;
    }

    NM_RS_DBG_LOGF(L"- StaggerFall During")
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);

    //spine whip upper body stiffnesses 
    if (m_staggerTimer <= m_parameters.m_timeAtStartValues)
    {
      m_armStiffness = m_parameters.m_armStiffnessStart;
      m_armDamping = m_parameters.m_armDampingStart;
      m_spineStiffness = m_parameters.m_spineStiffnessStart;
      m_spineDamping = m_parameters.m_spineDampingStart;
    }
    else if (m_staggerTimer <= m_parameters.m_timeAtStartValues + m_parameters.m_rampTimeFromStartValues)
    {
      float t = (m_staggerTimer - m_parameters.m_timeAtStartValues)/m_parameters.m_rampTimeFromStartValues;
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

    rage::Matrix34 tmCom;
    tmCom = m_character->m_COMTM;
    rage::Vector3 bodyRight = tmCom.a;
    //rage::Vector3 bodyUp = tmCom.b;
    rage::Vector3 bodyBack = tmCom.c;
    float backDotVel;
    float sideDotVel;
    rage::Vector3 comVel = m_character->m_COMvelRelative;
    float speed = rage::Clamp(comVel.Mag(),0.f,3.f)/2.f;
    comVel.Normalize();
    float dot = bodyBack.Dot(comVel);
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "StaggerFall", dot);
#endif

    bool goingBackWards = dot > 0.f;

    //ramp down leg stiffnesses
    float lowerBodyStiffness = m_parameters.m_lowerBodyStiffness;
    if (!goingBackWards)
      lowerBodyStiffness += 2.f;
    if (m_staggerTimer >= m_parameters.m_timeStartEnd + m_parameters.m_rampTimeToEndValues)
    {
      lowerBodyStiffness = m_parameters.m_lowerBodyStiffnessEnd;
    }
    else if (m_staggerTimer >= m_parameters.m_timeStartEnd)
    {
      float t = (m_staggerTimer - m_parameters.m_timeStartEnd)/m_parameters.m_rampTimeToEndValues;
      lowerBodyStiffness = m_parameters.m_lowerBodyStiffness + t*(m_parameters.m_lowerBodyStiffnessEnd - m_parameters.m_lowerBodyStiffness);
    }

    //Character setup
    m_body->setStiffness(m_spineStiffness , m_spineDamping, bvmask_Spine, NULL, true);

    // Because bodyBalance is sometimes used as upper body arms reaction, staggerFall sets the arm stiffness too high for bodyBalance to look good.
    // Don't set arm stiffnesses if staggerFall is not controlling the upper body reaction.
    // NOTE: that staggerFall still controls the spine stiffnesses.
    if (m_parameters.m_upperBodyReaction)
    {
      m_body->setStiffness(m_armStiffness , m_parameters.m_armDamping, bvmask_ArmLeft | bvmask_ArmRight, NULL, true);
    }

    NM_RS_DBG_LOGF(L" Lower Body Stiffness = %.4f", lowerBodyStiffness);

    m_staggerTimer += timeStep;

    //Work out what the character is doing and apply arms and leanInDircection
    rage::Vector3 braceTarget;
    braceTarget = 0.3f*(getSpine()->getSpine3Part()->getLinearVelocity() - m_character->getFloorVelocity());
    braceTarget += 0.3f*(getSpine()->getPelvisPart()->getLinearVelocity() - m_character->getFloorVelocity());
    getSpine()->getSpine1Part()->getBoundMatrix(&tmCom); 
    if (m_character->m_gUp.Dot(tmCom.c) > 0.8f) //facingdown falling forwards
      braceTarget -= braceTarget.Mag()*tmCom.c;//tmCom.b;bodyUp

    float reachSpeed = braceTarget.Mag(); 
    braceTarget.Normalize();
    getSpine()->getSpine3Part()->getBoundMatrix(&tmCom); 
    float reachDot = braceTarget.Dot(tmCom.c);//bodyBack; 
    NM_RS_DBG_LOGF(L"    reachDot = %.4f", reachDot);
    NM_RS_DBG_LOGF(L"    reachSpeed = %.4f", reachSpeed);

    float leanAmount = rage::Clamp(m_staggerTimer*m_parameters.m_leanInDirRate,0.f,1.0f);;
    float leanHipsAmount = leanAmount;
    if (goingBackWards)
    {
      leanAmount *= m_parameters.m_leanInDirMaxB;
      leanHipsAmount *= m_parameters.m_leanHipsMaxB;
    }
    else //forwards-sideways
    {
      leanAmount *= m_parameters.m_leanInDirMaxF;
      leanHipsAmount *= m_parameters.m_leanHipsMaxF;
    }
    if (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
      dynamicBalancerTask->autoLeanHipsInDirection(comVel, leanHipsAmount);
    if (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
      dynamicBalancerTask->autoLeanInDirection(comVel, leanAmount);



    backDotVel = bodyBack.Dot(comVel);
    sideDotVel = bodyRight.Dot(comVel);
    if (m_parameters.m_upperBodyReaction)
      UpperBodyReaction(timeStep,reachDot,reachSpeed,sideDotVel);


    if ((dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK) && (dynamicBalancerTask->footState() != NmRsCBUDynBal_FootState::kNotStepping))
    {
      float mess = m_character->getRandom().GetRanged(0.f, 1.f);
      dynamicBalancerTask->setPlantLeg(false);
      if (mess > (1.f - m_parameters.m_staggerStepProb))
        dynamicBalancerTask->setPlantLeg(true);
    }

    if (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
    {
      dynamicBalancerTask->setStagger(true);
      dynamicBalancerTask->setBalanceTime(m_parameters.m_predictionTime);

      //mmmmtodo replace this code by dynBal->getNumSteps() and storing numSteps on activate.
      //  You could then use (with some modification to ensure that decrementSteps doesn't go below activation steps)
      //  dynBal->decrementSteps to keep the character staggering for longer if hit multiple times
      //  m_staggerTimer could similarly be gotten from the balancer for this purpose.
      //If stepping with a different foot update num of steps and initialize the toe timers.
      if ((dynamicBalancerTask->footState() != NmRsCBUDynBal_FootState::kNotStepping) 
        && (m_lastFootState != dynamicBalancerTask->footState()))
      {
        m_lastFootState = dynamicBalancerTask->footState();
        //keep track of the number of steps
        m_numSteps++;
        if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)
          m_leftToeTimer = 4.f/60.f;
        if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
          m_rightToeTimer = 4.f/60.f;
      }
      m_leftToeTimer -= timeStep;
      m_rightToeTimer -= timeStep;

      //set stagger hipPitch and spine leans
      float lean1 = -speed*backDotVel/3.f;///3.f;//0.3f; 
      float lean2 = 0.0f;
      float hipPitch = -lean1*m_parameters.m_hipBendMult; 
      if (m_parameters.m_alwaysBendForwards)
        hipPitch = -rage::Abs(hipPitch);
      dynamicBalancerTask->setHipPitch(hipPitch);

      speed = rage::Clamp(m_character->m_COMvelRelativeMag,0.f,6.f)/2.5f;
      //Lean into sideways velocity
      //if (rage::Abs(backDotVel) < 0.8f && sideDotVel < 0.f)               
      if (sideDotVel < 0.f)               
        lean2 = speed*rage::Abs(sideDotVel);//0.5f;//left
      //if (rage::Abs(backDotVel) < 0.8f && sideDotVel > 0.f)               
      if (sideDotVel > 0.f)               
        lean2 = -speed*rage::Abs(sideDotVel);//-0.5f;//right
      if (balColReactTask->isActive())
      {
        //Lean against sideways velocity
        if (balColReactTask->m_balancerState == bal_LeanAgainst)
          lean2 = - lean2;
      }
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", lean2);
#endif
      if (goingBackWards)
      {
        lean2 *= m_parameters.m_lean2multB;
        lean1 = 0.f;
      }
      else
      {
        lean2 *= m_parameters.m_lean2multF;
        lean1 = m_parameters.m_spineBendMult;
      }
      if (m_character->m_COMvelMag < 1.f)
        lean2 *= 0.5f;

      //m_character->applySpineLean(/*lean1**/m_parameters.m_spineBendMult, lean2);
      getSpineInputData()->applySpineLean(lean1, lean2);

      if (balColReactTask->isActive())//wallSlump, LeanAgainstWall, start effect of balancerCollisionsReaction
      {
        if ((balColReactTask->m_impactOccurred) && (balColReactTask->m_balancerState != bal_Rebound))//mmmmtodo reduce for rebound?
        {
          //m_parameters.m_useBodyTurn = true;
          //m_parameters.m_turn2TargetProb = 1.f;
          //m_parameters.m_turn2VelProb = 0.f;
          //m_parameters.m_turnAwayProb = 0.f;
          //m_parameters.m_turnLeftProb = 0.f;
          //m_parameters.m_turnRightProb = 0.f;
          //m_parameters.m_turnOffProb = 0.f;
          //m_parameters.m_headLookPos = m_character->m_pos1stContact + 3.f*m_character->m_normal1stContact;

          //Start reducing the stiffnesses of the legs (perStepReduction will have to be set)
          //MMMMTodo: have a balancerCollisionsReaction perStepReduction that overides staggers' one?
          if (!m_reacted) 
          {
            m_reacted = true;
            if (m_numSteps < m_parameters.m_stepsTillStartEnd + 2)
              m_numSteps = m_parameters.m_stepsTillStartEnd + 2;
          }

          //balColReactTask->setBalanceTime();//sets the dynamicBalancers' balance time depending on its' state
          //balColReactTask->setHipPitch(timeStep);//sets the dynamicBalancers' hipPitch depending on its' state (to get back flat with wall)
        }
      }//wallSlump, LeanAgainstWall, start effect of balancerCollisionsReaction end
      float muscleStiffness = 1.f;
      if (!goingBackWards)
        muscleStiffness = 0.7f;
      float muscleStiffness2 = 4.f;
      float stepStiffnessReduction = 0.f;
      if (m_numSteps > m_parameters.m_stepsTillStartEnd) 
        stepStiffnessReduction = (m_numSteps - m_parameters.m_stepsTillStartEnd - 1)*m_parameters.m_perStepReduction1;
      NM_RS_DBG_LOGF(L" Step Stiffness Reduction= %.4f", stepStiffnessReduction);

      float ankleStiffnessL;
      float kneeStiffnessL;
      float hipStiffnessL;

      float ankleStiffnessR;
      float kneeStiffnessR;
      float hipStiffnessR;
      //Swing stiffnesses
      ankleStiffnessL = lowerBodyStiffness - 1.f - stepStiffnessReduction;
      kneeStiffnessL = lowerBodyStiffness - 3.f - stepStiffnessReduction;
      hipStiffnessL = lowerBodyStiffness - 1.f - stepStiffnessReduction;

      ankleStiffnessR = lowerBodyStiffness - 1.f - stepStiffnessReduction;
      kneeStiffnessR = lowerBodyStiffness - 3.f - stepStiffnessReduction;
      hipStiffnessR = lowerBodyStiffness - 1.f - stepStiffnessReduction;

      //bSpy only
      spyLeftLegState = ls_SwingLeg;
      spyRightLegState = ls_SwingLeg;

      if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)
      {
        if (m_numSteps >= m_parameters.m_stepsTillStartEnd) 
          stepStiffnessReduction = (m_numSteps - m_parameters.m_stepsTillStartEnd)*m_parameters.m_perStepReduction1;

        //stance stiffnesses
        ankleStiffnessR = lowerBodyStiffness + 2.f - stepStiffnessReduction;
        kneeStiffnessR = lowerBodyStiffness - stepStiffnessReduction;
        hipStiffnessR = lowerBodyStiffness - stepStiffnessReduction;

        //bSpy only
        spyLeftLegState = ls_SwingLeg;
        spyRightLegState = ls_Balance;
      }
      if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
      {
        if (m_numSteps >= m_parameters.m_stepsTillStartEnd) 
          stepStiffnessReduction = (m_numSteps - m_parameters.m_stepsTillStartEnd)*m_parameters.m_perStepReduction1;
        //stance stiffnesses
        ankleStiffnessL = lowerBodyStiffness + 2.f - stepStiffnessReduction;
        kneeStiffnessL = lowerBodyStiffness - stepStiffnessReduction;
        hipStiffnessL = lowerBodyStiffness - stepStiffnessReduction;
        //bSpy only
        spyLeftLegState = ls_Balance;
        spyRightLegState = ls_SwingLeg;
      }

      rage::Vector3 bodyVel(m_character->m_COMvelRelative),bodyCom(m_character->m_COM);
      rage::Vector3 rightFootP(getRightLeg()->getFoot()->getPosition()),leftFootP(getLeftLeg()->getFoot()->getPosition());
      if (goingBackWards)
      {
        rightFootP = getRightLeg()->getAnkle()->getJointPosition();
        leftFootP = getLeftLeg()->getAnkle()->getJointPosition();
      }
      m_character->levelVector(bodyCom, 0.f); 
      m_character->levelVector(bodyVel, 0.f); 
      m_character->levelVector(rightFootP, 0.f);
      m_character->levelVector(leftFootP, 0.f);
      rage::Vector3 l2Com(bodyCom),r2Com(bodyCom);
      l2Com -= leftFootP;
      r2Com -= rightFootP;

      float l2ComMag = l2Com.Mag(); 
      float r2ComMag = r2Com.Mag(); 
      float l2ComDotVel1 = l2Com.Dot(bodyVel);
      float r2ComDotVel1 = r2Com.Dot(bodyVel);          

      bodyVel.Normalize();

      l2Com.Normalize();
      r2Com.Normalize();
      float l2ComDotVel = l2Com.Dot(bodyVel);
      float r2ComDotVel = r2Com.Dot(bodyVel);          
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", l2ComMag);
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", r2ComMag);
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", l2ComDotVel);
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", r2ComDotVel);
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", l2ComDotVel1);
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", r2ComDotVel1);
#endif
      static float pushOffDotKneeOld = -20.2f;//mmmmRemove this after testing new 
      static float pushOffDotAnkleOld = -20.2f;//mmmmRemove this after testing new
      static float pushOffDotAnkle = 0.2f;//mmmmRemove this after testing new - may be a parameter also?
      //static float m_parameters.pushOffDist = 0.2f;
      //static float m_parameters.maxPushoffVel = 20.4f;
      static float desKnee = -0.2f;//mmmm turn to const this after testing new
      float hipVelUp = getSpine()->getPelvisPart()->getLinearVelocity().Dot(m_character->m_gUp);

      if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)//(rightFootBalance)
      {
        //float hipHeight = m_character->vectorHeight(getSpine()->getPelvisPart()->getPosition()) - m_character->vectorHeight(getRightLeg()->getFoot()->getPosition());
        //      if ((r2ComDotVel > pushOffDotKneeOld)&& getRightLeg()->getFoot()->collidedWithNotOwnCharacter() && (hipHeight < m_hipHeight))
        if ((r2ComDotVel > pushOffDotKneeOld)&& (r2ComDotVel1 > m_parameters.pushOffDist)&& getRightLeg()->getFoot()->collidedWithNotOwnCharacter() && (hipVelUp < m_parameters.maxPushoffVel))
        {
          spyRightLegState = ls_PushOff;
          float desiredKnee = getRightLeg()->getKnee()->getDesiredAngle();
#if ART_ENABLE_BSPY && 0
          bspyScratchpad(m_character->getBSpyID(), "StaggerFallR", desiredKnee);
#endif
          if (desiredKnee>desKnee)
          {
            bool pushOfWithKnee = true;
            if (balColReactTask->isActive())
              if (!((balColReactTask->m_balancerState == bal_Normal) || (balColReactTask->m_balancerState == bal_Rebound)))
                pushOfWithKnee = false;
            if (pushOfWithKnee)
            {
              spyRightLegState = ls_PushOffKnee;
              desiredKnee += rage::Clamp((desiredKnee - desKnee)* 25.f, 0.0f, 2.5f);

              getRightLegInputData()->getKnee()->setDesiredAngle(desiredKnee);

              //kneeStiffnessR = 6.f + lowerBodyStiffness + 3.f - stepStiffnessReduction;
              //hipStiffnessR = 1.f + lowerBodyStiffness + 2.f  - stepStiffnessReduction;
            }
#if ART_ENABLE_BSPY
            bspyScratchpad(m_character->getBSpyID(), "StaggerFallR", pushOfWithKnee);
#endif

          }
          //desiredKnee += rage::Clamp(r2ComMag*r2ComDotVel* 59.f, 0.0f, 2.5f);
          //getRightLeg()->getKnee()->setDesiredAngle(desiredKnee);//1.7f);
          //nmrsSetLean1(getRightLeg()->getHip(),rage::Clamp(nmrsGetDesiredLean1(getRightLeg()->getHip()) - r2ComMag*r2ComDotVel* 59.f,-9.f,9.f));
          //bSpy only
        }
        else
        {
          //softer knees on landing
          stepStiffnessReduction = 0.f;
          if (m_numSteps > m_parameters.m_stepsTillStartEnd) 
            stepStiffnessReduction = (m_numSteps - m_parameters.m_stepsTillStartEnd - 1)*m_parameters.m_perStepReduction1;
          //Strike leg stiffness
          kneeStiffnessR = lowerBodyStiffness - 3.f - stepStiffnessReduction;
          ankleStiffnessR =  lowerBodyStiffness - 1.f - stepStiffnessReduction;
          if (m_leftToeTimer >= 0.f)
            hipStiffnessR =  12.f - 1.f- stepStiffnessReduction;

          //bSpy only
          spyRightLegState = ls_FootStrike;
        }
        if ((m_leftToeTimer >= 0.f) && (l2ComDotVel > pushOffDotAnkleOld)&& (l2ComDotVel1 > pushOffDotAnkle))
        {
          //float desiredKnee = getLeftLeg()->getKnee()->getActualAngle();
          //if (desiredKnee>-0.2f)
          //{
          //  desiredKnee += rage::Clamp((desiredKnee + 0.2f)* 25.f, 0.0f, 2.5f);
          //  getLeftLeg()->getKnee()->setDesiredAngle(desiredKnee);//1.7f);
          //}

          getLeftLegInputData()->getAnkle()->setDesiredLean1(rage::Clamp(nmrsGetActualLean1(getLeftLeg()->getAnkle()) - 6.f,-9.f,9.f));

          //getLeftLeg()->getKnee()->setDesiredAngle(1.f);//1.7f);
          //desiredKnee += rage::Clamp(l2ComMag*l2ComDotVel* 59.f, 0.0f, 2.5f);
          //getLeftLeg()->getKnee()->setDesiredAngle(desiredKnee);//1.7f);
          //nmrsSetLean1(getLeftLeg()->getHip(),rage::Clamp(nmrsGetDesiredLean1(getLeftLeg()->getHip()) - l2ComMag*l2ComDotVel* 59.f,-9.f,9.f));
          //bSpy only
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "StaggerFallLA", true);
#endif

          spyLeftLegState = ls_ToeTimer;
        }
      }

      if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)//(leftFootBalance)
      {

        //float hipHeight = m_character->vectorHeight(getSpine()->getPelvisPart()->getPosition()) - m_character->vectorHeight(getLeftLeg()->getFoot()->getPosition());
        //      if ((l2ComDotVel > pushOffDotKneeOld) && getLeftLeg()->getFoot()->collidedWithNotOwnCharacter()&& (hipHeight < m_hipHeight))
        if ((l2ComDotVel1 > m_parameters.pushOffDist) && (l2ComDotVel > pushOffDotKneeOld) && getLeftLeg()->getFoot()->collidedWithNotOwnCharacter()&& (hipVelUp < m_parameters.maxPushoffVel))
        {
          spyLeftLegState = ls_PushOff;
          float desiredKnee = getLeftLeg()->getKnee()->getDesiredAngle();
#if ART_ENABLE_BSPY && 0
          bspyScratchpad(m_character->getBSpyID(), "StaggerFallL", desiredKnee);
#endif
          if (desiredKnee>desKnee)
          {
            bool pushOfWithKnee = true;
            if (balColReactTask->isActive())
              if (!((balColReactTask->m_balancerState == bal_Normal) || (balColReactTask->m_balancerState == bal_Rebound)))
                pushOfWithKnee = false;
            if (pushOfWithKnee)
            {
              spyLeftLegState = ls_PushOffKnee;
              desiredKnee += rage::Clamp((desiredKnee - desKnee)* 25.f, 0.0f, 2.5f);

              getLeftLegInputData()->getKnee()->setDesiredAngle(desiredKnee);

#if ART_ENABLE_BSPY
              bspyScratchpad(m_character->getBSpyID(), "StaggerFallL", pushOfWithKnee);
#endif
              //kneeStiffnessL = 6.f + lowerBodyStiffness + 3.f - stepStiffnessReduction;
              //hipStiffnessL = 1.f + lowerBodyStiffness + 2.f - stepStiffnessReduction;
            }
          }
          //desiredKnee += rage::Clamp(l2ComMag*l2ComDotVel* 59.f, 0.0f, 2.5f);
          //getLeftLeg()->getKnee()->setDesiredAngle(desiredKnee);//1.7f);
          //nmrsSetLean1(getLeftLeg()->getHip(),rage::Clamp(nmrsGetDesiredLean1(getLeftLeg()->getHip()) - l2ComMag*l2ComDotVel* 59.f,-9.f,9.f));
        }
        else
        {
          stepStiffnessReduction = 0.f;
          if (m_numSteps > m_parameters.m_stepsTillStartEnd) 
            stepStiffnessReduction = (m_numSteps - m_parameters.m_stepsTillStartEnd - 1)*m_parameters.m_perStepReduction1;
          //Strike leg stiffness
          kneeStiffnessL = lowerBodyStiffness - 3.f - stepStiffnessReduction;
          ankleStiffnessL = lowerBodyStiffness - 1.f - stepStiffnessReduction;
          if (m_rightToeTimer >= 0.f)
            hipStiffnessL = 12.f - 1.f - stepStiffnessReduction;

          //bSpy only
          spyLeftLegState = ls_FootStrike;
        }

        if ((m_rightToeTimer >= 0.f) && (r2ComDotVel > pushOffDotAnkleOld) && (r2ComDotVel1 > pushOffDotAnkle))
        {
          //float desiredKnee = getRightLeg()->getKnee()->getActualAngle();
          //if (desiredKnee>-0.2f)
          //{
          //  desiredKnee += rage::Clamp((desiredKnee + 0.2f)* 25.f, 0.0f, 2.5f);
          //  getRightLeg()->getKnee()->setDesiredAngle(desiredKnee);//1.7f);
          //}

          getRightLegInputData()->getAnkle()->setDesiredLean1(rage::Clamp(nmrsGetActualLean1(getRightLeg()->getAnkle()) - 6.f,-9.f,9.f));

#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "StaggerFallRA", true);
#endif
          //getRightLeg()->getKnee()->setDesiredAngle(1.f);//1.7f);
          //desiredKnee += rage::Clamp(l2ComMag*l2ComDotVel* 59.f, 0.0f, 2.5f);
          //getLeftLeg()->getKnee()->setDesiredAngle(desiredKnee);//1.7f);
          //nmrsSetLean1(getLeftLeg()->getHip(),rage::Clamp(nmrsGetDesiredLean1(getLeftLeg()->getHip()) - l2ComMag*l2ComDotVel* 59.f,-9.f,9.f));
          //bSpy only
          spyRightLegState = ls_ToeTimer;
        }
      }

      if ((dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep))// && (l2rDotVel < 0.3f))// && (getRightLeg()->getFoot()->collidedWithNotOwnCharacter()))
        //if ((dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)  && (getRightLeg()->getFoot()->collidedWithNotOwnCharacter()))
      {
        float addLean1 = - 30.f*(getRightLeg()->getFoot()->getLinearVelocity() - m_character->getFloorVelocity()).Mag();
        if (r2ComDotVel > 0.2f)
          addLean1 -= 50.f*r2ComMag;

        //point right toes more
        rage::Vector3 footVel(m_character->m_COMvelRelative);
        rage::Vector3 footBack(m_character->m_COMTM.c); //back
        m_character->levelVector(footVel,0.f);
        m_character->levelVector(footBack,0.f);
        footVel.Normalize();
        footBack.Normalize();
        float dum = footVel.Dot(footBack);
        if (dum<0.5f)//don't point toes if going backwards
        {
          getRightLegInputData()->getAnkle()->setDesiredLean1(rage::Clamp(nmrsGetDesiredLean1(getRightLeg()->getAnkle()) + addLean1,-9.f,9.f));

#if ART_ENABLE_BSPY && 0
          bspyScratchpad(m_character->getBSpyID(), "StaggerFallR", addLean1);
#endif
        }
        //getRightLeg()->getAnkle()->setMuscleStrength(180.f);
        //getRightLeg()->getAnkle()->setMuscleDamping(24.f);

      }
      if ((dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep) )//&& (l2rDotVel > -0.3f))// && (getLeftLeg()->getFoot()->collidedWithNotOwnCharacter()))
        //if ((dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)  && (getLeftLeg()->getFoot()->collidedWithNotOwnCharacter()))
      {
        float addLean1 = - 30.f*(getLeftLeg()->getFoot()->getLinearVelocity() - m_character->getFloorVelocity()).Mag();
        if (l2ComDotVel > 0.2f)
          addLean1 -= 50.f*l2ComMag;

        //point left toes more
        rage::Vector3 footVel(m_character->m_COMvelRelative);
        rage::Vector3 footBack(m_character->m_COMTM.c); //back
        m_character->levelVector(footVel,0.f);
        m_character->levelVector(footBack,0.f);
        footVel.Normalize();
        footBack.Normalize();
        float dum = footVel.Dot(footBack);
        if (dum<0.5f)//don't point toes if going backwards
        {
          getLeftLegInputData()->getAnkle()->setDesiredLean1(rage::Clamp(nmrsGetDesiredLean1(getLeftLeg()->getAnkle()) + addLean1,-9.f,9.f));

#if ART_ENABLE_BSPY && 0
          bspyScratchpad(m_character->getBSpyID(), "StaggerFallL", addLean1);
#endif
        }
        //getLeftLeg()->getAnkle()->setMuscleStrength(180.f);
        //getLeftLeg()->getAnkle()->setMuscleDamping(24.f);
      }

      float balTimer = dynamicBalancerTask->getTimer();
      float balMaximumBalanceTime = dynamicBalancerTask->getMaximumBalanceTime();
      if (balTimer>balMaximumBalanceTime  && !dynamicBalancerTask->getBalanceIndefinitely())
      {
        float balLegStiffness = dynamicBalancerTask->getLeftLegStiffness();
        float stiffnessReductionTime = 1.f/(0.5f*(balTimer - balMaximumBalanceTime)+1.f);
        ankleStiffnessL = rage::Min(ankleStiffnessL,stiffnessReductionTime*balLegStiffness);
        kneeStiffnessL = rage::Min(kneeStiffnessL,stiffnessReductionTime*balLegStiffness);
        hipStiffnessL = rage::Min(hipStiffnessL,stiffnessReductionTime*balLegStiffness);

        balLegStiffness = dynamicBalancerTask->getRightLegStiffness();
        ankleStiffnessR = rage::Min(ankleStiffnessR,stiffnessReductionTime*balLegStiffness);
        kneeStiffnessR = rage::Min(kneeStiffnessR,stiffnessReductionTime*balLegStiffness);
        hipStiffnessR = rage::Min(hipStiffnessR,stiffnessReductionTime*balLegStiffness);

      }
      ankleStiffnessL = rage::Max(7.f,ankleStiffnessL);
      kneeStiffnessL = rage::Max(5.f,kneeStiffnessL);
      hipStiffnessL = rage::Max(5.f,hipStiffnessL);
      ankleStiffnessR = rage::Max(7.f,ankleStiffnessR);
      kneeStiffnessR = rage::Max(5.f,kneeStiffnessR);
      hipStiffnessR = rage::Max(5.f,hipStiffnessR);
      //mmmmLegShaking
      //        ankleStiffnessL = rage::Max(8.f,ankleStiffnessL);
      //        kneeStiffnessL = rage::Max(5.5f,kneeStiffnessL);
      //        hipStiffnessL = rage::Max(5.5f,hipStiffnessL);
      //        ankleStiffnessR = rage::Max(8.f,ankleStiffnessR);
      //        kneeStiffnessR = rage::Max(5.5f,kneeStiffnessR);
      //        hipStiffnessR = rage::Max(5.5f,hipStiffnessR);

#if ART_ENABLE_BSPY && 0
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", ankleStiffnessL);
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", kneeStiffnessL);
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", hipStiffnessL);
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", ankleStiffnessR);
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", kneeStiffnessR);
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall", hipStiffnessR);
#endif

      getLeftLegInputData()->getKnee()->setStiffness(kneeStiffnessL, 0.9f, &muscleStiffness);
      getLeftLegInputData()->getAnkle()->setStiffness(ankleStiffnessL, 1.f, &muscleStiffness2);
      getLeftLegInputData()->getHip()->setStiffness(hipStiffnessL, 1.f, &muscleStiffness);
      getRightLegInputData()->getKnee()->setStiffness(kneeStiffnessR, 0.9f, &muscleStiffness);
      getRightLegInputData()->getAnkle()->setStiffness(ankleStiffnessR, 1.f, &muscleStiffness2);
      getRightLegInputData()->getHip()->setStiffness(hipStiffnessR, 1.f, &muscleStiffness);

      NM_RS_DBG_LOGF(L" ankleStiffnessL= %.4f", ankleStiffnessL);
      NM_RS_DBG_LOGF(L" kneeStiffnessL= %.4f", kneeStiffnessL);
      NM_RS_DBG_LOGF(L" hipStiffnessL= %.4f", hipStiffnessL);
      NM_RS_DBG_LOGF(L" ankleStiffnessR= %.4f", ankleStiffnessR);
      NM_RS_DBG_LOGF(L" kneeStiffnessR= %.4f", kneeStiffnessR);
      NM_RS_DBG_LOGF(L" hipStiffnessR= %.4f", hipStiffnessR);          //Apply muscle parameters

    }




    //if (balColReactTask->isActive())//Exaggerate Impact
    //  balColReactTask->exaggerateImpact(timeStep);//sets hipPitch,Back and neck angles

    if (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK)
    {
      bool dontDoFall = (balColReactTask->isActive() && (balColReactTask->m_balancerState == bal_Drape || balColReactTask->m_balancerState == bal_DrapeForward));// || balColReactTask->m_balancerState == bal_DrapeGlancingSpin));
      if (!dontDoFall)
        falling(timeStep);
    }

    // blend in the zero pose when body is not moving
    // (stops the arms rising up if the character stops with legs split)
    // always blend in a certain amount to try and retain some of the character's initial stance
    if (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK && m_parameters.m_upperBodyReaction)//blend
    {
      float motionMultiplier = rage::Clamp(2.f*rage::Sqrtf(0.5f * m_character->getKineticEnergyPerKilo_RelativeVelocity()), 0.001f, 1.0f); 
      NM_RS_DBG_LOGF(L"***** motionMultiplier = %.4f", motionMultiplier);
      float blendFactor = 1.0f - motionMultiplier;

      // limbs todo check. afaik, this is an external blend against other running behaviours...
      NmRsLimbInput leftArmInput  = createNmRsLimbInput<NmRsArmInputWrapper>(0, blendFactor);
      NmRsLimbInput rightArmInput = createNmRsLimbInput<NmRsArmInputWrapper>(0, blendFactor);
      NmRsLimbInput spineInput    = createNmRsLimbInput<NmRsSpineInputWrapper>(0, blendFactor);

      NmRsArmInputWrapper* leftArmInputData = leftArmInput.getData<NmRsArmInputWrapper>();
      NmRsArmInputWrapper* rightArmInputData = rightArmInput.getData<NmRsArmInputWrapper>();
      NmRsSpineInputWrapper* spineInputData = spineInput.getData<NmRsSpineInputWrapper>();

      // blend value is 1.0 because we the inputs themselves take the blend
      // weights into account (external blend).
      //m_body->blendToZeroPose(1.0f, bvmask_Spine | bvmask_ArmLeft | bvmask_ArmRight);
	  getLeftArm()->blendToZeroPose(leftArmInput, 1.0f);
	  getRightArm()->blendToZeroPose(rightArmInput, 1.0f);
	  getSpine()->blendToZeroPose(spineInput, 1.0f);

      if (!getLeftArm()->getClavicle()->hasStoredZeroPose())
        leftArmInputData->getClavicle()->setDesiredAngles(0.0f, 0.0f, 0.0f);
      if (!getLeftArm()->getShoulder()->hasStoredZeroPose())
        leftArmInputData->getShoulder()->setDesiredAngles(0.0f, 0.0f, 0.5f);
      if (!getLeftArm()->getElbow()->hasStoredZeroPose())
        leftArmInputData->getElbow()->setDesiredAngle(0.0f);
      if (!getLeftArm()->getWrist()->hasStoredZeroPose())
        leftArmInputData->getWrist()->setDesiredAngles(0.0f, 0.0f, 0.0f);

      if (!getRightArm()->getClavicle()->hasStoredZeroPose())
        rightArmInputData->getClavicle()->setDesiredAngles(0.0f, 0.0f, 0.0f);
      if (!getRightArm()->getShoulder()->hasStoredZeroPose())
        rightArmInputData->getShoulder()->setDesiredAngles(0.0f, 0.0f, 0.5f);
      if (!getRightArm()->getElbow()->hasStoredZeroPose())
        rightArmInputData->getElbow()->setDesiredAngle(0.0f);
      if (!getRightArm()->getWrist()->hasStoredZeroPose())
        rightArmInputData->getWrist()->setDesiredAngles(0.0f, 0.0f, 0.0f);

      if (!getSpine()->getUpperNeck()->hasStoredZeroPose())
        spineInputData->getUpperNeck()->setDesiredAngles(0.0f, 0.0f, 0.0f);
      if (!getSpine()->getLowerNeck()->hasStoredZeroPose())
        spineInputData->getLowerNeck()->setDesiredAngles(0.0f, 0.0f, 0.0f);

      getLeftArm()->postInput(leftArmInput);
      getRightArm()->postInput(rightArmInput);
      getSpine()->postInput(spineInput);

    }//blend end
    NM_RS_DBG_LOGF(L" LeftAnkle Strength,Stiffness,damping = %.4f, %.4f, %.4f", getLeftLeg()->getAnkle()->getMuscleStrength(), getLeftLeg()->getAnkle()->getMuscleStiffness(), getLeftLeg()->getAnkle()->getMuscleDamping());

    return eCBUTaskComplete;
  }

  void NmRsCBUStaggerFall::UpperBodyReaction(float timeStep, float reachDot, float reachSpeed, float sideDotVel)
  {
    NmRsCBUArmsWindmillAdaptive* armsWindmillAdaptiveTask = (NmRsCBUArmsWindmillAdaptive*)m_cbuParent->m_tasks[bvid_armsWindmillAdaptive];
    Assert(armsWindmillAdaptiveTask);

    bool usedRight = false;
    bool usedLeft = false;

    if (((reachDot<0.2f) || ((reachDot<0.7f) && rage::Abs(sideDotVel)>0.7f)) && (reachSpeed>0.5f))
    {

      usedRight = true;
      usedLeft = true;
      NM_RS_DBG_LOGF(L" BLIND BRACE*****************************");
      if (reachSpeed<1.2f)
      {
        usedLeft = m_character->getRandom().GetRanged(-0.4f,reachSpeed+0.1f) > 0.6f;
        usedRight = m_character->getRandom().GetRanged(-0.4f,reachSpeed+0.1f) > 0.6f;
      }
      if (sideDotVel < -0.7f)
      {
        usedRight = false;
        usedLeft = true;
      }
      if (sideDotVel > 0.7f) 
      {
        usedRight = true;
        usedLeft = false;
      }
      ArmsBrace();
      HeadLookAndTurn(timeStep);
      if (armsWindmillAdaptiveTask->isActive())
        armsWindmillAdaptiveTask->deactivate();
#if ART_ENABLE_BSPY && 0
      bspyScratchpad(m_character->getBSpyID(), "StaggerFallBrace",0.f);
#endif

    }

    //is shot doing arms?
    NmRsCBUShot* shotTask = (NmRsCBUShot*)m_cbuParent->m_tasks[bvid_shot];
    Assert(shotTask);
    bool shotIsDoingArms = (shotTask->isActive() && shotTask->m_parameters.brace == true);

    if ((reachDot>0.2f) && (reachSpeed>0.5f) && !shotIsDoingArms)//Going Backwards
    {
      if (!armsWindmillAdaptiveTask->isActive())
      {
        armsWindmillAdaptiveTask->updateBehaviourMessage(NULL); // initialise parameters.
        armsWindmillAdaptiveTask->m_parameters.disableOnImpact = true;
        //armsWindmillAdaptiveTask->m_parameters.amplitude = 0.8f;
        armsWindmillAdaptiveTask->m_parameters.phase = 0.8f;
        armsWindmillAdaptiveTask->m_parameters.armStiffness = 9.f;
        armsWindmillAdaptiveTask->m_parameters.bodyStiffness = m_spineStiffness;
        armsWindmillAdaptiveTask->m_parameters.leftElbowAngle = 0.6f;
        armsWindmillAdaptiveTask->m_parameters.rightElbowAngle = 0.6f;
        armsWindmillAdaptiveTask->m_parameters.armDirection = -1;  //Backwards  
        armsWindmillAdaptiveTask->m_parameters.setBackAngles = false; 

        armsWindmillAdaptiveTask->activate();
      }
      armsWindmillAdaptiveTask->m_parameters.angSpeed = 2.0f*PI/(rage::Clamp(10.f - rage::Abs(reachSpeed)*10.f,0.7f,2.f));

      bool useLeft = !usedLeft;
      bool useRight = !usedRight;
      usedRight = true;
      usedLeft = true;
      if (sideDotVel < -0.9f && useRight)
      {
        useRight = false;
      }
      if (sideDotVel > 0.9f && useLeft) 
      {
        useLeft = false;
      }
      armsWindmillAdaptiveTask->setUseArms(useLeft,useRight);
#if ART_ENABLE_BSPY && 0
      bspyScratchpad(m_character->getBSpyID(), "StaggerFallAW",usedRight);
      bspyScratchpad(m_character->getBSpyID(), "StaggerFallAW",usedLeft);
#endif
    }
    else
    {
      NmRsCBUArmsWindmillAdaptive* armsWindmillAdaptiveTask = (NmRsCBUArmsWindmillAdaptive*)m_cbuParent->m_tasks[bvid_armsWindmillAdaptive];
      Assert(armsWindmillAdaptiveTask);
      if (armsWindmillAdaptiveTask->isActive())
        armsWindmillAdaptiveTask->deactivate();
#if ART_ENABLE_BSPY && 0
      bspyScratchpad(m_character->getBSpyID(), "StaggerFallDef",true);
#endif

    }

    static bool useDefaultArms = true;
    if (useDefaultArms)
    {
#if ART_ENABLE_BSPY
      m_character->setCurrentSubBehaviour("-SFDefArms"); 
#endif
      rage::Vector3 lean = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
      rage::Vector3 leanVel = getSpine()->getSpine3Part()->getLinearVelocity() - getSpine()->getPelvisPart()->getLinearVelocity();
      rage::Vector3 fall = (lean*0.5f + leanVel*0.3f)*2.f;
      float fallMag = fall.Mag();
      fallMag = rage::Clamp(fallMag - 0.15f, 0.f, 1.f);
      rage::Matrix34 mat;

      getRightArm()->getShoulder()->getMatrix1(mat);
      // TDL default arm motion here.
      if (!usedLeft)
      {
        getLeftArm()->setBodyStiffness(getLeftArmInput(), 7.f + 3.f*fallMag, 1.f);

        getLeftArm()->getShoulder()->getMatrix1(mat);
        float fallX = 4.f * fall.Dot(mat.a);
        float fallZ = rage::Clamp(4.f * fall.Dot((mat.c - mat.b)*0.707f), -8.f, 8.f);

        float l1 = -sinf(fallX);
        float l2 = cosf(fallX) + fallZ - 0.5f*fallMag;

        getLeftArmInputData()->getShoulder()->setOpposeGravity(fallMag);
        getLeftArmInputData()->getClavicle()->setOpposeGravity(fallMag);
        // TDL we can replace the static pose with a zero pose if we need to. Make 1-fallMag the blend factor
        getLeftArmInputData()->getShoulder()->setDesiredAngles(
          (l1 + getLeftArm()->getShoulder()->getMidLean1())*fallMag,
          (l2 + getLeftArm()->getShoulder()->getMidLean2())*fallMag + (0.2f)*(1.f-fallMag), 0.f);
        getLeftArmInputData()->getClavicle()->setDesiredAngles(0.f, (l2 + getLeftArm()->getClavicle()->getMidLean2())*fallMag, 0.f);
        getLeftArmInputData()->getWrist()->setDesiredAngles(0,0,0);
        getLeftArmInputData()->getElbow()->setDesiredAngle(1.f*fallMag + 0.7f*(1.f-fallMag));
      }
      if (!usedRight)
      {
        getRightArm()->setBodyStiffness(getRightArmInput(), 7.f + 3.f*fallMag, 1.f);

        getRightArm()->getShoulder()->getMatrix1(mat);
        float fallX = 4.f * fall.Dot(mat.a);
        float fallZ = rage::Clamp(4.f * fall.Dot((mat.c + mat.b)*0.707f), -8.f, 8.f);

        float l1 = -sinf(fallX);
        float l2 = cosf(fallX) + fallZ - 0.5f*fallMag;

        getRightArmInputData()->getShoulder()->setOpposeGravity(fallMag);
        getRightArmInputData()->getClavicle()->setOpposeGravity(fallMag);
        getRightArmInputData()->getShoulder()->setDesiredAngles(
          (l1 + getRightArm()->getShoulder()->getMidLean1())*fallMag,
          (l2 + getRightArm()->getShoulder()->getMidLean2())*fallMag + (0.2f)*(1.f-fallMag), -0.1f);
        getRightArmInputData()->getClavicle()->setDesiredAngles(0.f, (l2 + getRightArm()->getClavicle()->getMidLean2())*fallMag, 0.f);
        getRightArmInputData()->getClavicle()->setDesiredAngles(0,0,0);
        getRightArmInputData()->getWrist()->setDesiredTwist(0.f);
        getRightArmInputData()->getElbow()->setDesiredAngle(0.8f*fallMag + 0.8f*(1.f-fallMag));
      }
    }
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif

  }

  void NmRsCBUStaggerFall::HeadLookAndTurn(float timeStep)
  {
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);

    //HEADLOOK:  Override BodyBalance Parameters
    if (m_parameters.m_useHeadLook) 
    {
      if (!headLookTask->isActive())//May not have been activated in bodyBalance::Activate
        headLookTask->updateBehaviourMessage(NULL); // initialise the parameters
      //Look at target
      rage::Vector3 posTarget = m_parameters.m_headLookPos;
      headLookTask->m_parameters.m_pos = (posTarget);

      headLookTask->m_parameters.m_stiffness = (m_spineStiffness+2.0f);
      headLookTask->m_parameters.m_damping = (m_spineDamping);
      headLookTask->m_parameters.m_alwaysLook = (true);
      headLookTask->m_parameters.m_instanceIndex = (m_parameters.m_headLookInstanceIndex);

      //Maybe Look in velocity direction stepping (otherwise just use target set above)
      if (dynamicBalancerTask->footState() != NmRsCBUDynBal_FootState::kNotStepping)
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
          posTarget = m_character->m_COMvelRelative;
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

    //BODYTURN Override BodyBalance Parameters
    float averageSpeed = rage::Sqrtf(2.0f * m_character->getKineticEnergyPerKilo_RelativeVelocity()) * 0.5f;
    float motionMultiplier = rage::Clamp(2.f*averageSpeed, 0.001f, 1.0f);
    NM_RS_DBG_LOGF(L"    > avSpeed = %.4f", averageSpeed);
    NM_RS_DBG_LOGF(L"    > motionMultiplier = %.4f", motionMultiplier);

    //Set the turn Direction in Dynamic Balance
    rage::Vector3 noTurn(0.f, 0.f, 0.f);
    dynamicBalancerTask->useCustomTurnDir(false, noTurn);

    if (m_parameters.m_useBodyTurn) 
    {
      rage::Vector3 posTargetLocal;
      rage::Vector3 dirTarget;

      m_turnTimer = m_turnTimer - timeStep;
      if (m_turnTimer < 0.f) //Randomize turn
      {
        m_turnTimer = m_character->getRandom().GetRanged(0.4f,1.0f);
        m_randomTurn = m_character->getRandom().GetFloat();
      }

      float sumOfWeights = m_parameters.m_turnOffProb + m_parameters.m_turn2TargetProb + m_parameters.m_turn2VelProb + m_parameters.m_turnAwayProb + m_parameters.m_turnLeftProb + m_parameters.m_turnRightProb;
      if (sumOfWeights > 0.0001f)//Choose turn type
      {
        float sum0 = m_parameters.m_turn2TargetProb/sumOfWeights;
        float sum1 = sum0 + m_parameters.m_turn2VelProb/sumOfWeights;
        float sum2 = sum1 + m_parameters.m_turnAwayProb/sumOfWeights;
        float sum3 = sum2 + m_parameters.m_turnLeftProb/sumOfWeights;
        float sum4 = sum3 + m_parameters.m_turnRightProb/sumOfWeights;

        if (m_randomTurn <= sum0) //Turn towards pusher/headlook target (prob. = .2)
        {
          posTargetLocal = m_parameters.m_headLookPos;
          m_character->instanceToWorldSpace(&dirTarget, posTargetLocal, m_parameters.m_headLookInstanceIndex);
          dirTarget -= m_character->m_COM;
        }
        else if ((m_randomTurn > sum0) && (m_randomTurn <= sum1))//Turn towards velocity (prob. = .3)
        {
          dirTarget = m_character->m_COMvelRelative;
        }
        else if ((m_randomTurn > sum1) && (m_randomTurn <= sum2)) //Turn Away from pusher/headlook target (prob. = .15)
        {
          posTargetLocal = m_parameters.m_headLookPos;
          m_character->instanceToWorldSpace(&dirTarget, posTargetLocal, m_parameters.m_headLookInstanceIndex);
          dirTarget -= m_character->m_COM;
          dirTarget *= -1.0f;
        }
        else if ((m_randomTurn > sum2) && (m_randomTurn <= sum3))//Turn Right (prob. = .125)
        {
          int instanceIndex = m_character->getFirstInstance()->GetLevelIndex();
          posTargetLocal.Set(-1.0f, 0.f, 0.f);
          m_character->instanceToWorldSpace(&dirTarget, posTargetLocal, instanceIndex);
          dirTarget -= m_character->m_COM;
        }
        else if ((m_randomTurn > sum3) && (m_randomTurn <= sum4))//Turn Left (prob. = .125)
        {
          int instanceIndex = m_character->getFirstInstance()->GetLevelIndex();
          posTargetLocal.Set(1.0f, 0.f, 0.f);
          m_character->instanceToWorldSpace(&dirTarget, posTargetLocal, instanceIndex);
          dirTarget -= m_character->m_COM;
        }
        if (m_randomTurn < sum4) //Turn off (prob. = .1)
        {
          dirTarget.Normalize();
          dynamicBalancerTask->useCustomTurnDir(true, dirTarget);
        }
      }
    }
    if (m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl)
    {
      if (motionMultiplier < 0.3f) 
        dynamicBalancerTask->useCustomTurnDir(false, noTurn);
    }
    else
    {
      if (motionMultiplier < 0.06f) 
        dynamicBalancerTask->useCustomTurnDir(false, noTurn);
    }

  }

  void NmRsCBUStaggerFall::ArmsBrace()    
  {
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-SFBrace"); 
#endif
    rage::Vector3 braceTarget;
    static bool randomArmLength = false;
    static bool gaitArmLength = false;
    static bool rampVelMult = false;
    static float velMult = 0.3f;//was 0.3f
    static float maxArmLength = 0.61f;//0.55 quite bent, 0.6 Just bent, 0.7 straight
    float maxArmLengthL = maxArmLength;
    if (randomArmLength)
      maxArmLengthL = m_character->getRandom().GetRanged(0.55f, 0.7f);//0.61f;//0.55 quite bent, 0.6 Just bent, 0.7 straight
    if (gaitArmLength)
    {
      float fudge1 = 1.2f; //actual hip multiplier (shoulder only)
      float fudge2 = 1.1f; //split parameter (shoulder only)
      float a = 0.6f; // actual leg split multiplier (shoulder only)
      // limbs todo these desired angles are last tick's values. this code may be attempting
      // to get information from the dynamic balancer by way to the hip angles. under the new
      // system, this info will be a frame old at best and possibly sourced from a completely
      // different behaviour at worst.  keep an eye on this.
      float angleDelta = nmrsGetDesiredLean1(getRightLeg()->getHip()) - fudge1*nmrsGetActualLean1(getRightLeg()->getHip());
      angleDelta = angleDelta - a*(fudge2*nmrsGetActualLean1(getRightLeg()->getHip()) - nmrsGetActualLean1(getLeftLeg()->getHip()));
      maxArmLengthL = 0.61f;//0.55 quite bent, 0.6 Just bent, 0.7 straight
      maxArmLengthL +=  angleDelta*0.04f;
    }
    rage::Vector3 vec;
    float armTwist = -0.5f;//arms in front or to side, 0 = arms a bit wider(1.0f for behind)
    if (rampVelMult)
      velMult = rage::Clamp(m_staggerTimer*0.1f,0.2f,0.3f);//was 0.3f
    braceTarget = velMult*(getSpine()->getSpine3Part()->getLinearVelocity() - m_character->getFloorVelocity());
    braceTarget += velMult*(getSpine()->getPelvisPart()->getLinearVelocity() - m_character->getFloorVelocity());
    rage::Matrix34 tmCom;
    getSpine()->getSpine1Part()->getBoundMatrix(&tmCom);

    getLeftArmInputData()->getWrist()->setDesiredAngles(0.4f, -0.4f, 1.0f);

    if (m_character->m_gUp.Dot(tmCom.c) > 0.8f) //facingdown falling forwards
    {
      braceTarget -= 0.2f * braceTarget.Mag()*tmCom.c;//tmCom.b;bodyUp

      getLeftArmInputData()->getWrist()->setDesiredAngles(0.4f, -3.8f, 1.0f);
    }

    braceTarget += getSpine()->getSpine0Part()->getPosition();//spine3 arms up more
    //set spine twist to this target maybe only if falling

    NM_RS_DBG_LOGF(L"pos x: %.4f  y: %.4f z: %.4f", braceTarget.x, braceTarget.y, braceTarget.z)

    getLeftArm()->setBodyStiffness(getLeftArmInput(), m_armStiffness, m_armDamping);
    getLeftArmInputData()->getElbow()->setStiffness(m_armStiffness, 0.75f*m_armDamping);
    getLeftArmInputData()->getWrist()->setStiffness(m_armStiffness - 1.0f, 1.75f);

    // clamp left arm not to reach too far
    vec = getLeftArm()->getShoulder()->getJointPosition();
    NM_RS_DBG_LOGF(L"leftshoulder x: %.4f  y: %.4f z: %.4f", vec.x, vec.y, vec.z)

      braceTarget -= vec;
    NM_RS_DBG_LOGF(L"leftHandPos2 x: %.4f  y: %.4f z: %.4f", braceTarget.x, braceTarget.y, braceTarget.z)

      float mag = braceTarget.Mag();
    braceTarget.Normalize();
    braceTarget *= rage::Min(mag , maxArmLengthL);
    NM_RS_DBG_LOGF(L"leftHandPos3 x: %.4f  y: %.4f z: %.4f", braceTarget.x, braceTarget.y, braceTarget.z)
      braceTarget += vec;
    NM_RS_DBG_LOGF(L"mag: %.4f  maxArmLength: %.4f",mag,maxArmLengthL)

#if ART_ENABLE_BSPY && 0
      bspyScratchpad(m_character->getBSpyID(), "StaggerFall3", braceTarget);
#endif
    NM_RS_DBG_LOGF(L"pos2 x: %.4f  y: %.4f z: %.4f", braceTarget.x, braceTarget.y, braceTarget.z)

      float dragReduction = 1.f;
    float straightness = m_character->getRandom().GetRanged(0.0f, 0.4f);
    float maxSpeed = 5.f;//from balance 200.f; //from catch fall - ie out of range

    rage::Vector3 targetVel = getLeftArm()->getClaviclePart()->getLinearVelocity();
    {
      NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
      NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();

      ikInputData->setTarget(braceTarget);
      ikInputData->setTwist(armTwist);
      ikInputData->setDragReduction(dragReduction);
      ikInputData->setVelocity(targetVel);
      ikInputData->setAdvancedStaightness(straightness);
      ikInputData->setAdvancedMaxSpeed(maxSpeed);
      ikInputData->setUseAdvancedIk(true);
      // limbs todo implement different clavicle matching methods on enum
      ikInputData->setMatchClavicle(kMatchClavicleUsingTwist);

      getLeftArm()->postInput(ikInput);
    }

    static float maxArmLengthR = maxArmLength;
    //Right arm
    if (randomArmLength)
      maxArmLengthR = m_character->getRandom().GetRanged(0.55f, 0.7f);//0.61f;//0.55 quite bent, 0.6 Just bent, 0.7 straight
    if (gaitArmLength)
    {
      float fudge1 = 1.2f; //actual hip multiplier (shoulder only)
      float fudge2 = 1.1f; //split parameter (shoulder only)
      float a = 0.6f; // actual leg split multiplier (shoulder only)
      float angleDelta = nmrsGetDesiredLean1(getLeftLeg()->getHip()) - fudge1*nmrsGetActualLean1(getLeftLeg()->getHip());
      angleDelta = angleDelta - a*(fudge2*nmrsGetActualLean1(getLeftLeg()->getHip()) - nmrsGetActualLean1(getRightLeg()->getHip()));
      maxArmLengthR = 0.61f;
      maxArmLengthR += angleDelta*0.04f;
    }
    braceTarget = getSpine()->getSpine0Part()->getPosition();
    braceTarget += velMult*(getSpine()->getSpine3Part()->getLinearVelocity() - m_character->getFloorVelocity());
    braceTarget += velMult*(getSpine()->getPelvisPart()->getLinearVelocity() - m_character->getFloorVelocity());

    getRightArmInputData()->getWrist()->setDesiredAngles(0.4f, -0.4f, 1.0f);

    if (m_character->m_gUp.Dot(tmCom.c) > 0.8f) //facingdown falling forwards
    {
      braceTarget -= 0.2f * braceTarget.Mag()*tmCom.c;//tmCom.b;bodyUp

      getRightArmInputData()->getWrist()->setDesiredAngles(0.4f, -3.8f, 1.0f);
    }

    getRightArm()->setBodyStiffness(getRightArmInput(), m_armStiffness, m_armDamping);
    getRightArmInputData()->getElbow()->setStiffness(m_armStiffness, 0.75f*m_armDamping);
    getRightArmInputData()->getWrist()->setStiffness(m_armStiffness - 1.0f, 1.75f);

    // clamp right arm not to reach too far
    vec = getRightArm()->getShoulder()->getJointPosition();
    braceTarget -= vec;

    mag = braceTarget.Mag();
    braceTarget.Normalize();
    braceTarget *= rage::Min(mag, maxArmLengthR);
    braceTarget += vec;

    straightness = m_character->getRandom().GetRanged(0.0f, 0.4f);
    targetVel = getRightArm()->getClaviclePart()->getLinearVelocity();
    {
      NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
      NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();

      ikInputData->setTarget(braceTarget);
      ikInputData->setTwist(armTwist);
      ikInputData->setDragReduction(dragReduction);
      ikInputData->setVelocity(targetVel);
      ikInputData->setAdvancedStaightness(straightness);
      ikInputData->setAdvancedMaxSpeed(maxSpeed);
      ikInputData->setUseAdvancedIk(true);
      // limbs todo implement different clavicle matching methods on enum
      ikInputData->setMatchClavicle(kMatchClavicleUsingTwist);

      getRightArm()->postInput(ikInput);
    }

#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif

  }

  void NmRsCBUStaggerFall::falling(float timeStep)
  {
    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);
    if (!m_balanceFailed)
    {
      m_balanceFailed = true;
      if (balColReactTask->isActive())
        balColReactTask->resetFrictionMultipliers();

      getRightLegInputData()->getHip()->setDesiredAngles(
        nmrsGetActualLean1(getRightLeg()->getHip()), 
        rage::Min(nmrsGetDesiredLean2(getRightLeg()->getHip()), -0.0f), 
        rage::Min(nmrsGetDesiredTwist(getRightLeg()->getHip()), 0.0f));
      getLeftLegInputData()->getHip()->setDesiredAngles(
        nmrsGetActualLean1(getLeftLeg()->getHip()), 
        rage::Min(nmrsGetDesiredLean2(getLeftLeg()->getHip()),-0.0f), 
        rage::Min(nmrsGetDesiredTwist(getLeftLeg()->getHip()), 0.0f));

      getRightLegInputData()->getKnee()->setDesiredAngle(nmrsGetActualAngle(getRightLeg()->getKnee()));
      getLeftLegInputData()->getKnee()->setDesiredAngle(nmrsGetActualAngle(getLeftLeg()->getKnee()));

      m_body->resetEffectors(kResetCalibrations);

      m_body->setStiffness(7.0f, 1.0f, bvmask_LegLeft | bvmask_LegRight, NULL, true);
    }

    //mmmmmtodo this isn't being used at the moment
    if (m_catchFallTimer < timeStep)
    {
      //loosen character between balance failing and catchfall activation
      m_body->resetEffectors(kResetCalibrations); // limbs todo...
      m_body->setStiffness(8.f, 1.f, bvmask_Full, NULL);
    }
    m_catchFallTimer += timeStep;

    if (m_catchFallTimer > 0.0f)
    {
      NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
      Assert(rdsTask);
      NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
      Assert(catchFallTask);


      //catch fall looks ok forwards/forwardsside/backwards but side/sidebackwards looks better with rds

      //if (rage::Abs(m_character->m_COMTM.b.Dot(m_character->m_COMrotvel)) > 2.f)
      //{
      //do rollup and/or catchfall: Catchfall if only falling forwards. As soon as some sideways/backwards movement go into rolldownstairs
      // if hands and knees is set only do catchFall (rolldownstairs will be called automatically from catchFall if the character moves to fast after landing)
      rage::Matrix34 tmCom;
      getSpine()->getSpine1Part()->getBoundMatrix(&tmCom); 
      if (m_character->m_gUp.Dot(tmCom.c) < 0.8f && !catchFallTask->m_handsAndKnees) //not facingdown
      {
        if (!rdsTask->isActive())
        {
          rdsTask->updateBehaviourMessage(NULL); // sets values to defaults

          rdsTask->m_parameters.m_AsymmetricalLegs = 0.4f;
          rdsTask->m_parameters.m_Stiffness = 7.f;
          rdsTask->m_parameters.m_ForceMag = 0.4f;
          rdsTask->m_parameters.m_AsymmetricalForces = 0.0f;//unused
          rdsTask->m_parameters.m_UseArmsToSlowDown = -0.9f;

          rdsTask->m_parameters.m_ArmReachAmount = 1.4f;
          rdsTask->m_parameters.m_SpinWhenInAir = true;
          rdsTask->m_parameters.m_LegPush = 0.2f;
          rdsTask->m_parameters.m_ArmL = 0.6f;

          float legAssmetry = m_character->getRandom().GetRanged(0.2f, 0.8f);
          rdsTask->m_parameters.m_AsymmetricalLegs = legAssmetry;
          rdsTask->m_parameters.m_useVelocityOfObjectBelow = true;
          rdsTask->m_parameters.m_useRelativeVelocity = true;
          if (balColReactTask->isActive())
          {
            if (balColReactTask->m_balancerState == bal_Slump)
            {
              rdsTask->m_parameters.m_Stiffness = 5.f;
              rdsTask->m_parameters.m_ArmReachAmount = 1.0f;
            }
          }
          NmRsCBUYanked* yankedTask = (NmRsCBUYanked*)m_cbuParent->m_tasks[bvid_yanked];
          Assert(yankedTask);
          if (!yankedTask->isActive())//mmmmYanked don't activate rds
            rdsTask->activate();
        }
      }
      if (!catchFallTask->isActive())
      {
        ////Turn off headlook
        //if (m_parameters.m_useHeadLook) 
        //  headLookTask->deactivate();//de-activate once only.  (HeadLook used by CatchFall)
        catchFallTask->updateBehaviourMessage(NULL);// sets values to defaults
        float defaultBodyStiffness = 9.f;  // for the bodyBalance
        catchFallTask->m_parameters.m_legsStiffness = 9.f * 2.5f/defaultBodyStiffness;
        catchFallTask->m_parameters.m_torsoStiffness = 9.f * 7.f/defaultBodyStiffness;
        catchFallTask->m_parameters.m_armsStiffness = 9.f * 10.f/defaultBodyStiffness;
        if (balColReactTask->isActive())
        {
          if (balColReactTask->m_balancerState == bal_Slump)
          {
            float legStiffness = rage::Min(balColReactTask->m_slumpStiffLKnee,balColReactTask->m_slumpStiffRKnee);
            legStiffness = rage::Max(5.f,legStiffness);
            catchFallTask->m_parameters.m_legsStiffness = legStiffness;
            catchFallTask->m_parameters.m_armsStiffness = 6.f;
          }
        }
        if (catchFallTask->m_handsAndKnees)
        {
          m_body->resetEffectors(kResetCalibrations);

          //We used to start the catchFall with an upperbody response only here - we wanted the dynBalancer to keep stepping for a while
          //as the character falls over.  CatchFall used to modify this parameter internally. 
          catchFallTask->m_parameters.m_effectorMask = bvmask_Full;
          //float defaultBodyStiffness = 11.f;  // for the bodyBalance
          catchFallTask->m_parameters.m_legsStiffness = 5.5f;//defaultBodyStiffness * 5.5f/defaultBodyStiffness;
          catchFallTask->m_parameters.m_torsoStiffness = 10.f;//defaultBodyStiffness * 10.f/defaultBodyStiffness;
          catchFallTask->m_parameters.m_armsStiffness = 15.f;//defaultBodyStiffness * 15.f/defaultBodyStiffness;

          NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
          Assert(dynamicBalancerTask);
          if (dynamicBalancerTask->isActive())
          {
            dynamicBalancerTask->setOpposeGravityAnkles(0.85f);
            dynamicBalancerTask->setOpposeGravityLegs(0.85f);
            dynamicBalancerTask->setLeftLegStiffness(9.5f);
            dynamicBalancerTask->setRightLegStiffness(9.5f);
            dynamicBalancerTask->setLowerBodyGravityOpposition(m_body);
            dynamicBalancerTask->calibrateLowerBodyEffectors(m_body);
          }

        }
        catchFallTask->activate();
      }
    }
    //if (balColReactTask->isActive())
    //  balColReactTask->m_balancerState = bal_End;
  }

#if ART_ENABLE_BSPY
  void NmRsCBUStaggerFall::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    static const char* legStateStrings[] =
    {
      "SwingLeg",
      "FootStrike",
      "Balance",
      "PushOff",
      "PushOffKnee",
      "ToeTimer",
    };

    static const char* failTypeStrings[] =
    {
#define BAL_NAME_ACTION(_name) #_name ,
      BAL_STATES(BAL_NAME_ACTION)
#undef BAL_NAME_ACTION
    };

    bspyTaskVar(m_parameters.m_armStiffness, true);
    bspyTaskVar(m_parameters.m_armDamping, true);
    bspyTaskVar(m_parameters.m_headLookPos, true);
    bspyTaskVar(m_parameters.m_headLookInstanceIndex, true);
    bspyTaskVar(m_parameters.m_spineStiffness, true);
    bspyTaskVar(m_parameters.m_headLookAtVelProb, true);

    bspyTaskVar(m_parameters.m_spineDamping, true);
    bspyTaskVar(m_parameters.m_turnOffProb, true);
    bspyTaskVar(m_parameters.m_turn2TargetProb, true);
    bspyTaskVar(m_parameters.m_turn2VelProb, true);
    bspyTaskVar(m_parameters.m_turnAwayProb, true);
    bspyTaskVar(m_parameters.m_turnLeftProb, true);
    bspyTaskVar(m_parameters.m_turnRightProb, true);

    bspyTaskVar(m_parameters.m_armStiffnessStart, true);
    bspyTaskVar(m_parameters.m_armDampingStart, true);
    bspyTaskVar(m_parameters.m_spineStiffnessStart, true);
    bspyTaskVar(m_parameters.m_spineDampingStart, true);

    bspyTaskVar(m_parameters.m_timeAtStartValues, true);
    bspyTaskVar(m_parameters.m_rampTimeFromStartValues, true);
    bspyTaskVar(m_parameters.m_staggerStepProb, true);
    bspyTaskVar(m_parameters.m_timeStartEnd, true);
    bspyTaskVar(m_parameters.m_rampTimeToEndValues, true);
    bspyTaskVar(m_parameters.m_lowerBodyStiffness, true);
    bspyTaskVar(m_parameters.m_lowerBodyStiffnessEnd, true);
    bspyTaskVar(m_parameters.m_predictionTime, true);

    bspyTaskVar(m_parameters.m_perStepReduction1, true);
    bspyTaskVar(m_parameters.m_leanInDirRate, true);
    bspyTaskVar(m_parameters.m_leanInDirMaxF, true);
    bspyTaskVar(m_parameters.m_leanInDirMaxB, true);
    bspyTaskVar(m_parameters.m_leanHipsMaxF, true);
    bspyTaskVar(m_parameters.m_leanHipsMaxB, true);
    bspyTaskVar(m_parameters.m_lean2multF, true);
    bspyTaskVar(m_parameters.m_lean2multB, true);
    bspyTaskVar(m_parameters.pushOffDist, true);
    bspyTaskVar(m_parameters.maxPushoffVel, true);

    bspyTaskVar(m_parameters.m_hipBendMult, true);
    bspyTaskVar(m_parameters.m_spineBendMult, true);
    bspyTaskVar(m_parameters.m_alwaysBendForwards, true);
    bspyTaskVar(m_parameters.m_stepsTillStartEnd, true);

    bspyTaskVar(m_parameters.m_upperBodyReaction, true);
    bspyTaskVar(m_parameters.m_useHeadLook, true);
    bspyTaskVar(m_parameters.m_useBodyTurn, true);

    bspyTaskVar(m_armStiffness, false);
    bspyTaskVar(m_armDamping, false);
    bspyTaskVar(m_spineStiffness, false);
    bspyTaskVar(m_spineDamping, false);

    bspyTaskVar(m_lastFootState, false);
    bspyTaskVar(m_numSteps, false);
    bspyTaskVar(m_staggerTimer, false);
    bspyTaskVar(m_lookInVelDir, false);


    //bSpy only
    bspyTaskVar_StringEnum(spyLeftLegState, legStateStrings, false);
    bspyTaskVar_StringEnum(spyRightLegState, legStateStrings, false);

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
