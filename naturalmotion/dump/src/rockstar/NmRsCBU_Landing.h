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

#ifndef NM_RS_CBU_LANDING_H 
#define NM_RS_CBU_LANDING_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"
#include "NmRsStateMachine.h"

#define LANDING_AUTOMATIC_WEAPON_DETECTION 1
#define LandingBSpyDraw 1

namespace ART
{
  class NmRsCharacter;

#define NMLandingFeedbackName      "landing" 

  class NmRsCBULanding : public CBUTaskBase, public StateMachine
  {
  public:
    NmRsCBULanding(ART::MemoryManager* services);
    ~NmRsCBULanding();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY
    void updateBehaviourMessage(const MessageParamsBase* const params);

    struct Parameters
    {
      float m_bodyStiffness;
      float m_bodydamping;
      float m_strengthKneeToStandUp;
      float m_catchfalltime;
      float m_crashOrLandCutOff;
      float m_angleToCatchFallCutOff;
      float m_pdStrength;
      float m_armsUp;
      float m_legRadius;
      float m_legAngSpeed;
      float m_aimAngleBase;
      float m_armsFrontward;
      float m_forwardVelRotation;
      float m_legL;
      float m_sideD;
      float m_legStrength;
      bool  m_orientateBodyToFallDirection;
      float m_predictedTimeToOrientateBodytoFallDirection;
      bool  m_ignorWorldCollisions;
      float m_limitNormalFall;
      bool  m_forwardRoll;
      float m_cheatingTorqueToForwardRoll;
      float m_feetBehindCOM;
      float m_feetBehindCOMVel;
      float m_factorToReduceInitialAngularVelocity;
      float m_stopFWCOMRoT;
      float m_stopEndFWCOMRoT;
      float m_standUpCOMBehindFeet;
      float m_standUpRotVel;
      float m_maxAngVelForForwardRoll;
      bool  m_sideRoll;
      float m_maxVelForSideRoll;
    } m_parameters;

  protected:

    void initialiseCustomVariables();

    // StateMachine
    virtual bool States( StateMachineEvent event, int state);
    enum States
    { 
      HF_Falling,
      HF_BailOut,
      HF_PrepareForLanding,
      HF_ForwardRoll,
      HF_CatchFall,
      HF_Balance,
      HF_StandUp,
      HF_BalanceStandUp,
      HF_SideRoll,       
    };

    void decideThings();
    void duringFall();
    void duringBailOut();
    void duringPrepareForLanding();
    void duringPrepareForSideRoll();
    void duringBalance();
    void duringCatchFall();
    void duringForwardRoll();
    void duringSideRoll();
    void duringStandUp();
    void duringBalanceStandUp();
    int  decidePhaseFW();
    float lengthLegsPercentage();
    float lengthLegs();
    bool isVerticalHighFall();
    bool isStayingOnHisHead(int time);
    int  decidePhaseBalance();
    void setFeetposition(float sideOffset,float feetBehindCOM,float lengthLegspercentage,float factorVelocityMovements);//do not use IK to be sure the length of the legs will be OK when landing.
    float lengthLegsPercentage(bool forRightLeg);
    void setHands(float angle);

    rage::Vector3 m_initPosFall;
    rage::Vector3 m_lastPostFall;

    float   m_timer;
    float   m_timeToImpact;
    float   m_forwardVel;
    float   m_rollToBBtimer;
    float   m_PFLtimer;
    float   m_averageComVel;
    float   m_StandUpTimer;
    float    m_isStayingOnHisHead;
    float   m_healthCatchFall;
    float   m_lastKneeStrength;
    float   m_rollDownTimer;
    float   m_HighFall;
    float   m_lastApplied;
    float   m_timeStep ;
    float   m_slopeGround;
    
    int     m_nextFW;
    int     m_nextBalance;
    int     m_waitFW;
    int     m_timerTryToStandUp;
    int     m_lastCOMUp;

    const LeftArmSetup *m_leftArm;
    const RightArmSetup *m_rightArm;
    const LeftLegSetup *m_leftLeg;
    const RightLegSetup *m_rightLeg;
    const SpineSetup *m_spine;

    bool    m_sideToRollisRight;
    bool    m_alreadyCollidedL;
    bool    m_alreadyCollidedR;
    bool    m__spineAlreadyCollided;
    bool    m_phase1FW;
    bool    m_phase2FW;
    bool    m_phase3FW;
    bool    m_phase1DoneFW;
    bool    m_phase2DoneFW;
    bool    m_firstFrame;
    bool    m_lastFrame;
    bool    m_testSideRoll;
    bool    m_sideRollTest;
    bool    m_rightSideRoll;
    bool    m_initCheatingTorque;
    bool    m_ForwardRollFinished;
    bool    alreadyForcedToInitialize;
    bool  m_hasCollidedWithWorld;
    bool  m_controlOrientation;
    bool  m_willLandOk;
    bool  m_messageSent; 
    bool  m_goToCatchFallFlag;
    //bool  m_rollToBB;

  };
}

#endif // NM_RS_CBU_LANDING_H
