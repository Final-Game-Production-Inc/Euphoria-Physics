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

#ifndef NM_RS_CBU_ROLLUP_H
#define NM_RS_CBU_ROLLUP_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMRollUpFeedbackName      "bodyRollUp"

  class NmRsCBURollUp : public CBUTaskBase
  {
  public:
    NmRsCBURollUp(ART::MemoryManager* services);
    ~NmRsCBURollUp();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      float m_stiffness;//10.f stiffness of whole body
      float m_useArmToSlowDown;//1.3f the degree to which the character will try to stop a barrel roll with his arms
      float m_armReachAmount;//1.4f the likeliness of the character reaching for the ground with its arms
      float m_legPush;//0.0f used to keep rolling down slope, 1 is full (kicks legs out when pointing upwards)
      float m_asymmetricalLegs;
      float m_noRollTimeBeforeSuccess;
      float m_rollVelForSuccess;
      float m_rollVelLinearContribution;
      float velocityScale; // scale perceived body velocity to avoid saturation of amountOfRoll measure.
      float velocityOffset; // decrease to create larger "dead zone" around zero velocity where character will be less rolled.
      bool  applyMinMaxFriction;
      BehaviourMask m_effectorMask;
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

    void initialiseCustomVariables();
 
    float m_armL;
    BehaviourMask m_excludeMask;//shot reachForWound can set this to be left arm and or rightArm
    bool  m_fromShot;
    
    // These params are set by setFallingReaction. 
    //They are only set to defaults in the constructor so that setFallingReaction values persist
    bool m_riflePose;


  protected:

    float plantArmGetBlend(float min, float max, float blendWidth, float x, float y);
    float plantArm(const NmRs3DofEffector* shoulderJoint, NmRsGenericPart* velocityPart, NmRsArmInputWrapper* input, float direction, float blend, float armL, float &relevance);

    inline float interpolate(float a, float b, float t){return a + (b-a)*t;}

    float           m_rollUpTimer;
    float           m_damping;
    float           m_blendL;
    float           m_blendR;
    float           m_timeNotRolling;
    float           m_rotCOMSmoothed;
    float           m_asymm;
    float           m_pedalSpeed;
    float           m_phase;
    float           m_extraSpread;
  };
}

#endif // NM_RS_CBU_ROLLUP_H


