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
#include "NmRsEngine.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_DynamicBalancer.h"

namespace ART
{
     //----------------SHOT FROM BEHIND------------------------------------------------
    void NmRsCBUShot::shotFromBehind_applyDefaultPose()
    {
      getSpineInputData()->getSpine0()->setDesiredLean1(0.05f);
      getSpineInputData()->getSpine1()->setDesiredLean1(0.05f);
      getSpineInputData()->getSpine2()->setDesiredLean1(0.05f);
      getSpineInputData()->getSpine3()->setDesiredLean1(-0.3f);
      getSpineInputData()->getUpperNeck()->setDesiredLean1(-0.3f);
      getSpineInputData()->getUpperNeck()->setDesiredLean2(-0.2f);
      getSpineInputData()->getLowerNeck()->setDesiredLean1(-0.3f);
    }
    bool NmRsCBUShot::shotFromBehind_entryCondition()
    {
      return m_parameters.shotFromBehind && m_hitFromBehind && m_newHit;
    }

    // entry
    // ---------------------------------------------------------------------------------------
    void NmRsCBUShot::shotFromBehind_entry()
    {
      m_shotFromBehind.timer = 0.f;

      m_spineStiffness = m_parameters.bodyStiffness * 14.f/m_defaultBodyStiffness;
      m_spineDamping = 1.f;
      m_armsStiffness = m_parameters.bodyStiffness * 14.f/m_defaultBodyStiffness;
      m_armsDamping = 1.f;
      m_wristStiffness = 17.f;//m_parameters.bodyStiffness * 20.f/m_defaultBodyStiffness;
    m_neckStiffness = m_parameters.neckStiffness * 20.f/m_defaultBodyStiffness;

      m_character->getRandom().Reset();
      m_shotFromBehind.sfbRandTimer = m_character->getRandom().GetRanged(0.0f, 5000.0f);
      m_shotFromBehind.sfbNoiseSeed = m_character->getRandom().GetRanged(0.0f, 5000.0f);
    }

    // tick
    // ---------------------------------------------------------------------------------------
    void NmRsCBUShot::shotFromBehind_tick(float timeStep)
    {
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);

      //done in shotTick now so that the backArchedBack and leanedTooFarBack balancer fails 
      // are inhibited when shotFromBehind is enabled and for sfbRecoveryPeriod after it has finished.
      //m_shotFromBehind.timer += timeStep;

      m_shotFromBehind.sfbRandTimer += timeStep;

      // do this immediately
      shotFromBehind_applyDefaultPose();

      getSpine()->setBodyStiffness(getSpineInput(), 13.f,.5f, bvmask_LowSpine);

      // apply spine banana: weight as offset returning to zero towards end
      // ---------------------------------------------------------------------------------------
      float wt = 0.0f;
      if (m_parameters.sfbPeriod > 0.00001f)//Protect against div by zero
        wt = 1.0f - m_shotFromBehind.timer/m_parameters.sfbPeriod;
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "shot.fromBehind", wt);
#endif

      getSpineInputData()->applySpineLean(-m_parameters.sfbSpineAmount * wt, 0.f);

      dynamicBalancerTask->setHipPitch(m_parameters.sfbHipAmount * wt); // find the current hip pitch or only do if standing fairly upright?
      if(!m_parameters.useHeadLook)
      {
        getSpine()->setBodyStiffness(getSpineInput(), 13.f,.5f, bvmask_CervicalSpine);
        getSpineInputData()->getLowerNeck()->setDesiredLean1(-m_parameters.sfbNeckAmount * wt);
        getSpineInputData()->getUpperNeck()->setDesiredLean1(-m_parameters.sfbNeckAmount * wt);
      }

      // apply arm pose, no returning to default. but other arm behaviour will be running as part of shot.
      // ---------------------------------------------------------------------------------------   
      if(m_shotFromBehind.timer >= m_parameters.sfbArmsOnset)
      {
        float leftRand = m_parameters.sfbNoiseGain * (m_character->getEngine()->perlin3(m_shotFromBehind.sfbRandTimer, (float)m_character->getID(), m_shotFromBehind.sfbNoiseSeed)-0.5f);
        float rightRand = m_parameters.sfbNoiseGain * (m_character->getEngine()->perlin3(m_shotFromBehind.sfbNoiseSeed, (float)m_character->getID(), m_shotFromBehind.sfbRandTimer)-0.5f);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "shot.fromBehind", leftRand);
      bspyScratchpad(m_character->getBSpyID(), "shot.fromBehind", rightRand);
#endif

        // left arm
        getLeftArmInputData()->getClavicle()->setDesiredLean1(-0.4f);
        getLeftArmInputData()->getClavicle()->setDesiredLean2(-0.15f);
        getLeftArmInputData()->getClavicle()->setDesiredTwist(0.f);

        getLeftArmInputData()->getShoulder()->setDesiredLean1(0.f);
        getLeftArmInputData()->getShoulder()->setDesiredLean2(0.f); //1.15f);
        getLeftArmInputData()->getShoulder()->setDesiredTwist(-0.35f);

        getLeftArmInputData()->getElbow()->setDesiredAngle(1.2f + leftRand); //1.f);

        getLeftArmInputData()->getWrist()->setDesiredLean1(-0.4f);
        getLeftArmInputData()->getWrist()->setDesiredLean2(-0.65f);
        getLeftArmInputData()->getWrist()->setDesiredTwist(0.4f);

        // right arm
        getRightArmInputData()->getClavicle()->setDesiredLean1(-0.5f);
        getRightArmInputData()->getClavicle()->setDesiredLean2(-0.15f);
        getRightArmInputData()->getClavicle()->setDesiredTwist(0.f);

        getRightArmInputData()->getShoulder()->setDesiredLean1(0.05f);
        getRightArmInputData()->getShoulder()->setDesiredLean2(0.f); //1.15f);
        getRightArmInputData()->getShoulder()->setDesiredTwist(-0.4f);

        getRightArmInputData()->getElbow()->setDesiredAngle(1.2f + rightRand); //1.5f);

        getRightArmInputData()->getWrist()->setDesiredLean1(-0.25f);
        getRightArmInputData()->getWrist()->setDesiredLean2(-0.75f);
        getRightArmInputData()->getWrist()->setDesiredTwist(0.4f);
      }

      if (!m_fTK.m_bendLegs)//fallToKnees has control of forceBalance and legStraightness
      {
        // apply knee bend, also returns to zero
        // ---------------------------------------------------------------------------------------
        if(m_shotFromBehind.timer >= m_parameters.sfbKneesOnset &&
          (m_parameters.sfbPeriod - m_parameters.sfbKneesOnset) > 0.00001f)//Protect against div by zero
        {
          float kneesTimer = m_shotFromBehind.timer - m_parameters.sfbKneesOnset;
          kneesTimer = rage::Clamp(kneesTimer, 0.0f, 1.0f);
          float wtKnees =  1.0f - ( kneesTimer / (m_parameters.sfbPeriod - m_parameters.sfbKneesOnset));
          dynamicBalancerTask->setLegStraightnessModifier(-m_parameters.sfbKneeAmount * wtKnees);
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "shot.fromBehind", kneesTimer);
          bspyScratchpad(m_character->getBSpyID(), "shot.fromBehind", wtKnees);
#endif
        }

        //Control forceBalance - stops the balancer stepping
        NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
        Assert(balColReactTask);
        if (balColReactTask->isActive())
        {
          if (!(balColReactTask->m_balancerState == bal_Trip || balColReactTask->m_balancerState == bal_Slump ))
            dynamicBalancerTask->setForceBalance(false);
        }
        else
        {
          dynamicBalancerTask->setForceBalance(false);
        }
        if (m_shotFromBehind.timer < m_parameters.sfbForceBalancePeriod)
          dynamicBalancerTask->setForceBalance(true);

      }//if (!m_fTK.m_bendLegs)//fallToKnees has control of forceBalance and legStraightness

    }

    bool NmRsCBUShot::shotFromBehind_exitCondition()
    {
      return (m_shotFromBehind.timer >= m_parameters.sfbPeriod) || m_newHit || m_falling;
    }

    void NmRsCBUShot::shotFromBehind_exit()
    {
      if (!m_fTK.m_bendLegs)//fallToKnees has control of forceBalance and legStraightness and hipPitch
      {
        NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
        Assert(dynamicBalancerTask);
        NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
        Assert(balColReactTask);
        if (balColReactTask->isActive())
        {
          if (!(balColReactTask->m_balancerState == bal_Trip || balColReactTask->m_balancerState == bal_Slump ))
            dynamicBalancerTask->setForceBalance(false);
        }
        else
          dynamicBalancerTask->setForceBalance(false);
        dynamicBalancerTask->setHipPitch(0.f);
        dynamicBalancerTask->setLegStraightnessModifier(0.f);
      }//(!m_fTK.m_bendLegs)//fallToKnees has control of forceBalance and legStraightness and hipPitch
      m_neckStiffness  = m_parameters.neckStiffness;
    }
}
