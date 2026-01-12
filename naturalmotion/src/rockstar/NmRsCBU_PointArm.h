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

#ifndef NM_RS_CBU_POINTARM_H 
#define NM_RS_CBU_POINTARM_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMPointArmFeedbackName      "pointArm" 

  class NmRsCBUPointArm : public CBUTaskBase
  {
  public:
    NmRsCBUPointArm(ART::MemoryManager* services);
    ~NmRsCBUPointArm();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

    bool isLeftArmPointing() const {return m_parameters_Left.pointing;}
    bool isRightArmPointing() const {return m_parameters_Right.pointing;}

    struct Parameters
    {
      rage::Vector3 target;

      float twist;
      float armStraightness;
      float armStiffness;
      float armDamping;
      float reachLength;
      float pointSwingLimit;
      float advancedStaightness;
      float advancedMaxSpeed;

      int instanceIndex;

      bool useLeftArm;
      bool useRightArm;
      bool useZeroPoseWhenNotPointing;
      bool pointing;
      bool useBodyVelocity;

      Parameters()
      { 
        reachLength = 0.7f;
        pointing = true;
        useBodyVelocity = false;
        advancedStaightness = 0.f;
        advancedMaxSpeed = 0.f;
      }
    } m_parameters_Right;

    Parameters m_parameters_Left;

  protected:

    void pointTheArm(Parameters &parameters);
    void initialiseCustomVariables();

  };
}

#endif // NM_RS_CBU_POINTARM_H


