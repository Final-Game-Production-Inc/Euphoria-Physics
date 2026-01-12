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
      NM_RS_DBG_LOGF(L"entered reach left: %.3f", m_woundLPart);
#if ART_ENABLE_BSPY
	  bspyLogf(info, L"entered reach left: %.3f", m_woundLPart);
#endif
      m_reachLeft.arm = kLeftArm;
      m_reachLeft.direction = 1;
      m_reachLeft.normal = m_woundLNormal;
      m_reachLeft.offset = m_woundLOffset;
      m_reachLeft.twist = 0.2f;
      m_reachLeft.woundPart = m_woundLPart;
      m_newReachL = false;
    }

    void NmRsCBUShot::reachArm_tick(float /*timeStep*/)
    {
	    //lower the cleverIK strength if shotRelax has completed on the upperbBody
      float cleverIKStrength = 4.f;
      if (m_relaxTimeUpper >= m_relaxPeriodUpper && m_relaxPeriodUpper != 0.f)
        cleverIKStrength = 0.5f;

	    bool shrug;
	    bool delayMovement;
      if(m_reachArm == &m_reachLeft)
      {
		    shrug = !(m_hitTimeLeft >  m_injuredArm.shrugTime  || !m_shrug);
        //Stop the non-injured arm beating the injured arm to it's destination
        delayMovement = (m_reachRightEnabled && m_injuredRArm && m_parameters.allowInjuredArm
          && m_reachArm->dist2Target < m_reachRight.dist2Target);
        cleverIKStrength *= m_cleverIKStrengthMultLeft;
      }
      else
      {
		    shrug = !(m_hitTimeRight >  m_injuredArm.shrugTime  || !m_shrug);
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
        shrug,
        false);

    }

    bool NmRsCBUShot::reachLeft_exitCondition()
    {
      return m_bcrArms || m_newReachL || m_hitTimeLeft > m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound|| 
        m_character->getCharacterConfiguration().m_leftHandState == CharacterConfiguration::eHS_Rifle ||
        !(m_character->getCharacterConfiguration().m_leftHandState == CharacterConfiguration::eHS_Free || m_parameters.allowLeftPistolRFW) || 
        !m_parameters.reachForWound ||
        (m_defaultArmMotion.releaseLeftWound && !m_injuredLArm && !((m_parameters.useExtendedCatchFall && m_onGroundEnabled) || m_hitTimeLeft < m_parameters.timeBeforeReachForWound + m_parameters.alwaysReachTime));
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
      NM_RS_DBG_LOGF(L"entered reach right: %.3f", m_woundRPart);
#if ART_ENABLE_BSPY
	  bspyLogf(info, L"entered reach right: %.3f", m_woundRPart);
#endif
      m_reachRight.arm = kRightArm;
      m_reachRight.direction = -1.f;
      m_reachRight.normal = m_woundRNormal;
      m_reachRight.offset = m_woundROffset;
      m_reachRight.twist = -0.2f;
      m_reachRight.woundPart = m_woundRPart;
      m_newReachR = false;
    }
    bool NmRsCBUShot::reachRight_exitCondition()
    {
      return  m_bcrArms || m_newReachR || m_hitTimeRight > m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound|| 
        m_character->getCharacterConfiguration().m_rightHandState == CharacterConfiguration::eHS_Rifle ||
        !(m_character->getCharacterConfiguration().m_rightHandState == CharacterConfiguration::eHS_Free || m_parameters.allowRightPistolRFW) || 
        !m_parameters.reachForWound ||
        (m_defaultArmMotion.releaseRightWound && !m_injuredRArm && !((m_parameters.useExtendedCatchFall && m_onGroundEnabled) || m_hitTimeRight < m_parameters.timeBeforeReachForWound + m_parameters.alwaysReachTime));
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
