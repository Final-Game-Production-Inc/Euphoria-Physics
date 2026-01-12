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

#ifndef NM_RS_SPY_H
#define NM_RS_SPY_H

#if ART_ENABLE_BSPY

#include "ART/bSpyServer.h"
#include "bspy/NmRsSpyPackets.h"
#include "NmRsHashtable.h"

namespace rage
{
  class phBound;
}

namespace ART
{
  class NmRsCharacter;
  class NmRsEngine;

  const unsigned  MAX_BSPY_AGENTS_2 = 24;//DO NOT EXCEED 31

  /**
   * NmRsSpy, implementation of bSpy data server for the Rockstar runtime
   */
  class NmRsSpy : public bSpyServer
  {
  public:

    NmRsSpy(ART::MemoryManager* services, NmRsEngine* engine);
    ~NmRsSpy();

    virtual void beginStep(float deltaTime);              // send start and what characters are inserted
    virtual void sendAgentActiveState();   // send what characters are inserted
    virtual void sendInfo();                              // send itms, behaviour and character information
    virtual void endStep(float deltaTime);
    virtual void sendUpdate(ART::NmRsCharacter* character);

    struct DynBoundTracker
    {
      unsigned int frameStamp;
      size_t sessionStamp;
    };
    typedef ART::NmRsHashtable<rage::phBound*, DynBoundTracker>   DynamicBoundTxTable;
    typedef ART::NmRsHashtable<rage::phBound*, unsigned int>      StaticBoundTxTable;

    DynamicBoundTxTable  *m_dynBoundTxTable;
    StaticBoundTxTable   *m_staticBoundTxTable;
  protected:

    virtual void onNewConnection();

    NmRsEngine           *m_engine;

    // bool-per-asset-ID representing whether or not we have transmitted it back to bSpy
    bool                  m_assetDescriptorSent[NUM_ASSETS];
  };

  void phBoundToShapePrimitive(const rage::phBound* bound, bSpy::bSpyShapePrimitive& shape);
}

BSPY_DEF_VARFN_BASIC(bspyCharacterVar, ART::CharacterUpdateVarPacket, float, f);
BSPY_DEF_VARFN_BASIC(bspyCharacterVar, ART::CharacterUpdateVarPacket, int, i);
BSPY_DEF_VARFN_BASIC(bspyCharacterVar, ART::CharacterUpdateVarPacket, bool, b);
BSPY_DEF_VARFN_STRING(bspyCharacterVar, ART::CharacterUpdateVarPacket);
BSPY_DEF_VARFN_VECTOR3(bspyCharacterVar, ART::CharacterUpdateVarPacket);

#define bspyCharacterVar(mbv) do_bspyCharacterVar(mbv, #mbv, false);

#endif // ART_ENABLE_BSPY

#endif // NM_RS_SPY_H

