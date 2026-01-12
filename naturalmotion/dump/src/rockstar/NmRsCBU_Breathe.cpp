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
 * Rolling motion. Apply when the character is to be tumbling, either down a hill,
 * after being blown by an explosion, or hit by a car.
 * The character is in a rough foetal position, he puts his arms out to brace against
 * collisions with the ground, and he will relax after he stops tumbling.
 *
 * Roll Up behaviour. This is a pre-condition type behaviour which has to occur
 * before the character starts rolling or tumbling. Rolling and tumbling are
 * largely dependent on the shape of the character before it hits the ground/terrain
 * and the slope etc. of the ground/terrain. Part of the post-ground impact suite of behaviours.
 *
 * TDL current implementation simple curls the character up in proportion to how fast he's rotating
 * This behaviour is designed to transition in from a fall behaviour as the character approaches the ground
 */


#include "NmRsInclude.h"
#include "NmRsCBU_Breathe.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"

namespace ART
{
     NmRsCBUBreathe::NmRsCBUBreathe(ART::MemoryManager* services) :
      CBUTaskBase(services, bvid_breathe),
      m_spine(0)
    {
      initialiseCustomVariables();
    }

    NmRsCBUBreathe::~NmRsCBUBreathe()
    {
    }

    void NmRsCBUBreathe::initialiseCustomVariables()
    {
      m_mask = bvmask_Spine;

      m_breatheTimer       = 0.0f;
    }

    void NmRsCBUBreathe::onActivate()
    {
      Assert(m_character);

      m_breatheTimer = 0;

      // locally cache the limb definitions
      m_spine = m_character->getSpineSetup();
    }

    void NmRsCBUBreathe::onDeactivate()
    {
      Assert(m_character);

      initialiseCustomVariables();

      m_spine         = NULL;
    }

    CBUTaskReturn NmRsCBUBreathe::onTick(float timeStep)
    {
      m_breatheTimer += timeStep;

      NM_RS_DBG_LOGF(L"during");

      Breathe();

      return eCBUTaskComplete;
    } 

    void NmRsCBUBreathe::Breathe()
    {
      static float amp = 1.0f;
      static float freq = 4.0f;
      float breathDiff = amp*0.05f*(NMutils::clampValue(20.f-m_breatheTimer,10.f,20.f))*
        rage::Sinf(freq*m_breatheTimer);

      static float neckFactor = 0.1f;
      nmrsSetLean1(m_spine->getSpine3(),nmrsGetDesiredLean1(m_spine->getSpine3())- 0.16f*breathDiff);
      nmrsSetLean1(m_spine->getSpine2(),nmrsGetDesiredLean1(m_spine->getSpine2())+ 0.12f*breathDiff);
      nmrsSetLean1(m_spine->getSpine1(),nmrsGetDesiredLean1(m_spine->getSpine1())- 0.06f*breathDiff);
      nmrsSetLean1(m_spine->getLowerNeck(),nmrsGetDesiredLean1(m_spine->getLowerNeck()) + neckFactor * breathDiff);

      static float stiffness = 10.0f;
      static float neckStiffness = 20.0f;
      m_spine->getSpine1()->setStiffness(stiffness, 1.0f);
      m_spine->getSpine2()->setStiffness(stiffness, 1.0f);
      m_spine->getSpine3()->setStiffness(stiffness, 1.0f);
      m_spine->getLowerNeck()->setStiffness(neckStiffness, 1.0f);
    }

#if ART_ENABLE_BSPY
    void NmRsCBUBreathe::sendParameters(NmRsSpy& spy)
    {
      CBUTaskBase::sendParameters(spy);

      bspyTaskVar(m_breatheTimer, false);
    }
#endif // ART_ENABLE_BSPY
}
