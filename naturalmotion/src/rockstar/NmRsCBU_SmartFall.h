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

#ifndef NM_RS_CBU_SMARTFALL_H
#define NM_RS_CBU_SMARTFALL_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"
#include "NmRsStateMachine.h"
#include "NmRsCharacter.h"

#define HIGHFALL_AUTOMATIC_WEAPON_DETECTION 1

namespace ART
{
#define NMSmartFallFeedbackName      "smartFall"

  class NmRsCBUSmartFall : public CBUTaskBase, public StateMachine
  {
  public:
    NmRsCBUSmartFall(ART::MemoryManager* services);
    ~NmRsCBUSmartFall();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

    struct Parameters
    {
      float  bodyStiffness;
      float  bodydamping;
      float  catchfalltime;
      float  crashOrLandCutOff;
      float  pdStrength;
      float  pdDamping;
      float  catchFallCutOff;
      float  armsUp;
      float  legRadius;
      float  legAngSpeed;
      float  legAsymmetry;
      float  arms2LegsPhase;
      int  arms2LegsSync;
      float  armAngSpeed;
      float  armAmplitude;
      float  armPhase;
      bool  armBendElbows;
      float  aimAngleBase;
      float  forwardVelRotation;
      float  footVelCompScale;
      float  legL;
      float  sideD;
      float  forwardOffsetOfLegIK;
      float  legStrength;
      bool   balance;
      bool   orientateBodyToFallDirection;
      bool   alanRickman;
      bool   orientateTwist;
      float  orientateMax;
      bool   forwardRoll;
      bool   useZeroPose_withForwardRoll;
      bool   ignorWorldCollisions;
      bool   adaptiveCircling;
      bool   hula;
      float  minSpeedForBrace;
      float  maxSpeedForRecoverableFall;
      float  landingNormal;
      float  rdsForceMag;
      float  rdsTargetLinearVelocityDecayTime;
      float  rdsTargetLinearVelocity;
      bool   rdsUseStartingFriction;
      float  rdsStartingFriction;
      float  rdsStartingFrictionMin;
      float  rdsForceVelThreshold;
      int    initialState;
      bool   changeExtremityFriction;
      float  stopRollingTime;
      bool   teeter;
      float  teeterOffset;
      float  reboundScale;
      bool   forceHeadAvoid;
      float  cfZAxisSpinReduction;
      float  splatWhenStopped;
      float  blendHeadWhenStopped;
      float  spreadLegs;
      BehaviourMask reboundMask;
    } m_parameters;

  #define SF_STATES(_action) \
    _action(SF_Falling) \
    _action(SF_Glide) \
    _action(SF_BailOut) \
    _action(SF_PrepareForLanding) \
    _action(SF_NonRecoverableFall) \
    _action(SF_ForwardRoll) \
    _action(SF_CatchFall) \
    _action(SF_Balance) \
    _action(SF_Splat)

    enum States
    {
#define SF_ENUM_ACTION(_name) _name,
      SF_STATES(SF_ENUM_ACTION)
#undef SF_ENUM_ACTION
    };

  protected:

    void initialiseCustomVariables();

    // StateMachine
    virtual bool States( StateMachineEvent event, int state);

    void duringFall();
    void duringGlide();
    void duringNonRecoverableFall();
    void duringBailOut();
    void duringPrepareForLanding();
    void duringBalance();
    void duringCatchFall();
    void duringForwardRoll();
    void duringSplat();

    void sendFeedback(bool success = false);

    void orientationControllerTick();
    void calculateCommonVariables(); //Calculates m_goToCatchFallFlag, m_horComVel, m_impactVisible, m_orientationStateCurrent
    float getLeftLegLength() const;
    void probeEnvironmentAndCalcForFall(); // Function updates the following member variables: m_probeHit, m_normalDotUp, m_willLandOnFeet, m_predImpactTime and m_orientationState.

    void doBrace();

    rage::Vector3 m_cameraPos;
    rage::Vector3 m_horComVel;
    rage::Vector3 m_reboundVelocity;

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
    float   m_smoothCollidedWithWorld;

    float   m_catchFallTime;
    float   m_reboundTime;

    float   m_sinkRatio;
    float   m_sinkRate;
    float   m_sinkAccel;
    float   m_height;
    float   m_slope;

    // state variables for arms brace. TODO make brace state struct.
    float m_leftHandSeparation;
    float m_rightHandSeparation;

    NmRsCharacter::OrientationStates m_orientationState;
    NmRsCharacter::OrientationStates m_orientationStateTimeHalved;
    NmRsCharacter::OrientationStates m_orientationStateCurrent;

    bool  m_hasCollidedWithWorld;
    bool  m_supported;
    bool  m_controlOrientation;
    bool  m_willLandOnFeet;
    bool  m_feedbackSent; 
    bool  m_goToCatchFallFlag;
    bool  m_rollToBB;
    bool  m_probeHit;
    bool  m_probeDownHit;
    bool  m_impactVisible;
    bool  m_landingNormalOK;
    bool  m_dead;
    bool  m_teeterFailed;
    bool  m_balanceFailed;

    // State transition variables (computed in calculateCommonVariables to
    // simplify state transition code).
    bool m_restartFall;
    bool m_fallingDown;
    bool m_headOrFeetFirst;
    bool m_aboutToLand;
    bool m_hasCollided;
    bool m_landingUpright;
    bool m_impactDown;
    bool m_impactFront;
    bool m_impactPredictedFront;
    bool m_toExitFall;
    bool m_slowEnoughToRecover;
  };
}

#endif // NM_RS_CBU_SMARTFALL_H
