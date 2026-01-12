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
#include "NmRsCBU_Teeter.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_ArmsWindmill.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_BodyBalance.h"
#include "NmRsCBU_BodyWrithe.h"
#include "NmRsCBU_Catchfall.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_SpineTwist.h"
#include "NmRsCBU_RollDownStairs.h"
#include "NmRsCBU_Grab.h"
#include "NmRsCBU_HighFall.h"


namespace ART
{
  NmRsCBUTeeter::NmRsCBUTeeter(ART::MemoryManager* services) : CBUTaskBase(services, bvid_teeter)
  {
    initialiseCustomVariables();
  }

  NmRsCBUTeeter::~NmRsCBUTeeter()
  {
  }

  void NmRsCBUTeeter::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
  }

  void NmRsCBUTeeter::onActivate()
  {
    Assert(m_character);

    //Teeter Entry
    NM_RS_DBG_LOGF(L"- Teeter Entry");
    m_characterIsFalling = false;
    m_setSpineToZeroWhenFinished = false;
    m_setTurnToZeroWhenFinished = false;
    m_teeterTimer = 0.f;
    m_restartTime = m_character->getRandom().GetRanged(0.f,1.2f);
    m_highFall = false;
    m_state = -1;//force feedback of teet_Pre
    sendFeedback(teet_Pre);
    m_restrictCatchFallArms = true;
  }

  void NmRsCBUTeeter::onDeactivate()
  {
    Assert(m_character);

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    dynamicBalancerTask->setTeeter(false);

    NmRsCBUBodyWrithe* writheTask = (NmRsCBUBodyWrithe*)m_cbuParent->m_tasks[bvid_bodyWrithe];
    Assert(writheTask);
    if (writheTask->isActive())
      writheTask->deactivate();

    //NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    //Assert(headLookTask);
    //if (headLookTask->isActive())
    //  headLookTask->deactivate();

    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist];    
    Assert(spineTwistTask);
    if (spineTwistTask->isActive())
      spineTwistTask->deactivate();

    NmRsCBUArmsWindmill* armsWindmillTask = (NmRsCBUArmsWindmill*)m_cbuParent->m_tasks[bvid_armsWindmill];
    Assert(armsWindmillTask);
    if (armsWindmillTask->isActive())
      armsWindmillTask->deactivate();   
    m_restrictCatchFallArms = false;

  }

  CBUTaskReturn NmRsCBUTeeter::onTick(float timeStep)
  { 
    NM_RS_DBG_LOGF(L"- Teeter During")
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);

    //NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    //Assert(headLookTask);

    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);
    NmRsCBUGrab* grabTask = (NmRsCBUGrab*)m_cbuParent->m_tasks[bvid_grab];
    Assert(grabTask);

    //Is the character past the edge?
    rage::Vector3 levelCom = m_character->m_COM;
    m_character->levelVector(levelCom);
    rage::Vector3 edge2Com = levelCom - balColReactTask->m_pos1stContact;//Assumes normal is level
    float com2Edge = edge2Com.Dot(balColReactTask->m_normal1stContact);
    rage::Vector3 edge2Foot;
    edge2Foot = getLeftLeg()->getFoot()->getPosition() - balColReactTask->m_pos1stContact;//Assumes normal is level
    m_character->levelVector(edge2Foot);
    float edge2LeftFoot = edge2Foot.Dot(balColReactTask->m_normal1stContact);
    edge2Foot = getRightLeg()->getFoot()->getPosition() - balColReactTask->m_pos1stContact;//Assumes normal is level
    m_character->levelVector(edge2Foot);
    float edge2RightFoot = edge2Foot.Dot(balColReactTask->m_normal1stContact);

    m_characterIsFalling = (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK);//MMMMHandsKnees replace all right with left//dynamicBalancerTask->hasFailed();

    if (m_characterIsFalling)
    {  
      bool dontDoFall = (balColReactTask->isActive() && (balColReactTask->m_balancerState == bal_Drape || balColReactTask->m_balancerState == bal_DrapeForward));// || balColReactTask->m_balancerState == bal_DrapeGlancingSpin);
      dontDoFall = dontDoFall || (com2Edge > 0.f);//if the character is not over the edge don't do special teeter fall just whatever the behaviour running with teeter's reaction
      if (!dontDoFall)
      {
        falling(timeStep);
      }
      else
        sendFeedback(teet_FallOnGround);
      dynamicBalancerTask->setTeeter(false);
    }
    else
    {

      ////mmmmtodo look at line sometimes
      //if (m_parameters.m_useHeadLook) 
      //{
      //  if (!headLookTask->isActive())//May not have been activated in teeter::Activate
      //    headLookTask->updateBehaviourMessage(NULL);
      //  //Look at target
      //  rage::Vector3 posTarget = feet?;
      //  headLookTask->m_parameters.m_pos = posTarget;
      //  headLookTask->m_parameters.m_vel.Set(0,0,0);
      //  headLookTask->m_parameters.m_stiffness = m_parameters.m_spineStiffness+1.0f;//11.0f);
      //  headLookTask->m_parameters.m_damping = 1.0f;
      //  headLookTask->m_parameters.m_alwaysLook = true;
      //  headLookTask->m_parameters.m_instanceIndex = m_parameters.m_headLookInstanceIndex;
      //} 

      //Test
      //leftfoot  3.447,0.087,0
      //rightfoot 3.394,-0.19,0
      //Game 15m shot platform: 
      //leftfoot  39.517,38.89, 21.12
      //rightfoot 39.46,39.57,21.12
      dynamicBalancerTask->setGiveUpThreshold(1.f);
      dynamicBalancerTask->setTeeter(true);
      catchFallTask->m_handsAndKnees = true;
      catchFallTask->m_callRDS = true;
      catchFallTask->m_comVelRDSThresh = 2.0f;
      catchFallTask->m_armReduceSpeed = 0.2f;

      float velForwards = -m_character->m_COMTM.c.Dot(m_character->m_COMvel);
      // TEST: setup balancer exclusion zone to avoid stepping with knee into wall   
      //if the character is going over the edge and the balancer is on, should start stepping down
      //this is generally when the non stance leg is dangling over the edge but the character is still balanced
      //without it the character stands tall as it leans over the edge
      if (com2Edge < -0.1f)
      {
        if (dynamicBalancerTask->getLegStraightnessModifier() > -0.04f)
          dynamicBalancerTask->setLegStraightnessModifier(m_character->getRandom().GetRanged(-0.7f,-0.04f));
        balColReactTask->m_impactOccurred = false;//turnOff exclusion zone in balancer
        dynamicBalancerTask->setTeeter(false);//turnOff probes to exclusion zone in balancer
      }
      else
        balColReactTask->m_impactOccurred = m_parameters.useExclusionZone;
      //          if (m_parameters.useExclusionZone) //mmmmtodo predict if going near or slumping fow etc  !balColReactTask->isActive())
      {
        balColReactTask->m_pos1stContact = m_parameters.edgeLeft;
        rage::Vector3 edgeThenNormal =  m_parameters.edgeRight - m_parameters.edgeLeft;
        //mmmmmtodo Check that the edge points define a line and that it isn't vertical
        edgeThenNormal.Cross(m_character->m_gUp);
        edgeThenNormal.Normalize();
        balColReactTask->m_normal1stContact = edgeThenNormal;
        balColReactTask->m_sideOfPlane = -1.f;
        balColReactTask->m_parameters.exclusionZone = 0.2f;
#if ART_ENABLE_BSPY
        m_character->bspyDrawLine(balColReactTask->m_pos1stContact, balColReactTask->m_pos1stContact+balColReactTask->m_normal1stContact, rage::Vector3(1,1,0));
#endif
      }

      NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist];      
      NmRsCBUArmsWindmill* armsWindmillTask = (NmRsCBUArmsWindmill*)m_cbuParent->m_tasks[bvid_armsWindmill];
      Assert(armsWindmillTask);
      dynamicBalancerTask->setHipPitch(-0.f);

      levelCom = m_character->m_COMvel;
      m_character->levelVector(levelCom);
      rage::Vector3 dirTarget(0,0,0);
      if (m_parameters.leanAway && /*com2Edge > 0.27f && */com2Edge < 2.f && levelCom.Dot(balColReactTask->m_normal1stContact) < 1.f)
      {
        sendFeedback(teet_LeanAwayZone);

        float lean = levelCom.Dot(balColReactTask->m_normal1stContact);
        lean = -(levelCom.Dot(balColReactTask->m_normal1stContact)+1.f);
        lean = rage::Clamp(-(levelCom.Dot(balColReactTask->m_normal1stContact)+1.f), 0.f,3.f);
        lean *= 0.3333f;
        //rage::Vector3 offset(0,0,0);
        //dynamicBalancerTask->autoLeanToObject(3,offset,lean*0.3f);
        //dynamicBalancerTask->autoLeanHipsToObject(3,offset,lean*0.3f);
        //dynamicBalancerTask->setBalanceTime(0.2f+lean*0.3f);
        dynamicBalancerTask->autoLeanInDirection(balColReactTask->m_normal1stContact,lean*0.5f);
        dynamicBalancerTask->autoLeanHipsInDirection(balColReactTask->m_normal1stContact,lean*0.5f);
        dynamicBalancerTask->setBalanceTime(0.2f+lean*0.3f);

        //rage::Vector3 vec(0,0,0);
        //m_character->instanceToWorldSpace(&dirTarget, vec, 6);

        static bool turn = false;
        if (turn)
        {
          dirTarget = m_parameters.edgeLeft;
          dirTarget -= m_character->m_COM;
          if (lean>0.05f)
            dynamicBalancerTask->useCustomTurnDir(true, dirTarget);
          else
            dynamicBalancerTask->useCustomTurnDir(false, dirTarget);
        }
      }
      else
      {
        dynamicBalancerTask->autoLeanCancel();
        dynamicBalancerTask->autoLeanHipsCancel();
        dynamicBalancerTask->setBalanceTime(0.2f);
        dynamicBalancerTask->useCustomTurnDir(false, dirTarget);
      }
      static bool restart = true;
      //dynamicBalancerTask->setPlantLeg(false);
      if (restart && com2Edge < 0.17f && m_teeterTimer > (0.7f + m_restartTime))//mmmtodo not moving away from edge //not bent over alot
      {
        //restart teeter
        m_teeterTimer = 0.f;
        m_restartTime = m_character->getRandom().GetRanged(0.f,1.2f);
        m_setSpineToZeroWhenFinished = false;
      }

      if (com2Edge < 0.27f && m_teeterTimer >0.1f && m_teeterTimer <0.5f)
      {
        sendFeedback(teet_Teeter);
        if (!spineTwistTask->isActive())
        {
          spineTwistTask->initialiseCustomVariables();
          //spineTwistTask->setSpineTwistStiffness(???);//only set on activate of spineTwist
          spineTwistTask->setSpineTwistTwistClavicles(false);
          spineTwistTask->setSpineTwistAllwaysTwist(true);
          spineTwistTask->activate();
        }
        dynamicBalancerTask->autoLeanCancel();
        dynamicBalancerTask->autoLeanHipsCancel();
        dynamicBalancerTask->setHipPitch(-1.6f + m_teeterTimer);
        //dynamicBalancerTask->setPlantLeg(true);
        if (velForwards < 0.f && m_teeterTimer <0.2f)
        {
          dynamicBalancerTask->setHipPitch(1.6f - m_teeterTimer);
          //dynamicBalancerTask->setHipPitch(0.f - m_teeterTimer);

          float spineLean1 = -0.5f;

          getSpineInputData()->getSpine0()->setDesiredLean1(spineLean1+ m_teeterTimer);
          getSpineInputData()->getSpine1()->setDesiredLean1(spineLean1+ m_teeterTimer);
          getSpineInputData()->getSpine2()->setDesiredLean1(spineLean1+ m_teeterTimer);
          getSpineInputData()->getSpine3()->setDesiredLean1(spineLean1+ m_teeterTimer);

          m_setSpineToZeroWhenFinished = true;
          spineLean1 = 0.7f;
          if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep || dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kNotStepping)
            getRightLegInputData()->getKnee()->setDesiredAngle(nmrsGetDesiredAngle(getRightLeg()->getKnee())-spineLean1+ m_teeterTimer);

          if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep || dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kNotStepping)
            getLeftLegInputData()->getKnee()->setDesiredAngle(nmrsGetDesiredAngle(getLeftLeg()->getKnee())-spineLean1+ m_teeterTimer);
        }
        else
        {
          //shot doesn't set spinelean1 after cpain
          getSpineInputData()->getSpine0()->setDesiredLean1(0.0f);
          getSpineInputData()->getSpine1()->setDesiredLean1(0.0f);
          getSpineInputData()->getSpine2()->setDesiredLean1(0.0f);
          getSpineInputData()->getSpine3()->setDesiredLean1(0.0f);

          m_setSpineToZeroWhenFinished = false;
        }

      }//(com2Edge < 0.27f && m_teeterTimer >0.1f && m_teeterTimer <0.5f)
      else
      {
        if (m_setSpineToZeroWhenFinished)//we just want to do this once
        {
          //shot doesn't set spinelean1 after cpain
          getSpineInputData()->getSpine0()->setDesiredLean1(0.0f);
          getSpineInputData()->getSpine1()->setDesiredLean1(0.0f);
          getSpineInputData()->getSpine2()->setDesiredLean1(0.0f);
          getSpineInputData()->getSpine3()->setDesiredLean1(0.0f);
        }
      }
      if (edge2RightFoot < 0.27f || edge2LeftFoot < 0.27f)
      {
        sendFeedback(teet_FootClose2Edge);
      }   


      if (com2Edge < 0.27f)
      {
        sendFeedback(teet_PreTeeter);
        m_teeterTimer += timeStep;
        if (m_teeterTimer < 0.8f)
        {
          if (!armsWindmillTask->isActive())
          {
            armsWindmillTask->updateBehaviourMessage(NULL); // initialise parameters.

            armsWindmillTask->m_parameters.m_angVelThreshold = 0.1f;
            armsWindmillTask->m_parameters.m_angVelGain = 1.f;
            armsWindmillTask->m_parameters.m_disableOnImpact = true;
            armsWindmillTask->m_parameters.m_mirrorMode = 1;
            armsWindmillTask->m_parameters.m_adaptiveMode = 0;
            armsWindmillTask->m_parameters.m_forceSync = true;
            armsWindmillTask->m_parameters.m_phaseOffset = m_character->getRandom().GetRanged(-100.f,100.f);
            armsWindmillTask->m_parameters.m_shoulderStiffness = 11.f;
            armsWindmillTask->m_parameters.m_useLeft = true;
            armsWindmillTask->m_parameters.m_useRight = true;
            armsWindmillTask->m_parameters.m_IKtwist = 0;
            armsWindmillTask->m_parameters.m_dragReduction = 0.2f;
            armsWindmillTask->m_parameters.m_leftCircleDesc.partID = 10;
            armsWindmillTask->m_parameters.m_leftCircleDesc.radius1 = 0.75f;
            armsWindmillTask->m_parameters.m_leftCircleDesc.radius2 = 0.75f;
            armsWindmillTask->m_parameters.m_leftCircleDesc.speed = 1.5f;
            if (velForwards < 0.f)
              armsWindmillTask->m_parameters.m_leftCircleDesc.speed *= -1.f;
            armsWindmillTask->m_parameters.m_leftCircleDesc.normal = rage::Vector3(0.f, 0.5f, -0.1f);
            armsWindmillTask->m_parameters.m_leftCircleDesc.centre = rage::Vector3(0.f, 0.5f, -0.1f);

            armsWindmillTask->m_parameters.m_rightCircleDesc.partID = 10;
            armsWindmillTask->m_parameters.m_rightCircleDesc.radius1 = 0.75f;
            armsWindmillTask->m_parameters.m_rightCircleDesc.radius2 = 0.75f;
            armsWindmillTask->m_parameters.m_rightCircleDesc.speed = 1.5f;
            if (velForwards < 0.f)
              armsWindmillTask->m_parameters.m_rightCircleDesc.speed *= -1.f;

            armsWindmillTask->m_parameters.m_rightCircleDesc.normal = rage::Vector3(0.f, 0.5f, -0.1f);
            armsWindmillTask->m_parameters.m_rightCircleDesc.centre = rage::Vector3(0.f, 0.5f, -0.1f);

            armsWindmillTask->activate();
            armsWindmillTask->setOK2Deactivate(false);//Dont let shot deactivate this behaviour if we're using it here

          }
        }//(m_teeterTimer < 0.8f)
        else
          armsWindmillTask->deactivate();//OK2Deactivate is set to true here

        float feetApart = 0.0f;//(getLeftLeg()->getFoot()->getPosition() - getRightLeg()->getFoot()->getPosition()).Mag();
        if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kNotStepping  && m_teeterTimer <0.3f && velForwards > 0.f && feetApart < 0.4f)
        {
          rage::Matrix34 footM;
          rage::Vector3 torque;
          if (edge2RightFoot > 0.f)
          {
            getRightLeg()->getFoot()->getMatrix(footM);
            torque = footM.a;
            torque *= -100.f;
            (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtAnkle_Right)))->ApplyTorque(torque);
          }
          if (edge2LeftFoot > 0.f)
          {
            getLeftLeg()->getFoot()->getMatrix(footM);
            torque = footM.a;
            torque *= -100.f;
            (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtAnkle_Left)))->ApplyTorque(torque);
          }
        }//(dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kNotStepping  && m_teeterTimer <0.3f && velForwards > 0.f && feetApart < 0.4f)

        if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)
        {
          float mult = 1.f;
          if (m_teeterTimer > 0.1f)
            mult *= -1.f;
          if (m_teeterTimer <0.5f)
          {
            // limbs todo rename getXInput...
            getLeftLeg()->holdPose(m_body->getInput(kLeftLeg));

            float spineLean1 = -1.2f;
            if (velForwards < 0.f)
            {
              if (getLeftLeg()->getFoot()->getPosition().z > balColReactTask->m_pos1stContact.z + 0.1f)//mmmmtodo ONLY FOR Z UP
                getLeftLegInputData()->getHip()->setDesiredLean1(nmrsGetDesiredLean1(getLeftLeg()->getHip())-spineLean1+ m_teeterTimer);
            }
            //swap mult sign good aswell
            getRightLegInputData()->getHip()->setDesiredTwist(getRightLeg()->getHip()->getDesiredTwist()+mult*0.5f);

            rage::Matrix34 footM;
            getRightLeg()->getFoot()->getMatrix(footM);
            rage::Vector3 torque = footM.a;
            torque *= -130.f;
            if (m_teeterTimer <0.3f && velForwards > 0.f && edge2RightFoot > 0.08f)
              (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtAnkle_Right)))->ApplyTorque(torque);
          }
          //good getRightLeg()->getAnkle()->setDesiredTwist(-mult*0.5f);
          int instanceIndex = m_character->getFirstInstance()->GetLevelIndex();
          rage::Vector3 dirTarget;
          rage::Vector3 posTargetLocal(-1.0f, 0.f, 0.f);
          m_character->instanceToWorldSpace(&dirTarget, posTargetLocal, instanceIndex);
          dirTarget -= m_character->m_COM;
          dynamicBalancerTask->useCustomTurnDir(true, dirTarget);
          m_setTurnToZeroWhenFinished = true;
          dirTarget.Set(0.f,-mult*10.f,0.f);
          spineTwistTask->setSpineTwistPos(dirTarget);

          //if (getLeftLeg()->getFoot()->getPosition().x > 3.8f)
          if (edge2LeftFoot < -0.35f)
          {
            catchFallTask->m_handsAndKnees = true;
            catchFallTask->m_callRDS = true;
            catchFallTask->m_comVelRDSThresh = 2.0f;
            catchFallTask->m_armReduceSpeed = 0.2f;

            dynamicBalancerTask->forceFailOnly();
          }
        }//(dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)

        if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
        {
          float mult = 1.f;
          if (m_teeterTimer > 0.1f)
            mult *= -1.f;
          if (m_teeterTimer <0.5f)
          {
            // limbs todo rename
            getRightLeg()->holdPose(m_body->getInput(kRightLeg));

            float spineLean1 = -1.2f;
            if (velForwards < 0.f)
            {
              if (getRightLeg()->getFoot()->getPosition().z > balColReactTask->m_pos1stContact.z + 0.1f)//mmmmtodo ONLY FOR Z UP
              {
                getRightLegInputData()->getHip()->setDesiredLean1(nmrsGetDesiredLean1(getRightLeg()->getHip())-spineLean1+ m_teeterTimer);
              }
            }

            // limbs todo is this internal?
            getLeftLegInputData()->getHip()->setDesiredTwist(getLeftLeg()->getHip()->getDesiredTwist()+mult*0.5f);

            rage::Matrix34 footM;
            getLeftLeg()->getFoot()->getMatrix(footM);
            rage::Vector3 torque = footM.a;
            torque *= -130.f;
            //if (m_teeterTimer <0.3f && velForwards > 0.f  && getLeftLeg()->getFoot()->getPosition().x < 3.53f)//3.447f
            if (m_teeterTimer <0.3f && velForwards > 0.f  && edge2LeftFoot > 0.08f)
              (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtAnkle_Left)))->ApplyTorque(torque);
            //good m_leftLeg->getAnkle()->setDesiredTwist(-mult*0.5f);
          }
          int instanceIndex = m_character->getFirstInstance()->GetLevelIndex();
          rage::Vector3 dirTarget;
          rage::Vector3 posTargetLocal(1.0f, 0.f, 0.f);
          m_character->instanceToWorldSpace(&dirTarget, posTargetLocal, instanceIndex);
          dirTarget -= m_character->m_COM;
          dynamicBalancerTask->useCustomTurnDir(true, dirTarget);
          m_setTurnToZeroWhenFinished = true;
          dirTarget.Set(0.f,mult*10.f,0.f);
          spineTwistTask->setSpineTwistPos(dirTarget);
          //if (getLeftLeg()->getFoot()->getPosition().x > 3.8f)
          if (edge2RightFoot < -0.35f)
          {
            catchFallTask->m_handsAndKnees = true;
            catchFallTask->m_callRDS = true;
            catchFallTask->m_comVelRDSThresh = 2.0f;
            catchFallTask->m_armReduceSpeed = 0.2f;
            dynamicBalancerTask->forceFailOnly();
          }
        }//(dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
      }//(com2Edge < 0.27f)
      else
      {
        if (spineTwistTask->isActive())
          spineTwistTask->deactivate();
        if (armsWindmillTask->isActive())
          armsWindmillTask->deactivate();
        if (m_setTurnToZeroWhenFinished)
        {
          rage::Vector3 noTurn(0.f, 0.f, 0.f);
          dynamicBalancerTask->useCustomTurnDir(false, noTurn);
          m_setTurnToZeroWhenFinished = false;
        }
      }

      if (grabTask->getTryingToGrab())
      {
        rage::Vector3 grabPos = grabTask->getGrabPoint();//Favours right hand
        grabPos -= m_character->m_COM;
        if (dynamicBalancerTask->getUseCustomTurnDir())
          dynamicBalancerTask->useCustomTurnDir(true, grabPos);
      }
    }//if !(m_characterIsFalling)

    return eCBUTaskComplete;
  }

  void NmRsCBUTeeter::falling(float timeStep)
  {
    NmRsCBUBodyWrithe* writheTask = (NmRsCBUBodyWrithe*)m_cbuParent->m_tasks[bvid_bodyWrithe];
    Assert(writheTask);
    NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
    Assert(rdsTask);
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    NmRsCBUArmsWindmill* armsWindmillTask = (NmRsCBUArmsWindmill*)m_cbuParent->m_tasks[bvid_armsWindmill];
    Assert(armsWindmillTask);
    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);

    rage::Vector3 edge2Shoulder;
    edge2Shoulder = getLeftArm()->getClaviclePart()->getPosition() - balColReactTask->m_pos1stContact;//Assumes normal is level
    m_character->levelVector(edge2Shoulder);
    float edge2LeftShoulder = edge2Shoulder.Dot(balColReactTask->m_normal1stContact);
    edge2Shoulder = getRightArm()->getClaviclePart()->getPosition() - balColReactTask->m_pos1stContact;//Assumes normal is level
    m_character->levelVector(edge2Shoulder);
    float edge2RightShoulder = edge2Shoulder.Dot(balColReactTask->m_normal1stContact);

    //restrict the catchFall arm target to not go over the edge?
    rage::Vector3 edge2Spine3 = getSpine()->getSpine3Part()->getPosition()- m_parameters.edgeLeft;
    rage::Vector3 left2Right = m_parameters.edgeRight - m_parameters.edgeLeft;
    float dot = edge2Spine3.Dot(left2Right);
    left2Right.Normalize();
    edge2Spine3 = getSpine()->getSpine3Part()->getPosition();
    edge2Spine3 -= m_parameters.edgeLeft + dot*left2Right;

    bool armsToFarAway = (edge2Spine3.Mag()> 0.8f);

    if (m_highFall || armsToFarAway)
      m_restrictCatchFallArms = false;
    else
      m_restrictCatchFallArms = true;
    if (!m_highFall)
    {
      sendFeedback(teet_OverEdge);
      if (writheTask->isActive() && m_character->hasCollidedWithWorld(bvmask_Full))
        writheTask->deactivate();
      else if (!writheTask->isActive() && !m_character->hasCollidedWithWorld(bvmask_Full))
      {
        writheTask->updateBehaviourMessage(NULL); // set parameters to defaults
        writheTask->m_parameters.m_armPeriod = 1.5f;
        writheTask->m_parameters.m_backPeriod = 3.f;
        writheTask->m_parameters.m_legPeriod = 1.f;
        writheTask->m_parameters.m_armAmplitude = 0.8f;
        writheTask->m_parameters.m_backAmplitude = 2.f;
        writheTask->m_parameters.m_legAmplitude = 1.5f;
        writheTask->m_parameters.m_armStiffness = 13.f;
        writheTask->m_parameters.m_backStiffness = 13.f;
        writheTask->m_parameters.m_legStiffness = 11.f;
        writheTask->m_parameters.m_armDamping = 0.3f;
        writheTask->m_parameters.m_backDamping = 0.3f;
        writheTask->m_parameters.m_legDamping = 0.3f;
        writheTask->m_parameters.m_rollOverFlag = false;
        writheTask->m_parameters.m_effectorMask = bvmask_Full;
        writheTask->m_parameters.m_blendArms = 0.5f;
        writheTask->m_parameters.m_blendLegs = 0.5f;
        writheTask->m_parameters.m_blendBack = 0.5f;
        writheTask->activate();
      }

      rdsTask->m_parameters.m_SpinWhenInAir = false;

      m_teeterTimer += timeStep;
      static float awTime = 1.8f;
      if (m_teeterTimer < awTime)
      {
        if (!armsWindmillTask->isActive())
        {
          armsWindmillTask->updateBehaviourMessage(NULL); // initialise parameters.

          armsWindmillTask->m_parameters.m_angVelThreshold = 0.1f;
          armsWindmillTask->m_parameters.m_angVelGain = 1.f;
          armsWindmillTask->m_parameters.m_disableOnImpact = true;
          armsWindmillTask->m_parameters.m_mirrorMode = 1;
          armsWindmillTask->m_parameters.m_adaptiveMode = 0;
          armsWindmillTask->m_parameters.m_forceSync = true;
          armsWindmillTask->m_parameters.m_phaseOffset = 60.f;
          armsWindmillTask->m_parameters.m_shoulderStiffness = 13.f;
          armsWindmillTask->m_parameters.m_useLeft = true;
          armsWindmillTask->m_parameters.m_useRight = true;
          armsWindmillTask->m_parameters.m_IKtwist = 0;
          armsWindmillTask->m_parameters.m_dragReduction = 0.2f;
          armsWindmillTask->m_parameters.m_leftCircleDesc.partID = 10;
          armsWindmillTask->m_parameters.m_leftCircleDesc.radius1 = 0.75f;
          armsWindmillTask->m_parameters.m_leftCircleDesc.radius2 = 0.75f;
          armsWindmillTask->m_parameters.m_leftCircleDesc.normal = rage::Vector3(0.f, 0.5f, -0.1f);
          armsWindmillTask->m_parameters.m_leftCircleDesc.centre = rage::Vector3(0.f, 0.5f, -0.1f);

          armsWindmillTask->m_parameters.m_rightCircleDesc.partID = 10;
          armsWindmillTask->m_parameters.m_rightCircleDesc.radius1 = 0.75f;
          armsWindmillTask->m_parameters.m_rightCircleDesc.radius2 = 0.75f;

          armsWindmillTask->m_parameters.m_rightCircleDesc.normal = rage::Vector3(0.f, 0.5f, -0.1f);
          armsWindmillTask->m_parameters.m_rightCircleDesc.centre = rage::Vector3(0.f, 0.5f, -0.1f);

          armsWindmillTask->activate();
          armsWindmillTask->setOK2Deactivate(false);//Dont let shot deactivate this behaviour if we're using it here
        }
        float velForwards = -m_character->m_COMTM.c.Dot(m_character->m_COMvel);
        armsWindmillTask->m_parameters.m_leftCircleDesc.speed = 1.5f;
        armsWindmillTask->m_parameters.m_rightCircleDesc.speed = 1.5f;
        if (velForwards < 0.f)
        {
          armsWindmillTask->m_parameters.m_leftCircleDesc.speed *= -1.f;
          armsWindmillTask->m_parameters.m_rightCircleDesc.speed *= -1.f;
        }

        catchFallTask->m_parameters.m_effectorMask = bvmask_Full;
        if (armsWindmillTask->getUsingLeft() && edge2LeftShoulder < -0.2f )
          catchFallTask->m_parameters.m_effectorMask &= ~bvmask_ArmLeft;
        if (armsWindmillTask->getUsingRight() && edge2RightShoulder < -0.2f )
          catchFallTask->m_parameters.m_effectorMask &= ~bvmask_ArmRight;

      }
      else
      {
        catchFallTask->m_parameters.m_effectorMask = bvmask_Full;
        armsWindmillTask->deactivate();
      }
    }

    NmRsCBUHighFall* highFallTask = (NmRsCBUHighFall*)m_cbuParent->m_tasks[bvid_highFall];
    Assert(highFallTask);
    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    Assert(bodyBalanceTask);

    rage::Vector3 edge2Foot;
    edge2Foot = getLeftLeg()->getFoot()->getPosition() - balColReactTask->m_pos1stContact;//Assumes normal is level
    m_character->levelVector(edge2Foot);
    float edge2LeftFoot = edge2Foot.Dot(balColReactTask->m_normal1stContact);
    edge2Foot = getRightLeg()->getFoot()->getPosition() - balColReactTask->m_pos1stContact;//Assumes normal is level
    m_character->levelVector(edge2Foot);
    float edge2RightFoot = edge2Foot.Dot(balColReactTask->m_normal1stContact);

    //mmmmtodo don't call highfall untill feet are both over edge?
    if (m_parameters.callHighFall && !highFallTask->isActive() && m_character->hasCollidedWithEnvironment(bvmask_Full)== false
      && (edge2LeftFoot < -0.1) && (edge2RightFoot < -0.1))
    {
      armsWindmillTask->deactivate();
      catchFallTask->deactivate();
      bodyBalanceTask->deactivate();//mmmmtodo deactivate the balancer here e.g. what if shot or stagger are running
      m_highFall = true;
      sendFeedback(teet_HighFall);
      highFallTask->updateBehaviourMessage(NULL); // set to params defaults
      highFallTask->m_parameters.m_bodyStiffness = 11.f; 
      highFallTask->m_parameters.m_bodydamping = 1.f; 
      highFallTask->m_parameters.m_catchfalltime = 0.3f; 
      highFallTask->m_parameters.m_crashOrLandCutOff = 0.868f;
      highFallTask->m_parameters.m_pdStrength = 0.0f; 
      highFallTask->m_parameters.m_pdDamping = 0.0f;
      highFallTask->m_parameters.m_armAngSpeed = 7.85f;
      highFallTask->m_parameters.m_armAmplitude = 2.0f; 
      highFallTask->m_parameters.m_legRadius = 0.4f; 
      highFallTask->m_parameters.m_legAngSpeed = 7.85f;
      highFallTask->m_parameters.m_orientateBodyToFallDirection = false; 
      highFallTask->m_parameters.m_forwardRoll = false; 
      highFallTask->m_parameters.m_useZeroPose_withForwardRoll = false; 
      highFallTask->m_parameters.m_aimAngleBase = 1.0f; 
      highFallTask->m_parameters.m_armsUp = 0.18f; 
      highFallTask->m_parameters.m_forwardVelRotation = -0.02f; 
      highFallTask->m_parameters.m_footVelCompScale = 0.05f; 
      highFallTask->m_parameters.m_sideD = -1.f; 
      highFallTask->m_parameters.m_forwardOffsetOfLegIK = 0.2f; 
      highFallTask->m_parameters.m_legL = 0.8f; 
      highFallTask->m_parameters.m_catchFallCutOff = 0.878f;
      highFallTask->m_parameters.m_legStrength = 12.f; 
      highFallTask->m_parameters.m_ignorWorldCollisions = false;
      highFallTask->activate(); 

    }
  }

  void NmRsCBUTeeter::sendFeedback(int newState)
  {
    if (m_state != newState)
    {
      m_state = newState;
      NM_RS_DBG_LOGF(L"+++++++++++CHANGED = %i", m_state);
      //send a feedback message;
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 1;

        ART::ARTFeedbackInterface::FeedbackUserdata data;
        data.setInt(m_state);
        feedback->m_args[0] = data;

        strcpy(feedback->m_behaviourName, NMTeeterFeedbackName);
        feedback->onBehaviourEvent();
      }

    }

  }

#if ART_ENABLE_BSPY
  void NmRsCBUTeeter::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);
    static const char* teeterStateStrings[] =
    {
      "teet_Pre",           //0 nothing happening at the minute
      "teet_LeanAwayZone",  //1 Inside the lean away from edge zone
      "teet_FootClose2Edge",//2 one of the feet is close to the edge (Stop applying push force here?)
      "teet_PreTeeter",     //3 Waving arms around (Stop applying push force here)
      "teet_Teeter",        //4 Teetering
      "teet_FallOnGround",  //5 Fell over but not over the edge
      "teet_OverEdge",      //6 Gone over the edge
      "teet_HighFall",      //7 Doing a highFall

    };

    bspyTaskVar(m_parameters.edgeLeft, true);
    bspyTaskVar(m_parameters.edgeRight, true);
    bspyTaskVar(m_parameters.useExclusionZone, true);
    bspyTaskVar(m_parameters.m_useHeadLook, true);
    bspyTaskVar(m_parameters.callHighFall, true);
    bspyTaskVar(m_parameters.leanAway, true);

    bspyTaskVar(m_teeterTimer, false);
    bspyTaskVar_StringEnum(m_state, teeterStateStrings, false);
    bspyTaskVar(m_characterIsFalling, false);

  }
#endif // ART_ENABLE_BSPY
}

