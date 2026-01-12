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

#ifndef NM_RS_CBU_STAGGERFALL_H 
#define NM_RS_CBU_STAGGERFALL_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMStaggerFallFeedbackName      "staggerFall" 

  class NmRsCBUStaggerFall : public CBUTaskBase
  {
  public:
    NmRsCBUStaggerFall(ART::MemoryManager* services);
    ~NmRsCBUStaggerFall();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

    //inline void setBodyStiffness(float inputbodyStiffness){m_parameters.m_bodyStiffness = inputbodyStiffness;}

    struct Parameters
    {
      float m_armStiffness;
      float m_armDamping;
      float m_spineStiffness;
      float m_spineDamping;

      float m_armStiffnessStart;
      float m_armDampingStart;
      float m_spineStiffnessStart;
      float m_spineDampingStart;

      float m_timeAtStartValues;
      float m_rampTimeFromStartValues;

      float m_staggerStepProb;
      int   m_stepsTillStartEnd;
      float m_timeStartEnd;
      float m_rampTimeToEndValues;
      float m_lowerBodyStiffness;
      float m_lowerBodyStiffnessEnd;
      float m_predictionTime;
      float m_perStepReduction1;
      float m_leanInDirRate;
      float m_leanInDirMaxF;
      float m_leanInDirMaxB;
      float m_leanHipsMaxF;
      float m_leanHipsMaxB;
      float m_lean2multF;
      float m_lean2multB;

      float m_hipBendMult;
      float m_spineBendMult;
      bool  m_alwaysBendForwards;

      float pushOffDist;
      float maxPushoffVel;

      rage::Vector3 m_headLookPos;
      int m_headLookInstanceIndex;
      float m_headLookAtVelProb;


      float m_turnOffProb;
      float m_turn2TargetProb;
      float m_turn2VelProb;
      float m_turnAwayProb;
      float m_turnLeftProb;
      float m_turnRightProb;

      bool m_useHeadLook;
      bool m_useBodyTurn;
      bool m_upperBodyReaction;
    } m_parameters;

  protected:

    void initialiseCustomVariables();
    void ArmsBrace();
    void HeadLookAndTurn(float timeStep);
    void UpperBodyReaction(float timeStep, float reachDot, float reachSpeed, float sideDotVel);
    void falling();
    enum legState
    {
      ls_SwingLeg,
      ls_FootStrike,
      ls_Balance,
      ls_PushOff,
      ls_PushOffKnee,
      ls_ToeTimer,
    };

    rage::Vector3 m_fscLeftPos,m_fscRightPos;

    float m_leftToeTimer;
    float m_rightToeTimer;
    float m_staggerTimer;
    float m_armStiffness;
    float m_armDamping;
    float m_spineStiffness;
    float m_spineDamping;

    float m_lookAtTimer;//Countdown to toggling headlook (whilst stepping) between target and velocity 
    float m_lookAtRandom;//Random number giving the ratio of LookAtTargets to LookAtVelocity. SET ON ACTIVATION
    float m_randomTurn;//Random number determining the type of Turn type. SET WHEN m_turnTimer < 0
    float m_turnTimer;//Countdown to randomizing turn type

    int m_lastFootState;//Remember the last foot state ie not stepping/stepping with left/stepping with right
    int m_numSteps;
    //bSpy output only
    int spyLeftLegState;//0=swingleg, 1=footstrike, 2=balance, 3=pushoff, 4=toetimer 
    int spyRightLegState;//0=swingleg, 1=footstrike, 2=balance, 3=pushoff, 4=toetimer 

    bool m_lookInVelDir;//(whilst stepping)if true Look in velocity direction, if false look at target. SET WHEN m_lookAtTimer < 0
    bool m_balanceFailed;
    bool m_reacted;

  };
}

#endif // NM_RS_CBU_STAGGERFALL_H


