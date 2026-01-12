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

#ifndef NM_RS_CBU_BALANCERCOLLISIONSREACTION_H 
#define NM_RS_CBU_BALANCERCOLLISIONSREACTION_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMBalancerCollisionsReactionFeedbackName              "balancerCollisionsReaction" 

  class NmRsCBUBalancerCollisionsReaction : public CBUTaskBase
  {
  public:

    NmRsCBUBalancerCollisionsReaction(ART::MemoryManager* services);
    ~NmRsCBUBalancerCollisionsReaction();

    struct Parameters;
    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

    void sendBalanceStateFeedback();
    void slump(float timeStep);
    void defaultArmMotion_armsBrace(float timeStep);
#if NM_UNUSED_CODE
    void exaggerateImpact();
#endif
    void setBalanceTime();
    void setHipPitch(float timeStep);
    void resetFrictionMultipliers();

    rage::Vector3 m_comVelPrevious;
    rage::Vector3 m_comVel_PreImpact;

    rage::Vector3 m_ubCollisionNormalTotal;
    rage::Vector3 m_lbCollisionNormalTotal;
    rage::Vector3 m_normal1stContactLocal;//Local to instance collided with
    rage::Vector3 m_pos1stContactLocal;//Local to instance collided with
    rage::Vector3 m_normal1stContact;//Local to instance collided with
    rage::Vector3 m_pos1stContact;//Local to instance collided with
    rage::Vector3 m_pos1stContactLocalForFOW;//Local to instance collided with
    rage::Vector3 m_pos1stContactForFOW;//Local to instance collided with
    float m_timeAfterImpact; 
    float m_sideOfPlane;
    float m_slumpStiffRLeg;
    float m_slumpStiffLLeg;
    float m_slumpStiffRKnee;
    float m_slumpStiffLKnee;
    float m_defaultArmMotiontimer1;
    float m_defaultArmMotiontimer2;
    float m_controlStiffnessStrengthScale;


#define BCR_OBSTACLE_TYPE(_type) \
  _type(Railing) \
  _type(Wall) \
  _type(Table) \
  _type(Unknown)

    // Environment Awareness
    enum ObstacleTypes
    {
#define BCR_ENUM_OBSTACLE_TYPES(_name) BCR_##_name,
      BCR_OBSTACLE_TYPE(BCR_ENUM_OBSTACLE_TYPES)
#undef BCR_ENUM_OBSTACLE_TYPES
    };

    int m_collisionLevelIndex;
    int m_balancerState; 
    int m_previousBalancerState;
    int m_numStepsAtImpact;
    ObstacleTypes m_obstacleType;

    bool m_impactOccurred;
    bool m_lbImpactOccurred;
    bool m_ubImpactOccurred;
    bool m_block;
    bool m_pushingOff;

    struct Parameters
    {

      rage::Vector3 objectBehindVictimPos;
      rage::Vector3 objectBehindVictimNormal;

      int  numStepsTillSlump; 
      float stable2SlumpTime;
      float exclusionZone;
      float footFrictionMultStart;
      float footFrictionMultRate;
      float backFrictionMultStart;
      float backFrictionMultRate;
      float impactLegStiffReduction;
      float slumpLegStiffReduction;
      float slumpLegStiffRate;
      float reactTime;
      float impactExagTime;

      float glanceSpinTime;
      float glanceSpinMag;
      float glanceSpinDecayMult;
      float ignoreColMassBelow;
      float ignoreColVolumeBelow;
      int ignoreColWithIndex;
      int slumpMode;
      int reboundMode;
      int forwardMode;
      bool braceWall;
      float timeToForward;
      float reboundForce;
      bool fallOverWallDrape;
      bool fallOverHighWalls;
      bool objectBehindVictim;

      bool snap;
      float snapMag;
      float snapDirectionRandomness;
      int snapHipType;
      bool snapLeftLeg;
      bool snapRightLeg;
      bool snapLeftArm;
      bool snapRightArm;
      bool snapSpine;
      bool snapNeck;
      bool snapPhasedLegs;
      float unSnapInterval;
      float unSnapRatio;
      bool snapUseTorques;

      float impactWeaknessZeroDuration;
      float impactWeaknessRampDuration;
      float impactLoosenessAmount;

    } m_parameters;

  protected:

    void initialiseCustomVariables();
    void decideBalancerState(float timeStep, int numOfSteps);
    void drape(float timeStep);     

    void probeEnvironment(const rage::Vector3& contactPos, const rage::Vector3& contactNormal);//const;
    ObstacleTypes getProbeResult(const rage::Vector3& contactPos, const rage::Vector3& contactNormal);

    rage::Vector3 m_probeHitPos;
    rage::Vector3 m_probeNormal;

    //Drape
    float m_drapeTimer;
    float m_rollAngVel;
    //Slump
    float m_footFrictionMult;
    float m_backFrictionMult;
    float m_amountOfMovement;
    //Lean AgainstWall and Slump
    float m_spineToWall;
    float m_stableTimer;

    bool m_balancing;//
    bool m_headCollided;//Drape
    bool m_slump;
    bool m_ignoreBlock;
    bool m_haveSnappedBack;
    bool m_waitingForAsynchProbe;
  };
}

#endif // NM_RS_CBU_BALANCERCOLLISIONSREACTION_H


