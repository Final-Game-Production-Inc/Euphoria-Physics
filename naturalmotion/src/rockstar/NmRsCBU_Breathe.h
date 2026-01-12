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

#ifndef NM_RS_CBU_BREATHE_H
#define NM_RS_CBU_BREATHE_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
     class NmRsCharacter;

    #define NMBreatheFeedbackName      "breathe"

    class NmRsCBUBreathe : public CBUTaskBase
    {
    public:

      NmRsCBUBreathe(ART::MemoryManager* services);
      ~NmRsCBUBreathe();

      void onActivate();
      void onDeactivate();
      CBUTaskReturn onTick(float timeStep);

      struct Parameters
      {
        float m_stiffness;//10.f stiffness of whole body
        BehaviourMask m_effectorMask;
      } m_parameters;

#if ART_ENABLE_BSPY
      virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
      void updateBehaviourMessage(const MessageParamsBase* const params);

      void Breathe();

      void initialiseCustomVariables();

    protected:

      const SpineSetup *m_spine;

      float           m_breatheTimer;
    };
}

#endif // NM_RS_CBU_BREATHE_H


