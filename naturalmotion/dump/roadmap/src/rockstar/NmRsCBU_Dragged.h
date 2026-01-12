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

#ifndef NM_RS_CBU_DRAGGED_H 
#define NM_RS_CBU_DRAGGED_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMDraggedFeedbackName              "dragged" 

  class NmRsCBUDragged : public CBUTaskBase
  {
  public:
    NmRsCBUDragged(ART::MemoryManager* services);
    ~NmRsCBUDragged();

    struct Parameters;
    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

    struct Parameters
    {
      float m_armStiffness;
      float m_armDamping;
      float m_armMuscleStiffness;
      float m_radiusTolerance;
      rage::Vector3 m_ropePos;
      int m_ropeAttachedToInstance;
      int m_ropedBodyPart;
      bool m_ropeTaut;
      bool m_playerControl;
      bool m_grabLeft;
      bool m_grabRight;

      float m_reach;       
      float m_armTwist;
      float m_lengthTolerance;
    } m_parameters;

  protected:

    void initialiseCustomVariables();
    void PullUp();
    void ReachForTarget(float timeStep);
    void GrabRope();
    void SetArmMuscles();
    void SendGrabbedFeedback(bool grabbedLeft, bool grabbedRight, bool leftHandIsUpperHand);

    rage::phConstraintHandle m_bodyPartConstraint;//Constraint to world/object
    rage::phConstraintHandle m_upperConstraint;//Constraint to world/object
    rage::phConstraintHandle m_lowerConstraint;//Constraint to world/object

    rage::Vector3 m_ropeAttachmentPos;
    float m_grabAlongDistance;
    float m_pullUpTimer;
    float m_timeTillRelax;
    float m_reachTimer;
    float m_taughtRopeLength;

    NmRsHumanArm *m_reachArm;
    NmRsHumanArm *m_ropeArm;

    bool m_reach;
    bool m_leftHandDominant;
    bool m_leftHandReaching;
    bool m_rightHandReaching;
    bool m_leftHandIsRopeHand;
    bool m_rightHandConstrained;
    bool m_leftHandPullup;
    bool m_rightHandPullup;
    bool m_grabbedLeft;
    bool m_grabbedRight;

  };
}

#endif // NM_RS_CBU_DRAGGED_H


