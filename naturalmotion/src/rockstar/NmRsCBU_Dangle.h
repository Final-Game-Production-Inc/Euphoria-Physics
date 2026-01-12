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

#ifndef NM_RS_CBU_DANGLE_H
#define NM_RS_CBU_DANGLE_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMDangleFeedbackName      "Dangle"

  class NmRsCBUDangle : public CBUTaskBase
  {
  public:
    NmRsCBUDangle(ART::MemoryManager* services);
    ~NmRsCBUDangle();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      bool m_doGrab;
      float m_grabFrequency;
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

    void Breathe();
    void Hanging(float timeStep);
    void ArmControl(float timeStep);
    void HeadControl();
    void GatherStateData();

    void initialiseCustomVariables();

  protected:

    rage::Vector3 m_bodyUp;
    rage::Vector3 m_bodySide;
    rage::Vector3 m_bodyBack;

    float m_DangleTimer;
    float m_ReachTimer;
    float m_ReachTimerStart;
    float m_TimeToSwitchPedalLegs;
    float m_TimeToSwitchHangingState;

    enum ReachForKnotStatus
    {
      kNone = 0,
      kReaching,
      kAborting
    } m_reachType;

    enum HangingStatus
    {
      kRelaxing = 0,
      kStruggling
    } m_hangType;

  };
}

#endif // NM_RS_CBU_DANGLE_H


