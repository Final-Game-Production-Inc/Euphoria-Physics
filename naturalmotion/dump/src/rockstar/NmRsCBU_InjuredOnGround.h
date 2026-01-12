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

#ifndef NM_RS_CBU_INJUREDONGROUND_H
#define NM_RS_CBU_INJUREDONGROUND_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMInjuredOnGroundFeedbackName      "injuredOnGround"

  class NmRsCBUInjuredOnGround : public CBUTaskBase
  {
  public:

    NmRsCBUInjuredOnGround(ART::MemoryManager* services);
    ~NmRsCBUInjuredOnGround();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      rage::Vector3 m_injury1LocalPosition;
      rage::Vector3 m_injury2LocalPosition; 
      rage::Vector3 m_injury1LocalNormal;
      rage::Vector3 m_injury2LocalNormal;
      rage::Vector3 m_attackerPos;
      int m_numInjuries; 
      int m_injury1Component;
      int m_injury2Component;
      bool m_dontReachWithLeft;
      bool m_dontReachWithRight;
      bool m_strongRollForce;  // used for males
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

    void GrabForWound();
    void Foetal(float severity = 0.5f);
    void Roll(float timeStep);
    void GatherStateData();
    void Breathe();
    void Pedal(float timestep);
    void HeadControl();

    void initialiseCustomVariables();

  protected:

    enum RollingInPainState
    {
      kRelaxed = 0,
      kRollLeft,
      kRollRight
    } m_painState;

    enum ReachType
    {
      kNone = 0,
      kLeft,
      kRight,
      kBoth
    } m_reachType;

    rage::phConstraintHandle m_ChestConstraint;
    rage::phConstraintHandle m_PelvisConstraint;

    ReachArm m_reachLeft, m_reachRight;
 
    rage::Vector3 m_bodyUp;
    rage::Vector3 m_bodySide;
    rage::Vector3 m_bodyBack;

    float m_injuredOnGroundTimer;
    float m_relaxTimer;
    float m_rollTimer;
    float m_PedalStateChangeCountdown;
    float m_wristLean2;

    float m_lastAngle;
    float m_CheatStrength;

    int m_braceWithLeftFrames;
    int m_braceWithRightFrames;

    bool m_downFacing;
    bool m_upFacing;
    bool m_onLeftSide;
    bool m_onRightSide;

    bool m_ChangerHeadTarget;
    bool m_LookingAtAttacker;

    bool m_ShotInLeftArm;
    bool m_ShotInRightArm;

    bool m_Initialized;
    bool m_doBrace;

  };
}

#endif // NM_RS_CBU_INJUREDONGROUND_H


