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

#define GET_TASK(taskName, TaskName, varName)\
  NmRsCBU##TaskName* varName = (NmRsCBU##TaskName*)m_cbuParent->m_tasks[bvid_##taskName];\
  Assert(varName);

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

    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    if (headLookTask->isActive())
      headLookTask->deactivate();

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


  /*
   *
   */
  CBUTaskReturn NmRsCBUTeeter::onTick(float timeStep)
  { 
    NM_RS_DBG_LOGF(L"- Teeter During")

    m_edgeLeft = m_parameters.edgeLeft;
    m_edge = m_parameters.edgeRight - m_edgeLeft;
    m_edgeNormal.Cross(m_edge, m_character->m_gUp);
    m_edgeNormal.Normalize();

    //
    // How far are we from the edge?
    m_levelCom = m_character->m_COM;
    m_character->levelVector(m_levelCom);
    rage::Vector3 edge2Com = m_levelCom - m_edgeLeft;
    m_com2Edge = edge2Com.Dot(m_edgeNormal);

    //
    // How soon are we likely to go over the edge?
    rage::Vector3 levelComVel = m_character->m_COMvel;
    m_character->levelVector(levelComVel);
    m_vel2Edge = -levelComVel.Dot(m_edgeNormal);
    m_time2Edge = m_com2Edge / m_vel2Edge;
    if(m_com2Edge < 0.0f) m_time2Edge = 0.0f;
    else if(m_time2Edge < 0) m_time2Edge = 10.0f;

    // Where are our feet?
    rage::Vector3 edge2Foot;
    edge2Foot = getLeftLeg()->getFoot()->getPosition() - m_edgeLeft;
    m_character->levelVector(edge2Foot);
    m_edge2LeftFoot = edge2Foot.Dot(m_edgeNormal);
    edge2Foot = getRightLeg()->getFoot()->getPosition() - m_edgeLeft;
    m_character->levelVector(edge2Foot);
    m_edge2RightFoot = edge2Foot.Dot(m_edgeNormal);


    GET_TASK(dynamicBalancer, DynamicBalancer, dynamicBalancerTask) 
    m_characterIsFalling = (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK) || (m_com2Edge < -0.3f);
    if (m_characterIsFalling)
    {  
      // Allow other behaviours to handle the fall if not over the edge or BCR has other plans
      GET_TASK(balancerCollisionsReaction, BalancerCollisionsReaction, balColReactTask) 
      if (((balColReactTask->isActive() &&
         (balColReactTask->m_balancerState == bal_Drape || balColReactTask->m_balancerState == bal_DrapeForward)) ||
         m_com2Edge > 0.f))
      {
        sendFeedback(teet_FallOnGround);
      }
      // FALL
      //
      else
      {
        falling(timeStep);
      }
        
      dynamicBalancerTask->setTeeter(false);
    }
    else
    {
      // TEETER
      //
      teetering(timeStep);
    }

    return eCBUTaskComplete;
  }

  void NmRsCBUTeeter::teetering(float timeStep)
  {
#if ART_ENABLE_BSPY
        bspyLogf(info, L"teeter TEETERING");
#endif

    GET_TASK(armsWindmill, ArmsWindmill, armsWindmillTask)
    GET_TASK(balancerCollisionsReaction, BalancerCollisionsReaction, balColReactTask)
    GET_TASK(dynamicBalancer, DynamicBalancer, dynamicBalancerTask)
    GET_TASK(catchFall, CatchFall, catchFallTask)
    GET_TASK(spineTwist, SpineTwist, spineTwistTask)

    // pick a TARGET ON EDGE for look, turn and grab
    //

    // CLOSEST POINT on line...
    rage::Vector3 edgeDir = m_edge;
    edgeDir.Normalize();
    float fEdgeTarget = edgeDir.Dot(m_levelCom - m_edgeLeft);
    m_edgeTarget.AddScaled(m_edgeLeft, edgeDir, fEdgeTarget);

    // OFFSET POINT based on orientation TODO velocity?
    //
    const float maxOffset = 0.5f;
    rage::Matrix34 tm;
    getSpine()->getSpine1Part()->getMatrix(tm);
    float fEdgeTargetOffset = -edgeDir.Dot(tm.c);
    float velFwd = -m_character->m_COMTM.c.Dot(m_character->m_COMvel);
    if(velFwd > 0.0f)
      fEdgeTargetOffset = (fEdgeTargetOffset > 0.0f ? 1.0f : -1.0f);
    m_edgeTarget.AddScaled(edgeDir, fEdgeTargetOffset * maxOffset);

    // HEAD LOOK at edge sometimes
    //
    // TODO look in direction of travel until close to edge/falling.
    GET_TASK(headLook, HeadLook, headLookTask);
    if (m_parameters.useHeadLook && m_time2Edge < 2.0f)
    {
      if (!headLookTask->isActive())
        headLookTask->updateBehaviourMessage(NULL);
      headLookTask->m_parameters.m_pos = m_edgeTarget;
      headLookTask->m_parameters.m_vel.Set(0,0,0);
      headLookTask->m_parameters.m_stiffness = 11.0f;
      headLookTask->m_parameters.m_damping = 1.0f;
      headLookTask->m_parameters.m_alwaysLook = true;
      headLookTask->m_parameters.m_instanceIndex = -1;
      if(!headLookTask->isActive())
        headLookTask->activate();
    }
    else
    {
      headLookTask->m_parameters.m_pos = m_character->m_COM + m_character->m_COMvel;
      headLookTask->m_parameters.m_alwaysLook = false;
    }

    dynamicBalancerTask->setHipPitch(0.0f);
    dynamicBalancerTask->setGiveUpThreshold(1.f);
    dynamicBalancerTask->setTeeter(true);
    dynamicBalancerTask->taperKneeStrength(true);

    catchFallTask->m_handsAndKnees = true;//mmmmtodo mmmmnote messes up setFallingReaction setup
    catchFallTask->m_callRDS = true;//mmmmtodo mmmmnote messes up setFallingReaction setup
    catchFallTask->m_comVelRDSThresh = 2.0f;//mmmmtodo mmmmnote messes up setFallingReaction setup
    catchFallTask->m_armReduceSpeed = 0.2f;//mmmmtodo mmmmnote messes up setFallingReaction setup

    // Instruct balancer not to step over the edge.
    //
    balColReactTask->m_impactOccurred = m_parameters.useExclusionZone;

    // Set up BALANCER EXCLUSION ZONE
    //
    if (m_parameters.useExclusionZone)
    {
      balColReactTask->m_pos1stContact = m_edgeLeft;
      balColReactTask->m_normal1stContact = m_edgeNormal;
      balColReactTask->m_sideOfPlane = -1.f;
      balColReactTask->m_parameters.exclusionZone = 0.2f;
    }

    // PRE TEETER
    //
    if (m_time2Edge < m_parameters.preTeeterTime)
    {
#if ART_ENABLE_BSPY
        bspyLogf(info, L"teeter PRE TEETER");
#endif
      
      sendFeedback(teet_PreTeeter);

      m_teeterTimer += timeStep;

      // WINDMILL
      //
      if(true) // TODO!
      {
        if (!armsWindmillTask->isActive())
        {
          armsWindmillTask->updateBehaviourMessage(NULL);

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
          armsWindmillTask->m_parameters.m_leftCircleDesc.radius1 = 0.5f;
          armsWindmillTask->m_parameters.m_leftCircleDesc.radius2 = 0.5f;
          armsWindmillTask->m_parameters.m_leftCircleDesc.speed = 1.5f;
          armsWindmillTask->m_parameters.m_leftCircleDesc.normal = rage::Vector3(0.f, 0.5f, -0.1f);
          armsWindmillTask->m_parameters.m_leftCircleDesc.centre = rage::Vector3(0.f, 0.5f, -0.1f);
          armsWindmillTask->m_parameters.m_rightCircleDesc.partID = 10;
          armsWindmillTask->m_parameters.m_rightCircleDesc.radius1 = 0.75f;
          armsWindmillTask->m_parameters.m_rightCircleDesc.radius2 = 0.75f;
          armsWindmillTask->m_parameters.m_rightCircleDesc.speed = 1.5f;
          armsWindmillTask->m_parameters.m_rightCircleDesc.normal = rage::Vector3(0.f, 0.5f, -0.1f);
          armsWindmillTask->m_parameters.m_rightCircleDesc.centre = rage::Vector3(0.f, 0.5f, -0.1f);
          if (velFwd < 0.f)
          {
            armsWindmillTask->m_parameters.m_leftCircleDesc.speed *= -1.f;
            armsWindmillTask->m_parameters.m_rightCircleDesc.speed *= -1.f;
          }

          armsWindmillTask->activate();
          armsWindmillTask->setOK2Deactivate(false); //Dont let shot deactivate this behaviour if we're using it here
        }

        //
        // Switch windmill directions based on velocity.
        if (velFwd < 0.f)
        {
          armsWindmillTask->m_parameters.m_leftCircleDesc.speed  = -1.5f;
          armsWindmillTask->m_parameters.m_rightCircleDesc.speed = -1.5f;
        }

        //
        // Increase windmill strength and amplitude based on approach velocity [11..15]
        armsWindmillTask->m_parameters.m_shoulderStiffness = rage::Min(11.f + rage::Abs(m_vel2Edge) * 2.0f, 15.0f);
      }

      // TORQUE ANKLES to pitch the feet toes-down
      //
      if(velFwd > 0.0f)
      {
        rage::Matrix34 footM;
        rage::Vector3 torque;
            
        // Is RIGHT FOOT planted?
        if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kNotStepping ||
            dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)
        {
          // Is foot RIGHT FOOT supported and flat?
          //
          getRightLeg()->getFoot()->getMatrix(footM);
          if (m_edge2RightFoot > 0.0f && footM.b.Dot(m_character->m_gUp) > 0.9f)
          {
            torque = footM.a;
            torque *= -100.f;
            (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtAnkle_Right)))->ApplyTorque(torque);
  #if ART_ENABLE_BSPY
            bspyLogf(info, L"teeter TORQUE RIGHT ANKLE");
  #endif
          }
        }

        // Is LEFT FOOT planted?
        if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kNotStepping ||
            dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
        {
          // Is LEFT FOOT supported and flat?
          //
          getLeftLeg()->getFoot()->getMatrix(footM);
          if (m_edge2LeftFoot > 0.0f && footM.b.Dot(m_character->m_gUp) > 0.9f)
          { 
            torque = footM.a;
            torque *= -100.f;
            (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtAnkle_Left)))->ApplyTorque(torque);
  #if ART_ENABLE_BSPY
            bspyLogf(info, L"teeter TORQUE LEFT ANKLE");
  #endif
          }
        }
      }
    }
    else
    {
      armsWindmillTask->deactivate();
    }

    // LEAN AWAY and TURN
    //
    rage::Vector3 turnTarget;
    if (/*m_time2Edge < m_parameters.leanAwayTime && */ m_vel2Edge > -1.0f && m_com2Edge < 1.0f)
    {
#if ART_ENABLE_BSPY
      bspyLogf(info, L"teeter LEAN AWAY");
#endif
      // Keep balancer going through deep steps.
      //
      dynamicBalancerTask->taperKneeStrength(false);

      sendFeedback(teet_LeanAwayZone);

      float lean = rage::Clamp(m_vel2Edge - 1.0f, 0.0f, 3.0f) / 3.0f; // [0..1]
      float leanAmount = lean * m_parameters.leanAwayScale;

      // BIGGER STEPS based on velocity
      //
      // TODO might better based on m_com2edge: Tune to encourage one big step
      // right up to the edge
      //
      float balanceTime = 0.2f + lean * 0.3f;

      // TODO Modulate balancer leg straighteness with distance to edge.D

      dynamicBalancerTask->autoLeanInDirection(m_edgeNormal, leanAmount);
      dynamicBalancerTask->autoLeanHipsInDirection(m_edgeNormal, leanAmount);
      dynamicBalancerTask->setBalanceTime(balanceTime);

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "teeter", leanAmount);
      bspyScratchpad(m_character->getBSpyID(), "teeter", balanceTime);
#endif

#if 0
      static bool turn = false;
      if (turn)
      {
        turnTarget = m_edgeLeft;
        turnTarget -= m_character->m_COM;
        if (lean > 0.05f)
          dynamicBalancerTask->useCustomTurnDir(true, turnTarget);
        else
          dynamicBalancerTask->useCustomTurnDir(false, turnTarget);
      }
#endif

    }
    else
    {
      dynamicBalancerTask->autoLeanCancel();
      dynamicBalancerTask->autoLeanHipsCancel();

      dynamicBalancerTask->setBalanceTime(0.2f);

      turnTarget.Zero();
      dynamicBalancerTask->useCustomTurnDir(false, turnTarget);
    }

    // TEETER
    //
    // NB: This portion of the code has been substantially rewritten.
    //
    // If going forward...
    //   ...and possibly recoverable (not yet over the edge?)
    //        arch back away from the fall
    //        pitch the hips back (+)
    //        torque feet toes-down around side axis
    //  ...and unrecoverable (over the edge?)
    //        arch the back into the fall (snappy)
    //        pitch the hips forward (-)
    //  torque any planted feet
    //
    if(m_time2Edge < m_parameters.teeterTime && m_vel2Edge > 0.0f)
    {
#if ART_ENABLE_BSPY
      bspyLogf(info, L"teeter TEETER");
#endif
      sendFeedback(teet_Teeter);

      m_teeterTimer += timeStep;

      float amount = rage::Clamp(m_vel2Edge, 0.0f, 1.0f); //1.0f - m_time2Edge / m_parameters.leanTime;

      // And RECOVERABLE (not yet over edge)
      //
      if(m_com2Edge > 0.0f)
      {
#if ART_ENABLE_BSPY
        bspyLogf(info, L"teeter RECOVERABLE");
#endif

        dynamicBalancerTask->setHipPitch(amount * 0.15f);

      }
      // NOT RECOVERBLE (over the edge)
      //
      else
      {
#if ART_ENABLE_BSPY
        bspyLogf(info, L"teeter NOT RECOVERABLE");
#endif

        dynamicBalancerTask->setHipPitch(amount * 0.5f);

        // LEAN FORWARD
        //
        float spineLean1 = 0.5f * amount;
        getSpineInputData()->getSpine0()->setDesiredLean1(spineLean1);
        getSpineInputData()->getSpine1()->setDesiredLean1(spineLean1);
        getSpineInputData()->getSpine2()->setDesiredLean1(spineLean1);
        getSpineInputData()->getSpine3()->setDesiredLean1(spineLean1);

        // DROP and GRAB
        //
        if(m_com2Edge < 0.1f) // && dynamicBalancerTask->getLegStraightnessModifier() > -0.04f)
        {
          // Bend them to try to drop character on edge
          //
  #if ART_ENABLE_BSPY
          bspyLogf(info, L"teeter BEND KNEES");
  #endif
          dynamicBalancerTask->setLegStraightnessModifier(m_character->getRandom().GetRanged(-0.7f,-0.04f));
        }


#if 0
        if(m_com2Edge < 0.1f)
        {
          // Turn off exclusion zone in balancer to allow it to step beyond the edge.
          //
          balColReactTask->m_impactOccurred = false;
          dynamicBalancerTask->setTeeter(false);
        }
#endif
      }

      // MODIFY CATCH FALL if either foot is far over the edge
      //
      const float footDist = -0.35f;
      if (m_edge2LeftFoot < footDist || m_edge2RightFoot < footDist)
      {
        catchFallTask->m_handsAndKnees = true;//mmmmtodo mmmmnote messes up setFallingReaction setup
        catchFallTask->m_callRDS = true;//mmmmtodo mmmmnote messes up setFallingReaction setup
        catchFallTask->m_comVelRDSThresh = 2.0f;//mmmmtodo mmmmnote messes up setFallingReaction setup
        catchFallTask->m_armReduceSpeed = 0.2f;//mmmmtodo mmmmnote messes up setFallingReaction setup

        dynamicBalancerTask->forceFailOnly();
      }

      // SPINE TWIST and CUSTOM TURN DIR in target direction
      //
      dynamicBalancerTask->useCustomTurnDir(true, m_edgeTarget);
      spineTwistTask->setSpineTwistPos(m_edgeTarget);
      if(!spineTwistTask->isActive())
        spineTwistTask->activate();

      // TODO GRAB if possible
      //
      // If edge target is approaching and more-or-less within reach
      // override the Catch Fall with a grab?
    }
    else
    {
      spineTwistTask->deactivate();

      turnTarget.Zero();
      dynamicBalancerTask->useCustomTurnDir(false, turnTarget);
    }
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

    rage::Vector3 edge2Shoulder;
    edge2Shoulder = getLeftArm()->getClaviclePart()->getPosition() - m_edgeLeft;//Assumes normal is level
    m_character->levelVector(edge2Shoulder);
    float edge2LeftShoulder = edge2Shoulder.Dot(m_edgeNormal);
    edge2Shoulder = getRightArm()->getClaviclePart()->getPosition() - m_edgeLeft;//Assumes normal is level
    m_character->levelVector(edge2Shoulder);
    float edge2RightShoulder = edge2Shoulder.Dot(m_edgeNormal);

    //restrict the catchFall arm target to not go over the edge?
    rage::Vector3 edge2Spine3 = getSpine()->getSpine3Part()->getPosition() - m_edgeLeft;
    rage::Vector3 left2Right = m_edge;
    float dot = edge2Spine3.Dot(left2Right);
    left2Right.Normalize();
    edge2Spine3 = getSpine()->getSpine3Part()->getPosition();
    edge2Spine3 -= m_parameters.edgeLeft + dot * left2Right;
    const float fEdge2Spine3 = edge2Spine3.Mag();
    bool armsToFarAway = (fEdge2Spine3 > 0.8f);

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "teeter", fEdge2Spine3);
    bspyScratchpad(m_character->getBSpyID(), "teeter", armsToFarAway);
#endif

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

    if (m_character->hasCollidedWithEnvironment(bvmask_Full)== false && (m_edge2LeftFoot < -0.1) && (m_edge2RightFoot < -0.1))
    {
      m_highFall = true;
      if(m_parameters.callHighFall && !highFallTask->isActive())
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
    bspyTaskVar(m_parameters.useHeadLook, true);
    bspyTaskVar(m_parameters.callHighFall, true);
    bspyTaskVar(m_parameters.leanAway, true);

    bspyTaskVar(m_teeterTimer, false);
    bspyTaskVar(m_edgeLeft, false);
    bspyTaskVar(m_edge, false);
    bspyTaskVar(m_edgeNormal, false);
    bspyTaskVar(m_edgeTarget, false);
    bspyTaskVar(m_levelCom, false);
    bspyTaskVar(m_com2Edge, false);
    bspyTaskVar(m_time2Edge, false);
    bspyTaskVar(m_vel2Edge, false);
    bspyTaskVar(m_edge2LeftFoot, false);
    bspyTaskVar(m_edge2RightFoot, false);
    bspyTaskVar_StringEnum(m_state, teeterStateStrings, false);
    bspyTaskVar(m_characterIsFalling, false);

  }
#endif // ART_ENABLE_BSPY
}
