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

#ifndef NM_RS_CBU_DOGDEATH_H 
#define NM_RS_CBU_DOGDEATH_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

  class NmRsCBUDogDeath : public CBUTaskBase
  {
  public:
    NmRsCBUDogDeath(ART::MemoryManager* services) ;
    ~NmRsCBUDogDeath();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    void updateBehaviourMessage(const MessageParamsBase* const params);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void initialiseCustomVariables();

  protected:

    struct Parameters
    {
      float startStiffness;
      float endStiffness;
      float legStartStiffness;
      float legEndStiffness;
      float bodyRampDuration;
      float neckRampDuration;
      float legsRampDuration;
      float dampingScale;
      float headLift;
      float muscleStiffness;
      float legsFlexAmount;
      float legsFlexDuration;
      float tendonOffset;
      float helperImpulse;
      float angVelScale;
      BehaviourMask  angVelScaleMask;
    } m_parameters;


    void pointSpineJoint(NmRs3DofEffector* joint, const rage::Vector3& boneDirection, rage::Vector3& direction);

    void poseLimb(const ForeLegSetup* limb, const float angle, const float step);
    void poseLimb(const HindLegSetup* limb, const float angle, const float step);

    void poseJoint(NmRs1DofEffector* joint, float angle, const float step);
    void poseJoint(NmRs3DofEffector* joint, float angle, const float step);

    void solveTendon(const HindLegSetup* limb, float offset);
    void solveTendon(const ForeLegSetup* limb, float offset);

    float enforceSoftLimit(NmRs1DofEffector* effector, float uLimit, float lLimit, float limitStiff, float motorStiff);
    float enforceSoftLimit(NmRs3DofEffector* effector, float uLimit, float lLimit, float limitStiff, float motorStiff);


    float m_behaviourTime;

    float m_spineStiffness;
    float m_neckStiffness;
    float m_legStiffness;

    float m_legFlexLengths[LimbIKSetup::LimbIKSetupLimbs];

    float m_shoveMag;

    // useful debug variables
    float m_leftAnkleError;
    float m_leftFootError;
    float m_leftToeError;
    float m_rightAnkleError;
    float m_rightFootError;
    float m_rightToeError;
    float m_leftWristError;
    float m_leftFingerError;
    float m_leftNailError;
    float m_rightWristError;
    float m_rightFingerError;
    float m_rightNailError;
    float m_pelvisHeight;

    const HindLegSetup* m_leftLeg;
    const HindLegSetup* m_rightLeg;
    const ForeLegSetup* m_leftArm;
    const ForeLegSetup* m_rightArm;
    const SpineSetup* m_spine;

    bool m_collided;
    bool m_fallLeft;
  };
}

#endif // NM_RS_CBU_DOGDEATH_H


