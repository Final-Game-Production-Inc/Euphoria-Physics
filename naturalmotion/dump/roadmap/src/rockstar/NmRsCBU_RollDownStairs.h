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

#ifndef NM_RS_CBU_ROLLDOWNSTAIRS_H 
#define NM_RS_CBU_ROLLDOWNSTAIRS_H 

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define RDSBSpyDraw 1

  class NmRsCBURollDownStairs : public CBUTaskBase
  {
  public:
    NmRsCBURollDownStairs(ART::MemoryManager* services);
    ~NmRsCBURollDownStairs();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      float m_Stiffness;
      float m_Damping;
      float m_ForceMag;
      float m_AsymmetricalForces;//unused
      float m_AsymmetricalLegs;
      float m_UseArmsToSlowDown;
      float m_ArmReachAmount;
      bool  m_UseZeroPose;
      bool  m_SpinWhenInAir;
      float m_LegPush;
      bool  m_TryToAvoidHeadButtingGround;
      float m_ArmL;
      float m_StiffnessDecayTime;
      float m_StiffnessDecayTarget;
      bool  m_UseCustomRollDir;
      float m_zAxisSpinReduction;
      float m_targetLinearVelocity;
      float m_targetLinearVelocityDecayTime;
      rage::Vector3 m_CustomRollDir;
      bool  m_onlyApplyHelperForces;

      bool  m_applyFoetalToLegs;
      float  m_movementLegsInFoetalPosition;

      bool  m_useVelocityOfObjectBelow;
      bool  m_useRelativeVelocity;

      float  m_maxAngVelAroundFrontwardAxis;
      bool  m_applyNewRollingCheatingTorques;
      float m_magOfTorqueToRoll;
      float m_maxAngVel;
      float m_minAngVel;

      bool  m_applyHelPerTorqueToAlign;
      float m_delayToAlignBody;
      float m_magOfTorqueToAlign;
      float m_airborneReduction;
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

    //Fall2Knees
    float m_ftk_SpineBend;
    bool m_fall2Knees;
    bool m_ftk_StiffSpine;
    bool m_ftk_armsIn;
    bool m_ftk_armsOut;

  protected:

    void initialiseCustomVariables();
    bool avoidSpinAroundFrontalAxis();
    void applyAlignBodyOrthogonally(float magOfTorque);
    void applyFoetalToLegs();

    rage::Vector3         m_forwardVelVec,
      m_accel;

    float                 m_decayTime;
    float                 m_currentSlope;

    bool m_alreadyCollided;
  };
}

#endif 


