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

#ifndef NM_RS_CBU_YANKED_H 
#define NM_RS_CBU_YANKED_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMYankedFeedbackName      "yanked" 

  class NmRsCBUYanked : public CBUTaskBase
  {
  public:

    NmRsCBUYanked(ART::MemoryManager* services);
    ~NmRsCBUYanked();

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

      int   m_stepsTillStartEnd;
      float m_perStepReduction;
      float m_timeStartEnd;
      float m_rampTimeToEndValues;
      float m_lowerBodyStiffness;
      float m_lowerBodyStiffnessEnd;

      float m_hipPitchForward;
      float m_hipPitchBack;
      float m_spineBend;
      float m_footFriction;
      float m_comVelRDSThresh;

      rage::Vector3 m_headLookPos;
      int m_headLookInstanceIndex;
      float m_headLookAtVelProb;

      float m_turnThresholdMin;
      float m_turnThresholdMax;
      //Wriggle params
      float m_hulaPeriod;
      float m_hipAmplitude;
      float m_spineAmplitude;
      float m_minRelaxPeriod;
      float m_maxRelaxPeriod;
      float m_rollHelp;
      float m_groundLegStiffness;
      float m_groundArmStiffness;
      float m_groundSpineStiffness;
      float m_groundArmDamping;
      float m_groundLegDamping;
      float m_groundSpineDamping;
      float m_groundFriction;

      bool m_useHeadLook;
    } m_parameters;

  protected:

    void initialiseCustomVariables();
    void ReachForRope();
    void HeadLookOrAvoid(float timeStep);
    void ToggleTurnAndUpperBodyReaction();      
    void OnGround();
    void RollOver();
    void Escapologist();
    void setFrictionMultipliers(float frictionMult); 

    rage::Vector3 m_turnTo;

    float m_YankTimer;
    float m_armStiffness;
    float m_armDamping;
    float m_spineStiffness;
    float m_spineDamping;

    float m_lookAtTimer;//Countdown to toggling headlook (whilst stepping) between target and velocity 
    float m_lookAtRandom;//Random number giving the ratio of LookAtTargets to LookAtVelocity. SET ON ACTIVATION
    float m_turnThreshold;
    float m_spineBendDir;
    float m_hipPitch;
    float m_spineTwistDir;

    float m_reachTimer;
    float m_hulaTimer;
    float m_rollOverPeriod;
    float m_rollOverTimer;
    
    int m_lastFootState;
    int m_hulaState;
    int m_hulaDirection;
    int m_stepsLeftThisCycle;
    int m_stepsThisCycle;
    int m_numStepsAtStart;

    bool m_characterIsFalling;
    bool m_lookInVelDir;//(whilst stepping)if true Look in velocity direction, if false look at target. SET WHEN m_lookAtTimer < 0
    bool m_turnLeft; 
    bool m_turn; 
  };
}

#endif // NM_RS_CBU_YANKED_H


