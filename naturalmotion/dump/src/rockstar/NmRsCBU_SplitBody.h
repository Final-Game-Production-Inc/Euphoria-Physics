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

#ifndef NM_RS_CBU_SPLITBODY_H
#define NM_RS_CBU_SPLITBODY_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

  class NmRsCBUSplitBody : public CBUTaskBase
  {
  public:
    NmRsCBUSplitBody(ART::MemoryManager* services);
    ~NmRsCBUSplitBody();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      BehaviourMask activePoseMask;
      BehaviourMask hardConstraintMask;
      BehaviourMask softConstraintMask;
      BehaviourMask collisionMask;
      BehaviourMask gravityCompensationMask;
      float stiffness;
      float damping;
      int leftHandWeaponIndex;
      int rightHandWeaponIndex;
      bool constrainHands;
      bool adjustJointLimits;
      bool controlHandOrientation;
      int rightHandStatus;
      int leftHandStatus;
      float controllerStiffness;
      float controllerDamping;
      float breathingScale;
      bool handStable;
      float handStableStiff;
      float handStableDamp;
      bool recoilRelax;
      float recoilRelaxAmount;
      float recoilRelaxTime;
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:

    void initialiseCustomVariables();
    void readParameters(bool start);

    void updateFireWeaponRelax(float timeStep);

  public:
    void fireWeapon(int hand = 1);

  protected:


#if ART_ENABLE_BSPY
    void debugPartPositions();
#endif

    float m_behaviourTime;

    const LeftLegSetup   *m_leftLeg;
    const RightLegSetup  *m_rightLeg;
    const LeftArmSetup   *m_leftArm;
    const RightArmSetup  *m_rightArm;
    const SpineSetup     *m_spine;

    ArmSetup* m_primaryArm;
    ArmSetup* m_supportArm;

    float m_supportHandError;
    float m_primaryHandError;

    rage::Vector3 m_constraintPos;
    bool m_constraintActive;

    struct SplitBodyArmData
    {
      float relaxTimer;
      float relaxScale;
    };

    SplitBodyArmData m_armData[2];

  };
}

#endif // NM_RS_CBU_SPLITBODY_H
