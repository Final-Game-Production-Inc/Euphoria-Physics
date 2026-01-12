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

#ifndef NM_RS_CBU_CATCHFALL_H 
#define NM_RS_CBU_CATCHFALL_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMCatchFallFeedbackName "catchFall" 
#define NMHandAndKneesAngryCat 0  // use angry-cat back pose when on handsAndKnees

  class NmRsCBUCatchFall : public CBUTaskBase
  {
  public:
    NmRsCBUCatchFall(ART::MemoryManager* services);
    ~NmRsCBUCatchFall();

    struct Parameters;
    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);
    bool isOnGround() { return m_onGround; }
    float getLeftArmLength() const;
    // Function sets m_reachLengthMultiplier and updates reach length and the probe length.
    // NOTE: The following function is also called by setFallingReaction message.
    void applyReachLengthMultiplier(float reachLengthMultiplier);
    void updateReachLength(bool hasProbeHit);

    struct Parameters
    {
      float m_legsStiffness;
      float m_torsoStiffness;
      float m_armsStiffness;
      float m_forwardMaxArmOffset;
      float m_backwardsMinArmOffset;
      float m_zAxisSpinReduction;
      float extraSit;
      bool  m_useHeadLook;
      BehaviourMask m_effectorMask;
    } m_parameters;

    BehaviourMask m_excludeMask;//shot reachForWound can set this to be left arm and or rightArm

    // These params are set by setFallingReaction. 
    //They are only set to defaults in the constructor so that setFallingReaction values persist
    //mmmmnote mmmmtodo stumble, teeter and yanked set them so break the model above.
    float m_armReduceSpeed;
    float m_comVelRDSThresh;
    //float m_reachLengthMultiplier; Moved to private.
    float m_inhibitRollingTime;
    float m_changeFrictionTime;
    float m_groundFriction;
    float m_groundFrictionMin;
    float m_stopManual;
    float m_stoppedStrengthDecay;
    float m_spineLean1Offset;
    bool m_handsAndKnees;
    bool m_hkHeadAvoid;
    bool m_callRDS;
    bool m_resistRolling;
    bool m_stopOnSlopes;
    bool m_riflePose;
    bool m_antiPropClav;
    bool m_antiPropWeak;
    bool m_headAsWeakAsArms;
    float m_successStrength;

    // Set by SmartFall
    bool m_allowFrictionChangeToExtremities;

    //MMMMHandsKnees possible parameter float m_floorTimeValue;
    //MMMMHandsKnees possible parameter float m_restartTime;
    bool m_fall2Knees;
    bool m_ftk_armsIn;
    bool m_ftk_armsOut;

    void setHeadAvoidActive(bool active) { m_headAvoidActive = active; }

  protected:

    void initialiseCustomVariables();

    class ArmState 
    {
    public:
      void init(NmRsCharacter *character, NmRsCBUCatchFall *parent)
      {
        m_character = character;
        m_parent = parent;
        m_floorVel.Set(0,0,0);
        m_lastTarget.Zero();
        m_liftArm = false;
        m_timeToImpact = 10.f;
      }

      void enter(NmRsHumanArm* armSetup, bool isLeftArm, BehaviourMask armMask);

      void tick(float timeStep);
      void armIK(NmRsIKInputWrapper* ikInputData, const rage::Vector3 &target, float armTwist, float dragReduction, const rage::Vector3 *vel = NULL);
      void wristIK(NmRsIKInputWrapper* ikInputData, const rage::Vector3 &wristTarget, const rage::Vector3 &wristNormal);
      void getDesiredWristNormal(const rage::Vector3 &groundNormal, const rage::Vector3 &probeEndHit, bool hasProbeHit, rage::Vector3 &wristNormal) const;


      rage::Vector3 m_floorVel;

      // Last arm IK target, relative to pelvis.
      rage::Vector3 m_lastTarget;

      NmRsCharacter *m_character;
      NmRsCBUCatchFall *m_parent;
      NmRsHumanArm* m_armSetup;

      float m_armStrength;
      float m_onBackRandomL1;
      float m_onBackRandomL2;
      float m_maxElbowAngleRandom;
      float m_liftArmTime;
      float m_timeToImpact;
      BehaviourMask m_armMask;

      bool m_isLeftArm;
      bool m_liftArm;

    } m_leftArmState, m_rightArmState;

    rage::Vector3 m_fallDirection;
    float m_fallVelocityMag;

#if ART_ENABLE_BSPY
    rage::Vector3 m_groundNormalL;
    rage::Vector3 m_wristNormalL;
    rage::Vector3 m_reachTargetL;
    rage::Vector3 m_groundNormalR;
    rage::Vector3 m_wristNormalR;
    rage::Vector3 m_reachTargetR;
#endif // ART_ENABLE_BSPY

    float m_armLength;
    float m_reachLength;
    float m_reachLengthMultiplier; // Set by setFallingReaction.
    float m_probeLength;

    float m_forwardsAmount;
    float m_bodyStrength;
    const float m_predictionTime;
    float m_behaviourTime;

    float m_upwardsness; 
    float m_faceUpness;
    
    float m_slope; // Metric of ground slope 0..1 (flat..~25degrees)
    float m_slopeAlignment; // Body alignment with slope direction 0..1 (not-aligned..aligned)

    float m_kneeBendL;
    float m_kneeBendR;
    float m_randomSpineL2;
    float m_inhibitRollingTimer;
    float m_changeFrictionTimer;
    float m_floorTime;//MMMMHandsKnees
    float m_restart;//MMMMHandsKnees
    float m_OnFloorTimer;
    
    unsigned int m_effectorMask;

    bool m_headAvoidActive;
    bool m_onGround;
    bool m_rdsActivatedByCatchFall;

  private:

    NmRsCBUCatchFall(const NmRsCBUCatchFall& from);
    NmRsCBUCatchFall& operator=(const NmRsCBUCatchFall& from);
  };
}

#endif // NM_RS_CBU_CATCHFALL_H


