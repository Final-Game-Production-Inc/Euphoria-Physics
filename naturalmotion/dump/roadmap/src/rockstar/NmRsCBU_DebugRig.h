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

#ifndef NM_RS_CBU_DEBUGRIG_H
#define NM_RS_CBU_DEBUGRIG_H

#if ALLOW_DEBUG_BEHAVIOURS

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

  class NmRsCBUDebugRig : public CBUTaskBase
  {
  public:
    NmRsCBUDebugRig(ART::MemoryManager* services);
    ~NmRsCBUDebugRig();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      float m_stiffness;
      float m_damping;
      float muscleStiffness;
      float speed;
      int joint; 
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:

    void initialiseCustomVariables();

    rage::phConstraintHandle m_bodyPartConstraint;//Constraint to world/object
    float m_angle;      
    float m_time;      
    int m_joint;
    int m_angleType;
  };
}

#endif //ALLOW_DEBUG_BEHAVIOURS

#endif // NM_RS_CBU_DEBUGRIG_H

