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
 * Curls into foetal position, at a speed defined by the strength and damping values;
 * This behaviour is full-body and resets the character when it starts.
 *
 */


#include "NmRsInclude.h"
#include "NmRsCBU_CrushReaction.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_BodyBalance.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_CatchFall.h"
#include "NmRsCBU_BraceForImpact.h"
#include "NmRsCBU_Flinch.h"
#include "NmRsCBU_BodyFoetal.h"

namespace ART
{
  NmRsCBUCrushReaction::NmRsCBUCrushReaction(ART::MemoryManager* services) :
CBUTaskBase(services, bvid_crushReaction)
{
  initialiseCustomVariables();
}

NmRsCBUCrushReaction::~NmRsCBUCrushReaction()
{
}

void NmRsCBUCrushReaction::initialiseCustomVariables()
{
  Reset();

  m_mask = bvmask_Full;

  // in case no ID is send
  m_parameters.m_obstacleID = -1;
}

void NmRsCBUCrushReaction::onActivate()
{
  Assert(m_character);

  // state machine init: will execute first real state's enter function
  Initialize();

  m_hasCrushed = false;
  m_isCrushing = false;
  m_crushOver = false;
  m_collisionCounter = 0;
  m_time = 0.0f;
  m_headObstacleDir.Zero();
  m_headObstacleDist = 0.0f;
  m_headObstacleSpeed2 = 0.0f;
  m_energy = 0.0f;

  m_obsVel = rage::Vector3(0,0,0);
  m_obsMass = 0.0f;
  m_obsPos = rage::Vector3(0,0,0);

  //m_character->pushMaskCode(m_parameters.effectorMask);
  //m_character->processEffectorMaskCode(m_parameters.effectorMask);

  //effector->setMuscleStrength(randStiff*randStiff);
  //effector->setMuscleDamping(2*randStiff*randDamp);

  //m_character->popEffectorMask();
}

void NmRsCBUCrushReaction::onDeactivate()
{
  Assert(m_character);

  initialiseCustomVariables();
}

CBUTaskReturn NmRsCBUCrushReaction::onTick(float timeStep)
{        
  // global stuff here 
  updateGlobalState();
  handleCollisions();

  // state machine update
  Update();   

  m_time += timeStep;

  return eCBUTaskComplete;
}

// useful physics data
void NmRsCBUCrushReaction::updateGlobalState()
{
  rage::phCollider *obsInst = NULL;
  bool bLegitLevelIndex =   m_character->getLevel()->LegitLevelIndex(m_parameters.m_obstacleID);
  // TG - 20/07/10 : make sure we have a legit level index before trying to get a collider from it
  // Do not revert on integration. Need to bring this into NM branch.
  if(bLegitLevelIndex)
    obsInst = m_character->getSimulator()->GetCollider(m_parameters.m_obstacleID);

  if(obsInst)
  {
    m_obsVel = obsInst->GetVelocity();            
    m_obsMass = obsInst->GetMass();
    m_obsPos = obsInst->GetPosition();
  }

  // obstacle-head relative direction and velocity
  rage::Vector3 headPos = m_spine->getHeadPart()->getPosition();
  rage::Vector3 headVel = m_spine->getHeadPart()->getLinearVelocity();
  m_headObstacleDir = m_obsPos - headPos;
  m_headObstacleDist = m_headObstacleDir.Mag();
  rage::Vector3 headObstacleVel = m_obsVel - headVel;
  m_headObstacleSpeed2 = headObstacleVel.Mag2();

  // time to closest point of approach
  m_timeToCPA = -m_headObstacleDir.Dot(headObstacleVel);
  if (m_headObstacleSpeed2 > 1E-7)
    m_timeToCPA /= m_headObstacleSpeed2;
  else
    m_timeToCPA = 0.0f;

  rage::Vector3 cpaHeadPos = headPos + m_timeToCPA*headVel;
  rage::Vector3 cpaObsPos = m_obsPos + m_timeToCPA*m_obsVel;
  m_distAtCPA = (cpaHeadPos-cpaObsPos).Mag();
#if ART_ENABLE_BSPY
  rage::Vector3 col;
  if(m_timeToCPA > 0 && m_distAtCPA < 3.5f)
    col.Set(1,0,1);
  else
    col.Set(1,1,0);

  m_character->bspyDrawPoint(cpaHeadPos, 0.2f, col);
  m_character->bspyDrawPoint(cpaObsPos, 0.2f, col);
  m_character->bspyDrawLine(cpaHeadPos, cpaObsPos, col);
#endif

  // character's kinetic energy (for monitoring impact)
  m_energy = m_character->getKineticEnergyPerKilo_RelativeVelocity();
}

// checks for collisions and applies injuries to parts hit
void NmRsCBUCrushReaction::handleCollisions()
{
  rage::Vector3 zmpPos, zmpNormal;
  float zmpDepth = 0;
  rage::phInst* zmpInst = NULL;

  // feet contact
  m_leftLeg->getFoot()->getCollisionZMPWithEnvironment(zmpPos, zmpNormal, &zmpDepth, &zmpInst);
  m_feetGroundContact = (zmpInst && (zmpInst->GetLevelIndex() != m_parameters.m_obstacleID));
  if(!m_feetGroundContact)
  {
    m_rightLeg->getFoot()->getCollisionZMPWithEnvironment(zmpPos, zmpNormal, &zmpDepth, &zmpInst);
    m_feetGroundContact = (zmpInst && (zmpInst->GetLevelIndex() != m_parameters.m_obstacleID));
  }

  checkForInjury();                     
}

// iterates over ALL character joints and checks for collisions between the joints child 
// and instance obstID. when collision are found, it injures the corresponding part
void NmRsCBUCrushReaction::checkForInjury()
{
  //rage::phCollider *obsInst = m_character->getSimulator()->GetCollider(m_parameters.m_obstacleID);
  //if (obsInst)
  //{
  //  float obsMom = obsInst->GetMomentum().Mag();
  //  NM_RS_DBG_LOGF_SHORT(L"Crush: Obstacle Mom1: %f Mom: %f", obsMom, obsMass*obsSpeed);
  //}

  // need this until API exists for getting just zmpInst
  rage::Vector3 zmpPos, zmpNormal;
  float zmpDepth = 0;
  rage::phInst* zmpInst1 = NULL;

  int partN = m_character->getNumberOfParts();
  int effN = m_character->getNumberOfEffectors();

  // iterate over joints
  m_isCrushing = false;
  float totalImpactForce = 0.0;
  for (int i = 0; i < effN ; i++)
  {
    // get joint's child part
    int childID = m_character->getEffector(i)->getChildIndex();
    if( (childID >= 0) && (childID < partN))
    {
      // convert child to GenericPart
      NmRsGenericPart* part = m_character->lookupPartForArticulatedBodyPart(m_character->getEffector(i)->getChildPart());
      Assert(part);

      // get collision data
      part->getCollisionZMPWithEnvironment(zmpPos, zmpNormal, &zmpDepth, &zmpInst1);
      bool obsCol = (zmpInst1 && (zmpInst1->GetLevelIndex() == m_parameters.m_obstacleID));

      // if "real" collision
      if(obsCol && zmpDepth < 0)
      {
        if(!m_isCrushing)
          m_isCrushing = true;

        float impact = 0.0f;
        // calculate individual impact strengths properly
        rage::phCollider *obsInst = m_character->getSimulator()->GetCollider(m_parameters.m_obstacleID);
        if (obsInst)
        {
          float partMass = m_character->getEffector(i)->getChildPart()->GetMass().Getf();
          rage::Vector3 partPos = part->getPosition();        
          rage::Vector3 partVel = part->getLinearVelocity();        
          // get velocity of approach
          rage::Vector3 partObsDir = m_obsPos - partPos;
          rage::Vector3 partObsRelVel = m_obsVel - partVel;
          float partObsRelSpeed = partObsRelVel.Dot(partObsDir);
          impact = partObsRelSpeed*(partMass + m_obsMass);
          totalImpactForce += impact;
          NM_RS_DBG_LOGF(L"Crush: IMPACT %i | pM %f oM: %f speed: %f imp: %f", childID, partMass, m_obsMass, partObsRelSpeed, impact);
        }

        // apply injury proportional to impact force. "safely" clamps to range [0,1] and 
        // doesn't "heal" previous injuries accidentally

        if(m_parameters.m_useInjuries)
        {
          float damage = impact*0.1f;
          applyInjurySafely(m_character->getEffector(i), damage);
          //NM_RS_DBG_LOGF(L"Crush: IMPACT %i | injTried: %f | injGot: %f", childID, damage, inj);
        } // if useInjuries

      } // if real collision
    } // if child exist
  } // for all children

  // flag that we collided
  if(!m_hasCrushed && m_isCrushing)
    m_hasCrushed = true;

  // count for how long we haven't had contact after first collision
  // after certain threshold the crush phase is considered over
  if(m_hasCrushed && !m_isCrushing)
  {
    m_collisionCounter++;
    if(m_collisionCounter > 10)
      m_crushOver = true;
  }
  // if we had previous collision and are colliding again, reset counter
  // unless we already decided that the crush phase is over
  else if(m_hasCrushed && !m_crushOver)
  {
    m_collisionCounter = 0;
  }

  // debug
#if ART_ENABLE_BSPY
  if(m_isCrushing)
    bspyScratchpad(m_character->getBSpyID(), "Crush: impact total: %f", totalImpactForce);
#endif
}


// state machine logic
bool NmRsCBUCrushReaction::States( StateMachineEvent event, int state )
{
  NmRsCBUDynamicBalancer* balancer = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
  Assert(balancer);
  NmRsCBUBodyBalance* bodyBalance = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
  Assert(bodyBalance);
  NmRsCBUHeadLook* headLook = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
  Assert(headLook);
  NmRsCBUBraceForImpact* brace = (NmRsCBUBraceForImpact*)m_cbuParent->m_tasks[bvid_braceForImpact];
  Assert(brace);
  NmRsCBUFlinch* flinch = (NmRsCBUFlinch*)m_cbuParent->m_tasks[bvid_upperBodyFlinch];
  Assert(flinch);
  NmRsCBUCatchFall* catchFall = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
  Assert(catchFall);
  NmRsCBUBodyFoetal* foetal = (NmRsCBUBodyFoetal*)m_cbuParent->m_tasks[bvid_bodyFoetal];
  Assert(foetal);

  BeginStateMachine

    // Initial State ////////////////////////////////////////////////////
    State( SM_INITIAL )
    OnEnter          
    // start a balancer
    bodyBalance->updateBehaviourMessage(NULL);
  bodyBalance->activate();

  OnUpdate
    // give some initial time for impact prediction to wind up
    if(m_time > 0.03f)
    {
      // only react when some impact is imminent
      if(m_timeToCPA < 1.5f && m_timeToCPA > 0.0f && m_distAtCPA < 3.5f)
      {
        // when doing a "brace"
        if(m_parameters.m_flinchMode == FM_BRACE)
        {
          SetState( SM_BI_FLINCH );
        }
        // when doing a protective flinch: quick glance, followed by flinch
        else
        {
          headLook->updateBehaviourMessage(NULL);
          headLook->m_parameters.m_instanceIndex = m_parameters.m_obstacleID;
          headLook->activate();
          if(m_timeToCPA < 1.0f)
          {
            SetState( SM_BI_FLINCH );
          }
        }

      } // cpa < x
    } // time > x

    OnExit


      // Before Impact ////////////////////////////////////////////////////
      State( SM_BI_FLINCH )      
      OnEnter
      // activate flinch
      flinch->updateBehaviourMessage(NULL);
    flinch->m_parameters.m_pos = m_obsPos;
    flinch->m_parameters.m_bodyStiffness = 15.0f; 

    // choose whether to use flinch or "brace" mode
    //if(m_timeToCPA < 0.7f)
    if(m_parameters.m_flinchMode == FM_FLINCH)
    {
      flinch->m_parameters.m_protectHeadToggle = true; 
      flinch->m_parameters.m_headLookAwayFromTarget = true; 
      flinch->m_parameters.m_turnTowards = 0; 
    }
    // bracing: don't turn away
    else
    {
      flinch->m_parameters.m_protectHeadToggle = false; 
      flinch->m_parameters.m_headLookAwayFromTarget = false; 
      flinch->m_parameters.m_turnTowards = 1; 
    }

    flinch->activate();

    OnUpdate
      flinch->m_parameters.m_pos = m_obsPos;

    // when impact imminent prepare for it:
    if(m_timeToCPA < 0.33f)
      SetState( SM_DURING_IMPACT);

    OnExit



      // DURING IMPACT  ////////////////////////////////////////////////////
      State( SM_DURING_IMPACT )

      OnEnter
      // TODO: make character loser ?
      //float stiffness = 0.5f;
      //m_character->setBodyStiffness(3.0f,1.0f,"ua", &stiffness);

      OnUpdate
      // TODO: CHECK WHETHER IMPACT HAS OCCURED OR NOT !!!!
      // do a fall or foetal
      if(m_energy > 5.0f)
      {
        SetState(SM_DURING_FALL);
      }    
      else if(m_energy > 1.0f)
      {
        SetState(SM_TRY_BALANCE);
      }

      OnExit
        brace->deactivate();
      flinch->deactivate();

      // DURING FALL ////////////////////////////////////////////////////
      State( SM_TRY_BALANCE )

        OnEnter
        //bodyBalance->updateBehaviourMessage(NULL);
        bodyBalance->m_parameters.m_headLookInstanceIndex = m_parameters.m_obstacleID;    
      //bodyBalance->activate();

      OnUpdate
        int footState = balancer->footState();
      NM_RS_DBG_LOGF(L"Crush: TRYING BALANCE | footState: %i", footState);

      if(balancer->hasFailed())
      {
        SetState(SM_AFTER_FALL);
      }
      else if(!balancer->hasFailed() && footState == 0 && m_timeToCPA < 0)
      {
        SetState(SM_INITIAL);
        headLook->updateBehaviourMessage(NULL);
        headLook->m_parameters.m_instanceIndex = m_parameters.m_obstacleID;
        headLook->activate();
      }              
      // reset logic path
      m_hasCrushed = false;
      m_crushOver = false;


      // DURING FALL ////////////////////////////////////////////////////
      State( SM_DURING_FALL )

        OnEnter
        balancer->deactivate();
      bodyBalance->deactivate();
      // "low" energy impact
      if(m_parameters.m_flinchMode == FM_BRACE || m_energy < 15.0f)
      {
        catchFall->updateBehaviourMessage(NULL);
        catchFall->m_parameters.m_useHeadLook = true;
        catchFall->activate();          
        //m_character->applyInjuryMask()
      }
      // high energy impact
      else
      {
        foetal->updateBehaviourMessage(NULL);
        foetal->m_parameters.m_stiffness = 5.0f;
        foetal->m_parameters.m_damping = 1.0f;
        foetal->activate();
      }

      OnUpdate
        if ( (m_leftLeg->getShin()->collidedWithEnvironment()
          //||m_leftLeg->getThigh()->collidedWithEnvironment()
          ||m_rightLeg->getShin()->collidedWithEnvironment()
          //||m_rightLeg->getThigh()->collidedWithEnvironment()
          ) 
          && m_character->hasCollidedWithWorld("ub") 
          )
        {
          SetState(SM_AFTER_FALL);
        }

        OnExit
          if(catchFall->isActive())
          {
            catchFall->deactivate();
          }

          // DURING FALL ////////////////////////////////////////////////////
          State( SM_AFTER_FALL )

            OnEnter
            if(!foetal->isActive())
            {
              foetal->updateBehaviourMessage(NULL);
              foetal->m_parameters.m_stiffness = 8.0f;
              foetal->m_parameters.m_damping = 1.0f;
              foetal->activate();
            }

            EndStateMachine
}

#if ART_ENABLE_BSPY
void NmRsCBUCrushReaction::sendParameters(NmRsSpy& spy)
{
  CBUTaskBase::sendParameters(spy);

  bspyTaskVar(m_parameters.m_obstacleID, true);
  bspyTaskVar(m_parameters.m_stiffness, true);
  bspyTaskVar(m_parameters.m_damping, true);
  bspyTaskVar(m_parameters.m_effectorMask, true);
  bspyTaskVar(m_parameters.m_flinchMode, true);
  bspyTaskVar(m_parameters.m_useInjuries, true);

  // debug output
  bspyTaskVar(m_time, false);
  bspyTaskVar(m_headObstacleDist, false);
  bspyTaskVar(m_headObstacleSpeed2, false);
  bspyTaskVar(m_obsMass, false);
  bspyTaskVar(m_timeToCPA, false);
  bspyTaskVar(m_distAtCPA, false);
  bspyTaskVar(m_energy, false);

  bspyTaskVar(m_headObstacleDir, false);
  bspyTaskVar(m_obsPos, false);
  bspyTaskVar(m_obsVel, false);

  bspyTaskVar(m_collisionCounter, false);

  bspyTaskVar(m_doBrace, false);
  bspyTaskVar(m_hasCrushed, false);
  bspyTaskVar(m_isCrushing, false);
  bspyTaskVar(m_crushOver, false);
  bspyTaskVar(m_feetGroundContact, false);

  static const char* state_names[] = 
  {
    "Initial",
    "Flinch",
    "DuringImpact",
    "TryBalance",
    "DuringFall",
    "AfterFall"
  };
  bspyTaskVar_StringEnum(GetState(),state_names, false);
}
#endif // ART_ENABLE_BSPY
}

