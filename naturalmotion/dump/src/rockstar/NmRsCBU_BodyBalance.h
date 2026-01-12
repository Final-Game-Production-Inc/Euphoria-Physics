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

#ifndef NM_RS_CBU_BODYBALANCE_H 
#define NM_RS_CBU_BODYBALANCE_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMBodyBalanceFeedbackName      "bodyBalance" 

  class NmRsCBUBodyBalance : public CBUTaskBase
  {
  public:

    NmRsCBUBodyBalance(ART::MemoryManager* services);
    ~NmRsCBUBodyBalance();

    struct Parameters;
    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);
    void MoveAwayFromPusher();

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

    struct Parameters
    {
      float m_armStiffness;//NB. Was m_bodyStiffness
      float m_elbow;
      float m_shoulder;
      float m_armDamping;//NB. Was m_damping
      rage::Vector3 m_headLookPos;
      int m_headLookInstanceIndex;
      float m_spineStiffness;//NB. Was m_headLookTimeout
      float m_headLookAtVelProb;

      float m_somersaultAngle;
      float m_somersaultAngleThreshold;
      float m_sideSomersaultAngle;
      float m_sideSomersaultAngleThreshold;

      float m_somersaultAngVel;
      float m_somersaultAngVelThreshold;
      float m_twistAngVel;
      float m_twistAngVelThreshold;
      float m_sideSomersaultAngVel;
      float m_sideSomersaultAngVelThreshold;

      float m_armsOutOnPushMultiplier;
      float m_armsOutOnPushTimeout;

      float m_returningToBalanceArmsOut;
      float m_armsOutStraightenElbows;
      float m_armsOutMinLean2 ;
      float m_spineDamping;//NB. Was m_bodyTurnTimeout
      float m_turnOffProb;
      float m_turn2TargetProb;
      float m_turn2VelProb;
      float m_turnAwayProb;
      float m_turnLeftProb;
      float m_turnRightProb;

      float m_elbowAngleOnContact;
      float m_bendElbowsTime;
      float m_bendElbowsGait;
      float m_hipL2ArmL2; 
      float m_shoulderL2; 
      float m_shoulderL1; 
      float m_shoulderTwist;

      bool m_useHeadLook;
      bool m_armsOutOnPush;
      bool m_useBodyTurn; 
      bool m_backwardsAutoTurn;
      bool m_backwardsArms;
      bool m_blendToZeroPose;
      float turnWithBumpRadius;
      
      //Brace
      float braceDistance;
      float targetPredictionTime;
      float minBraceTime;
      float timeToBackwardsBrace;
      float handsDelayMin;
      float handsDelayMax;
      float reachAbsorbtionTime;
      float braceStiffness;
      float braceOffset;
      //move
      float moveRadius;
      float moveAmount;
      float moveWhenBracing;

    } m_parameters;

  protected:

    void initialiseCustomVariables();
    void falling(float timeStep);

#if ART_ENABLE_BSPY
    rage::Vector3 m_angVel;
    rage::Vector3 m_angVelClamped;
#endif
    rage::Vector3 m_bodyUp;
    rage::Vector3 m_bodyRight;
    rage::Vector3 m_bodyBack;

    rage::Vector3 m_target;//Brace
    rage::Vector3 m_targetVel;//Brace
    rage::Vector3 m_leftHandPos;//Brace only for bspy
    rage::Vector3 m_rightHandPos;//Brace only for bspy

    float m_braceTime;//Brace
    float m_backwardsBraceTimer;//Brace
    float m_distanceToTarget;//Brace
    float m_handsDelay;//Brace
    float m_leftHandSeparation;//Brace
    float m_rightHandSeparation;//Brace

    float m_armsOutOnPushTimer;
    float m_armsOutOnPushThresh;

    float m_oldqTilt;
    float m_oldqSom;
    float m_oldqTwist;

    float m_bendElbowsTimer;
    float m_maxLean2OnPush;//Lifts arm(s) up if pushed from other side(behind). SET EVERY FOOT STEP
    float m_lookAtTimer;//Countdown to toggling headlook (whilst stepping) between target and velocity 
    float m_lookAtRandom;//Random number giving the ratio of LookAtTargets to LookAtVelocity. SET ON ACTIVATION
    float m_randomTurn;//Random number determining the type of Turn type. SET WHEN m_turnTimer < 0
    float m_turnTimer;//Countdown to randomizing turn type
    float m_bendTimer;//Bending over duration after falling down has begun (for hands and knees catchFall)
    float m_turnStepCount;//mmmmDrunk

    int m_lastFootState;//Remember the last foot state ie not stepping/stepping with left/stepping with right

    bool m_useCOMAngVel;
    bool m_characterIsFalling;
    bool m_lookInVelDir;//(whilst stepping)if true Look in velocity direction, if false look at target. SET WHEN m_lookAtTimer < 0
    bool m_turnLeft;//mmmmDrunk
    bool m_moveLeft;//mmmmDrunk
    bool m_doBrace;//Brace
    bool m_shouldBrace;//Brace
    bool m_braceLeft;//Brace
    bool m_braceRight;//Brace
    bool m_delayLeftHand;//Brace

  };
}

#endif // NM_RS_CBU_BODYBALANCE_H


