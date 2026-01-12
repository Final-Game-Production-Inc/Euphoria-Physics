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

#ifndef NM_RS_CBU_ARMSWINDMILLADAPTIVE_H 
#define NM_RS_CBU_ARMSWINDMILLADAPTIVE_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

  class NmRsCBUArmsWindmillAdaptive : public CBUTaskBase
  {
  public:

    NmRsCBUArmsWindmillAdaptive(ART::MemoryManager* services);
    ~NmRsCBUArmsWindmillAdaptive();

    void onActivate();
    void onDeactivate();

    CBUTaskReturn onTick(float timeStep);

    void moveArm(float timestep, NmRsHumanArm* arm, NmRsArmInputWrapper* inputData, float angle, float elbowBlendAngle, bool bendElbow, bool forwards);

    struct Parameters
    {
      float bodyStiffness;
      float armStiffness;
      float angSpeed;
      float amplitude;
      float phase;
      float leftElbowAngle;
      float rightElbowAngle;
      float lean1mult;
      float lean1offset;
      float elbowRate;
      int   armDirection;
      unsigned int effectorMask;
      bool  disableOnImpact;
      bool  setBackAngles;
      bool  useAngMom;
      bool  bendLeftElbow;
      bool  bendRightElbow;
    } m_parameters;

    void updateBehaviourMessage(const MessageParamsBase* const params);
    inline void setUseArms(bool useLeft,bool useRight){m_doLeftArm = useLeft; m_doRightArm = useRight; }
    inline void setAngle(float angle){m_armsWindmillAdaptiveAngle = angle; }
    inline float getAngle(){return m_armsWindmillAdaptiveAngle; }

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    inline bool getUsingLeft() const { return m_doLeftArm; }
    inline bool getUsingRight() const { return m_doRightArm; }

  protected:

    void initialiseCustomVariables();

    rage::Vector3   m_comAngVel;
    rage::Vector3   m_rotationAxis;

    float           m_armsWindmillAdaptiveAngle;
    float           m_elbowAngle;

    float           m_ElbowBend;
    float           m_Elbow_w;
    float           m_bodyDamping;
    float           m_armDamping;

    bool            m_leftHandCollided;
    bool            m_doLeftArm;
    bool            m_rightHandCollided;
    bool            m_doRightArm;
  };
}

#endif // NM_RS_CBU_ARMSWINDMILLADAPTIVE_H


