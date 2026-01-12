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
#include "NmRsCBU_HeadLook.h"

namespace ART
{
    //----------------HEAD LOOK------------------------------------------------
    // by default:
    //   looks at provided target
    //   or if no target looks straight ahead or in velocity direction. 
    // but if reachForWound is enabled, switches between looking at the wound and the target
    // (look at wound is triggered in shotNewHit)
    //-------------------------------------------------------------------------   
    bool NmRsCBUShot::headLook_entryCondition()
    {
      return m_parameters.useHeadLook;
    }

    void NmRsCBUShot::headLook_entry()
    {
      NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      Assert(headLookTask);
      headLookTask->initialiseCustomVariables();
      headLookTask->updateBehaviourMessage(NULL);
      headLookTask->m_parameters.m_pos = rage::Vector3(0,0,0);
      headLookTask->m_parameters.m_stiffness = m_parameters.bodyStiffness + 1.f;
      headLookTask->m_parameters.m_damping = 1.5f;
      headLookTask->m_parameters.m_alwaysLook = true;
      headLookTask->m_parameters.m_instanceIndex = -1;
      headLookTask->activate();
    }

    // toggleTimer initially is equal to headLookAtWoundMaxTimer (initialized in activate)
    //--------------------------------------------------------------------------------------
    void NmRsCBUShot::headLook_tick(float /*timeStep*/)
    {
      NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      Assert(headLookTask);

      //set stiffness and damping of headlook to what controlStiffness has just set them to
      headLookTask->m_parameters.m_stiffness = rage::SqrtfSafe(getSpine()->getLowerNeck()->getMuscleStrength());
      headLookTask->m_parameters.m_damping = getSpine()->getLowerNeck()->getMuscleDamping()/headLookTask->m_parameters.m_stiffness/2.f;

      // ensure sensible default target to look at when game didn't explicitly provide a target
      rage::Vector3 target = m_parameters.headLookPos;
      if (target.Dist(rage::Vector3(0,0,0)) < 0.01f)
      {        
        // update "state-machine" for switching between look at velocity and straight-ahead with hysteresis                
        rage::Vector3 comVel = m_character->m_COMvel;
        float comVelMag = comVel.Mag();
        if(m_headLook.state == m_headLook.hlFwd)
        {
          if(comVelMag > 0.6f)
            m_headLook.state = m_headLook.hlAtVel;
        }
        else if (m_headLook.state == m_headLook.hlAtVel)
        {
          if(comVelMag < 0.4f)
            m_headLook.state = m_headLook.hlFwd;
        }

		//looking behind in the direction of velocity causes the head to wobble wildly (especially when neck is weak)
		//Test to make sure this doesn't cause a lot of switching
		//Could give headlook a secondary target if 1st is out of range?
		rage::Vector3 back = m_character->m_COMTM.c;
		m_character->levelVector(comVel);
		m_character->levelVector(back);
		comVel.Normalize();
		back.Normalize();
		bool movingBackwards = (comVel.Dot(back) > 0.5f);
		if (movingBackwards)
		  m_headLook.state = m_headLook.hlFwd;

        // specify target based on current state of machine
        if(m_headLook.state == m_headLook.hlAtVel)
        {
          m_character->levelVector(comVel);
          target = getSpine()->getHeadPart()->getPosition() + comVel;
        }
        // look forward always
        else if (m_headLook.state == m_headLook.hlFwd)
        {
          rage::Vector3 fwd = -m_character->m_COMTM.c;
          m_character->levelVector(fwd);
          target = getSpine()->getHeadPart()->getPosition() + fwd;
        }

        // finally point down a little: looks better aesthetically
        target -= 0.2f*m_character->m_gUp;
      }

      // if reachForWound is running it will also trigger a lookAtWound. This overwrites the specified target.
      // but instead of always looking at wound it will toggle between wound and real target. 
      // the toggle is triggered by headLookToggleTimer being ramped up and down
      static float HEADLOOK_ATWOUND_DELAY = 0.4f;//Set to give bullet impact wobble then look down - may look a bit samey
      //m_headLookAtWound = true if m_pararameters.reachForWound and not head or neck hit
      bool doLookAtWoundToggle = m_headLookAtWound && 
        ((m_hitTimeLeft > HEADLOOK_ATWOUND_DELAY + m_parameters.timeBeforeReachForWound) ||
         (m_hitTimeRight > HEADLOOK_ATWOUND_DELAY + m_parameters.timeBeforeReachForWound));

      if(doLookAtWoundToggle)
      {
        // when actually looking at wound
        if (m_headLook.toggleTimer > 0.0f)
        {
          target = m_hitPointWorld;
          m_headLook.toggleTimer -= m_character->getLastKnownUpdateStep();
          // switch back to looking at target provided by game:
          if (m_headLook.toggleTimer < 0.0f)
          {
            m_headLook.toggleTimer = m_character->getRandom().GetRanged(-m_parameters.headLookAtHeadPosMaxTimer, -m_parameters.headLookAtHeadPosMinTimer);
            if (!m_feedbackSent_FinishedLookingAtWound)//mmmmtodo add label to this feeback
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
        }
        // when finished looking at wound: look at target provided or default position as specified above
        // i.e. don't overwrite above target
        else
        {
          // switch back to looking at wound
          m_headLook.toggleTimer += m_character->getLastKnownUpdateStep();
          if (m_headLook.toggleTimer > 0)
          {
            m_headLook.toggleTimer = m_character->getRandom().GetRanged(m_parameters.headLookAtWoundMinTimer, m_parameters.headLookAtWoundMaxTimer);
            m_feedbackSent_FinishedLookingAtWound = false;
          }          
        }
      }

      // actually set the target
      headLookTask->m_parameters.m_pos = target;
      headLookTask->m_parameters.m_instanceIndex = (-1);
#if ART_ENABLE_BSPY
      float distToOrigin = target.Dist(rage::Vector3(0,0,0));
      bspyScratchpad(m_character->getBSpyID(), "headLook", m_headLook.state);
      bspyScratchpad(m_character->getBSpyID(), "headLook", m_headLookAtWound);
      bspyScratchpad(m_character->getBSpyID(), "headLook", doLookAtWoundToggle);
      bspyScratchpad(m_character->getBSpyID(), "headLook", m_headLook.toggleTimer);
      bspyScratchpad(m_character->getBSpyID(), "headLook", target);
      bspyScratchpad(m_character->getBSpyID(), "headLook", distToOrigin);
      m_character->bspyDrawPoint(target, 0.3f, rage::Vector3(0.2f, 0.4f, 0.8f));
      m_character->bspyDrawLine(getSpine()->getHeadPart()->getPosition(), target, rage::Vector3(0.2f, 0.4f, 0.8f));
#endif
    }

    bool NmRsCBUShot::headLook_exitCondition()
    {
	  return !m_parameters.useHeadLook;
    }

    void NmRsCBUShot::headLook_exit()
    {
      NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      Assert(headLookTask);
      headLookTask->deactivate();

      // limbs todo implement resetAngles
      //getSpine()->getLowerNeck()->resetAngles();
      //getSpine()->getUpperNeck()->resetAngles();
    }
}