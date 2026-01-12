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

#ifndef NM_RS_CBU_ONFIRE_H
#define NM_RS_CBU_ONFIRE_H

#include "NmRsCharacter.h"
#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
#define NMOnFireFeedbackName "onFire"

  class NmRsCBUOnFire : public CBUTaskBase
  {
  public:
    NmRsCBUOnFire(ART::MemoryManager* services);
    ~NmRsCBUOnFire();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

    struct Parameters
    {
      float staggerTime;
      float staggerLeanRate;
      float stumbleMaxLeanBack;
      float stumbleMaxLeanForward;
      float armsWindmillWritheBlend;
      float spineStumbleWritheBlend;
      float legsStumbleWritheBlend;
      float armsPoseWritheBlend;
      float spinePoseWritheBlend;
      float legsPoseWritheBlend;
      float rollTorqueScale;
      float predictTime;
      bool rollOverFlag;
      float maxRollOverTime;
      float rollOverRadius;
    } m_parameters;

  protected:
    void initialiseCustomVariables();
    void setActivateBehaviours();

    void duringOnFireUp(float timeStep);
    void duringOnFireFront();
    void duringOnFireBack();
    void duringOnFireLeft();
    void duringOnFireRight();

    NmRsCharacter::OrientationStates m_orientationState;

    float m_timer;
    float m_hipPitchAim;

  };
}

#endif // NM_RS_CBU_ONFIRE_H

