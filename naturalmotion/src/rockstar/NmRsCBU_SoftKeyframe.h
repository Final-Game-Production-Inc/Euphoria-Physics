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

#ifndef NM_RS_SOFTKEYFRAME_H 
#define NM_RS_SOFTKEYFRAME_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

#define NUM_PARTS 21

namespace ART
{
  class NmRsCharacter;

#define NMSoftKeyframeFeedbackName      "softKeyframe" 

  class NmRsCBUSoftKeyframe : public CBUTaskBase
  {
  public:
    NmRsCBUSoftKeyframe(ART::MemoryManager* services);
    ~NmRsCBUSoftKeyframe();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      float followMultiplier;
      float maxAcceleration;
      bool  goSlowerWhenWeaker;
      float yawOffset;
      rage::Vector3 targetPosition;
      BehaviourMask mask;
    } m_parameters;

    int m_numberOfFrames, m_frame;
    rage::Matrix34 *m_ctmTransforms;
    rage::Vector3 supposedToBe;
    float m_yawOffset;

    const rage::Vector3 getPosition(const rage::Matrix34& matrix) const;

    void updateBehaviourMessage(const MessageParamsBase* const params);
#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

  protected:
    void initialiseCustomVariables();

    const LeftArmSetup *m_leftArm;
    const RightArmSetup *m_rightArm;
    const LeftLegSetup *m_leftLeg;
    const RightLegSetup *m_rightLeg;
    const SpineSetup *m_spine;

    /*    enum Parts // so only currently works in RDR
    {
    pSpine0,
    pSpine1,
    pSpine2,
    pSpine3,
    pNeck,
    pHead,
    pLeftClavicle,
    pLeftUpperArm,
    pLeftLowerArm,
    pLeftHand,
    pRightClavicle,
    pRightUpperArm,
    pRightLowerArm,
    pRightHand,
    pPelvis,
    pLeftThigh,
    pLeftShin,
    pLeftFoot,
    pRightThigh,
    pRightShin,
    pRightFoot,
    }; */
  };
}

#endif // NM_RS_SOFTKEYFRAME_H



