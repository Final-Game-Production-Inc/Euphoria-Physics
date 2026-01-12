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

#ifndef NM_RS_CBU_HIGHFALL_H 
#define NM_RS_CBU_HIGHFALL_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"
#include "NmRsStateMachine.h"
#include "NmRsCharacter.h"

#define HIGHFALL_AUTOMATIC_WEAPON_DETECTION 1

namespace ART
{
#define NMHighFallFeedbackName      "highFall"

  class NmRsCBUHighFall : public CBUTaskBase, public StateMachine
  {
  public:
    NmRsCBUHighFall(ART::MemoryManager* services);
    ~NmRsCBUHighFall();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

    struct Parameters
    {
      float m_bodyStiffness;
      float m_bodydamping;
      float m_catchfalltime;
      float m_crashOrLandCutOff;
      float m_pdStrength;
      float m_pdDamping;
      // bool  useZeroPose;
      float m_catchFallCutOff;
      float m_armsUp;
      float m_legRadius;
      float m_legAngSpeed;
      float m_legAsymmetry;
      float m_arms2LegsPhase;
      int m_arms2LegsSync;
      float m_armAngSpeed;
      float m_armAmplitude;
      float m_armPhase;
      bool m_armBendElbows;
      float m_aimAngleBase;
      float m_forwardVelRotation;
      float m_footVelCompScale;
      float m_legL;
      float m_sideD;
      float m_forwardOffsetOfLegIK;
      float m_legStrength;
      bool  m_balance;
      bool  m_orientateBodyToFallDirection;
      bool  m_alanRickman;
      bool  m_orientateTwist;
      float m_orientateMax;
      bool  m_forwardRoll;
      bool  m_useZeroPose_withForwardRoll;
      bool  m_ignorWorldCollisions;
      bool  m_adaptiveCircling;
      bool  m_hula;
      float m_minSpeedForBrace;
      float m_maxSpeedForRecoverableFall;
      float m_landingNormal;
    } m_parameters;

  #define HF_STATES(_action) \
    _action(HF_Falling) \
    _action(HF_BailOut) \
    _action(HF_PrepareForLanding) \
    _action(HF_NonRecoverableFall) \
    _action(HF_BraceForImpact) \
    _action(HF_ForwardRoll) \
    _action(HF_CatchFall) \
    _action(HF_Balance)

    enum States
    {
#define HF_ENUM_ACTION(_name) _name,
      HF_STATES(HF_ENUM_ACTION)
#undef HF_ENUM_ACTION
    };

  protected:

    void initialiseCustomVariables();

    // StateMachine
    virtual bool States( StateMachineEvent event, int state);

    void orientationControllerTick();

    void duringFall();
    void duringNonRecoverableFall();
    void duringBailOut();
    void duringPrepareForLanding();
    void duringBalance();
    void duringCatchFall();
    void duringForwardRoll();

    void calculateCommonVariables(); //Calculates m_goToCatchFallFlag, m_horComVel, m_impactVisible, m_orientationStateCurrent
    float getLeftLegLength() const;
    void probeEnvironmentAndCalcForFall(); // Function updates the following member variables: m_probeHit, m_normalDotUp, m_willLandOnFeet, m_predImpactTime and m_orientationState.
    bool isCharacterFacingWhereItTravels(float facingCone = 0.2f) const;

    void handAnimationFeedback();

    rage::Vector3 m_cameraPos;
    rage::Vector3 m_horComVel;

#if ART_ENABLE_BSPY
    rage::Vector3 predCOMTMXAxis;
    rage::Vector3 predCOMTMYAxis;
    rage::Vector3 predCOMTMZAxis;
    rage::Vector3 m_probeHitPos;
#endif

    float   m_timer;
    float   m_predImpactTime;
    float   m_forwardVelMag;
    float   m_rollToBBtimer;
    float   m_averageComVel;
    float   m_hulaTime;
    float   m_hulaTotalTime;
    float   m_leftLegLength;
    float   m_normalDotUp;
    float   m_impactAnimationTime;

    NmRsCharacter::OrientationStates m_orientationState;
    NmRsCharacter::OrientationStates m_orientationStateTimeHalved;
    NmRsCharacter::OrientationStates m_orientationStateCurrent;

    HandAnimationType m_handAnimationType;

    bool  m_hasCollidedWithWorld;
    bool  m_controlOrientation;
    bool  m_willLandOnFeet;
    bool  m_messageSent; 
    bool  m_goToCatchFallFlag;
    bool  m_rollToBB;
    bool  m_probeHit;
    bool  m_fallRecoverable;
    bool  m_impactVisible;
    bool  m_landingNormalOK;
  };
}

#endif // NM_RS_CBU_<___>_H


