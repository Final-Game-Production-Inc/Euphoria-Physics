/*
 * Copyright (c) 2005-2013 NaturalMotion Ltd. All rights reserved. 
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

#ifndef NM_RS_FRICTION_HELPER_H
#define NM_RS_FRICTION_HELPER_H

#include "NmRsInclude.h"
#include "NmRsCommon.h"
#include "NmRsCBU_Shared.h"

namespace ART
{
  class NmRsCharacter;

  // Friction helper
  //
  class NmRsFrictionHelper {

  public:

    #define MAX_ENTRIES 8

    void init(NmRsCharacter* character);
#if ART_ENABLE_BSPY
    void setPreScale(float scale, BehaviourMask mask, BehaviourID bvid = bvid_Invalid);
#else
    void setPreScale(float scale, BehaviourMask mask);
#endif
    void setPostScale(float scale, BehaviourMask mask);
    void setStepUpFriction(float left, float right) { m_stepUpScaleLeft = left; m_stepUpScaleRight = right; }
    void tick();

  protected:

    NmRsCharacter* m_character;

    float m_preScale[TotalKnownParts];
    float m_postScale[TotalKnownParts];
    float m_stepUpScaleLeft;
    float m_stepUpScaleRight;
#if ART_ENABLE_BSPY
    BehaviourID m_bvid[TotalKnownParts];
#endif
  };

} //namespace ART

#endif // NM_RS_FRICTION_HELPER_H
