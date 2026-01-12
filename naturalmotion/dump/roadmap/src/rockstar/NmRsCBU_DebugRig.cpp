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
 *
 * Debug only behaviour - Cycles through the effectors and moves them from 2*(min to max) 
 *
 */

#include "NmRsInclude.h"

#if ALLOW_DEBUG_BEHAVIOURS

#include "NmRsCBU_DebugRig.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

namespace ART
{
  NmRsCBUDebugRig::NmRsCBUDebugRig(ART::MemoryManager* services) : CBUTaskBase(services, bvid_debugRig)
  {
    initialiseCustomVariables();
  }

  NmRsCBUDebugRig::~NmRsCBUDebugRig()
  {
  }

  void NmRsCBUDebugRig::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
  }

  void NmRsCBUDebugRig::onActivate()
  {
    Assert(m_character);

    m_joint = 0;
    m_angleType = 0;
    m_time = 0.f;
    m_angle = 0.f;

    m_body->resetEffectors(kResetCalibrations | kResetAngles);
    float muscleStiff = m_parameters.muscleStiffness;
    m_body->setStiffness(m_parameters.m_stiffness, m_parameters.m_damping, bvmask_Full, &muscleStiff);

    //pin head with constraint
    rage::Vector3 m_attachmentPos;
    NmRsGenericPart* head = m_body->getSpine()->getHeadPart();
    m_attachmentPos = head->getPosition();
    m_character->fixPart(head->getPartIndex(), m_attachmentPos,m_attachmentPos, 0.0f ,m_bodyPartConstraint);
  }

  void NmRsCBUDebugRig::onDeactivate()
  {
    Assert(m_character);

    //release constraint on head
    m_character->ReleaseConstraintSafetly(m_bodyPartConstraint);


    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUDebugRig::onTick(float timeStep)
  {

    if (m_parameters.joint >= 0 && m_parameters.joint < m_character->getNumberOfEffectors())
      m_joint = m_parameters.joint;
    NM_RS_DBG_LOGF(L"debugRig Active");
    NM_RS_DBG_LOGF(L"- testCharacter m_Joint  %i", m_joint);
    NmRsEffectorBase *effector = m_character->getEffectorDirect(m_joint);
    float lowerLimit = 0.f,upperLimit = 0.f;

    if (effector->is3DofEffector())
    {
      NmRs3DofEffector *threedof = (NmRs3DofEffector *)effector;
      switch (m_angleType)
      {
      case 0://twist
        lowerLimit = threedof->getMinTwist();
        upperLimit = threedof->getMaxTwist();
        m_angle = rage::Clamp(2.f*(lowerLimit + m_time*1.f*(upperLimit-lowerLimit)), -9.99f, 9.99f);
        if (((m_angle > 2.f*upperLimit) && (upperLimit >= 0.f)) || ((m_angle < 2.f*upperLimit) && (upperLimit < 0.f))) 
        {
          m_angle = 0.f;
          m_time = 0.f;
          m_angleType++;
        }

        threedof->setDesiredTwist(m_angle);
        NM_RS_DBG_LOGF(L"3Dof Twist = %f", m_angle);

        break;
      case 1://lean1
        lowerLimit = threedof->getMinLean1();
        upperLimit = threedof->getMaxLean1();
        m_angle = rage::Clamp(2.f*(lowerLimit + m_time*1.f*(upperLimit-lowerLimit)), -9.99f, 9.99f);
        if (((m_angle > 2.f*upperLimit) && (upperLimit >= 0.f)) || ((m_angle < 2.f*upperLimit) && (upperLimit < 0.f))) 
        {
          m_angle = 0.f;
          m_time = 0.f;
          m_angleType++;
        }

        threedof->setDesiredLean1(m_angle);
        NM_RS_DBG_LOGF(L"3Dof Lean1 = %f", m_angle);
        break;
      case 2://lean2
        lowerLimit = threedof->getMinLean2();
        upperLimit = threedof->getMaxLean2();
        m_angle = rage::Clamp(2.f*(lowerLimit + m_time*1.f*(upperLimit-lowerLimit)), -9.99f, 9.99f);
        if (((m_angle > 2.f*upperLimit) && (upperLimit >= 0.f)) || ((m_angle < 2.f*upperLimit) && (upperLimit < 0.f))) 
        {
          m_angle = 0.f;
          m_joint = m_joint + 1;
          m_time = 0.f;
          m_angleType = 0;
        }

        threedof->setDesiredLean2(m_angle);
        NM_RS_DBG_LOGF(L"3Dof Lean2 = %f", m_angle);
        break;
      }
    }
    else
    {
      NmRs1DofEffector *onedof = (NmRs1DofEffector *)effector;
      lowerLimit = onedof->getMinAngle();
      upperLimit = onedof->getMaxAngle();
      m_angle = rage::Clamp(2.f*(lowerLimit + m_time*1.f*(upperLimit-lowerLimit)), -9.99f, 9.99f);
      if (((m_angle > upperLimit) && (upperLimit >= 0.f)) || ((m_angle < upperLimit) && (upperLimit < 0.f))) 
      {
        m_angle = 0.f;
        m_joint = m_joint + 1;
        m_time = 0.f;
      }

      onedof->setDesiredAngle(m_angle);
      NM_RS_DBG_LOGF(L"1Dof Angle = %f", m_angle);
    }

    NM_RS_DBG_LOGF(L"- testCharacter LowerLimit %f", lowerLimit);
    NM_RS_DBG_LOGF(L"- testCharacter UpperLimit %f", upperLimit);

    if (m_joint >= m_character->getNumberOfEffectors())
      m_joint = 0;
    m_time += m_parameters.speed*timeStep;
    return eCBUTaskComplete;
  }

#if ART_ENABLE_BSPY
  void NmRsCBUDebugRig::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_stiffness, true);
    bspyTaskVar(m_parameters.m_damping, true);

    bspyTaskVar(m_time, true);
    bspyTaskVar(m_joint, true);
    bspyTaskVar(m_angleType, true);

    bspyTaskVar(m_angle, true);

  }
#endif // ART_ENABLE_BSPY
}
#endif //ALLOW_DEBUG_BEHAVIOURS

