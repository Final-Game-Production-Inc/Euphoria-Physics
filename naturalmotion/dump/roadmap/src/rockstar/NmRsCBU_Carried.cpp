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
 */


#include "NmRsInclude.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"
#include "NmRsCBU_Carried.h"

#include "fragment/instance.h"
#include "fragment/cache.h"

namespace ART
{
  NmRsCBUCarried::NmRsCBUCarried(ART::MemoryManager* services) : CBUTaskBase(services, bvid_carried),
    m_Initialized(false)
  {
    initialiseCustomVariables();
  }

  NmRsCBUCarried::~NmRsCBUCarried()
  {
  }

  void NmRsCBUCarried::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_CarriedTimer       = 0.0f;
  }

  void NmRsCBUCarried::onActivate()
  {
    Assert(m_character);
  }

  void NmRsCBUCarried::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUCarried::onTick(float timeStep)
  {
    NM_RS_DBG_LOGF(L"during");

    // Update the timer
    m_CarriedTimer += timeStep;

    if (!m_Initialized)
    {
      m_body->holdPose();
    }

    static float stillLim = 0.1f;
    bool still = getLeftLeg()->getThigh()->getLinearVelocity().Mag2() < stillLim*stillLim;

    // Get orientation data
    rage::Matrix34 tmCom;
    tmCom.Set(m_character->m_COMTM);
    m_bodySide = tmCom.a;  // faces right
    m_bodyUp = m_character->m_gUp;
    m_bodyBack.Cross(m_bodySide, m_bodyUp);

    // Disable collision on most of the body
    getRightLeg()->getThigh()->setCollisionEnabled(false);
    getSpine()->getPelvisPart()->setCollisionEnabled(false);
    getSpine()->getSpine0Part()->setCollisionEnabled(false);
    getSpine()->getSpine1Part()->setCollisionEnabled(false);
    getSpine()->getSpine2Part()->setCollisionEnabled(false);
    getSpine()->getSpine3Part()->setCollisionEnabled(false);

    // curl the hips
    static float f0 = 0.85f;
    static float f1 = 1.0f;

    getLeftLegInputData()->getHip()->setDesiredLean1(f0);
    getRightLegInputData()->getHip()->setDesiredLean1(f1);

    // straighten the knees
    static float k0 = -0.3f;
    static float k1 = -0.3f;
    static float k2 = -0.15f;

    getLeftLegInputData()->getKnee()->setDesiredAngle(still ? k2 : k0);
    getRightLegInputData()->getKnee()->setDesiredAngle(k1);

    // curl the torso
    static float f2 = 0.3f;
    static float f4 = 0.6f;
    static float f5 = 0.3f;

    getSpineInputData()->getSpine0()->setDesiredLean1(f2);
    getSpineInputData()->getSpine1()->setDesiredLean1(f2);
    getSpineInputData()->getSpine2()->setDesiredLean1(f4);
    getSpineInputData()->getSpine3()->setDesiredLean1(f5);

    // control stiffness
    static float s1 = 12.0f;
    static float s2 = 11.0f;
    static float s3 = 8.0f;
    static float s4 = 8.0f;
    static float s5 = 8.0f;
    static float s6 = 8.0f;

    m_body->setStiffness(s1, 1.0f);
    m_body->setStiffness(still ? s6 : s2, 1.0f, bvmask_LegLeft);
    m_body->setStiffness(s6, 1.0f, bvmask_LegRight);
    m_body->setStiffness(s3, 1.0f, bvmask_ArmLeft | bvmask_ArmRight);
    m_body->setStiffness(s4, 1.0f, bvmask_ClavicleLeft | bvmask_ClavicleRight);
    m_body->setStiffness(s5, 1.0f, bvmask_HighSpine);

    // widen shoulder limits
    static float l1 = 3.0f;
    static float l2 = 3.0f;
    static float l3 = 1.5f;

    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtShoulder_Left)))->setLimits(l1, l2, l3);
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtShoulder_Right)))->setLimits(l1, l2, l3);

    // widen spine limits
    static float c1 = 0.6f;
    static float c2 = 1.0f;
    static float c3 = 1.0f;

    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtSpine_0)))->setLimits(c1, c2, c3);
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtSpine_1)))->setLimits(c1, c2, c3);
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtSpine_2)))->setLimits(c1, c2, c3);
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtSpine_3)))->setLimits(c1, c2, c3);

    // Done
    m_Initialized = true;
    return eCBUTaskComplete;
  }

#if ART_ENABLE_BSPY
  void NmRsCBUCarried::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_CarriedTimer, false);
  }
#endif // ART_ENABLE_BSPY
}
