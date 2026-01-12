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
* the character catches his fall when falling over. 
* He will twist his spine and look at where he is falling. He will also relax after hitting the ground.
* He always braces against a horizontal ground.
*/


#include "NmRsInclude.h"

#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V

#include "NmRsCBU_Stumble.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_BodyBalance.h"
#include "NmRsCBU_CatchFall.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_Grab.h" 
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_SpineTwist.h"
#include "NmRsCBU_Teeter.h"


namespace ART
{
  NmRsCBUStumble::NmRsCBUStumble(ART::MemoryManager* services) : CBUTaskBase(services, bvid_stumble),
    m_reachLength(0.58f),
    m_probeLength(m_reachLength + 3.0f),
    m_predictionTime(1.f),
    m_behaviourTime(0.0f),
    m_onGround(false)
  {
    initialiseCustomVariables();
  }

  NmRsCBUStumble::~NmRsCBUStumble()
  {
  }

  void NmRsCBUStumble::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
  }

  void NmRsCBUStumble::onActivate()
  {

    Assert(m_character);
    m_allFours = true;
    m_onGround = false;
    m_floorTime = -0.1f;//MMMMHandsKnees
    m_restart = 2.f;//MMMMHandsKnees
    m_injuryVal = 0.f;
    m_hipPitchAim = 0.f;

    m_fallDirection.Set(0,0,0);
    m_forwardsAmount = 0;
    m_bodyStrength = 1.f;
    m_upwardsness = 0.f;
    m_headAvoidActive = false;
    m_behaviourTime = 0.0f;
    m_OnFloorTimer = 0.0f;
    m_2Kneestimer = -0.1f;

    m_body->resetEffectors(kResetCalibrations | kResetAngles);

    getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_torsoStiffness, 0.5f, bvmask_LowSpine);
    if (m_parameters.m_useHeadLook)
      getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_torsoStiffness, 0.5f, bvmask_CervicalSpine);
    m_body->setStiffness(m_parameters.m_armsStiffness-1.f, 1.f, bvmask_ArmLeft | bvmask_ArmRight);
    m_body->setStiffness(m_parameters.m_legsStiffness, 0.5f, bvmask_LegLeft | bvmask_LegRight);

    getLeftArmInputData()->getWrist()->setMuscleStrength(m_parameters.wristMS);
    getRightArmInputData()->getWrist()->setMuscleStrength(m_parameters.wristMS);

    m_body->setOpposeGravity(1.0f);

    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    if (catchFallTask->m_handsAndKnees)//bend knees 
    {
      m_kneeBendL = m_character->getRandom().GetRanged(0.1f, 0.7f);
      m_kneeBendR = m_character->getRandom().GetRanged(0.1f, 0.7f);
    }
    else//have little or no knee bend (to stop character holding legs off ground in forward catch fall)
    {
      m_kneeBendL = m_character->getRandom().GetRanged(0.f, 0.3f);
      m_kneeBendR = m_character->getRandom().GetRanged(0.f, 0.3f);
    }

    m_randomSpineL2 = m_character->getRandom().GetRanged(-0.1f, 0.1f);

    getLeftLegInputData()->getAnkle()->setDesiredLean1(-m_kneeBendL);
    getRightLegInputData()->getAnkle()->setDesiredLean1(-m_kneeBendR);
    getLeftLegInputData()->getHip()->setDesiredLean1(m_kneeBendL*m_bodyStrength);
    getRightLegInputData()->getHip()->setDesiredLean1(m_kneeBendR*m_bodyStrength);
    getLeftLegInputData()->getKnee()->setDesiredAngle(-2*m_kneeBendL*m_bodyStrength);
    getRightLegInputData()->getKnee()->setDesiredAngle(-2*m_kneeBendR*m_bodyStrength);

    m_leftArmState.init(m_character, this);
    m_leftArmState.enter(getLeftArm(), true, bvmask_ArmLeft);
    m_rightArmState.init(m_character, this);
    m_rightArmState.enter(getRightArm(), false, bvmask_ArmRight);

#ifdef NM_RS_CBU_ASYNCH_PROBES
    m_character->InitializeProbe(NmRsCharacter::pi_catchFallLeft);
    m_character->InitializeProbe(NmRsCharacter::pi_catchFallRight);
#endif
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);
    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    Assert(bodyBalanceTask);
    bodyBalanceTask->updateBehaviourMessage(NULL);// sets values to defaults
    bodyBalanceTask->m_parameters.m_useBodyTurn = false;
    bodyBalanceTask->m_parameters.m_useHeadLook = false;
    bodyBalanceTask->m_parameters.m_backwardsAutoTurn = true;
    bodyBalanceTask->m_parameters.m_backwardsArms = true;
    bodyBalanceTask->m_parameters.m_spineStiffness = 10.0f;
    bodyBalanceTask->m_parameters.m_spineDamping = 0.9f;
    bodyBalanceTask->m_parameters.m_armStiffness = 10.0f;
    bodyBalanceTask->m_parameters.m_elbow = 1.2f;
    bodyBalanceTask->m_parameters.m_shoulder = 0.8f;

    bodyBalanceTask->activate();


    dynamicBalancerTask->setAnkleEquilibrium(-0.00f);
    if (dynamicBalancerTask->isActive())
    {
      if (!balColReactTask->isActive() || balColReactTask->m_balancerState == bal_Normal || balColReactTask->m_balancerState == bal_Rebound)
      {
        dynamicBalancerTask->setLeftLegStiffness(10.f);
        dynamicBalancerTask->setRightLegStiffness(10.f);
      }
      dynamicBalancerTask->setOpposeGravityLegs(1.f);
      dynamicBalancerTask->setOpposeGravityAnkles(1.f);

      dynamicBalancerTask->setLowerBodyGravityOpposition(m_body);
      dynamicBalancerTask->calibrateLowerBodyEffectors(m_body);

      //mmmmADDEDdynamicBalancerTask->autoLeanHipsRandom(-0.5f,0.4f,0.5f,1.2f);
      //mmmmADDEDdynamicBalancerTask->autoLeanRandom(0.07f,0.15f,0.5f,1.f);
      //mmmmADDEDrage::Vector3 position(87.3087f, 158.152f, 6.87029f);
      //mmmmADDEDdynamicBalancerTask->autoLeanForceToObject(-1,position, 0.02f, 0);
    }

    catchFallTask->m_handsAndKnees = true;//mmmmtodo mmmmnote messes up setFallingReaction setup
    catchFallTask->m_callRDS = true;//mmmmtodo mmmmnote messes up setFallingReaction setup
    catchFallTask->m_comVelRDSThresh = 2.0f;//mmmmtodo mmmmnote messes up setFallingReaction setup
    catchFallTask->m_armReduceSpeed = 0.2f;//mmmmtodo mmmmnote messes up setFallingReaction setup
    m_stumbleState = sbleS_Normal;
  }

  void NmRsCBUStumble::onDeactivate()
  {
    Assert(m_character);
    if (m_parameters.m_useHeadLook)
    {
      m_cbuParent->m_tasks[bvid_headLook]->deactivate();
    }
    m_cbuParent->m_tasks[bvid_spineTwist]->deactivate();
    m_cbuParent->m_tasks[bvid_bodyBalance]->deactivate();

    //De-activate subTasks
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    if (dynamicBalancerTask->isActive() && m_character->noBehavioursUsingDynBalance())
      dynamicBalancerTask->deactivate();

  }

  CBUTaskReturn NmRsCBUStumble::onTick(float timeStep)
  { 

    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    Assert(bodyBalanceTask);
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);

    float hipYaw, hipPitch, hipRoll;
    float drop = m_behaviourTime*0.12f;//1.2f;
    bool onFloor1 = m_character->hasCollidedWithEnvironment(bvmask_HandLeft | bvmask_HandRight) ||
      getLeftLeg()->getThigh()->collidedWithNotOwnCharacter() ||
      getRightLeg()->getThigh()->collidedWithNotOwnCharacter();
    float dropVal = m_parameters.dropVal;
    static float hipPitchVal = 3.0f;
    static float hipPitchVal1 = 4.0f;
    if (bodyBalanceTask->isActive())
    {
      if (m_behaviourTime > m_parameters.staggerTime - 0.3f) //0.15f
      {
        rage::Matrix34 tmCom;
        tmCom.Set(m_character->m_COMTM);
        //make angvel from COM or Spine2
        float angComp = rage::Clamp(m_character->m_COMrotvel.Dot(tmCom.a), -3.f, 3.f);//side
        float linComp = rage::Clamp(m_character->m_COMvel.Dot(tmCom.c), -3.f, 3.f);//back
        rage::Vector3 hip2head = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
        hip2head.Normalize();
        float posComp = hip2head.Dot(tmCom.c);//back

        float hipPitch = rage::Clamp(-0.7f*posComp - 0.25f*linComp - 0.25f*angComp, -0.7f, 0.5f);
        dynamicBalancerTask->setHipPitch(hipPitch);
        dynamicBalancerTask->autoLeanHipsCancel();
      }
      else
      {
        float inc = m_parameters.leanRate*timeStep*60.f;
        float minHP = -m_parameters.maxLeanForward;
        float maxHP = m_parameters.maxLeanBack;
        if ( dynamicBalancerTask->getHipPitch() < m_hipPitchAim)
        {
          dynamicBalancerTask->setHipPitch(dynamicBalancerTask->getHipPitch() + inc);
          if (dynamicBalancerTask->getHipPitch() >= m_hipPitchAim)
            m_hipPitchAim = m_character->getRandom().GetRanged(minHP,maxHP);
        }
        else
        {
          dynamicBalancerTask->setHipPitch(dynamicBalancerTask->getHipPitch() - inc);
          if (dynamicBalancerTask->getHipPitch() <= m_hipPitchAim)
            m_hipPitchAim = m_character->getRandom().GetRanged(minHP,maxHP);
        }
      }

      m_stumbleState = sbleS_Normal;
      //Move towards grab point?
      //pick only the nearest to head to at the moment
      NmRsCBUGrab* grabTask = (NmRsCBUGrab*)m_cbuParent->m_tasks[bvid_grab];
      Assert(grabTask);
      if (grabTask->isActive())
      {
        grabTask->m_parameters.move2Radius = rage::SqrtfSafe(m_parameters.grabRadius2);//for EA
        if (grabTask->getGrabing())
        {
          dynamicBalancerTask->setDontChangeStep(true);
        }
        //grab will pull the hands to the grab point itself if trying to grab
        else if (!grabTask->getTryingToGrab())
        {
          rage::Vector3 grabPos;
          rage::Vector3 localCOMPos;
          int grabType = grabTask->getGrabType();
#if NM_EA
          if (grabType == 4)//fromEA
          {
            grabTask->m_parameters.move2Radius = rage::SqrtfSafe(m_parameters.grabRadius2);
            grabPos.Zero();
            if (grabTask->m_parameters.useLeft && grabTask->m_parameters.useRight)
            {
              rage::Vector3 posL = grabTask->getClosestGrabPointLeft();
              rage::Vector3 posR = grabTask->getClosestGrabPointRight();
              if (!posL.IsZero() && posR.IsZero())
                grabPos = posL;
              if (!posR.IsZero() && posL.IsZero())
                grabPos = posR;
              if (!posR.IsZero() && !posL.IsZero())
              {//goto closest
                if ((m_character->m_COM - posL).Mag2() > (m_character->m_COM - posR).Mag2())
                  grabPos = posR;
                else
                  grabPos =  posL;
              }
            }
            else if (grabTask->m_parameters.useLeft)
            {
              if (!grabTask->getClosestGrabPointLeft().IsZero())
                grabPos = grabTask->getClosestGrabPointLeft();
            }
            else if (grabTask->m_parameters.useRight)
            {
              if (!grabTask->getClosestGrabPointRight().IsZero())
                grabPos = grabTask->getClosestGrabPointRight();
            }
            if (!grabPos.IsZero())
            {
              float dist2 = (grabPos - m_character->m_COM).Mag2();
              static float mult = m_parameters.leanTowards;
              dynamicBalancerTask->autoLeanForceToPosition(grabPos, (m_parameters.grabRadius2-dist2)*mult/m_parameters.grabRadius2, 8);
              dynamicBalancerTask->useCustomTurnDir(true, grabPos);
              m_stumbleState = sbleS_ToGrab;
#if ART_ENABLE_BSPY
              bspyScratchpad(m_character->getBSpyID(), "LFTo", grabPos);
#endif // ART_ENABLE_BSPY
            }
            else
              dynamicBalancerTask->autoLeanForceCancel();
          }
          else
#endif//#if NM_EA
          {
            m_character->boundToLocalSpace(false, &localCOMPos,m_character->m_COM,grabTask->m_parameters.instanceIndex,grabTask->m_parameters.boundIndex);
            float distance2,distance2B;
            switch (grabType)
            {
            case 0://1 or 2 Points
              if (grabTask->m_parameters.useLeft && grabTask->m_parameters.useRight)
              {
                grabPos = 0.5f*(grabTask->m_parameters.pos+grabTask->m_parameters.pos1);
              }
              else if (grabTask->m_parameters.useLeft)
              {
                grabPos = grabTask->m_parameters.pos1;
              }
              else if (grabTask->m_parameters.useRight)
              {
                grabPos = grabTask->m_parameters.pos;
              }
              break;
            case 1://line
              grabPos = 0.5f*(grabTask->m_parameters.pos+grabTask->m_parameters.pos1);
              break;
            case 2://plane //should be nearest point?
              grabPos = 0.25f*(grabTask->m_parameters.pos+grabTask->m_parameters.pos1+grabTask->m_parameters.pos2+grabTask->m_parameters.pos3);
              break;
            case 3://4points//perhaps should be nearest of the 2 2pointaverages?
              distance2 = (grabTask->m_parameters.pos - localCOMPos).Mag2();
              grabPos = grabTask->m_parameters.pos;
              distance2B = (grabTask->m_parameters.pos1 - localCOMPos).Mag2();
              if (distance2 > distance2B)
              {
                distance2 = distance2B;
                grabPos = grabTask->m_parameters.pos1;
              }
              distance2B = (grabTask->m_parameters.pos2 - localCOMPos).Mag2();
              if (distance2 > distance2B)
              {
                distance2 = distance2B;
                grabPos = grabTask->m_parameters.pos2;
              }
              distance2B = (grabTask->m_parameters.pos3 - localCOMPos).Mag2();
              if (distance2 > distance2B)
              {
                distance2 = distance2B;
                grabPos = grabTask->m_parameters.pos3;
              }
              break;
            default:
              grabPos.Zero();
              break;
            } 
            float dist2 = (grabPos - localCOMPos).Mag2();
            if (!grabPos.IsZero() && dist2 < m_parameters.grabRadius2)
            {
              rage::Vector3 grabPosWorld;
              dynamicBalancerTask->autoLeanForceToObject(grabTask->m_parameters.instanceIndex, grabTask->m_parameters.boundIndex, grabPos, (m_parameters.grabRadius2-dist2)*0.27f/m_parameters.grabRadius2, 8);
              grabPosWorld -= m_character->m_COM;
              dynamicBalancerTask->useCustomTurnDir(true, grabPosWorld);
              m_stumbleState = sbleS_ToGrab;
#if ART_ENABLE_BSPY
              bspyScratchpad(m_character->getBSpyID(), "LFTo", grabPosWorld);
#endif // ART_ENABLE_BSPY
            }
            else
              dynamicBalancerTask->autoLeanForceCancel();
          }
        }
        else
        {
          dynamicBalancerTask->autoLeanForceCancel();
          dynamicBalancerTask->setDontChangeStep(false);
        }
      }
      else
      {
        dynamicBalancerTask->setDontChangeStep(false);
      }


      if (m_behaviourTime > m_parameters.staggerTime || dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK)
        bodyBalanceTask->deactivate();
      //if ( dynamicBalancerTask->getHipPitch()<0.f)
      {
        float straightness = rage::Max(-rage::Abs(dynamicBalancerTask->getHipPitch())*.25f,-0.4f);
        if (dynamicBalancerTask->footState() != NmRsCBUDynBal_FootState::kNotStepping)
          straightness = 0.f;
        dynamicBalancerTask->setLegStraightnessModifier(straightness);
      }
      /*else
      dynamicBalancerTask->setLegStraightnessModifier(0.f);*/
      rage::Vector3 hip2head = getSpine()->getHeadPart()->getPosition() - getSpine()->getPelvisPart()->getPosition();
      hip2head.Normalize();
      float maxHipHeight;
      maxHipHeight = hip2head.Dot(m_character->m_gUp);
      hip2head = -getLeftLeg()->getFoot()->getPosition() + getSpine()->getPelvisPart()->getPosition();
      maxHipHeight = hip2head.Dot(m_character->m_gUp);
      hip2head = -getRightLeg()->getFoot()->getPosition() + getSpine()->getPelvisPart()->getPosition();
      if (maxHipHeight < hip2head.Dot(m_character->m_gUp))
        maxHipHeight = hip2head.Dot(m_character->m_gUp);

      hip2head = getSpine()->getHeadPart()->getPosition() - getSpine()->getPelvisPart()->getPosition();
      if (m_parameters.useArmsBrace && ((hip2head.Dot(m_character->m_gUp) < 0.7f) || (maxHipHeight < 0.6f)))
      {
        armsBrace();
      }
    }
    else//(bodyBalanceTask->isActive())
    {
      m_mask = m_parameters.fallMask;
      m_stumbleState = sbleS_Falling;
      m_character->m_posture.useZMP = false;
      m_body->setOpposeGravity(0.5f, bvmask_ArmLeft | bvmask_ArmRight);
      m_body->setOpposeGravity(0.0f, bvmask_HandLeft | bvmask_HandRight);

      m_character->m_posture.leftArmAutoSet = true;
      m_character->m_posture.rightArmAutoSet = true;


      //mmmmADDEDarmsBrace(timeStep);
      NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)m_cbuParent->m_tasks[bvid_headLook];    
      if (m_parameters.m_useHeadLook && !headLookTask->isActive())
      {
        headLookTask->initialiseCustomVariables();
        headLookTask->updateBehaviourMessage(NULL); // initialise the parameters
        headLookTask->m_parameters.m_stiffness = m_parameters.m_torsoStiffness;
        headLookTask->activate();
      }

      NM_RS_DBG_LOGF(L"CATCH FALLING");
      if (catchFallTask->m_handsAndKnees)
      {
        if (m_floorTime>0.f)
          m_floorTime += timeStep;
        if (m_floorTime>0.1f && m_floorTime<0.5f)
          dynamicBalancerTask->setHipPitch(-0.5f);//helps the character stand up (hip pitch should be set to zero to keep it standing up)
        //else
        //  dynamicBalancerTask->setHipPitch(0.f);
      }

      if (m_parameters.m_zAxisSpinReduction>0.f)
        m_character->antiSpinAroundVerticalAxisJuice(m_parameters.m_zAxisSpinReduction);

      float strengthLeft = rage::Max(0.3f, m_leftArmState.m_strength);
      float strengthRight = rage::Max(0.3f, m_rightArmState.m_strength);
      NM_RS_DBG_LOGF(L"strengthLeft: %.3f", strengthLeft, L", strengthRight: %.3f", strengthRight);

      float averageSpeed = rage::Sqrtf(2.f*m_character->getKineticEnergyPerKilo_RelativeVelocity());
      NM_RS_DBG_LOGF(L"averageSpeed: %.3f", averageSpeed);

      float add = (averageSpeed*2.f - m_bodyStrength)*5.f*timeStep;
      m_bodyStrength = rage::Clamp(m_bodyStrength + add, 0.2f, 1.f);

      if (rage::Min(strengthLeft, strengthRight) == 0.3f)//mmmmADDED && !catchFallTask->m_handsAndKnees)
      {
        //m_character->sendFeedbackFinish(NMStumbleFeedbackName);
        //mmmmADDEDm_character->sendFeedbackSuccess(NMStumbleFeedbackName);
      }
      if (rage::Min(strengthLeft, strengthRight) < 0.9f && m_bodyStrength < 1.f)
      {
        //m_character->sendFeedbackSuccess(NMStumbleFeedbackName);
        //NM_RS_CBU_DRAWPOINT(m_character->m_COM, 1.f, rage::Vector3(1,0,0));
      }

      float strength = m_bodyStrength;
      NM_RS_DBG_LOGF(L"strength: %.3f", strength);

      float effectorStiffnessArms = 0.5f;
      float effectorStiffnessSpine = 0.5f;
      float strengthLeftWrist = rage::Max(0.6f, strengthLeft);//stop wrist going too floppy
      float strengthRightWrist = rage::Max(0.6f, strengthRight);//stop wrist going too floppy
      if (catchFallTask->m_handsAndKnees)
      {
        static float strengthLeftHand = 1.1f;//go lower than arms
        static float strengthRightHand = 1.1f;//go lower than arms
        strengthLeftWrist = strengthLeftHand;
        strengthRightWrist = strengthRightHand;
        //if (m_floorTime>0.1f)//Gives a nice sink to the arm impact before pushing back
        {
          //Set joint stiffness parameters for a hands and knees type onFloor
          static float strengthLeftArm = 1.1f;
          static float strengthRightArm = 1.1f;
          static float strengthBody = 1.0f;//0.9f slightly too loose
          static float effectorStiffArms = 1.0f;
          static float effectorStiffSpine = 1.0f;

          strengthLeft = strengthLeftArm;
          strengthRight = strengthRightArm;
          strength = strengthBody;
          effectorStiffnessArms = effectorStiffArms;
          effectorStiffnessSpine = effectorStiffSpine;

        }
      }

      if (catchFallTask->m_handsAndKnees)
        getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_torsoStiffness*strength*1.2f, 0.5f, bvmask_LowSpine, &effectorStiffnessSpine);
      else
        getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_torsoStiffness*strength, 0.5f, bvmask_LowSpine, &effectorStiffnessSpine);
      if (m_parameters.m_useHeadLook)
        getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_torsoStiffness*strength, 0.5f, bvmask_CervicalSpine);

      m_body->setStiffness(m_parameters.m_armsStiffness*strengthLeft, 1.0f, bvmask_ArmLeft| bvmask_ArmRight, &effectorStiffnessArms);
      m_body->setStiffness(m_parameters.m_legsStiffness*strength, 0.5f, bvmask_LegLeft| bvmask_LegRight);

      // TDL make wrists not go mega floppy, it causes nastiness, because wrist limits aren't too great
      float wristMuscleStiffness = m_parameters.wristMS;
      m_body->setStiffness((13.f + m_parameters.m_armsStiffness)*0.5f*strengthLeftWrist, 1.f, bvmask_HandLeft | bvmask_HandRight, &wristMuscleStiffness);
      m_body->setStiffness(10.f, 1.f, bvmask_FootLeft | bvmask_FootRight);

      // set head look target
      rage::Vector3 averageFloorVel = (m_rightArmState.m_floorVel + m_leftArmState.m_floorVel) * 0.5f;
      m_character->setFloorVelocityFromColliderRefFrameVel();//This takes precedent over DynamicBalancer but not rollDownStairs

      m_fallDirection = getSpine()->getSpine3Part()->getLinearVelocity() - averageFloorVel;
      m_character->levelVector(m_fallDirection, m_character->vectorHeight(m_fallDirection) - 0.5f*9.8f*m_predictionTime); // prediction into future
      m_fallDirection.Normalize();

      m_forwardsAmount = -m_fallDirection.Dot(m_character->m_COMTM.c);

      float velForwards = -(m_character->m_COMvelRelative).Dot(m_character->m_COMTM.c);

      NM_RS_DBG_LOGF(L"velForwards: %.3f", velForwards);
      float faceDown = rage::Clamp(m_character->m_COMTM.c.Dot(m_character->getUpVector()), 0.f, 1.f);
      m_upwardsness = rage::Clamp(m_character->m_COMTM.b.Dot(m_character->getUpVector()), 0.f, 1.f);

      NM_RS_DBG_LOGF(L"kneeBendL: %.3f", m_kneeBendL, L", kneeBendR: %.3f", m_kneeBendR);
      getLeftLegInputData()->getAnkle()->setDesiredLean1(-m_kneeBendL);
      getRightLegInputData()->getAnkle()->setDesiredLean1(-m_kneeBendR);

      float pitch = rage::Clamp(-velForwards*2.f, 0.f, 1.f) * m_upwardsness;
      NM_RS_DBG_LOGF(L"pitch: %.3f", pitch);

      float maxExtraSit = 1.f;
      float spineLean1Scale = 0.2f;
      if (/*(m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl) &&*/ !catchFallTask->m_handsAndKnees)
      {
        maxExtraSit = 0.5f; // 0.2f;// Extra sit 
        spineLean1Scale = 0.1f;
      }
      //extra sit passed to the spine 
      float extraSit = rage::Clamp(pitch + -0.5f*m_forwardsAmount, 0.f, maxExtraSit); 
      //extra sit passed to the hips
      float extraSitHips = extraSit;

      if (catchFallTask->m_handsAndKnees)
      {
        extraSit=0.8f;
        extraSitHips = extraSit;
        rage::Vector3 bodyBack = m_character->m_COMTM.c;
        float onBackness = -bodyBack.Dot(m_character->m_gUp);
        if (onBackness > 0.f)
        {
          extraSit += rage::Min(2.f*onBackness*1.1f,1.1f/spineLean1Scale/strength);
          extraSitHips = 1.3f - 0.2f*onBackness;
        }
      }

      getSpineInputData()->getSpine0()->setDesiredLean1(extraSit*spineLean1Scale*strength);
      getSpineInputData()->getSpine1()->setDesiredLean1(extraSit*spineLean1Scale*strength);
      getSpineInputData()->getSpine2()->setDesiredLean1(extraSit*spineLean1Scale*strength);
      getSpineInputData()->getSpine3()->setDesiredLean1(extraSit*spineLean1Scale*strength);

      getSpineInputData()->getSpine0()->setDesiredLean2(m_randomSpineL2);
      getSpineInputData()->getSpine1()->setDesiredLean2(m_randomSpineL2);
      getSpineInputData()->getSpine2()->setDesiredLean2(m_randomSpineL2);
      getSpineInputData()->getSpine3()->setDesiredLean2(m_randomSpineL2);

      bool doingTwist = ((NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist])->doingTwist();
      if ((!doingTwist) && (!catchFallTask->m_handsAndKnees))
      {
#ifdef NM_COWBOY
        if (m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl)
        {
          // twist into fall
          rage::Matrix34 spine3TM;
          getSpine()->getSpine3Part()->getBoundMatrix(&spine3TM);

          // project fall direction on spine3 yz plane and normalize
          //NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, -spine3TM.c, rage::Vector3(0,0,1));
          rage::Vector3 fallDirProjection;
          fallDirProjection.Cross(spine3TM.a, m_fallDirection);
          fallDirProjection.Cross(spine3TM.a);
          fallDirProjection.Normalize();
          //NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, m_fallDirection, rage::Vector3(1,1,0));
          //NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, fallDirProjection, rage::Vector3(1,0,1));

          // cross with spine3 forward to get twist vector
          rage::Vector3 torqueVector;
          torqueVector.Cross(fallDirProjection, -spine3TM.c);
          //NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, torqueVector, rage::Vector3(1,0,0));

          // twist spine
          float spineTwistAmount = torqueVector.Mag() / -1.0f;
          if(torqueVector.Dot(spine3TM.a) < 0.0f)
            spineTwistAmount *= -1.0f;
          float twistSpeed = 0.7f;

          // limbs: will be one frame behind.
          getSpineInputData()->getSpine0()->setDesiredTwist(getSpine()->getSpine0()->getDesiredTwist() + (spineTwistAmount - getSpine()->getSpine0()->getDesiredTwist())*twistSpeed);
          getSpineInputData()->getSpine1()->setDesiredTwist(getSpine()->getSpine1()->getDesiredTwist() + (spineTwistAmount - getSpine()->getSpine1()->getDesiredTwist())*twistSpeed);
          getSpineInputData()->getSpine2()->setDesiredTwist(getSpine()->getSpine2()->getDesiredTwist() + (spineTwistAmount - getSpine()->getSpine2()->getDesiredTwist())*twistSpeed);
          getSpineInputData()->getSpine3()->setDesiredTwist(getSpine()->getSpine3()->getDesiredTwist() + (spineTwistAmount - getSpine()->getSpine3()->getDesiredTwist())*twistSpeed);

          // apply torque to spine0 to counter spine twist
          float torqueScale = 1.0f/(1.0f+20.0f*m_behaviourTime);
          if(torqueScale > 0.1)
          {
            torqueScale *= -50.0f * torqueScale;
            getSpine()->getSpine0Part()->applyTorque(torqueVector * torqueScale);
            //NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, torqueVector * torqueScale, rage::Vector3(1,0,0));
          }
        } 
        else 
#endif//#ifdef NM_COWBOY
        {
          float timeScale = 1.f/(1.f + 0.4f*30.f*timeStep);

          // limbs: will be one frame behind.
          getSpineInputData()->getSpine0()->setDesiredTwist(getSpine()->getSpine0()->getDesiredTwist()*timeScale);
          getSpineInputData()->getSpine1()->setDesiredTwist(getSpine()->getSpine1()->getDesiredTwist()*timeScale);
          getSpineInputData()->getSpine2()->setDesiredTwist(getSpine()->getSpine2()->getDesiredTwist()*timeScale);
          getSpineInputData()->getSpine3()->setDesiredTwist(getSpine()->getSpine3()->getDesiredTwist()*timeScale);
        }
      }

      getLeftLegInputData()->getHip()->blendToZeroPose((NmRsEffectorBase*)getLeftLeg()->getHip(), 1.f);
      getRightLegInputData()->getHip()->blendToZeroPose((NmRsEffectorBase*)getRightLeg()->getHip(), 1.f);

      getLeftLegInputData()->getHip()->setDesiredLean1((m_kneeBendL+extraSitHips)*strength);
      getRightLegInputData()->getHip()->setDesiredLean1((m_kneeBendR+extraSitHips)*strength);

      float kneeScale = (1.f+faceDown)*strength - faceDown; // this complex line is to get the guy to not have his knee at an extreme angle when falling on his front

      if (catchFallTask->m_handsAndKnees)
      {
        kneeScale = 1.f;

        getLeftLegInputData()->getHip()->setDesiredLean2(-0.3f);
        getRightLegInputData()->getHip()->setDesiredLean2(-0.3f);

        getLeftLegInputData()->getHip()->setDesiredTwist(-0.0f);
        getRightLegInputData()->getHip()->setDesiredTwist(-0.0f);
      }
      //minimum kneebend below is enough to just keep knee on floor if lying flat onface. i.e. knee/ankle/toe triangle
      getLeftLegInputData()->getKnee()->setDesiredAngle(rage::Min(-2.f*m_kneeBendL*kneeScale,-0.3f));
      getRightLegInputData()->getKnee()->setDesiredAngle(rage::Min(-2.f*m_kneeBendR*kneeScale,-0.3f));

      rage::Vector3 headTarget = getSpine()->getHeadPart()->getPosition() + m_fallDirection;
      //NM_RS_CBU_DRAWPOINT(headTarget, 0.2f, rage::Vector3(1,0,0));

      if (m_parameters.m_useHeadLook)
      {
        rage::Vector3 headTargetVel = getSpine()->getSpine3Part()->getLinearVelocity();
        NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)m_cbuParent->m_tasks[bvid_headLook];    
        if (catchFallTask->m_handsAndKnees)
        {
          //look horizontally forward
          rage::Matrix34 tmCom;
          getSpine()->getSpine2Part()->getBoundMatrix(&tmCom); 
          rage::Vector3 headUp = tmCom.a - tmCom.c; //spine up(a) and spine forward(c)
          m_character->levelVector(headUp);
          headTarget = getSpine()->getHeadPart()->getPosition() + headUp;
        }
        headLookTask->m_parameters.m_pos = headTarget;
        headLookTask->m_parameters.m_instanceIndex = -1;          
        headLookTask->m_parameters.m_stiffness = (m_parameters.m_torsoStiffness+2.f)*rage::Max(strengthLeft, strengthRight);
        headLookTask->m_parameters.m_vel = headTargetVel;
        NM_RS_DBG_LOGF(L"headLookStiffness1: %.3f", (m_parameters.m_torsoStiffness+2.f)*rage::Max(strengthLeft, strengthRight));
      }

      if (catchFallTask->m_handsAndKnees)
      {
        NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
        Assert(balColReactTask);

        if ((m_character->hasCollidedWithWorld(bvmask_ArmLeft | bvmask_ArmRight) || !dynamicBalancerTask->isActive()) && 
          (!(dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK && (balColReactTask->m_balancerState == bal_Drape || balColReactTask->m_balancerState == bal_DrapeForward))))
        {
          //redundant lines below
          dynamicBalancerTask->taperKneeStrength(false);//in order that the character is to stand up from bent legs / bent legs should be strong enough
          //dynamicBalancerTask->setStepWithBoth(true);
        }
        else
        {
          //redundant line below
          dynamicBalancerTask->setStepWithBoth(false);
        }
      }

      bool probeHasHit = false;
      if (m_leftArmState.tick(timeStep))
        if ((m_character->m_probeHitPos[NmRsCharacter::pi_catchFallLeft]-getLeftArm()->getHand()->getPosition()).Mag() <0.7f)
          probeHasHit = true;
      if (m_rightArmState.tick(timeStep))
        if ((m_character->m_probeHitPos[NmRsCharacter::pi_catchFallRight]-getRightArm()->getHand()->getPosition()).Mag() <0.7f)
          probeHasHit = true;

      //nmrsSetAngles(getLeftArm()->getWrist(),0.f,0.f,0.f);
      //nmrsSetAngles(getRightArm()->getWrist(),0.f,0.f,0.f);

      if (m_parameters.twistSpine > 0)
      {
        NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist];      
        if (probeHasHit && !spineTwistTask->isActive())
        {
          spineTwistTask->initialiseCustomVariables();
          spineTwistTask->setSpineTwistTwistClavicles(true);
          spineTwistTask->activate();
        }
        if (!probeHasHit)
          spineTwistTask->deactivate();

        if (m_parameters.twistSpine == 2)
        {
          rage::Vector3 hip2Head = getSpine()->getHeadPart()->getPosition()- getSpine()->getPelvisPart()->getPosition();
          rage::Vector3 left2Right = getRightLeg()->getFoot()->getPosition()- getLeftLeg()->getFoot()->getPosition();
          left2Right.Normalize();
          float spineTwist = left2Right.Dot(hip2Head);
          left2Right = getRightArm()->getShoulder()->getJointPosition()- getLeftArm()->getShoulder()->getJointPosition();
          left2Right.Normalize();
          spineTwist = left2Right.Dot(m_character->m_gUp);
          if (spineTwist>0.f)
            spineTwist = 1.f;
          else
            spineTwist = -1.f;
          if (getLeftArm()->getHand()->collidedWithEnvironment() && !getRightArm()->getHand()->collidedWithEnvironment())
            spineTwist = 1.f;
          if (!getLeftArm()->getHand()->collidedWithEnvironment() && getRightArm()->getHand()->collidedWithEnvironment())
            spineTwist = -1.f;
          if (m_forwardsAmount<0.f)
            spineTwist *= -1.f;

          headTarget = 0.5f*(getRightLeg()->getFoot()->getPosition()+ getLeftLeg()->getFoot()->getPosition());
          headTarget += left2Right*spineTwist*9.f;
        }
        spineTwistTask->setSpineTwistPos(headTarget);
      }

      if (m_character->hasCollidedWithWorld(bvmask_ArmLeft | bvmask_ArmRight))
      {
        m_headAvoidActive = true;
        m_onGround = true; // this may (most likely is) naive. will trigger shot on ground reaction if it hits anything.
      }

      if (m_character->hasCollidedWithWorld(bvmask_ArmLeft) && m_character->hasCollidedWithWorld(bvmask_ArmRight))
      {
        m_OnFloorTimer += timeStep;
        if (m_OnFloorTimer > 0.5f) // && stationary?
        {
          ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
          if (feedback)
          {
            feedback->m_agentID = m_character->getID();
            feedback->m_argsCount = 1;

            ART::ARTFeedbackInterface::FeedbackUserdata data;
            data.setInt(1);//Success state:0 = dunno, 2 = hands+Knees, 3 = sitting down
            feedback->m_args[0] = data;

            strcpy(feedback->m_behaviourName, NMStumbleFeedbackName);
            feedback->onBehaviourEvent();
          }
        }
      }
      else
      {
        m_OnFloorTimer = 0.f;
      }

      if (m_headAvoidActive)
      {
        NM_RS_DBG_LOGF(L"normalX: ", 0, ", avoid: ", 0.7f);
        rage::Vector3 down = -m_fallDirection;

        getSpine()->keepHeadAwayFromGround(getSpineInput(), 1.0f, &down);
      }

      //}
      bool onFloor = m_character->hasCollidedWithEnvironment(bvmask_HandLeft | bvmask_HandRight) /*||
                                                                   getLeftLeg()->getThigh()->collidedWithNotOwnCharacter() ||
                                                                   getRightLeg()->getThigh()->collidedWithNotOwnCharacter()*/;
      onFloor = onFloor || getRightLeg()->getThigh()->collidedWithNotOwnCharacter() || getLeftLeg()->getThigh()->collidedWithNotOwnCharacter();
      if (onFloor)
        m_stumbleState = sbleS_onFloor;
      if (catchFallTask->m_handsAndKnees)
      {
        rage::Matrix34 buttockstm;
        rage::Matrix34 foottm;
        float footLength = 0.1f;
        hipYaw = 0.f;
        hipPitch = -0.0f;
        hipRoll = 0.f;

        getSpine()->getPelvisPart()->getMatrix(buttockstm);
        rage::Vector3 pelvisBack(buttockstm.c);
        rage::Vector3 pelvisPos = getSpine()->getPelvisPart()->getPosition();
        getLeftLeg()->getFoot()->getMatrix(foottm);
        rage::Vector3 hipAimPos;
        rage::Vector3 feetPos = getLeftLeg()->getFoot()->getPosition() - footLength*foottm.c;
        getRightLeg()->getFoot()->getMatrix(foottm);
        feetPos += getRightLeg()->getFoot()->getPosition() - footLength*foottm.c;
        feetPos *= 0.5f;
        m_character->levelVector(feetPos, pelvisPos.z);
        hipAimPos = feetPos;
        hipAimPos += pelvisPos - m_character->m_COM;

        if (onFloor1) 
          drop = 0.f;
        else
        {
          drop = dropVal;
          hipPitch += -hipPitchVal*(pelvisPos-feetPos).Mag();
        }
        if (!onFloor1)
          hipPitch += -hipPitchVal1*(pelvisPos-feetPos).Mag();
        if (m_parameters.different)
        {
          rage::Matrix34 tmCom;
          tmCom.Set(m_character->m_COMTM);
          //make angvel from COM or Spine2
          float angComp = rage::Clamp(m_character->m_COMrotvel.Dot(tmCom.a), -3.f, 3.f);//side
          float linComp = rage::Clamp(m_character->m_COMvel.Dot(tmCom.c), -3.f, 3.f);//back
          rage::Vector3 hip2head = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
          hip2head.Normalize();
          float posComp = hip2head.Dot(tmCom.c);//back

          hipPitch = -2.f*rage::Clamp(-0.7f*posComp - 0.25f*linComp - 0.25f*angComp, -0.7f, 0.5f);
        }


        if (!m_allFours)
          hipPitch = 0.f;

        pelvisPos = feetPos; 

        rage::Matrix34 tempMat, newMat;
        NmRsCharacter::rayProbeIndex armProbeIndex = NmRsCharacter::pi_catchFallLeft;
        rage::Vector3 levCom = m_character->m_COM;
        rage::Vector3 tiltedUp = m_character->m_gUp;
        rage::Vector3 floorPos = m_character->m_COM;
        floorPos.z -= 1.0f;
        if (m_character->m_probeHit[armProbeIndex])
          floorPos = m_character->m_probeHitPos[armProbeIndex];
        static float floorOffset = 0.00f;
        floorPos.z -= floorOffset;
        tempMat.a.Set(tiltedUp);//m_character->m_gUp);//up
        m_character->levelVector(levCom,floorPos.z);
        tiltedUp = 2.f*getSpine()->getPelvisPart()->getPosition() - pelvisPos - levCom;
        tiltedUp = pelvisPos - levCom;
        tiltedUp = 2.f*pelvisPos -getSpine()->getPelvisPart()->getPosition() - levCom;
        tiltedUp.Normalize();

        tempMat.b.Cross(pelvisBack, tempMat.a);//side (pelvisback x up)
        tempMat.b.Normalize();//back
        tempMat.c.Cross(tempMat.a, tempMat.b);
        newMat.FromEulersXYZ(rage::Vector3(hipYaw, hipPitch, hipRoll));
        newMat.d.Zero();
        newMat.Dot3x3(tempMat);
        if (m_parameters.dampPelvis)
        {
          rage::Matrix34 velRot;
          rage::Quaternion quatRot;
          rage::Vector3 angVel(getSpine()->getPelvisPart()->getAngularVelocity());
          angVel.Scale(-0.1f);
          quatRot.FromRotation(angVel);
          velRot.FromQuaternion(quatRot);
          newMat.Dot3x3(velRot);
        }
        pelvisPos = getSpine()->getPelvisPart()->getPosition();
        feetPos = getLeftLeg()->getFoot()->getPosition() + getRightLeg()->getFoot()->getPosition();
        feetPos *= 0.5f;
        m_character->levelVector(feetPos,pelvisPos.z);
        pelvisPos = feetPos;
        newMat.d.Set(pelvisPos);

        newMat.FastInverse();
        newMat.Dot(buttockstm);

        rage::Vector3 leftHip2rightHip = getLeftLeg()->getHip()->getJointPosition();
        leftHip2rightHip -= getRightLeg()->getHip()->getJointPosition();
        m_character->levelVector(leftHip2rightHip);
        leftHip2rightHip.Normalize();
        rage::Vector3 targetIn;
        rage::Vector3 target;
        rage::Vector3 hipVel = getSpine()->getPelvisPart()->getLinearVelocity();

        {
          //NmRsCharacter::rayProbeIndex armProbeIndex = NmRsCharacter::pi_balLeft;
          targetIn = getLeftLeg()->getKnee()->getJointPosition() - 0.4f*m_character->m_COMTM.b;
          //targetIn = hipAimPos;
          targetIn += leftHip2rightHip*0.05f;
          m_character->levelVector(targetIn,floorPos.z+drop);
#if ART_ENABLE_BSPY
          m_character->bspyDrawPoint(targetIn, 0.1f, rage::Vector3(0,0.5f,0));
#endif
          //target = targetIn;
          if (getLeftLeg()->getThigh()->collidedWithNotOwnCharacter() 
            || (!getLeftLeg()->getFoot()->collidedWithNotOwnCharacter() && m_parameters.pitchInContact)  
            || onFloor)
            target = targetIn;
          else
          {
            //targetIn -= 4.f*(pelvisPos-getSpine()->getPelvisPart()->getPosition()); 
            newMat.Transform(targetIn, target);
          }
          //m_character->levelVector(target,0.19799913f+drop);

          NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
          NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
          ikInputData->setTarget(target);
          ikInputData->setTwist(0.0f);
          ikInputData->setDragReduction(0.0f);
          ikInputData->setVelocity(hipVel);
          getLeftLeg()->postInput(ikInput);

#if ART_ENABLE_BSPY
          m_character->bspyDrawPoint(target, 0.1f, rage::Vector3(0,1,0));
#endif
        }

        {
          //NmRsCharacter::rayProbeIndex armProbeIndex = NmRsCharacter::pi_balRight;
          targetIn = getRightLeg()->getKnee()->getJointPosition() - 0.4f*m_character->m_COMTM.b;
          //targetIn = hipAimPos;
          targetIn -= leftHip2rightHip*0.05f;
          m_character->levelVector(targetIn,floorPos.z+drop);
#if ART_ENABLE_BSPY
          m_character->bspyDrawPoint(targetIn, 0.1f, rage::Vector3(0.5f,0,0));
#endif
          //target = targetIn;
          if (getRightLeg()->getThigh()->collidedWithNotOwnCharacter() 
            || (!getRightLeg()->getFoot()->collidedWithNotOwnCharacter() && m_parameters.pitchInContact)
            || onFloor)
            target = targetIn;
          else
          {
            //targetIn -= 4.f*(pelvisPos-getSpine()->getPelvisPart()->getPosition()); 
            newMat.Transform(targetIn, target);
          }
          //m_character->levelVector(target,0.19799913f+drop);

          NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
          NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
          ikInputData->setTarget(target);
          ikInputData->setTwist(0.0f);
          ikInputData->setDragReduction(0.0f);
          ikInputData->setVelocity(hipVel);
          getRightLeg()->postInput(ikInput);

#if ART_ENABLE_BSPY
          m_character->bspyDrawPoint(target, 0.1f, rage::Vector3(1,0,0));
#endif
        }
        bool bendForwardsNow = m_allFours && getRightLeg()->getThigh()->collidedWithNotOwnCharacter() && getLeftLeg()->getThigh()->collidedWithNotOwnCharacter();
        static bool allowBendForwards = true;
        bendForwardsNow = bendForwardsNow && allowBendForwards;

        if (!bendForwardsNow)
        {
          if (getRightLeg()->getThigh()->collidedWithNotOwnCharacter() || onFloor)
            getRightLegInputData()->getHip()->setDesiredLean1(-0.f);
          if (getLeftLeg()->getThigh()->collidedWithNotOwnCharacter() || onFloor)
            getLeftLegInputData()->getHip()->setDesiredLean1(-0.f);
        }
        if (!m_allFours)
        {
          getLeftLegInputData()->getKnee()->setDesiredAngle(nmrsGetActualAngle(getLeftLeg()->getKnee()));
          getRightLegInputData()->getKnee()->setDesiredAngle(nmrsGetActualAngle(getRightLeg()->getKnee()));
          getSpineInputData()->applySpineLean(-0.0f,0.f);

          if (m_leftArmState.m_liftArmTime<=0.f)
            getLeftArmInputData()->getWrist()->setDesiredLean2(0.1f);
          if (m_rightArmState.m_liftArmTime<=0.f) 
            getRightArmInputData()->getWrist()->setDesiredLean2(0.1f);
        }

        tiltedUp = m_character->m_gUp;
        tiltedUp-= 2.5f*(hipAimPos-getSpine()->getPelvisPart()->getPosition())-2.5f*getSpine()->getPelvisPart()->getLinearVelocity();
        tiltedUp.Normalize();
        rage::Matrix34 kneetm;
        getLeftLeg()->getKnee()->getMatrix1(kneetm);
        float ankleLean;
        rage::Vector3 kneePos = getLeftLeg()->getKnee()->getJointPosition();
        rage::Vector3 footPos = getLeftLeg()->getFoot()->getPosition();
        ankleLean = getAnkleAngle(kneePos, footPos, kneetm.c, tiltedUp);

        getLeftLegInputData()->getAnkle()->setDesiredLean1(ankleLean+0.1f);

        kneePos = getRightLeg()->getKnee()->getJointPosition();
        footPos = getRightLeg()->getFoot()->getPosition();
        getRightLeg()->getKnee()->getMatrix1(kneetm);
        ankleLean = getAnkleAngle(kneePos, footPos, kneetm.c, tiltedUp);

        getRightLegInputData()->getAnkle()->setDesiredLean1(ankleLean+0.1f);

        if (m_parameters.fallMask != bvmask_Full) 
        {
          float muscleStiff = 2.f;

          float stiffness = m_parameters.m_legsStiffness;
          muscleStiff = 1.f;
          if (onFloor)
            stiffness = 7.0f;
          m_body->setStiffness(stiffness, 1.f, bvmask_LegLeft | bvmask_LegRight, &muscleStiff);
          muscleStiff = m_parameters.feetMS;
          getLeftLegInputData()->getAnkle()->setStiffness(m_parameters.m_legsStiffness, 1.0f, &muscleStiff);
          getRightLegInputData()->getAnkle()->setStiffness(m_parameters.m_legsStiffness, 1.0f, &muscleStiff);

          if (bendForwardsNow)
            getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_torsoStiffness, 0.5f, bvmask_LowSpine);
        }

        bool start2Knees = m_character->hasCollidedWithEnvironment(bvmask_HandLeft | bvmask_HandRight)
          &&  getRightLeg()->getThigh()->collidedWithNotOwnCharacter() && getLeftLeg()->getThigh()->collidedWithNotOwnCharacter();
        static float landTime = 0.4f;
        static float toKneesTime = 0.7f;
        if (start2Knees && m_2Kneestimer<0.f)
          m_2Kneestimer = toKneesTime + landTime;
        if (m_2Kneestimer >=0.f)
        {
          m_2Kneestimer -= timeStep;
          if (m_2Kneestimer <= toKneesTime)
            m_allFours = false;
        }
        else
          m_allFours = true;

        if ((getSpine()->getHeadPart()->getPosition().z < floorPos.z + 0.4f)
          || getSpine()->getPelvisPart()->collidedWithEnvironment())
        {
          //mmmmADDEDif (catchFallTask->m_handsAndKnees)
          //mmmmADDED{
          //mmmmADDED	m_leftArmState.m_strength = 1.f;
          //mmmmADDED	m_rightArmState.m_strength = 1.f;
          //mmmmADDED}
          catchFallTask->m_handsAndKnees = false;//mmmmtodo mmmmnote messes up setFallingReaction setup
          catchFallTask->m_callRDS = false;//mmmmtodo mmmmnote messes up setFallingReaction setup
          catchFallTask->m_comVelRDSThresh = 2.0f;//mmmmtodo mmmmnote messes up setFallingReaction setup
          catchFallTask->m_armReduceSpeed = 2.5f;//mmmmtodo mmmmnote messes up setFallingReaction setup
        }
        //mmmmADDEDif (onFloor && m_allFours)
        //mmmmADDED{
        //mmmmADDED	m_injuryVal += m_behaviourTime*m_parameters.injuryRate;
        //mmmmADDED	m_character->applyInjuryMask("fb", m_injuryVal);
        //mmmmADDED}

      }
      if (m_forwardsAmount < -0.45f && onFloor)
      {
        catchFallTask->m_handsAndKnees = false;//mmmmtodo mmmmnote messes up setFallingReaction setup
        catchFallTask->m_callRDS = false;//mmmmtodo mmmmnote messes up setFallingReaction setup
        catchFallTask->m_comVelRDSThresh = 2.0f;//mmmmtodo mmmmnote messes up setFallingReaction setup
        catchFallTask->m_armReduceSpeed = 2.5f;//mmmmtodo mmmmnote messes up setFallingReaction setup
        m_parameters.fallMask = bvmask_Full;
      }
    }
    m_behaviourTime += timeStep;
    return eCBUTaskComplete;
  }

  float NmRsCBUStumble::getAnkleAngle(
    rage::Vector3 &kneePos, 
    rage::Vector3 &footPos, 
    rage::Vector3 &sideways, 
    const rage::Vector3 &upVector)
  {
    rage::Vector3 toKnee, kneeCross;

    toKnee.Subtract(kneePos, footPos);
    toKnee.Normalize();
    kneeCross.Cross(toKnee, upVector);

    float sinAngle = kneeCross.Dot(sideways);
    return rage::AsinfSafe(sinAngle);
  }


  void NmRsCBUStumble::armsBrace()    
  {
    const float scaleRightness = 1.f;
    const float scaleBackwardsRightness = 0.5f;
    const float scaleContraRightness = 0.8f;
    const float backwardsScale = 0.5f;//1 has no problems 

    //Set muscle stiffnesses here for clarity
    float armStiffness = 11.f;
    float armMuscleStiffness = 1.f;//mmmmADDED
    const float armDamping = 0.7f;
    //if(m_defaultArmMotion.leftBrace)
    {
      getLeftArm()->setBodyStiffness(getLeftArmInput(), armStiffness, armDamping, bvmask_Full, &armMuscleStiffness);
      getLeftArmInputData()->getElbow()->setStiffness(armStiffness*0.75f, 0.75f*armDamping, &armMuscleStiffness);
      getLeftArmInputData()->getWrist()->setStiffness(armStiffness - 1.0f, 1.75f);
    }
    //if(m_defaultArmMotion.rightBrace)
    {
      getRightArm()->setBodyStiffness(getRightArmInput(), armStiffness, armDamping, bvmask_Full, &armMuscleStiffness);
      getRightArmInputData()->getElbow()->setStiffness(armStiffness*0.75f, 0.75f*armDamping, &armMuscleStiffness);
      getRightArmInputData()->getWrist()->setStiffness(armStiffness - 1.0f, 1.75f);
    }


    rage::Vector3 braceTarget;
    //static float velMult = 0.6f;//was 0.3f
    static float maxArmLength = 0.61f;//0.55 quite bent, 0.6 Just bent, 0.7 straight
    rage::Matrix34 tmCom;
    getSpine()->getSpine1Part()->getBoundMatrix(&tmCom); 
    rage::Vector3 vec;
    float armTwist;
    rage::Vector3 targetVel;
    rage::Vector3 rightDir1 = -tmCom.b;
    rage::Vector3 forwardDir1 = -tmCom.c;
    m_character->levelVector(rightDir1);
    m_character->levelVector(forwardDir1);

    rage::Vector3 reachVel = getSpine()->getSpine3Part()->getLinearVelocity() - m_character->getFloorVelocity();
    m_character->levelVector(reachVel);
    getLeftLeg()->getThigh()->getBoundMatrix(&tmCom);
    rage::Vector3 rightDir = tmCom.a;
    rage::Vector3 forwardDir = -tmCom.c;
    m_character->levelVector(rightDir);
    m_character->levelVector(forwardDir);

    float rightness = scaleRightness*reachVel.Dot(rightDir);
    float forwardness = reachVel.Dot(forwardDir);
    bool goingBackwards = false;
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", rightness);
    bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", forwardness);
#endif // ART_ENABLE_BSPY
    if (forwardness < 0.f)
    {
      //forwardness = 0.f;//both these work 0 gives more arms out but more arms back. Leaving it as a negative number makes a backward moving bending forward look odd
      forwardness = -backwardsScale*forwardness;//0..1  actually 2 is ok.
      goingBackwards = true;
    }
    float feetHeight = 0.5f*(getLeftLeg()->getFoot()->getPosition().Dot(m_character->m_gUp) + getRightLeg()->getFoot()->getPosition().Dot(m_character->m_gUp));
    float leftHandRelativeHeight = rage::Max(0.f,getRightArm()->getHand()->getPosition().Dot(m_character->m_gUp) - feetHeight);
    float rightHandRelativeHeight = rage::Max(0.f,getRightArm()->getHand()->getPosition().Dot(m_character->m_gUp) - feetHeight);


    //LEFT ARM
    {
      armTwist = -0.5f;//arms in front or to side, 0 = arms a bit wider(1.0f for behind)
      braceTarget = -m_character->m_gUp;
      if (rightness > 0.f)
      {
        rightness *= scaleContraRightness*rage::Sinf(3.f*m_behaviourTime);
        maxArmLength = m_character->getRandom().GetRanged(0.51f,0.62f);
      }
      if (goingBackwards)
      {
        armTwist = m_character->getRandom().GetRanged(-0.5f,0.5f);
        rightness *= scaleBackwardsRightness;
        braceTarget += rightness*rightDir;//didn't make alot of difference
        braceTarget += forwardness*forwardDir;
      }
      else
      {
        braceTarget += rightness*rightDir1;
        braceTarget += forwardness*forwardDir1;
      }


      // clamp left arm not to reach too far
      float mag = braceTarget.Mag();
      braceTarget.Normalize();
      braceTarget *= rage::Min(mag , maxArmLength);
      braceTarget += getLeftArm()->getShoulder()->getJointPosition();;
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", rightness);
      bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", forwardness);
      bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", braceTarget);
      bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", goingBackwards);
      bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", armTwist);
#endif // ART_ENABLE_BSPY

      //straightness = 0.8f;//m_character->getRandom().GetRanged(0.0f, 0.4f);
      targetVel = getLeftArm()->getClaviclePart()->getLinearVelocity();
      NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(0, 1.0f DEBUG_LIMBS_PARAMETER("armsBrace"));
      NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
      ikInputData->setTarget(braceTarget);
      ikInputData->setTwist(armTwist);
      ikInputData->setMaxReachDistance(maxArmLength);
      ikInputData->setVelocity(targetVel);
      ikInputData->setMatchClavicle(kMatchClavicleBetter);
      getLeftArm()->postInput(ikInput);

      // by default, characters tip hands up in preparation for connecting with object
      float desiredLean2 = 0.f;
      if (leftHandRelativeHeight < 0.7f)
      {
        desiredLean2 -= 2.5f*(0.7f-leftHandRelativeHeight);
      }
      getLeftArmInputData()->getWrist()->setDesiredLean2(desiredLean2);
    }

    //RIGHT ARM
    getRightLeg()->getThigh()->getBoundMatrix(&tmCom);
    rightDir = tmCom.a;
    forwardDir = -tmCom.c;
    m_character->levelVector(rightDir);
    m_character->levelVector(forwardDir);

    rightness = scaleRightness*reachVel.Dot(rightDir);
    forwardness = reachVel.Dot(forwardDir);
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", rightness);
    bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", forwardness);
#endif // ART_ENABLE_BSPY
    if (forwardness < 0.f)
    {
      //forwardness = 0.f;//both these work 0 gives more arms out but more arms back. Leaving it as a negative number makes a backward moving bending forward look odd
      forwardness = -backwardsScale*forwardness;//0..1  actually 2 is ok.
      goingBackwards = true;
    }

    {
      armTwist = -0.5f;//arms in front or to side, 0 = arms a bit wider(1.0f for behind)
      braceTarget = -m_character->m_gUp;
      if (rightness < 0.f)
      {
        rightness *= scaleContraRightness*rage::Sinf(3.f*m_behaviourTime);
        maxArmLength = m_character->getRandom().GetRanged(0.51f,0.62f);
      }
      if (goingBackwards)
      {
        //armTwist = (1.f+rage::Cosf(m_hitTime))*0.35f;// m_character->getRandom().GetRanged(0.f,0.7f);
        armTwist = m_character->getRandom().GetRanged(-0.0f,0.5f);
        rightness *= scaleBackwardsRightness;
        braceTarget += rightness*rightDir;
        braceTarget += forwardness*forwardDir;
      }
      else
      {
        braceTarget += rightness*rightDir1;
        braceTarget += forwardness*forwardDir1;
      }

      // clamp left arm not to reach too far
      float mag = braceTarget.Mag();
      braceTarget.Normalize();
      braceTarget *= rage::Min(mag , maxArmLength);
      braceTarget += getRightArm()->getShoulder()->getJointPosition();;
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", rightness);
      bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", forwardness);
      bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", braceTarget);
      bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", goingBackwards);
      bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", armTwist);
#endif // ART_ENABLE_BSPY

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", braceTarget);
#endif // ART_ENABLE_BSPY

      //straightness = 0.8f;//m_character->getRandom().GetRanged(0.0f, 0.4f);
      targetVel = getRightArm()->getClaviclePart()->getLinearVelocity();
      NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(0, 1.0f DEBUG_LIMBS_PARAMETER("armsBrace"));
      NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
      ikInputData->setTarget(braceTarget);
      ikInputData->setTwist(armTwist);
      ikInputData->setMaxReachDistance(maxArmLength);
      ikInputData->setVelocity(targetVel);
      ikInputData->setMatchClavicle(kMatchClavicleBetter);
      getRightArm()->postInput(ikInput);

      float desiredLean2 = 0.f;
      if (rightHandRelativeHeight < 0.7f)
      {
        desiredLean2 -= 2.5f*(0.7f-rightHandRelativeHeight);
      }
      getRightArmInputData()->getWrist()->setDesiredLean2(desiredLean2);
    }
  }

  void NmRsCBUStumble::ArmState::armIK(NmRsIKInputWrapper* ikInputData, const rage::Vector3 &target, float armTwist, float dragReduction, const rage::Vector3 *vel)
  {
    float straightness = 0.4f;
    float maxSpeed = 200.f; // ie out of range

    //If teeter says so then restrict the hands to not reach over the edge 
    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_parent->m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);
    NmRsCBUTeeter* teeterTask = (NmRsCBUTeeter*)m_parent->m_cbuParent->m_tasks[bvid_teeter];
    Assert(teeterTask);
    rage::Vector3 targetCopy(target);
    if (teeterTask->isActive() && teeterTask->restrictCatchFallArms())
    {
      m_character->levelVector(targetCopy,m_character->m_gUp.Dot(balColReactTask->m_pos1stContact));
      float exclusionZone = 0.05f;
      if (balColReactTask->m_normal1stContact.Dot(targetCopy-balColReactTask->m_pos1stContact-balColReactTask->m_normal1stContact*exclusionZone)*balColReactTask->m_sideOfPlane > 0.f)//increase offset
      {
        targetCopy -= balColReactTask->m_normal1stContact.Dot(targetCopy - balColReactTask->m_pos1stContact-balColReactTask->m_normal1stContact*exclusionZone)* balColReactTask->m_normal1stContact;
        m_character->levelVector(targetCopy,balColReactTask->m_pos1stContact.z);
      }
      else
        targetCopy.Set(target);
    }

    ikInputData->setTarget(targetCopy);
    ikInputData->setTwist(armTwist);
    ikInputData->setDragReduction(dragReduction);
    if(vel)
    {
      ikInputData->setVelocity(*vel);
    }
    ikInputData->setTwistIsFixed(true);
    ikInputData->setAdvancedStaightness(straightness);
    ikInputData->setAdvancedMaxSpeed(maxSpeed);
    ikInputData->setUseAdvancedIk(true);
  }

  void NmRsCBUStumble::ArmState::wristIK(NmRsIKInputWrapper *input, const rage::Vector3 &target, const rage::Vector3 &normal)
  {
    bool useActualAngles = false;
    float twistLimit = 2.4f;
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_parent->m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    if (catchFallTask->m_handsAndKnees)//mmmmtodo recheck this makes a difference
    {
      useActualAngles = true;
      twistLimit = 1.f;
    }

    input->setWristTarget(target);
    input->setWristNormal(normal);
    input->setWristUseActualAngles(useActualAngles);
    input->setWristTwistLimit(twistLimit);
  }

  void NmRsCBUStumble::ArmState::enter(NmRsHumanArm *armSetup, bool leftArm, BehaviourMask armMask)
  {
    m_armSetup = armSetup;
    m_isLeftArm = leftArm;
    m_strength = 1.f;
    m_armMask = armMask;
    m_onBackRandomL1 = m_character->getRandom().GetRanged(-0.4f, 0.f);
    m_onBackRandomL2 = m_character->getRandom().GetRanged(0.f, 0.5f);
    m_maxElbowAngleRandom = m_character->getRandom().GetRanged(0.7f, 1.3f);
  }

  bool NmRsCBUStumble::ArmState::tick(float timeStep)
  {
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_parent->m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);

    NmRsLimbInput ikInput = m_parent->createNmRsLimbInput<NmRsIKInputWrapper>(0, 1.0f DEBUG_LIMBS_PARAMETER("ArmState::tick"));
    NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();

    // slightly higher priority so we can overwrite IK if necessary
    NmRsLimbInput armInput = m_parent->createNmRsLimbInput<NmRsArmInputWrapper>(0, 1.0f DEBUG_LIMBS_PARAMETER("ArmState::tick"));
    NmRsArmInputWrapper* armInputData = armInput.getData<NmRsArmInputWrapper>();

    //NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)m_parent->m_cbuParent->m_tasks[bvid_spineTwist];      
    //float twist = spineTwistTask->getTwist();
    //NM_RS_DBG_LOGF_FROMPARENT(L"twist: %.3f", twist);
    float armTwist = m_parent->m_parameters.armTwist;//-0.5f;// - m_parent->m_forwardsAmount; // rotating elbows in on forwards and out on backwards
    NM_RS_DBG_LOGF_FROMPARENT(L">>>armTwist: %.3f", armTwist);
    rage::Vector3 bodyUpModified = m_character->m_COMTM.b;//bodyUp
    rage::Vector3 side;
    side.Cross(bodyUpModified, m_character->m_gUp);
    if (side.Mag2() > 1e-10f)
      side.Normalize();
    rage::Vector3 leanDir;
    leanDir.Cross(bodyUpModified, side);

    //rage::Vector3 zAxis = m_character->m_COMTM.c;//bodyBack
    if (m_parent->m_forwardsAmount > 0.f)
    {
      bodyUpModified += leanDir; // -= zAxis;
      bodyUpModified.Normalize();
      bodyUpModified.Scale( 0.4f + 0.6f * rage::Clamp(m_parent->m_upwardsness, 0.f, 1.f) );
    }

    rage::Vector3 shoulderPos = m_armSetup->getShoulder()->getJointPosition();
    //shoulderPos += bodyUpModified;// * rage::Clamp(m_parent->m_forwardsAmount, m_parent->m_parameters.m_backwardsMinArmOffset, m_parent->m_parameters.m_forwardMaxArmOffset);
    //NM_RS_CBU_DRAWPOINT(shoulderPos, 0.1f, rage::Vector3(1,1,1));
    rage::Matrix34 tmCom;
    m_armSetup->getClaviclePart()->getBoundMatrix(&tmCom);
    //put arms out more for better hands and knees stability unless on back or sitting up
    rage::Vector3 armOut = -0.1f*tmCom.b;
    if (!catchFallTask->m_handsAndKnees || (m_parent->m_forwardsAmount<-0.3f && m_character->hasCollidedWithWorld(m_armMask)))
      armOut.Zero();
    //NM_RS_CBU_DRAWVECTORCOL(m_armSetup->getClaviclePart()->getPosition(), -tmCom.b, rage::Vector3(1,1,1)); 

    rage::Vector3 probeEnd = shoulderPos + m_parent->m_fallDirection*m_parent->m_probeLength + armOut;
    rage::Vector3 probeEndHit;
    rage::Vector3 handPos = m_armSetup->getHand()->getPosition();
    rage::Vector3 groundNormal = m_character->getUpVector();
    rage::Vector3 groundNormalHit;
    shoulderPos += 0.3f*(shoulderPos-probeEnd);
    NmRsCharacter::rayProbeIndex armProbeIndex = NmRsCharacter::pi_catchFallRight;
    if (m_isLeftArm)
      armProbeIndex = NmRsCharacter::pi_catchFallLeft;
    bool hasHit = m_character->probeRay(armProbeIndex, shoulderPos, probeEnd, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);

    //m_floorVel.Set(0,0,0);
    m_floorVel = m_character->getFloorVelocity();
    //NM_RS_CBU_DRAWLINE(shoulderPos, probeEnd, rage::Vector3(1.0f,0.0f,0.0f));
    bool allowVelFromProbe = true;
    if (hasHit)
    {
      probeEndHit = m_character->m_probeHitPos[armProbeIndex];
      groundNormalHit = m_character->m_probeHitNormal[armProbeIndex];
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "Stumble", m_character->m_probeHitInst[armProbeIndex]->GetLevelIndex());
#endif
      //NM_RS_CBU_DRAWLINE(shoulderPos, probeEndHit, rage::Vector3(0.0f,1.0f,0.0f));
      //NM_RS_CBU_DRAWPOINT(probeEndHit, 1.1f, rage::Vector3(0.5f, 0.5f, 0.5f));
      if (m_character->m_probeHitInst[armProbeIndex])
      {
        //ignore velocity if hit say a gun //onFloor only uses m_floorVel for the extraSit
        //		  bool allowVelFromProbe = true;
        if (m_character->getDontRegisterProbeVelocityActive())
        {
          rage::Vector3 objectSize = m_character->m_probeHitInstBoundingBoxSize[armProbeIndex];
          float vol = objectSize.x * objectSize.y * objectSize.z; 
          allowVelFromProbe = PHLEVEL->IsFixed(m_character->m_probeHitInstLevelIndex[armProbeIndex]) ||
            ((m_character->m_probeHitInstMass[armProbeIndex] >= m_character->getDontRegisterProbeVelocityMassBelow())
            && (vol >= m_character->getDontRegisterProbeVelocityVolBelow()));
#if ART_ENABLE_BSPY
          if (!allowVelFromProbe)
          {
            bspyScratchpad(m_character->getBSpyID(), "CF DontRegisterProbeVelocity", m_character->m_probeHitInst[armProbeIndex]->GetLevelIndex());
          }
#endif
        }
        if (allowVelFromProbe)
        {
          m_character->getVelocityOnInstance(m_character->m_probeHitInst[armProbeIndex]->GetLevelIndex(),probeEnd,&m_floorVel);
        }
        else
        {
          //ignore this hit - it usually has a strange normal mmmmtodo more explanation 
          hasHit = false;
        }
      }
    }
    if (hasHit)
    {
      probeEnd = probeEndHit;
      groundNormal = groundNormalHit;
      ////If probe only just hit something make the probe longer next time to stop jitter
      //rage::Vector3 probeVec = m_parent->m_fallDirection*m_parent->m_probeLength + armOut;
      //rage::Vector3 probeHitVec = probeEndHit - shoulderPos;
      //if (rage::Abs(probeVec.Mag() - probeHitVec.Mag()) < 0.005)
      //  m_parent->m_probeLength = m_parent->m_reachLength + 0.65f;
      //else
      //  m_parent->m_probeLength = m_parent->m_reachLength + 0.6f;
#if ART_ENABLE_BSPY
      if (allowVelFromProbe)
      {
        m_character->bspyDrawLine(shoulderPos,probeEnd,rage::Vector3(1.0f,0.0f,0.0f));
        m_character->bspyDrawLine(probeEnd,probeEnd+groundNormal,rage::Vector3(1.0f,1.0f,1.0f));
      }
      else
      {
        m_character->bspyDrawLine(shoulderPos,probeEnd,rage::Vector3(0.0f,0.0f,1.0f));
        m_character->bspyDrawLine(probeEnd,probeEnd+groundNormal,rage::Vector3(0.4f,0.40f,0.4f));
      }
#endif
    }
#if ART_ENABLE_BSPY
    else
    {
      if (allowVelFromProbe)
      {
        m_character->bspyDrawLine(shoulderPos,probeEnd,rage::Vector3(0.0f,1.0f,0.0f));
        m_character->bspyDrawLine(probeEnd,probeEnd+groundNormal,rage::Vector3(0.8f,0.80f,0.8f));
      }
      else
      {
        m_character->bspyDrawLine(shoulderPos,probeEnd,rage::Vector3(0.0f,0.0f,1.0f));
        m_character->bspyDrawLine(probeEnd,probeEnd+groundNormal,rage::Vector3(0.8f,0.80f,0.8f));
      }
    }
    rage::Vector3 probeEndReal = probeEnd;
#endif

    if (m_character->hasCollidedWithWorld(m_armMask))
      m_strength = rage::Min(m_parent->m_bodyStrength, m_strength - m_parent->m_parameters.armReduceSpeed*timeStep);
    else
    {
      float maxStrength = rage::Min(1.f, m_parent->m_bodyStrength*3.f);
      m_strength = rage::Min(m_strength + 2.f*timeStep, maxStrength);
    }
    shoulderPos = m_armSetup->getShoulder()->getJointPosition();

    if (catchFallTask->m_handsAndKnees)
    {
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_parent->m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      //Lying on back 
      if (m_parent->m_forwardsAmount<-0.3f)// && m_character->hasCollidedWithWorld(m_armMask))
      {
        //push off with hands
        //probeEnd = handPos-groundNormal*0.3f;
        //lean forward if balancer on
        dynamicBalancerTask->setHipPitch(-0.5f);//+3.5f*(m_parent->m_forwardsAmount+0.3f));
        //below gives more colour to legs, but less stable and perhaps too many bad leg poses
        //if (m_character->hasCollidedWithWorld("ub"))
        //{
        //  //lean forward if balancer on
        //  dynamicBalancerTask->setHipPitch(-0.5f+3.5f*(m_parent->m_forwardsAmount+0.3f));
        //  m_parent->m_parameters.m_effectorMask[0] = 'f';
        //}
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "CFPushOff", true);
#endif
      }
    }
    else
    {
      if (m_character->hasCollidedWithWorld(m_armMask))
        probeEnd = (probeEnd + handPos)*0.5f;

      if (m_parent->m_forwardsAmount<-0.5f && m_character->hasCollidedWithWorld(m_armMask))
        probeEnd = handPos;
    }
#if ART_ENABLE_BSPY
    if (m_isLeftArm)
    {
      bspyScratchpad(m_character->getBSpyID(), "Left shoulder", shoulderPos);
      bspyScratchpad(m_character->getBSpyID(), "Left probe", probeEnd);
    }
    else
    {
      bspyScratchpad(m_character->getBSpyID(), "Right shoulder", shoulderPos);
      bspyScratchpad(m_character->getBSpyID(), "Right probe", probeEnd);
    }
#endif
    rage::Vector3 shoulderPos2 = m_armSetup->getShoulder()->getJointPosition();
    rage::Vector3 target = probeEnd - shoulderPos2;

    float dist = target.Mag();
    NM_RS_DBG_LOGF_FROMPARENT(L"dist %f", dist);

    float pushOff = 0.4f;
    if (catchFallTask->m_handsAndKnees)
    {
      pushOff = 0.6f;//to push up from arms
      //if (m_parent->m_floorTime>0.1f)//to sink then pushoff
      if (!m_parent->m_allFours)
        pushOff = 0.9f;//to spring up from arms
    }

    if (dist > m_parent->m_reachLength)
      target *= m_parent->m_reachLength / dist;
    //else if (dist < pushOff)
    //  //target *= pushOff / (dist + 0.0001f);
    //  target -= m_character->m_gUp*pushOff / (dist + 0.0001f);
    else
    {
      rage::Vector3 target2 = target;
      target2 -= m_character->m_gUp*pushOff;
      float dist2 = target2.Mag();
      NM_RS_DBG_LOGF_FROMPARENT(L"dist2 %f", dist2);
      target = target2;
      if (dist2 > m_parent->m_reachLength)
        target = target2*m_parent->m_reachLength / dist2;
    }
    target += shoulderPos2;

    if (catchFallTask->m_handsAndKnees)
    {
      //hand orientated wrong lift it up - only lift off one hand at time
      //This code simulates taking a hands and knees arm step sometimes - maybe set this off another way to increase movement
      rage::Vector3 normal2BackofHand,fingers2Wrist, palm2thumb;
      rage::Matrix34 handTM;
      m_armSetup->getHand()->getBoundMatrix(&handTM);
      normal2BackofHand = handTM.a;
      if (m_isLeftArm)
        normal2BackofHand *= -1.f;
      fingers2Wrist = handTM.b;
      palm2thumb = handTM.c;
      //NM_RS_CBU_DRAWCOORDINATEFRAME(10.f,handTM);
      //NM_RS_CBU_DRAWVECTORCOL(handTM.d, normal2BackofHand, rage::Vector3(1.f,0.f,0.f));
      //NM_RS_CBU_DRAWVECTORCOL(handTM.d, groundNormal, rage::Vector3(1.f,1.f,1.f));
      bool otherHandIsLifted = (m_isLeftArm ? m_parent->m_rightArmState.m_liftArm : m_parent->m_leftArmState.m_liftArm);
      NM_RS_DBG_LOGF_FROMPARENT(L"BackOfHand %f", normal2BackofHand.Dot(groundNormal));
      NM_RS_DBG_LOGF_FROMPARENT(L"fingers2Wrist %f", fingers2Wrist.Dot(groundNormal));
      if ((m_liftArm && m_armSetup->getHand()->collidedWithEnvironment()) ||
        (((normal2BackofHand.Dot(groundNormal)<-0.2f /*&& fingers2Wrist.Dot(groundNormal)>0.7f*/) ||
        (normal2BackofHand.Dot(groundNormal)<0.0f && rage::Abs(palm2thumb.Dot(groundNormal))>0.7f))
        && !otherHandIsLifted))
      {
        if (m_liftArmTime<=0.f)
        {
          m_liftArm = true;
          m_liftArmTime = 3.f*0.0167f;
        }
        target = probeEnd - shoulderPos;
        target += 0.2f*groundNormal;
        if (dist > m_parent->m_reachLength)
          target *= m_parent->m_reachLength / dist;
        target += shoulderPos;

        //handPos += 0.6f*groundNormal;
        if (m_isLeftArm)
        {
          NM_RS_DBG_LOGF_FROMPARENT(L"LLLLLLLLlifting left arm ");
        }
        else
        {
          NM_RS_DBG_LOGF_FROMPARENT(L"RRRRRRRRlifting right arm ");
        }
#if ART_ENABLE_BSPY
        if (m_isLeftArm)
        {
          bspyScratchpad(m_character->getBSpyID(), "LiftingLeftArm", true);
        }
        else
        {
          bspyScratchpad(m_character->getBSpyID(), "LiftingRightArm", true);
        }
#endif

      }
      else
      {
        if (m_liftArm)
          m_liftArmTime -= timeStep;
        if (m_liftArmTime<0.f)
          m_liftArm = false;
      }
    }

    rage::Vector3 vel = m_armSetup->getClaviclePart()->getLinearVelocity();
    if (m_character->hasCollidedWithWorld(m_armMask))
    {
      if (hasHit)
      {
        //crude way of stopping alot of the hands and knees handstanding and arms lifting whole body when lying on side on top of an arm
        //if (catchFallTask->m_handsAndKnees)
        //{
        //  float mag = (target-handPos).Mag();
        //  if (mag > 0.5f)
        //    target = handPos + 0.25f*(target-handPos);
        //}
        armIK(ikInputData, target, armTwist, 0);

        if (catchFallTask->m_handsAndKnees)
        {
          if (m_parent->m_floorTime <= 0.f)
          {
            m_parent->m_floorTime = 0.00001f;
            m_parent->m_restart = 0.1f;
          }
          if (m_parent->m_restart<1.f)
            m_parent->m_restart = 0.1f;
        }
      }
      else
        armIK(ikInputData, handPos, armTwist, 0);
    }
    else 
    {
      if (catchFallTask->m_handsAndKnees)
      {
        if (m_parent->m_restart<1.f)
          m_parent->m_restart -= timeStep;
        if (m_parent->m_restart<0.f)
          m_parent->m_floorTime = -1.f;
      }
      armIK(ikInputData, target, armTwist, 0.5f, &vel);
    }

    // limbs: will not work exactly as before. this will overwrite, but the source angles
    // will be the result of the entire previous tick rather than the immediate output of ik.
    if (m_parent->m_forwardsAmount < -0.3f)
      armInputData->getElbow()->setDesiredAngle(rage::Min(m_armSetup->getElbow()->getDesiredAngle(), m_maxElbowAngleRandom));
    armInputData->getClavicle()->setDesiredTwist(0.f);

    if (hasHit && catchFallTask->m_handsAndKnees && m_armSetup->getShoulder()->getDesiredTwist() > 0.6f) 
      armInputData->getShoulder()->setDesiredTwist(0.5f*(m_armSetup->getShoulder()->getDesiredTwist()+ 0.6f));

    ikInputData->setMatchClavicle(kMatchClavicle);

    if (m_parent->m_forwardsAmount>0.f)
      armInputData->getClavicle()->setDesiredLean1(0.7f);//forward
    else
      armInputData->getClavicle()->setDesiredLean1(-0.4f);//forward
    if (hasHit)
    {
      ikInputData->setWristTarget(target);
      ikInputData->setWristNormal(groundNormal);
    }
    else
    {
      if (catchFallTask->m_handsAndKnees)
      {
        //want better solution than this ie below is better but want to orientate earlier
        ikInputData->setWristTarget(target);
        ikInputData->setWristNormal(groundNormal);
      }
      else
      {
        armInputData->getWrist()->setDesiredLean1(0.3f);
        armInputData->getWrist()->setDesiredLean2(-0.5f);
        armInputData->getWrist()->setDesiredTwist(1.f);
      }
    }

    m_armSetup->postInput(ikInput);
    m_armSetup->postInput(armInput);

    return hasHit;
  }

#if ART_ENABLE_BSPY
  void NmRsCBUStumble::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);
    static const char* stumbleStateStrings[] = 
    {
      "sbleS_Normal",           //0
      "sbleS_ToGrab",           //1
      "sbleS_Falling",          //2
      "sbleS_onFloor",          //3
      "sbleS_onKnees",          //4 
      "sbleS_onAllFours",       //5 
      "sbleS_Collapse",         //6
      "sbleS_CollapseBackwards",//7
      "sbleS_End",              //8
    };                           
    bspyTaskVar_StringEnum(m_stumbleState, stumbleStateStrings, false);  
    static const char* stumbleTransitionStrings[] = 
    {
      "sbleT_fall2Knees",           //0
      "sbleT_fall2AllFours",           //1
      "sbleT_knees2AllFours",          //2
      "sbleT_allFours2Knees",          //3
      "sbleT_Knees2Collapse",          //4 
      "sbleT_allFours2Collapse",       //5 
      "sbleT_fallBackWards",         //6
    };                           
    bspyTaskVar_StringEnum(m_stumbleTransition, stumbleTransitionStrings, false);  


    bspyTaskVar(m_parameters.m_legsStiffness, true);
    bspyTaskVar(m_parameters.m_torsoStiffness, true);
    bspyTaskVar(m_parameters.m_armsStiffness, true);
    bspyTaskVar(m_parameters.armReduceSpeed, true);
    bspyTaskVar_Bitfield32(m_parameters.fallMask, true);

    bspyTaskVar(m_parameters.m_forwardMaxArmOffset, true);
    bspyTaskVar(m_parameters.m_backwardsMinArmOffset, true);
    bspyTaskVar(m_parameters.m_zAxisSpinReduction, true);
    bspyTaskVar(m_parameters.m_useHeadLook, true);

    bspyTaskVar(m_parameters.wristMS, true);
    bspyTaskVar(m_parameters.feetMS, true);
    bspyTaskVar(m_parameters.armTwist, true);
    bspyTaskVar(m_parameters.staggerTime, true);
    bspyTaskVar(m_parameters.dropVal, true);
    bspyTaskVar(m_parameters.twistSpine, true);
    bspyTaskVar(m_parameters.dampPelvis, true);
    bspyTaskVar(m_parameters.pitchInContact, true);
    bspyTaskVar(m_parameters.different, true);
    bspyTaskVar(m_parameters.leanRate, true);
    bspyTaskVar(m_parameters.maxLeanBack, true);
    bspyTaskVar(m_parameters.maxLeanForward, true);
    bspyTaskVar(m_parameters.grabRadius2, true);
    bspyTaskVar(m_parameters.leanTowards, true);
    bspyTaskVar(m_parameters.useArmsBrace, true);


    bspyTaskVar(m_reachLength, false);
    bspyTaskVar(m_kneeBendL, false);
    bspyTaskVar(m_kneeBendR, false);
    bspyTaskVar(m_randomSpineL2, false);
    bspyTaskVar(m_floorTime, false);
    bspyTaskVar(m_restart, false);
    bspyTaskVar(m_OnFloorTimer, false);
    bspyTaskVar(m_2Kneestimer, false);
    bspyTaskVar(m_hipPitchAim, false);

    bspyTaskVar(m_fallDirection, false);

    bspyTaskVar(m_probeLength, false);
    bspyTaskVar(m_forwardsAmount, false);
    bspyTaskVar(m_bodyStrength, false);
    bspyTaskVar(m_predictionTime, false);
    bspyTaskVar(m_upwardsness, false);

    bspyTaskVar(m_headAvoidActive, false);
    bspyTaskVar(m_onGround, false);
    bspyTaskVar(m_leftArmState.m_liftArm, false);
    bspyTaskVar(m_rightArmState.m_liftArm, false);

    bspyTaskVar(m_leftArmState.m_strength, false);
    bspyTaskVar(m_leftArmState.m_onBackRandomL1, false);
    bspyTaskVar(m_leftArmState.m_onBackRandomL2, false);
    bspyTaskVar(m_leftArmState.m_maxElbowAngleRandom, false);
    bspyTaskVar(m_leftArmState.m_floorVel, false);
    bspyTaskVar(m_leftArmState.m_isLeftArm, false);
    bspyTaskVar_Bitfield32(m_leftArmState.m_armMask, false);

    bspyTaskVar(m_rightArmState.m_strength, false);
    bspyTaskVar(m_rightArmState.m_onBackRandomL1, false);
    bspyTaskVar(m_rightArmState.m_onBackRandomL2, false);
    bspyTaskVar(m_rightArmState.m_maxElbowAngleRandom, false);
    bspyTaskVar(m_rightArmState.m_floorVel, false);
    bspyTaskVar(m_rightArmState.m_isLeftArm, false);
    bspyTaskVar_Bitfield32(m_rightArmState.m_armMask, false);

    //Hands and Knees specific
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    bspyTaskVar(catchFallTask->m_handsAndKnees, true);

  }
#endif // ART_ENABLE_BSPY
}

#endif // #if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
