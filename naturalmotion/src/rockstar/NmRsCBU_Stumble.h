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

#ifndef NM_RS_CBU_STUMBLE_H 
#define NM_RS_CBU_STUMBLE_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMStumbleFeedbackName      "stumble" 
#define NMHandAndKneesAngryCat 0  // use angry-cat back pose when on handsAndKnees

  class NmRsCBUStumble : public CBUTaskBase
  {
  public:

    NmRsCBUStumble(ART::MemoryManager* services);
    ~NmRsCBUStumble();

    struct Parameters;
    void onActivate();
    void onDeactivate();

		CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

    bool isOnGround() { return m_onGround; }

    struct Parameters
    {
      float m_legsStiffness;
      float m_torsoStiffness;
      float m_armsStiffness;
      float armReduceSpeed;
      unsigned int fallMask;
      float m_forwardMaxArmOffset;
      float m_backwardsMinArmOffset;
      float m_zAxisSpinReduction;
      bool  m_useHeadLook;

      float wristMS;
      float feetMS;
      float armTwist;
      float staggerTime;
      float dropVal;
      int  twistSpine;

      bool dampPelvis;
      bool pitchInContact;
      bool  different;
      float leanRate;
      float maxLeanBack;
      float maxLeanForward;
      float grabRadius2;
      float leanTowards;
      bool useArmsBrace;

    } m_parameters;

    //MMMMHandsKnees possible parameter float m_floorTimeValue;
    //MMMMHandsKnees possible parameter float m_restartTime;

  protected:

		enum stumbleState
		{
			sbleS_Normal,           //0
			sbleS_ToGrab,           //1
			sbleS_Falling,          //2
			sbleS_onFloor,          //3
			sbleS_onKnees,          //4 
			sbleS_onAllFours,       //5 
			sbleS_Collapse,         //6
			sbleS_CollapseBackwards,//7
			sbleS_End,              //8
		}
		m_stumbleState;

		enum stumbleTransition
		{
			sbleT_fall2Knees,           //0
			sbleT_fall2AllFours,          //1 
			sbleT_knees2AllFours,       //2 
			sbleT_allFours2Knees,         //3
			sbleT_Knees2Collapse,              //4
			sbleT_allFours2Collapse,    //5
			sbleT_fallBackWards,        //6
		}
		m_stumbleTransition;
    
		enum stumbleGrab
		{
			sg_noGrab,           //0
			sg_attemptGrab,          //1 
			sg_grabbing,          //1 
			sg_letGo,          //1 
		}
		m_stumbleGrab;

		void initialiseCustomVariables();
	  float getAnkleAngle(
		  rage::Vector3 &kneePos, 
		  rage::Vector3 &footPos, 
		  rage::Vector3 &sideways, 
		  const rage::Vector3 &upVector);
    void armsBrace();

    class ArmState 
    {
    public:
      void init(NmRsCharacter *character, NmRsCBUStumble *parent)
      {
        m_character = character;
        m_parent = parent;
        m_floorVel.Set(0,0,0);
        m_liftArm = false;
        m_liftArmTime = -0.001f;
      }

      void enter(NmRsHumanArm *armSetup, bool leftArm, BehaviourMask armMask);
      bool tick(float timeStep);
      void armIK(NmRsIKInputWrapper *input, const rage::Vector3 &target, float armTwist, float dragReduction, const rage::Vector3 *vel = NULL);
      void wristIK(NmRsIKInputWrapper *input, const rage::Vector3 &target, const rage::Vector3 &normal);

      rage::Vector3 m_floorVel;

      NmRsCharacter *m_character;
      NmRsCBUStumble *m_parent;

      NmRsHumanArm *m_armSetup;

      float m_strength;
      float m_onBackRandomL1;
      float m_onBackRandomL2;
      float m_maxElbowAngleRandom;
      float m_liftArmTime;
      BehaviourMask m_armMask;

      bool m_isLeftArm;
      bool m_liftArm;

    } m_leftArmState, m_rightArmState;

    rage::Vector3 m_fallDirection;

    const float m_reachLength;
    float m_probeLength;
    float m_forwardsAmount;
    float m_bodyStrength;
    const float m_predictionTime;
    float m_behaviourTime;
    float m_injuryVal;

    float m_upwardsness;

    float m_kneeBendL;
    float m_kneeBendR;
    float m_randomSpineL2;
    float m_floorTime;//MMMMHandsKnees
    float m_restart;//MMMMHandsKnees
    float m_OnFloorTimer;
    float m_2Kneestimer;
    float m_hipPitchAim;

    bool m_headAvoidActive;
    bool m_onGround;
    bool m_allFours;

  private:

    NmRsCBUStumble(const NmRsCBUStumble& from);
    NmRsCBUStumble& operator=(const NmRsCBUStumble& from);
  };
}

#endif // NM_RS_CBU_STUMBLE_H

