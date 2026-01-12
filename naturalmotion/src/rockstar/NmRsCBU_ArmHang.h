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

#ifndef NM_RS_CBU_ARMHANG_H 
#define NM_RS_CBU_ARMHANG_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;
  
  class NmRsCBUArmHang : public CBUTaskBase
  {
  public:

    NmRsCBUArmHang(ART::MemoryManager* services);
    ~NmRsCBUArmHang();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      rage::Vector3 m_constraintPosition;
      rage::Vector3 m_targetPosition;
      int m_instanceIndex;
      int m_boundIndex;
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

   void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:
    
    void initialiseCustomVariables();

    rage::phConstraint *m_constraint;
    rage::phConstraint *m_constraintR;
  };
}

#endif // NM_RS_CBU_ARMHANG_H


