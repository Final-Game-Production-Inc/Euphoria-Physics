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
 * Bracing for an impact, specifically from a slow moving car.
 * The character is expected to be upright and in a roughly balanced position. 
 * He will turn to face the object that he is to brace against. 
 * He will take corrective steps if he unbalances and will also catch his fall if he falls over.
 * The character will crouch slightly and lean into the impact, 
 * and if the impact object starts to move away, 
 * the character will righten itself and its upper body will assume the zero pose.
 */


#include "NmRsInclude.h"
#include "NmRsCBU_BraceForImpact.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_BodyBalance.h"
#include "NmRsCBU_Catchfall.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_Grab.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_Pedal.h"
#include "NmRsCBU_RollDownStairs.h"
#include "NmRsCBU_Shot.h"
#include "NmRsCBU_SpineTwist.h"

namespace ART
{
  NmRsCBUBraceForImpact::NmRsCBUBraceForImpact(ART::MemoryManager* services) : CBUTaskBase(services, bvid_braceForImpact)
  {
    initialiseCustomVariables();
  }

  NmRsCBUBraceForImpact::~NmRsCBUBraceForImpact()
  {
  }

  void NmRsCBUBraceForImpact::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
    //Variables to be initialized on behaviour construction and when behaviour deactivates.
    //e.g.for variables that are set during dInvoke so cannot be initialized in onActivate.
    m_carInstGenID = -1;
  }

  void NmRsCBUBraceForImpact::onActivate()
  {
    Assert(m_character);

    m_snap1stImpact = false;
    m_snap1stImpactTorso = false;
    m_snap1stImpactGround = false;

    m_body->resetEffectors(kResetCalibrations);
    m_body->setOpposeGravity(1.0f);

    m_character->instanceToWorldSpace(&m_target, m_parameters.pos, m_parameters.instanceIndex);
    m_doBrace = false;
    m_braceTime = -100.0f;
    m_lookAtHands = false;
    m_spineLean1 = 0.f;
    m_spineLean2 = 0.f;
    m_distanceToTarget = 100.f;

    m_toggleHeadLookTimer = 0.0f;//force random look at hands target or headlook

    m_leftHandPos.Zero();
    m_rightHandPos.Zero();

    m_leftHandGrabPos.Zero();
    m_rightHandGrabPos.Zero();
    m_backwardsBraceTimer = 0.f;


    m_turnDirection.Set(0.f, 0.f, 0.f);
    m_targetVel.Set(0.f, 0.f, 0.f);

    m_legStiffOld = m_parameters.legStiffness;

    m_balanceFailHandled = false;

    m_braceLeft = false;
    m_braceRight = false;

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    dynamicBalancerTask->activate();


    // do this after activate because the dynamicBalancer resets stiffness to default of 12 at activate.
    // In Bspy it will look like A_BraceForImpact set the leg muscles which is fair enough I guess
    //NB. Not integrated with balancerCollisionsReaction leg muscle parameters
    dynamicBalancerTask->setLeftLegStiffness(m_parameters.legStiffness);
    dynamicBalancerTask->setRightLegStiffness(m_parameters.legStiffness);
    if (dynamicBalancerTask->isActive())
    {
      dynamicBalancerTask->setOpposeGravityAnkles(1.f);
      dynamicBalancerTask->setOpposeGravityLegs(1.f);
    }



    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    headLookTask->updateBehaviourMessage(NULL);
    headLookTask->m_parameters.m_alwaysLook = true;
    headLookTask->activate();

	m_kneeBend = 0.0f;

    m_delayLeftHand = false;
    m_handsDelay = 0.f;

    //Randomize values
    //Hand offset from target. Also re-randomized after standing up
    if (m_character->getBodyIdentifier() == gtaWilma)
    {
      //hands together to little more than shoulder width
      m_leftHandSeparation = m_character->getRandom().GetRanged(0.075f, 0.23f);
      m_rightHandSeparation = m_character->getRandom().GetRanged(0.075f, 0.23f);
    }
    else //(m_character->getBodyIdentifier() == gtaFred || m_character->getBodyIdentifier() == rdrCowboy )
    {
      //little less than shoulder width to wide
      m_leftHandSeparation = m_character->getRandom().GetRanged(0.17f, 0.40f);
      m_rightHandSeparation = m_character->getRandom().GetRanged(0.17f, 0.40f);
    } 
    //Randomized here only
    m_kneeBendBrace = m_character->getRandom().GetRanged(0.02f, 0.1f);//0 = no knee bend
    m_spineShape = m_character->getRandom().GetRanged(-0.5f, 0.5f); //spineShape = (-0.5,0.0,0.5) -> (Double spine2/3 bend , same bend, double spine0/1 bend) 

    m_spineBendMult = m_character->getRandom().GetRanged(0.2f, 0.6f);//0 look straight but ok, 1 very bent but esp good head hit bonnet from behind
    if (m_kneeBendBrace < 0.05f)
      m_spineBendMult = m_character->getRandom().GetRanged(0.2f, 0.35f);
    m_hasGrabbed = false;
    m_onCarCalculated = false;
    m_carVel.Zero();
    m_carSpine3Pos.Zero();
    m_carPelvisPos.Zero();
    m_moveAwayDir.Zero();

    //initialized to this so that spineBend and kneeBend are initially applied
    // as MoveAwayFromCar() which sets the m_zone is called at end of BeforeImpact after kneeBend and spineBend are set   
    m_zone = toFront;
    //Below only really to give heads up for glancing spin friction
    GetCarData();
  }

  void NmRsCBUBraceForImpact::onDeactivate()
  {
    Assert(m_character);

    //De-activate subTasks
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    if (dynamicBalancerTask->isActive() && m_character->noBehavioursUsingDynBalance())
    {
      rage::Vector3 noTurn(0.f, 0.f, 0.f);
      dynamicBalancerTask->useCustomTurnDir(false, noTurn);

      dynamicBalancerTask->setLegStraightnessModifier(0.0f);

      dynamicBalancerTask->requestDeactivate();
    }


    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    headLookTask->deactivate();

    NmRsCBUSpineTwist* spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
    Assert(spineTwistTask);
    spineTwistTask->deactivate();

    if (m_balanceFailHandled)
    {
      NmRsCBUGrab* grabTask = (NmRsCBUGrab*)m_cbuParent->m_tasks[bvid_grab];
      Assert(grabTask);
      grabTask->deactivate();

      NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
      Assert(catchFallTask);
      catchFallTask->deactivate();

      NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
      Assert(rdsTask);
      rdsTask->deactivate();

      NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
      Assert(pedalTask);
      pedalTask->deactivate();
    }
    m_character->resetVehicleHit();
    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUBraceForImpact::onTick(float timeStep)
  { 

    m_onCarCalculated = false;
    GetCarData();

    // run BeforeImpact until balance fails then exit and run CatchFall
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    if ((dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK))
    {
      OnFallen();
    }
    else
    {
      BeforeImpact(timeStep);
    }

    if (m_parameters.snapImpacts)
      ExaggerateImpacts();
    if (m_parameters.dampVel)
      DampenUpwardVelocityAndAngularVelocity();

//Draw the car
#if ART_ENABLE_BSPY && BraceBSpyDraw
    if (m_carExists)
    {
      rage::Vector3 col(0.0f, 1.0f, 0.0f);//green - not bracing
      if ((dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK))
      {
        //Draw car bounding box
        col.Set(0.0f, 0.5f, 0.0f);//dark green - Falling:not onCar
        if (OnCar())
          col.Set(0.0f, 0.0f, 0.5f);//dark blue - Falling:onCar
      }
      else
      {
        if (m_doBrace)
          col.Set(1.0f, 0.0f, 0.5f);//red - Bracing
      }
      m_character->bspyDrawLine(m_corner1,m_corner2, col);
      m_character->bspyDrawLine(m_corner2,m_corner3, col);
      m_character->bspyDrawLine(m_corner3,m_corner4, col);
      m_character->bspyDrawLine(m_corner4,m_corner1, col);
    }

#endif
    return eCBUTaskComplete;
  }

  void NmRsCBUBraceForImpact::DampenUpwardVelocityAndAngularVelocity()
  {
    if (OnCar())
    {
      NmRsGenericPart* pelvis = getSpine()->getPelvisPart();
      if (m_character->m_COMrotvelMag > m_parameters.dampSpinThresh)
      {
        rage::Matrix34 tmCom = m_character->m_COMTM;
        rage::Vector3 bodyRight = tmCom.a;
        rage::Vector3 bodyBack = tmCom.c;

        rage::Vector3 comAngMom = m_character->m_angMom/8.7f;
        if (m_character->getBodyIdentifier() == gtaWilma)
          comAngMom *= 1.26f;
        float somVelXS = comAngMom.Dot(bodyRight);
        float tiltVelXS = comAngMom.Dot(bodyBack);
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", comAngMom.Mag()/8.7f);
        bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", somVelXS);
        bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", tiltVelXS);
#endif

        rage::Vector3 torque;
        float scale = rage::Clamp(60.f*m_character->getLastKnownUpdateStep(), 1.0f, 60.0f);//clamp to keep torque same above 60fps and reduce torque from 60fps to 1fps
        Assert(scale > 0.f);
        if (rage::Abs(somVelXS) > m_parameters.dampSpinThresh)
        {
          somVelXS += rage::Selectf(somVelXS, -m_parameters.dampSpinThresh, m_parameters.dampSpinThresh);
          torque = bodyRight;
          //only apply more damping than the character has already given on this axis
          float somersaultDampingXS = rage::Abs(somVelXS * m_parameters.dampSpin) - rage::Abs(m_character->m_spinDamping.somersaultDamping);
          if (somersaultDampingXS > 0.0f)
          {
            somersaultDampingXS = -rage::Selectf(somVelXS * m_parameters.dampSpin, somersaultDampingXS, -somersaultDampingXS);
            pelvis->applyTorque(somersaultDampingXS * torque / scale);
#if ART_ENABLE_BSPY
            bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", somersaultDampingXS);
#endif
          }
        }
        if (rage::Abs(tiltVelXS) > m_parameters.dampSpinThresh)
        {
          tiltVelXS += rage::Selectf(tiltVelXS > 0.0f, -m_parameters.dampSpinThresh, m_parameters.dampSpinThresh);
          torque = bodyBack;
          //only apply more damping than the character has already given on this axis
          float cartwheelDampingXS = rage::Abs(tiltVelXS * m_parameters.dampSpin) - rage::Abs(m_character->m_spinDamping.cartwheelDamping);
          if (cartwheelDampingXS > 0.0f)
          {
            cartwheelDampingXS = -rage::Selectf(tiltVelXS * m_parameters.dampSpin, cartwheelDampingXS, -cartwheelDampingXS);
            pelvis->applyTorque(cartwheelDampingXS * torque / scale);
#if ART_ENABLE_BSPY
            bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", cartwheelDampingXS);
#endif
          }
        }
      }
      
      //Damp the upward velocity
      if (m_character->m_COMvel.z > m_parameters.dampUpVelThresh)
      {
        pelvis->applyImpulse(-(m_character->m_COMvel.z - m_parameters.dampUpVelThresh)*m_parameters.dampUpVel*m_character->m_gUp,pelvis->getPosition());
      }

    }//OnCar
  }
  void NmRsCBUBraceForImpact::ExaggerateImpacts()
  {
    rage::Vector3 spine3Vel(0,0,0);
    rage::Vector3 pelvisVel(0,0,0);
    if (!m_snap1stImpactTorso)
    {
      rage::Vector3 partPos;
      m_character->instanceToLocalSpace(&partPos, getSpine()->getSpine3Part()->getPosition(), m_parameters.instanceIndex);//checks whether IsInLevel(instanceIndex)
      if (!m_carSpine3Pos.IsZero())
        spine3Vel = (partPos - m_carSpine3Pos)/m_character->getLastKnownUpdateStep();
      m_carSpine3Pos = partPos;
      m_character->instanceToLocalSpace(&partPos, getSpine()->getPelvisPart()->getPosition(), m_parameters.instanceIndex);//checks whether IsInLevel(instanceIndex)
      if (!m_carPelvisPos.IsZero())
        pelvisVel = (partPos - m_carPelvisPos)/m_character->getLastKnownUpdateStep();
      m_carPelvisPos = partPos;

    }

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    NmRsCBUShot* shotTask = (NmRsCBUShot*)m_cbuParent->m_tasks[bvid_shot];
    Assert(shotTask);
    //Reduce the snap the looser the character is (to keep snap effect the same and keep the character stable)
    float snapLoosenessMult = 1.0f;
    //10% snap if character very loose (strengthScale = 0.0) then linearly up to 100% snap as looseness drops to 0 (strengthScale = 1.0) 
    if (shotTask->isActive())
      snapLoosenessMult = 0.1f + 0.9f*shotTask->getStiffnessStrengthScale();

     if (OnCar() && !m_snap1stImpact)
    {
      bool collidedWithCar = false;
      rage::Vector3 pos;
      rage::Vector3 normal;
      float depth = 0;
      rage::phInst *collisionInst = NULL;
      if (getRightLeg()->getShin()->collidedWithEnvironment())
      {
        getRightLeg()->getShin()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
      }
      if (getRightLeg()->getThigh()->collidedWithEnvironment())
      {
        getRightLeg()->getThigh()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
      }
      if (getLeftLeg()->getShin()->collidedWithEnvironment())
      {
        getLeftLeg()->getShin()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
      }
      if (getLeftLeg()->getThigh()->collidedWithEnvironment())
      {
        getLeftLeg()->getThigh()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
      }
      if (getSpine()->getPelvisPart()->collidedWithEnvironment())
      {
        getSpine()->getPelvisPart()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
      }
      if (collidedWithCar)
      {
        //Reduce the snap dependent on car speed 0:2 10% snap, linear to 2:6 10%:100% snap
        float carVelMag = 0.25f*(rage::Clamp(m_carVel.Mag(), 2.0f, 6.0f) - 2.0f);// 0:1
        float snapLoosenessMult1st = 0.1f + 0.9f*carVelMag;
        m_snap1stImpact = true;
          m_character->snap(
            m_parameters.snapImpact*snapLoosenessMult*snapLoosenessMult1st,
            0.0f, 
            2,
            true,
            true,
            true, 
            true, 
            true,  
            false, 
            true, 
            true,
            1.f,
            -1,
            &normal);
      }
    }
    if (OnCar() && m_snap1stImpact && !m_snap1stImpactTorso && !m_snap1stImpactGround)
    {
      bool collidedWithCar = false;
      rage::Vector3 pos;
      rage::Vector3 normal;
      float depth = 0;
      rage::phInst *collisionInst = NULL;
      if (getLeftArm()->getClaviclePart()->collidedWithEnvironment())
      {
        getLeftArm()->getClaviclePart()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
      }
      if (getRightArm()->getClaviclePart()->collidedWithEnvironment())
      {
        getRightArm()->getClaviclePart()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
      }
      if (getSpine()->getSpine0Part()->collidedWithEnvironment())
      {
        getSpine()->getSpine0Part()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
      }
      if (getSpine()->getSpine1Part()->collidedWithEnvironment())
      {
        getSpine()->getSpine1Part()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
      }
      if (getSpine()->getSpine2Part()->collidedWithEnvironment())
      {
        getSpine()->getSpine2Part()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
      }
      if (getSpine()->getSpine3Part()->collidedWithEnvironment())
      {
        getSpine()->getSpine3Part()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
      }

      if (collidedWithCar)
      {
        //Reduce the snap dependent on car speed 0:2 10% snap, linear to 2:6 10%:100% snap
        float carVelMag = 0.25f*(rage::Clamp(m_carVel.Mag(), 2.0f, 6.0f) - 2.0f);// 0:1
        //Unless the impact speed is large then ignore car speed
        //float impactVelMag = (getSpine()->getSpine2Part()->getLinearVelocity() - m_carVel).Dot(normal);
        float impactVelMag3 = -spine3Vel.Dot(normal);
        float impactVelMagH = -pelvisVel.Dot(normal);
        float impactVelMag = rage::Max(impactVelMag3, impactVelMagH); 
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", impactVelMag3);
        bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", impactVelMagH);
        bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", normal);
#endif
        impactVelMag = 0.25f*rage::Clamp(impactVelMag, 0.0f, 4.0f);// 0:1
        float snapLoosenessMultTorso = impactVelMag*carVelMag;
        if (impactVelMag > 2.0f)
          snapLoosenessMultTorso = 1.0f;
        m_snap1stImpactTorso = true;
        m_character->snap(
          m_parameters.snapBonnet*snapLoosenessMult*snapLoosenessMultTorso,
          0.0f, 
          2,
          true,
          true,
          true, 
          true, 
          true,  
          true, 
          true, 
          true,
          1.f,
          -1,
          &normal);

      }
    }
    if (!OnCar() && m_snap1stImpact && !m_snap1stImpactGround && m_character->m_COMvelMag > 4.1f && m_character->m_COMvel.z < -1.7f && (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK))
    {
      bool collidedWithCar = false;
      bool collidedWithGround = false;
      rage::Vector3 pos;
      rage::Vector3 normal;
      float depth = 0;
      rage::phInst *collisionInst = NULL;
      if (getLeftArm()->getUpperArm()->collidedWithEnvironment())
      {
        getLeftArm()->getUpperArm()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
        collidedWithGround = true;
      }
      if (getRightArm()->getUpperArm()->collidedWithEnvironment())
      {
        getRightArm()->getUpperArm()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
        collidedWithGround = true;
      }      if (getLeftArm()->getClaviclePart()->collidedWithEnvironment())
      {
        getLeftArm()->getClaviclePart()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
        collidedWithGround = true;
      }
      if (getRightArm()->getClaviclePart()->collidedWithEnvironment())
      {
        getRightArm()->getClaviclePart()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
        collidedWithGround = true;
      }
      if (getSpine()->getSpine0Part()->collidedWithEnvironment())
      {
        getSpine()->getSpine0Part()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
        collidedWithGround = true;
      }
      if (getSpine()->getSpine1Part()->collidedWithEnvironment())
      {
        getSpine()->getSpine1Part()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
        collidedWithGround = true;
      }
      if (getSpine()->getSpine2Part()->collidedWithEnvironment())
      {
        getSpine()->getSpine2Part()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
        collidedWithGround = true;
      }
      if (getSpine()->getSpine3Part()->collidedWithEnvironment())
      {
        getSpine()->getSpine3Part()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
        collidedWithGround = true;
      }
      if (getSpine()->getPelvisPart()->collidedWithEnvironment())
      {
        getSpine()->getPelvisPart()->getCollisionZMPWithEnvironment(pos, normal,&depth,&collisionInst);
        if (m_character->IsInstValid_NoGenIDCheck(collisionInst) && (collisionInst->GetLevelIndex()==m_parameters.instanceIndex))
          collidedWithCar = true;
        collidedWithGround = true;
      }

      if (!collidedWithCar && collidedWithGround)
      {
        //rage::Vector3 velOfCOMOnCar(0.0f, 0.0f, 0.0f);
        //Get velocity of com considered as a fixed point on the car.
        // instance exists if OnCar = true.
        //m_character->getVelocityOnInstance(m_parameters.instanceIndex, m_character->m_COM, &velOfCOMOnCar);
        //velOfCOMOnCar -= getSpine()->getSpine3Part()->getLinearVelocity();
        m_snap1stImpactGround = true;
        m_character->snap(
          m_parameters.snapFloor*snapLoosenessMult,
          0.0f, 
          2,
          true,
          true,
          true, 
          true, 
          true,  
          true, 
          true, 
          true,
          1.f,
          -1,
          &normal);

      }
    }
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", spine3Vel);
    bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", pelvisVel);
#endif
  }

  void NmRsCBUBraceForImpact::OnFallen()
  {
    NmRsCBUGrab* grabTask = (NmRsCBUGrab*)m_cbuParent->m_tasks[bvid_grab];
    Assert(grabTask);

    if (grabTask->getGrabing())
    {
      NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
      Assert(pedalTask);
      pedalTask->updateBehaviourMessage(NULL); // initialise params

      pedalTask->m_parameters.randomSeed=(100);
      pedalTask->m_parameters.pedalOffset=(0.02f);
      pedalTask->m_parameters.pedalLeftLeg=(true);
      pedalTask->m_parameters.pedalRightLeg=(true);
      pedalTask->m_parameters.speedAsymmetry=(4.0f);
      pedalTask->m_parameters.radius=(0.3f);
      pedalTask->m_parameters.legStiffness=(rage::Clamp(m_parameters.bodyStiffness - 1.f,1.f,20.f));
      pedalTask->m_parameters.backPedal=(false);
      pedalTask->m_parameters.adaptivePedal4Dragging=(true);
      pedalTask->m_parameters.angSpeedMultiplier4Dragging=(0.3f);
      pedalTask->activate();
    }
    else
    {
      NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
      Assert(pedalTask);
      pedalTask->deactivate();
      if (m_balanceFailHandled)
      {  // delays for a frame
        NmRsCBUGrab* grabTask = (NmRsCBUGrab*)m_cbuParent->m_tasks[bvid_grab];
        Assert(grabTask);
        grabTask->deactivate();}
    }

    NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
    Assert(rdsTask);
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    if (!m_balanceFailHandled)
    {
      m_balanceFailHandled = true;
      rage::Vector3 noTurn(0.f, 0.f, 0.f);
      dynamicBalancerTask->useCustomTurnDir(false, noTurn);
      dynamicBalancerTask->setLegStraightnessModifier(0.0f);
      if (dynamicBalancerTask->isActive() && m_character->noBehavioursUsingDynBalance())
        dynamicBalancerTask->requestDeactivate();

      NmRsCBUSpineTwist* spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
      Assert(spineTwistTask);
      spineTwistTask->deactivate();

      NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      Assert(headLookTask);
      headLookTask->deactivate();

      bool collidedWithWorldUpper = m_character->hasCollidedWithWorld(bvmask_UpperBody);
      bool chooseBailOutReaction = m_character->getRandom().GetBool();
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "braceForImpact", collidedWithWorldUpper);
      bspyScratchpad(m_character->getBSpyID(), "braceForImpact", chooseBailOutReaction);
#endif

      if (!collidedWithWorldUpper)
      {
        bool footCollided = getLeftLeg()->getFoot()->collidedWithNotOwnCharacter();
        footCollided |= getRightLeg()->getFoot()->collidedWithNotOwnCharacter();
        //mmmmtodo check shins not colliding with floor only
        bool legCollided = getLeftLeg()->getShin()->collidedWithNotOwnCharacter();
        legCollided |= getRightLeg()->getShin()->collidedWithNotOwnCharacter();
        legCollided |= getLeftLeg()->getThigh()->collidedWithNotOwnCharacter();
        legCollided |= getRightLeg()->getThigh()->collidedWithNotOwnCharacter();
        legCollided |= getSpine()->getPelvisPart()->collidedWithNotOwnCharacter();
        if (footCollided && (!legCollided))
          chooseBailOutReaction = false; // do the catchFall if we where standing and not touching anything
      }

      if (chooseBailOutReaction)
      {
        rdsTask->updateBehaviourMessage(NULL); // sets values to defaults

        rdsTask->m_parameters.m_AsymmetricalLegs = 0.4f;
        float rollStiff = rage::Clamp(m_parameters.bodyStiffness-3.0f,1.0f,15.0f);
        rdsTask->m_parameters.m_Stiffness = rollStiff;
        rdsTask->m_parameters.m_ForceMag = 0.4f;
        rdsTask->m_parameters.m_UseArmsToSlowDown = -0.9f;

        rdsTask->m_parameters.m_ArmReachAmount = 1.4f;
        rdsTask->m_parameters.m_SpinWhenInAir = true;
        rdsTask->m_parameters.m_LegPush = 0.2f;
        rdsTask->m_parameters.m_ArmL = 0.6f;

        float legAssmetry = m_character->getRandom().GetRanged(0.2f, 0.8f);
        rdsTask->m_parameters.m_AsymmetricalLegs = legAssmetry;
        rdsTask->m_parameters.m_useVelocityOfObjectBelow = true;
        rdsTask->m_parameters.m_useRelativeVelocity = false;//To make impact with cars the same as before relative velocity was introduced

        rdsTask->activate();
      }
      else
      {
        NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
        Assert(catchFallTask);
        catchFallTask->updateBehaviourMessage(NULL); // sets values to defaults

        float defaultBodyStiffness = 12.f;
        catchFallTask->m_parameters.m_legsStiffness = m_parameters.bodyStiffness * 6.f/defaultBodyStiffness;
        catchFallTask->m_parameters.m_torsoStiffness = m_parameters.bodyStiffness * 9.f/defaultBodyStiffness;
        catchFallTask->m_parameters.m_armsStiffness = m_parameters.bodyStiffness * 15.f/defaultBodyStiffness;
        catchFallTask->activate();
      }
    }

    //Change rollDownStairs helper torques if on top of the car 
    if (rdsTask->isActive())
    {
      if (OnCar())
          {
            rage::Vector3 velOfCOMOnCar(0.0f, 0.0f, 0.0f);
            //Get velocity of com considered as a fixed point on the car.
        // instance exists if OnCar = true.
            m_character->getVelocityOnInstance(m_parameters.instanceIndex, m_character->m_COM, &velOfCOMOnCar);

            if (m_parameters.rollType == 0)//original
            {
              //roll off/stay on car:  Roll with character velocity
              rdsTask->m_parameters.m_UseCustomRollDir = false;
              rdsTask->m_parameters.m_useRelativeVelocity = false;
            }
            else if (m_parameters.rollType == 1)
            {
              //Gentle: roll off/stay on car = use relative velocity of character to car to roll against
              rdsTask->m_parameters.m_UseCustomRollDir = true;
              rdsTask->m_parameters.m_CustomRollDir = -(m_character->m_COMvel - velOfCOMOnCar);//-relative velocity of character to car
              rdsTask->m_parameters.m_useRelativeVelocity = true;
            }
            else if (m_parameters.rollType == 2)
            {
              //roll over car:  Roll against character velocity.  i.e. roll against any velocity picked up by hitting car 
              rdsTask->m_parameters.m_UseCustomRollDir = true;
              rdsTask->m_parameters.m_CustomRollDir = -m_character->m_COMvel;
              rdsTask->m_parameters.m_useRelativeVelocity = false;
            }
            else if (m_parameters.rollType == 3)
            {
              //Gentle: roll over car:  use relative velocity of character to car to roll with
              rdsTask->m_parameters.m_UseCustomRollDir = true;
              rdsTask->m_parameters.m_CustomRollDir = m_character->m_COMvel - velOfCOMOnCar;//relative velocity of character to car
              rdsTask->m_parameters.m_useRelativeVelocity = true;
            }
          }
          else//not on car
          {
            //roll on floor:  Roll with character velocity
            rdsTask->m_parameters.m_UseCustomRollDir = false;
            rdsTask->m_parameters.m_useRelativeVelocity = false;
          }
      }//if car exists
      else//revert to default rollDownsStairs behaviour if the car has ceased to exist
      {
        //roll on floor:  Roll with character velocity
        rdsTask->m_parameters.m_UseCustomRollDir = false;
        rdsTask->m_parameters.m_useRelativeVelocity = false;
      }

    // ideally we need to make Grab time-out-able so we can increase the constraint strength
    // but still have the ped fall off after 1 or 2 seconds

    if (!m_hasGrabbed)
    {  
      // only try to grab once - on the frame that the balancer fails
      // work out the velocity of the car
      rage::Vector3 carVel(m_targetVel);//m_targetVel = velocity of target if car inst doesn't exist or has -1 for intanceIndex
      if (m_carExists)
      {
        rage::Vector3 carPos(0.0f,0.0f,0.0f);
        rage::Vector3 carPosLocal(0.0f,0.0f,0.0f);
        //We could get the velocity of the collision with each hand (which would take into account spinning of the car?
        //For now just get the Velocity of Instance - 
        m_character->instanceToWorldSpace(&carPos, carPosLocal, m_parameters.instanceIndex);//checks whether IsInLevel(instanceIndex)
        m_character->getVelocityOnInstance(m_parameters.instanceIndex,carPos,&carVel);
      }
      float velM = carVel.Mag();
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "And the Velocity is", velM);
#endif

      bool rightConnectedWithCar = false;
      bool handCollided = getRightArm()->getHand()->collidedWithEnvironment();
      if (handCollided || getRightArm()->getLowerArm()->collidedWithEnvironment())
      {
        rage::Vector3 pos;
        float depth = 0;
        rage::phInst *collisionInst = NULL;
        int collisionInstGenID = -1;
        if (handCollided)
          getRightArm()->getHand()->getCollisionZMPWithEnvironment(pos, m_rightHandNorm,&depth,&collisionInst, &collisionInstGenID);
        else
          getRightArm()->getLowerArm()->getCollisionZMPWithEnvironment(pos, m_rightHandNorm,&depth,&collisionInst, &collisionInstGenID);

        m_rightHandGrabPos = pos;

        if (m_character->IsInstValid(collisionInst, collisionInstGenID))
        {
          int collisionObjectLevelIndex = collisionInst->GetLevelIndex();
          if (collisionObjectLevelIndex==m_parameters.instanceIndex)
            rightConnectedWithCar = true;
          else
            rightConnectedWithCar = false;
        }
      }

      bool leftConnectedWithCar = false;
      handCollided = getLeftArm()->getHand()->collidedWithEnvironment();
      if (handCollided || getLeftArm()->getLowerArm()->collidedWithEnvironment())
      {
        rage::Vector3 pos;
        float depth = 0;
        rage::phInst *collisionInst = NULL;
        int collisionInstGenID = -1;
        if (handCollided)
          getLeftArm()->getHand()->getCollisionZMPWithEnvironment(pos, m_leftHandNorm,&depth,&collisionInst, &collisionInstGenID);
        else
          getLeftArm()->getLowerArm()->getCollisionZMPWithEnvironment(pos, m_leftHandNorm,&depth,&collisionInst, &collisionInstGenID);

        m_leftHandGrabPos = pos;

        if (m_character->IsInstValid(collisionInst, collisionInstGenID))
        {
          int collisionObjectLevelIndex = collisionInst->GetLevelIndex();
          if (collisionObjectLevelIndex==m_parameters.instanceIndex)
            leftConnectedWithCar = true;
          else
            leftConnectedWithCar = false;
        }
      }


      if ((leftConnectedWithCar || rightConnectedWithCar)&& (velM<m_parameters.maxGrabCarVelocity)&&(!grabTask->isActive()))
      {
        NmRsCBUGrab* grabTask = (NmRsCBUGrab*)m_cbuParent->m_tasks[bvid_grab];
        Assert(grabTask);
        grabTask->updateBehaviourMessage(NULL); // set to params defaults

        m_hasGrabbed = true;

        rage::Vector3 temp;
        grabTask->m_parameters.instanceIndex = m_parameters.instanceIndex;

        if (leftConnectedWithCar)
        {
#if ART_ENABLE_BSPY && BraceBSpyDraw
          m_character->bspyDrawPoint(m_leftHandGrabPos, 0.1f, rage::Vector3(1.0f,0.0f,0.0f));
#endif
          m_character->instanceToLocalSpace(&temp, m_leftHandGrabPos, m_parameters.instanceIndex);
          grabTask->m_parameters.pos1 = temp;
          grabTask->m_parameters.normalL = m_leftHandNorm;
        }

        if (rightConnectedWithCar)
        {
#if ART_ENABLE_BSPY && BraceBSpyDraw
          m_character->bspyDrawPoint(m_rightHandGrabPos, 0.1f, rage::Vector3(1.0f,0.0f,0.0f));
#endif

          m_character->instanceToLocalSpace(&temp, m_rightHandGrabPos, m_parameters.instanceIndex);
          grabTask->m_parameters.pos = temp;
          grabTask->m_parameters.normalR = m_rightHandNorm;
        }

#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", rightConnectedWithCar);
        bspyScratchpad(m_character->getBSpyID(), "BraceForImpact", leftConnectedWithCar);
#endif

        grabTask->m_parameters.dontLetGo=(m_parameters.grabDontLetGo); //false
        grabTask->m_parameters.grabStrength = m_parameters.grabStrength; //105
        grabTask->m_parameters.grabDistance=(m_parameters.grabDistance);//1
        grabTask->m_parameters.reachAngle=(m_parameters.grabReachAngle);//1.5
        grabTask->m_parameters.grabHoldMaxTimer = m_parameters.grabHoldTimer;//3.5

        grabTask->m_parameters.useLineGrab=(false);
        grabTask->m_parameters.useLeft = leftConnectedWithCar;
        grabTask->m_parameters.useRight = rightConnectedWithCar;
        grabTask->activate();
      }//if ((leftConnectedWithCar || rightConnectedWithCar)&& (velM<m_parameters.maxGrabCarVelocity)&&(!grabTask->isActive()))
    }//if (!m_hasGrabbed)
  }

  void NmRsCBUBraceForImpact::BeforeImpact(float timeStep)
  {
    //Work out world target m_target and m_targetVel
    rage::Vector3 newTarget;
    m_character->instanceToWorldSpace(&newTarget, m_parameters.pos, m_parameters.instanceIndex);
    //Get velocity of that point on instance if it exists.  Gives better velocity.
    //or
    //get target velocity by looking at current target and old target
    if (m_carExists && m_parameters.instanceIndex != -1) 
    {
      m_character->getVelocityOnInstance(m_parameters.instanceIndex,newTarget,&m_targetVel);
    }
    else
    {
      m_targetVel.Subtract(newTarget, m_target);
      m_targetVel *= 1.f/timeStep;
    }
    float mag = m_targetVel.Mag();
    if (mag > 10.f)
      m_targetVel *= 10.f/mag;
    m_target = newTarget;

    //DecideToBrace uses it's own metric for bracing unless overrideBraceDecision
    //  is not NULL in which case it uses that value
    bool* overrideBraceDecision = NULL;
    bool overrideBD = false;
    if (m_parameters.newBrace && m_carExists)
    {
      overrideBD = ShouldBrace();
      overrideBraceDecision = &overrideBD;
    }
    bool allowTimedBrace = (m_zone==toFront || m_zone==toRear);

    m_character->DecideToBrace(
      timeStep,
      m_target,
      m_targetVel,
      m_parameters.braceDistance,
      m_parameters.targetPredictionTime,
      m_parameters.minBraceTime,
      m_parameters.timeToBackwardsBrace,
      m_distanceToTarget,
      m_braceTime,
      m_backwardsBraceTimer,
      m_shouldBrace,
      m_doBrace,
      overrideBraceDecision,
      allowTimedBrace);
    m_character->DecideBraceHands(
      timeStep,
      m_target,
      m_doBrace,
      m_braceLeft,
      m_braceRight,
      m_braceTime,
      m_handsDelay,
      m_parameters.handsDelayMin,
      m_parameters.handsDelayMax,
      m_delayLeftHand,
      m_leftHandSeparation,
      m_rightHandSeparation);
    if (m_doBrace)
      m_character->ArmsBrace(
        m_target,
        m_targetVel,
        m_parameters.reachAbsorbtionTime,
        m_parameters.braceDistance,
        m_parameters.bodyStiffness,
        m_braceLeft,
        m_braceRight,
        m_leftHandSeparation,
        m_rightHandSeparation,
        m_body,
        m_leftHandPos, 
        m_rightHandPos);
    //Move the arms if they haven't braced and we're not using bodyBalance arms instead
    DefaultArms();//f(m_braceRight, m_braceLeft);
    //Bend hips,spine and legs.  Twist spine and turn balancer
    BendAndTwistCharacter(timeStep);//f(m_doBrace, m_shouldBrace, m_braceLeft, m_braceRight)
    LookAtHandsOrCar(timeStep);//f(m_doBrace, m_braceLeft, m_braceRight)
    MoveAwayFromCar();
  }

  void NmRsCBUBraceForImpact::LookAtHandsOrCar(float timeStep)
  {
    //Headlook
    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);

    m_toggleHeadLookTimer -= timeStep;
    if (m_toggleHeadLookTimer <= 0.0f)
    {
      m_toggleHeadLookTimer = m_character->getRandom().GetRanged(0.25f, 1.5f);
      m_lookAtHands = m_character->getRandom().GetBool();
    }

    rage::Vector3 bodyBack = m_character->m_COMTM.c;
    rage::Vector3 handTargetWorld;
    m_character->instanceToWorldSpace(&handTargetWorld, m_parameters.pos, m_parameters.instanceIndex);
    rage::Vector3 com2HandTarget = handTargetWorld - m_character->m_COM;//i.e. handTarget is temporarily com2Target
    com2HandTarget.Normalize();
    bool lookingBackwards = (bodyBack.Dot(com2HandTarget) > 0.7f);//remove hands headlook target if target is behind ped
    if (m_lookAtHands && m_doBrace && (m_braceLeft || m_braceRight) && (!lookingBackwards))
    {
      //Look slighltly infront of handTarget
      rage::Vector3 originLook = m_parameters.look;
      rage::Vector3 interpHeadLook = 0.17f*originLook;
      interpHeadLook.AddScaled(m_parameters.pos, 1.0f - 0.17f);

      m_character->instanceToWorldSpace(&handTargetWorld, interpHeadLook, m_parameters.instanceIndex);
      headLookTask->m_parameters.m_pos = handTargetWorld;
      headLookTask->m_parameters.m_instanceIndex = -1;
    }
    else
    {
      headLookTask->m_parameters.m_pos = m_parameters.look;
      headLookTask->m_parameters.m_instanceIndex = m_parameters.instanceIndex;
    }
    headLookTask->m_parameters.m_stiffness = m_parameters.bodyStiffness - 1.f;
    headLookTask->m_parameters.m_alwaysLook = true;
  }

  //Bend hips,spine and legs.  Twist spine and turn balancer
  //  Set bend if doing brace otherwise...
  //   automatically unbend - straighten the spine, hips and legs
  void NmRsCBUBraceForImpact::BendAndTwistCharacter(float timeStep)
  {
    NmRsCBUSpineTwist* spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
    Assert(spineTwistTask);
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    if (m_doBrace)
    {
      //SpineTwist if reaching with both
      if (m_braceLeft && m_braceRight)
      {
        //spineTwistTask->setSpineTwistStiffness(m_parameters.bodyStiffness);
        spineTwistTask->setSpineTwistTwistClavicles(false);
        spineTwistTask->setSpineTwistPos(m_target);

        spineTwistTask->setSpineTwistVelX(m_targetVel.x);
        spineTwistTask->setSpineTwistVelY(m_targetVel.y);
        spineTwistTask->setSpineTwistVelZ(m_targetVel.z);
        spineTwistTask->activate();
      }
      else
        //mmmtodo don't deactivate if headlook or other behaviour using it? 
        spineTwistTask->deactivate();

      //Set m_spineLean1 (controls spineLean and hipPitch)
      rage::Vector3 chest;
      chest = getSpine()->getSpine3Part()->getPosition();
      //lean back max for targets 0.5m higher than chest, lean forward max for targets no lower than knees about 1.1 m below chest.
      //Clamping h clamps lean1 of spine when m_spineLean1 is applied later
      float h = rage::Clamp(m_character->vectorHeight(chest) - m_character->vectorHeight(m_target), -0.5f, 1.1f);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(),"braceForImpact.bracing",h);
#endif

      rage::Vector3 forward;
      rage::Vector3 target2COMDirLevelled;
      target2COMDirLevelled.Subtract(m_character->m_COM, m_target);
      m_character->levelVector(target2COMDirLevelled); 
      target2COMDirLevelled.Normalize();
      forward = m_character->m_COMTM.c;
      //mmmmtodo   if target is higher than spine3 behind then leaning forward makes no sense
      //           if target is higher than spine3 from in front then leaning back a little is ok
      float scale = forward.Dot(target2COMDirLevelled); // if contact is behind, then lean back if its low rather than forward
      if (!((!m_shouldBrace) && m_doBrace) && (m_zone == toFront || m_zone == toRear))//standup slowly if this is false
        m_spineLean1 = 0.2f + (h*m_spineBendMult - 0.0f)*scale;//m_spineBendMult [0.2f, 0.6f]
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(),"braceForImpact.bracing",scale);
#endif
      //Set Knee bend and balancer stiffnesses
      if (!((!m_shouldBrace) && m_doBrace) && (m_zone == toFront || m_zone == toRear))//standup slowly if this is false
        m_kneeBend = m_kneeBendBrace;
      if (!(m_parameters.legStiffness==m_legStiffOld))
      {
        dynamicBalancerTask->setLeftLegStiffness(m_parameters.legStiffness);
        dynamicBalancerTask->setRightLegStiffness(m_parameters.legStiffness);
        if (dynamicBalancerTask->isActive())
        {
          m_legStiffOld = m_parameters.legStiffness;
        }
      }

    }
    else
      //mmmtodo don't deactivate if headlook or other behaviour using it? 
      spineTwistTask->deactivate();

    //Bend the character
    float lean1 = m_spineLean1;
    getSpineInputData()->applySpineLean(lean1*1.5f, m_spineLean2);

    lean1 *= (1.5f / 6.0f);
    // set lean1s //spineShape = (-0.5,0.0,0.5) -> (Double spine2/3 bend , same bend, double spine0/1 bend)
    getSpineInputData()->getSpine0()->setDesiredLean1(lean1 * (1.5f + m_spineShape));
    getSpineInputData()->getSpine1()->setDesiredLean1(lean1 * (1.5f + m_spineShape));
    getSpineInputData()->getSpine2()->setDesiredLean1(lean1 * (1.5f - m_spineShape));
    getSpineInputData()->getSpine3()->setDesiredLean1(lean1 * (1.5f - m_spineShape));

    dynamicBalancerTask->setHipPitch(-m_spineLean1*4.0f/6.0f);
    dynamicBalancerTask->setLegStraightnessModifier(-m_kneeBend);
    
    //Balancer Turn
    m_turnDirection = m_target - m_character->m_COM;
    m_turnDirection.Normalize();
    dynamicBalancerTask->useCustomTurnDir(true, m_turnDirection);

    //Automatically unbend - straighten the spine, hips and legs
    static float spineDec1 = 0.3f;
    static float spineDec2 = 0.1f;
    static float kneeDec1 = 0.053f;
    static float kneeDec2 = 0.02f;
    float spineDec = spineDec1;
    float kneeDec = kneeDec1;
    if ((!m_shouldBrace) && m_doBrace)//standup slowly
    {
      spineDec = spineDec2;
      kneeDec = kneeDec2;
    }
    m_spineLean1 /= (1.f + spineDec*60.f*timeStep); // TDL implicit time independant version of *= 0.95
    m_kneeBend /= (1.f + kneeDec*60.f*timeStep);
  }

  void NmRsCBUBraceForImpact::DefaultArms()
  {
#if ART_ENABLE_BSPY
    m_character->m_currentSubBehaviour = "-DefaultArms"; 
#endif
    rage::Vector3 lean = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
    rage::Vector3 leanVel = getSpine()->getSpine3Part()->getLinearVelocity() - getSpine()->getPelvisPart()->getLinearVelocity();
    rage::Vector3 fall = (lean*0.5f + leanVel*0.3f)*2.f;
    float fallMag = fall.Mag();
    fallMag = rage::Clamp(fallMag - 0.15f, 0.f, 1.f);
    rage::Matrix34 mat;
    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    bool allowBodyBalanceArms = m_parameters.bbArms && bodyBalanceTask->isActive(); 
    // TDL default arm motion here.
    if (!m_braceLeft && (!allowBodyBalanceArms))
    {
      getLeftArm()->setBodyStiffness(getLeftArmInput(), 7.f + 3.f*fallMag, 1.f);
      getLeftArm()->getShoulder()->getMatrix1(mat);
#if ART_ENABLE_BSPY && BraceBSpyDraw
      m_character->bspyDrawCoordinateFrame(0.1f, mat);
#endif
      float fallX = 4.f * fall.Dot(mat.a);
      float fallZ = rage::Clamp(4.f * fall.Dot((mat.c - mat.b)*0.707f), -8.f, 8.f);

      float l1 = -sinf(fallX);
      float l2 = cosf(fallX) + fallZ - 0.5f*fallMag;
      //mmmtodo consider making this blend with zero pose like shot default_default now does
      getLeftArmInputData()->getShoulder()->setOpposeGravity(fallMag);
      getLeftArmInputData()->getClavicle()->setOpposeGravity(fallMag);
      // TDL we can replace the static pose with a zero pose if we need to. Make 1-fallMag the blend factor
      getLeftArmInputData()->getShoulder()->setDesiredAngles(
        (l1 + getLeftArm()->getShoulder()->getMidLean1())*fallMag,
        (l2 + getLeftArm()->getShoulder()->getMidLean2())*fallMag + (0.2f)*(1.f-fallMag), 0.f);
      getLeftArmInputData()->getClavicle()->setDesiredAngles(0.f, (l2 + getLeftArm()->getClavicle()->getMidLean2())*fallMag, 0.f);
      getLeftArmInputData()->getWrist()->setDesiredAngles(0,0,0);
      getLeftArmInputData()->getElbow()->setDesiredAngle(1.f*fallMag + 0.7f*(1.f-fallMag));
      if (m_character->getCharacterConfiguration().m_leftHandState == CharacterConfiguration::eHS_Rifle)//holding something
        getLeftArmInputData()->getWrist()->setStiffness(m_parameters.bodyStiffness,1.5f);
    }
    if (!m_braceRight && (!allowBodyBalanceArms))
    {
      getRightArm()->setBodyStiffness(getRightArmInput(), 7.f + 3.f*fallMag, 1.f);
      getRightArm()->getShoulder()->getMatrix1(mat);
#if ART_ENABLE_BSPY && BraceBSpyDraw
      m_character->bspyDrawCoordinateFrame(0.1f, mat);
#endif
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
      getRightArmInputData()->getWrist()->setDesiredAngles(0,0,0); // change from below to better match left arm code above. possible typo.
      getRightArmInputData()->getElbow()->setDesiredAngle(0.8f*fallMag + 0.8f*(1.f-fallMag));

      if (m_character->getCharacterConfiguration().m_rightHandState == CharacterConfiguration::eHS_Rifle)//holding something
        getRightArmInputData()->getWrist()->setStiffness(m_parameters.bodyStiffness,1.5f);
    }
#if ART_ENABLE_BSPY
    m_character->m_currentSubBehaviour = ""; 
#endif
  }

  void NmRsCBUBraceForImpact::MoveAwayFromCar()
  {
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    if (m_parameters.moveAway && m_carExists && m_doBrace) 
    {
      dynamicBalancerTask->autoLeanForceInDirection(m_moveAwayDir,m_parameters.moveAwayAmount,8);
      dynamicBalancerTask->autoLeanInDirection(m_moveAwayDir,m_parameters.moveAwayLean);
    }
    else
    {
      dynamicBalancerTask->autoLeanForceCancel();
      dynamicBalancerTask->autoLeanCancel();
    }

  }

  bool NmRsCBUBraceForImpact::ShouldBrace()
  {
    bool doBrace = false;

    if(m_carExists && m_character->IsInstValid(m_parameters.instanceIndex, m_carInstGenID))
    {
      //Early out if character is in contact with the car
      if (m_parameters.braceOnImpact)
      {
        rage::Vector3 collisionNormal;//module level
        rage::Vector3 collisionPos;
        float depth = 0;
        rage::phInst *collisionInst = NULL;
        int collisionInstGenID = -1;

#if ART_ENABLE_BSPY
        bool impactOccurred = false;
        if (getLeftLeg()->getThigh()->collidedWithNotOwnCharacter())
        {
          getLeftLeg()->getThigh()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
          if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() == m_parameters.instanceIndex)
            impactOccurred = true;
        }
        if (getRightLeg()->getThigh()->collidedWithNotOwnCharacter())
        {
          getRightLeg()->getThigh()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
          if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() == m_parameters.instanceIndex)
            impactOccurred = true;
        }
        if (getLeftLeg()->getShin()->collidedWithNotOwnCharacter())
        {
          getLeftLeg()->getShin()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
          if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() == m_parameters.instanceIndex)
            impactOccurred = true;
        }
        if (getRightLeg()->getShin()->collidedWithNotOwnCharacter())
        {
          getRightLeg()->getShin()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
          if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() == m_parameters.instanceIndex)
            impactOccurred = true;
        }
        bspyScratchpad(m_character->getBSpyID(), "BraceForImpact.CarVel", impactOccurred);
        if (impactOccurred)
          return true;
#else
        if (getLeftLeg()->getThigh()->collidedWithNotOwnCharacter())
        {
          getLeftLeg()->getThigh()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
          if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() == m_parameters.instanceIndex)
            return true;
        }
        if (getRightLeg()->getThigh()->collidedWithNotOwnCharacter())
        {
          getRightLeg()->getThigh()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
          if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() == m_parameters.instanceIndex)
            return true;
        }
        if (getLeftLeg()->getShin()->collidedWithNotOwnCharacter())
        {
          getLeftLeg()->getShin()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
          if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() == m_parameters.instanceIndex)
            return true;
        }
        if (getRightLeg()->getShin()->collidedWithNotOwnCharacter())
        {
          getRightLeg()->getShin()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
          if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() == m_parameters.instanceIndex)
            return true;
        }

#endif
      }

      rage::phInst* pInst = NULL;
      pInst = m_character->getLevel()->GetInstance(m_parameters.instanceIndex);
      if(pInst)
      {
        rage::Vector3 com2Car = m_target - m_character->m_COM;
        float dist2 = com2Car.Mag();
        com2Car.Normalize();
        rage::Vector3 relVel = m_targetVel-m_character->m_COMvel;
        float speed2 = com2Car.Dot(relVel);
        float predTime = rage::Clamp(-dist2/speed2, 0.f, 0.5f);

        int numPoints = 4;
        rage::Vector3 supportPoints[8];
        supportPoints[0] = m_corner1;
        supportPoints[1] = m_corner2;
        supportPoints[2] = m_corner3;
        supportPoints[3] = m_corner4;
        if (predTime>0.0f)
        {
          //Get predicted car bounding box at impact
          float height = m_corner1.z;
          rage::Vector3 vel1, vel2, vel3, vel4;
          m_character->getVelocityOnInstance(m_parameters.instanceIndex, m_corner1, &vel1);
          m_character->getVelocityOnInstance(m_parameters.instanceIndex, m_corner2, &vel2);
          m_character->getVelocityOnInstance(m_parameters.instanceIndex, m_corner3, &vel3);
          m_character->getVelocityOnInstance(m_parameters.instanceIndex, m_corner4, &vel4);
          rage::Vector3 pred_corner1 = m_corner1 + predTime*(vel1-m_character->m_COMvel);
          rage::Vector3 pred_corner2 = m_corner2 + predTime*(vel2-m_character->m_COMvel);
          rage::Vector3 pred_corner3 = m_corner3 + predTime*(vel3-m_character->m_COMvel);
          rage::Vector3 pred_corner4 = m_corner4 + predTime*(vel4-m_character->m_COMvel);
          m_character->levelVector(pred_corner1, height+0.3f);
          m_character->levelVector(pred_corner2, height+0.3f);
          m_character->levelVector(pred_corner3, height+0.3f);
          m_character->levelVector(pred_corner4, height+0.3f);

          numPoints = 8;
          supportPoints[4] = pred_corner1;
          supportPoints[5] = pred_corner2;
          supportPoints[6] = pred_corner3;
          supportPoints[7] = pred_corner4;
#if ART_ENABLE_BSPY && BraceBSpyDraw
          //Draw predicted car bounding box at impact
          rage::Vector3 col(0.5f, 0.5f,0.0f);
          m_character->bspyDrawLine(pred_corner1,pred_corner2, col);
          m_character->bspyDrawLine(pred_corner2,pred_corner3, col);
          m_character->bspyDrawLine(pred_corner3,pred_corner4, col);
          m_character->bspyDrawLine(pred_corner4,pred_corner1, col);
#endif
        }
        buildConvexHull2D(supportPoints, numPoints, rage::Vector3(0,0,1));
        rage::Vector3 upCopy(m_character->m_gUp),pointCopy(m_character->m_COM);
        m_character->levelVector(pointCopy,0.0f);//mmmmtodo nearestPoint may need to be calculated in 3d remember (see comment below)
        rage::Vector3 nrstPoint;
        float insideResult = getDistanceToPoint(pointCopy, upCopy/*, radius*/, &nrstPoint); //when a member function make const;
        if (insideResult <0.0f)
        {
          doBrace = true;
        }
        doBrace |= dist2 < m_parameters.braceDistance;
        dist2 = (m_target - getSpine()->getSpine3Part()->getPosition()).Mag();
        doBrace |= dist2 < m_parameters.braceDistance;
        float dist3 = (m_target - getLeftArm()->getHand()->getPosition()).Mag();
        if (m_braceLeft && m_doBrace)
          doBrace |= (dist2 < 0.75f && dist3 < m_parameters.braceDistance);
        dist3 = (m_target - getRightArm()->getHand()->getPosition()).Mag();
        if (m_braceRight && m_doBrace)
          doBrace |= (dist2 < 0.75f && dist3 < m_parameters.braceDistance);
#if ART_ENABLE_BSPY && BraceBSpyDraw
        rage::Vector3 col(1.0f, 1.0f, 1.0f);
        if (insideResult <0.0f)
          col.Set(1.0f, 0.0f, 0.0f);
        drawConvexHull(col);
#endif

#if ART_ENABLE_BSPY && BraceBSpyDraw
        m_character->bspyDrawLine(m_target, m_character->m_COM, rage::Vector3(1.0f,1.0f,1.0f));//white
        m_character->bspyDrawLine(m_target, m_target+relVel, rage::Vector3(1.0f,0.0f,0.0f));//white
#endif
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "braceForImpact.CarVel", com2Car);
        bspyScratchpad(m_character->getBSpyID(), "braceForImpact.CarVel", relVel);
        bspyScratchpad(m_character->getBSpyID(), "braceForImpact.CarVel", dist2);
        bspyScratchpad(m_character->getBSpyID(), "braceForImpact.CarVel", speed2);
        bspyScratchpad(m_character->getBSpyID(), "braceForImpact.CarVel", predTime);
        bspyScratchpad(m_character->getBSpyID(), "braceForImpact.CarVel", m_carPos);
#endif

      }//if(pInst)
    }//if(m_parameters.instanceIndex != -1 && m_character->IsInstValid(m_parameters.instanceIndex, m_carInstGenID))
    return doBrace;
  }

  bool NmRsCBUBraceForImpact::OnCar()
  {
    if (!m_onCarCalculated)
    {
      m_onCarCalculated = true;
      m_onCar = false;
      if(m_carExists && m_character->IsInstValid(m_parameters.instanceIndex, m_carInstGenID))
      {
        rage::phInst* pInst = NULL;
        pInst = m_character->getLevel()->GetInstance(m_parameters.instanceIndex);
        if(pInst)
        {
          setNumOfCorners(4);
          setCorner(0, m_corner4);
          setCorner(1, m_corner3);
          setCorner(2, m_corner2);
          setCorner(3, m_corner1);

          rage::Vector3 upCopy(m_character->m_gUp),pointCopy(m_character->m_COM);
          m_character->levelVector(pointCopy);//mmmmtodo nearestPoint may need to be calculated in 3d remember (see comment below)
          rage::Vector3 nrstPoint;
          float insideResult = getDistanceToPoint(pointCopy, upCopy/*, radius*/, &nrstPoint); //when a member function make const;
          m_onCar = (insideResult <= 0.f);
        }
      }
    }

    return m_onCar;
  }
  void NmRsCBUBraceForImpact::GetCarData()
  {
    //identify car
    m_carExists = false;
    //Warn if the car inst disappears
#if __DEV
    if (!(m_parameters.instanceIndex == -1 || m_character->IsInstValid(m_parameters.instanceIndex, m_carInstGenID)))
	{
		Warningf("NmRsCBUBraceForImpact::GetCarData() - car level index/ gen ID no longer good");
	}
#endif
    //Get bounding box around car
    if(m_character->IsInstValid(m_parameters.instanceIndex, m_carInstGenID))
    {
      rage::phInst* pInst = NULL;
      pInst = m_character->getLevel()->GetInstance(m_parameters.instanceIndex);
      if(pInst)
      {
        m_carExists = true;
        //Get bounding box around car
        rage::Vector3 objectSize = VEC3V_TO_VECTOR3(pInst->GetArchetype()->GetBound()->GetBoundingBoxSize());
        objectSize *= 0.5f;
        rage::Vector3 corner1L(objectSize.x,objectSize.y,0.0f);
        rage::Vector3 corner2L(objectSize.x,-objectSize.y,0.0f);
        rage::Vector3 corner3L(-objectSize.x,-objectSize.y,0.0f);
        rage::Vector3 corner4L(-objectSize.x,objectSize.y,0.0f);

        //Offset by centroid not cofg
        rage::Vector3 offset = VEC3V_TO_VECTOR3(pInst->GetArchetype()->GetBound()->GetCentroidOffset());
        corner1L += offset;
        corner2L += offset;
        corner3L += offset;
        corner4L += offset;
        m_character->instanceToWorldSpace(&m_corner1, corner1L, m_parameters.instanceIndex);
        m_character->instanceToWorldSpace(&m_corner2, corner2L, m_parameters.instanceIndex);
        m_character->instanceToWorldSpace(&m_corner3, corner3L, m_parameters.instanceIndex);
        m_character->instanceToWorldSpace(&m_corner4, corner4L, m_parameters.instanceIndex);

        //Work out time to impact
        rage::Vector3 carPosLocal(0.0f, 0.0f, 0.0f);;
        m_character->instanceToWorldSpace(&m_carPos, carPosLocal, m_parameters.instanceIndex);
        m_character->getVelocityOnInstance(m_parameters.instanceIndex, carPosLocal, &m_carVel);


        //Where is the character in relation to the car?
        //Is the character good to glanceSpin against the side of the car?
        //Get moveAwayDir
        //m_moveAwayDir = m_character->m_COM - m_carPos;//move away from car centre

        //if at the side move away from side of car
        //if at front or back move away from front/back but also to the side a bit if desired
        rage::Vector3 toCharacter = m_character->m_COM - m_carPos;
        rage::Vector3 tofront = m_corner1 - m_corner2;
        rage::Vector3 toright = m_corner1 - m_corner4;
        rage::Vector3 tofrontRight = m_corner1 - m_carPos;
        float carWidth = toright.Mag()*0.5f;
        float carLength = tofront.Mag()*0.5f;
        //rage::Vector3 tobackRight = m_corner2 - m_carPos;
        tofront.Normalize();
        toright.Normalize();
        tofrontRight.Normalize();

        float distToSide = toright.Dot(toCharacter);
        float distToEnd = tofront.Dot(toCharacter);
        float uprightness = m_character->m_gUpReal.Dot(m_character->m_COMTM.b);
        distToSide -= rage::Selectf(distToSide, carWidth, -carWidth);
        distToEnd -= rage::Selectf(distToEnd, carLength, -carLength);
        toCharacter.Normalize();
        //tobackRight.Normalize();
        float chDotFront = tofront.Dot(toCharacter);
        float chDotRight = toright.Dot(toCharacter);
        distToSide = rage::Selectf(chDotRight, distToSide, -distToSide);//-ve inside car, +ve outside
        distToEnd = rage::Selectf(chDotFront, distToEnd, -distToEnd);//-ve inside car, +ve outside
        float frDotFront = tofront.Dot(tofrontRight);
        float frDotRight = toright.Dot(tofrontRight);
        m_moveAwayDir = -toright;//Left
        float sgnRight = 1.0f;
        if (chDotRight < 0.0f)
          sgnRight = -1.0f;
        m_zone = toLeft;
        if (chDotFront>=0.f && chDotFront>frDotFront)//as symmetrical don't need to check chDotFront>flDotFront
        {
          m_moveAwayDir = tofront + (m_parameters.moveSideways *toright*sgnRight);//front
          m_zone = toFront;
        }
        else if (chDotFront<0.f && chDotFront<-frDotFront)//as symmetrical brDotFront=-frDotFront and don't need to check chDotFront<blDotFront
        {
          m_moveAwayDir = -tofront + (m_parameters.moveSideways*toright*sgnRight);//back
          m_zone = toRear;
        }
        else if (chDotRight>=0.f && chDotRight>frDotRight)//as symmetrical don't need to check chDotRight>brDotRight
        {
          m_moveAwayDir = toright;//right
          m_zone = toRight;
        }

        //Glancing spin
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "braceForImpact", distToEnd);
        bspyScratchpad(m_character->getBSpyID(), "braceForImpact", distToSide);
        bspyScratchpad(m_character->getBSpyID(), "braceForImpact", carWidth);
        bspyScratchpad(m_character->getBSpyID(), "braceForImpact", carLength);
        bspyScratchpad(m_character->getBSpyID(), "braceForImpact", uprightness);
#endif
        m_character->resetVehicleHit();
        if (m_parameters.gsHelp && distToSide > m_parameters.gsSideMin && distToSide < m_parameters.gsSideMax && 
          distToEnd < m_parameters.gsEndMin && uprightness > m_parameters.gsUpness && m_carVel.Mag() > m_parameters.gsCarVelMin)
        {
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "ChangingFriction", distToSide);
#endif
          m_character->setVehicleHit(m_parameters.gsScale1Foot, m_parameters.gsFricScale1, m_parameters.gsFricMask1, m_parameters.gsFricScale2, m_parameters.gsFricMask2);
        }


      }//if(pInst)
    }//if(m_character->IsInstValid(m_parameters.instanceIndex, m_carInstGenID))

  }

  void NmRsCBUBraceForImpact::setCarInstGenID()
  {
    if (m_parameters.instanceIndex != -1 && m_character->getIsInLevel(m_parameters.instanceIndex))
    {
      rage::phInst* pInst = NULL;
      pInst = m_character->getLevel()->GetInstance(m_parameters.instanceIndex);
      if(pInst)
      {
        m_carInstGenID = PHLEVEL->GetGenerationID(m_parameters.instanceIndex);
      }
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUBraceForImpact::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.bodyStiffness, true);
    bspyTaskVar(m_parameters.braceDistance, true);
    bspyTaskVar(m_parameters.targetPredictionTime, true);
    bspyTaskVar(m_parameters.reachAbsorbtionTime, true);
    bspyTaskVar(m_parameters.grabDistance, true);
    bspyTaskVar(m_parameters.grabReachAngle, true);
    bspyTaskVar(m_parameters.grabHoldTimer, true);
    bspyTaskVar(m_parameters.maxGrabCarVelocity, true);
    bspyTaskVar(m_parameters.timeToBackwardsBrace, true);
    bspyTaskVar(m_parameters.legStiffness, true);
    bspyTaskVar(m_parameters.pos, true);
    bspyTaskVar(m_parameters.look, true);
    bspyTaskVar(m_parameters.grabDontLetGo, true);
    bspyTaskVar(m_parameters.instanceIndex, true);
    bspyTaskVar(m_parameters.minBraceTime, true);
    bspyTaskVar(m_parameters.handsDelayMin, true);
    bspyTaskVar(m_parameters.handsDelayMax, true);

    bspyTaskVar(m_parameters.moveAway, true);
    bspyTaskVar(m_parameters.moveAwayAmount, true);
    bspyTaskVar(m_parameters.moveAwayLean, true);
    bspyTaskVar(m_parameters.moveSideways, true);

    bspyTaskVar(m_parameters.bbArms, true);
    bspyTaskVar(m_parameters.newBrace, true);
    bspyTaskVar(m_parameters.braceOnImpact, true);
    bspyTaskVar(m_parameters.roll2Velocity, true);
    bspyTaskVar(m_parameters.rollType, true);

    bspyTaskVar(m_parameters.snapImpacts, true);
    bspyTaskVar(m_parameters.snapImpact, true);
    bspyTaskVar(m_parameters.snapBonnet, true);
    bspyTaskVar(m_parameters.snapFloor, true);

    bspyTaskVar(m_parameters.dampVel, true);
    bspyTaskVar(m_parameters.dampSpin, true);
    bspyTaskVar(m_parameters.dampUpVel, true);
    bspyTaskVar(m_parameters.dampSpinThresh, true);
    bspyTaskVar(m_parameters.dampUpVelThresh, true);

    bspyTaskVar(m_parameters.gsHelp, true);
    bspyTaskVar(m_parameters.gsEndMin, true);
    bspyTaskVar(m_parameters.gsSideMin, true);
    bspyTaskVar(m_parameters.gsSideMax, true);
    bspyTaskVar(m_parameters.gsUpness, true);
    bspyTaskVar(m_parameters.gsScale1Foot, true);
    bspyTaskVar(m_parameters.gsFricScale1, true);
    bspyTaskVar_Bitfield32(m_parameters.gsFricMask1, true);
    bspyTaskVar(m_parameters.gsFricScale2, true);
    bspyTaskVar_Bitfield32(m_parameters.gsFricMask2, true);
    bspyTaskVar(m_parameters.gsCarVelMin, true);   

    //Before Impact
    bspyTaskVar(m_target, false);
    bspyTaskVar(m_targetVel, false);
    bspyTaskVar(m_turnDirection, false);
    bspyTaskVar(m_leftHandPos, false);
    bspyTaskVar(m_leftHandNorm, false);
    bspyTaskVar(m_rightHandPos, false);
    bspyTaskVar(m_rightHandNorm, false);

    bspyTaskVar(m_leftHandGrabPos, false);
    bspyTaskVar(m_rightHandGrabPos, false);

    bspyTaskVar(m_backwardsBraceTimer, false);
    bspyTaskVar(m_distanceToTarget, false);
    bspyTaskVar(m_spineLean1, false);
    bspyTaskVar(m_spineLean2, false);
    bspyTaskVar(m_toggleHeadLookTimer, false);
    bspyTaskVar(m_handsDelay, false);
    bspyTaskVar(m_leftHandSeparation, false);
    bspyTaskVar(m_rightHandSeparation, false);
    bspyTaskVar(m_kneeBend, false);
    bspyTaskVar(m_spineShape, false);
    bspyTaskVar(m_spineBendMult, false);
    bspyTaskVar(m_legStiffOld, false);

    bspyTaskVar(m_hasGrabbed, false);
    bspyTaskVar(m_delayLeftHand, false);
    bspyTaskVar(m_balanceFailHandled, false);
    bspyTaskVar(m_doBrace, false);
    bspyTaskVar(m_shouldBrace, false);
    bspyTaskVar(m_braceTime, false);


    bspyTaskVar(m_braceLeft, false);
    bspyTaskVar(m_braceRight, false);

    bspyTaskVar(m_lookAtHands, false);
    bspyTaskVar(m_carExists, false);
    bspyTaskVar(m_onCarCalculated, false);
    bspyTaskVar(m_onCar, false);
    bspyTaskVar(m_carInstGenID, false);
    bspyTaskVar(m_carPos, false);
    bspyTaskVar(m_carVel, false);
    bspyTaskVar(m_moveAwayDir, false);
    
    bspyTaskVar(m_snap1stImpact, false);
    bspyTaskVar(m_snap1stImpactTorso, false);
    bspyTaskVar(m_snap1stImpactGround, false);


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

    bspyTaskVar(m_corner1, false);
    bspyTaskVar(m_corner2, false);
    bspyTaskVar(m_corner3, false);
    bspyTaskVar(m_corner4, false);
    bspyTaskVar(m_zone, false);

  }
#endif // ART_ENABLE_BSPY
}
