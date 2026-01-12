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
     //----------------CHICKEN ARMS------------------------------------------------
    void NmRsCBUShot::chickenArms_applyDefaultPose()
    {
      // phase 2 todo implement some sort of pose storage...
      getLeftArmInputData()->getClavicle()->setDesiredLean1(-0.4f);
      getLeftArmInputData()->getClavicle()->setDesiredLean2(-0.15f);
      getLeftArmInputData()->getClavicle()->setDesiredTwist(0.f);
      getLeftArmInputData()->getShoulder()->setDesiredLean1(0.f);
      getLeftArmInputData()->getShoulder()->setDesiredLean2(1.15f);
      getLeftArmInputData()->getShoulder()->setDesiredTwist(-0.35f);
      getLeftArmInputData()->getElbow()->setDesiredAngle(1.f);
      getLeftArmInputData()->getWrist()->setDesiredLean1(-0.4f);
      getLeftArmInputData()->getWrist()->setDesiredLean2(0.15f);
      getLeftArmInputData()->getWrist()->setDesiredTwist(0.4f);

      getRightArmInputData()->getClavicle()->setDesiredLean1(-0.5f);
      getRightArmInputData()->getClavicle()->setDesiredLean2(-0.15f);
      getRightArmInputData()->getClavicle()->setDesiredTwist(0.f);
      getRightArmInputData()->getShoulder()->setDesiredLean1(0.05f);
      getRightArmInputData()->getShoulder()->setDesiredLean2(1.15f);
      getRightArmInputData()->getShoulder()->setDesiredTwist(-0.4f);
      getRightArmInputData()->getElbow()->setDesiredAngle(1.5f);
      getRightArmInputData()->getWrist()->setDesiredLean1(-0.25f);
      getRightArmInputData()->getWrist()->setDesiredLean2(0.25f);
      getRightArmInputData()->getWrist()->setDesiredTwist(0.4f);

      getSpineInputData()->getSpine0()->setDesiredLean1(0.05f);
      getSpineInputData()->getSpine1()->setDesiredLean1(0.05f);
      getSpineInputData()->getSpine2()->setDesiredLean1(0.05f);
      getSpineInputData()->getSpine3()->setDesiredLean1(-0.3f);
      getSpineInputData()->getUpperNeck()->setDesiredLean1(-0.3f);
      getSpineInputData()->getUpperNeck()->setDesiredLean2(-0.2f);
      getSpineInputData()->getLowerNeck()->setDesiredLean1(-0.3f);
    }
    bool NmRsCBUShot::chickenArms_entryCondition()
    {
      return m_parameters.chickenArms && m_newHit;
    }
    void NmRsCBUShot::chickenArms_entry()
    {
      m_chickenArmsTimeSpaz = 0.f;
      m_spineStiffness = m_parameters.bodyStiffness * 14.f/m_defaultBodyStiffness;
      m_spineDamping = 1.f;
      m_armsStiffness = m_parameters.bodyStiffness * 14.f/m_defaultBodyStiffness;
      m_armsDamping = 1.f;
      m_wristStiffness = 17.f;//m_parameters.bodyStiffness * 20.f/m_defaultBodyStiffness;
      m_neckStiffness = 14.f;//m_parameters.bodyStiffness * 20.f/m_defaultBodyStiffness;
    }
    void NmRsCBUShot::chickenArms_tick(float timeStep)
    {
      chickenArms_applyDefaultPose();

      m_chickenArmsTimeSpaz += timeStep;
      float spazPeriod = m_reactionTime;
      Assert(rage::Abs(spazPeriod) > 1e-10f);
      float strength = 1.f - rage::Min(m_chickenArmsTimeSpaz / spazPeriod, 1.f);
      getLeftArmInputData()->getShoulder()->setDesiredTwist(getLeftArmInputData()->getShoulder()->getDesiredTwist() + rage::Cosf(m_chickenArmsTimeSpaz * 13.f)*strength*1.f);
      getRightArmInputData()->getShoulder()->setDesiredTwist(getRightArmInputData()->getShoulder()->getDesiredTwist() + rage::Cosf(0.5f + m_chickenArmsTimeSpaz * 10.f)*strength*1.f);
      m_armsStiffness = 7.f + 7.f*rage::Min(m_chickenArmsTimeSpaz*2.f, 1.f);
      getLeftArmInputData()->getElbow()->setStiffness(m_armsStiffness, 1.f);
      getRightArmInputData()->getElbow()->setStiffness(m_armsStiffness, 1.f);
    }
    bool NmRsCBUShot::chickenArms_exitCondition()
    {
      return (m_hitTime > m_reactionTime) || m_newHit;
    }
}
