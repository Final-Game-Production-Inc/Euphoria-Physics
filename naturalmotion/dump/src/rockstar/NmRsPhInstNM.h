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

#ifndef NM_RS_PHINST_H
#define NM_RS_PHINST_H

namespace ART
{
  class NmRsCharacter;

#define NM_RS_INSTNM_CLASS_TYPE   (100)

  rage::phInst *createPhInst(NmRsCharacter *character);
  void deletePhInst(rage::phInst *phInst);

  class phInstNM : public rage::phInst
  {
  public:
    virtual void PreComputeImpacts (rage::phContactIterator impacts);
    virtual int GetClassType () const  { return NM_RS_INSTNM_CLASS_TYPE; }
    virtual int GetNMAgentID() const;

    NmRsCharacter    *m_character;
  };
}

#endif // NM_RS_PHINST_H

