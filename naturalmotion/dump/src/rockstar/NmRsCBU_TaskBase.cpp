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
#include "NmRsCBU_TaskBase.h"
#include "NmRsCBU_TaskManager.h"
#include "NmRsCharacter.h"

namespace ART
{

  void CBUTaskBase::init(NmRsCharacter* character, CBURecord* cbuParent)
  {
    Assert(m_character == 0);
    m_active = false;
    m_character = character;
    m_cbuParent = cbuParent;

    m_mask = bvmask_Full;
    m_blend = 1.0f;

    m_body = m_character->getBody();

    initialiseCustomVariables();
  }

  void CBUTaskBase::activate()
  {
    ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      // fill in feedback structure
      strcpy(feedback->m_behaviourName, getBvidNameSafe(this->getBvID()));
#if ART_ENABLE_BSPY
      char calledByString[ART_FEEDBACK_BHNAME_LENGTH];
      sprintf(calledByString, "%s", getBvidNameSafe(m_character->m_currentBehaviour));
      strcpy(feedback->m_calledByBehaviourName, calledByString);
#endif
      feedback->m_argsCount = 0;
      feedback->m_agentID = m_character->getID();
      feedback->onBehaviourStart();
    }

    if (m_cbuParent->activateTask(getBvID()))
    {
#if ART_ENABLE_BSPY
      BehaviourID currentBehaviour = m_character->m_currentBehaviour;
      m_character->setCurrentBehaviour(getBvID());
      m_updatePhase = kActivate;
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]);
#endif

      m_active = true;

      m_body->setup(getBvID(), m_priority, -10, m_blend, m_mask DEBUG_LIMBS_PARAMETER(s_phaseNames[kActivate]));

      m_cbuParent->m_tasks[getBvID()]->onActivate();

      m_body->postLimbInputs();

#if ART_ENABLE_BSPY
      m_character->setCurrentBehaviour(currentBehaviour);
#endif
    }
  }

  CBUTaskReturn CBUTaskBase::tick(float timeStep)
  {
    if(m_active)
    {
#if ART_ENABLE_BSPY
      m_updatePhase = kTick;
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]);
#endif

      m_body->setup(getBvID(), m_priority, 0, m_blend, m_mask DEBUG_LIMBS_PARAMETER(s_phaseNames[kTick]));

      CBUTaskReturn result = m_cbuParent->m_tasks[getBvID()]->onTick(timeStep);
      
      m_body->postLimbInputs();

      return result;
    }
    return eCBUTaskComplete;
  }

  void CBUTaskBase::deactivate()
  {
    if (m_cbuParent->deactivateTask(getBvID()))
    {
#if ART_ENABLE_BSPY
      BehaviourID currentBehaviour = m_character->m_currentBehaviour;
      m_character->setCurrentBehaviour(getBvID());
      m_updatePhase = kDeactivate;
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]);
#endif
      if(m_active)
      {
        m_body->setup(getBvID(), m_priority, 10, m_blend, m_mask DEBUG_LIMBS_PARAMETER(s_phaseNames[kDeactivate]));

        //must be set to false before onDeactivate otherwise:
        //  balancer might not shut down
        //  ClearAsynchProbe_IfNotInUse will not work properly
        m_active = false;
        m_cbuParent->m_tasks[getBvID()]->onDeactivate();

        m_body->postLimbInputs();
      }
#if ART_ENABLE_BSPY
      m_character->setCurrentBehaviour(currentBehaviour);
#endif
    }

    ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      // fill in feedback structure
#if ART_ENABLE_BSPY
      strcpy(feedback->m_behaviourName, getBvidNameSafe(this->getBvID()));
      char calledByString[ART_FEEDBACK_BHNAME_LENGTH];
      // todo correctly support sub behaviour feedback?
      sprintf(calledByString, "%s", getBvidNameSafe(m_character->m_currentBehaviour));
      strcpy(feedback->m_calledByBehaviourName, calledByString);
#endif
      feedback->m_argsCount = 0;
      feedback->m_agentID = m_character->getID();
      feedback->onBehaviourFinish();
    }
  }

  void CBUTaskBase::term()
  {
    // ensure the task is deactivated before we terminate
    Assert(!isActive());

    // null out the appropriate pointers to flag
    // ourselves as terminated
    m_character = 0;
    m_cbuParent = 0;
  }

#if ART_ENABLE_BSPY
  void CBUTaskBase::sendParameters(NmRsSpy& /*spy*/)
  {
    bspyTaskVar(m_bvid, false);
    bspyTaskVar(m_priority, false);
    bspyTaskVar(m_active, false);
    bspyTaskVar_Bitfield32(m_mask, false);
  }
#endif // ART_ENABLE_BSPY

  // !hdd! eliminate function overhead by making m_activeTasks of type BehaviourID...?

} // namespace ART