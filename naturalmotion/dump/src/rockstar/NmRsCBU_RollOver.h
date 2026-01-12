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

#ifndef NM_RS_CBU_ROLLOVER_H
#define NM_RS_CBU_ROLLOVER_H

#include "NmRsCBU_Shared.h"
#include "NmRsBodyLayout.h"

namespace ART
{
     class NmRsCharacter;

    #define NMRollOverFeedbackName      "rollOver"

    class NmRsCBURollOver : public CBUTaskBase
    {
    public:

      NmRsCBURollOver(ART::MemoryManager* services);
      ~NmRsCBURollOver();

      void onActivate();
      void onDeactivate();
      CBUTaskReturn onTick(float timeStep);

      enum RollingInPainState
      {
        kRelaxed = 0,
        kRollLeft,
        kRollRight
      } m_painState;

      struct Parameters
      {
        float m_rollingInPain;
        float m_rollingInPainIntensity;
      } m_parameters;

#if ART_ENABLE_BSPY
      virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
      void updateBehaviourMessage(const MessageParamsBase* const params);

      void GatherStateData();
      bool IsOnGround();

      void initialiseCustomVariables();

    protected:

      rage::Vector3 m_bodyUp;
      rage::Vector3 m_bodySide;
      rage::Vector3 m_bodyBack;

      const LeftArmSetup *m_leftArm;
      const RightArmSetup *m_rightArm;
      const LeftLegSetup *m_leftLeg;
      const RightLegSetup *m_rightLeg;
      const SpineSetup *m_spine;

      float m_rollOverTimer;  // behavior level
      float m_relaxTimer;
      float m_finishingTimer;

      bool m_currentRollDirection;

      bool m_downFacing;
      bool m_upFacing;
      bool m_onLeftSide;
      bool m_onRightSide;
    };
}

#endif // NM_RS_CBU_ROLLOVER_H


