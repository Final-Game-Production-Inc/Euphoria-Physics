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

#ifndef NM_RS_CBU_ANIMPOSE_H 
#define NM_RS_CBU_ANIMPOSE_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"
#include "NmRsLimbs.h"

namespace ART
{
  class NmRsCharacter;

#define AnimPoseBSpyDraw 0

  class NmRsCBUAnimPose : public CBUTaskBase
  {
  public:

    NmRsCBUAnimPose(ART::MemoryManager* services);
    ~NmRsCBUAnimPose();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      float stiffness;
      float damping;
      float muscleStiffness;
      float gravityCompensation;
      BehaviourMask effectorMask;
      bool overideHeadlook; 
      bool overidePointArm; 
      bool overidePointGun;

      float stiffnessLeftArm;
      float stiffnessRightArm;
      float stiffnessSpine;
      float stiffnessLeftLeg;
      float stiffnessRightLeg;

      float dampingLeftArm;
      float dampingRightArm;
      float dampingSpine;
      float dampingLeftLeg;
      float dampingRightLeg;

      float muscleStiffnessLeftArm;
      float muscleStiffnessRightArm;
      float muscleStiffnessSpine;
      float muscleStiffnessLeftLeg;
      float muscleStiffnessRightLeg;

      bool useZMPGravityCompensation;
      float gravCompLeftArm;
      float gravCompRightArm;
      float gravCompSpine;
      float gravCompLeftLeg;
      float gravCompRightLeg;
      int connectedLeftHand;
      int connectedRightHand;
      int connectedLeftFoot;
      int connectedRightFoot;
      int animSource;
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:

    void initialiseCustomVariables();
  };
}

#endif // NM_RS_CBU_ANIMPOSE_H


