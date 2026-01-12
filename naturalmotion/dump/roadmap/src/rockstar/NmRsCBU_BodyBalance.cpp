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
#include "NmRsCBU_BodyBalance.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_Catchfall.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_Shot.h"


namespace ART
{
  NmRsCBUBodyBalance::NmRsCBUBodyBalance(ART::MemoryManager* services) : CBUTaskBase(services, bvid_bodyBalance)
  {
    initialiseCustomVariables();
  }

  NmRsCBUBodyBalance::~NmRsCBUBodyBalance()
  {
  }

  void NmRsCBUBodyBalance::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_armsOutOnPushThresh = 0.03f;
    m_useCOMAngVel = false;
    m_turnStepCount = 4;
    m_turnLeft = true;
  }


  void NmRsCBUBodyBalance::onActivate()
  {
    Assert(m_character);

    m_bendElbowsTimer = 0.f;

    //BodyBalance Entry
    NM_RS_DBG_LOGF(L"- Body Balance Entry");
    m_characterIsFalling = false;

    m_body->resetEffectors(kResetAngles);// | kResetCalibrations);

    if (m_parameters.m_useHeadLook) 
    {         
      NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      Assert(headLookTask);
      headLookTask->updateBehaviourMessage(NULL); // initialise the parameters

      headLookTask->m_parameters.m_pos = m_parameters.m_headLookPos;
      headLookTask->m_parameters.m_vel.Set(0.f, 0.f, 0.f);
      headLookTask->m_parameters.m_stiffness = m_parameters.m_spineStiffness+1.0f;//11.0f);
      headLookTask->m_parameters.m_damping = 1.0f;
      headLookTask->m_parameters.m_alwaysLook = true;
      headLookTask->m_parameters.m_instanceIndex = m_parameters.m_headLookInstanceIndex;
      // .. and activate it
      headLookTask->activate();
    }

    // seconds before we allow armsOutOnPush to turn off
    m_armsOutOnPushTimer = m_parameters.m_armsOutOnPushTimeout;

    getSpineInputData()->getUpperNeck()->setOpposeGravity(1.f);
    getSpineInputData()->getLowerNeck()->setOpposeGravity(1.f);

    getSpineInputData()->getSpine0()->setOpposeGravity(2.f); // just to keep the back from limboing too much
    getSpineInputData()->getSpine1()->setOpposeGravity(2.f);
    getSpineInputData()->getSpine2()->setOpposeGravity(2.f);

    // try to stop hunching shoulders
    getLeftArmInputData()->getClavicle()->setOpposeGravity(2.f);
    getRightArmInputData()->getClavicle()->setOpposeGravity(2.f);

    //hd: set clavicles to zero pose angles if available
    if (getLeftArm()->getClavicle()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose)) 
      getLeftArmInputData()->getClavicle()->blendToZeroPose((NmRsEffectorBase*)getLeftArm()->getClavicle(), 1.0f);
    else
      getLeftArmInputData()->getClavicle()->setDesiredLean2(-0.4f);

    if (getRightArm()->getClavicle()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose)) 
      getRightArmInputData()->getClavicle()->blendToZeroPose((NmRsEffectorBase*)getRightArm()->getClavicle(), 1.0f);
    else
      getRightArmInputData()->getClavicle()->setDesiredLean2(-0.4f);

    rage::Matrix34 tmCom;
    if (m_useCOMAngVel) //Use COM angVel
    {
      tmCom.Set(m_character->m_COMTM);
      //make angvel from COM or Spine2
      m_bodyRight = tmCom.a;
      m_bodyUp = tmCom.b;
      m_bodyBack = tmCom.c;
    }
    else //Use Spine2 angVel
    {
      getSpine()->getSpine2Part()->getBoundMatrix(&tmCom); 
      //make angvel from COM or Spine2
      m_bodyUp = tmCom.a;
      m_bodyRight = -tmCom.b;
      m_bodyBack = tmCom.c;
    }

    //get angles
    ////body123 //NB gets sign of twist wrong half the time
    if (m_character->m_gUp.y > 0.9f) //y_Up
    {
      float arcsin = m_bodyBack.y;
      m_oldqSom = rage::AsinfSafe(arcsin);
      float c2 = rage::Cosf(m_oldqSom);
      Assert(rage::Abs(c2) > 1e-10f);
      arcsin = -m_bodyRight.y / c2;
      m_oldqTilt = rage::AsinfSafe(arcsin);
      Assert(rage::Abs(c2) > 1e-10f);
      arcsin = -m_bodyBack.z / c2;
      m_oldqTwist = rage::AsinfSafe(arcsin);
    }
    else
    {
      float arcsin = m_bodyBack.z;
      m_oldqSom = rage::AsinfSafe(arcsin);
      float c2 = rage::Cosf(m_oldqSom);
      Assert(rage::Abs(c2) > 1e-10f);
      arcsin = -m_bodyRight.z / c2;
      m_oldqTilt = rage::AsinfSafe(arcsin);
      Assert(rage::Abs(c2) > 1e-10f);
      arcsin = -m_bodyBack.y / c2;
      m_oldqTwist = rage::AsinfSafe(arcsin);
    }

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);

    if (!balColReactTask->isActive() || balColReactTask->m_balancerState == bal_Normal)
    {
      dynamicBalancerTask->setLeftLegStiffness(12.f);
      dynamicBalancerTask->setRightLegStiffness(12.f);
    }
    dynamicBalancerTask->activate();
    if (dynamicBalancerTask->isActive())
    {
      dynamicBalancerTask->setOpposeGravityAnkles(1.f);
      dynamicBalancerTask->setOpposeGravityLegs(1.f);
    }

    //For Arms out randomness
    m_maxLean2OnPush = m_character->getRandom().GetRanged(0.0f, 0.8f);//Reset per foot step was 0.0,1.0
    m_lastFootState = NmRsCBUDynBal_FootState::kNotStepping;//Force armsOut to be randomized on next foot step
    //For Look at type randomness
    //m_parameters.m_headLookAtVelProb 0 = no look at vel. >1 = always look at vel
    //if negative (-1) m_parameters.m_headLookAtVelProb set to m_lookAtRandom i.e. randomizeed
    m_lookAtRandom = m_character->getRandom().GetRanged(0.0f, 1.0f); //Set here only        
    m_lookAtTimer = 0.f; //Force lookAt to be randomized on tick
    //For Turn type randomness
    m_randomTurn = m_character->getRandom().GetFloat();
    m_turnTimer = 0.0f;//Force bodyTurn to be randomized on tick 

    //If using HandsKnees catchfall: bend the character forward at hips to prepare for catch fall landing
    m_bendTimer = 0.f;
  }

  void NmRsCBUBodyBalance::onDeactivate()
  {
    Assert(m_character);

    //De-activate subTasks
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    if (dynamicBalancerTask->isActive() && m_character->noBehavioursUsingDynBalance())
      dynamicBalancerTask->deactivate();

    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    if (catchFallTask->isActive())
      catchFallTask->deactivate();

    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    if (headLookTask->isActive())
      headLookTask->deactivate();

  }

  CBUTaskReturn NmRsCBUBodyBalance::onTick(float timeStep)
  { 
    NM_RS_DBG_LOGF(L"- Body Balance During")
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    m_characterIsFalling = (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK);//MMMMHandsKnees replace all right with left//dynamicBalancerTask->hasFailed();

    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);

    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);

    NmRsCBUShot* shotTask = (NmRsCBUShot*)m_cbuParent->m_tasks[bvid_shot];
    Assert(shotTask);

    if (m_characterIsFalling)
    {  
      bool dontDoFall = (balColReactTask->isActive() && (balColReactTask->m_balancerState == bal_Drape || balColReactTask->m_balancerState == bal_DrapeForward));// || balColReactTask->m_balancerState == bal_DrapeGlancingSpin);
      dontDoFall = dontDoFall || shotTask->isActive();
      if (!dontDoFall)
        falling(timeStep);

    }
    else
    {

      //dynamicBalancerTask->setAnkleEquilibrium(-0.041f);
      //if (m_character->getBodyIdentifier() == rdrCowboy)
      //  dynamicBalancerTask->setAnkleEquilibrium(-0.1f);

      //DoBalance During
      //   //mmmmDrunkTest individually give nice colour to the hips movement
      //static float hipPitch = -0.0f;
      //dynamicBalancerTask->setHipPitch(hipPitch);
      //static float hipRoll = -0.0f;
      //dynamicBalancerTask->setHipRoll(hipRoll);
      //static float hipYaw = -0.0f;
      //dynamicBalancerTask->setHipYaw(hipYaw);
      //   //mmmmDrunkTest individually give nice colour to the hips movement

      //m_parameters.m_backwardsAutoTurn:  AutoTurn around if going backwards
      //m_parameters.m_backwardsArms:  Change arm stiffnesses and bendElbowGait if going backwards (bending arms more when going backwards gives better look)
      //                               Turnoff spine twisting
      bool movingBackwards = false;
      if (m_parameters.m_backwardsAutoTurn || m_parameters.m_backwardsArms)
      {
        rage::Vector3 comVel = m_character->m_COMvel;
        rage::Vector3 back = m_character->m_COMTM.c;
        m_character->levelVector(comVel);
        m_character->levelVector(back);
        comVel.Normalize();
        back.Normalize();
        movingBackwards = (comVel.Dot(back) > 0.5f);
      }

      //If stepping with a different foot randomize the amount of arms(s) up if pushed from other side(behind).
      if ((dynamicBalancerTask->footState() != NmRsCBUDynBal_FootState::kNotStepping) 
        && (m_lastFootState != dynamicBalancerTask->footState()))
      {
        m_lastFootState = dynamicBalancerTask->footState();
        m_maxLean2OnPush = m_character->getRandom().GetRanged(0.0f, 0.8f);

        if (movingBackwards)
          m_turnStepCount -= 1;
      }

      getSpineInputData()->getSpine0()->setMuscleStiffness(1.5f);
      getSpineInputData()->getSpine1()->setMuscleStiffness(1.5f);
      getSpineInputData()->getSpine2()->setMuscleStiffness(1.5f);
      getSpineInputData()->getSpine3()->setMuscleStiffness(1.5f);
      getSpineInputData()->getLowerNeck()->setMuscleStiffness(1.5f);
      getSpineInputData()->getUpperNeck()->setMuscleStiffness(1.5f);

      ////mmmmDrunk
      //getSpine()->getSpine0()->setMuscleStiffness(0.75f);
      //getSpine()->getSpine1()->setMuscleStiffness(0.75f);
      //getSpine()->getSpine2()->setMuscleStiffness(0.75f);
      //getSpine()->getSpine3()->setMuscleStiffness(0.75f);

      NM_RS_DBG_LOGF(L" Arm/Neck Stiffness is= %.4f", m_parameters.m_armStiffness);

      float bendElbowsGait = m_parameters.m_bendElbowsGait;

      getSpineInputData()->setStiffness(m_parameters.m_spineStiffness, m_parameters.m_spineDamping);
      getSpineInputData()->getLowerNeck()->setStiffness(m_parameters.m_armStiffness, 1.0f);
      getSpineInputData()->getUpperNeck()->setStiffness(m_parameters.m_armStiffness, 1.0f);
      getLeftArmInputData()->setStiffness(m_parameters.m_armStiffness, m_parameters.m_armDamping);
      getLeftArmInputData()->getClavicle()->setStiffness(9.0f, 1.0f);
      getRightArmInputData()->setStiffness(m_parameters.m_armStiffness, m_parameters.m_armDamping);
      getRightArmInputData()->getClavicle()->setStiffness(9.0f, 1.0f);

      //Change arm stiffnesses and bendElbowGait if going backwards (bending arms more when going backwards gives better look)
      if (movingBackwards && m_parameters.m_backwardsArms)
      {
        //static float rarms = 3.f;//mmmmtodo scale this around 9.f
        getLeftArmInputData()->getElbow()->setStiffness(m_parameters.m_armStiffness, m_parameters.m_armDamping);
        getRightArmInputData()->getElbow()->setStiffness(m_parameters.m_armStiffness, m_parameters.m_armDamping);
        bendElbowsGait = 1.5f;
      }

      float elbowStiff = m_parameters.m_armStiffness * 0.9f;
      float elbowDamp = 2.0f * elbowStiff * (m_parameters.m_armDamping * 0.8f);
      elbowStiff = elbowStiff * elbowStiff;

      getLeftArmInputData()->getElbow()->setMuscleDamping(elbowDamp);
      getLeftArmInputData()->getElbow()->setMuscleStrength(elbowStiff);
      getRightArmInputData()->getElbow()->setMuscleDamping(elbowDamp);
      getRightArmInputData()->getElbow()->setMuscleStrength(elbowStiff);

      float averageSpeed = rage::Sqrtf(2.0f * m_character->getKineticEnergyPerKilo_RelativeVelocity()) * 0.5f;

      if (m_parameters.m_useHeadLook) 
      {
        if (!headLookTask->isActive())//May not have been activated in bodyBalance::Activate
        {
          headLookTask->updateBehaviourMessage(NULL);
          headLookTask->activate();
        }
        //Look at target
        rage::Vector3 posTarget = m_parameters.m_headLookPos;
        headLookTask->m_parameters.m_pos = posTarget;
        headLookTask->m_parameters.m_vel.Set(0,0,0);
        headLookTask->m_parameters.m_stiffness = m_parameters.m_spineStiffness+1.0f;//11.0f);
        headLookTask->m_parameters.m_damping = 1.0f;
        headLookTask->m_parameters.m_alwaysLook = true;
        headLookTask->m_parameters.m_instanceIndex = m_parameters.m_headLookInstanceIndex;

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
            headLookTask->m_parameters.m_pos = posTarget;
            headLookTask->m_parameters.m_instanceIndex = -1;
          }
        }
        else
        {
          m_lookAtTimer = 0.f; //Force randomization of LookAt type when the character begins stepping again
        }
      } 

      NM_RS_DBG_LOGF(L">> Body Balance >> DoBalance");


      float motionMultiplier = rage::Clamp(2.f*averageSpeed, 0.001f, 1.0f); 
      float elbowAngle = m_parameters.m_elbowAngleOnContact;

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

      //AutoTurn around if going backwards
      if (movingBackwards && m_parameters.m_backwardsAutoTurn)
      {
        if (m_turnStepCount >= 3)
        {
          m_turnLeft = (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep);
          m_turnStepCount = 3;
        }
        rage::Vector3 posTargetLocal;
        rage::Vector3 dirTarget;
        int instanceIndex = m_character->getFirstInstance()->GetLevelIndex();
        static float y = 2.f;
        static float z = 0.f;
        if (m_turnLeft) 
          posTargetLocal.Set(-2.0f, y, z);//Turn Left
        else
          posTargetLocal.Set(2.0f, y, z);//Turn Right

        m_character->instanceToWorldSpace(&dirTarget, posTargetLocal, instanceIndex);
        dirTarget -= m_character->m_COM;
        m_character->levelVector(dirTarget);
        dynamicBalancerTask->useCustomTurnDir(true, dirTarget);
#if ART_ENABLE_BSPY
        m_character->bspyDrawLine(getSpine()->getHeadPart()->getPosition(), getSpine()->getHeadPart()->getPosition()+dirTarget, rage::Vector3(1,0,0));
#endif
      } 
      else
      {
        m_turnStepCount = 4;
      }

      if (motionMultiplier < 0.3f)
        dynamicBalancerTask->useCustomTurnDir(false, noTurn);

      getSpineInputData()->setNeckAngles(0.f, 0.f, 0.f);

      rage::Matrix34 tmCom(m_character->m_COMTM);
      m_bodyRight = tmCom.a;
      m_bodyUp = tmCom.b;
      m_bodyBack = tmCom.c;
      m_bodyUp.Cross(m_character->m_gUp);

      // Used to give f(angle from upright) arms out and 
      //  to turn off f(angVel) arms out when returning to balance 
      float forwardOrBack = -2.f*rage::Sinf(m_bodyUp.Dot(m_bodyRight));//mmmmNote this should be asin or for approx no sin
      float leftOrRight = 2.f*rage::Sinf(m_bodyUp.Dot(m_bodyBack));//mmmmNote this should be asin or for approx no sin

      //apply s shaped lean1
      float spineLean1 = 0.f;
      getSpineInputData()->getSpine0()->setDesiredLean1(0.05f*spineLean1-0.07f);
      getSpineInputData()->getSpine1()->setDesiredLean1(0.05f*spineLean1+0.25f);
      getSpineInputData()->getSpine2()->setDesiredLean1(0.05f*spineLean1-0.07f);
      getSpineInputData()->getSpine3()->setDesiredLean1(0.05f*spineLean1+0.07f);

      getSpineInputData()->getSpine1()->setDesiredLean2(0.0f);
      getSpineInputData()->getSpine2()->setDesiredLean2(0.0f);
      getSpineInputData()->getSpine3()->setDesiredLean2(0.0f);

      //Twist the spine proportionally to the split of the legs
      //Mix of actual and desired split of legs.  Wanted a smooth twist that occupies the whole of the time taken to step
      //  proportional to actual split of legs
      float spineTwist = nmrsGetActualLean1(getLeftLeg()->getHip()) - nmrsGetActualLean1(getRightLeg()->getHip());
      //  Add in some proportional to desired split of legs  

      // todo danger! pulling desired lean1 from hips as a proxy for balancer state. not strictly limbs compatible - will return last tick's values.
      spineTwist = spineTwist+0.15f*(nmrsGetDesiredLean1(getLeftLeg()->getHip()) - nmrsGetDesiredLean1(getRightLeg()->getHip()));
      spineTwist = rage::Clamp(spineTwist, -0.8f, 0.8f);
      if (!movingBackwards)
      {
        getSpineInputData()->getSpine0()->setDesiredTwist(0.35f*spineTwist);
        getSpineInputData()->getSpine1()->setDesiredTwist(0.3f*spineTwist);
        getSpineInputData()->getSpine2()->setDesiredTwist(0.2f*spineTwist);
        getSpineInputData()->getSpine3()->setDesiredTwist(0.15f*spineTwist);
      }

      //Arms Out to Balance when pushed start
      //f(angle from upright) arms out
      //inhibit arms out behaviour if not been pushed using a threshold (Maximum arms out controlled by clamp later on)
      float absForwardOrBack = rage::Abs(forwardOrBack);
      float absLeftOrRight = rage::Abs(leftOrRight);
      float angleX = rage::Clamp(absForwardOrBack, m_parameters.m_somersaultAngleThreshold, 0.8f) - m_parameters.m_somersaultAngleThreshold; //based on forward or back angle
      float angleZ = rage::Clamp(absLeftOrRight, m_parameters.m_sideSomersaultAngleThreshold, 0.8f) - m_parameters.m_sideSomersaultAngleThreshold; //based on left or right angle

      if (m_useCOMAngVel) //Use COM angVel
      {
        tmCom.Set(m_character->m_COMTM);
        //make angvel from COM or Spine2
        m_bodyRight = tmCom.a;
        m_bodyUp = tmCom.b;
        m_bodyBack = tmCom.c;
      }
      else //Use Spine2 angVel
      {
        getSpine()->getSpine2Part()->getBoundMatrix(&tmCom); 
        //make angvel from COM or Spine2
        m_bodyUp = tmCom.a;
        m_bodyRight = -tmCom.b;
        m_bodyBack = tmCom.c;
      }

      //get angles
      ////body123 //NB gets sign of twist wrong half the time
      float qSom;
      float qTilt;
      float qTwist;
      if (m_character->m_gUp.y > 0.9f) //y_Up
      {
        float arcsin = m_bodyBack.y;
        qSom = rage::AsinfSafe(arcsin);
        float c2 = rage::Cosf(qSom);
        Assert(rage::Abs(c2) > 1e-10f);
        arcsin = -m_bodyRight.y / c2;
        qTilt = rage::AsinfSafe(arcsin);
        Assert(rage::Abs(c2) > 1e-10f);
        arcsin = -m_bodyBack.z / c2;
        qTwist = rage::AsinfSafe(arcsin);
      }
      else//z_Up
      {
        float arcsin = m_bodyBack.z;
        qSom = rage::AsinfSafe(arcsin);
        float c2 = rage::Cosf(qSom);
        Assert(rage::Abs(c2) > 1e-10f);
        arcsin = -m_bodyRight.z / c2;
        qTilt = rage::AsinfSafe(arcsin);
        Assert(rage::Abs(c2) > 1e-10f);
        arcsin = -m_bodyBack.y / c2;
        qTwist = rage::AsinfSafe(arcsin);
      }

      Assert(rage::Abs(timeStep) > 1e-10f);
      float wTilt = -(qTilt-m_oldqTilt)/timeStep;
      float wSom = -(qSom-m_oldqSom)/timeStep;
      float wTwist = -(qTwist-m_oldqTwist)/timeStep;

      NM_RS_DBG_LOGF(L" forwardOrBack = %.4f", forwardOrBack);
      NM_RS_DBG_LOGF(L"      ");
      NM_RS_DBG_LOGF(L" leftOrRight = %.4f", leftOrRight);
      NM_RS_DBG_LOGF(L"      ");
      NM_RS_DBG_LOGF(L"qSom = %.4f", qSom);
      NM_RS_DBG_LOGF(L"qTilt = %.4f", qTilt);
      NM_RS_DBG_LOGF(L"qTwist = %.4f", qTwist);
      NM_RS_DBG_LOGF(L"wSom = %.4f", wSom);
      NM_RS_DBG_LOGF(L"wTilt = %.4f", wTilt);
      NM_RS_DBG_LOGF(L"wTwist = %.4f", wTwist);

//#if NM_RS_ENABLE_DEBUGDRAW & 0 //mmmmTodo convert to bspy
//      //Draw angVel and angVel thresholds
//      if (rage::NMRenderBuffer::getInstance())
//      {
//        rage::Vector3 comPos;
//        if (m_useCOMAngVel) //Use COM angVel
//        {
//          comPos = m_character->m_COM;
//        }
//        else
//        {
//          comPos = getSpine()->getSpine2Part()->getPosition(); 
//        }
//
//        //draw angular vel and thresholds
//        rage::Vector3 col(0.5, 0, 0);//Red
//        rage::Vector3 angVelComponent = comPos + m_bodyRight*m_parameters.m_somersaultAngVelThreshold;
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//        col.Set(1,0,0);
//        angVelComponent = comPos + m_bodyRight*rage::Abs(wSom);
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//
//        col.Set(0, 0.5, 0);//Green
//        angVelComponent = comPos + m_bodyBack*m_parameters.m_sideSomersaultAngVelThreshold;
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//        col.Set(0,1,0);
//        angVelComponent = comPos + m_bodyBack*rage::Abs(wTilt);
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//
//        col.Set(0, 0, 0.5);//Blue
//        angVelComponent = comPos + m_bodyUp*m_parameters.m_twistAngVelThreshold;
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//        col.Set(0,0,1);
//        angVelComponent = comPos + m_bodyUp*rage::Abs(wTwist);
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//
//
//        //draw angles at head
//        comPos = getSpine()->getHeadPart()->getPosition(); 
//        comPos.Set(0,0,0);
//        col.Set(0.5, 0, 0);//Red
//        if (m_useCOMAngVel){ //Use COM angVel
//          angVelComponent.Set(1,0,0);
//          angVelComponent.RotateY(-m_parameters.m_sideSomersaultAngleThreshold);
//        }
//        else
//        {
//          angVelComponent.Set(-1,0,0);
//          angVelComponent.RotateY(m_parameters.m_sideSomersaultAngleThreshold);
//        }
//        angVelComponent += comPos;
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//        col.Set(1, 0, 0);//Red
//        if (m_useCOMAngVel){ //Use COM angVel
//          angVelComponent.Set(1,0,0);
//          angVelComponent.RotateY(-rage::Abs(leftOrRight));
//        }
//        else
//        {
//          angVelComponent.Set(-1,0,0);
//          angVelComponent.RotateY(rage::Abs(leftOrRight));
//        }
//        angVelComponent += comPos;
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//
//        col.Set(0, 0.5, 0);//Green
//        angVelComponent.Set(0,-1,0);
//        angVelComponent.RotateX(-m_parameters.m_somersaultAngleThreshold);
//        angVelComponent += comPos;
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//        col.Set(0, 1, 0);//Green
//        angVelComponent.Set(0,-1,0);
//        angVelComponent.RotateX(-rage::Abs(forwardOrBack));
//        angVelComponent += comPos;
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//
//
//
//        //Draw Axes in ground space for angles
//        col.Set(0.5, 0, 0);//Red
//        angVelComponent.Set(-1,0,0);
//        if (m_useCOMAngVel) //Use COM angVel
//          angVelComponent.Set(1,0,0);
//        angVelComponent += comPos;
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//
//        col.Set(0, 0.5, 0);//Green
//        angVelComponent.Set(0,-1,0);
//        angVelComponent += comPos;
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//
//        col.Set(0, 0, 0.5);//Blue
//        angVelComponent.Set(0,0,1);
//        angVelComponent += comPos;
//        NM_RS_CBU_DRAWARROW( comPos, angVelComponent, col);
//      }
//#endif // NM_RS_ENABLE_DEBUGDRAW

      m_oldqTilt = qTilt; 
      m_oldqSom = qSom ;
      m_oldqTwist = qTwist; 

      float angVelSom = wSom;
      float angVelTwist = wTwist;
      float angVelTilt = wTilt;
#if ART_ENABLE_BSPY
      m_angVel.Set(angVelSom,angVelTwist,angVelTilt);
#endif

      // inhibit arms out on balance if returning to upright position
      // TDL have balance deadzone
      float turnOnangVelSom = 1.f; 
      if (angVelSom*forwardOrBack < 0.f) //if not same sign then character moving towards balance point - turn off arms up
      {
        turnOnangVelSom = m_parameters.m_returningToBalanceArmsOut;
      } 
      float turnOnangVelTilt = m_parameters.m_returningToBalanceArmsOut; 
      if (angVelTilt*leftOrRight < 0.f) //if same sign then character moving towards balance point - turn off arms up
      {
        turnOnangVelTilt = 1.f;
      }

      angVelSom = rage::Abs(angVelSom);
      angVelTwist = rage::Abs(angVelTwist);
      angVelTilt = rage::Abs(angVelTilt);

      //inhibit arms out behaviour if not been pushed using a threshold (Maximum arms out controlled by clamp later on)
      angVelSom = rage::Clamp(angVelSom,m_parameters.m_somersaultAngVelThreshold,7.f) - m_parameters.m_somersaultAngVelThreshold;
      angVelTwist = rage::Clamp(angVelTwist,m_parameters.m_twistAngVelThreshold,7.f) - m_parameters.m_twistAngVelThreshold;
      angVelTilt = rage::Clamp(angVelTilt,m_parameters.m_sideSomersaultAngVelThreshold,7.f) - m_parameters.m_sideSomersaultAngVelThreshold;

#if ART_ENABLE_BSPY
      m_angVelClamped.Set(angVelSom,angVelTwist,angVelTilt);
#endif
      NM_RS_DBG_LOGF(L"returningToBalanceArmsOut = %.4f", m_parameters.m_returningToBalanceArmsOut);
      NM_RS_DBG_LOGF(L"somersaultAngle = %.4f", m_parameters.m_somersaultAngle);
      NM_RS_DBG_LOGF(L"sideSomersaultAngle = %.4f", m_parameters.m_sideSomersaultAngle);
      NM_RS_DBG_LOGF(L"somersaultAngVel = %.4f", m_parameters.m_somersaultAngVel);
      NM_RS_DBG_LOGF(L"twistAngVel = %.4f", m_parameters.m_twistAngVel);
      NM_RS_DBG_LOGF(L"sideSomersaultAngVel = %.4f", m_parameters.m_sideSomersaultAngVel);

      NM_RS_DBG_LOGF(L"turnOnangVelSom = %.4f", turnOnangVelSom);
      NM_RS_DBG_LOGF(L"turnOnangVelTilt = %.4f", turnOnangVelTilt);
      NM_RS_DBG_LOGF(L"Clamped angleSom = %.4f", angleX);
      NM_RS_DBG_LOGF(L"Clamped angleSideSom = %.4f", angleZ);
      NM_RS_DBG_LOGF(L"Clamped angVelSom = %.4f", angVelSom);
      NM_RS_DBG_LOGF(L"Clamped angVelTwist = %.4f", angVelTwist);
      NM_RS_DBG_LOGF(L"Clamped angVelSideSom = %.4f", angVelTilt);

      //symmetrical add2Lean2 based on angle from upright and angular velocity 
      float add2Lean2 = 0.f;
      float add2Lean2Side = 0.f;
      //add in f(angle) of body 
      add2Lean2 = - m_parameters.m_somersaultAngle*angleX*turnOnangVelSom; //added turnOnangVelSom
      add2Lean2 = add2Lean2 - m_parameters.m_sideSomersaultAngle*angleZ*turnOnangVelTilt; //added turnOnangVelTilt
      //add in f(angvel) of body 
      add2Lean2 = add2Lean2 - m_parameters.m_somersaultAngVel*angVelSom*turnOnangVelSom;
      add2Lean2 = add2Lean2 - m_parameters.m_twistAngVel*angVelTwist;
      add2Lean2 = add2Lean2 - m_parameters.m_sideSomersaultAngVel*angVelTilt*turnOnangVelTilt;

      NM_RS_DBG_LOGF(L"add2Lean2 = %.4f", add2Lean2);

      //Arms Out to Balance when pushed end

      //Swing arm in time with opposite leg
      // For shoulder and elbow:
      //Desired angle = Actual angle + k*angleDelta ie to (sort of) swing about gravity/dynamic equilibrium angle (well lean1 for shoulder) 
      // angleDelta is the difference between the desired and actual hip lean1 angle (ie a measure of where the hip will be) but 
      // slightly fudged by weighting the Actual lean1 more
      //Shoulder only: adds to angleDelta a proportional to actual leg split part slightly fudged by weighting the Actual lean1 off the opposite leg more.
      //This helped slow/smooth the arm swing and 'extend the swing'/'slow the return' where the lower body balance takes a larger/longer step than 1st thought
      // eg when walking down a slope.  TDL: maybe add this to the elbow aswell?  
      //TDL: If one hand constrained then swing slightly more with free arm 
      float a = 0.6f; // actual leg split multiplier (shoulder only)
      float angleDelta;
      //Left Arm
      float fudge1 = 1.2f; //actual hip multiplier (shoulder only)
      float fudge2 = 1.1f; //split parameter (shoulder only)
      float fudge3 = m_parameters.m_hipL2ArmL2; // 0.3f; //mmmmdrunk = 0.2f //shoulder lean2: lean2hip multiplier
      float fudge4 = m_parameters.m_shoulderL2; //0.5f; //mmmmdrunk = 0.7f//shoulder lean2 offset
      float frankOffset = m_parameters.m_shoulderL1; //0.f;//mmmmdrunk 1.1
      float shoulderTwist = m_parameters.m_shoulderTwist;//-0.35f;//mmmmdrunk = 0.0f
      float desiredLean2;
      float desiredElbowR;
      float desiredElbowL;
      float armsOutOnPushMult = 0.f; // armsOutOnPushMultiplier
      if (m_armsOutOnPushTimer > 0.f)
      {
        m_armsOutOnPushTimer = m_armsOutOnPushTimer - timeStep;
        armsOutOnPushMult = m_parameters.m_armsOutOnPushMultiplier;
      }

      if (m_bendElbowsTimer > 0.f)
        m_bendElbowsTimer -= timeStep;

      if (m_character->hasCollidedWithWorld(bvmask_UpperBody))
      {
        m_bendElbowsTimer = m_parameters.m_bendElbowsTime;
      }

      if (m_character->getCharacterConfiguration().m_leftHandState != CharacterConfiguration::eHS_Rifle)
      {
        if ((getLeftLeg()->getFoot()->collidedWithNotOwnCharacter()) || (getRightLeg()->getFoot()->collidedWithNotOwnCharacter()))
        {
          //Shoulder: lean1
          // todo danger! getDesired is not limbs compatible. will return last tick's values...
          angleDelta = nmrsGetDesiredLean1(getRightLeg()->getHip()) - fudge1*nmrsGetActualLean1(getRightLeg()->getHip());
          angleDelta = angleDelta - a*(fudge2*nmrsGetActualLean1(getRightLeg()->getHip()) - nmrsGetActualLean1(getLeftLeg()->getHip()));
          if (m_parameters.m_shoulder > 0.1f)
            angleDelta += frankOffset/m_parameters.m_shoulder;
          getLeftArmInputData()->getShoulder()->setDesiredLean1(rage::Clamp(nmrsGetActualLean1(getLeftArm()->getShoulder()) + m_parameters.m_shoulder*angleDelta, -9.99f, 9.99f));
          getLeftArmInputData()->getShoulder()->setDesiredTwist(shoulderTwist);

          float maxLean2 = 0.f;//6.0f;
          //TDL to stop hand hitting head on elbows bent defend
          //Have -ve if upper arms swinging backwards to +ve upper arms swinging forward?
          //-ve otherwise hands could get stuck on hips
          //+ve so hand doesn't hit head but crosses across neck
          //getLeftArm()->getShoulder()->setDesiredTwist(0.35f);

          //Elbow
          // todo danger! getDesired is not limbs compatible. will return last tick's values...
          angleDelta = nmrsGetDesiredLean1(getRightLeg()->getHip()) - nmrsGetActualLean1(getRightLeg()->getHip());
          desiredElbowL = nmrsGetActualAngle(getLeftArm()->getElbow())+m_parameters.m_elbow*angleDelta ;
          if (m_parameters.m_armsOutOnPush)
          { 
            desiredElbowL = desiredElbowL + m_parameters.m_armsOutStraightenElbows*(add2Lean2 + add2Lean2Side);
          }

          if (m_bendElbowsTimer > 0.f)
          {
            rage::Vector3 comVel = m_character->m_COMvelRelative;
            comVel.Normalize();
            float backDotVel = m_bodyBack.Dot(comVel);
            float sideDotVel = m_bodyRight.Dot(comVel);
            //if (hit from left side to front) or (from front 120deg sweep) then bend elbow
            //otherwise lift this arm (lean2)
            if ((backDotVel > 0.0f && sideDotVel > 0.f) || (backDotVel > 0.5f))
            {
              if (desiredElbowL <  elbowAngle)
                desiredElbowL = elbowAngle;
            }
            else
            {
              maxLean2 = m_maxLean2OnPush;
            }
            //if (m_character->m_balancerState == bal_LeanAgainst)
            //{
            //  if (desiredElbowL <  elbowAngle)
            //    desiredElbowL = elbowAngle;
            //  maxLean2 = m_maxLean2OnPush;
            //}
          }

          //clamp the elbow angle (stops assertion error if armsOutStraightenElbows too high)
          desiredElbowL = rage::Clamp(desiredElbowL,bendElbowsGait,6.f);

          NM_RS_DBG_LOGF(L"left desiredElbow = %.4f", desiredElbowL);
          getLeftArmInputData()->getElbow()->setDesiredAngle(desiredElbowL);

          //Shoulder: lean2
          //hard coded lean2 plus proportional to lean2 from same side hip - to stop hand hitting thigh
          desiredLean2 = fudge4+fudge3*nmrsGetActualLean2(getLeftLeg()->getHip()); 

          if (m_parameters.m_armsOutOnPush) 
          {
            //arms out based on legs away from midline
            // todo danger! getDesired is not limbs compatible. will return last tick's values...
            add2Lean2Side = -0.5f*armsOutOnPushMult*(rage::Clamp(rage::Abs(nmrsGetDesiredLean2(getRightLeg()->getHip())),m_armsOutOnPushThresh,10.f) -m_armsOutOnPushThresh)
              -0.25f*armsOutOnPushMult*(rage::Clamp(rage::Abs(nmrsGetDesiredLean2(getLeftLeg()->getHip())),m_armsOutOnPushThresh,10.f) - m_armsOutOnPushThresh);
            add2Lean2Side = add2Lean2Side-0.5f*armsOutOnPushMult*(rage::Clamp(rage::Abs(nmrsGetActualLean2(getRightLeg()->getHip())), m_armsOutOnPushThresh,10.f) - m_armsOutOnPushThresh)
              -0.25f*armsOutOnPushMult*(rage::Clamp(rage::Abs(nmrsGetActualLean2(getLeftLeg()->getHip())),m_armsOutOnPushThresh,10.f) - m_armsOutOnPushThresh);
            NM_RS_DBG_LOGF(L"Left add2Lean2Side = %.4f", add2Lean2Side);
            //clamp the angle (eg. to stop arms going above shoulder height)
            desiredLean2 = rage::Clamp(desiredLean2 + add2Lean2 + add2Lean2Side,m_parameters.m_armsOutMinLean2,desiredLean2 - maxLean2);
          }
          NM_RS_DBG_LOGF(L"left desiredLean2 = %.4f", desiredLean2);

          getLeftArmInputData()->getShoulder()->setDesiredLean2(desiredLean2);
        }
      }

      //Right Arm
      if (m_character->getCharacterConfiguration().m_rightHandState != CharacterConfiguration::eHS_Rifle)
      {
        if ((getLeftLeg()->getFoot()->collidedWithNotOwnCharacter()) || (getRightLeg()->getFoot()->collidedWithNotOwnCharacter()))
        {
          //Shoulder: lean1
          // todo danger! getDesired is not limbs compatible. will return last tick's values...
          angleDelta = nmrsGetDesiredLean1(getLeftLeg()->getHip()) - fudge1*nmrsGetActualLean1(getLeftLeg()->getHip());
          angleDelta = angleDelta - a*(fudge2*nmrsGetActualLean1(getLeftLeg()->getHip())-nmrsGetActualLean1(getRightLeg()->getHip()));
          if (m_parameters.m_shoulder > 0.1f)
            angleDelta += frankOffset/m_parameters.m_shoulder;
          getRightArmInputData()->getShoulder()->setDesiredLean1(rage::Clamp(nmrsGetActualLean1(getRightArm()->getShoulder()) + m_parameters.m_shoulder*angleDelta, -9.99f, 9.99f));
          getRightArmInputData()->getShoulder()->setDesiredTwist(shoulderTwist);

          //Elbow
          // todo danger! getDesired is not limbs compatible. will return last tick's values...
          angleDelta = nmrsGetDesiredLean1(getLeftLeg()->getHip()) - nmrsGetActualLean1(getLeftLeg()->getHip());
          desiredElbowR = nmrsGetActualAngle(getRightArm()->getElbow()) + m_parameters.m_elbow*angleDelta;
          if (m_parameters.m_armsOutOnPush)
          {
            desiredElbowR = desiredElbowR + m_parameters.m_armsOutStraightenElbows*(add2Lean2 + add2Lean2Side);
          }

          float maxLean2 = 0.f;//6.0f;
          if (m_bendElbowsTimer > 0.f)
          {
            rage::Vector3 comVel = m_character->m_COMvelRelative;
            comVel.Normalize();
            float backDotVel = m_bodyBack.Dot(comVel);
            float sideDotVel = m_bodyRight.Dot(comVel);
            //if (hit from left side to front) or (from front 120deg sweep) then bend elbow
            //otherwise lift this arm (lean2)
            if ((backDotVel > 0.0f && sideDotVel < 0.f) || (backDotVel > 0.5f))
            {
              if (desiredElbowR <  elbowAngle)
                desiredElbowR = elbowAngle;
            }
            else
            {
              //Put arm out more
              maxLean2 = m_maxLean2OnPush;
            }
            //if (m_character->m_balancerState == bal_LeanAgainst)
            //{
            //  if (desiredElbowR <  elbowAngle)
            //    desiredElbowR = elbowAngle;
            //  maxLean2 = m_maxLean2OnPush;
            //}
          }
          //clamp the elbow angle (stops assertion error if armsOutStraightenElbows too high)
          desiredElbowR = rage::Clamp(desiredElbowR,bendElbowsGait,6.f);
          NM_RS_DBG_LOGF(L"right desiredElbow = %.4f", desiredElbowR);
          getRightArmInputData()->getElbow()->setDesiredAngle(desiredElbowR);

          //Shoulder: lean2
          //hard coded lean2 plus proportional to lean2 from same side hip - to stop hand hitting thigh
          desiredLean2 = fudge4+fudge3*nmrsGetActualLean2(getRightLeg()->getHip());
          if (m_parameters.m_armsOutOnPush)
          { 
            //arms out based on legs away from midline
            // todo danger! getDesired is not limbs compatible. will return last tick's values...
            add2Lean2Side = -0.5f*armsOutOnPushMult*(rage::Clamp(rage::Abs(getLeftLeg()->getHip()->getDesiredLean2()),m_armsOutOnPushThresh,10.f) - m_armsOutOnPushThresh)
              -0.25f*armsOutOnPushMult*(rage::Clamp(rage::Abs(nmrsGetDesiredLean2(getRightLeg()->getHip())),m_armsOutOnPushThresh,10.f) - m_armsOutOnPushThresh);
            add2Lean2Side = add2Lean2Side-0.5f*armsOutOnPushMult*(rage::Clamp(rage::Abs(nmrsGetActualLean2(getLeftLeg()->getHip())),m_armsOutOnPushThresh,10.f) - m_armsOutOnPushThresh)
              -0.25f*armsOutOnPushMult*(rage::Clamp(rage::Abs(nmrsGetActualLean2(getRightLeg()->getHip())),m_armsOutOnPushThresh,10.f) - m_armsOutOnPushThresh);
            NM_RS_DBG_LOGF(L"Right add2Lean2Side = %.4f", add2Lean2Side);
            //clamp the angle (eg. to stop arms going above shoulder height)
            desiredLean2 = rage::Clamp(desiredLean2 + add2Lean2 + add2Lean2Side,m_parameters.m_armsOutMinLean2,desiredLean2 - maxLean2);
          }
          NM_RS_DBG_LOGF(L"right desiredLean2 = %.4f", desiredLean2);

          getRightArmInputData()->getShoulder()->setDesiredLean2(desiredLean2);
        }
      }

      // lock the wrists to align along the forearm
      getLeftArmInputData()->getWrist()->setDesiredAngles(0.0f, 0.0f, 0.0f);
      getRightArmInputData()->getWrist()->setDesiredAngles(0.0f, 0.0f, 0.0f);

      // blend in the zero pose when body is not moving
      // (stops the arms rising up if the character stops with legs split)
      // always blend in a certain amount to try and retain some of the character's initial stance
      float blendFactor = 0.2f + ( (1.0f - motionMultiplier) * 0.8f); 

      if (m_parameters.m_blendToZeroPose)
      {
        getSpine()->blendToZeroPose(getSpineInput(), blendFactor, bvmask_UpperBody & ~bvmask_Spine0);

        if (!getSpine()->getSpine0()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        {
          // spine0 is an exception. we want lean2 to remain intact from dynamic balancer, so
          // don't set it.
          rage::Vector3 zeroAngles(getSpine()->getSpine0()->getZeroPoseAngles());
          getSpineInputData()->getSpine0()->setDesiredTwist(m_character->blendToSpecifiedPose(getSpineInputData()->getSpine0()->getDesiredTwist(), zeroAngles.x, blendFactor));
          getSpineInputData()->getSpine0()->setDesiredLean1(m_character->blendToSpecifiedPose(getSpineInputData()->getSpine0()->getDesiredLean1(), zeroAngles.y, blendFactor));
        }

        getLeftArm()->blendToZeroPose(getLeftArmInput(), blendFactor, bvmask_UpperBody);
        getRightArm()->blendToZeroPose(getRightArmInput(), blendFactor, bvmask_UpperBody);
      }

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "BodyBalance", blendFactor);
#endif

      // iterate over each effector in the arms and spine
      // if effector does not have a stored zero pose or blendToZeroPose parameter is false,
      // blend the pose to zero (the number, not the pose).

      // clavicles
      rage::Vector3 targetPose(0.0f, 0.0f, 0.0f);
      if (!getLeftArm()->getClavicle()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getLeftArmInputData()->getClavicle()->blendToSpecifiedPose(targetPose, blendFactor);
      if (!getRightArm()->getClavicle()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getRightArmInputData()->getClavicle()->blendToSpecifiedPose(targetPose, blendFactor);

      // shoulders
      targetPose.Set(0.0f, 0.0f, 0.5f);
      if (!getLeftArm()->getShoulder()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getLeftArmInputData()->getShoulder()->blendToSpecifiedPose(targetPose, blendFactor);
      if (!getRightArm()->getShoulder()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getRightArmInputData()->getShoulder()->blendToSpecifiedPose(targetPose, blendFactor);

      //elbows
      if (!getLeftArm()->getElbow()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getLeftArmInputData()->getElbow()->blendToSpecifiedPose(0.4f, blendFactor);
      if (!getRightArm()->getElbow()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getRightArmInputData()->getElbow()->blendToSpecifiedPose(0.4f, blendFactor);

      //wrists
      targetPose.Set(0.0f, 0.0f, 0.0f);
      if (!getLeftArm()->getWrist()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getLeftArmInputData()->getWrist()->blendToSpecifiedPose(targetPose, blendFactor);
      if (!getRightArm()->getWrist()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getRightArmInputData()->getWrist()->blendToSpecifiedPose(targetPose, blendFactor);

      // spine
      if (!getSpine()->getSpine0()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
      {
        // blend lean1 and twist to target pose. leave lean2 un-set.
        getSpineInputData()->getSpine0()->setDesiredTwist((getSpineInputData()->getSpine0()->getDesiredTwist() + targetPose.x)/2.f);
        getSpineInputData()->getSpine0()->setDesiredLean1((getSpineInputData()->getSpine0()->getDesiredLean1() + targetPose.y)/2.f);  
      }
      if (!getSpine()->getSpine1()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getSpineInputData()->getSpine1()->blendToSpecifiedPose(targetPose, blendFactor);
      if (!getSpine()->getSpine2()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getSpineInputData()->getSpine2()->blendToSpecifiedPose(targetPose, blendFactor);
      if (!getSpine()->getSpine3()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getSpineInputData()->getSpine3()->blendToSpecifiedPose(targetPose, blendFactor);
      if (!getSpine()->getLowerNeck()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getSpineInputData()->getLowerNeck()->blendToSpecifiedPose(targetPose, blendFactor);
      if (!getSpine()->getUpperNeck()->hasStoredZeroPose() || (!m_parameters.m_blendToZeroPose))
        getSpineInputData()->getUpperNeck()->blendToSpecifiedPose(targetPose, blendFactor);
    }

    return eCBUTaskComplete;
  }

  void NmRsCBUBodyBalance::falling(float timeStep)
  {
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);

    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);

    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    if (catchFallTask->m_handsAndKnees)
    {
      //bend the character forward at hips to prepare for catch fall landing
      m_bendTimer += timeStep;
      if (m_bendTimer<0.2f)
        dynamicBalancerTask->setHipPitch(-0.5f);
      else
        dynamicBalancerTask->setHipPitch(0.f);       
    }

    NM_RS_DBG_LOGF(L"- Body Balance >> DoFall");
    if (!catchFallTask->isActive())// && m_character->m_balancerState != bal_Drape)
    {
      //Turn off headlook
      if (m_parameters.m_useHeadLook) 
        headLookTask->deactivate();//de-activate once only.  (HeadLook used by CatchFall)

      catchFallTask->updateBehaviourMessage(NULL);// sets values to defaults
      float defaultBodyStiffness = 9.f;  // for the bodyBalance
      catchFallTask->m_parameters.m_legsStiffness = m_parameters.m_armStiffness * 5.5f/defaultBodyStiffness;
      catchFallTask->m_parameters.m_torsoStiffness = m_parameters.m_armStiffness * 10.f/defaultBodyStiffness;
      catchFallTask->m_parameters.m_armsStiffness = m_parameters.m_armStiffness * 15.f/defaultBodyStiffness;

      //MMMMHandsKnees mmmmtodo also change the params above
      if (catchFallTask->m_handsAndKnees)
      {
        catchFallTask->m_parameters.m_legsStiffness = 5.5f;
        catchFallTask->m_parameters.m_torsoStiffness = 10.f;
        catchFallTask->m_parameters.m_armsStiffness = 15.f;
        //We used to start the catchFall with an upperbody response only here - we wanted the dynBalancer to keep stepping for a while
        //as the character falls over.  CatchFall used to modify this parameter internally. 
        catchFallTask->m_parameters.m_effectorMask = bvmask_Full;
        if (dynamicBalancerTask->isActive())
        {
          dynamicBalancerTask->setOpposeGravityAnkles(0.85f);
          dynamicBalancerTask->setOpposeGravityLegs(0.85f);
          dynamicBalancerTask->setLeftLegStiffness(9.5f);
          dynamicBalancerTask->setRightLegStiffness(9.5f);

          // needs refactor. we can't be calling these functions on the balancer
          // from outside. there is no guarantee that the limbs or other internal
          // data are set up at this point. we have been getting by on convention
          // so far...
          dynamicBalancerTask->setLowerBodyGravityOpposition(m_body);
          dynamicBalancerTask->calibrateLowerBodyEffectors(m_body);
        }

      }

      //match leg stiffness of catch fall to current value so as to stop standing up if legs bent in leanagainst wall 
      if (balColReactTask->isActive())
      {
        float legStiffness = catchFallTask->m_parameters.m_legsStiffness;
        if (balColReactTask->m_balancerState == bal_Slump || balColReactTask->m_balancerState == bal_Trip)
        {
          legStiffness = rage::Min(sqrt(getRightLeg()->getKnee()->getMuscleStrength()),sqrt(getLeftLeg()->getKnee()->getMuscleStrength()), balColReactTask->m_slumpStiffLKnee, balColReactTask->m_slumpStiffRKnee);
        }
        if (balColReactTask->m_balancerState == bal_LeanAgainst)
        {
          legStiffness = rage::Min(sqrt(getRightLeg()->getKnee()->getMuscleStrength()),sqrt(getLeftLeg()->getKnee()->getMuscleStrength()));
        }             
        legStiffness = rage::Max(5.f,legStiffness);               
        catchFallTask->m_parameters.m_legsStiffness = legStiffness;
        balColReactTask->resetFrictionMultipliers();
        //balColReactTask->m_balancerState = bal_End;
      }
      catchFallTask->activate();
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUBodyBalance::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    static const char* failTypeStrings[] =
    {
#define BAL_NAME_ACTION(_name) #_name ,
      BAL_STATES(BAL_NAME_ACTION)
#undef BAL_NAME_ACTION
    };

    bspyTaskVar(m_parameters.m_armStiffness, true);
    bspyTaskVar(m_parameters.m_elbow, true);
    bspyTaskVar(m_parameters.m_shoulder, true);
    bspyTaskVar(m_parameters.m_armDamping, true);
    bspyTaskVar(m_parameters.m_spineStiffness, true);
    bspyTaskVar(m_parameters.m_spineDamping, true);

    bspyTaskVar(m_parameters.m_useHeadLook, true);
    bspyTaskVar(m_parameters.m_headLookPos, true);
    bspyTaskVar(m_parameters.m_headLookInstanceIndex, true);
    bspyTaskVar(m_parameters.m_headLookAtVelProb, true);

    bspyTaskVar(m_parameters.m_somersaultAngle, true);
    bspyTaskVar(m_parameters.m_somersaultAngleThreshold, true);
    bspyTaskVar(m_parameters.m_sideSomersaultAngle, true);
    bspyTaskVar(m_parameters.m_sideSomersaultAngleThreshold, true);

    bspyTaskVar(m_parameters.m_somersaultAngVel, true);
    bspyTaskVar(m_parameters.m_somersaultAngVelThreshold, true);
    bspyTaskVar(m_parameters.m_twistAngVel, true);
    bspyTaskVar(m_parameters.m_twistAngVelThreshold, true);
    bspyTaskVar(m_parameters.m_sideSomersaultAngVel, true);
    bspyTaskVar(m_parameters.m_sideSomersaultAngVelThreshold, true);

    bspyTaskVar(m_parameters.m_armsOutOnPushMultiplier, true);
    bspyTaskVar(m_parameters.m_armsOutOnPushTimeout, true);

    bspyTaskVar(m_parameters.m_returningToBalanceArmsOut, true);
    bspyTaskVar(m_parameters.m_armsOutStraightenElbows, true);
    bspyTaskVar(m_parameters.m_armsOutMinLean2, true);

    bspyTaskVar(m_parameters.m_elbowAngleOnContact, true);
    bspyTaskVar(m_parameters.m_bendElbowsTime, true);
    bspyTaskVar(m_parameters.m_bendElbowsGait, true);

    bspyTaskVar(m_parameters.m_hipL2ArmL2, true);
    bspyTaskVar(m_parameters.m_shoulderL2, true);
    bspyTaskVar(m_parameters.m_shoulderL1, true);
    bspyTaskVar(m_parameters.m_shoulderTwist, true);

    bspyTaskVar(m_parameters.m_armsOutOnPush, true);
    bspyTaskVar(m_parameters.m_backwardsArms, true);
    bspyTaskVar(m_parameters.m_blendToZeroPose, true);


    bspyTaskVar(m_parameters.m_useBodyTurn, true);
    bspyTaskVar(m_parameters.m_turnOffProb, true);
    bspyTaskVar(m_parameters.m_turn2TargetProb, true);
    bspyTaskVar(m_parameters.m_turn2VelProb, true);
    bspyTaskVar(m_parameters.m_turnAwayProb, true);
    bspyTaskVar(m_parameters.m_turnLeftProb, true);
    bspyTaskVar(m_parameters.m_turnRightProb, true);
    bspyTaskVar(m_parameters.m_backwardsAutoTurn, true);

    bspyTaskVar(m_angVel, false);
    bspyTaskVar(m_angVelClamped, false);
    bspyTaskVar(m_bodyUp, false);
    bspyTaskVar(m_bodyRight, false);
    bspyTaskVar(m_bodyBack, false);

    bspyTaskVar(m_armsOutOnPushTimer, false);
    bspyTaskVar(m_armsOutOnPushThresh, false);

    bspyTaskVar(m_oldqTilt, false);
    bspyTaskVar(m_oldqSom, false);
    bspyTaskVar(m_oldqTwist, false);

    bspyTaskVar(m_bendElbowsTimer, false);
    bspyTaskVar(m_maxLean2OnPush, false);
    bspyTaskVar(m_lastFootState, false);
    bspyTaskVar(m_lookAtTimer, false);
    bspyTaskVar(m_lookAtRandom, false);
    bspyTaskVar(m_randomTurn, false);

    bspyTaskVar(m_useCOMAngVel, false);
    bspyTaskVar(m_characterIsFalling, false);
    bspyTaskVar(m_lookInVelDir, false);

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

