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

#ifndef NM_RS_CBU_CARRIED_H
#define NM_RS_CBU_CARRIED_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMCarriedFeedbackName      "Carried"

  class NmRsCBUCarried : public CBUTaskBase
  {
  public:

    NmRsCBUCarried(ART::MemoryManager* services);
    ~NmRsCBUCarried();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

    void initialiseCustomVariables();

  protected:

    rage::Vector3 m_bodyUp;
    rage::Vector3 m_bodySide;
    rage::Vector3 m_bodyBack;

    rage::Vector3 m_toV;

    float  m_CarriedTimer;

    bool m_Initialized;
  };
}

#endif // NM_RS_CBU_CARRIED_H


