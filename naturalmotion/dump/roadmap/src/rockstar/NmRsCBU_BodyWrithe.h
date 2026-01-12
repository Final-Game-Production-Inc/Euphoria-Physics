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

#ifndef NM_RS_CBU_BODYWRITHE_H
#define NM_RS_CBU_BODYWRITHE_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;


  class NmRsCBUBodyWrithe : public CBUTaskBase
  {
  public:

    NmRsCBUBodyWrithe(ART::MemoryManager* services);
    ~NmRsCBUBodyWrithe();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      float m_armStiffness;
      float m_backStiffness;
      float m_legStiffness;//The stiffness of the character will determine how 'determined' a writhe this is - high values will make him thrash about wildly

      float m_armDamping;//damping amount, less is underdamped
      float m_backDamping;//damping amount, less is underdamped
      float m_legDamping;//damping amount, less is underdamped

      float m_armPeriod;//Controls how fast the writhe is executed, smaller values make faster motions
      float m_backPeriod;//Controls how fast the writhe is executed, smaller values make faster motions
      float m_legPeriod;//Controls how fast the writhe is executed, smaller values make faster motions

      float m_armAmplitude;
      float m_backAmplitude;//scales the amount of writhe. 0 = no writhe
      float m_legAmplitude;//scales the amount of writhe. 0 = no writhe

      float m_elbowAmplitude;
      float m_kneeAmplitude;
      bool  m_rollOverFlag;
      float m_blendArms;
      float m_blendBack;
      float m_blendLegs;
      bool  m_applyStiffness;

      BehaviourMask m_effectorMask;
    } m_parameters;
    inline void setWritheRollOverPeriod(float inputRollOverPeriod){m_rollOverPeriod = inputRollOverPeriod;}

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:

    void initialiseCustomVariables();
    void writeBackToEffectors();

    float           m_writheTimer;
    float           m_subTimer;
    float           m_noiseSeed;
    float           m_rollOverPeriod;
    float           m_rollOverDirection;
    float           m_rollOverTimer;
  };
}

#endif // NM_RS_CBU_BODYWRITHE_H


