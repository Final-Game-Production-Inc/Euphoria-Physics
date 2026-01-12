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

#ifndef NM_RS_CBU_SHOT_H 
#define NM_RS_CBU_SHOT_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMShotFeedbackName      "shot" 
#define NMShotRFWFeedbackName      "shotRFW" 
#define NMShotFallToKneesFeedbackName      "shotFallToKnees"
#define NMShotImpulseDebug
#define sfbRecoveryPeriod 0.3f

  class NmRsCBUShot : public CBUTaskBase
  {
  public:

    NmRsCBUShot(ART::MemoryManager* services);
    ~NmRsCBUShot();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

    void setRelaxPeriodUpper(float rp) { m_relaxPeriodUpper = rp; }
    void setRelaxPeriodLower(float rp) { m_relaxPeriodLower = rp; }
    void setHeadLookToggleTimer(float t) { m_headLook.toggleTimer = t; };

    void setCleverIKStrengthMultLeft(float cleverIKStrengthMult) {m_cleverIKStrengthMultLeft = cleverIKStrengthMult; };
    void setCleverIKStrengthMultRight(float cleverIKStrengthMult) {m_cleverIKStrengthMultRight = cleverIKStrengthMult; };

    //So that the backArchedBack and leanedTooFarBack balancer fails 
    // are inhibited when shotFromBehind is enabled and for sfbRecoveryPeriod after it has finished.
    bool ignoreSFBBackArchedFailure() { return (m_shotFromBehind.timer < m_parameters.sfbPeriod + sfbRecoveryPeriod && (m_parameters.sfbIgnoreFail == 1 || m_parameters.sfbIgnoreFail == 2)); }
    bool ignoreSFBLeanedBackFailure() { return (m_shotFromBehind.timer < m_parameters.sfbPeriod + sfbRecoveryPeriod && (m_parameters.sfbIgnoreFail == 1 || m_parameters.sfbIgnoreFail == 3)); }
    bool getFallToKneesIsRunning() { return m_fallToKneesEnabled; }
    bool getFallToKneesIsFalling() { return (m_fallToKneesEnabled && m_fTK.m_bendLegs); }
    bool getFling2LeftActive() { return (m_fling2Enabled && m_fling2Left); }
    bool getFling2RightActive() { return (m_fling2Enabled && m_fling2Right); }
    const float getStiffnessStrengthScale() { return m_controlStiffnessEnabled ? m_controlStiffnessStrengthScale : 1.f; }
    const float getArmsDamping() { return m_armsDamping; }
    const float getSpineDamping() { return m_spineDamping; }
    const float getNeckStiffness() { return m_neckStiffness; }
    const float getNeckDamping() { return m_neckDamping; }
    const float getWristStiffness() { return m_wristStiffness; }
    const float getWristDamping() { return m_wristDamping; }
    const float getUpperBodyStiffness() { return m_upperBodyStiffness; }
    const float getLowerBodyStiffnesss() { return m_lowerBodyStiffness; }
    const bool getInjuredLArm() { return m_injuredLArm; }
    const bool getInjuredRArm() { return m_injuredRArm; }
    const bool getInjuredLLeg() { return m_injuredLLeg; }
    const bool getInjuredRLeg() { return m_injuredRLeg; }

    struct Parameters
    {
      rage::Vector3 normal;
      rage::Vector3 headLookPos;
      rage::Vector3 hitPoint;
      rage::Vector3 bulletVel;
      float angVelScale;
      BehaviourMask  angVelScaleMask;
      float flingWidth;
      float flingTime;
      //reachForWound
      float grabHoldTime;
      float timeBeforeReachForWound;
      //defaultArms add default arms.  if none set default?
      bool brace; 
      bool useArmsWindmill; 
      int releaseWound;
      int reachFalling;
      int reachFallingWithOneHand;
      int reachOnFloor;


      //BulletExaggeration/reflex - just affects spine at the mo
      float exagDuration;
      float exagMag;
      float exagTwistMag;
      float exagSmooth2Zero;
      float exagZeroTime;
      //Conscious pain - just affects spine at the mo
      float cpainSmooth2Time;
      float cpainDuration;
      float cpainMag;
      float cpainTwistMag;
      float cpainSmooth2Zero;

      float bodyStiffness;
      float armStiffness;
      float spineDamping;
      float initialNeckStiffness;
      float initialNeckDamping;
      float neckStiffness;
      float neckDamping;
      float kMultOnLoose;
      float kMult4Legs;
      float loosenessAmount;
      float looseness4Fall;
      float looseness4Stagger;
      float alwaysReachTime;
      float headLookAtWoundMinTimer;
      float headLookAtWoundMaxTimer;
      float headLookAtHeadPosMaxTimer;
      float headLookAtHeadPosMinTimer;
      int   bodyPart;
      float shockSpinMin;
      float shockSpinMax;
      float shockSpinLiftForceMult;
      float shockSpinDecayMult;
      float shockSpinScalePerComponent;
      float shockSpinMaxTwistVel;
      float shockSpinAirMult;
      float shockSpin1FootMult;
      float shockSpinFootGripMult;
      bool shockSpinScaleByLeverArm;
	  float bracedSideSpinMult;
      //legInjury
      bool allowInjuredLeg;
      bool allowInjuredThighReach;
      bool allowInjuredLowerLegReach;
      float timeBeforeCollapseWoundLeg;
      float legLimpBend;
      float legInjuryTime;
      float legLiftTime;
      float legInjury;
      float legInjuryLiftHipPitch;
      float legInjuryHipPitch;
      float legInjuryLiftSpineBend;
      float legInjurySpineBend;
      bool legForceStep;
      //armInjury
      bool allowInjuredArm;

      float ftkBalanceTime;
      float ftkHelperForce;
      float ftkLeanHelp;
      float ftkSpineBend;
      float ftkImpactLooseness;
      float ftkImpactLoosenessTime;
      float ftkBendRate;
      float ftkHipBlend;
      float ftkLungeProb;
      bool ftkKneeSpin;
      float ftkFricMult;
      float ftkHipAngleFall;
      float ftkPitchForwards;
      float ftkPitchBackwards;
      float ftkFallBelowStab;
      float ftkBalanceAbortThreshold;
      bool ftkFailMustCollide;
      int ftkOnKneesArmType;
      float ftkReleaseReachForWound;
      bool ftkReachForWound;
      bool ftkReleasePointGun;
      bool ftkHelperForceOnSpine;
      bool ftkAlwaysChangeFall;
      bool ftkStiffSpine;
      float sfbSpineAmount;
      float sfbNeckAmount;
      float sfbHipAmount;
      float sfbKneeAmount;
      float sfbPeriod;
      float sfbForceBalancePeriod;
      float sfbArmsOnset;
      float sfbKneesOnset;
      float sfbNoiseGain;
      int sfbIgnoreFail;
      float sigSpineAmount;
      float sigNeckAmount;
      float sigHipAmount;
      float sigKneeAmount;
      float sigPeriod;
      float sigForceBalancePeriod;
      float sigKneesOnset;
      bool melee;
      bool fling;
      bool crouching;
      bool useHeadLook;
      bool pointGun;
      float AWSpeedMult;
      float AWRadiusMult;
      float AWStiffnessAdd;
      int reachWithOneHand;
      bool allowLeftPistolRFW;
      bool allowRightPistolRFW;
      bool rfwWithPistol;

      bool shotFromBehind;
      bool shotInGuts;
      bool addShockSpin;
      bool randomizeShockSpinDirection;
      bool alwaysAddShockSpin;
      bool fallToKnees;
      bool chickenArms;
      bool reachForWound;
      int fallingReaction;
      bool useExtendedCatchFall;
      bool stableHandsAndNeck;
      bool localHitPointInfo;
      float initialWeaknessRampDuration;
      float initialWeaknessZeroDuration;
      float initialNeckRampDuration;
      float initialNeckDuration;

      // scale body effector strengths with characterStrength
      bool useCStrModulation;
      float cStrUpperMin;
      float cStrUpperMax;
      float cStrLowerMin;
      float cStrLowerMax;
      bool snap;
      float snapMag;
      float snapMovingMult;
      float snapBalancingMult;
      float snapAirborneMult;
      float snapMovingThresh;
      float snapDirectionRandomness;
      int snapHipType;
      bool snapLeftLeg;
      bool snapRightLeg;
      bool snapLeftArm;
      bool snapRightArm;
      bool snapSpine;
      bool snapNeck;
      bool snapPhasedLegs;
      bool snapUseBulletDir;
      bool snapHitPart;
      float unSnapInterval;
      float unSnapRatio;
      bool snapUseTorques;
      float spineBlendZero;
      float minArmsLooseness;
      float minLegsLooseness;
      bool bulletProofVest;
      bool alwaysResetLooseness;
      bool alwaysResetNeckLooseness;
      bool spineBlendExagCPain;

      bool fling2;
      bool fling2Left;
      bool fling2Right;
      bool fling2OverrideStagger;
      float fling2TimeBefore;
      float fling2Time;
      float fling2MStiffL;
      float fling2MStiffR;
      float fling2RelaxTimeL;
      float fling2RelaxTimeR;
      float fling2AngleMinL;
      float fling2AngleMaxL;
      float fling2AngleMinR;
      float fling2AngleMaxR;
      float fling2LengthMinL;
      float fling2LengthMaxL;
      float fling2LengthMinR;
      float fling2LengthMaxR;

      float bustElbowLift;
      bool bust;
      float cupSize;
      bool cupBust;

      float deathTime;

    } m_parameters;
    enum ftkArms
    {
      ftk_useFallArms,
      ftk_armsIn,
      ftk_armsOut
    };

    // holds everything related injuredArm reaction
    // as parameter struct is becoming full, break this out here
    // enabled via separate one-shot message: configureShotInjuredArm
    struct injuredArm
    {
      // "parameters"
      float 
        hipYaw,//default=1.5
        hipRoll,//default=0.0
        forceStepExtraHeight,//default = 0.07
        //shrugTime,//default = 0.4 possible reflex, exaggerate parameter - controls timing of shrugging by clavicles
        velMultiplierStart,
        velMultiplierEnd,
        velForceStep,
        velStepTurn,
        injuredArmTime;

      bool
        forceStep,
        stepTurn,
        velScales;

      // variables
      bool 
        m_previousUseCustomTurnDir;
      rage::Vector3 
        m_turnDir,
        m_previousCustomTurnDir; 

    } m_injuredArm;


    // [jrp] to debug use of addImpulse/addForceToPart while shot is running
    rage::Vector3 m_lastForce;

    void newHit(NmRsHumanBody& body);

  protected:

    void initialiseCustomVariables();
    
    void substateTick(bool (NmRsCBUShot::*entryCondition)(), 
      void (NmRsCBUShot::*entry)(), 
      void (NmRsCBUShot::*tick)(float step),
      bool (NmRsCBUShot::*exitCondition)(), 
      void (NmRsCBUShot::*exit)(), bool& enabled, float timeStep);

    bool crouchShot_entryCondition();
    void crouchShot_tick(float timeStep);
    bool crouchShot_exitCondition();
    void crouchShot_exit();

    bool fallToKnees_entryCondition();
    void fallToKnees_entry();
    void fallToKnees_tick(float timeStep);
    bool fallToKnees_exitCondition();
    void fallToKnees_exit();
    void fallToKnees_startFall();

    bool injuredLeftArm_entryCondition();
    void injuredLeftArm_entry();
    void injuredArm_tick(float timeStep); // general and use by both left and right arms
    bool injuredArm_exitCondition();
    void injuredArm_exit();
    bool injuredRightArm_entryCondition();
    void injuredRightArm_entry();

    bool injuredLeftLeg_entryCondition();
    bool injuredRightLeg_entryCondition();
    void injuredLeftLeg_entry();
    void injuredRightLeg_entry();
    void injuredLeg_tick(float timeStep); // general and use by both left and right Legs
    bool injuredLeftLeg_exitCondition(); 
    bool injuredRightLeg_exitCondition(); 
    void injuredLeg_exit();
    void injuredLeftLeg_exit(); 
    void injuredRightLeg_exit(); 

    bool defaultArmMotion_entryCondition();
    void defaultArmMotion_entry();
    void defaultArmMotion_tick(float timeStep);
    bool defaultArmMotion_exitCondition();
    void defaultArmMotion_exit();
    void defaultArmMotion_armsBrace();
    void defaultArmMotion_deDefaultMotion();

    bool reachLeft_entryCondition();
    void reachLeft_entry();
    bool reachLeft_exitCondition();
    void reachLeft_exit();

    void reachArm_tick(float timeStep);

    bool reachRight_entryCondition();
    void reachRight_entry();
    bool reachRight_exitCondition();
    void reachRight_exit();

    bool controlStiffness_entryCondition();
    void controlStiffness_entry();
    // Version that accepts body reference is necessary to allow newHit (which
    // calls controlStiffness_tick) to be called outside of the normal task
    // tick sequence (before m_body has been correctly set up for this task).
    void controlStiffness_tick(float timeStep, NmRsHumanBody& body);
    void controlStiffness_tick(float timeStep);
    bool controlStiffness_exitCondition();
    void controlStiffness_exit();

    bool headLook_entryCondition();
    void headLook_entry();
    void headLook_tick(float timeStep);
    bool headLook_exitCondition();
    void headLook_exit();

    bool justWhenFallen_entryCondition();
    void justWhenFallen_entry();
    void justWhenFallen_tick(float timeStep);
    bool justWhenFallen_exitCondition();
    void justWhenFallen_exit();

    void chickenArms_applyDefaultPose();
    bool chickenArms_entryCondition();
    void chickenArms_entry();
    void chickenArms_tick(float timeStep);
    bool chickenArms_exitCondition();

    void shotFromBehind_applyDefaultPose();
    bool shotFromBehind_entryCondition();
    void shotFromBehind_entry();
    void shotFromBehind_tick(float timeStep);
    bool shotFromBehind_exitCondition();
    void shotFromBehind_exit();

    bool shotInGuts_entryCondition();
    void shotInGuts_entry();
    void shotInGuts_tick(float timeStep);
    bool shotInGuts_exitCondition();
    void shotInGuts_exit();

    bool fling_entryCondition();
    void fling_entry();
    void fling_tick(float timeStep);
    bool fling_exitCondition();
    void fling_exit();

    bool fling2_entryCondition();
    void fling2_entry();
    void fling2_tick(float timeStep);
    bool fling2_exitCondition();
    void fling2_exit();

    bool melee_entryCondition();
    void melee_entry();
    void melee_tick(float timeStep);
    bool melee_exitCondition();
    void melee_exit();

    bool onGround_entryCondition();
    void onGround_entry();
    void onGround_tick(float timeStep);
    bool onGround_exitCondition();
    void onGround_exit();

    bool pointGun_entryCondition();
    void pointGun_entry();
    void pointGun_tick(float timeStep);
    bool pointGun_exitCondition();
    void pointGun_exit();

    struct HeadLook
    {
      float toggleTimer;
      enum State
      {
        hlAtVel,
        hlFwd
      } state;
      HeadLook() : state(hlAtVel){}; // ensure correct initial state
    } m_headLook;

    struct DefaultArmMotion
    {
      float timer1;
      float timer2;
      float armsWindmillTimeLeft;
      float armsWindmillTimeRight;
      bool  leftArmsWindmill;
      bool  rightArmsWindmill;
      bool  leftBrace;
      bool  rightBrace;
      bool  leftDefault;
      bool  rightDefault;
      bool  releaseLeftWound;
      bool  releaseRightWound;
    } m_defaultArmMotion;

    struct ArmProperty
    {
      // phase2 todo figure out what the intention is here. clearly this doesn't need
      // to be a struct...
      NmRsHumanArm *arm;

    } m_armProperty;

    struct Melee
    {
      float motionMultiplier;
      float neckTilt;
      float LESwingMin;
      float LESwingMax;
      float RESwingMin;
      float RESwingMax;

      float hipYaw;
      float headYaw;
      float hipangle;
      float bodyangle;
      float headangle;

      float headLookyTimer;
    } m_melee;

    struct Fling
    {
      float flingTimer;
      float leftDir;
      float rightDir;
      float backLeanDir;
      float period;
      int bodyRegion;
      float sAngle;//only used for spine reaction
      float fAngle;//only used for spine reaction
      bool useLeft;
      bool useRight;
    } m_fling;

    struct Fling2
    {
      float flingTimer;
      float relaxTimeL;
      float relaxTimeR;
      float lengthL;
      float lengthR;
      float angleL;
      float angleR;
      float period;
      bool hasFlungL;
      bool hasFlungR;
    } m_fling2;

    struct ShotFromBehind
    {
      float timer;
      float sfbRandTimer;
      float sfbNoiseSeed;
    } m_shotFromBehind;

    struct ShotInGuts
    {
      float timer;
    } m_shotInGuts;

    struct InjuredLeg
    {
      float legLiftTimer;
      float legInjuryTimer;
      float forceStepTimer;
    }; 

    struct FallToKnees
    {
      float m_hipMoventBackwards;
      float m_ftkStuckTimer;
      float m_ftkLoosenessTimer;
      bool m_LkneeHitLooseness;
      bool m_RkneeHitLooseness;
      bool m_bendLegs;
      bool m_LkneeHasHit;
      bool m_RkneeHasHit;
      bool m_fallingBack;
      bool m_squatting;
      bool m_doLunge;
    } m_fTK;

    ReachArm m_reachLeft, m_reachRight;
    ReachArm* m_reachArm;

    InjuredLeg m_injuredRightLeg;
    InjuredLeg m_injuredLeftLeg;


    rage::Vector3 m_hitPointWorld;
    rage::Vector3 m_hitPointLocal;
    rage::Vector3 m_hitNormalLocal;
    rage::Vector3 m_woundLOffset;
    rage::Vector3 m_woundROffset;
    rage::Vector3 m_woundLNormal;
    rage::Vector3 m_woundRNormal;
    rage::Vector3 m_turnTo;
    rage::Vector3 m_leftShoulderAngles;//for fling2
    rage::Vector3 m_rightShoulderAngles;//for fling2

    float m_hitTime;//used for snap, cPain, exag, fling, chicken, melee, injuredLimb
    float m_controlStiffnessTime;
    float m_controlNeckStiffnessTime;//controls the neck stiffness and damping - at the moment m_controlStiffnessTime still controls the neck muscleStiffness
    float m_hitTimeLeft;//used for reachForWound, headlook
    float m_hitTimeRight;//used for reachForWound, headlook
    float m_defaultBodyStiffness;
    float m_spineStiffness;
    float m_spineDamping;
    float m_armsStiffness;
    float m_armsDamping;
    float m_neckStiffness;
    float m_neckDamping;
    float m_wristStiffness;
    float m_wristDamping;
    float m_upperBodyStiffness;
    float m_lowerBodyStiffness;
    float m_torqueSpin;
    float m_torqueSpinTime;

    // previously _g.
    float m_spineLean1;
    float m_relaxTimeUpper;
    float m_relaxPeriodUpper;
    float m_relaxTimeLower;
    float m_relaxPeriodLower;
    float m_velForwards;
    float m_hitPointRight;
    float m_time;//total time spent in shot behaviour
    float m_armTwist;
    float m_controlStiffnessStrengthScale;
    float m_controlNeckStiffnessScale;
    float m_chickenArmsTimeSpaz;
    float m_injuredArmElbowBend;
    float m_hipYaw;
    float m_hipRoll;
    float m_twistMultiplier;
    float m_exagLean1;
    float m_exagLean2;
    float m_exagTwist;
    float m_snapDirection;

    float m_cleverIKStrengthMultLeft;
    float m_cleverIKStrengthMultRight;

    int m_woundLPart;
    int m_woundRPart;

    BehaviourMask m_injuredLegMask;

    bool m_newReachL;
    bool m_newReachR;
    bool m_injuredLArm;

    bool m_injuredRArm;
    bool m_injuredLLeg;
    bool m_injuredRLeg;
    bool m_falling;
    bool m_reachedForFallen;

    bool m_archBack;
    bool m_headLookAtWound;


    bool m_injuredLeftArmEnabled;
    bool m_injuredRightArmEnabled;

    bool m_injuredLeftLegEnabled;
    bool m_injuredRightLegEnabled;
    bool m_defaultArmMotionEnabled;
    bool m_newHitEnabled;

    bool m_reachLeftEnabled;
    bool m_reachRightEnabled;
    bool m_controlStiffnessEnabled;
    bool m_headLookEnabled;

    bool m_justWhenFallenEnabled;
    bool m_chickenArmsEnabled;
    bool m_flingEnabled;
    bool m_fling2Enabled;
    bool m_meleeEnabled;
    bool m_crouchShotEnabled;
    bool m_fallToKneesEnabled;
    bool m_shotFromBehindEnabled;
    bool m_shotInGutsEnabled;
    bool m_onGroundEnabled;
    bool m_pointGunEnabled;

    bool m_hitFromBehind;
    bool m_newHit;
    bool m_disableBalance;//used to be a parameter - but not set by dInvoke
    //bool m_shrug;//possible reflex, exaggerate parameter - controls shrugging by clavicles

    bool m_feedbackSent_FinishedLookingAtWound;

    bool m_shotFromTheFront;
    bool m_bcrArms;
    bool m_fling2Left;
    bool m_fling2Right;

  };
}

#endif // NM_RS_CBU_SHOT_H



