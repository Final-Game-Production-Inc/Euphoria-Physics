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

#ifndef NM_RS_CBU_RELAXUNWIND_H
#define NM_RS_CBU_RELAXUNWIND_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

  class NmRsCBURelaxUnwind : public CBUTaskBase
  {
  public:
    NmRsCBURelaxUnwind(ART::MemoryManager* services);
    ~NmRsCBURelaxUnwind();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      float spineStiffness;
      float armStiffness;
      float legStiffness;
      float relaxTime;
      float unwindTime;
      float unwindAmount;
    } m_parameters;

    enum RelaxUnwindWhichWayUp
    {
      kFaceDown,
      kFaceUp,
      kLeftSide,
      kRightSide
    } m_whichWayUp;

    struct LeanLeanTwist {
      void set(float l1, float l2, float t)
      {
        m_lean1 = l1;
        m_lean2 = l2;
        m_twist = t;
      }

      float getLean1() const {return m_lean1;}
      float getLean2()  const {return m_lean2;}
      float getTwist() const {return m_twist;}

      float m_lean1;
      float m_lean2;
      float m_twist;
    };

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:

    void initialiseCustomVariables();

    const LeftArmSetup *m_leftArm;
    const RightArmSetup *m_rightArm;
    const LeftLegSetup *m_leftLeg;
    const RightLegSetup *m_rightLeg;
    const SpineSetup *m_spine;

    float m_startingStiffness[TotalKnownHumanEffectors];
    rage::Vector3 m_startingPose[TotalKnownHumanEffectors];

    float m_behaviourTime;
  };
}

#endif // NM_RS_CBU_RELAXUNWIND_H


