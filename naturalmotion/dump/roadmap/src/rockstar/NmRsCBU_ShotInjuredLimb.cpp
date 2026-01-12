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

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_DynamicBalancer.h"

namespace ART
{
     //----------------INJURED LEFT ARM ------------------------------------------------
    bool NmRsCBUShot::injuredLeftArm_entryCondition()
    {
      return m_injuredLArm && !m_parameters.melee;
      //return (m_hitPointRight < 0.f);//Use this to force the arm reaction w/o having to hit the arm
    }
    void NmRsCBUShot::injuredLeftArm_entry()
    {
      m_injuredArmElbowBend = 2.9f;

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "shot.injuredLimb.left", m_injuredArmElbowBend);
#endif
      
      //do velocity scaling of parameters
      float velMag = m_character->m_COMvelRelativeMag;//mmmmtodo? add in angvel component?#
      float velScale = 1.f;
      if (m_injuredArm.velScales)
      {
        if (m_injuredArm.velMultiplierEnd > m_injuredArm.velMultiplierStart)
          velScale = 1.f - rage::Clamp((velMag-m_injuredArm.velMultiplierStart)/(m_injuredArm.velMultiplierEnd-m_injuredArm.velMultiplierStart),0.f,1.f);
      }

      m_turnTo = m_parameters.bulletVel;
      m_character->levelVector(m_turnTo, 0.f);
      m_turnTo.Normalize();
      rage::Vector3 m_bodyBack  = m_character->m_COMTM.c;
      m_character->levelVector(m_bodyBack, 0.f);
      m_bodyBack.Normalize();

      m_shotFromTheFront = (m_bodyBack.Dot(m_turnTo) < 0.f ? false : true);
      m_turnTo.Cross(m_character->m_gUp);

      if (!m_shotFromTheFront)
      {
        m_hipYaw = -velScale*m_injuredArm.hipYaw;
        m_hipRoll = velScale*m_injuredArm.hipRoll;//-0.5f;
        m_twistMultiplier = -1.f;
      }
      else
      {
        m_hipYaw = velScale*m_injuredArm.hipYaw;
        m_hipRoll = velScale*m_injuredArm.hipRoll;//-0.5f;
        m_twistMultiplier = 1.f;
      }
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      if (m_injuredArm.forceStep && (velMag < m_injuredArm.velForceStep))
        dynamicBalancerTask->setForceStep(1, velScale*m_injuredArm.forceStepExtraHeight, true);
      m_shrug = true;//shrug the clavicles to emphasize arm shot
    }
    //----------------INJURED RIGHT ARM ------------------------------------------------
    bool NmRsCBUShot::injuredRightArm_entryCondition()
    {
      return m_injuredRArm && !m_parameters.melee;
      //return (m_hitPointRight > 0.f);//Use this to force the arm reaction w/o having to hit the arm
    }
    void NmRsCBUShot::injuredRightArm_entry()
    {
      m_injuredArmElbowBend = 2.9f;

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "shot.injuredLimb.right", m_injuredArmElbowBend);
#endif      
      
      //do velocity scaling of parameters
      float velMag = m_character->m_COMvelRelativeMag;//mmmmtodo? add in angvel component?#
      float velScale = 1.f;
      if (m_injuredArm.velScales)
      {
        if (m_injuredArm.velMultiplierEnd > m_injuredArm.velMultiplierStart)
          velScale = 1.f - rage::Clamp((velMag-m_injuredArm.velMultiplierStart)/(m_injuredArm.velMultiplierEnd-m_injuredArm.velMultiplierStart),0.f,1.f);
      }

      m_turnTo = -m_parameters.bulletVel;
      m_character->levelVector(m_turnTo, 0.f);
      m_turnTo.Normalize();
      rage::Vector3 m_bodyBack  = m_character->m_COMTM.c;
      m_character->levelVector(m_bodyBack, 0.f);
      m_bodyBack.Normalize();
      m_shotFromTheFront = (m_bodyBack.Dot(m_turnTo) > 0.f ? false : true);
      m_turnTo.Cross(m_character->m_gUp);


      if (!m_shotFromTheFront)
      {
        m_hipYaw = velScale*m_injuredArm.hipYaw;
        m_hipRoll = velScale*m_injuredArm.hipRoll;//-0.5f;
        m_twistMultiplier = -1.f;
      }
      else
      {
        m_hipYaw = -velScale*m_injuredArm.hipYaw;
        m_hipRoll = velScale*m_injuredArm.hipRoll;//-0.5f;
        m_twistMultiplier = 1.f;
      }
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      if (m_injuredArm.forceStep)
        dynamicBalancerTask->setForceStep(2, m_injuredArm.forceStepExtraHeight, true);
      m_shrug = true;//shrug the clavicles to emphasize arm shot
    }

    //----------------INJURED EITHER ARM ------------------------------------------------
    void NmRsCBUShot::injuredArm_tick(float /*timeStep*/)
    {
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);

      //do velocity scaling of parameters
      float velMag = m_character->m_COMvelRelativeMag;//mmmmtodo? add in angvel component?#
      float velScale = 1.f;
      if (m_injuredArm.velScales)
      {
        if (m_injuredArm.velMultiplierEnd > m_injuredArm.velMultiplierStart)
          velScale = 1.f - rage::Clamp((velMag-m_injuredArm.velMultiplierStart)/(m_injuredArm.velMultiplierEnd-m_injuredArm.velMultiplierStart),0.f,1.f);
      }

      int forceStepLeg = m_armProperty.arm == getLeftArm() ? 1:2;
      if (velMag > m_injuredArm.velForceStep)
        dynamicBalancerTask->setForceStep(0, 0.f, false);
      else if (m_injuredArm.forceStep)
        dynamicBalancerTask->setForceStep(forceStepLeg, velScale*m_injuredArm.forceStepExtraHeight, false);

      float mult = m_armProperty.arm == getLeftArm() ? 1.0f:-1.0f;
      m_hipRoll = velScale*m_injuredArm.hipRoll;//-0.5f;
      if (!m_shotFromTheFront)
        m_hipYaw = -velScale*m_injuredArm.hipYaw*mult;
      else
        m_hipYaw = velScale*m_injuredArm.hipYaw*mult;

      //emphasize hit/pain/reflex with clavicles - overidden by match clavicles in reacforwound IK if hitTime>m_shrugTime

      // todo this is clunky. consider adding non-injured arm and input to the armproperty structure.
      NmRsHumanArm* nonInjuredArm;
      NmRsArmInputWrapper* nonInjuredInput;
      NmRsArmInputWrapper* injuredInput;
      if(m_armProperty.arm->getType() == kLeftArm)
      {
        injuredInput = getLeftArmInputData();
        nonInjuredArm = getRightArm();
        nonInjuredInput = getRightArmInputData();
      }
      else
      {
        injuredInput = getRightArmInputData();
        nonInjuredArm = getLeftArm();
        nonInjuredInput = getLeftArmInputData();
      }

      if (m_shotFromTheFront)//shot from the front
      {
        //lift up injured clavicle and send backwards to emphasize hit then mmmmtodo send forward to emphasize protection
        injuredInput->getClavicle()->setDesiredLean1(m_armProperty.arm->getClavicle()->getMinLean1());//back
        injuredInput->getClavicle()->setDesiredLean2(m_armProperty.arm->getClavicle()->getMinLean2());//up
        //non-injured clavicle down and forwards
        nonInjuredInput->getClavicle()->setDesiredLean1(nonInjuredArm->getClavicle()->getMaxLean1());//forward
        nonInjuredInput->getClavicle()->setDesiredLean2(nonInjuredArm->getClavicle()->getMaxLean2());//down
      }
      else//shot from the behind
      {
        //lift up injured clavicle and send forwards to emphasize hit and protection 
        injuredInput->getClavicle()->setDesiredLean1(m_armProperty.arm->getClavicle()->getMaxLean1());//forward
        injuredInput->getClavicle()->setDesiredLean2(m_armProperty.arm->getClavicle()->getMinLean2());//up
        //non-injured clavicle up and backwards
        nonInjuredInput->getClavicle()->setDesiredLean1(nonInjuredArm->getClavicle()->getMinLean1());//back
        nonInjuredInput->getClavicle()->setDesiredLean2(nonInjuredArm->getClavicle()->getMinLean2());//up
      }

      // Turn Overrides bd, stagger, brace, yanked  
      // Turn Overwritten by shot_Melee, flinch, grab
      if (m_injuredArm.stepTurn)
      {
        m_injuredArm.m_previousUseCustomTurnDir = dynamicBalancerTask->getUseCustomTurnDir();
        m_injuredArm.m_previousCustomTurnDir = dynamicBalancerTask->getCustomTurnDir();
        if (m_hitTime < m_injuredArm.injuredArmTime && !(m_injuredArm.velScales && (velMag > m_injuredArm.velForceStep)))//mmmmtodo make vel dependent
        {
          dynamicBalancerTask->useCustomTurnDir(true, m_turnTo);//is a global direction//mmmmtodo make vel dependent by taking weighted average of turn dir an front?
        }
        else
        {
          //turn off turn unless some other behaviour has set it
          if ((m_injuredArm.m_previousCustomTurnDir - m_turnTo).Mag() > 0.0001f)
            dynamicBalancerTask->useCustomTurnDir(m_injuredArm.m_previousUseCustomTurnDir, m_injuredArm.m_previousCustomTurnDir);
          else
            dynamicBalancerTask->useCustomTurnDir(false, m_injuredArm.m_previousCustomTurnDir);
        }
      }
      if (m_hitTime < m_injuredArm.injuredArmTime )//mmmmtodo make vel dependent
      {
        dynamicBalancerTask->setHipYaw(m_hipYaw);//mmmmtodo make vel dependent
        dynamicBalancerTask->setHipRoll(m_hipRoll);
      }
      else
      {
        //dynamicBalancerTask->useCustomTurnDir(false, m_turnTo);
        dynamicBalancerTask->setHipYaw(0.f);
        dynamicBalancerTask->setHipRoll(0.f);
        //dynamicBalancerTask->autoLeanCancel();
      }

    }
    bool NmRsCBUShot::injuredArm_exitCondition()
    {
      return m_falling || 
        m_hitTime > m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound ||//should m_hitTime be replace with m_hitTimeLeft/Right on this line?
        m_hitTime > m_injuredArm.injuredArmTime;
    }

    void NmRsCBUShot::injuredArm_exit()
    {
      m_injuredLArm = false;
      m_injuredRArm = false;
      
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      if (m_injuredArm.stepTurn)
      {
        //turn off turn unless some other behaviour has set it
        if ((m_injuredArm.m_previousCustomTurnDir - m_turnTo).Mag() > 0.0001f)
          dynamicBalancerTask->useCustomTurnDir(m_injuredArm.m_previousUseCustomTurnDir, m_injuredArm.m_previousCustomTurnDir);
        else
          dynamicBalancerTask->useCustomTurnDir(false, m_injuredArm.m_previousCustomTurnDir);
      }

      dynamicBalancerTask->setForceStep(0, 0.07f, false);

      dynamicBalancerTask->setHipYaw(0.f);
      dynamicBalancerTask->setHipRoll(0.f);
      m_twistMultiplier = 1.f;
      m_shrug = false;//shrug the clavicles to emphasize arm shot
    }

    //----------------INJURED LEFT LEG ------------------------------------------------
    bool NmRsCBUShot::injuredLeftLeg_entryCondition()
    {
      return m_parameters.allowInjuredLeg && m_injuredLLeg && !m_parameters.melee && m_hitTime > m_parameters.timeBeforeCollapseWoundLeg;
    }

    //----------------INJURED RIGHT LEG ------------------------------------------------
    bool NmRsCBUShot::injuredRightLeg_entryCondition()
    {
      return m_parameters.allowInjuredLeg && m_injuredRLeg && !m_parameters.melee && m_hitTime > m_parameters.timeBeforeCollapseWoundLeg;
  }
    
    void NmRsCBUShot::injuredLeftLeg_entry()
    {
	  m_injuredLeftLeg.legLiftTimer = m_parameters.legLiftTime;
	  m_injuredLeftLeg.legInjuryTimer = m_parameters.legInjuryTime;
	  m_injuredLeftLeg.forceStepTimer = 0.2f;//stops a slowly moving out of balance character taking the wrong step 

      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      static float IL_LiftL = 0.07f;
      if (m_parameters.legForceStep)
        dynamicBalancerTask->setForceStep(1, IL_LiftL, true);
    }

    //----------------INJURED RIGHT LEG ------------------------------------------------
    void NmRsCBUShot::injuredRightLeg_entry()
    {
	  m_injuredRightLeg.legLiftTimer = m_parameters.legLiftTime;
	  m_injuredRightLeg.legInjuryTimer = m_parameters.legInjuryTime;
	  m_injuredRightLeg.forceStepTimer = 0.2f;//stops a slowly moving out of balance character taking the wrong step 

      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      static float IL_LiftR = 0.07f;
	  if (m_parameters.legForceStep)
        dynamicBalancerTask->setForceStep(2, IL_LiftR, true);
    }

    void NmRsCBUShot::injuredLeg_tick(float timeStep)
    {
	  NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];
	  
	  if (m_parameters.legLimpBend > 0.f)
	  {
		float feetApart = m_character->horizDistance(getLeftLeg()->getFoot()->getPosition(),getRightLeg()->getFoot()->getPosition());
		static float minFD = 0.6f;
		static float maxFD = 1.f;
		feetApart = (rage::Clamp(feetApart,minFD, maxFD) - minFD)/(maxFD-minFD);

		float straightnessReduction = m_parameters.legLimpBend*(1.f - feetApart);
		if ((m_injuredLegMask & bvmask_LegRight) && dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep ||
		  (m_injuredLegMask & bvmask_LegLeft) && dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
		{
		  if (!m_fTK.m_bendLegs)
			dynamicBalancerTask->setLegStraightnessModifier(-straightnessReduction);
		}
		else
		{
		  if (!m_fTK.m_bendLegs)
			dynamicBalancerTask->setLegStraightnessModifier(0.f);
		}
	  }
	  else 
	  {
		//if m_parameters.legLimpBend changes to 0 on the fly then we should setLegStraightnessModifier(0.f)
		if (!m_fTK.m_bendLegs)
		  dynamicBalancerTask->setLegStraightnessModifier(0.f);
	  }

      // todo this whole system is pretty 'orrible. consider refactor.
      //NmRsHumanLeg * injuredLegSetup;
      InjuredLeg* injuredLeg = &m_injuredLeftLeg;
      NmRsLegInputWrapper* injuredLegInput;
      if ((m_injuredLegMask & bvmask_LegLeft) && (m_injuredLegMask & bvmask_LegRight))
      {
        //injuredLegSetup = getRightLeg();
        injuredLeg = &m_injuredRightLeg;
        injuredLegInput = getRightLegInputData();
      }
      else
      {
        //injuredLegSetup = getLeftLeg();
        injuredLeg = &m_injuredLeftLeg;
        injuredLegInput = getLeftLegInputData();
      }

      if (injuredLeg->legLiftTimer > 0.0f &&
        (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kNotStepping ||
         (m_injuredLegMask & bvmask_LegLeft) && dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep ||
         (m_injuredLegMask & bvmask_LegRight) && dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep ||
         m_character->m_underwater))
      {
        dynamicBalancerTask->setHipPitch(m_parameters.legInjuryLiftHipPitch);

        getSpineInputData()->getSpine0()->setDesiredLean1(m_parameters.legInjuryLiftSpineBend);
        getSpineInputData()->getSpine1()->setDesiredLean1(m_parameters.legInjuryLiftSpineBend);
        getSpineInputData()->getSpine2()->setDesiredLean1(m_parameters.legInjuryLiftSpineBend);
        getSpineInputData()->getSpine3()->setDesiredLean1(m_parameters.legInjuryLiftSpineBend);

        injuredLegInput->getHip()->setDesiredLean1(2.0f + rage::Clamp(2.f*injuredLeg->legLiftTimer, 0.f, 1.f));
        if (m_character->m_underwater)
          injuredLegInput->getKnee()->setDesiredAngle(-2.0f); // -2.0f);
        else
          injuredLegInput->getKnee()->setDesiredAngle(-1.5f); // -2.0f);

        injuredLeg->legLiftTimer -= timeStep;
        m_character->applyInjuryMask(m_injuredLegMask,0.f);
      }
      else if (injuredLeg->legInjuryTimer > 0.0f)
      {
        dynamicBalancerTask->setHipPitch(m_parameters.legInjuryHipPitch);
        m_character->applyInjuryMask(m_injuredLegMask,m_parameters.legInjury);
        injuredLeg->legInjuryTimer -= timeStep;

        getSpineInputData()->getSpine0()->setDesiredLean1(m_parameters.legInjurySpineBend);
        getSpineInputData()->getSpine1()->setDesiredLean1(m_parameters.legInjurySpineBend);
        getSpineInputData()->getSpine2()->setDesiredLean1(m_parameters.legInjurySpineBend);
        getSpineInputData()->getSpine3()->setDesiredLean1(m_parameters.legInjurySpineBend);
      }
      else
      {
        dynamicBalancerTask->setHipPitch(0.0f);
        m_character->applyInjuryMask(m_injuredLegMask,0.f);
      }

	  if (injuredLeg->forceStepTimer > 0.0f)
	  {
		injuredLeg->forceStepTimer -= timeStep;
		if (injuredLeg->forceStepTimer <= 0.0f)
		  dynamicBalancerTask->setForceStep(0, 0.07f, false);
	  }
    }

    //Allows leg to continue to be injured if another leg is injured or there is a newShot.
    bool NmRsCBUShot::injuredLeftLeg_exitCondition()//mmmmtodo split up for each leg
    {
      return ((m_injuredLeftLeg.legLiftTimer <= 0.0f && m_injuredLeftLeg.legInjuryTimer <= 0.0f) ||
        !m_parameters.allowInjuredLeg || m_parameters.melee );
    }
    //Allows leg to continue to be injured if another leg is injured or there is a newShot.
    bool NmRsCBUShot::injuredRightLeg_exitCondition()//mmmmtodo split up for each leg
    {
      return ((m_injuredRightLeg.legLiftTimer <= 0.0f && m_injuredRightLeg.legInjuryTimer <= 0.0f) ||
               !m_parameters.allowInjuredLeg || m_parameters.melee );
    }

	void NmRsCBUShot::injuredLeg_exit()
    {
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      dynamicBalancerTask->setHipPitch(0.0f);
      dynamicBalancerTask->setKneeStrength(1.f);
      if (!m_fTK.m_bendLegs)
        dynamicBalancerTask->setLegStraightnessModifier(0.f);

      getSpineInputData()->getSpine0()->setDesiredLean1(0.0f);
      getSpineInputData()->getSpine1()->setDesiredLean1(0.0f);
      getSpineInputData()->getSpine2()->setDesiredLean1(0.0f);
      getSpineInputData()->getSpine3()->setDesiredLean1(0.0f);
	}

	void NmRsCBUShot::injuredRightLeg_exit()
	{
      injuredLeg_exit();
      m_character->applyInjuryMask(bvmask_LegRight, 0.f);
				m_injuredRLeg = false;
    }    
    void NmRsCBUShot::injuredLeftLeg_exit()
    {
      injuredLeg_exit();
      m_character->applyInjuryMask(bvmask_LegLeft, 0.f);
				m_injuredLLeg = false;
    }
}

