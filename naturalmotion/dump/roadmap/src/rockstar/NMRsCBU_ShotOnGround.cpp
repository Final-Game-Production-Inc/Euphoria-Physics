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
#include "NmRsCBU_Catchfall.h"
#include "NmRsCBU_BodyWrithe.h"

namespace ART
{
     //----------------ON GROUND------------------------------------------------

    bool NmRsCBUShot::onGround_entryCondition()
    {
      NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
      Assert(catchFallTask);
      return m_parameters.useExtendedCatchFall && catchFallTask->isActive() && catchFallTask->isOnGround();
    }
    void NmRsCBUShot::onGround_entry()
    {
      NM_RS_DBG_LOGF(L"On Ground Entry");

      // set wound reaching parts to avoid badness
      m_woundLPart = m_parameters.bodyPart;
      m_woundRPart = m_parameters.bodyPart;

      NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
      Assert(catchFallTask);
      if(catchFallTask->isActive())
        catchFallTask->deactivate();

      NmRsCBUBodyWrithe* writheTask = (NmRsCBUBodyWrithe*)m_cbuParent->m_tasks[bvid_bodyWrithe];
      Assert(writheTask);

      writheTask->updateBehaviourMessage(NULL); // set parameters to defaults
      writheTask->m_parameters.m_armPeriod = 3.f;
      writheTask->m_parameters.m_backPeriod = 5.f;
      writheTask->m_parameters.m_legPeriod = 4.f;
      writheTask->m_parameters.m_armAmplitude = 1.f;
      writheTask->m_parameters.m_backAmplitude = 2.f;
      writheTask->m_parameters.m_legAmplitude = 0.5f;
      writheTask->m_parameters.m_armStiffness = 13.f;
      writheTask->m_parameters.m_backStiffness = 13.f;
      writheTask->m_parameters.m_legStiffness = 11.f;
      writheTask->m_parameters.m_armDamping = 0.3f;
      writheTask->m_parameters.m_backDamping = 0.3f;
      writheTask->m_parameters.m_legDamping = 0.3f;
      writheTask->m_parameters.m_rollOverFlag = false;
      writheTask->m_parameters.m_effectorMask = bvmask_Full;
      writheTask->m_parameters.m_blendArms = 0.1f;
      writheTask->m_parameters.m_blendLegs = 0.4f;
      writheTask->m_parameters.m_blendBack = 0.4f;
      writheTask->m_parameters.m_applyStiffness = false;

      if(!writheTask->isActive())
        writheTask->activate();

    }
    void NmRsCBUShot::onGround_tick(float UNUSED_PARAM(timeStep))
    {
      NM_RS_DBG_LOGF(L"On Ground During");

      float effectorStiffness = 0.5f;
      getLeftArm()->setBodyStiffness(getLeftArmInput(),   m_parameters.bodyStiffness, 0.5f, bvmask_UpperBody, &effectorStiffness);
      getRightArm()->setBodyStiffness(getRightArmInput(), m_parameters.bodyStiffness, 0.5f, bvmask_UpperBody, &effectorStiffness);
      getSpine()->setBodyStiffness(getSpineInput(),       m_parameters.bodyStiffness, 0.5f, bvmask_UpperBody, &effectorStiffness);

      // TODO: At some point, return feedback message to indicate we are stable on the ground.

    }
    bool NmRsCBUShot::onGround_exitCondition()
    {
      return !m_parameters.useExtendedCatchFall;
    }
    void NmRsCBUShot::onGround_exit()
    {
      NM_RS_DBG_LOGF(L"On Ground Exit");
      
      NmRsCBUBodyWrithe* writheTask = (NmRsCBUBodyWrithe*)m_cbuParent->m_tasks[bvid_bodyWrithe];
      Assert(writheTask);
      if(writheTask->isActive())
        writheTask->deactivate();
    }
}

