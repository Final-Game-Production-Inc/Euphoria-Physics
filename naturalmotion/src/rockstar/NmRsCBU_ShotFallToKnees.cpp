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
#include "NmRsCBU_Shot.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_CatchFall.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_PointGun.h"
#include "NmRsCBU_RollDownStairs.h"
#include "NmRsCBU_Rollup.h"
#include "NmRsCBU_StaggerFall.h"

#include "NmRsLimbs.h"

namespace ART
{
     
  //----------------FALL TO KNEES SHOT ------------------------------------------------
  bool NmRsCBUShot::fallToKnees_entryCondition()
  {
    return m_parameters.fallToKnees && (m_time > m_parameters.ftkBalanceTime) & !m_falling;
  }
  void NmRsCBUShot::fallToKnees_entry()
  {
    //FallToKnees
    m_fTK.m_bendLegs = false;
    m_fTK.m_LkneeHasHit = false;
    m_fTK.m_RkneeHasHit = false;
    m_fTK.m_ftkStuckTimer = 0.f;
    m_fTK.m_ftkLoosenessTimer = 0.f;
    m_fTK.m_LkneeHitLooseness = false;
    m_fTK.m_RkneeHitLooseness = false;
    m_fTK.m_fallingBack = false;
    m_fTK.m_squatting = false;
    m_fTK.m_doLunge = m_character->getRandom().GetFloat() < m_parameters.ftkLungeProb;

    m_fTK.m_hipMoventBackwards = m_character->getRandom().GetRanged(0.1f, 0.4f);
    if (m_character->getRandom().GetBool())
      m_fTK.m_hipMoventBackwards *= -1.f;

  }
  void NmRsCBUShot::fallToKnees_startFall()
  {
    NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    dynamicBalancerTask->setOpposeGravityAnkles(0.f);
    dynamicBalancerTask->setOpposeGravityLegs(0.f);

    if (dynamicBalancerTask->isActive())
      dynamicBalancerTask->setLowerBodyGravityOpposition(m_body);

    dynamicBalancerTask->setGiveUpThreshold(m_parameters.ftkBalanceAbortThreshold);
    dynamicBalancerTask->setFailMustCollide(m_parameters.ftkFailMustCollide);
    dynamicBalancerTask->setFallToKnees(true);

    //MMMMtodo do we need this?
    //Reduce shots upper body stiffness
    m_upperBodyStiffness = rage::Min(m_upperBodyStiffness,0.5f);

    NmRsCBUStaggerFall* staggerFallTask = (NmRsCBUStaggerFall*)m_cbuParent->m_tasks[bvid_staggerFall];
    if (staggerFallTask->isActive())
      staggerFallTask->deactivate();
  }

  void NmRsCBUShot::fallToKnees_tick(float timeStep)
  {
    static float legFrictionMultiplier = 10.0f;
    m_character->setFrictionPreScale(legFrictionMultiplier, bvmask_ThighLeft | bvmask_ShinLeft | bvmask_ThighRight | bvmask_ShinRight);

    NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kNotStepping && dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
    {
      float muscleStiffness = 4.f;
      getLeftLegInputData()->getAnkle()->setStiffness(7.f,1.f,&muscleStiffness);
      getRightLegInputData()->getAnkle()->setStiffness(7.f,1.f,&muscleStiffness);
    }

    rage::Vector3 vel = m_character->m_COMvel;
    float velMag = vel.Mag();
    vel.Normalize();
    float forwardsness = -m_character->m_COMTM.c.Dot(vel);
    rage::Matrix34 leftFootTM;
    rage::Matrix34 rightFootTM;
    float footLength = 0.305f;
    float footHeight = 0.07522f;
    getLeftLeg()->getFoot()->getBoundMatrix(&leftFootTM);
    rage::Vector3 ltoe = getLeftLeg()->getFoot()->getPosition();
    ltoe -= leftFootTM.c*footLength*0.5f;
    ltoe -= leftFootTM.b*footHeight*0.5f;
    getRightLeg()->getFoot()->getBoundMatrix(&rightFootTM);
    rage::Vector3 rtoe = getRightLeg()->getFoot()->getPosition();
    rtoe -= rightFootTM.c*footLength*0.5f;
    rtoe -= rightFootTM.b*footHeight*0.5f;

    m_character->levelVector(rtoe);
    m_character->levelVector(ltoe);
    rage::Vector3 footCentreLevelled = 0.5f*(getLeftLeg()->getFoot()->getPosition()+getRightLeg()->getFoot()->getPosition());
    m_character->levelVector(footCentreLevelled);
    rage::Vector3 comLevelled = m_character->m_COM + 0.2f*m_character->m_COMvel;
    m_character->levelVector(comLevelled);
    rage::Vector3 normal;
    rage::Vector3 l2rToe = rtoe-ltoe;
    normal.Cross(l2rToe, m_character->m_gUp);
    bool overBalancing = false;

    if ((normal.Dot(footCentreLevelled-rtoe)> 0.f ? 1 : -1) !=  (normal.Dot(comLevelled-rtoe) > 0.f ? 1 : -1))
      overBalancing = true;
    overBalancing = overBalancing || m_fTK.m_LkneeHasHit || m_fTK.m_RkneeHasHit;

    bool airborne = !(m_body->getLeftLeg()->getFoot()->collidedWithNotOwnCharacter() ||
      m_body->getRightLeg()->getFoot()->collidedWithNotOwnCharacter());

    //apply Helper forces to tilt the character over the balance point of the toe 2 toe line if the character is nearly balanced
    //add in some reduced help if upright constraint is not active but the character isn't so nearly balanced
    if (!overBalancing && (!m_fTK.m_squatting) &&
      ((dynamicBalancerTask->m_balanceInstability < 2.0f && m_character->m_uprightConstraint.forceActive )
      || (dynamicBalancerTask->m_balanceInstability < 1.0f)))
    {
      float leanHelpReduction = rage::Clamp(dynamicBalancerTask->m_balanceInstability, 1.f,2.f);
      //full helper forces if m_balanceInstability<1.f  Taper off to zero as m_balanceInstability approaches 2
      //leanHelpReduction -= 1.f;//put in range 0..1
      //leanHelpReduction = -balanceInstability + 1.f;
      //i.e.
      leanHelpReduction = -leanHelpReduction + 2.f;

      rage::Vector3 tipOver = 0.5f*(rtoe + ltoe);
      m_character->levelVector(tipOver);
      tipOver -= comLevelled;
      float fMag = tipOver.Dot(normal);
      
      rage::Vector3 helperForce = fMag*2.f*m_parameters.ftkHelperForce*normal*leanHelpReduction;
      helperForce.ClampMag(0.0f, 150.0f);

      if (!airborne)
        getSpine()->getPelvisPart()->applyForce(helperForce);
      if (m_parameters.ftkHelperForceOnSpine && !airborne)
        getSpine()->getSpine3Part()->applyForce(helperForce);


      tipOver.Normalize();
      dynamicBalancerTask->autoLeanInDirection(fMag*normal*leanHelpReduction,m_parameters.ftkLeanHelp);
      if (!m_fTK.m_bendLegs) 
        dynamicBalancerTask->autoLeanHipsInDirection(fMag*normal*leanHelpReduction,m_parameters.ftkLeanHelp*0.5f);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "FallToKnees", leanHelpReduction);
      bspyScratchpad(m_character->getBSpyID(), "FallToKnees", fMag);
#endif     

    }

#if ART_ENABLE_BSPY
    rage::Vector3 col(1,1,0);
    m_character->bspyDrawLine(ltoe,rtoe, col);
    if (overBalancing)
      col.Set(0.f,0.f,1.f);
    m_character->bspyDrawPoint(comLevelled, 0.6f, col);
    bspyScratchpad(m_character->getBSpyID(), "FallToKnees", overBalancing);
    bspyScratchpad(m_character->getBSpyID(), "FallToKnees", forwardsness);
    bspyScratchpad(m_character->getBSpyID(), "FallToKnees", velMag);
    bspyScratchpad(m_character->getBSpyID(), "FallToKnees", dynamicBalancerTask->m_balanceInstability);
#endif     
    if (!m_fTK.m_bendLegs)
    {
      //Fall over if nearly balanced no matter what your direction of movement
      bool startFall = (dynamicBalancerTask->m_balanceInstability < m_parameters.ftkFallBelowStab && (!airborne));
      //The character will probably do a good fallToKnees forward (balanced but going forwards or overbalanced going forwards slowly)
      startFall = startFall || ((((dynamicBalancerTask->m_balanceInstability < 0.99f) && forwardsness > 0.3f && velMag > 0.2f) || (overBalancing && forwardsness > 0.3f && velMag < 0.99f)) && (!airborne));
      //The character is falling already
      startFall = startFall || (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK);
      if (startFall)
      {
        fallToKnees_startFall();
        m_fTK.m_bendLegs = true;
      }
    }
    else//We are falling
    {
      //mmmmtodo alot of this falling part could be put into the if below
      if (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
      {
        //Stop the character from balancing too long on knees.  Calibrated to allow some wobble on knees to count as being balanced
        if (m_character->m_COMvelMag < 0.05f)
          m_fTK.m_ftkStuckTimer += timeStep;
        if (m_character->m_COMvelMag < 0.02f)
          m_fTK.m_ftkStuckTimer += timeStep;
        if (m_character->m_COMvelMag > 0.075f)
          m_fTK.m_ftkStuckTimer = 0.0f;

        if (m_fTK.m_ftkStuckTimer > 2.0f)
          dynamicBalancerTask->forceFailOnly();
      }

      //Find and store State (1.Falling to knees, 2. Hit Knees, 3.Hit upper body)
      rage::Vector3 pos, collisionNormal;
      float depth = 0;
      static float kneeHeightHit = 0.23f;
      static float kneeHeightSquat = 0.33f;
      //Knees have nearly hit floor - the character can sometimes balance almost kneeling down for too long,
      //  so call it an impact to lessen this
      if (!m_fTK.m_squatting && (m_character->m_COMvelMag < 0.5f))
      {
        if ((getLeftLeg()->getAnkle()->getJointPosition()-getSpine()->getPelvisPart()->getPosition()).Mag() < 0.35f &&
          (getRightLeg()->getAnkle()->getJointPosition()-getSpine()->getPelvisPart()->getPosition()).Mag() < 0.35f )
          m_fTK.m_squatting = true;
      }
      if ((!m_fTK.m_LkneeHasHit) && getLeftLeg()->getFoot()->collidedWithEnvironment())
      {
        getLeftLeg()->getFoot()->getCollisionZMPWithEnvironment(pos, collisionNormal,&depth);
        pos -= getLeftLeg()->getKnee()->getJointPosition();
        float ground2Knee = -m_character->vectorHeight(pos);
        if (ground2Knee < kneeHeightHit)
          m_fTK.m_LkneeHasHit = true;
        if (ground2Knee < kneeHeightSquat  && (m_character->m_COMvelMag < 0.5f))
          m_fTK.m_squatting = true;
      }
      if ((!m_fTK.m_RkneeHasHit) && getRightLeg()->getFoot()->collidedWithNotOwnCharacter())
      {
        rage::Vector3 pos, collisionNormal;
        float depth = 0;
        getRightLeg()->getFoot()->getCollisionZMPWithEnvironment(pos, collisionNormal,&depth);
        pos -= getRightLeg()->getKnee()->getJointPosition();
        float ground2Knee = -m_character->vectorHeight(pos);
        if (ground2Knee < kneeHeightHit)
          m_fTK.m_RkneeHasHit = true;
        if (ground2Knee < kneeHeightSquat && (m_character->m_COMvelMag < 0.5f))
          m_fTK.m_squatting = true;
      }

      //stop the character staying in the squatting position if the uprightConstraint is on
      if (m_fTK.m_squatting)
      {
        m_character->m_uprightConstraint.forceActive = false;
        m_character->m_uprightConstraint.torqueActive = false;
      }
      //Hit Knees?
      if ((!m_fTK.m_LkneeHasHit) && ((getLeftLeg()->getThigh()->collidedWithNotOwnCharacter()) ||
        (getLeftLeg()->getShin()->collidedWithNotOwnCharacter())))
      {

        if (getLeftLeg()->getThigh()->collidedWithNotOwnCharacter() )
        {
          getLeftLeg()->getThigh()->getCollisionZMPWithEnvironment(pos, collisionNormal,&depth);
          pos -= getLeftLeg()->getKnee()->getJointPosition();
          if (pos.Mag() < 0.2f)
            m_fTK.m_LkneeHasHit = true;
        }

        //Are shin collisions near the knee? (shin collides while walking)
        if (getLeftLeg()->getShin()->collidedWithEnvironment())
        {
          getLeftLeg()->getShin()->getCollisionZMPWithEnvironment(pos, collisionNormal,&depth);
          pos -= getLeftLeg()->getKnee()->getJointPosition();
          if (pos.Mag() < 0.2f)
            m_fTK.m_LkneeHasHit = true;
        }
        //
      }
      if ((!m_fTK.m_RkneeHasHit) && ((getRightLeg()->getThigh()->collidedWithNotOwnCharacter())
        || (getRightLeg()->getShin()->collidedWithNotOwnCharacter())))
      {

        if (getRightLeg()->getThigh()->collidedWithNotOwnCharacter())
        {
          getRightLeg()->getThigh()->getCollisionZMPWithEnvironment(pos, collisionNormal,&depth);
          pos -= getRightLeg()->getKnee()->getJointPosition();
          if (pos.Mag() < 0.2f)
            m_fTK.m_RkneeHasHit = true;
        }

        //Are shin collisions near the knee? (shin collides while walking)
        if (getRightLeg()->getShin()->collidedWithEnvironment())
        {
          getRightLeg()->getShin()->getCollisionZMPWithEnvironment(pos, collisionNormal,&depth);
          pos -= getRightLeg()->getKnee()->getJointPosition();
          if (pos.Mag() < 0.2f)
            m_fTK.m_RkneeHasHit = true;
        }
      }

      float kneesApart = rage::Abs(getLeftLeg()->getKnee()->getJointPosition().z - getRightLeg()->getKnee()->getJointPosition().z);
      bool lunge =  kneesApart > 0.2f;


      //Reduce the friction on the feet as they become less flat to the ground 
      //The character drops much quicker and fluidly if friction is reduced and balancing in the squatting position for too long is avoided
      //Looks bad if feet slide forwards hence the friction if they are flat.
      //  Maybe friction on if feet go forwards relative to the body would be better?
      float heelUp = leftFootTM.c.Dot(m_character->m_gUp);
      float fricMult = 1.f;
      fricMult -= m_parameters.ftkFricMult*heelUp;
      if (fricMult > 1.f)
        fricMult = 1.f;
      if (fricMult < 0.1f)
        fricMult = 0.1f;
      m_character->setFrictionPreScale(fricMult, bvmask_FootLeft);
      heelUp = rightFootTM.c.Dot(m_character->m_gUp);
      fricMult = 1.f;
      fricMult -= m_parameters.ftkFricMult*heelUp;
      if (fricMult > 1.f)
        fricMult = 1.f;
      if (fricMult < 0.1f)
        fricMult = 0.1f;
      m_character->setFrictionPreScale(fricMult, bvmask_FootRight);

      //Bend the legs and tilt the hips - slightlty differently depending on falling forwards or backwards
      //  This tries to control the angle that the torso will have when we land on the knees
      if (forwardsness > 0.3f)
      {
        dynamicBalancerTask->setLegStraightnessModifier(dynamicBalancerTask->getLegStraightnessModifier()-m_parameters.ftkBendRate*timeStep);
        dynamicBalancerTask->setHipPitch(-m_parameters.ftkPitchForwards);
        dynamicBalancerTask->setHipRoll(0.0f);
        dynamicBalancerTask->setHipYaw(0.0f);
      }
      //This is a very rough metric for identifying when the character is going to slump horribly backwards
      else if (forwardsness < -0.98f && (m_fTK.m_LkneeHasHit && m_fTK.m_RkneeHasHit) && velMag > 0.1f)//
      {
        m_fTK.m_fallingBack =true;
      }
      else
      {
        dynamicBalancerTask->setLegStraightnessModifier(dynamicBalancerTask->getLegStraightnessModifier()-m_parameters.ftkBendRate*timeStep);
        dynamicBalancerTask->setHipPitch(-m_parameters.ftkPitchBackwards);
        dynamicBalancerTask->setHipRoll(0.0f);
        dynamicBalancerTask->setHipYaw(0.0f);
      }
      if (dynamicBalancerTask->getLegStraightnessModifier() < -0.5f)
        dynamicBalancerTask->setLegStraightnessModifier(-0.5f);
      if (dynamicBalancerTask->getLegStraightnessModifier() >= -0.0f)
        dynamicBalancerTask->setLegStraightnessModifier(0.0f);

      if (m_fTK.m_LkneeHasHit || m_fTK.m_RkneeHasHit || m_fTK.m_squatting || (dynamicBalancerTask->getLegStraightnessModifier() <= -0.49f))
      {
        dynamicBalancerTask->setForceBalance(true);//because shotFromBehind/shotInGuts can set this false
        //can cause the character to have a more stable base - but looks better
        dynamicBalancerTask->setLegCollision(true);//mmmmtodo if there is alot of leg hitting leg falling over then use the startFall conditions with ftkFallBelowStab = 0.5ish
        //Can give a more spinny reaction when landed on knees if no footSlip comp
        m_character->enableFootSlipCompensationActive(!m_parameters.ftkKneeSpin);
      }

      //The character is on it's knees...
      if (m_fTK.m_LkneeHasHit || m_fTK.m_RkneeHasHit)
      {
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "FallToKnees", kneesApart);
        bspyScratchpad(m_character->getBSpyID(), "FallToKnees", lunge);
#endif
        NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)m_cbuParent->m_tasks[bvid_pointGun];
        Assert(pointGunTask);
#if NM_FALL2KNEESPOINTGUN
        if (pointGunTask->isActive())
          pointGunTask->m_forceNeutral = true;
#endif

        if (m_fTK.m_LkneeHasHit || m_fTK.m_RkneeHasHit)
        {
          if (m_fTK.m_doLunge)
          {
            //if knees are close vertically - i.e. if a 2 kneed landing is possible we start to try to balance on knees with both hips
            //if knees aren't close vertically (lunge = true)then let the dynamicBalancer keep control of the leg whose knee hasn't hit
            //  this allows the hip and knee to keep bent.
            if (m_fTK.m_LkneeHasHit || (!lunge))
              getLeftLegInputData()->getHip()->setDesiredLean1(m_character->blendToSpecifiedPose(getLeftLeg()->getHip()->getActualLean1(),0.f, m_parameters.ftkHipBlend));
            if (m_fTK.m_RkneeHasHit || (!lunge))
              getRightLegInputData()->getHip()->setDesiredLean1(m_character->blendToSpecifiedPose(getRightLeg()->getHip()->getActualLean1(),0.f, m_parameters.ftkHipBlend));
          }
          else
            //This is the on knees "balancer" code.  
            //You could make this actually balance on the knees but is intended to be an unstable equilibrium to get the character to eventually fall naturally
            //The bent knee position is quite stable by itself so more balancing can make the performance look samey or controlled
          {
            getLeftLegInputData()->getHip()->setDesiredLean1(m_character->blendToSpecifiedPose(getLeftLeg()->getHip()->getActualLean1(),0.f, m_parameters.ftkHipBlend));
            getRightLegInputData()->getHip()->setDesiredLean1(m_character->blendToSpecifiedPose(getRightLeg()->getHip()->getActualLean1(),0.f, m_parameters.ftkHipBlend));
          }
        }


        //mmmmtodo do we need this?
        static float upStiff = 0.35f; 
        m_upperBodyStiffness = rage::Min(m_upperBodyStiffness,upStiff);
        if (m_parameters.ftkReleaseReachForWound >= 0.0f)
        {//mmmmTodo allow useExtendedCatchFall to keep reachForWound on?
          if (m_reachLeftEnabled)
          {
            float reachTimeToGo = rage::Min(m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound -  m_hitTimeLeft, m_parameters.ftkReleaseReachForWound);
            reachTimeToGo = rage::Max(reachTimeToGo, -0.1f); //to stop m_hitTimeLeft increasing exponentially once  > m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound
            m_hitTimeLeft = m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound - reachTimeToGo;
          }
          if (m_reachRightEnabled)
          {
            float reachTimeToGo = rage::Min(m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound -  m_hitTimeRight, m_parameters.ftkReleaseReachForWound);
            reachTimeToGo = rage::Max(reachTimeToGo, -0.1f); //to stop m_hitTimeRight increasing exponentially once  > m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound
            m_hitTimeRight = m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound - reachTimeToGo;
          }
        }
        if (m_parameters.ftkReleasePointGun)
        {
          pointGunTask->deactivate();
        }
      }

      //Apply looseness to exagerate knee impacts
      if (m_fTK.m_LkneeHasHit && !m_fTK.m_LkneeHitLooseness)
      {
        m_fTK.m_ftkLoosenessTimer = m_parameters.ftkImpactLoosenessTime;
        m_fTK.m_LkneeHitLooseness = true;
      }
      if (m_fTK.m_RkneeHasHit && !m_fTK.m_RkneeHitLooseness)
      {
        m_fTK.m_ftkLoosenessTimer = m_parameters.ftkImpactLoosenessTime;
        m_fTK.m_RkneeHitLooseness = true;
      }
      if (m_fTK.m_ftkLoosenessTimer > 0.f)
      {
        
        m_body->callMaskedEffectorDataFunctionFloatArg(
          bvmask_UpperBody,
          1.01f - m_parameters.ftkImpactLooseness,
          &NmRs1DofEffectorInputWrapper::setMuscleStiffness,
          &NmRs3DofEffectorInputWrapper::setMuscleStiffness);

        m_fTK.m_ftkLoosenessTimer -= timeStep;

        if (m_fTK.m_ftkLoosenessTimer <= 0.f)
          m_body->resetEffectors(kResetMuscleStiffness);
      }

      //Fight against getting stuck leaning backwards - 1) lean/twist to the side using the balancer (also brings the knees together)
      float fbYaw = m_fTK.m_hipMoventBackwards;
      float fbRoll = m_fTK.m_hipMoventBackwards*2.f;
      if (m_fTK.m_fallingBack)
      {
        getLeftLegInputData()->getHip()->setDesiredLean1(m_character->blendToSpecifiedPose(getLeftLeg()->getHip()->getActualLean1(),m_parameters.ftkHipAngleFall, m_parameters.ftkHipBlend));
        getRightLegInputData()->getHip()->setDesiredLean1(m_character->blendToSpecifiedPose(getRightLeg()->getHip()->getActualLean1(),m_parameters.ftkHipAngleFall, m_parameters.ftkHipBlend));
        getSpineInputData()->getSpine0()->setDesiredLean2(0.f);

        m_character->enableFootSlipCompensationActive(false);
        dynamicBalancerTask->setHipRoll(fbRoll);
        dynamicBalancerTask->setHipYaw(fbYaw);
      }

      //Fight against getting stuck leaning backwards - 2) Let the feet slip
      // This doesn't work if against a wall
      if (m_fTK.m_LkneeHasHit && m_fTK.m_RkneeHasHit)
      {
        m_character->setFrictionPreScale(0.0f, bvmask_FootLeft | bvmask_FootRight);
      }

      //Control landing subBehaviours
      NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
      NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
      if (lunge && !m_parameters.ftkAlwaysChangeFall)
      {
        rdsTask->m_fall2Knees = false;
        catchFallTask->m_fall2Knees = false;
      }
      else
      {
        rdsTask->m_fall2Knees = true;
        catchFallTask->m_fall2Knees = true;
      }

      //Set arm reaction
      if (rdsTask->isActive())
      {
        rdsTask->m_ftk_SpineBend = m_parameters.ftkSpineBend;
        rdsTask->m_ftk_StiffSpine = m_parameters.ftkStiffSpine;       
        rdsTask->m_ftk_armsIn = (m_parameters.ftkOnKneesArmType == ftk_armsIn);
        rdsTask->m_ftk_armsOut = (m_parameters.ftkOnKneesArmType == ftk_armsOut);
      }
      if (catchFallTask->isActive())
      {
        catchFallTask->m_ftk_armsIn = (m_parameters.ftkOnKneesArmType == ftk_armsIn);
        catchFallTask->m_ftk_armsOut = (m_parameters.ftkOnKneesArmType == ftk_armsOut);
      }
    }//Falling
  }
  bool NmRsCBUShot::fallToKnees_exitCondition()
  {
    //mmmmTodo //mmmmhere
    return false;//m_falling;
  }
  void NmRsCBUShot::fallToKnees_exit()
  {
    NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    dynamicBalancerTask->setLegStraightnessModifier(0.f);
    dynamicBalancerTask->setForceBalance(false);
    dynamicBalancerTask->setGiveUpThreshold(0.6f);
    dynamicBalancerTask->setFailMustCollide(false);
    dynamicBalancerTask->setFallToKnees(false);

    m_character->setFrictionPreScale(1.0f, bvmask_LegLeft | bvmask_LegRight);

    NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
    rdsTask->m_fall2Knees = false;
    rdsTask->m_ftk_armsIn = false;
    rdsTask->m_ftk_armsOut = false;
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    catchFallTask->m_fall2Knees = false;
    catchFallTask->m_ftk_armsIn = false;
    catchFallTask->m_ftk_armsOut = false;

    m_body->resetEffectors(kResetMuscleStiffness);

#if NM_FALL2KNEESPOINTGUN
    NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)m_cbuParent->m_tasks[bvid_pointGun];
    Assert(pointGunTask);
    if (pointGunTask->isActive())
      pointGunTask->m_forceNeutral = false;
#endif
  }
}
