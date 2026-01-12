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

#ifndef NM_RS_CBU_HARDCONSTRAINT_H
#define NM_RS_CBU_HARDCONSTRAINT_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

  class NmRsCBUHardConstraint : public CBUTaskBase
  {
  public:
    NmRsCBUHardConstraint(ART::MemoryManager* services);
    ~NmRsCBUHardConstraint();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      int m_partIndex[21];
      bool m_constrainOrientation[21];
      bool m_constrainTranslation[21];
      bool m_blendWithPreviousFrame[21];
      int  m_FramesSinceTMUpdate;
      bool m_applyTeleporation;
      bool m_stabiliseHead;
      bool m_telportedVelOnDeactivation;
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:

    void initialiseCustomVariables();
    void updateConstraint(int partIndex, bool constraintO, bool constraintT, bool blendWithPreviousFrame);
    void removeConstraint(int partIndex);
    void getIncomingTranformByBodyPart(rage::Matrix34 &matrix, int partIndex, bool blendWithPreviousFrame);
    void applyVelocityOnDeactivation();

    const SpineSetup *m_spine;
    rage::phConstraint *m_hardConstraintOnCharacter[21];
    rage::phConstraint *m_hardConstraintRotationOnCharacter[21];
  };
}

#endif // NM_RS_CBU_HARDCONSTRAINT_H
