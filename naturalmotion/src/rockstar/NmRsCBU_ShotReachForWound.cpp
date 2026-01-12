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
#include "NmRsCBU_Rollup.h"

namespace ART
{
     //------------ REACH LEFT---------------------------------------
    bool NmRsCBUShot::reachLeft_entryCondition()
    {
      return 
        (!m_bcrArms && m_parameters.reachForWound && m_hitTimeLeft > m_parameters.timeBeforeReachForWound && 
         m_newReachL && m_hitTimeLeft < m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound && 
         m_character->getCharacterConfiguration().m_leftHandState != CharacterConfiguration::eHS_Rifle &&
         (m_character->getCharacterConfiguration().m_leftHandState == CharacterConfiguration::eHS_Free || m_parameters.allowLeftPistolRFW) &&
         (!m_defaultArmMotion.releaseLeftWound || m_hitTimeLeft < m_parameters.timeBeforeReachForWound + m_parameters.alwaysReachTime) 
        ) 
        || (m_parameters.useExtendedCatchFall && m_onGroundEnabled);
    }
    void NmRsCBUShot::reachLeft_entry()
    {
#if ART_ENABLE_BSPY
	  bspyLogf(info, L"entered reach left: %.3f", m_woundLPart);
#endif
      m_reachLeft.arm = kLeftArm;
      m_reachLeft.direction = 1;
      m_reachLeft.normal = m_woundLNormal;
      m_reachLeft.offset = m_woundLOffset;
      //m_reachLeft.twist = 0.2f;
      m_reachLeft.woundPart = m_woundLPart;
      m_newReachL = false;
    }

    void NmRsCBUShot::reachArm_tick(float /*timeStep*/)
    {
      //Decide whether to reachForWound based on falling
      bool onGround = m_character->hasCollidedWithEnvironment(bvmask_UpperBody);
      bool reachOnGround = onGround && ((m_parameters.reachOnFloor == 2 && !m_reachedForFallen) || //Only reach while falling once
        m_parameters.reachOnFloor == 1);
      bool reachWhenFalling = !onGround && ((m_parameters.reachFalling == 2 && !m_reachedForFallen) || //Only reach while falling once
                                            m_parameters.reachFalling == 1);
      float strengthScale = 0.5f + 0.5f*m_upperBodyStiffness;
#if ART_ENABLE_BSPY
      bool leftArm = m_reachArm == &m_reachLeft;
      bspyScratchpad(m_character->getBSpyID(), "rfw", leftArm);
      bspyScratchpad(m_character->getBSpyID(), "rfw", onGround);
      bspyScratchpad(m_character->getBSpyID(), "rfw", reachWhenFalling);
      bspyScratchpad(m_character->getBSpyID(), "rfw", reachOnGround);
#endif

      if (m_falling)
      {
        //fallToKnees request to reachForWound is respected
        //i.e. reach with available arms whether falling or not.
        if (!m_fallToKneesEnabled || !m_parameters.ftkReachForWound)
        {
          if (reachWhenFalling || reachOnGround)
          {
            bool twoHandedReach = m_reachLeftEnabled && m_reachRightEnabled;
#if ART_ENABLE_BSPY
            bspyScratchpad(m_character->getBSpyID(), "rfw", twoHandedReach);
#endif
            if (twoHandedReach)//early out for left or right only reach if could reach with both arms
            {
              if (m_parameters.reachFallingWithOneHand == 1 && m_reachArm == &m_reachRight)
                return;
              if (m_parameters.reachFallingWithOneHand == 2 && m_reachArm == &m_reachLeft)
                return;
            }//if (twoHandedReach)
            
            rage::Matrix34 predCOMTM;
            static float predTime = 0.2f;
            m_character->getPredictedCOMOrientation(predTime, &predCOMTM);
            NmRsCharacter::OrientationStates orientationState = m_character->getFacingDirectionFromCOMOrientation(predCOMTM);
            //disable reach if in certain orientations on ground
            //  If falling but not on ground then use the below conditions to decide the best one handed reach if reaching with both possible and m_parameters.reachFallingWithOneHand == 3
            //  We could also disable reach for falling to ground for e.g landing on front which gives a good tradeoff between reachingForWound and bracing for impact
#if ART_ENABLE_BSPY
            static const char* orientationState_names[] =
            {
#define HF_ORIENTATION_STATE_NAME_ACTION(_name) #_name ,
              ORIENTATION_STATES(HF_ORIENTATION_STATE_NAME_ACTION)
#undef HF_ORIENTATION_STATE_NAME_ACTION
            };
            bspyScratchpad(m_character->getBSpyID(), "rfw", orientationState_names[orientationState]);
#endif
            bool disableLeft = false;
            bool disableRight = false;
            switch (orientationState)
            {
            case NmRsCharacter::OS_Front:
            case NmRsCharacter::OS_Down:
              {
                disableLeft = true;
                disableRight = true;
              }
              break;
            case NmRsCharacter::OS_Left:
              {
                //if (m_reachArm == &m_reachLeft)
                  disableLeft = true;
              }
              break;
            case NmRsCharacter::OS_Right:
              {
                //if (m_reachArm == &m_reachRight)
                  disableRight = true;
              }
              break;
            case NmRsCharacter::OS_Back:
            case NmRsCharacter::OS_Up:
            default:
              break;
            } // End of switch.
            //if the character is on ground then disable reach based on the switch above
#if ART_ENABLE_BSPY
            bspyScratchpad(m_character->getBSpyID(), "rfw", disableLeft);
            bspyScratchpad(m_character->getBSpyID(), "rfw", disableRight);
#endif
            if (onGround && ((m_reachArm == &m_reachLeft && disableLeft) || (m_reachArm == &m_reachRight && disableRight)))
              return;
            //if could reach with both arms disable one of them.  Best arm to disable is based on the switch above.
            //Left arm ticks before right arm
#if ART_ENABLE_BSPY
            bspyScratchpad(m_character->getBSpyID(), "rfw", true);
#endif
            if (twoHandedReach && m_reachArm == &m_reachRight && m_parameters.reachFallingWithOneHand == 3 && !disableRight && !disableLeft)
              return;//disable the left hand if both are good to reach
#if ART_ENABLE_BSPY
            bspyScratchpad(m_character->getBSpyID(), "rfw", true);
#endif
            //if the right arm isn't going to be disabled consider disabling the leftarm
            //if the left arm hasn't been disabled consider disabling the rightarm
            if (twoHandedReach && m_parameters.reachFallingWithOneHand == 3 && (disableRight || disableLeft) && 
                ((m_reachArm == &m_reachLeft && !disableRight) || (m_reachArm == &m_reachRight && !disableLeft)))
              return;
          }//if (reachWhenFalling || reachOnGround)
          else
            return;//falling/onGround reach not allowed by parameters
        }//if (!m_fallToKneesEnabled || !m_parameters.ftkReachForWound)

        //This arm should reach while falling/on ground.  Exclude the arm from the falling behaviours
        //Because controlStiffness only sets the scaling on effectors when falling we also have to set the arm muscles here.
        NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
        Assert(rollUpTask);
        NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
        Assert(catchFallTask);
        if (m_reachArm == &m_reachLeft)
        {
          rollUpTask->m_excludeMask |= bvmask_ArmLeft;
          catchFallTask->m_excludeMask |= bvmask_ArmLeft;
          m_body->setStiffness(m_parameters.armStiffness*strengthScale, m_armsDamping, bvmask_ArmLeft);
        }
        else
        {
          rollUpTask->m_excludeMask |= bvmask_ArmRight;
          catchFallTask->m_excludeMask |= bvmask_ArmRight;
          m_body->setStiffness(m_parameters.armStiffness*strengthScale, m_armsDamping, bvmask_ArmRight);
        }

      }//if (m_falling)

	    //lower the cleverIK strength if shotRelax has completed on the upperbBody
      float cleverIKStrength = 4.f;
      if (m_relaxTimeUpper >= m_relaxPeriodUpper && m_relaxPeriodUpper != 0.f)
        cleverIKStrength = 0.5f;

	    //bool shrug;
	    bool delayMovement;
      if(m_reachArm == &m_reachLeft)
      {
		    //shrug = !(m_hitTimeLeft >  m_injuredArm.shrugTime  || !m_shrug);
        //Stop the non-injured arm beating the injured arm to it's destination
        delayMovement = (m_reachRightEnabled && m_injuredRArm && m_parameters.allowInjuredArm
          && m_reachArm->dist2Target < m_reachRight.dist2Target);
        cleverIKStrength *= m_cleverIKStrengthMultLeft;
      }
      else
      {
		    //shrug = !(m_hitTimeRight >  m_injuredArm.shrugTime  || !m_shrug);
        //Stop the non-injured arm beating the injured arm to it's destination
		    delayMovement = (m_reachLeftEnabled && m_injuredLArm && m_parameters.allowInjuredArm
			    && m_reachArm->dist2Target < m_reachLeft.dist2Target);
        cleverIKStrength *= m_cleverIKStrengthMultRight;
      }


    //NB: sets the opposeGravity of the arm to 1
      m_armTwist =  m_character->reachForBodyPart(
        m_body,
        m_reachArm,
        delayMovement,
        cleverIKStrength,
        false,
        m_parameters.bustElbowLift,
        m_parameters.bust);

    }

    bool NmRsCBUShot::reachLeft_exitCondition()
    {
      return (m_bcrArms || m_newReachL || m_hitTimeLeft > m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound|| 
        m_character->getCharacterConfiguration().m_leftHandState == CharacterConfiguration::eHS_Rifle ||
        !(m_character->getCharacterConfiguration().m_leftHandState == CharacterConfiguration::eHS_Free || m_parameters.allowLeftPistolRFW) || 
        !m_parameters.reachForWound ||
              (m_defaultArmMotion.releaseLeftWound && !m_injuredLArm && m_hitTimeLeft > m_parameters.timeBeforeReachForWound + m_parameters.alwaysReachTime))
             && !(m_parameters.useExtendedCatchFall && m_onGroundEnabled);
    }
    void NmRsCBUShot::reachLeft_exit()
    {
			//Wrist angles not set by shot (could be left in reachForWound orientation)
      getLeftArmInputData()->getWrist()->setDesiredAngles(0.f, 0.f, 0.f);

     if (reachRight_exitCondition())//if Both Arms are no longer reaching
     {
       m_headLookAtWound = false;
       if (!m_feedbackSent_FinishedLookingAtWound)//mmmmtodo add label to this feedback
         {
           ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
           Assert(feedback); // should exist
           m_feedbackSent_FinishedLookingAtWound = true;
           // add feedback.
           feedback->m_agentID = m_character->getID();
           feedback->m_argsCount = 0;
           strcpy(feedback->m_behaviourName, NMShotFeedbackName);
           feedback->onBehaviourEvent();
         }
       }
     m_newReachL = true;//so that arms can be restarted if necessary e.g. after impact for bcr
    }

    //-----------REACH RIGHT---------------------------------------
    bool NmRsCBUShot::reachRight_entryCondition()
    {
      return 
        (!m_bcrArms && m_parameters.reachForWound && m_hitTimeRight > m_parameters.timeBeforeReachForWound && 
         m_newReachR && m_hitTimeRight < m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound && 
         m_character->getCharacterConfiguration().m_rightHandState != CharacterConfiguration::eHS_Rifle &&
         (m_character->getCharacterConfiguration().m_rightHandState == CharacterConfiguration::eHS_Free || m_parameters.allowRightPistolRFW) &&
         (!m_defaultArmMotion.releaseRightWound || m_hitTimeRight < m_parameters.timeBeforeReachForWound + m_parameters.alwaysReachTime) 
        )
        || (m_parameters.useExtendedCatchFall && m_onGroundEnabled);
    }
    void NmRsCBUShot::reachRight_entry()
    {
#if ART_ENABLE_BSPY
	  bspyLogf(info, L"entered reach right: %.3f", m_woundRPart);
#endif
      m_reachRight.arm = kRightArm;
      m_reachRight.direction = -1.f;
      m_reachRight.normal = m_woundRNormal;
      m_reachRight.offset = m_woundROffset;
      //m_reachRight.twist = -0.2f;
      m_reachRight.woundPart = m_woundRPart;
      m_newReachR = false;
    }
    bool NmRsCBUShot::reachRight_exitCondition()
    {
      return  (m_bcrArms || m_newReachR || m_hitTimeRight > m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound|| 
        m_character->getCharacterConfiguration().m_rightHandState == CharacterConfiguration::eHS_Rifle ||
        !(m_character->getCharacterConfiguration().m_rightHandState == CharacterConfiguration::eHS_Free || m_parameters.allowRightPistolRFW) || 
        !m_parameters.reachForWound ||
               (m_defaultArmMotion.releaseRightWound && !m_injuredRArm && m_hitTimeRight > m_parameters.timeBeforeReachForWound + m_parameters.alwaysReachTime))
              && !(m_parameters.useExtendedCatchFall && m_onGroundEnabled);
    }
    void NmRsCBUShot::reachRight_exit()
    {
			//Wrist angles not set by shot (could be left in reachForWound orientation)
      getRightArmInputData()->getWrist()->setDesiredAngles(0.f, 0.f, 0.f);

      if (reachLeft_exitCondition())//if Both Arms are no longer reaching
      {
        m_headLookAtWound = false;
        if (!m_feedbackSent_FinishedLookingAtWound)//mmmmtodo add label to this feedback
          {
            ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
            Assert(feedback); // should exist
            m_feedbackSent_FinishedLookingAtWound = true;
            // add feedback.
            feedback->m_agentID = m_character->getID();
            feedback->m_argsCount = 0;
            strcpy(feedback->m_behaviourName, NMShotFeedbackName);
            feedback->onBehaviourEvent();
        }
      }
      m_newReachR = true;//so that arms can be restarted if necessary e.g. after impact for bcr

    }
}
