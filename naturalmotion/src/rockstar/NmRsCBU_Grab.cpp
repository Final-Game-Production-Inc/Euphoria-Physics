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
 * A behaviour only on the arms and clavicles of the character. 
 * Will grab/brace to a point, the nearest point on a line or the nearest point on a surface. Grab will create a constrant between the hand and a object. Brace will not create any constraint. 
 * This behaviour can be played over other behaviours. There is NO logic about when to grab/brace. Will always try to grab/brace to the desired points, no matter what. 
 *
 * There are 3 ways to specify the grab points.
 * Points: right grab/brace point (pos1) and/or left grab/brace point(pos2).
 * Line: grabs/braces with the left and/or right hand to the nearest point on the line between (pos1) and (pos2). 
 * Quad Surface: grabs/braces to the surface specified by (pos1), (pos2), (pos3) and (pos4). These points must be specified in a anitclockwise order.
 *
 *  The normals ( right/left are normal(normalR)/normal(normalL) respectively) can be specified for all of the grab point input methods. If no normal is specified the behaviour will attempt to find the appropriate normal.
 * 
 * The grab points are specified in the coord frame of the instance specified by instanceIndex. ( -1 = world space).
 *
 * PullUp: setting a pull up strength , pullUpStrength, result in the arms trying to pull relative to this strength over a time, pullUpTime. 0 = no attempt to pull up. 1 = attemp to pull up the maximum amount.  Pull up is inhibited when both hands are constrained.
 */


#include "NmRsInclude.h"
#include "NmRsCBU_Grab.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_SpineTwist.h" 
#include "NmRsCBU_HeadLook.h" 
#include "NmRsCBU_DynamicBalancer.h"
#if ART_ENABLE_BSPY
#include "physics/constraintspherical.h"
#endif
namespace ART
{
  NmRsCBUGrab::NmRsCBUGrab(ART::MemoryManager* services) : CBUTaskBase(services, bvid_grab)
  {
    initialiseCustomVariables();
  }

  NmRsCBUGrab::~NmRsCBUGrab()
  {
  }

  void NmRsCBUGrab::initialiseCustomVariables()
  {
    m_mask = bvmask_UpperBody;

    // internal variables
#if NM_EA
    m_rightClosestTarget.Set(0);
    m_leftClosestTarget.Set(0);
#endif//#if NM_EA
    m_rightTarget.Set(0);
    m_leftTarget.Set(0);
    m_rightNormal.Set(0);
    m_leftNormal.Set(0);
    m_holdTimerRight = 0.0f;
    m_holdTimerLeft = 0.0f;
    m_rightIsConstrained = false;
    m_leftIsConstrained = false;
    m_constraintL.Reset();
    m_constraintR.Reset();
    m_doGrab = false;
    m_1stTimeHeadLookDeactivated = true;   
    m_behaviourTime = 0.0f;
    m_constraintInhibitTimeRight = 0.0f;
    m_constraintInhibitTimeLeft = 0.0f;
    m_alreadyInit = false;
    m_alreadyGrabTimer = 0.f;
  }

  void NmRsCBUGrab::onActivate()
  {
    Assert(m_character);

    m_constraintL.Reset();
    m_constraintR.Reset();
    m_1stTimeHeadLookDeactivated = true;
#if NM_EA
    m_grabPatchL = -1;
    m_grabPatchR = -1;
#endif//#if NM_EA

    tick(0.0f);
  }

  void NmRsCBUGrab::onDeactivate()
  {
    Assert(m_character);

    // turn off any constraints that may be on
    m_character->ReleaseConstraintSafely(m_constraintL);
    m_character->ReleaseConstraintSafely(m_constraintR);

    // enable the hand collision if the are turned off
    if (!(getLeftArm()->getHand()->isCollisionEnabled()))
#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
      m_character->reEnableCollision(getLeftArm()->getHand());
#else
      getLeftArm()->getHand()->setCollisionEnabled(true);
#endif

    if (!(getLeftArm()->getLowerArm()->isCollisionEnabled()))
#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
      m_character->reEnableCollision(getLeftArm()->getLowerArm());
#else
      getLeftArm()->getLowerArm()->setCollisionEnabled(true);   
#endif

    if (!(getRightArm()->getHand()->isCollisionEnabled()))
#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
      m_character->reEnableCollision(getRightArm()->getHand());
#else
      getRightArm()->getHand()->setCollisionEnabled(true);
#endif

    if (!(getRightArm()->getLowerArm()->isCollisionEnabled()))
#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
      m_character->reEnableCollision(getRightArm()->getLowerArm());
#else
      getRightArm()->getLowerArm()->setCollisionEnabled(true);
#endif

    // need to deactivate the headLook and the SpineTwist if they are active
    // controlHeadLook(false,NULL);
    // controlSpineTwist(false,false,false,NULL);
    //Allow headlook to stay on during braceForImpact: replaced above with below
    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    headLookTask->deactivate();

    NmRsCBUSpineTwist* spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
    Assert(spineTwistTask);
    spineTwistTask->deactivate();

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    dynamicBalancerTask->useCustomTurnDir(false,m_rightTarget);//m_rightTarget is local but doesn't matter as we're just turning this off

    initialiseCustomVariables();

    m_constraintL.Reset();
    m_constraintR.Reset();

    // set the muscle stiffness back to normal
    //Only reset what is set i.e. Spine,clavicles,and arms.  Only the arms set the MuscleStiffness not to 1
    float mStiff = 1;

    m_body->setStiffness(10.0f, 1.0f, bvmask_ArmLeft | bvmask_ArmRight, &mStiff);
    m_body->setStiffness(10.0f, 1.0f, bvmask_LowSpine);
  }


  CBUTaskReturn NmRsCBUGrab::onTick(float timeStep)
  {

    bool moveRight = m_parameters.useRight;
    bool moveLeft = m_parameters.useLeft;
    m_reachDistance = m_parameters.maxReachDistance;

    // 1. decide if we should be grabbing
    //--------------------------------------------------------------------------------------------------------------------
    if (moveLeft || moveRight)
    {
      if (!(m_parameters.dontLetGo 
        && m_constraintR.IsValid() && m_parameters.useRight 
        && m_constraintL.IsValid() && m_parameters.useLeft))
        decideToGrab(moveLeft, moveRight);
      if (m_parameters.dontLetGo)
      {
        if (m_constraintL.IsValid() && m_parameters.useLeft)
          moveLeft = true;
        if (m_constraintR.IsValid() && m_parameters.useRight)
          moveRight = true;
      }
    }

    if (m_parameters.grabHoldMaxTimer > 0.f)
    {
      if (m_holdTimerRight > m_parameters.grabHoldMaxTimer)
        moveRight = false;

      if (m_holdTimerLeft > m_parameters.grabHoldMaxTimer)
        moveLeft = false;
    }

    // Turn off sub behaviours and tech 
    //   only after we have used them (i.e. we grabbed last frame), otherwise interferes with other behaviours
    if (!(moveLeft || moveRight) && m_doGrab)
    {
      // deactivate the HeadLook and the SpineTwist
      controlHeadLook(false,NULL);
      controlSpineTwist(false,NULL);
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      dynamicBalancerTask->useCustomTurnDir(false,m_rightTarget);//m_rightTarget is local but doesn't matter as we're just turning this off
      dynamicBalancerTask->autoLeanHipsCancel();       
      
      //release the constraints
      if (m_constraintL.IsValid()) 
      {
        m_character->ReleaseConstraintSafely(m_constraintL);
        m_leftIsConstrained = false;
        //m_constraintInhibitTimeLeft = m_behaviourTime + 1.0f; // delay attempts to re-constrain by 1 second
      }
      if (m_constraintR.IsValid()) 
      {
        m_character->ReleaseConstraintSafely(m_constraintR);
        m_rightIsConstrained = false;
        //m_constraintInhibitTimeRight = m_behaviourTime + 1.0f; // delay attempts to re-constrain by 1 second
      }

    }

    m_doGrab = (moveLeft || moveRight);

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "Grab", m_doGrab);
    bspyScratchpad(m_character->getBSpyID(), "Grab", moveLeft);
    bspyScratchpad(m_character->getBSpyID(), "Grab", moveRight);
    bspyScratchpad(m_character->getBSpyID(), "Grab", (m_constraintL.IsValid()));
    bspyScratchpad(m_character->getBSpyID(), "Grab", (m_constraintR.IsValid()));
#endif

    // 2. update the position to grab to if needed
    //--------------------------------------------------------------------------------------------------------------------
    if (m_doGrab)
    {
      rage::Vector3 leftTargetWorld(0.f, 0.f, 0.f);
      rage::Vector3 leftNormalWorld(1.f, 0.f, 0.f);
      rage::Vector3 rightTargetWorld(0.f, 0.f, 0.f);
      rage::Vector3 rightNormalWorld(1.f, 0.f, 0.f);

#if NM_EA
      if (m_parameters.fromEA)
      {
        leftTargetWorld = m_leftTarget;
        leftNormalWorld = m_leftNormal;
        rightTargetWorld = m_rightTarget;
        rightNormalWorld = m_rightNormal;
      }
      else
#endif//#if NM_EA
      {
        if (moveRight && !m_constraintR.IsValid()) 
        {
          rage::Vector3 rightShoulderPos = getRightArm()->getShoulder()->getJointPosition();
          if (m_parameters.pointsX4grab)
          {
            rage::Vector3 bodyTargetTemp;
            m_character->boundToLocalSpace(false, &bodyTargetTemp,getSpine()->getSpine3Part()->getPosition(),m_parameters.instanceIndex,m_parameters.boundIndex);
            if (bodyTargetTemp.Dist2(m_parameters.pos) < bodyTargetTemp.Dist2(m_parameters.pos2))
              updateGrabPos(m_rightTarget, m_rightNormal, m_parameters.pos, m_parameters.pos1, m_parameters.normalR,rightShoulderPos);
            else
              updateGrabPos(m_rightTarget, m_rightNormal, m_parameters.pos2, m_parameters.pos3, m_parameters.normalR2,rightShoulderPos);
          }
          else
            updateGrabPos(m_rightTarget, m_rightNormal, m_parameters.pos, m_parameters.pos1, m_parameters.normalR,rightShoulderPos);
        }
        if (moveLeft && !m_constraintL.IsValid())
        {
          rage::Vector3 leftShoulderPos = getLeftArm()->getShoulder()->getJointPosition();
          if (m_parameters.pointsX4grab)
          {
            rage::Vector3 bodyTargetTemp;
            m_character->boundToLocalSpace(false, &bodyTargetTemp,getSpine()->getSpine3Part()->getPosition(),m_parameters.instanceIndex,m_parameters.boundIndex);
            if (bodyTargetTemp.Dist2(m_parameters.pos1) < bodyTargetTemp.Dist2(m_parameters.pos3))
              updateGrabPos(m_leftTarget, m_leftNormal, m_parameters.pos1, m_parameters.pos, m_parameters.normalL,leftShoulderPos);
            else
              updateGrabPos(m_leftTarget, m_leftNormal, m_parameters.pos3, m_parameters.pos2, m_parameters.normalL2,leftShoulderPos);
          }
          else
            updateGrabPos(m_leftTarget, m_leftNormal, m_parameters.pos1, m_parameters.pos, m_parameters.normalL,leftShoulderPos);
        }

        if (m_parameters.useLineGrab && (m_leftTarget.Dist(m_rightTarget) <  0.15f))
        {
          //modifies m_leftTarget and m_rightTarget
          //uses m_parameters.pos and pos1
          moveGrabPointsApart((moveLeft && !m_constraintL.IsValid()), (moveRight && !m_constraintR.IsValid()));
        }

        m_character->boundToWorldSpace(&leftTargetWorld,m_leftTarget,m_parameters.instanceIndex,m_parameters.boundIndex);
        m_character->rotateBoundToWorldSpace(&leftNormalWorld,m_leftNormal,m_parameters.instanceIndex,m_parameters.boundIndex);

        m_character->boundToWorldSpace(&rightTargetWorld,m_rightTarget,m_parameters.instanceIndex,m_parameters.boundIndex);
        m_character->rotateBoundToWorldSpace(&rightNormalWorld,m_rightNormal,m_parameters.instanceIndex,m_parameters.boundIndex);


      }

      // 2b. check whether to drop a weapon if the carrying hand is close enough to target
      //-----------------------------------------------------------------------------------------------------------
      float leftHandToTargetDist = getLeftArm()->getHand()->getPosition().Dist(leftTargetWorld);
      float rightHandToTargetDist = getRightArm()->getHand()->getPosition().Dist(rightTargetWorld);

      // left hand to drop weapon?
      if(m_character->getCharacterConfiguration().m_leftHandState != CharacterConfiguration::eHS_Free &&
        m_parameters.dropWeaponIfNecessary && moveLeft && !m_constraintL.IsValid() && !m_constraintR.IsValid())
      {
        if(leftHandToTargetDist < m_parameters.dropWeaponDistance && rightHandToTargetDist > m_parameters.dropWeaponDistance)
          sendDropWeaponFeedback(0);
      }

      // right hand to drop weapon?
      if(m_character->getCharacterConfiguration().m_rightHandState != CharacterConfiguration::eHS_Free &&
        m_parameters.dropWeaponIfNecessary && moveRight && !m_constraintL.IsValid() && !m_constraintR.IsValid())
      {
        if(rightHandToTargetDist < m_parameters.dropWeaponDistance && leftHandToTargetDist > m_parameters.dropWeaponDistance)
          sendDropWeaponFeedback(1);
      }
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "Grab", leftHandToTargetDist);
      bspyScratchpad(m_character->getBSpyID(), "Grab", rightHandToTargetDist);
#endif
#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
      float lengthLowerArmAndHand = getLeftArm()->getHand()->getPosition().Dist(getLeftArm()->getWrist()->getJointPosition()) + getLeftArm()->getWrist()->getJointPosition().Dist(getLeftArm()->getElbow()->getJointPosition());
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "Grab", lengthLowerArmAndHand);
#endif
#endif
      // 3. move the hands, if active
      //-----------------------------------------------------------------------------------------------------------
      if ( moveLeft ) 
      {
        bool disableCollisions = true;
#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
        disableCollisions = lengthLowerArmAndHand>leftHandToTargetDist;
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "Grab.lefthand", disableCollisions);
#endif
#endif
        if (!m_parameters.justBrace && disableCollisions && !m_parameters.handsCollide) 
        {
          getLeftArm()->getHand()->setCollisionEnabled(false);
          //getLeftArm()->getLowerArm()->setCollisionEnabled(false);
        }
        // reach arm; allow constraint only if hands are not crossed and left arm constraint is not being inhibited.
        // with hard constraint:
        if( (!moveRight || !handsCrossed()) && m_behaviourTime >= m_constraintInhibitTimeLeft)
          moveArm(leftTargetWorld,leftNormalWorld,bvmask_ArmLeft,m_constraintL,getLeftArm(),getLeftArmInput(),1,m_leftIsConstrained,m_holdTimerLeft,m_parameters.pullUpStrengthLeft);
        // without hard constraint:
        else
          moveArm(leftTargetWorld,leftNormalWorld,bvmask_ArmLeft,m_constraintL,getLeftArm(),getRightArmInput(),1,m_leftIsConstrained,m_holdTimerLeft,m_parameters.pullUpStrengthLeft, false);
      }

      if ( moveRight )
      {
        bool disableCollisions = true;
#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
        disableCollisions = lengthLowerArmAndHand>rightHandToTargetDist;
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "Grab.righthand", disableCollisions);
#endif
#endif
        if (!m_parameters.justBrace && disableCollisions  && !m_parameters.handsCollide)
        {
          getRightArm()->getHand()->setCollisionEnabled(false);
          //getRightArm()->getLowerArm()->setCollisionEnabled(false);
        }
        // reach arm; allow constraint only if right arm constraint is not being inhibited.
        // with hard constraint:
        if(m_behaviourTime >= m_constraintInhibitTimeRight)
          moveArm(rightTargetWorld,rightNormalWorld,bvmask_ArmRight,m_constraintR,getRightArm(),getRightArmInput(),-1,m_rightIsConstrained,m_holdTimerRight,m_parameters.pullUpStrengthRight);
        // without hard constraint:
        else
          moveArm(rightTargetWorld,rightNormalWorld,bvmask_ArmRight,m_constraintR,getRightArm(),getLeftArmInput(),-1,m_rightIsConstrained,m_holdTimerRight,m_parameters.pullUpStrengthRight, false);
      }

      // 7. release the constraints if there is an active constraint and we are not using this hand
      //-----------------------------------------------------------------------------------------------------------
      if (!moveRight)
      {
        if (m_constraintR.IsValid())
        {
          m_character->ReleaseConstraintSafely(m_constraintR);
          sendFailureFeedback(1);
          m_rightIsConstrained = false;
          if (m_parameters.grabHoldMaxTimer > 0)
            m_holdTimerRight = m_parameters.grabHoldMaxTimer + 0.3f;
        }
        if ( !(getRightArm()->getHand()->isCollisionEnabled()) )
          getRightArm()->getHand()->setCollisionEnabled(true);

        if ( !(getRightArm()->getLowerArm()->isCollisionEnabled()) )
          getRightArm()->getLowerArm()->setCollisionEnabled(true);
      }
      if (!moveLeft)
      {
        if(m_constraintL.IsValid())
        {
          m_character->ReleaseConstraintSafely(m_constraintL);
          sendFailureFeedback(-1);
          m_leftIsConstrained = false;
          if (m_parameters.grabHoldMaxTimer > 0)
            m_holdTimerLeft = m_parameters.grabHoldMaxTimer + 0.3f;
        }
        if ( !(getLeftArm()->getHand()->isCollisionEnabled()) )
          getLeftArm()->getHand()->setCollisionEnabled(true);

        if ( !(getLeftArm()->getLowerArm()->isCollisionEnabled()) )
          getLeftArm()->getLowerArm()->setCollisionEnabled(true);
      }

      if (!m_parameters.useLeft)
        m_holdTimerLeft = 0.f;

      if (!m_parameters.useRight)
        m_holdTimerRight = 0.f;

      //8 - Release constraint if it is broken
      //    maybe should do this earlier?  needs to do before anything checks that constraint isActive
      //mmmmmtodo 
      if (m_constraintL.IsValid()) 
      {
        rage::phConstraintBase* baseConstraint = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(m_constraintL) );
        if (baseConstraint && baseConstraint->IsBroken())
        {
          m_character->ReleaseConstraintSafely(m_constraintL);
          m_leftIsConstrained = false;
          m_constraintInhibitTimeLeft = m_behaviourTime + 1.0f; // delay attempts to re-constrain by 1 second
        }
      }
      if (m_constraintR.IsValid()) 
      {
        rage::phConstraintBase* baseConstraint = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(m_constraintR) );
        if (baseConstraint && baseConstraint->IsBroken())
        {
          m_character->ReleaseConstraintSafely(m_constraintR);
          m_rightIsConstrained = false;
          m_constraintInhibitTimeRight = m_behaviourTime + 1.0f; // delay attempts to re-constrain by 1 second
        }
      }

      // 9. [JRP] release one constraint if both hands are grabbing and are crossed
      //-----------------------------------------------------------------------------------------------------------
      bool leftIsConstrained = false;
      if (m_constraintL.IsValid())
      {
        rage::phConstraintBase* baseConstraint = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(m_constraintL) );
        if (baseConstraint && (baseConstraint->GetInstanceA() == m_character->getFirstInstance() || 
          baseConstraint->GetInstanceB() == m_character->getFirstInstance()))
        {
          leftIsConstrained = true;
        }
      }

      bool rightIsConstrained = false;
      if (m_constraintR.IsValid())
      {
        rage::phConstraintBase* baseConstraint = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(m_constraintR) );
        if (baseConstraint && (baseConstraint->GetInstanceA() == m_character->getFirstInstance() || 
          baseConstraint->GetInstanceB() == m_character->getFirstInstance()))
        {
          rightIsConstrained = true;
        }
      }

      if(leftIsConstrained && rightIsConstrained && handsCrossed())
      {
        if(m_character->getRandom().GetBool())
        {
          m_character->ReleaseConstraintSafely(m_constraintL);
          sendFailureFeedback(-1);
          m_leftIsConstrained = false;
          m_constraintInhibitTimeLeft = m_behaviourTime + 1.0f; // delay attempts to re-constrain by 1 second
        }
        else
        {
          m_character->ReleaseConstraintSafely(m_constraintR);
          sendFailureFeedback(1);
          m_rightIsConstrained = false;
          m_constraintInhibitTimeRight = m_behaviourTime + 1.0f; // delay attempts to re-constrain by 1 second
        }
      }


      // 4. headLook
      //-----------------------------------------------------------------------------------------------------------
      rage::Vector3 aimPos(0,0,1);//Note if aimPos is not set below then it isn't used - we initialize just in case
      if (m_parameters.useHeadLookToTarget && (m_rightIsConstrained || m_leftIsConstrained))
      {
        aimPos = m_parameters.targetForHeadLook;
        controlHeadLook(true, &aimPos);
      }
      else
      {
        if (moveRight && moveLeft)
        {
          aimPos = rightTargetWorld + leftTargetWorld;
          aimPos.Scale(0.5f);
        }
        else if (moveLeft)
          aimPos = leftTargetWorld;
        else if (moveRight)
          aimPos = rightTargetWorld;

        // control the headLook
        NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
        Assert(headLookTask);
        float grabConstraintTimer = (m_holdTimerLeft > m_holdTimerRight) ? m_holdTimerLeft : m_holdTimerRight; //max(m_holdTimerLeft,m_holdTimerRight);
        NM_RS_DBG_LOGF(L" grabConstraintTimer = %.4f",grabConstraintTimer);        
        bool headLookStartCond = (grabConstraintTimer<0.05f);
        bool headLookEndCond = ( (!moveLeft && !moveRight) || (grabConstraintTimer > 1.60f) );
        if (headLookEndCond)
        {
          controlHeadLook(false,NULL);
        }
        else if(m_parameters.lookAtGrab && (headLookStartCond || headLookTask->isActive()))
        {
          if (moveRight || moveLeft)
            controlHeadLook(true, &aimPos);
        }
      }
      // 5. control the Spine Twist:
      //-----------------------------------------------------------------------------------------------------------
      NmRsCBUSpineTwist* spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
      Assert(spineTwistTask);
      if (m_parameters.useHeadLookToTarget && (m_rightIsConstrained || m_leftIsConstrained))
      {
        getSpine()->setBodyStiffness(getSpineInput(), m_parameters.bodyStiffness, 1.0f, bvmask_LowSpine);
        float clavStiffness = rage::Clamp(m_parameters.bodyStiffness*1.5f,0.f,20.f);
        controlSpineTwist(true,&aimPos);//aimpos is targetForHeadLook
        //help to turn by rotating the hand grabs
        if (m_leftIsConstrained && !m_rightIsConstrained)
        {
          getLeftArmInputData()->getClavicle()->setStiffness(clavStiffness,1.f);
          m_alreadyGrabTimer+= m_character->getLastKnownUpdateStep();
          rage::Vector3 posHand = getLeftArm()->getHand()->getPosition();
          rage::Vector3 posHandLevelled=posHand;
          m_character->levelVector(posHandLevelled);
          rage::Vector3 posCOM = m_character->m_COM;
          m_character->levelVector(posCOM);
          float dist = (posHandLevelled- posCOM).Mag();
          rage::Vector3 velCOM = m_character->m_COMvel;
          bool under = posHand.Dot(m_character->getUpVector())> (m_character->m_COM.Dot(m_character->getUpVector())+ (1.f - 0.4f*m_parameters.pullUpStrengthLeft));//COM must be under hands
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "Grab.useHeadLookToTarget", under);
          bspyScratchpad(m_character->getBSpyID(), "Grab.useHeadLookToTarget", m_alreadyInit);
          bspyScratchpad(m_character->getBSpyID(), "Grab.useHeadLookToTarget", m_alreadyGrabTimer);
          bspyScratchpad(m_character->getBSpyID(), "Grab.useHeadLookToTarget", dist);
          bspyScratchpad(m_character->getBSpyID(), "Grab.useHeadLookToTarget", velCOM.Mag());
#endif
          if ((under && m_alreadyGrabTimer>0.5f && dist <0.5f && velCOM.Mag()<2.0f) || m_alreadyInit)
          {
            //will start to rotate hand so need to modify parameters
            m_parameters.maxWristAngle = PI;
            m_parameters.orientationConstraintScale = 0.0f;

            rage::Matrix34 obj;
            m_alreadyInit = true;
            rage::Vector3 desiredFrontward = aimPos - m_character->m_COM;
            m_character->levelVector(desiredFrontward);
            desiredFrontward.Normalize();
            obj.a = desiredFrontward;
            rage::Vector3 desiredDown = m_character->getUpVector();
            desiredDown.Negate();
            obj.b = desiredDown;
            rage::Vector3 desiredSide ;
            desiredSide.Cross(desiredDown,desiredFrontward);
            desiredSide.Negate();
            obj.c = desiredSide;
            obj.d =posHand;
            //rotation to compensate pullup
            if (m_parameters.pullUpStrengthLeft > 0.0f)
            {
              float rotation = 4.0f*PI/6.0f*m_parameters.pullUpStrengthLeft*m_parameters.pullUpStrengthLeft;
              obj.Rotate(m_character->getUpVector(),-rotation);
            }
#if ART_ENABLE_BSPY && GrabBSpyDraw//desired
            rage::Vector3 d1s =obj.a;
            d1s.Normalize();
            rage::Vector3 d2s =obj.b;
            d2s.Normalize();
            rage::Vector3 d3s =obj.c;
            d3s.Normalize();
            m_character->bspyDrawLine(posHand, posHand+d1s*0.1f, rage::Vector3(1,0,0));
            m_character->bspyDrawLine(posHand, posHand+d2s*0.1f, rage::Vector3(0,1,0));
            m_character->bspyDrawLine(posHand, posHand+d3s*0.1f, rage::Vector3(0,0,1));
#endif

#if ART_ENABLE_BSPY && GrabBSpyDraw && 0 //actual 
            rage::Matrix34 handTM;
            getLeftArm()->getHand()->getMatrix(handTM);
            posHand.AddScaled(m_character->getUpVector(),0.3f);
            rage::Vector3 d1 =handTM.a;
            d1.Normalize();
            rage::Vector3 d2 =handTM.b;
            d2.Normalize();
            rage::Vector3 d3 =handTM.c;
            d3.Normalize();
            m_character->bspyDrawLine(posHand, posHand+d1*0.1f, rage::Vector3(1,0,0));
            m_character->bspyDrawLine(posHand, posHand+d2*0.1f, rage::Vector3(0,1,0));
            m_character->bspyDrawLine(posHand, posHand+d3*0.1f, rage::Vector3(0,0,1));
#endif
            m_character->pdPartOrientationToM(getLeftArm()->getHand(),obj, NULL, 5.0f, 1.0f);
            getLeftArmInputData()->getWrist()->setDesiredTwist(0.f);
          }
        }
        if (m_rightIsConstrained && !m_leftIsConstrained)
        {
          getRightArmInputData()->getClavicle()->setStiffness(clavStiffness,1.f);
          m_alreadyGrabTimer+= m_character->getLastKnownUpdateStep();
          rage::Vector3 posHand = getRightArm()->getHand()->getPosition();
          rage::Vector3 posHandLevelled=posHand;
          m_character->levelVector(posHandLevelled);
          rage::Vector3 posCOM = m_character->m_COM;
          m_character->levelVector(posCOM);
          float dist = (posHandLevelled- posCOM).Mag();
          rage::Vector3 velCOM = m_character->m_COMvel;
          bool under = posHand.Dot(m_character->getUpVector())> (m_character->m_COM.Dot(m_character->getUpVector()) + (1.f - 0.4f*m_parameters.pullUpStrengthRight));
#if ART_ENABLE_BSPY 
          bspyScratchpad(m_character->getBSpyID(), "Grab.useHeadLookToTarget", under);
          bspyScratchpad(m_character->getBSpyID(), "Grab.useHeadLookToTarget", m_alreadyInit);
          bspyScratchpad(m_character->getBSpyID(), "Grab.useHeadLookToTarget", m_alreadyGrabTimer);
          bspyScratchpad(m_character->getBSpyID(), "Grab.useHeadLookToTarget", dist);
          bspyScratchpad(m_character->getBSpyID(), "Grab.useHeadLookToTarget", velCOM.Mag());
#endif
          if ((under && m_alreadyGrabTimer>0.5f && dist <0.5f && velCOM.Mag()<2.0f) || m_alreadyInit)
          {
            //will start to rotate hand so need to modify parameters
            m_parameters.maxWristAngle = PI;
            m_parameters.orientationConstraintScale = 0.0f;
            rage::Matrix34 obj;
            m_alreadyInit = true;
            rage::Vector3 desiredBackward = m_character->m_COM - aimPos;
            m_character->levelVector(desiredBackward);
            desiredBackward.Normalize();
            obj.a = desiredBackward;
            rage::Vector3 desiredDown = m_character->getUpVector();
            desiredDown.Negate();
            obj.b = desiredDown;
            rage::Vector3 desiredSide ;
            desiredSide.Cross(desiredDown,desiredBackward);
            desiredSide.Negate();
            obj.c = desiredSide;
            obj.d =posHand;
            if (m_parameters.pullUpStrengthRight >0.f)
            {
              float rotation = 4.0f*PI/6.0f*m_parameters.pullUpStrengthRight*m_parameters.pullUpStrengthRight;
              obj.Rotate(m_character->getUpVector(),rotation);
            }
#if ART_ENABLE_BSPY && GrabBSpyDraw//desired
            rage::Vector3 d1s =obj.a;
            d1s.Normalize();
            rage::Vector3 d2s =obj.b;
            d2s.Normalize();
            rage::Vector3 d3s =obj.c;
            d3s.Normalize();
            m_character->bspyDrawLine(posHand, posHand+d1s*0.1f, rage::Vector3(1,0,0));
            m_character->bspyDrawLine(posHand, posHand+d2s*0.1f, rage::Vector3(0,1,0));
            m_character->bspyDrawLine(posHand, posHand+d3s*0.1f, rage::Vector3(0,0,1));
#endif


#if ART_ENABLE_BSPY && GrabBSpyDraw && 0 //actual 
            rage::Matrix34 handTM;
            getRightArm()->getHand()->getMatrix(handTM);
            posHand.AddScaled(m_character->getUpVector(),0.3f);
            rage::Vector3 d1 =handTM.a;
            d1.Normalize();
            rage::Vector3 d2 =handTM.b;
            d2.Normalize();
            rage::Vector3 d3 =handTM.c;
            d3.Normalize();
            m_character->bspyDrawLine(posHand, posHand+d1*0.1f, rage::Vector3(1,0,0));
            m_character->bspyDrawLine(posHand, posHand+d2*0.1f, rage::Vector3(0,1,0));
            m_character->bspyDrawLine(posHand, posHand+d3*0.1f, rage::Vector3(0,0,1));
#endif
            m_character->pdPartOrientationToM(getRightArm()->getHand(),obj, NULL, 5.0f, 1.0f);
            getRightArmInputData()->getWrist()->setDesiredTwist(0.f);
          }
        }
      }
      else
      {
        bool twistStartCond = (moveRight || moveLeft);
        bool twistEndCond = ((!moveLeft)&&(!moveRight));
        if (twistEndCond)
        {
          controlSpineTwist(false,NULL);
        }
        else if((twistStartCond)||(spineTwistTask->isActive()))
        {
          if (moveRight||moveLeft)
            controlSpineTwist(true,&aimPos);
        }
      }
      // 6. set a turn direction for the balance 
      //-----------------------------------------------------------------------------------------------------------
      //bool decis = (moveLeft && moveRight);
      //if (m_parameters.justBrace)
      bool decis = (moveLeft || moveRight);

      if (decis)
      {
        NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
        Assert(dynamicBalancerTask);
        rage::Vector3 turnD = aimPos - m_character->m_COM ;
        turnD.Normalize();
        if (m_parameters.turnToTarget == 2)//turnAwayFromTarget
          turnD *= -1;
        if (m_parameters.turnToTarget != 0)
          dynamicBalancerTask->useCustomTurnDir(true,turnD);
#if NM_EA 
        rage::Vector3 leanPos(0,0,0);
        //select closest target
        if (m_grabPatchR != -1 && m_grabPatchL == -1)
          leanPos = m_character->m_patches[m_grabPatchR].corner.global;
        if (m_grabPatchL != -1 && m_grabPatchR == -1)
          leanPos = m_character->m_patches[m_grabPatchL].corner.global;
        if (m_grabPatchL != -1 && m_grabPatchR != -1)
        {
          if ((m_character->m_COM - m_character->m_patches[m_grabPatchL].corner.global).Mag2() >
            (m_character->m_COM - m_character->m_patches[m_grabPatchR].corner.global).Mag2())
            leanPos = m_character->m_patches[m_grabPatchL].corner.global;
          else
            leanPos = m_character->m_patches[m_grabPatchL].corner.global;
        }
        if (!leanPos.IsZero())
          dynamicBalancerTask->autoLeanHipsToPosition(leanPos, 0.2f);
        else
          dynamicBalancerTask->autoLeanHipsCancel();
#endif
      }
    } // if m_doGrab
    else
    {
      //mmmmtodo this sends NMGrabFailureToGrabFeedBackName even if we have previously succeeded
      //mmmmtodo should also send NMGrabLetGoGrabFeedBackName if we have previously succeeded?
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 0;
        strcpy(feedback->m_behaviourName, NMGrabFailureToGrabFeedBackName);
        feedback->onBehaviourEvent();
      }
    }

    m_behaviourTime += timeStep;

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "Grab", (m_constraintL.IsValid()));
    bspyScratchpad(m_character->getBSpyID(), "Grab", (m_constraintR.IsValid()));
#endif
    //mmmmmhere
    static float wristStiff = 2.f;

    getLeftArmInputData()->getWrist()->setMuscleStiffness(wristStiff);
    getRightArmInputData()->getWrist()->setMuscleStiffness(wristStiff);

    return eCBUTaskComplete;
  }

  void NmRsCBUGrab::controlHeadLook(bool active, rage::Vector3 *pos)
  {
    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);

    if (!active) 
    {
      // we want to get the head to look forward for 1 frame but without turning off the headlook
      if (m_1stTimeHeadLookDeactivated)
      {
        m_1stTimeHeadLookDeactivated = false;
        rage::Vector3 bodyUp,bodyBack,lookAt;
        rage::Matrix34 tmCom(m_character->m_COMTM);
        getSpine()->getSpine3Part()->getBoundMatrix(&tmCom);
        bodyUp = tmCom.b;
        bodyBack = tmCom.c;
        bodyBack *= -1.f;//look 1m infront of spine3
        bodyUp *= 0.4f;
        bodyBack += bodyUp;//and up about 40cm
        bodyBack += getSpine()->getSpine3Part()->getPosition();
        //NM_RS_CBU_DRAWPOINT(bodyBack, 13.2f, rage::Vector3(1,0,1));

        //Convert global look position "bodyBack" to local character space
        m_character->instanceToLocalSpace(&lookAt,bodyBack,m_character->getFirstInstance()->GetLevelIndex());

        //set instance to this character
        headLookTask->m_parameters.m_instanceIndex = m_character->getFirstInstance()->GetLevelIndex();
        headLookTask->m_parameters.m_pos = lookAt;
      }
    }
    else
    {
      // if not currently on, turn it on
      if (!(headLookTask->isActive()))
      {
        headLookTask->updateBehaviourMessage(NULL);
        headLookTask->activate();
      }

      // set/update the HeadLook parameters

      headLookTask->m_parameters.m_pos = *pos;

      headLookTask->m_parameters.m_instanceIndex = -1;
      headLookTask->m_parameters.m_stiffness = m_parameters.bodyStiffness;
      headLookTask->m_parameters.m_damping = 1.20f;
      headLookTask->m_parameters.m_alwaysLook = true;
    }
  }

  void NmRsCBUGrab::controlSpineTwist(bool active, rage::Vector3 *pos)
  {

    NmRsCBUSpineTwist* spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
    Assert(spineTwistTask);

    if (!active) 
    {
      // we want to deactivate the spineTwist
      if (spineTwistTask->isActive())
        spineTwistTask->deactivate();
    }
    else
    {
      // if not currently on, turn it on
      if (!(spineTwistTask->isActive()))
        spineTwistTask->activate();

      // set/update the spinetwist parameters

      spineTwistTask->setSpineTwistPos(*pos);
      //spineTwistTask->setSpineTwistStiffness(m_parameters.bodyStiffness);
      spineTwistTask->setSpineTwistAllwaysTwist(true);
      if (m_parameters.useHeadLookToTarget)
      {
        spineTwistTask->setSpineTwistTwistClavicles(true);
      }
      else
      {
        spineTwistTask->setSpineTwistTwistClavicles(false);
      }
    }
  }


#if ART_ENABLE_BSPY
  void NmRsCBUGrab::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.useRight, true);
    bspyTaskVar(m_parameters.useLeft, true);
    bspyTaskVar(m_parameters.dropWeaponIfNecessary, true);
    bspyTaskVar(m_parameters.dropWeaponDistance, true);
    bspyTaskVar(m_parameters.grabStrength, true);
    bspyTaskVar(m_parameters.stickyHands, true);
    bspyTaskVar(m_parameters.turnToTarget, true);

    bspyTaskVar(m_parameters.pullUpTime, true);
    bspyTaskVar(m_parameters.pullUpStrengthLeft, true);
    bspyTaskVar(m_parameters.pullUpStrengthRight, true);
    bspyTaskVar(m_parameters.grabHoldMaxTimer, true);


    bspyTaskVar(m_parameters.handsCollide, true);
    bspyTaskVar(m_parameters.pointsX4grab, true);
    bspyTaskVar(m_parameters.useLineGrab, true);
    bspyTaskVar(m_parameters.surfaceGrab, true);
    bspyTaskVar(m_parameters.dontLetGo, true);
    bspyTaskVar(m_parameters.justBrace, true);

    bspyTaskVar(m_parameters.pos, true);
    bspyTaskVar(m_parameters.pos1, true);
    bspyTaskVar(m_parameters.pos2, true);
    bspyTaskVar(m_parameters.pos3, true);
    bspyTaskVar(m_parameters.normalL, true);
    bspyTaskVar(m_parameters.normalR, true);
    bspyTaskVar(m_parameters.normalL2, true);
    bspyTaskVar(m_parameters.normalR2, true);

    bspyTaskVar(m_parameters.boundIndex, true);
    bspyTaskVar(m_parameters.instanceIndex, true);

    bspyTaskVar(m_parameters.armStiffness, true);
    bspyTaskVar(m_parameters.grabDistance, true);
    bspyTaskVar(m_parameters.reachAngle, true);
    bspyTaskVar(m_parameters.oneSideReachAngle, true);
    bspyTaskVar(m_parameters.bodyStiffness, true);
    bspyTaskVar(m_parameters.maxReachDistance, true);
    bspyTaskVar(m_parameters.maxWristAngle, true);

    bspyTaskVar(m_leftTarget, false);
    bspyTaskVar(m_rightTarget, false);
    bspyTaskVar(m_leftNormal, false);
    bspyTaskVar(m_rightNormal, false);

    rage::Vector3 leftTargetWorld;
    rage::Vector3 leftNormalWorld;
    rage::Vector3 rightTargetWorld;
    rage::Vector3 rightNormalWorld;
    m_character->boundToWorldSpace(&leftTargetWorld,m_leftTarget,m_parameters.instanceIndex,m_parameters.boundIndex);
    m_character->rotateBoundToWorldSpace(&leftNormalWorld,m_leftNormal,m_parameters.instanceIndex,m_parameters.boundIndex);
    m_character->boundToWorldSpace(&rightTargetWorld,m_rightTarget,m_parameters.instanceIndex,m_parameters.boundIndex);
    m_character->rotateBoundToWorldSpace(&rightNormalWorld,m_rightNormal,m_parameters.instanceIndex,m_parameters.boundIndex);
    
    bspyTaskVar(leftTargetWorld, false);
    bspyTaskVar(rightTargetWorld, false);
    bspyTaskVar(leftNormalWorld, false);
    bspyTaskVar(rightNormalWorld, false);

#if NM_EA
    bspyTaskVar(m_rightClosestTarget, false);
    bspyTaskVar(m_leftClosestTarget, false);
#endif //NM_EA 

    bspyTaskVar(m_holdTimerRight, false);
    bspyTaskVar(m_holdTimerLeft, false);

    bspyTaskVar(m_rightIsConstrained, false);
    bspyTaskVar(m_leftIsConstrained, false);

    bspyTaskVar(m_doGrab, false);

    bspyTaskVar(m_1stTimeHeadLookDeactivated, false);

    bspyTaskVar(m_reachDistance, false);

    bspyTaskVar(m_leftHandOriError, false);
    bspyTaskVar(m_rightHandOriError, false);

    bspyTaskVar(m_parameters.useHeadLookToTarget, true);
    bspyTaskVar(m_parameters.lookAtGrab, true);
    bspyTaskVar(m_parameters.targetForHeadLook, true);

    bspyTaskVar(m_constraintInhibitTimeLeft, false);
    bspyTaskVar(m_constraintInhibitTimeRight, false);

#define drawConstraints 0
    rage::phConstraintBase* constraintBaseL = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(m_constraintL) );
    if (constraintBaseL)
    {
      bspyTaskVar(constraintBaseL->GetType(), false);
#if 1 //constraintL is always spherical for grab
      if (constraintBaseL->GetType() == rage::phConstraintBase::DISTANCE) //distance;
      {
        rage::phConstraintDistance* constraintL = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_constraintL) );
        rage::phInst *pInstA = NULL;
        rage::phInst *pInstB = NULL;
        pInstA = constraintL->GetInstanceA();
        pInstB = constraintL->GetInstanceB();
        if (pInstA)
          bspyTaskVar(constraintL->GetInstanceA()->GetLevelIndex(), false);
        if (pInstB)
          bspyTaskVar(constraintL->GetInstanceB()->GetLevelIndex(), false);
        bspyTaskVar(constraintL->GetComponentA(), false);
        bspyTaskVar(constraintL->GetComponentB(), false);
        bspyTaskVar(VEC3V_TO_VECTOR3(constraintL->GetWorldPosA()), false);
        bspyTaskVar(VEC3V_TO_VECTOR3(constraintL->GetWorldPosB()), false);
        bspyTaskVar(constraintL->GetTypeName(), false);
        bspyTaskVar(constraintL->GetMaxDistance(), false);
        bspyTaskVar(constraintL->IsBroken(), false);
        
        rage::Vector3 worldPosA(VEC3V_TO_VECTOR3(constraintL->GetWorldPosA()));
        rage::Vector3 worldPosB(VEC3V_TO_VECTOR3(constraintL->GetWorldPosB()));
        rage::Vector3 worldPosB2A = worldPosA - worldPosB;
        worldPosB2A.Normalize();
        float radius = constraintL->GetMaxDistance();
        rage::Vector3 constraintRErrorPos = worldPosA - radius*worldPosB2A;
        bspyTaskVar(constraintRErrorPos, false);
#if drawConstraints
        m_character->bspyDrawPoint(worldPosB, 0.05f, rage::Vector3(0,1,0));//green - point on gun
        m_character->bspyDrawLine(constraintRErrorPos, worldPosA, rage::Vector3(0,1,0));//constraint length
        m_character->bspyDrawLine(constraintRErrorPos, worldPosB, rage::Vector3(1,0,0));//constraint error
        m_character->bspyDrawPoint(worldPosA, 0.05f, rage::Vector3(1,0,0));//red - point on hand
#endif
     }
#endif
      if (constraintBaseL->GetType() == rage::phConstraintBase::SPHERICAL) //spherical;
      {
        rage::phConstraintSpherical* constraintL = static_cast<rage::phConstraintSpherical*>( PHCONSTRAINT->GetTemporaryPointer(m_constraintL) );
        rage::phInst *pInstA = NULL;
        rage::phInst *pInstB = NULL;
        pInstA = constraintL->GetInstanceA();
        pInstB = constraintL->GetInstanceB();
        if (pInstA)
          bspyTaskVar(constraintL->GetInstanceA()->GetLevelIndex(), false);
        if (pInstB)
          bspyTaskVar(constraintL->GetInstanceB()->GetLevelIndex(), false);
        bspyTaskVar(constraintL->GetComponentA(), false);
        bspyTaskVar(constraintL->GetComponentB(), false);
        bspyTaskVar(VEC3V_TO_VECTOR3(constraintL->GetWorldPosA()), false);
        bspyTaskVar(VEC3V_TO_VECTOR3(constraintL->GetWorldPosB()), false);
        bspyTaskVar(constraintL->GetTypeName(), false);
        bspyTaskVar(constraintL->IsBroken(), false);
#if drawConstraints
        rage::Vector3 worldPosAL(VEC3V_TO_VECTOR3(constraintL->GetWorldPosA()));
        rage::Vector3 worldPosBL(VEC3V_TO_VECTOR3(constraintL->GetWorldPosB()));
        // draw a cross representing the radius of the constraint.
        m_character->bspyDrawPoint(leftTargetWorld, 0.05f, rage::Vector3(0,1,0));//green - point on gun
        m_character->bspyDrawLine(leftTargetWorld, worldPosBL, rage::Vector3(0,1,0));//constraint length to go
        m_character->bspyDrawPoint(worldPosBL, 0.05f, rage::Vector3(0,0,1));//blue - current constraint target
        m_character->bspyDrawLine(worldPosAL, worldPosBL, rage::Vector3(1,0,0));//red error in constraint
        m_character->bspyDrawPoint(worldPosAL, 0.05f, rage::Vector3(1,0,0));//red - point on hand
#endif
      }//spherical
    }//constraintL exists

    rage::phConstraintBase* constraintBaseR = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(m_constraintR) );
    if (constraintBaseR)
    {
      bspyTaskVar(constraintBaseR->GetType(), false);
#if 1 //constraintR is always spherical for grab
      if (constraintBaseR->GetType() == rage::phConstraintBase::DISTANCE) //distance;
      {
        rage::phConstraintDistance* constraintR = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_constraintR) );
        rage::phInst *pInstA = NULL;
        rage::phInst *pInstB = NULL;
        pInstA = constraintR->GetInstanceA();
        pInstB = constraintR->GetInstanceB();
        if (pInstA)
          bspyTaskVar(constraintR->GetInstanceA()->GetLevelIndex(), false);
        if (pInstB)
          bspyTaskVar(constraintR->GetInstanceB()->GetLevelIndex(), false);
        bspyTaskVar(constraintR->GetComponentA(), false);
        bspyTaskVar(constraintR->GetComponentB(), false);
        bspyTaskVar(VEC3V_TO_VECTOR3(constraintR->GetWorldPosA()), false);
        bspyTaskVar(VEC3V_TO_VECTOR3(constraintR->GetWorldPosB()), false);
        bspyTaskVar(constraintR->GetTypeName(), false);
        bspyTaskVar(constraintR->GetMaxDistance(), false);
        bspyTaskVar(constraintR->IsBroken(), false);
#if drawConstraints
        rage::Vector3 worldPosA(VEC3V_TO_VECTOR3(constraintR->GetWorldPosA()));
        rage::Vector3 worldPosB(VEC3V_TO_VECTOR3(constraintR->GetWorldPosB()));
        float radius = constraintR->GetMaxDistance();
        rage::Vector3 worldPosB2A = worldPosA - worldPosB;
        worldPosB2A.Normalize();
        rage::Vector3 constraintRErrorPos = worldPosA - radius*worldPosB2A;
        m_character->bspyDrawPoint(worldPosB, 0.05f, rage::Vector3(0,1,0));//green - point on gun
        m_character->bspyDrawLine(constraintRErrorPos, worldPosA, rage::Vector3(0,1,0));//constraint length
        m_character->bspyDrawLine(constraintRErrorPos, worldPosB, rage::Vector3(1,0,0));//constraint error
        m_character->bspyDrawPoint(worldPosA, 0.05f, rage::Vector3(1,0,0));//red - point on hand
#endif
      }
#endif
      if (constraintBaseR->GetType() == rage::phConstraintBase::SPHERICAL) //spherical;
      {
        rage::phConstraintSpherical* constraintR = static_cast<rage::phConstraintSpherical*>( PHCONSTRAINT->GetTemporaryPointer(m_constraintR) );
        rage::phInst *pInstA = NULL;
        rage::phInst *pInstB = NULL;
        pInstA = constraintR->GetInstanceA();
        pInstB = constraintR->GetInstanceB();
        if (pInstA)
          bspyTaskVar(constraintR->GetInstanceA()->GetLevelIndex(), false);
        if (pInstB)
          bspyTaskVar(constraintR->GetInstanceB()->GetLevelIndex(), false);
        bspyTaskVar(constraintR->GetComponentA(), false);
        bspyTaskVar(constraintR->GetComponentB(), false);
        bspyTaskVar(VEC3V_TO_VECTOR3(constraintR->GetWorldPosA()), false);
        bspyTaskVar(VEC3V_TO_VECTOR3(constraintR->GetWorldPosB()), false);
        bspyTaskVar(constraintR->GetTypeName(), false);
        bspyTaskVar(constraintR->IsBroken(), false);
#if drawConstraints
        rage::Vector3 worldPosAL(VEC3V_TO_VECTOR3(constraintR->GetWorldPosA()));
        rage::Vector3 worldPosBL(VEC3V_TO_VECTOR3(constraintR->GetWorldPosB()));
        // draw a cross representing the radius of the constraint.
        m_character->bspyDrawPoint(leftTargetWorld, 0.05f, rage::Vector3(0,1,0));//green - point on gun
        m_character->bspyDrawLine(leftTargetWorld, worldPosBL, rage::Vector3(0,1,0));//constraint length to go
        m_character->bspyDrawPoint(worldPosBL, 0.05f, rage::Vector3(0,0,1));//blue - current constraint target
        m_character->bspyDrawLine(worldPosAL, worldPosBL, rage::Vector3(1,0,0));//red error in constraint
        m_character->bspyDrawPoint(worldPosAL, 0.05f, rage::Vector3(1,0,0));//red - point on hand
#endif
      }//spherical
    }//constraintR exists
  }
#endif // ART_ENABLE_BSPY
}
