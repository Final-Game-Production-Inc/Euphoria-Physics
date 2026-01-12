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

#ifndef NM_RS_CBU_BUOYANCY_H
#define NM_RS_CBU_BUOYANCY_H

#include "NmRsCBU_TaskBase.h"

namespace ART
{
  class NmRsCBUBuoyancy : public CBUTaskBase
  {
  public:

    NmRsCBUBuoyancy(ART::MemoryManager* services);
    ~NmRsCBUBuoyancy();

    void onActivate();
    void onDeactivate();

    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      float buoyancy;
      float chestBuoyancy;
      float damping;
      rage::Vector3 surfacePoint;
      rage::Vector3 surfaceNormal;
      bool righting;
      float rightingStrength;
      float rightingTime;
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:

    void initialiseCustomVariables();

    float m_timeSubmerged;
  };
}
#endif // NM_RS_CBU_BUOYANCY_H
