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
#include "NmRsPhInstNM.h"
#include "NmRsCharacter.h"

namespace ART
{
  extern rage::phInst *cachedPhInstPointer;

  void phInstNM::PreComputeImpacts (rage::phContactIterator impacts)
  {
    for (rage::phContactIterator impact = impacts; !impact.AtEnd(); impact++)
    {
      m_character->handleCollision(impact);
    }
  }

  int phInstNM::GetNMAgentID() const { return m_character->getID(); }

  rage::phInst *createPhInst(NmRsCharacter *character)
  {
    if (cachedPhInstPointer)
      return cachedPhInstPointer;

    phInstNM *inst = rage_new(phInstNM);
    inst->m_character = character;
    return inst;
  }

  void deletePhInst(rage::phInst *phInst)
  {
    if (phInst->GetClassType() == NM_RS_INSTNM_CLASS_TYPE)
      delete((phInstNM *)phInst);
  }
}
