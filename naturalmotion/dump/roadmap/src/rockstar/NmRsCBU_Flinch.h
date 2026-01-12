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

#ifndef NM_RS_CBU_FLINCH_H 
#define NM_RS_CBU_FLINCH_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

  class NmRsCBUFlinch : public CBUTaskBase
  {
  public:
    NmRsCBUFlinch(ART::MemoryManager* services);
    ~NmRsCBUFlinch();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      rage::Vector3 m_pos;
      bool  m_lefthanded;
      bool  m_righthanded;
      float m_hand_dist_vert;
      float m_hand_dist_lr;
      float m_hand_dist_fb;
      float m_bodyStiffness;
      float m_bodyDamping;
      float m_backBendAmount;
      float m_noiseScale;
      bool  m_useRight;
      bool  m_useLeft;
      bool  m_newHit;
      bool  m_protectHeadToggle;
      bool  m_applyStiffness;
      bool  m_dontBraceHead;//mmmmmtodo should be option of m_protectHeadToggle
      bool  m_headLookAwayFromTarget;
      bool  m_useHeadLook;
      int   m_turnTowards;
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:

    void initialiseCustomVariables();
    //behaviours only referenced by Flinch
    void braceHead();
    bool armBracingHeadPose(NmRsHumanArm *limb, NmRsArmInputWrapper* input, const rage::Vector3 &spine3ToHeadDir, const rage::Vector3 &spine3JointPos);

    void braceFront();
    void chooseFlinchType();

    void setLowerArmCollision(bool enable);

    void controlStiffness_entry();
    void controlStiffness_tick(float timeStep);

    rage::Vector3 m_doomdirection;//Used in Brace Front = levelled and normalized com2Doom
    rage::Vector3 m_doompos;      //Used to init SpineTwist, HeadLook

    float m_braceHeadTimer;
    float m_braceFrontTimer;
    float m_legStraightnessMod;
    float m_angleFront;

    float m_lookBackAtDoomTimer;

    float m_noiseSeed;

    float m_armCollisionTimer;
    float m_relaxTime;
    float m_relaxPeriod;
    float m_controlStiffnessStrengthScale;
    int   m_turnTowardsMultiplier;

    bool  m_armCollisionDisabled;
    bool  m_braceHead;
    bool  m_rightSide;             // Used in Brace Front, false -> left side
    bool  m_wasFromBackBehind;
    bool  m_wasFromFrontBehind;
    bool  m_armsBracingHead;

  };
}

#endif // NM_RS_CBU_FLINCH_H


