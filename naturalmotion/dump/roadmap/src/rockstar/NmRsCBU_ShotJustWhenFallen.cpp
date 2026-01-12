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
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_RollUp.h"
#include "NmRsCBU_RollDownStairs.h"
#include "NmRsCBU_CatchFall.h"

namespace ART
{
     //----------------JUST WHEN FALLEN------------------------------------------------
    bool NmRsCBUShot::justWhenFallen_entryCondition()
    {
      NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
      Assert(balColReactTask);
    bool falling = m_falling;

#if NM_HANDSANDKNEES_FIX
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
	NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
	Assert(catchFallTask);
    falling = falling || (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK && catchFallTask->m_handsAndKnees); 
#endif    
      if (balColReactTask->isActive())
      return falling && (balColReactTask->m_balancerState != bal_Drape) && (balColReactTask->m_balancerState != bal_DrapeForward); //&& !(m_parameters.useExtendedCatchFall && m_onGroundEnabled);
      else
      return falling; //&&  !(m_parameters.useExtendedCatchFall && m_onGroundEnabled);
    }

    void NmRsCBUShot::justWhenFallen_entry()
    {
      m_body->resetEffectors(kResetCalibrations);
      m_body->setStiffness(5.0f, 0.75f, bvmask_LegLeft | bvmask_LegRight);
      m_body->setStiffness(9.0f, 0.75f, bvmask_ArmLeft | bvmask_ArmRight | bvmask_Spine);

      // limbs todo is this commented out of original code?
      //m_character->holdPoseAllEffectors();
    }
    void NmRsCBUShot::justWhenFallen_tick(float /*timeStep*/)
    {
      if (m_newHit)
      {
        m_body->resetEffectors(kResetCalibrations);
        m_body->setStiffness(m_parameters.bodyStiffness * 5.f/m_defaultBodyStiffness, 0.75f, bvmask_LegLeft | bvmask_LegRight);
        m_body->setStiffness(m_parameters.bodyStiffness * 9.f/m_defaultBodyStiffness, 0.75f, bvmask_ArmLeft | bvmask_ArmRight | bvmask_Spine);
      }
      getSpineInputData()->setOpposeGravity(2.0f);
      getLeftLegInputData()->getHip()->setOpposeGravity(2.f);
      getRightLegInputData()->getHip()->setOpposeGravity(2.f);

      //do rollup or catchfall
      //The catch fall is relaxed by the shotRelax ramps (except if in melee)
      //The roll up is not and the hands and knees catch fall are not relaxed by the shot relax ramps
      //MMMMtodo The falling reactions should be integrated with the current character stiffnesses.
      
	  NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
	  Assert(catchFallTask);
      // if hands and knees is set only do catchFall (rolldownstairs will be called automatically from catchFall if the character moves to fast after landing)
      if (!m_parameters.useCatchFallOnFall && !catchFallTask->m_handsAndKnees) // do roll up
      {
      //For Fall2Knees call RollDownStairs instead of rollup
      if (!m_fallToKneesEnabled)
      {
        NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
        Assert(rollUpTask);
        if (!rollUpTask->isActive())
        {
          rollUpTask->initialiseCustomVariables();
          rollUpTask->updateBehaviourMessage(NULL);
          rollUpTask->m_parameters.m_useArmToSlowDown=(2.f);
          rollUpTask->m_parameters.m_armReachAmount=(0.f);
          rollUpTask->m_parameters.m_stiffness=(m_parameters.bodyStiffness * 9.f/m_defaultBodyStiffness);
          rollUpTask->m_parameters.m_legPush=(0.f);
          rollUpTask->m_parameters.m_asymmetricalLegs=(0.75f);
          rollUpTask->m_fromShot=(true);
          rollUpTask->activate();
        }
      }
      else
      {

        NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
        Assert(rdsTask);
        if (!rdsTask->isActive())
        {
          rdsTask->updateBehaviourMessage(NULL); // sets values to defaults

          rdsTask->m_parameters.m_Stiffness = 9.f;
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
          rdsTask->m_parameters.m_StiffnessDecayTime = 3.f;
          rdsTask->m_parameters.m_StiffnessDecayTarget = 3.f;
          //if (balColReactTask->isActive())
          //{
          //  if (balColReactTask->m_balancerState == bal_Slump)
          //  {
          //    rdsTask->m_parameters.m_Stiffness = 5.f;
          //    rdsTask->m_parameters.m_ArmReachAmount = 1.0f;
          //  }
          //}
            rdsTask->activate();
        }
        }
        // nothing done in the tick here
      }
      else // do catchfall
      {
        NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
        Assert(catchFallTask);
        if (!catchFallTask->isActive())
        catchFallTask->updateBehaviourMessage(NULL); // set parameters to defaults
        float uStiffnessScale = 0.5f + 0.5f*m_upperBodyStiffness;
        float lStiffnessScale = 0.5f + 0.5f*m_lowerBodyStiffness;


        if (m_parameters.melee) // catch fall isn't relaxed by the shotRelax ramps for melee
        {
          catchFallTask->m_parameters.m_legsStiffness = m_parameters.bodyStiffness * 0.3f;//3.3
          catchFallTask->m_parameters.m_torsoStiffness = m_parameters.bodyStiffness * 0.7f;//7.7
          catchFallTask->m_parameters.m_armsStiffness = m_parameters.bodyStiffness * 0.9f;//9.9
        }
        else // The catch fall is relaxed by the shotRelax ramps
        {
          catchFallTask->m_parameters.m_legsStiffness = 6.0f*lStiffnessScale;
          catchFallTask->m_parameters.m_torsoStiffness = 9.0f*uStiffnessScale;
          catchFallTask->m_parameters.m_armsStiffness = 15.0f*uStiffnessScale;
        }

        if (catchFallTask->m_handsAndKnees)// catch fall isn't relaxed by the shotRelax ramps for handsAndKnees
        {
          //float defaultBodyStiffness = 11.f;  // for the bodyBalance
          catchFallTask->m_parameters.m_legsStiffness = 5.5f;//m_parameters.bodyStiffness * 5.5f/defaultBodyStiffness;
          catchFallTask->m_parameters.m_torsoStiffness = 10.f;//m_parameters.bodyStiffness * 10.f/defaultBodyStiffness;
          catchFallTask->m_parameters.m_armsStiffness = 15.f;//m_parameters.bodyStiffness * 15.f/defaultBodyStiffness;
        }

        if (!catchFallTask->isActive())
        {
          if (catchFallTask->m_handsAndKnees)
          {
            //We used to start the catchFall with an upperbody response only here - we wanted the dynBalancer to keep stepping for a while
            //as the character falls over.  CatchFall used to modify this parameter internally. 
            catchFallTask->m_parameters.m_effectorMask = bvmask_Full;

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
    }
    bool NmRsCBUShot::justWhenFallen_exitCondition()
    {
      //NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
      //Assert(catchFallTask);
      return false;//m_parameters.useExtendedCatchFall && catchFallTask->isOnGround();
    }
    void NmRsCBUShot::justWhenFallen_exit()
    {
      NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
      Assert(rollUpTask);
      if (rollUpTask->isActive())
        rollUpTask->deactivate();

      NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
      Assert(catchFallTask);
      if (catchFallTask->isActive())
        catchFallTask->deactivate();
    }
}

