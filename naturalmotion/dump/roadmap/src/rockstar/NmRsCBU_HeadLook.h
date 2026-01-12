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

#ifndef NM_RS_CBU_HEADLOOK_H 
#define NM_RS_CBU_HEADLOOK_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define HeadLookBSpyDraw 0

  class NmRsCBUHeadLook : public CBUTaskBase
  {
  public:
    NmRsCBUHeadLook(ART::MemoryManager* services);
    ~NmRsCBUHeadLook();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      rage::Vector3 m_pos;//The point being looked at
      rage::Vector3 m_vel;//The velocity of the point being looked at
      float m_stiffness;//Stiffness of the muscles
      float m_damping;//Damping  of the muscles
      int   m_instanceIndex; //levelIndex of object to be looked at. vel parameters are ignored if this is non -1 
      bool  m_eyesHorizontal; //Look but keeping eyes horizontal
      bool  m_alwaysEyesHorizontal; //Look but keeping eyes horizontal even when angle means there will probably be an error
      bool  m_alwaysLook;//Flag to force always to look
      bool  m_keepHeadAwayFromGround;//Flag to do as it says
      bool  twistSpine; // allow twist to be carried down through spine?
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

    void initialiseCustomVariables();
  protected:

    float           m_viewAngle;
    float           m_NLSwing1Min, m_NLSwing1Max;
    float           m_NLSwing2Min, m_NLSwing2Max, m_NLTwistMin;
    float           m_NLTwistMax, m_NLStrength, m_NLDamping;
    float           m_NUSwing1Min, m_NUSwing1Max, m_NUSwing2Min, m_NUSwing2Max, m_NUTwistMin;
    float           m_NUTwistMax;
    //not used float           m_NSwing1Min, m_NSwing1Max, m_NSwing2Min, m_NSwing2Max, 
    float           m_NTwistMin, m_NTwistMax;
    float           m_NStrength, m_NDamping;
    float           m_headOffset;

    bool          m_canLook;
 };
}

#endif // NM_RS_CBU_HEADLOOK_H


