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

#ifndef NM_RS_CBU_CRUSHREACTION_H
#define NM_RS_CBU_CRUSHREACTION_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"
#include "NmRsStateMachine.h"

namespace ART
{
  class NmRsCharacter;

  class NmRsCBUCrushReaction : public CBUTaskBase, public StateMachine
  {
  public:
    NmRsCBUCrushReaction(ART::MemoryManager* services);
    ~NmRsCBUCrushReaction();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    // message parameters
    struct Parameters
    {
      char m_effectorMask[3]; //Two character body-masking value (see Active Pose notes for possible values)
      float m_stiffness;
      float m_damping;
      int m_obstacleID; // instance ID
      int m_flinchMode; // 0 for head protection, 1 for brace against incoming instance
      bool m_useInjuries;
    } m_parameters;

    enum FlinchModes
    {
      FM_FLINCH,
      FM_BRACE
    };

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:

    void initialiseCustomVariables();
    void updateGlobalState();
    void handleCollisions();

    // injury system
    void checkForInjury();
    bool checkJointsObstacleCollision(NmRsEffectorBase* joints[], int numJoints, int obsID);
    bool checkLimbObstacleCollision(NmRsGenericPart* limbParts[], int numParts, int obsID);
    void checkAndApplyInjuryByLimb();
    float applyInjurySafely(NmRsEffectorBase* effector, float newInjury);

    // StateMachine
    virtual bool States( StateMachineEvent event, int state);
    enum States{ 
      SM_INITIAL,
      SM_BI_FLINCH,
      SM_DURING_IMPACT,
      SM_TRY_BALANCE,
      SM_DURING_FALL,
      SM_AFTER_FALL
    };

    rage::Vector3
      m_obsPos,
      m_obsVel,
      m_headObstacleDir;

    float
      m_time,
      m_headObstacleDist,
      m_headObstacleSpeed2,
      m_obsMass,
      m_timeToCPA,
      m_distAtCPA,
      m_energy;

    int
      m_collisionCounter;

    bool m_doBrace;
    bool m_hasCrushed;
    bool m_isCrushing;
    bool m_crushOver;
    bool m_feetGroundContact;

  }; // class
} // nmsp

#endif // NM_RS_CBU_CRUSHREACTION_H


