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

#ifndef NM_RS_CBU_BODYFOETAL_H
#define NM_RS_CBU_BODYFOETAL_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;


  class NmRsCBUBodyFoetal : public CBUTaskBase
  {
  public:

    NmRsCBUBodyFoetal(ART::MemoryManager* services);
    ~NmRsCBUBodyFoetal();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    //inline bool isActive() const { return m_balancerActive; }
    //inline bool hasFailed() const { return !isActive() && m_failed; }
    struct Parameters
    {
      float m_backTwist; 
      float m_asymmetrical;
      float m_stiffness;
      float m_damping;
      int m_randomSeed;
      BehaviourMask m_effectorMask;
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:

    void initialiseCustomVariables();

      void doAllTheThings();

      rage::mthRandom   m_randGen;
  };
}
#endif // NM_RS_CBU_BODYFOETAL_H


