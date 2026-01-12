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
#include "NmRsCBU_BalancerCollisionsReaction.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_BodyFoetal.h" 
#include "NmRsCBU_Catchfall.h"
#include "NmRsCBU_DynamicBalancer.h" 
#include "NmRsCBU_FallOverWall.h" 
#include "NmRsCBU_RollDownStairs.h" 
#include "NmRsCBU_Shot.h" 
#include "NmRsCBU_Stumble.h" 

namespace ART
{
  NmRsCBUBalancerCollisionsReaction::NmRsCBUBalancerCollisionsReaction(ART::MemoryManager* services) : CBUTaskBase(services, bvid_balancerCollisionsReaction)
  {
    initialiseCustomVariables();
  }

  NmRsCBUBalancerCollisionsReaction::~NmRsCBUBalancerCollisionsReaction()
  {
  }

  void NmRsCBUBalancerCollisionsReaction::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_previousBalancerState = -1; //Force feedback to send Bal_Normal at start
    m_balancerState = bal_Normal;
    m_impactOccurred = false;
    m_pushingOff = false;
    m_ubImpactOccurred = false;
    m_lbImpactOccurred = false;
    m_block = false;
    m_collisionLevelIndex = -1;
    m_numStepsAtImpact = INT_MAX/2;
    m_sideOfPlane = 0.f;
    m_timeAfterImpact = 0.f;
    m_ubCollisionNormalTotal.Zero();
    m_lbCollisionNormalTotal.Zero();
    m_normal1stContactLocal.Zero();
    m_pos1stContactLocal.Zero();
    m_pos1stContactLocalForFOW.Zero();
    m_normal1stContact.Zero();
    m_pos1stContact.Zero();
    m_pos1stContactForFOW.Zero();
    m_comVelPrevious.Zero();
    m_comVel_PreImpact.Zero();

    m_slumpStiffRLeg = 12.f;
    m_slumpStiffLLeg = 12.f;
    m_slumpStiffRKnee = 12.f;
    m_slumpStiffLKnee = 12.f;
    m_stableTimer = 0.f;
    m_drapeTimer = 0.f;
    m_headCollided = false;
    m_rollAngVel = 0.f;
    m_slump = false;
    m_ignoreBlock = false;
    m_haveSnappedBack = false;
    m_controlStiffnessStrengthScale = 1.001f;
    
    m_probeHitPos.Zero();
    m_probeNormal.Zero();
    m_obstacleType = BCR_Unknown;
    m_waitingForAsynchProbe = false;
  }


  void NmRsCBUBalancerCollisionsReaction::onActivate()
  {
    Assert(m_character);

    //BalancerCollisionsReaction Entry
    initialiseCustomVariables();
    m_character->setDontRegisterCollsion(m_parameters.ignoreColMassBelow, m_parameters.ignoreColVolumeBelow); 
    m_comVelPrevious = m_comVel_PreImpact = m_character->m_COMvel;//better than initializing with zero

    if (m_parameters.fallOverWallDrape)
    {
      //Initialise fallOverWallParameters so that they can be changed after balancerCollisionsReaction has activated
      NmRsCBUFallOverWall* fowTask = (NmRsCBUFallOverWall*)m_cbuParent->m_tasks[bvid_fallOverWall];
      Assert(fowTask);
      fowTask->updateBehaviourMessage(NULL); // sets values to defaults
      fowTask->m_parameters.magOfForce = 0.5f;
      float bodytwist = 0.1f;
      fowTask->m_parameters.minTwistTorqueScale = 45.0f * bodytwist;
      fowTask->m_parameters.maxTwistTorqueScale = fowTask->m_parameters.minTwistTorqueScale + (40.0f * bodytwist);
      fowTask->m_parameters.maxTwist = PI;
      fowTask->m_parameters.moveArms = false;
      fowTask->m_parameters.bendSpine = true;
      fowTask->m_parameters.moveLegs = true;
    }
    m_defaultArmMotiontimer1 = m_character->getRandom().GetRanged(-2.f*PI, 2.f*PI);;
    m_defaultArmMotiontimer2 = m_character->getRandom().GetRanged(-2.f*PI, 2.f*PI);;

  }

  void NmRsCBUBalancerCollisionsReaction::onDeactivate()
  {
    Assert(m_character);

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    dynamicBalancerTask->setBalanceTime(0.2f);
    resetFrictionMultipliers();
    initialiseCustomVariables();

    //Turn off not Registering certain collisions
    m_character->setDontRegisterCollsion(-1.f, -1.f); //mmmmtodo note this may reset highFall's settings

  }

  CBUTaskReturn NmRsCBUBalancerCollisionsReaction::onTick(float timeStep)
  {
    if (!m_character->getArticulatedBody())
      return eCBUTaskComplete;

    // check to see if the articulated body is asleep, in which case we are 'stable' and can't / needn't balance
    if (rage::phSleep *sleep = m_character->getArticulatedWrapper()->getArticulatedCollider()->GetSleep())
    {
      if (sleep->IsAsleep())
        return eCBUTaskComplete;
    }

      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    decideBalancerState(timeStep, dynamicBalancerTask->getNumSteps());
    setBalanceTime();
    //setHipPitch(timeStep);
    //mmmmbcr
    if (m_impactOccurred)//because the balancer can just call slump to fall over (setBalance time not set if bal_Slump so OK)
    {
      if ((dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK) && 
        m_balancerState > bal_Impact)
      {
        if (m_timeAfterImpact > 0.0001f && m_timeAfterImpact < m_parameters.reactTime && dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
          defaultArmMotion_armsBrace(timeStep);
      }

      //exaggerateImpact
      if (m_controlStiffnessStrengthScale < 1.0f)
      {
        //exaggerateImpact(timeStep);
        // ramp up MUSCLE STIFFNESS: an initial flat response is followed by ramp:
        if(m_timeAfterImpact >= m_parameters.impactWeaknessZeroDuration)
          m_controlStiffnessStrengthScale = rage::Min(1.f, m_controlStiffnessStrengthScale + timeStep/m_parameters.impactWeaknessRampDuration);

        NmRsCBUShot* shotTask = (NmRsCBUShot*)m_cbuParent->m_tasks[bvid_shot];
        Assert(shotTask);
        if (shotTask->isActive())
        {
          //don't interrupt a bullet - actually go for the lower of the stiffnesses from newHit or impact
          //Could have just hiJacked shotControlStiffness which would mean the code is not repeated
          // But would be 1 frame late
          // But this overrides the arms stiffnesses for shotDefault arms
          if (shotTask->getStiffnessStrengthScale() >= m_controlStiffnessStrengthScale)
          {
            m_character->controlStiffness(
              *m_body,
              timeStep,
              m_controlStiffnessStrengthScale,
              shotTask->m_parameters.bodyStiffness,
              shotTask->m_parameters.armStiffness,
              shotTask->getArmsDamping(),
              shotTask->getSpineDamping(),
              shotTask->getNeckStiffness(),
              shotTask->getNeckDamping(),
              shotTask->getUpperBodyStiffness(),
              shotTask->getLowerBodyStiffnesss(),
              shotTask->getInjuredLArm(),
              shotTask->getInjuredRArm(),
              m_parameters.impactLoosenessAmount,
              shotTask->m_parameters.kMultOnLoose,
              shotTask->m_parameters.kMult4Legs,
              shotTask->m_parameters.minLegsLooseness,
              shotTask->m_parameters.minArmsLooseness,
              false, //shotTask->m_parameters.bulletProofVest,
              false, //shotTask->m_parameters.allowInjuredArm,
              false//shotTask->m_parameters.stableHandsAndNeck
              );
          }
        }
        else
        {
          //NOT IMPLEMENTED YET FOR OTHER UNDERLYING BEHAVIOURS e.g. bodyBalance, flinch, etc
          ////match underlying behaviours stiffnesses?
          //m_character->controlStiffness(
          //  timeStep,
          //  m_controlStiffnessStrengthScale,
          //  m_spineStiffness,
          //  m_armsStiffness,
          //  m_armsDamping,
          //  m_spineDamping,
          //  m_neckStiffness,
          //  m_neckDamping,
          //  m_upperBodyStiffness,
          //  m_lowerBodyStiffness,
          //  m_injuredLArm,
          //  m_injuredRArm,
          //  m_parameters.loosenessAmount,
          //  m_parameters.kMultOnLoose,
          //  m_parameters.kMult4Legs,
          //  m_parameters.minLegsLooseness,
          //  m_parameters.minArmsLooseness,
          //  m_parameters.bulletProofVest,
          //  m_parameters.allowInjuredArm,
          //  m_parameters.stableHandsAndNeck);

        }
      }


      //UNSNAP
      //mmmtodo choose direction 
      if (m_parameters.snap && m_timeAfterImpact>m_parameters.unSnapInterval && !m_haveSnappedBack)
      {
        rage::Vector3 snapDirection;
        rage::Vector3 projOnNormal = m_normal1stContact*(m_comVel_PreImpact.Dot(m_normal1stContact));
        snapDirection = 2.f*projOnNormal - m_comVel_PreImpact;
        snapDirection.Normalize();
        float mag = rage::Abs(m_normal1stContact.Dot(m_comVel_PreImpact));
        mag = rage::Clamp(mag-0.5f, 0.f, 1.f);
        m_character->snap(
          m_parameters.snapMag*-1.f*m_parameters.unSnapRatio*mag,
          m_parameters.snapDirectionRandomness, 
          m_parameters.snapHipType,
          m_parameters.snapLeftArm,
          m_parameters.snapRightArm,
          m_parameters.snapLeftLeg,  
          m_parameters.snapRightLeg,  
          m_parameters.snapSpine,  
          m_parameters.snapNeck, 
          m_parameters.snapPhasedLegs, 
          m_parameters.snapUseTorques,
          1.f,
          -1,
          &snapDirection);
        m_haveSnappedBack = true;
      }
    }
    //mmmmtodo turn of slump and restore friction after certain Drape time or drapeForward success.
    //HasFailed here so that handsAndKnees will call slump even though balancer below fail value to stop it getting stuck on the wall 
    if ((m_balancerState == bal_Slump || m_balancerState == bal_Trip) && !dynamicBalancerTask->hasFailed())//mmmmtodo || m_balancerState == bal_DrapeForward)
      slump(timeStep);



    return eCBUTaskComplete;
  }

  void NmRsCBUBalancerCollisionsReaction::decideBalancerState(float timeStep, int numOfSteps)
  {
    //todo:  unexpected collision normals ie less than horizontal
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    if (m_impactOccurred)
    {
      //Impacted object could be removed from physics eg. gun/horse
      if (PHLEVEL->IsInLevel(m_collisionLevelIndex))
      {
        m_character->instanceToWorldSpace(&m_pos1stContact,m_pos1stContactLocal,m_collisionLevelIndex);
        m_character->instanceToWorldSpace(&m_pos1stContactForFOW,m_pos1stContactLocalForFOW,m_collisionLevelIndex);
        m_character->rotateInstanceToWorldSpace(&m_normal1stContact,m_normal1stContactLocal,m_collisionLevelIndex);
        m_normal1stContact.Normalize();
      }
      else
      {
        m_collisionLevelIndex = -1;
        m_pos1stContactLocal = m_pos1stContact;
        m_pos1stContactLocalForFOW = m_pos1stContactForFOW;
        m_normal1stContactLocal = m_normal1stContact;
      }

      m_timeAfterImpact += timeStep;
    }


    if (numOfSteps >= m_parameters.numStepsTillSlump + m_numStepsAtImpact)
    {
      //mmmmbcr dynamicBalancerTask->setForceBalance(true);
      //if (m_balancerState == bal_Slump || m_balancerState == bal_LeanAgainst || m_balancerState == bal_LeanAgainstStable)
      m_balancerState = bal_Slump;
      //else
      //  m_balancerState = bal_Trip;
      sendBalanceStateFeedback();
      NmRsCBUShot* shotTask = (NmRsCBUShot*)m_cbuParent->m_tasks[bvid_shot];
      Assert(shotTask);

      if ((m_parameters.reboundMode == 0 && !shotTask->isActive()) || m_parameters.reboundMode == 2) 
        return;//don't let slump become a rebound if rebound behaviour is slump
    }

    if (!m_impactOccurred && (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK))
      m_balancerState = bal_End;

    //Put turn on LegCollisions if not interpenetrating here if bal_LeanAgainstStable, bal_Trip, bal_Slump
    //DeadEnd states: return
    //if ((m_balancerState == bal_Rebound) || (m_balancerState == bal_DrapeGlancingSpin) || (m_balancerState == bal_End))
    if ((m_balancerState == bal_Rebound) || (m_balancerState == bal_End))
    {
      sendBalanceStateFeedback();
      return;
    }

    //if (m_balancerState == bal_LeanAgainstStable)//mmmmtodo decide if this is necessary ie if push at this point won't step
    //  dynamicBalancerTask->setForceBalance(true);

    //Don't lean in direction if collided with something and not rebounding from it //mmmmtodo decide if this is necessary, also turn off stagger lean?
    //if ((m_impactOccurred) && (m_balancerState != bal_Rebound))
    //  dynamicBalancerTask->autoLeanCancel();


    //m_obstacleType = Wall / Table / step / stairs
    // 

    if (m_waitingForAsynchProbe)
    {
      m_obstacleType = getProbeResult(m_pos1stContactForFOW, m_normal1stContact);
      if (m_obstacleType != BCR_Unknown)
        m_waitingForAsynchProbe = false;
    }

    //Decide state from impact
    rage::Vector3 collisionNormal;//module level
    rage::Vector3 collisionPos;
    float depth = 0;
    rage::phInst *collisionInst = NULL;
    int collisionInstGenID = -1;
    rage::phInst *collisionInstOK = NULL;
    //order from least important
    bool impactOccurred = false;
    int height = 0;//now unused
    if (!m_impactOccurred)
    {
      m_comVel_PreImpact = m_comVelPrevious;
    }

    NmRsCBUFallOverWall* fowTask = (NmRsCBUFallOverWall*)m_cbuParent->m_tasks[bvid_fallOverWall];
    Assert(fowTask);
    bool updatePivotPoint = ((m_balancerState == bal_Drape || m_balancerState == bal_DrapeForward) 
      && fowTask->isActive());

    if ((!m_ubImpactOccurred && !m_block) || updatePivotPoint)//Don't need to look for collisions after an upperbody collision or we've decided it's a block
    {
      //NB: collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex sometimes misses contacts with other things 
      //  as collisionZMP could be made up of other collisions but the last one was with ignoreColWithIndex.

      //In height order to do is it a block?
      //For Thigh collisions - check whether the character has landed on knees on the floor.  If so ignore it.
      float leftFootHeight = m_character->m_gUp.Dot(getLeftLeg()->getFoot()->getPosition());
      float rightFootHeight = m_character->m_gUp.Dot(getRightLeg()->getFoot()->getPosition());
      float floorHeight = rage::Min(leftFootHeight, rightFootHeight);         
      if (getLeftLeg()->getThigh()->collidedWithNotOwnCharacter())
      {
        getLeftLeg()->getThigh()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
        float colHeight = m_character->vectorHeight(collisionPos); 
        //if (collisionInst->GetClassType() == rage::NM_RS_INSTNM_CLASS_TYPE || collisionInst->GetClassType() == rage::PH_INST_FRAG_PED)
        if ((colHeight - floorHeight) > 0.1f && m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex)
        {
          collisionInstOK = collisionInst;
          impactOccurred = true;
          m_lbImpactOccurred = true;
          m_lbCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          height = (height < 11)? 11:height;
        }
      }
      if (getRightLeg()->getThigh()->collidedWithNotOwnCharacter())
      {
        //Check wether the character has landed on knees on the floor.  If so ignore it.
        getRightLeg()->getThigh()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
        float colHeight = m_character->vectorHeight(collisionPos); 
        if ((colHeight - floorHeight) > 0.1f && m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex)
        {
          collisionInstOK = collisionInst;
          impactOccurred = true;
          m_lbImpactOccurred = true;
          m_lbCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          height = (height < 10)? 10:height;
        }

      }
      if (getSpine()->getPelvisPart()->collidedWithNotOwnCharacter())
      {
        getSpine()->getPelvisPart()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
        if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex)
        {
          collisionInstOK = collisionInst;
          impactOccurred = true;
          m_lbImpactOccurred = true;
          m_lbCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          height = (height < 9)? 9:height;
        }
      }
      if (getSpine()->getSpine0Part()->collidedWithNotOwnCharacter())
      {
        getSpine()->getSpine0Part()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
        if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex)
        {
          collisionInstOK = collisionInst;
          impactOccurred = true;
          m_lbImpactOccurred = true;
          m_lbCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          height = (height < 8)? 8:height;
        }
      }
      if (getSpine()->getSpine1Part()->collidedWithNotOwnCharacter())
      {
        getSpine()->getSpine1Part()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
        if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex)
        {
          collisionInstOK = collisionInst;
          impactOccurred = true;
          m_lbImpactOccurred = true;
          m_lbCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          height = (height < 7)? 7:height;
        }
      }
      if (getSpine()->getSpine2Part()->collidedWithNotOwnCharacter())
      {
        getSpine()->getSpine2Part()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
        if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex)
        {
          collisionInstOK = collisionInst;
          impactOccurred = true;
          if (m_parameters.fallOverHighWalls)
          {
            m_lbImpactOccurred = true;
            m_lbCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          }
          else
          {
            m_ubImpactOccurred = true;
            m_ubCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          }
          height = (height < 6)? 6:height;
        }
      }
      if (getSpine()->getSpine3Part()->collidedWithNotOwnCharacter())
      {
        getSpine()->getSpine3Part()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
        if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex)
        {
          collisionInstOK = collisionInst;
          impactOccurred = true;
          m_ubImpactOccurred = true;
          m_ubCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          height = (height < 5)? 5:height;
        }
      }
      if (getRightArm()->getClaviclePart()->collidedWithNotOwnCharacter())
      {
        getRightArm()->getClaviclePart()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
        if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex)
        {
          collisionInstOK = collisionInst;
          impactOccurred = true;
          m_ubImpactOccurred = true;
          m_ubCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          height = (height < 4)? 4:height;
        }
      }
      if (getLeftArm()->getClaviclePart()->collidedWithNotOwnCharacter())
      {
        getLeftArm()->getClaviclePart()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
        if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex)
        {
          collisionInstOK = collisionInst;
          impactOccurred = true;
          m_ubImpactOccurred = true;
          m_ubCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          height = (height < 3)? 3:height;
        }

      }
      if (getSpine()->getNeckPart()->collidedWithNotOwnCharacter())
      {
        getSpine()->getNeckPart()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
        if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex)
        {
          collisionInstOK = collisionInst;
          impactOccurred = true;
          m_ubImpactOccurred = true;
          m_ubCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          height = (height < 2)? 2:height;
        }
      }
      if (getSpine()->getHeadPart()->collidedWithNotOwnCharacter())
      {
        getSpine()->getHeadPart()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst, &collisionInstGenID);
        if (m_character->IsInstValid(collisionInst, collisionInstGenID) && collisionInst->GetLevelIndex() != m_parameters.ignoreColWithIndex)
        {
          collisionInstOK = collisionInst;
          impactOccurred = true;
          m_ubImpactOccurred = true;
          m_ubCollisionNormalTotal += collisionNormal*rage::Abs(depth)*5.f;
          height = (height < 1)? 1:height;
        }
      }
    }

    //TEST CODE for MP3 fow. Changes the fallOverWall pivot point if there is a higher impact.
    //e.g. the knee was hitting the wall and therefore the pivot point was too low - however
    //  this folds and pulls the body towards the wall nicely.  Just need to change the pivot point when 
    //  the upperbody hits the edge of the wall
    bool updatedPivotPoint = false;
    //mmmmtodo make this up aware i.e. assumes up is .z ATM
    rage::Vector3 old2New = collisionPos - m_pos1stContactForFOW;
    bool moveHorizontal = (rage::Abs(old2New.z) < 0.01f) && old2New.Dot(m_normal1stContact) < 0.0f;
    if (updatePivotPoint && (collisionPos.z > m_pos1stContactForFOW.z || moveHorizontal))
    {
      m_character->instanceToLocalSpace(&m_pos1stContactLocalForFOW,collisionPos,m_collisionLevelIndex);
      m_pos1stContactForFOW = collisionPos;
      //Might not do below in the end
      m_drapeTimer = 0.f;//restart the drape time to give fow more time
      updatedPivotPoint = true;
    }
    if (updatedPivotPoint)
    {
      //update fow (for moving blocks)
      fowTask->m_parameters.fallOverWallEndA = m_pos1stContactForFOW;
      rage::Vector3 topOfWall = m_normal1stContact;
      topOfWall.Cross(m_character->m_gUp);
      fowTask->m_parameters.fallOverWallEndB = m_pos1stContactForFOW + topOfWall;
      //because m_normal1stContact never changes ok to not update?
      //fowTask->m_wallNormal
      //fowTask->m_wallEdge
    }

    //TEST CODE END

    rage::Vector3 collisionNormalTotal = m_ubCollisionNormalTotal + m_lbCollisionNormalTotal;
    collisionNormalTotal.Normalize();
    //When the character 1st collides with something
    if ((!m_impactOccurred) && (impactOccurred))//impactOccurred = true ensures that collisionInstOK is valid
    {
      //Get Velocity of object you've hit. 
      m_collisionLevelIndex = collisionInstOK->GetLevelIndex();
      rage::Vector3 collisionObjectVelocity(0.0f,0.0f,0.0f);
      m_character->getVelocityOnInstance(m_collisionLevelIndex,collisionPos,&collisionObjectVelocity);     

      //comVel_PreImpact relative to impact object. Also m_comVel_PreImpact relative to this (used for spin later)
      rage::Vector3 comVel_PreImpact = m_comVel_PreImpact - collisionObjectVelocity;
      m_comVel_PreImpact = comVel_PreImpact;
      comVel_PreImpact.Normalize();

      m_impactOccurred = true;

      m_balancerState = bal_Impact;
      m_numStepsAtImpact = numOfSteps;
      //Work out angle of incidence after initial collision
      //float angleOfIncidence = rage::AcosfSafe(collisionNormalTotal.Dot(-comVel_PreImpact));


      //if 1st collision comes from a block corner then glancing will be wrong
      //if (m_normal1stContactLocal.Dot(m_character->m_gUp) > 0.9f)//25deg cone //could guess block height with 1st one of these
      //can get around this if assume sides are upright and project 1stColNormal onto horizontal plane
      //helped by velocity being projected onto horizontal plane
      //CHeck for near vertical vel or collision.  If so do something else e.g. impact 2 feet levelelled as normal
      collisionNormalTotal.Cross(m_character->m_gUp);
      collisionNormalTotal.CrossSafe(m_character->m_gUp,collisionNormalTotal);
      collisionNormalTotal.Normalize();
      comVel_PreImpact.Cross(m_character->m_gUp);
      comVel_PreImpact.CrossSafe(m_character->m_gUp,comVel_PreImpact);
      comVel_PreImpact.Normalize();

      float angleOfIncidenceHorizontal = rage::AcosfSafe(collisionNormalTotal.Dot(-comVel_PreImpact));

      //2.21rads  <2.5 rads glancing.  >2.5rads rebound forwards, leanagainst backwards
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "GlancingSpin", collisionObjectVelocity);
      bspyScratchpad(m_character->getBSpyID(), "GlancingSpin", collisionNormalTotal);
      bspyScratchpad(m_character->getBSpyID(), "GlancingSpin", -comVel_PreImpact);
      bspyScratchpad(m_character->getBSpyID(), "GlancingSpin", angleOfIncidenceHorizontal);
#endif
      if (angleOfIncidenceHorizontal > 0.6f)//34deg
      {
        m_balancerState = bal_GlancingSpin;           
      }

      m_character->instanceToLocalSpace(&m_pos1stContactLocal,collisionPos,m_collisionLevelIndex);
      m_character->instanceToLocalSpace(&m_normal1stContactLocal,collisionPos+collisionNormalTotal,m_collisionLevelIndex);
      m_normal1stContactLocal -= m_pos1stContactLocal;

      //m_sideOfPlane = m_normal1stContact.Dot(m_comVel_PreImpact-m_pos1stContact); 
      m_sideOfPlane = collisionNormalTotal.Dot(m_comVel_PreImpact); 

      m_pos1stContact = collisionPos;
      m_normal1stContact = collisionNormalTotal;

      m_pos1stContactForFOW = m_pos1stContact;
      m_pos1stContactLocalForFOW = m_pos1stContactLocal;

      //If the upperbody has not contacted yet then we don't know whether it is a wall/table or rail
      //  so do a probe to find out- ans put in m_obstacleType
      //(If the upperbody has contacted then we assume it is a wall)
      //Remove the if to test the probe code - once m_ubImpactOccurred bcr acts like it's a wall no matter what m_obstacleType is 
      if (!m_ubImpactOccurred)
        probeEnvironment(m_pos1stContactForFOW, m_normal1stContact);

      //reduce legstiffness
      dynamicBalancerTask->setLeftLegStiffness(dynamicBalancerTask->getLeftLegStiffness() - m_parameters.impactLegStiffReduction);
      dynamicBalancerTask->setRightLegStiffness(dynamicBalancerTask->getRightLegStiffness() - m_parameters.impactLegStiffReduction);

      //getLeftLeg()->getKnee()->setStiffness(8.3f, 0.9f);
      //getLeftLeg()->getAnkle()->setStiffness(9.f, 1.f);
      //getLeftLeg()->getHip()->setStiffness(9.f, 1.f);

      m_controlStiffnessStrengthScale = 0.01f;

      //SNAP
      //mmmtodo choose direction
      if (m_parameters.snap)
      {
        rage::Vector3 snapDirection;
        rage::Vector3 projOnNormal = m_normal1stContact*(m_comVel_PreImpact.Dot(m_normal1stContact));
        snapDirection = 2.f*projOnNormal - m_comVel_PreImpact;
        snapDirection.Normalize();
        float mag = rage::Abs(m_normal1stContact.Dot(m_comVel_PreImpact));
        mag = rage::Clamp(mag-0.5f, 0.f, 1.f);
        m_character->snap(
          m_parameters.snapMag*mag,
          m_parameters.snapDirectionRandomness, 
          m_parameters.snapHipType,
          m_parameters.snapLeftArm,
          m_parameters.snapRightArm,
          m_parameters.snapLeftLeg,  
          m_parameters.snapRightLeg,  
          m_parameters.snapSpine,  
          m_parameters.snapNeck, 
          m_parameters.snapPhasedLegs, 
          m_parameters.snapUseTorques,
          1.f,
          -1,
          &snapDirection);
      }//if (m_parameters.snap)

    }//if ((!m_impactOccurred) && (impactOccurred))

    if ((m_timeAfterImpact > m_parameters.glanceSpinTime) && ((m_balancerState == bal_GlancingSpin) || (m_balancerState == bal_DrapeGlancingSpin)) )
    {
      m_balancerState = bal_Impact;
    }

    //WorkOut character orientation to collision normal
    rage::Vector3 bodyRight = m_character->m_COMTM.a;
    rage::Vector3 bodyUp = m_character->m_COMTM.b;
    rage::Vector3 bodyBack = m_character->m_COMTM.c;
    float backDotNormal = bodyBack.Dot(collisionNormalTotal);
    float rightDotNormal = bodyRight.Dot(collisionNormalTotal);
    float upDotNormal = bodyUp.Dot(collisionNormalTotal);
    NM_RS_DBG_LOGF(L"    Steps = %i", numOfSteps);
    NM_RS_DBG_LOGF(L"  Steps@Impact = %i", m_numStepsAtImpact);
    NM_RS_DBG_LOGF(L"    Back = %f", backDotNormal);
    NM_RS_DBG_LOGF(L"  Right = %f", rightDotNormal);
    NM_RS_DBG_LOGF(L"    Up = %f", upDotNormal);

    NM_RS_DBG_LOGF(L"    ");
    if (backDotNormal < 0.f)
    {NM_RS_DBG_LOGF(L"    Backwards ");}
    else
    {NM_RS_DBG_LOGF(L"    FORWARDS ");}
    if (rightDotNormal < 0.f)
    {NM_RS_DBG_LOGF(L"  Right");}
    else
    {NM_RS_DBG_LOGF(L"  LEFT ");}
    if (upDotNormal < 0.f)
    {NM_RS_DBG_LOGF(L"    Up");}
    else
    {NM_RS_DBG_LOGF(L"    DOWN");}

    if ((m_ubImpactOccurred) && (m_balancerState == bal_Impact)) //ie not glancing
    {
      if (backDotNormal < 0.f) //if not forwards
        m_balancerState = bal_LeanAgainst;
      //else
      //turn to side, force rebound, brace, less feetaway distance

    }

    if ((m_impactOccurred) && (m_balancerState != bal_GlancingSpin) && (m_balancerState != bal_DrapeGlancingSpin))
    {
      //REBOUND?
      float exclusionZone = 0.4f;
      rage::Vector3 collisionObjectVelocity(0.0f,0.0f,0.0f);
      m_character->getVelocityOnInstance(m_collisionLevelIndex,m_pos1stContact,&collisionObjectVelocity);//m_collisionLevelIndex has already been checked for validity or changed to -1     
      rage::Vector3 comVelDir(m_character->m_COMvel);
      comVelDir -= collisionObjectVelocity;
      comVelDir.Normalize();
      //if comVel moving away from impactPos && Com >x from impactPos
      if ((comVelDir.Dot(m_normal1stContact) > 0.2f) && (m_normal1stContact.Dot(m_character->m_COM-m_pos1stContact-m_normal1stContact*exclusionZone)*m_sideOfPlane < 0.f))//increase offset
      {
        NmRsCBUShot* shotTask = (NmRsCBUShot*)m_cbuParent->m_tasks[bvid_shot];
        Assert(shotTask);
        m_balancerState = bal_Rebound;
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "BCR Rebound", true);
#endif // ART_ENABLE_BSPY

        dynamicBalancerTask->setForceBalance(false);
        if (m_parameters.reboundMode == 0)//fall2knees/slump if shot not running
        {
          if (!shotTask->isActive())//Otherwise fall2knees set in shot
            m_balancerState = bal_Slump;
        }
        else if (m_parameters.reboundMode == 1)//stumble
        {
          NmRsCBUStumble* stumbleTask = (NmRsCBUStumble*)m_cbuParent->m_tasks[bvid_stumble];
          Assert(stumbleTask);
          if (!stumbleTask->isActive())
          {
            stumbleTask->updateBehaviourMessage(NULL); // set parameters to defaults
            stumbleTask->m_parameters.staggerTime = 1.0f;
            stumbleTask->activate();
          }
          if (shotTask->isActive())
            shotTask->deactivate();
        }
        else if (m_parameters.reboundMode == 2)//slump
        {
          m_balancerState = bal_Slump;
        }
        else if (m_parameters.reboundMode == 3)//restart
        {
          NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
          Assert(dynamicBalancerTask);
          NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
          Assert(catchFallTask);

          //mmmmtodo maybe not true - cache these at start?
          //undo impact
          dynamicBalancerTask->setLeftLegStiffness(dynamicBalancerTask->getLeftLegStiffness() + m_parameters.impactLegStiffReduction);//mmmmtodo - cache at start?
          dynamicBalancerTask->setRightLegStiffness(dynamicBalancerTask->getRightLegStiffness() + m_parameters.impactLegStiffReduction);//mmmmtodo - cache at start?
          dynamicBalancerTask->setHipPitch(0.f);//mmmmtodo - cache at start?
          if (dynamicBalancerTask->isActive())
          {
            if (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
              catchFallTask->deactivate();//could have been draping
          }

          //undo slump
          if (m_balancerState == bal_Slump)
          {
            //dynamicBalancerTask->setBalanceTime(0.2f);
            dynamicBalancerTask->setForceBalance(false);
            dynamicBalancerTask->setOpposeGravityAnkles(1.f);//mmmmtodo - cache at start?
            dynamicBalancerTask->setOpposeGravityLegs(1.f);//mmmmtodo - cache at start?
            if (dynamicBalancerTask->isActive())
              dynamicBalancerTask->setLowerBodyGravityOpposition(m_body);
          }
          initialiseCustomVariables();
          resetFrictionMultipliers();
          m_comVelPrevious = m_comVel_PreImpact = m_character->m_COMvel;//better than initializing with zero

          return;
        }

      }
    }

    if ((m_lbImpactOccurred) && (!m_ubImpactOccurred) && (!m_block) && (!m_ignoreBlock))
    {
      //possibly a block
      if (m_obstacleType == BCR_Railing || m_obstacleType == BCR_Table)
        m_block = true;
      else
      {
      //1
      //if "POSITIONS of ub" are infront(decided by m_comVel_PreImpact) of plane (center lb1stCollision and normal m_pos1stContact) then "its a block"
      //Will be fooled by window sills if hit edge.
      if (m_normal1stContact.Dot(getSpine()->getSpine1Part()->getPosition()-m_pos1stContact)*m_sideOfPlane > 0.f)//increase offset
        m_block = true;
      else if (m_normal1stContact.Dot(getSpine()->getSpine2Part()->getPosition()-m_pos1stContact)*m_sideOfPlane > 0.f)//increase offset
        m_block = true;
      else if (m_normal1stContact.Dot(getSpine()->getSpine3Part()->getPosition()-m_pos1stContact)*m_sideOfPlane > 0.f)//increase offset
        m_block = true;
      else if (m_normal1stContact.Dot(getLeftArm()->getClaviclePart()->getPosition()-m_pos1stContact)*m_sideOfPlane > 0.f)//increase offset
        m_block = true;
      else if (m_normal1stContact.Dot(getRightArm()->getClaviclePart()->getPosition()-m_pos1stContact)*m_sideOfPlane > 0.f)//increase offset
        m_block = true;
      else if (m_normal1stContact.Dot(getLeftArm()->getShoulder()->getJointPosition()-m_pos1stContact)*m_sideOfPlane > 0.f)//increase offset
        m_block = true;
      else if (m_normal1stContact.Dot(getRightArm()->getShoulder()->getJointPosition()-m_pos1stContact)*m_sideOfPlane > 0.f)//increase offset
        m_block = true;
      //2 not needed as velocity and normal no projected onto horizontal plane
      //if the 1st collision normal is nearly upright then probably hit the top edge of a block (above code will not work in this case)
      //if (m_normal1stContact.Dot(m_character->m_gUp) > 0.9f)//25deg cone //could guess block height with 1st one of these
      //  m_block = true;
      }

      //3
      //Advance warning for block in cases where upperbody not over the top of it yet

      if (m_block)
      {
        //mmmmtodo perhaps bal_DrapeGlancingSpin should do fallOverWall?
        if (m_balancerState == bal_GlancingSpin)
        {
          m_balancerState = bal_DrapeGlancingSpin;
        }
        else if ((m_balancerState == bal_LeanAgainst) || (m_balancerState == bal_Impact))
        {
          rage::Vector3 hip2Hip = getRightLeg()->getHip()->getJointPosition() -  getLeftLeg()->getHip()->getJointPosition();
          rage::Vector3 sideVector;
          sideVector = m_normal1stContact;
          sideVector.Cross(m_character->m_gUp);
          if (sideVector.Dot(hip2Hip) < 0.f)
            m_balancerState = bal_DrapeForward;
          else
            m_balancerState = bal_Drape;
          if (m_parameters.fallOverWallDrape)
          {
            NmRsCBUFallOverWall* fowTask = (NmRsCBUFallOverWall*)m_cbuParent->m_tasks[bvid_fallOverWall];
            Assert(fowTask);
            if (!fowTask->isActive())
            {
              fowTask->m_parameters.fallOverWallEndA = m_pos1stContactForFOW;
              rage::Vector3 topOfWall = m_normal1stContact;
              topOfWall.Cross(m_character->m_gUp);
              fowTask->m_parameters.fallOverWallEndB = m_pos1stContactForFOW + topOfWall;
              fowTask->activate();
            }
          }

        }
        //else if (m_balancerState == bal_LeanAgainstStable)
        //{
        //  //low speed drape or no drape
        //}
      }

    }

    //mmmmtodo don't always do this - maybe spin till not side on?
    if (m_balancerState == bal_DrapeGlancingSpin && (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK) && m_block)
    {
      rage::Vector3 hip2Hip = getRightLeg()->getHip()->getJointPosition() -  getLeftLeg()->getHip()->getJointPosition();
      rage::Vector3 sideVector;
      sideVector = m_normal1stContact;
      sideVector.Cross(m_character->m_gUp);
      if (sideVector.Dot(hip2Hip) < 0.f)
        m_balancerState = bal_DrapeForward;
      else
        m_balancerState = bal_Drape;
      if (m_parameters.fallOverWallDrape)
      {
        NmRsCBUFallOverWall* fowTask = (NmRsCBUFallOverWall*)m_cbuParent->m_tasks[bvid_fallOverWall];
        Assert(fowTask);
        if (!fowTask->isActive())
        {
          fowTask->m_parameters.fallOverWallEndA = m_pos1stContactForFOW;
          rage::Vector3 topOfWall = m_normal1stContact;
          topOfWall.Cross(m_character->m_gUp);
          fowTask->m_parameters.fallOverWallEndB = m_pos1stContactForFOW + topOfWall;
          fowTask->activate();
        }
      }
      //m_balancerState = bal_DrapeForward;
    }

    //Apply cheat torques for spin
    rage::Vector3 velAlongWall = m_normal1stContact;
    velAlongWall *= -m_character->m_COMvel.Dot(m_normal1stContact);
    velAlongWall += m_character->m_COMvel;
    float velReduced = rage::Min(velAlongWall.Mag()*0.25f, 1.f);
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "GlancingSpin", velReduced);
#endif
    float spinMag = m_parameters.glanceSpinMag * velReduced;

    if ((m_timeAfterImpact < m_parameters.glanceSpinTime) && ((m_balancerState == bal_GlancingSpin) || (m_balancerState == bal_DrapeGlancingSpin)) )
    {
      //copied from shot
      rage::Vector3 torqueVector;
      rage::Vector3 spine0Pos;
      rage::Vector3 spine3Pos;
      float direction = 1.f;
      torqueVector.Cross(m_normal1stContact,m_comVel_PreImpact);//m_normal1stContact can be moving with impacted object (could lead switching of spin direction therefore mmmmtodo remember direction?)
      if (torqueVector.Dot(m_character->m_gUp) < 0.f)
        direction = -1.f;
      spine0Pos = getSpine()->getSpine0()->getJointPosition();
      spine3Pos = getSpine()->getSpine3()->getJointPosition();
      torqueVector = spine3Pos - spine0Pos;
      torqueVector.Normalize();
      spinMag *= (m_parameters.glanceSpinTime - m_parameters.glanceSpinDecayMult*m_timeAfterImpact)/m_parameters.glanceSpinTime;
      spinMag = rage::Max(0.f,spinMag);
      NM_RS_DBG_LOGF(L"spinMag = %f", spinMag);
      torqueVector.Scale(direction*spinMag);
      if (m_balancerState == bal_DrapeGlancingSpin)
      {
        getSpine()->getSpine0Part()->applyTorque(torqueVector);

        torqueVector.Scale(0.5f);
        getSpine()->getSpine1Part()->applyTorque(torqueVector);
        getSpine()->getPelvisPart()->applyTorque(torqueVector);
      }
      else
      {
        getSpine()->getSpine2Part()->applyTorque(torqueVector);

        torqueVector.Scale(0.5f);
        getSpine()->getSpine1Part()->applyTorque(torqueVector);
        getSpine()->getSpine3Part()->applyTorque(torqueVector);

        torqueVector.Scale(0.5f);
        getSpine()->getSpine0Part()->applyTorque(torqueVector);
      }

    }

    sendBalanceStateFeedback();

    //do stuff that might change the balancerState Again but you want the change reported next step
    if (m_balancerState == bal_Drape || m_balancerState == bal_DrapeForward)
      drape(timeStep);

    if (m_balancerState == bal_LeanAgainstStable)
    {
      if (m_stableTimer > m_parameters.stable2SlumpTime)
        m_balancerState = bal_Slump;

      m_stableTimer += timeStep;
    }

    m_comVelPrevious = m_character->m_COMvel;

  }

  void NmRsCBUBalancerCollisionsReaction::sendBalanceStateFeedback()
  {
    if (m_previousBalancerState != m_balancerState)
    {
      //send a feedback message;
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 1;

        ART::ARTFeedbackInterface::FeedbackUserdata data;
        data.setInt(m_balancerState);
        feedback->m_args[0] = data;

        strcpy(feedback->m_behaviourName, NMBalanceStateFeedbackName);
        feedback->onBehaviourEvent();
      }

    }
    m_previousBalancerState = m_balancerState;

  }

  /*
  */
  void NmRsCBUBalancerCollisionsReaction::defaultArmMotion_armsBrace(float /*timeStep*/)    
  {
#if 0//Turned off defaultArmMotion
    const float scaleRightness = 1.f;
    const float scaleBackwardsRightness = 0.5f;
    const float scaleContraRightness = 0.8f;
    //const float backwardsScale = 0.5f;//1 has no problems 

    //Set muscle stiffnesses here for clarity
    float armStiffness = 11.f;
    const float armDamping = 0.7f;
    //if(m_defaultArmMotion.leftBrace)
    {
      m_character->setBodyStiffness(armStiffness, armDamping, "ul");
      getLeftArm()->getElbow()->setStiffness(0.75f*armStiffness, 0.75f*armDamping);
      getLeftArm()->getWrist()->setStiffness(armStiffness - 1.0f, 1.75f);
    }
    //if(m_defaultArmMotion.rightBrace)
    {
      m_character->setBodyStiffness(armStiffness, armDamping, "ur");
      getRightArm()->getElbow()->setStiffness(armStiffness*0.75f, 0.75f*armDamping);
      getRightArm()->getWrist()->setStiffness(armStiffness - 1.0f, 1.75f);
    }

    rage::Vector3 braceTarget;
    static float maxArmLength = 0.61f;//0.55 quite bent, 0.6 Just bent, 0.7 straight
    rage::Matrix34 tmCom;
    getSpine()->getSpine1Part()->getBoundMatrix(&tmCom); 
    rage::Vector3 vec;
    float mag, armTwist;//, straightness;
    float dragReduction = 1.f;
    //float maxSpeed = 5.f;//from balance 200.f; //from catch fall - ie out of range
    rage::Vector3 targetVel;
    //float magForwards = rage::Clamp(m_character->m_COMrotvel.Dot(tmCom.b),0.f,4.f);
    //magForwards *=0.25;

    rage::Vector3 rightDir1 = -tmCom.b;
    rage::Vector3 forwardDir1 = -tmCom.c;
    m_character->levelVector(rightDir1);
    m_character->levelVector(forwardDir1);


    rage::Vector3 reachVel = getSpine()->getSpine3Part()->getLinearVelocity();//m_character->m_COMvel;
    m_character->levelVector(reachVel);
    getLeftLeg()->getThigh()->getBoundMatrix(&tmCom);
    rage::Vector3 rightDir = tmCom.a;
    rage::Vector3 forwardDir = -tmCom.c;
    m_character->levelVector(rightDir);
    m_character->levelVector(forwardDir);

    float rightness = scaleRightness*reachVel.Dot(rightDir)*2.f;;
    float forwardness = reachVel.Dot(forwardDir)*2.f;
    bool goingBackwards = false;
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", rightness);
    bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", forwardness);
#endif // ART_ENABLE_BSPY
    if (forwardness < 0.f)
    {
      //forwardness = 0.f;//both these work 0 gives more arms out but more arms back. Leaving it as a negative number makes a backward moving bending forward look odd
      //forwardness = -backwardsScale*forwardness;//0..1  actually 2 is ok.
      goingBackwards = true;
    }
    else
      forwardness *= 2.f;

    float feetHeight = 0.5f*(getLeftLeg()->getFoot()->getPosition().Dot(m_character->m_gUp) + getRightLeg()->getFoot()->getPosition().Dot(m_character->m_gUp));
    float leftHandRelativeHeight = rage::Max(0.f,getRightArm()->getHand()->getPosition().Dot(m_character->m_gUp) - feetHeight);
    float rightHandRelativeHeight = rage::Max(0.f,getRightArm()->getHand()->getPosition().Dot(m_character->m_gUp) - feetHeight);


    //LEFT ARM
    //if(m_defaultArmMotion.leftBrace)
    {
      armTwist = -0.5f;//arms in front or to side, 0 = arms a bit wider(1.0f for behind)
      braceTarget = -m_character->m_gUp;
      if (rightness > 0.f)
      {
        rightness *= scaleContraRightness*rage::Sinf(3.f*m_defaultArmMotiontimer1);
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
      mag = braceTarget.Mag();
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
      m_character->C_LimbIK(getLeftArm(), 1.f, 1.f, false, &braceTarget, &armTwist, NULL, NULL, &targetVel, NULL, NULL, NULL);       
      m_character->matchClavicleToShoulderBetter(getLeftArm()->getClavicle(),getLeftArm()->getShoulder());
      // by default, characters tip hands up in preparation for connecting with object
      float desiredLean2 = 0.f;
      if (leftHandRelativeHeight < 0.7f)
      {
        desiredLean2 -= 2.5f*(0.7f-leftHandRelativeHeight);
      }
      getLeftArm()->getWrist()->setDesiredLean2(desiredLean2);
      if (goingBackwards)
      {
        nmrsSetAngle(getLeftArm()->getElbow(), nmrsGetDesiredAngle(getLeftArm()->getElbow()) + 0.5f*rage::Sinf(5.f*m_defaultArmMotiontimer2));
      }
      nmrsSetAngle(getLeftArm()->getElbow(), rage::Max(nmrsGetDesiredAngle(getLeftArm()->getElbow()), 0.3f));

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
      //forwardness = -backwardsScale*forwardness;//0..1  actually 2 is ok.
      goingBackwards = true;
    }
    //if(m_defaultArmMotion.rightBrace)
    {
      armTwist = -0.5f;//arms in front or to side, 0 = arms a bit wider(1.0f for behind)
      braceTarget = -m_character->m_gUp;
      if (rightness < 0.f)
      {
        rightness *= scaleContraRightness*rage::Sinf(3.f*m_defaultArmMotiontimer2);
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
      mag = braceTarget.Mag();
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
      m_character->C_LimbIK(getRightArm(), -1.f, 1.f, false, &braceTarget, &armTwist, &dragReduction, NULL, &targetVel, NULL, NULL, NULL);       
      m_character->matchClavicleToShoulderBetter(getRightArm()->getClavicle(),getRightArm()->getShoulder());
      float desiredLean2 = 0.f;
      if (rightHandRelativeHeight < 0.7f)
      {
        desiredLean2 -= 2.5f*(0.7f-rightHandRelativeHeight);
      }
      getRightArm()->getWrist()->setDesiredLean2(desiredLean2);
      if (goingBackwards)
      {
        nmrsSetAngle(getRightArm()->getElbow(), nmrsGetDesiredAngle(getRightArm()->getElbow()) + 0.5f*rage::Sinf(5.f*m_defaultArmMotiontimer1));
      }
      nmrsSetAngle(getRightArm()->getElbow(), rage::Max(nmrsGetDesiredAngle(getRightArm()->getElbow()), 0.3f));
    }
#endif
  }


  void NmRsCBUBalancerCollisionsReaction::slump(float timeStep)
  {
    float footFrictionMultStart = m_parameters.footFrictionMultStart;
    float footFrictionMultRate = m_parameters.footFrictionMultRate;
    float backFrictionMultStart = m_parameters.backFrictionMultStart;
    float backFrictionMultRate = m_parameters.backFrictionMultRate;
    float slumpLegStiffRate = m_parameters.slumpLegStiffRate;
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    dynamicBalancerTask->setForceBalance(true);
    if (!m_slump)
    {
      m_slump = true;
      //Decide slump side
      rage::Vector3 left2Right = getRightLeg()->getFoot()->getPosition() - getLeftLeg()->getFoot()->getPosition();
      float goLeft = left2Right.Dot(m_character->m_COM - getLeftLeg()->getFoot()->getPosition());
      float goRight = left2Right.Dot(getRightLeg()->getFoot()->getPosition() - m_character->m_COM);
      //ramp down stiffnesses, put on both legs, decrease friction if movement doesn't increase
      m_amountOfMovement = m_character->getKineticEnergyPerKilo_RelativeVelocity();
      m_footFrictionMult = footFrictionMultStart;
      m_backFrictionMult = backFrictionMultStart;

      m_slumpStiffLLeg = sqrt(getLeftLeg()->getHip()->getMuscleStrength());
      m_slumpStiffRLeg = sqrt(getRightLeg()->getHip()->getMuscleStrength());
      m_slumpStiffLKnee = sqrt(getLeftLeg()->getKnee()->getMuscleStrength());
      m_slumpStiffRKnee = sqrt(getRightLeg()->getKnee()->getMuscleStrength());
      //make static friction and slump params and add num steps till slump enforced
      if (goLeft < goRight)
      {
        m_slumpStiffLLeg -= m_parameters.slumpLegStiffReduction;
        m_slumpStiffLKnee -= m_parameters.slumpLegStiffReduction;
        if (m_character->getRandom().GetFloat() > 0.8f)
        {
          m_slumpStiffRLeg -= m_parameters.slumpLegStiffReduction * m_character->getRandom().GetRanged(0.f,1.f);
          m_slumpStiffRKnee -= m_parameters.slumpLegStiffReduction * m_character->getRandom().GetRanged(0.f,1.f);
        }
      }
      else
      {
        m_slumpStiffRLeg -= m_parameters.slumpLegStiffReduction;
        m_slumpStiffRKnee -= m_parameters.slumpLegStiffReduction;
        if (m_character->getRandom().GetFloat() > 0.8f)
        {
          m_slumpStiffLLeg -= m_parameters.slumpLegStiffReduction * m_character->getRandom().GetRanged(0.f,1.f);
          m_slumpStiffLKnee -= m_parameters.slumpLegStiffReduction * m_character->getRandom().GetRanged(0.f,1.f);
        }
      }
    }
    float amountOfMovement = m_character->getKineticEnergyPerKilo_RelativeVelocity();
    //if the character is moving less than before increase his slump chances by reducing stiffness/friction
    if ((m_amountOfMovement >= amountOfMovement && amountOfMovement < 0.4f)//mmmmtodo tune the 0.6
      || m_parameters.slumpMode == 1
      || (m_parameters.slumpMode == 2 && m_amountOfMovement >= amountOfMovement) 
      )
    {
      m_slumpStiffRLeg -= timeStep*slumpLegStiffRate;
      m_slumpStiffLLeg -= timeStep*slumpLegStiffRate;
      m_slumpStiffRKnee -= timeStep*slumpLegStiffRate;
      m_slumpStiffLKnee -= timeStep*slumpLegStiffRate;
      m_footFrictionMult -= timeStep*footFrictionMultRate;
      m_backFrictionMult -= timeStep*backFrictionMultRate;
    }
    m_amountOfMovement = amountOfMovement;
    //Apply friction changes
    m_footFrictionMult = rage::Max(0.05f,m_footFrictionMult);
    m_backFrictionMult = rage::Max(0.3f,m_backFrictionMult);
    getLeftLeg()->getFoot()->setFrictionMultiplier(m_footFrictionMult);
    getRightLeg()->getFoot()->setFrictionMultiplier(m_footFrictionMult);
    getSpine()->getPelvisPart()->setFrictionMultiplier(m_backFrictionMult);
    getSpine()->getSpine0Part()->setFrictionMultiplier(m_backFrictionMult);
    getSpine()->getSpine1Part()->setFrictionMultiplier(m_backFrictionMult);
    getSpine()->getSpine2Part()->setFrictionMultiplier(m_backFrictionMult);
    getSpine()->getSpine3Part()->setFrictionMultiplier(m_backFrictionMult);
    getLeftArm()->getClaviclePart()->setFrictionMultiplier(m_backFrictionMult);
    getRightArm()->getClaviclePart()->setFrictionMultiplier(m_backFrictionMult);

    //Apply muscle stengths.  Take min of slump values and current(coming from eg. staggerFall)
    float ankleStiffnessL = rage::Min(sqrt(getLeftLeg()->getAnkle()->getMuscleStrength()), m_slumpStiffLLeg+2.f);
    float kneeStiffnessL = rage::Min(sqrt(getLeftLeg()->getKnee()->getMuscleStrength()),m_slumpStiffLKnee);
    float hipStiffnessL = rage::Min(sqrt(getLeftLeg()->getHip()->getMuscleStrength()),m_slumpStiffLLeg);
    float ankleStiffnessR = rage::Min(sqrt(getRightLeg()->getAnkle()->getMuscleStrength()),m_slumpStiffRLeg+2.f);
    float kneeStiffnessR = rage::Min(sqrt(getRightLeg()->getKnee()->getMuscleStrength()),m_slumpStiffRKnee);
    float hipStiffnessR = rage::Min(sqrt(getRightLeg()->getHip()->getMuscleStrength()),m_slumpStiffRLeg);

    //Don't reduce so much they go unstable
    ankleStiffnessL = rage::Max(7.f,ankleStiffnessL);
    kneeStiffnessL = rage::Max(5.f,kneeStiffnessL);
    hipStiffnessL = rage::Max(5.f,hipStiffnessL);
    ankleStiffnessR = rage::Max(7.f,ankleStiffnessR);
    kneeStiffnessR = rage::Max(5.f,kneeStiffnessR);
    hipStiffnessR = rage::Max(5.f,hipStiffnessR);

    getLeftLegInputData()->getAnkle()->setStiffness(ankleStiffnessL, 1.f);
    getLeftLegInputData()->getKnee()->setStiffness(kneeStiffnessL, 0.9f);
    getLeftLegInputData()->getHip()->setStiffness(hipStiffnessL, 1.f);

    getRightLegInputData()->getAnkle()->setStiffness(ankleStiffnessR, 1.f);
    getRightLegInputData()->getKnee()->setStiffness(kneeStiffnessR, 0.9f);
    getRightLegInputData()->getHip()->setStiffness(hipStiffnessR, 1.f);

  }

  void NmRsCBUBalancerCollisionsReaction::exaggerateImpact()
  {
#if 0//turned off exaggerateImpact
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    //mmmmtodo writhe causes the right arm to always come up and look samey
    //NmRsCBUBodyWrithe* writheTask = (NmRsCBUBodyWrithe*)m_cbuParent->m_tasks[bvid_bodyWrithe];
    //Assert(writheTask);
    NmRsCBUFlinch* flinchTask = (NmRsCBUFlinch*)m_cbuParent->m_tasks[bvid_upperBodyFlinch];
    Assert(flinchTask);
    m_pushingOff = false;
    if (m_timeAfterImpact > 0.0001f && (m_timeAfterImpact < m_parameters.reactTime || (m_timeAfterImpact < 2.0f*m_parameters.reactTime && m_balancerState != bal_LeanAgainst)) && dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
    {
      m_pushingOff = true;
      //writheTask->updateBehaviourMessage(NULL); // set parameters to defaults
      //writheTask->m_parameters.m_armPeriod = 3.f;
      //writheTask->m_parameters.m_backPeriod = 5.f;
      //writheTask->m_parameters.m_legPeriod = 4.f;
      //writheTask->m_parameters.m_armAmplitude = 1.f;
      //writheTask->m_parameters.m_backAmplitude = 2.f;
      //writheTask->m_parameters.m_legAmplitude = 0.5f;
      //writheTask->m_parameters.m_armStiffness = 13.f;
      //writheTask->m_parameters.m_backStiffness = 13.f;
      //writheTask->m_parameters.m_legStiffness = 11.f;
      //writheTask->m_parameters.m_armDamping = 0.3f;
      //writheTask->m_parameters.m_backDamping = 0.3f;
      //writheTask->m_parameters.m_legDamping = 0.3f;
      //writheTask->m_parameters.m_rollOverFlag = false;
      //writheTask->m_parameters.m_effectorMask = bvmask_UpperBody;
      //writheTask->m_parameters.m_blendBack = 0.3f;
      //writheTask->m_parameters.m_blendArms = 0.7f;
      //writheTask->m_parameters.m_applyStiffness = false;

      //if(!writheTask->isActive())
      //  writheTask->activate(m_taskParent);

      flinchTask->updateBehaviourMessage(NULL); // set parameters to defaults
      flinchTask->m_parameters.m_pos = m_pos1stContact;
      flinchTask->m_parameters.m_turnTowards = -1;//turn away
      flinchTask->m_parameters.m_noiseScale = 0.8f;
      flinchTask->m_parameters.m_bodyStiffness = 13.f;
      flinchTask->m_parameters.m_applyStiffness = false;
      flinchTask->m_parameters.m_dontBraceHead = true;

      if(!flinchTask->isActive())
        flinchTask->activate();

      if (m_timeAfterImpact < m_parameters.reactTime*0.5f)
      {
        float muscleStiff = 1.f;
        m_character->setBodyStiffness(15.f,0.7f,"ub", &muscleStiff);//mmmmtodo make parameter
      }
      else
      {
        m_character->setBodyStiffness(13.f,0.7f,"ub");//mmmmtodo make parameter
      }
    }
    else
    {
      //if(writheTask->isActive())
      //  writheTask->deactivate();
      if(flinchTask->isActive())
        flinchTask->deactivate();
    }

    //if (m_timeAfterImpact > 0.0001f && m_timeAfterImpact < m_parameters.impactExagTime && dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
    if (m_timeAfterImpact > 0.0001f && m_timeAfterImpact < m_parameters.reactTime && dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
    {

      float backDotVel;
      float sideDotVel;
      rage::Matrix34 tmCom = m_character->m_COMTM;
      rage::Vector3 bodyRight = tmCom.a;
      rage::Vector3 bodyBack = tmCom.c;
      rage::Vector3 comVel = m_normal1stContact;//getSpine()->getSpine3Part()->getLinearVelocity();
      if (m_timeAfterImpact > m_parameters.reactTime/2.f)
        comVel *= -1.f;
      float speed = rage::Clamp(m_comVel_PreImpact.Dot(m_normal1stContact),-3.f,0.f);
      speed *= -0.5f*rage::Max(0.f,1.f-m_timeAfterImpact/m_parameters.reactTime);//max 0.6

      comVel.Normalize();
      backDotVel = bodyBack.Dot(comVel);
      sideDotVel = bodyRight.Dot(comVel);
      float lean1 = 0.0f; 
      float lean2 = 0.0f;
      lean1 = speed*backDotVel; 
      if (rage::Abs(backDotVel) < 0.8f && sideDotVel < 0.f)               
        lean2 = -speed*rage::Abs(sideDotVel);//left
      if (rage::Abs(backDotVel) < 0.8f && sideDotVel > 0.f)               
        lean2 = speed*rage::Abs(sideDotVel);//right
      m_character->applySpineLean(lean1, lean2);
      m_character->setNeckAngles(lean1, lean2,0.f);
    }
    else if (m_timeAfterImpact > 0.0001f && (m_timeAfterImpact < m_parameters.reactTime + 2.0f*timeStep))
      //else if (m_timeAfterImpact > 0.0001f && (m_timeAfterImpact < m_parameters.impactExagTime + 2.0f*timeStep))
    {
      m_character->applySpineLean(0.f, 0.f);
      m_character->setNeckAngles(0.f, 0.f,0.f);
    }
#endif
  }

  void NmRsCBUBalancerCollisionsReaction::setHipPitch(float timeStep)
  {
    rage::Vector3 hip2Neck = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
    rage::Vector3 hip2Hip = getRightLeg()->getHip()->getJointPosition() -  getLeftLeg()->getHip()->getJointPosition();
    rage::Vector3 forwards;
    hip2Neck.Normalize();
    hip2Hip.Normalize();
    forwards.Cross(hip2Neck,hip2Hip);
    forwards.Normalize();
    float spineToWall = rage::AcosfSafe(hip2Neck.Dot(m_character->m_gUp));
    float hipPitch = spineToWall + 0.1f*(spineToWall - m_spineToWall)/timeStep;
    hipPitch *= 1.5f*(1.f - hip2Hip.Dot(m_normal1stContact));
    m_spineToWall = spineToWall;//Could be in bal_Impact but need frame before to do rate
    if ((m_balancerState == bal_LeanAgainst) 
      || (m_balancerState == bal_LeanAgainstStable)
      || (m_balancerState == bal_Slump))
    {
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      if (forwards.Dot(m_character->m_gUp) < 0.f)
        hipPitch *= -0.0f;
      //hipPitch = rage::Min(hipPitch,0.5f);
      dynamicBalancerTask->setHipPitch(-hipPitch);
      NM_RS_DBG_LOGF(L"LeanAgainstWall    > hipPitch = %.4f", hipPitch);
    }
  }

  void NmRsCBUBalancerCollisionsReaction::setBalanceTime()
  {
    //NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    //Assert(dynamicBalancerTask);
    ////Randomize this (via an activate randomized variable) or have as parameter - both look good
    //if ((m_balancerState == bal_GlancingSpin) || (m_balancerState == bal_DrapeGlancingSpin) )
    //  dynamicBalancerTask->setBalanceTime(0.0f);
    ////if (m_balancerState == bal_LeanAgainst || m_balancerState == bal_LeanAgainstStable)
    ////  dynamicBalancerTask->setBalanceTime(0.1f);//mmmmRandomize this?
  }

  void NmRsCBUBalancerCollisionsReaction::resetFrictionMultipliers()
  {
    getLeftLeg()->getFoot()->setFrictionMultiplier(1.f);
    getRightLeg()->getFoot()->setFrictionMultiplier(1.f);

    getSpine()->getPelvisPart()->setFrictionMultiplier(1.f);
    getSpine()->getSpine0Part()->setFrictionMultiplier(1.f);
    getSpine()->getSpine1Part()->setFrictionMultiplier(1.f);
    getSpine()->getSpine2Part()->setFrictionMultiplier(1.f);
    getSpine()->getSpine3Part()->setFrictionMultiplier(1.f);
    getLeftArm()->getClaviclePart()->setFrictionMultiplier(1.f);
    getRightArm()->getClaviclePart()->setFrictionMultiplier(1.f);
  }

  void NmRsCBUBalancerCollisionsReaction::drape(float timeStep)
  {
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    NmRsCBUBodyFoetal* bodyFoetalTask = (NmRsCBUBodyFoetal*)m_cbuParent->m_tasks[bvid_bodyFoetal];
    Assert(bodyFoetalTask);
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
    Assert(rdsTask);
    NmRsCBUFallOverWall* fowTask = (NmRsCBUFallOverWall*)m_cbuParent->m_tasks[bvid_fallOverWall];
    Assert(fowTask);

    static bool hitClavicles = true;
    static bool rampDown = true;
    bool spine3HasCollided = false;
    if (hitClavicles)
    {
      if (getLeftArm()->getClaviclePart()->collidedWithNotOwnCharacter())        
      {
        spine3HasCollided = true;
      }
      if (getRightArm()->getClaviclePart()->collidedWithNotOwnCharacter())        
      {
        spine3HasCollided = true;
      }

    }
    else
    {
      if (getSpine()->getSpine3Part()->collidedWithNotOwnCharacter())        
      {
        rage::Vector3 collisionNormal;//module level
        rage::Vector3 collisionPos;
        float depth = 0;
        rage::phInst *collisionInst = NULL;

        getSpine()->getSpine3Part()->getCollisionZMPWithNotOwnCharacter(collisionPos, collisionNormal,&depth,&collisionInst);
        if (depth > 0.0f)
          spine3HasCollided = true;
      }
    }
    bool ubCollision = getSpine()->getHeadPart()->collidedWithNotOwnCharacter() || //mmmmfow
      spine3HasCollided;//mmmmfow

    if (fowTask->isActive())
    {
      // Measure if head is above the hinge.
      // Head has to be above the hinge to ramp the force down. We want it to ramp down if it hits the top of say a table.
      rage::Vector3 wallHitPos2HeadPos = fowTask->getWallHitPos();
      const rage::Vector3 headPos = getSpine()->getHeadPart()->getPosition();
      wallHitPos2HeadPos -= headPos;
      const bool headAboveHinge = (wallHitPos2HeadPos.Dot(m_character->m_gUp) > -0.1f) ? true : false; // Add some extra for tables at angle for example.

      if (ubCollision && !m_headCollided && headAboveHinge)//mmmmfow
      {
        //m_drapeTimer += m_character->getRandom().GetRanged(0.f, 0.2f);
        if (rampDown)
          fowTask->m_parameters.magOfForce *= 0.95f;
        else
        {
          m_drapeTimer += m_character->getRandom().GetRanged(0.f, 0.2f);
          m_headCollided = true;
        }
      }
      //Do some more intelligent things with the fow feedback 
      //  e.g. fow_OverTheWall no slump just catchFall
      //       fow_StuckOnWall slump?
      if (fowTask->getFallOverWallState() == ART::NmRsCBUFallOverWall::fow_Aborted
          || fowTask->getFallOverWallState() == ART::NmRsCBUFallOverWall::fow_OverTheWall
          || fowTask->getFallOverWallState() == ART::NmRsCBUFallOverWall::fow_RollingBack)
      {
        m_balancerState = bal_LeanAgainst;
        fowTask->deactivate();
        dynamicBalancerTask->setForceBalance(false);//mmmmfow
      }
      else if (m_headCollided)// && dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)//mmmmfow
      {
        fowTask->deactivate();
        dynamicBalancerTask->setForceBalance(false);//mmmmfow
      }
    }
    else
    {
      m_drapeTimer += timeStep;
      if (m_drapeTimer > 0.3f && dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
      {
        m_balancerState = bal_LeanAgainst;
        m_ignoreBlock = true;
        rdsTask->deactivate();
        bodyFoetalTask->deactivate();
        catchFallTask->deactivate();
      }

      if ((m_balancerState == bal_Drape))
      {
        dynamicBalancerTask->setPlantLeg(false);
        rage::Vector3 angVel = getSpine()->getSpine3Part()->getAngularVelocity();
        angVel += getSpine()->getSpine2Part()->getAngularVelocity();
        angVel += getSpine()->getSpine1Part()->getAngularVelocity();
        rage::Vector3 hip2Hip = getRightLeg()->getHip()->getJointPosition() -  getLeftLeg()->getHip()->getJointPosition();
        rage::Vector3 sideVector;
        sideVector = m_normal1stContact;
        sideVector.Cross(m_character->m_gUp);


        //////Apply cheat torques for drape
        ////rage::Vector3 torqueVector;
        ////torqueVector = m_normal1stContact;
        ////torqueVector.Cross(m_character->m_gUp);
        ////rage::Vector3 hipLeft, hipRight, hip2Spine;
        ////hipLeft = getLeftLeg()->getHip()->getJointPosition();
        ////hipRight = getRightLeg()->getHip()->getJointPosition();
        ////hip2Spine = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
        ////hip2Spine.Normalize();
        //////if ((m_drapeTimer < 0.3f) && (torqueVector.Dot(hipRight-hipLeft) > 0.f))//if backwards or sideways
        ////if ((torqueVector.Dot(hipRight-hipLeft) > 0.f))//if backwards or sideways
        ////{
        ////  torqueVector *= 1.0f - torqueVector.Dot(hip2Spine)*torqueVector.Dot(hip2Spine);
        ////  torqueVector *= 50.f * (bodyFoetalTask->m_parameters.m_stiffness - 6.f)*0.25f;
        ////  getSpine()->getSpine0Part()->applyTorque(torqueVector);
        ////  getSpine()->getSpine1Part()->applyTorque(torqueVector);
        ////}

        if (ubCollision && !m_headCollided)//mmmmfow
        {
          m_drapeTimer += m_character->getRandom().GetRanged(0.f, 0.2f);
          m_headCollided = true;
        }

        static bool useFoetal = false;
        if (useFoetal)
        {
          if (!bodyFoetalTask->isActive())
          {
            bodyFoetalTask->updateBehaviourMessage(NULL); // sets values to defaults
            bodyFoetalTask->m_parameters.m_stiffness = 10.f;
            bodyFoetalTask->m_parameters.m_damping = 0.8f;
            bodyFoetalTask->m_parameters.m_effectorMask = bvmask_LowSpine;
            bodyFoetalTask->activate();
          }
          else
          {
            if ((dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK))
            {
              bodyFoetalTask->m_parameters.m_damping = 0.8f;
              bodyFoetalTask->m_parameters.m_effectorMask = bvmask_Full;
              if (dynamicBalancerTask->isActive())
                dynamicBalancerTask->forceFail();
            }
          }

          //if ( (m_drapeTimer > 0.0f))//MMMMTodo Make this larger for forwards into block?
          //{
          //  if (dynamicBalancerTask->isActive())
          //    dynamicBalancerTask->forceFail();
          //}
          //getLeftLeg()->getHip()->setStiffness(3.f, 1.f);
          //getRightLeg()->getHip()->setStiffness(3.f, 1.f);

          bodyFoetalTask->m_parameters.m_stiffness = rage::Max(6.f,bodyFoetalTask->m_parameters.m_stiffness - 3.f*timeStep);

        }
        else
        {
          if (!rdsTask->isActive())
          {
            rdsTask->updateBehaviourMessage(NULL); // sets values to defaults

            rdsTask->m_parameters.m_AsymmetricalLegs = 0.4f;
            rdsTask->m_parameters.m_Stiffness = 10.f;
            rdsTask->m_parameters.m_ForceMag = 1.0f;
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
            //    rdsTask->m_parameters.m_Stiffness = 5.f;
            //    rdsTask->m_parameters.m_ArmReachAmount = 1.0f;

            rdsTask->activate();
          }

        }

        bool der = false;
        if (m_headCollided)
        {
          //float rollAngVel = sideVector.Dot(angVel);
          ////if (m_rollAngVel > rollAngVel) 
          //if ((rollAngVel*rollAngVel < 0.25f) || !getSpine()->getSpine3Part()->collidedWithNotOwnCharacter())  
          //  der = true;

          //m_rollAngVel = rollAngVel;
        }

        if (der)//(m_drapeTimer > 1.00f)
        {
          m_balancerState = bal_DrapeForward;// bal_Draped;
          rdsTask->deactivate();
          bodyFoetalTask->deactivate();
        }
        //if (rdsTask->isActive())
        //  rdsTask->deactivate();
        //if (catchFallTask->isActive())
        //  catchFallTask->deactivate();

      }


    }


    if ((m_balancerState == bal_DrapeForward) || fowTask->isActive())
    {
      dynamicBalancerTask->setPlantLeg(false);
      //if ( (m_drapeTimer > 3.0f))//MMMMTodo Make this larger for forwards into block?
      //{
      //  if (dynamicBalancerTask->isActive())
      //    dynamicBalancerTask->forceFail();
      //}

      //forwards drape
      if (!catchFallTask->isActive())
      {
        catchFallTask->updateBehaviourMessage(NULL); // sets values to defaults
        catchFallTask->m_parameters.m_legsStiffness = 6.f;
        catchFallTask->m_parameters.m_torsoStiffness = 9.f;
        catchFallTask->m_parameters.m_armsStiffness = 11.f;
        catchFallTask->m_parameters.m_effectorMask = bvmask_UpperBody;
        catchFallTask->activate();
      }
      if ((dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK) && (!fowTask->isActive()))
      {
        m_balancerState = bal_LeanAgainst;
        m_ignoreBlock = true;
      }
    }
  }

  void NmRsCBUBalancerCollisionsReaction::probeEnvironment(const rage::Vector3& contactPos, const rage::Vector3& contactNormal)
  {
    rage::Vector3 probeFrom = contactPos;

    // Place probe at height where spine2 would be on impact if the character was standing up straight at impact.
    // 1st pass lowest foot if in contact plus spine2 height of standing character
    // if feet not in contact then default to current spine2.

    // If feet not in contact then default to current spine2
    float height = m_character->vectorHeight(getSpine()->getSpine2Part()->getPosition());
    // Either foot in contact?
    const bool eitherFootInContact = (getLeftLeg()->getFoot()->collidedWithNotOwnCharacter() || getRightLeg()->getFoot()->collidedWithNotOwnCharacter());
    if (eitherFootInContact)
    {
      // Distance from the foot to spine2 for standing up character.
      const float feetCenter2Spine2Distance = 1.35f;//mmmmtodo make rig independent 
      //2. Probe height is this distance from the lowest foot on impact
      // Find which foot is lower.
      height = rage::Min(m_character->vectorHeight(getLeftLeg()->getFoot()->getPosition()), m_character->vectorHeight(getRightLeg()->getFoot()->getPosition()));
      height += feetCenter2Spine2Distance;
    }
    probeFrom += (height - m_character->vectorHeight(probeFrom))*m_character->m_gUp;

    // Move the start of the probe back from the wall a bit (otherwise probe start maybe inside the wall and give a false result).
    const float backFromObstacleOffset = 0.05f;
    probeFrom.AddScaled(contactNormal, backFromObstacleOffset);

    // Probe end position.
    const float depth = 0.30f; // 30 cm
    const float probeAngle = 30.0f*PI/180.0f;
    const float probeLength = 1.0f;
    rage::Vector3 probeTo = -contactNormal*depth; //BBDD: contactNormal in not always perpendicular to the wall - make sure it is leveled.
    probeTo -= m_character->m_gUp*depth/rage::Tanf(probeAngle); 
    probeTo.Normalize(); // Add safe.
    probeTo.Scale(probeLength);
    probeTo.Add(probeFrom);

    rage::phSegment probe;
    probe.Set(probeFrom, probeTo);
    m_character->SubmitAsynchProbe(probe.A, probe.B, NmRsCharacter::pi_LightWeightAsync, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);

    m_waitingForAsynchProbe = true;
  }
  
  NmRsCBUBalancerCollisionsReaction::ObstacleTypes NmRsCBUBalancerCollisionsReaction::getProbeResult(const rage::Vector3& contactPos, const rage::Vector3& contactNormal)
  {
    rage::Vector3 probeHitPos;
    rage::Vector3 probeHitNormal;
    NmRsCharacter::probeResult probeResult = m_character->GetAsynchProbeResult(NmRsCharacter::pi_LightWeightAsync, 
      &probeHitPos, &probeHitNormal, NULL, NULL, NULL, NULL);
    if (probeResult == NmRsCharacter::probe_Late)
      //Keep waiting for result?
      return BCR_Unknown;
    if (probeResult > NmRsCharacter::probe_Late)
    {
      //probe is bad - perhaps try another?
      m_waitingForAsynchProbe = false;
      return BCR_Unknown;
    }
    // Wall, table or railing?
    if (probeResult == NmRsCharacter::probe_Hit)
    {
      m_probeHitPos = probeHitPos;
      m_probeNormal = probeHitNormal;

      // Check whether the probe hits on the plane of the impact position and normal then it is a wall.
      rage::Vector3 contactPos2HitPos = m_probeHitPos - contactPos;
      const float impactPlaneThickness = 0.01f;
      const bool hitOnImpactPlane = (rage::Abs(contactPos2HitPos.Dot(contactNormal)) < impactPlaneThickness) ? true : false;
      if (hitOnImpactPlane)
      {
        return BCR_Wall;
      }
      else
      {
        return BCR_Table;
      }
    }
    else //If probe doesn't hit then it's a rail Obstacle depth is less then 30 cm.
    {
      return BCR_Railing;
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUBalancerCollisionsReaction::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    static const char* balancerStateStrings[] =
    {
#define BCR_NAME_ACTION(_name) #_name ,
      BCR_STATES(BCR_NAME_ACTION)
#undef BCR_NAME_ACTION
    };

    //Parameters
    bspyTaskVar(m_parameters.glanceSpinTime, true);
    bspyTaskVar(m_parameters.glanceSpinMag, true);
    bspyTaskVar(m_parameters.glanceSpinDecayMult, true);
    bspyTaskVar(m_parameters.footFrictionMultStart, true);
    bspyTaskVar(m_parameters.footFrictionMultRate, true);
    bspyTaskVar(m_parameters.backFrictionMultStart, true);
    bspyTaskVar(m_parameters.backFrictionMultRate, true);
    bspyTaskVar(m_parameters.impactLegStiffReduction, true);
    bspyTaskVar(m_parameters.slumpLegStiffReduction, true);
    bspyTaskVar(m_parameters.slumpLegStiffRate, true);
    bspyTaskVar(m_parameters.reactTime, true);
    bspyTaskVar(m_parameters.stable2SlumpTime, true);
    bspyTaskVar(m_parameters.exclusionZone, true);
    bspyTaskVar(m_parameters.numStepsTillSlump, true); 
    bspyTaskVar(m_parameters.ignoreColWithIndex, true); 
    bspyTaskVar(m_parameters.slumpMode, true);   
    bspyTaskVar(m_parameters.reboundMode, true);   
    bspyTaskVar(m_parameters.ignoreColMassBelow, true);
    bspyTaskVar(m_parameters.ignoreColVolumeBelow, true);
    bspyTaskVar(m_parameters.fallOverWallDrape, true);
    bspyTaskVar(m_parameters.fallOverHighWalls, true);

    //snap
    bspyTaskVar(m_parameters.snap, true);
    if(m_parameters.snap)
    {
      bspyTaskVar(m_parameters.snapMag, true);
      bspyTaskVar(m_parameters.snapDirectionRandomness, true);
      bspyTaskVar(m_parameters.snapLeftArm, true);
      bspyTaskVar(m_parameters.snapRightArm, true);       
      bspyTaskVar(m_parameters.snapLeftLeg, true);
      bspyTaskVar(m_parameters.snapRightLeg, true);       
      bspyTaskVar(m_parameters.snapSpine, true);       
      bspyTaskVar(m_parameters.snapNeck, true);       
      bspyTaskVar(m_parameters.snapPhasedLegs, true);       
      bspyTaskVar(m_parameters.snapHipType, true);  
      bspyTaskVar(m_parameters.unSnapInterval, true);
      bspyTaskVar(m_parameters.unSnapRatio, true);
      bspyTaskVar(m_parameters.snapUseTorques, true);       
    }
    bspyTaskVar(m_parameters.impactWeaknessZeroDuration, true);
    bspyTaskVar(m_parameters.impactWeaknessRampDuration, true);
    bspyTaskVar(m_parameters.impactLoosenessAmount, true);

    bspyTaskVar(m_parameters.objectBehindVictim, true);
    bspyTaskVar(m_parameters.objectBehindVictimPos, true);
    bspyTaskVar(m_parameters.objectBehindVictimNormal, true);



    bspyTaskVar(m_timeAfterImpact, false);
//     bspyTaskVar(m_impactOccurred, false); HDD: duplicate
    bspyTaskVar(m_pos1stContact, false);
    bspyTaskVar(m_normal1stContact, false);
    bspyTaskVar(m_pos1stContactForFOW, false);
    bspyTaskVar(m_collisionLevelIndex, false);
    bspyTaskVar_StringEnum(m_balancerState, balancerStateStrings, false);
    bspyTaskVar(m_numStepsAtImpact, false);

    bspyTaskVar(m_footFrictionMult, false);
    bspyTaskVar(m_backFrictionMult, false);
    bspyTaskVar(m_slumpStiffLLeg, false);
    bspyTaskVar(m_slumpStiffRLeg, false);
    bspyTaskVar(m_slumpStiffLKnee, false);
    bspyTaskVar(m_slumpStiffRKnee, false);

    bspyTaskVar(m_comVelPrevious, false);
    bspyTaskVar(m_comVel_PreImpact, false);

    bspyTaskVar(m_ubCollisionNormalTotal, false);
    bspyTaskVar(m_lbCollisionNormalTotal, false);
    bspyTaskVar(m_normal1stContactLocal, false);
    bspyTaskVar(m_pos1stContactLocal, false);
    bspyTaskVar(m_pos1stContactLocalForFOW, false);

    bspyTaskVar(m_sideOfPlane, false);
    bspyTaskVar(m_drapeTimer, false);
    bspyTaskVar(m_rollAngVel, false);
    bspyTaskVar(m_amountOfMovement, false);
    bspyTaskVar(m_spineToWall, false);
    bspyTaskVar(m_stableTimer, false);

    if( m_previousBalancerState>=0 )
      bspyTaskVar_StringEnum(m_previousBalancerState, balancerStateStrings, false);

    bspyTaskVar(m_impactOccurred, false);
    bspyTaskVar(m_lbImpactOccurred, false);
    bspyTaskVar(m_ubImpactOccurred, false);
    bspyTaskVar(m_block, false);
    bspyTaskVar(m_headCollided, false);
    bspyTaskVar(m_slump, false);
    bspyTaskVar(m_ignoreBlock, false);

    bspyTaskVar(m_waitingForAsynchProbe, false);
    bspyTaskVar(m_probeHitPos, false);
    bspyTaskVar(m_probeNormal, false);
    static const char* obstacleType_names[] =
    {
#define BCR_OBSTACLE_NAME(_name) #_name ,
      BCR_OBSTACLE_TYPE(BCR_OBSTACLE_NAME)
#undef BCR_OBSTACLE_NAME
    };
    bspyTaskVar_StringEnum(m_obstacleType, obstacleType_names, false);

#if NM_OBJECTS_IN_COLLISIONS
    bspyTaskVar(m_character->m_objectMass, false);
    bspyTaskVar(m_character->m_objectSize, false);
    bspyTaskVar(m_character->m_objectFixed, false);
#endif
  }
#endif // ART_ENABLE_BSPY
}

