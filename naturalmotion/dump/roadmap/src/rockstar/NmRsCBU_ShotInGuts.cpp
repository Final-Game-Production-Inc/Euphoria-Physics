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
     //----------------SHOT IN GUTS------------------------------------------------
    bool NmRsCBUShot::shotInGuts_entryCondition()
    {
      return m_parameters.shotInGuts && m_newHit;
    }

    // entry
    // ---------------------------------------------------------------------------------------
    void NmRsCBUShot::shotInGuts_entry()
    {
      m_shotInGuts.timer = 0.f;
      m_spineStiffness = m_parameters.bodyStiffness * 14.f/m_defaultBodyStiffness;
      m_spineDamping = 1.f;
    m_neckStiffness = m_parameters.neckStiffness * 20.f/m_defaultBodyStiffness;
    }

    // tick
    // ---------------------------------------------------------------------------------------
    void NmRsCBUShot::shotInGuts_tick(float timeStep)
    {
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);

      m_shotInGuts.timer += timeStep;

      getSpine()->setBodyStiffness(getSpineInput(), m_spineStiffness, 1.f, bvmask_LowSpine);

      // apply spine bend: weight as offset returning to zero towards end
      // ---------------------------------------------------------------------------------------
      float wt = 1.0f - m_shotInGuts.timer/m_parameters.sigPeriod;
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "shot.inGuts", m_shotInGuts.timer);
    bspyScratchpad(m_character->getBSpyID(), "shot.inGuts", wt);
#endif

    getSpineInputData()->applySpineLean(-m_parameters.sigSpineAmount * wt, 0.f);

      dynamicBalancerTask->setHipPitch(-m_parameters.sigHipAmount * wt); // find the current hip pitch or only do if standing fairly upright?
      if(!m_parameters.useHeadLook)
      {
        getSpine()->setBodyStiffness(getSpineInput(), 13.f,.5f, bvmask_HighSpine);
        getSpineInputData()->getLowerNeck()->setDesiredLean1(m_parameters.sigNeckAmount * wt);
        getSpineInputData()->getUpperNeck()->setDesiredLean1(m_parameters.sigNeckAmount * wt);
      }

      // apply knee bend, also returns to zero
      // ---------------------------------------------------------------------------------------
      if(m_shotInGuts.timer >= m_parameters.sigKneesOnset)
      {
        float kneesTimer = m_shotInGuts.timer - m_parameters.sigKneesOnset;
        kneesTimer = rage::Clamp(kneesTimer, 0.0f, 1.0f);
        float wtKnees =  1.0f - ( kneesTimer / (m_parameters.sigPeriod - m_parameters.sigKneesOnset));
		if (!m_fTK.m_bendLegs)
        dynamicBalancerTask->setLegStraightnessModifier(-m_parameters.sigKneeAmount * wtKnees);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "shot.inGuts", kneesTimer);
      bspyScratchpad(m_character->getBSpyID(), "shot.inGuts", wtKnees);
#endif
      }

      //MMMM decideBalancerState
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
      if (m_shotInGuts.timer < m_parameters.sigForceBalancePeriod)
        dynamicBalancerTask->setForceBalance(true);
    }

    bool NmRsCBUShot::shotInGuts_exitCondition()
    {
      return (m_shotInGuts.timer >= m_parameters.sigPeriod) || m_newHit || m_falling;
    }
    
    void NmRsCBUShot::shotInGuts_exit()
    {
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
      Assert(balColReactTask);
      if (balColReactTask->isActive())
      {
        if (!(balColReactTask->m_balancerState == bal_Trip  || balColReactTask->m_balancerState == bal_Slump ))
          dynamicBalancerTask->setForceBalance(false);
      }
      else
        dynamicBalancerTask->setForceBalance(false);
      
      dynamicBalancerTask->setHipPitch(0.f);
	  if (!m_fTK.m_bendLegs)
		dynamicBalancerTask->setLegStraightnessModifier(0.f);

      m_neckStiffness  = m_parameters.neckStiffness;

    }

}
