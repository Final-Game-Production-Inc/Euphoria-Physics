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

#include "NmRsInclude.h"
#include "NmRsFrictionHelper.h"
#include "NmRsCharacter.h"
#include "NmRsGenericPart.h"

#define DEBUG_FRICTION 0

namespace ART
{

  void NmRsFrictionHelper::init(NmRsCharacter* character)
  {
    Assert(character);
    m_character = character;

    for(int i = 0; i < m_character->getNumberOfParts(); ++i)
    {
      m_preScale[i] = 1.0f;
      m_postScale[i] = 1.0f;
#if ART_ENABLE_BSPY
      m_bvid[i] = bvid_Invalid;
#endif
    }

    m_stepUpScaleLeft = 1.0f;
    m_stepUpScaleRight = 1.0f;
  }

  // preScale is pretty much what the original friction multiplier does.
  // Individual behaviours set scalers via the setFrictionMultiplier
  // function.
  // 
#if ART_ENABLE_BSPY
  void NmRsFrictionHelper::setPreScale(float scale, BehaviourMask mask, BehaviourID bvid /*= bvid_Invalid*/)
#else
  void NmRsFrictionHelper::setPreScale(float scale, BehaviourMask mask)
#endif
  {
    Assert(scale < 20.0f);    
    Assert(scale >= 0.0f);

#if ART_ENABLE_BSPY & DEBUG_FRICTION
    bspyScratchpad(m_character->getBSpyID(), "setPreScale", getBvidNameSafe(bvid));
    bspyScratchpad_Bitfield32(m_character->getBSpyID(), "setPreScale", mask);
    bspyScratchpad(m_character->getBSpyID(), "setPreScale", scale);
#endif
    for(int i = 0; i < m_character->getNumberOfParts(); ++i)
    {
      if(m_character->isPartInMask(mask, i))
      {
#if ART_ENABLE_BSPY
        m_bvid[i] = bvid;
#endif
        m_preScale[i] = scale;
      }
    }
  }

  // postScale allows non-behaviours (read: individual direct invoke messages)
  // to scale the results of the current accumulated friction preScale values.
  // 
  void NmRsFrictionHelper::setPostScale(float scale, BehaviourMask mask)
  {
    Assert(scale < 10.0f);    
    Assert(scale >= 0.0f);

#if ART_ENABLE_BSPY & DEBUG_FRICTION
    bspyScratchpad_Bitfield32(m_character->getBSpyID(), "setPostScale", mask);
    bspyScratchpad(m_character->getBSpyID(), "setPostScale", scale);
#endif
    for(int i = 0; i < m_character->getNumberOfParts(); ++i)
    {
      if(m_character->isPartInMask(mask, i))
      {
        m_postScale[i] = scale;
      }
    }
  }

  void NmRsFrictionHelper::tick()
  {
    for(int i = 0; i < m_character->getNumberOfParts(); ++i)
    {
#if ART_ENABLE_BSPY & DEBUG_FRICTION
      bspyLogf(info, L"part %d friction %f %f (%s)", i, m_preScale[i], m_postScale[i], getBvidNameSafe(m_bvid[i]));
#endif
      if(m_character->isPartInMask(bvmask_FootLeft, i))
      {
        m_character->getGenericPartByIndex(i)->setFrictionMultiplier(m_preScale[i] * m_postScale[i] * m_stepUpScaleLeft);
      }
      else if(m_character->isPartInMask(bvmask_FootRight, i))
      {
        m_character->getGenericPartByIndex(i)->setFrictionMultiplier(m_preScale[i] * m_postScale[i] * m_stepUpScaleRight);
      }
      else
      {
        m_character->getGenericPartByIndex(i)->setFrictionMultiplier(m_preScale[i] * m_postScale[i]);
      }
    }
  }

} // namespace ART
