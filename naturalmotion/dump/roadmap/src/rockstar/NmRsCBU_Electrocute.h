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

#ifndef NM_RS_CBU_ELECTROCUTE_H 
#define NM_RS_CBU_ELECTROCUTE_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMElectrocuteFeedbackName      "electrocute" 

  class NmRsCBUElectrocute : public CBUTaskBase
  {
  public:
    NmRsCBUElectrocute(ART::MemoryManager* services);
    ~NmRsCBUElectrocute();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

    struct Parameters
    {
      float tazeMag;
      float initialMult;
      float largeMult;
      float largeMinTime;
      float largeMaxTime;
      float movingMult;
      float balancingMult;
      float airborneMult;
      float movingThresh;
      float tazeInterval;
      float directionRandomness;
      int hipType;
      bool leftLeg;
      bool rightLeg;
      bool leftArm;
      bool rightArm;
      bool spine;
      bool neck;
      bool phasedLegs;
      bool applyStiffness;
      bool useTorques;
    } m_parameters;

  protected:

    void initialiseCustomVariables();

    float m_tazeTimer;
    float m_largeTazeTimer;
    float m_subTimer;
    float m_direction;
    float m_noiseSeed;

    int m_intialSnapCount;
  };
}

#endif // NM_RS_CBU_ELECTROCUTE_H
