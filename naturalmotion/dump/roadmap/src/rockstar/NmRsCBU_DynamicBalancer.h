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

#ifndef NM_RS_CBU_DYNBAL_H
#define NM_RS_CBU_DYNBAL_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsCBU_DBMCommon.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

  class NmRsCBUDynamicBalancer : public CBUTaskBase
  {
  public:
    NmRsCBUDynamicBalancer(ART::MemoryManager* services);
    ~NmRsCBUDynamicBalancer();

    void onActivate();
    void onDeactivate();
    void requestDeactivate();
    CBUTaskReturn onTick(float timeStep);

    inline bool hasFailed() const { return !isActive() && m_failed; }
    inline int getNumSteps() const { return m_footState.m_numOfSteps; }
    //so staggerFall can force a collapse after time/steps
    inline float getMaximumBalanceTime() const { return m_balanceTimeAtRampDownStart; }
    inline float getTimer() const { return m_timer; }
    inline float getLeftLegStiffness() const { return m_leftLegStiffness; }
    inline float getRightLegStiffness() const { return m_rightLegStiffness; }
    inline rage::Vector3 getCustomTurnDir() { return m_customTurnDir; }
    inline rage::Vector3 getLeanHipUpVector() { return m_roPacket.m_leanHipgUp; }
    inline bool getUseCustomTurnDir() { return m_useCustomTurnDir; }
    inline bool getBalanceIndefinitely() { return m_balanceIndefinitely; }

    // functions to configure the balancer
    inline void setForceStep(int forceStep, float extraHeight, bool initialize) 
    { 
      m_footState.m_forceStep = forceStep; 
      m_footState.m_forceStepExtraHeight = extraHeight;

      if (initialize && (forceStep != 0))
      {
        m_footState.state.m_leftGround = false; 
        m_footState.m_footChoice = NmRsCBUDynBal_FootState::kNotStepping;
        m_footState.m_numOfStepsAtForceStep = m_footState.m_numOfSteps;
      }
    }
    inline void setLegStraightnessModifier(float mod) { m_legStraightnessModifier = mod; }
    inline float getLegStraightnessModifier() { return m_legStraightnessModifier; }
    inline void setGiveUpThreshold(float thresh) { m_roPacket.m_giveUpThreshold = thresh; }
    inline void setMovingFloor(bool movingFloor) { m_roPacket.m_movingFloor = movingFloor; }
    inline void setFlatterSwingFeet(bool flatterSwingFeet) { m_roPacket.m_flatterSwingFeet = flatterSwingFeet; }
    inline void setFlatterStaticFeet(bool flatterStaticFeet) { m_roPacket.m_flatterStaticFeet = flatterStaticFeet; }
    inline void setLeanAgainstVelocity(float leanAgainstVelocity) { m_leanAgainstVelocity = leanAgainstVelocity; }
    inline void setTeeter(bool teeter) { m_roPacket.m_teeter = teeter; }
    inline void setFallToKnees(bool fallToKnees) { m_roPacket.m_fallToKnees = fallToKnees; }
    inline void setGiveUpHeight(float height) { m_giveUpHeight = height; }
    inline void useCustomTurnDir(bool enabled, rage::Vector3 &turnDir) { m_useCustomTurnDir = enabled; m_customTurnDir = turnDir; }
    inline void setHipPitch(float hipPitch) { m_hipPitch = rage::Clamp(hipPitch, -1.5f,1.5f); }
    inline void setHipRoll(float hipRoll) { m_hipRoll = hipRoll; }
    inline void setHipYaw(float hipYaw) { m_hipYaw = hipYaw; }
    inline float getHipPitch() { return m_hipPitch; }
    inline float getHipRoll() { return m_hipRoll; }
    inline float getHipYaw() { return m_hipYaw; }
    inline void setAnkleEquilibrium(float eql) { m_roPacket.m_ankleEquilibrium = eql; }
    inline void setExtraFeetApart(float extraFeetApart) { m_roPacket.m_extraFeetApart = extraFeetApart;}
    inline void setMaxSteps(int maxSteps) { m_footState.m_maxSteps = maxSteps; }
    inline void setFallType(int fallType) { m_fallType = fallType; }
    inline void setFallMult(float fallMult) { m_fallMult = fallMult; }
    inline void setFallReduceGravityComp(bool fallReduceGravityComp) { m_fallReduceGravityComp = fallReduceGravityComp; }
    inline bool getFallReduceGravityComp() { return m_fallReduceGravityComp; }
    inline float getFallMult() { return m_fallMult; }
    inline void setStepHeight(float stepHeight) { m_stepHeight = stepHeight; }
#if NM_STEP_UP
    inline void setStepHeightInc4Step(float stepHeightInc4Step) { m_stepHeightInc4Step = stepHeightInc4Step; }
#endif //#if NM_STEP_UP      
    inline void setLegsApartRestep(float legsApartRestep) { m_roPacket.m_legsApartRestep = legsApartRestep; }
    inline void setLegsTogetherRestep(float legsTogetherRestep) { m_roPacket.m_legsTogetherRestep = legsTogetherRestep; }      
    inline void setLegsApartMax(float legsApartMax) { m_roPacket.m_legsApartMax = legsApartMax; }      
    inline void setKneeStrength(float kneeStrength) { m_kneeStrength = kneeStrength; }
    inline void setStableSuccessMinimumRotSpeed(float stable) { m_stableSuccessMinimumRotSpeed = stable; }
    inline void setStableSuccessMinimumLinSpeed(float stable) { m_stableSuccessMinimumLinSpeed = stable; }
    inline void setMinKneeAngle(float minKneeAngle) { m_minKneeAngle = minKneeAngle; }
    inline void taperKneeStrength(bool taper) { m_taperKneeStrength = taper; }
    inline bool getTaperKneeStrength() { return m_taperKneeStrength; }
    inline void setLeftLegStiffness(float stiffness){ m_leftLegStiffness = stiffness; }
    inline void setRightLegStiffness(float stiffness){ m_rightLegStiffness = stiffness; }
    inline void setLeftLegSwingDamping(float damping){ m_leftLegSwingDamping = damping; }
    inline void setRightLegSwingDamping(float damping){ m_rightLegSwingDamping = damping; }
    inline float getLeftLegSwingDamping(){ return m_leftLegSwingDamping;}
    inline float getRightLegSwingDamping(){ return m_rightLegSwingDamping; }
    inline void setOpposeGravityLegs(float opposeGravityLegs){ m_opposeGravityLegs = opposeGravityLegs; }
    inline void setOpposeGravityAnkles(float opposeGravityAnkles){ m_opposeGravityAnkles = opposeGravityAnkles; }

    inline void setLeanAcc(float leanAcc){ m_leanAcc = leanAcc; }
    inline void setHipLeanAcc(float hipLeanAcc){ m_hipLeanAcc = hipLeanAcc; }
    inline void setLeanAccMax(float leanAccMax){ m_leanAccMax = leanAccMax; }
    inline void setResistAcc(float resistAcc){ m_resistAcc = resistAcc; }
    inline void setResistAccMax(float resistAccMax){ m_resistAccMax = resistAccMax; }
    inline void setFootSlipCompOnMovingFloor(bool footSlipCompOnMovingFloor){ m_footSlipCompOnMovingFloor = footSlipCompOnMovingFloor; }

    inline void setStepDecisionThreshold(float threshold){ m_roPacket.m_stepDecisionThreshold = threshold; }
    inline void setStepClampScale(float scale){ m_stepClampScale = scale; m_stepClampScaleIn = scale; }
    inline void setStepClampScaleVariance(float variance){ m_stepClampScaleVariance = variance; }
    inline void setMaximumBalanceTime(float tme){ m_maximumBalanceTime = tme; }
    inline void setBalanceIndefinitely(bool balanceIndefinitely){ m_balanceIndefinitely = balanceIndefinitely; }
    inline void setBalanceTimeHip(float time){ m_roPacket.m_balanceTimeHip = time;}
    inline void setBalanceTime(float time){ if (time > m_balanceTimeIn+0.0000000001f || time < m_balanceTimeIn-0.0000000001f) m_roPacket.m_balanceTime = time; m_balanceTimeIn = time;}
    inline void setBalanceTimeVariance(float variance){ m_balanceTimeVariance = variance; }
    inline void setStagger(bool stagger){ m_stagger = stagger; }
    inline void setPlantLeg(bool plantLeg){ m_plantLeg = plantLeg; }
    inline void setAirborneStep(bool airborneStep){ m_airborneStep = airborneStep; }    
    inline void setRampHipPitchOnFail(bool rampHipPitchOnFail){ m_rampHipPitchOnFail = rampHipPitchOnFail; }    
    inline void forceFail(){ m_failed = true; m_failType = balFail_Forced; deactivate(); }
    inline void forceFailOnly(){ m_failed = true; m_failType = balFail_Forced;}
    inline void setForceBalance(bool forceBalance){ m_footState.m_forceBalance = forceBalance; }
    inline void setDontChangeStep(bool dontChangeStep){ m_footState.m_dontChangeStep = dontChangeStep; }
    inline void setStepWithBoth(bool stepWithBoth){ m_footState.m_stepWithBoth = stepWithBoth; }
    inline void setFailMustCollide(bool failMustCollide){ m_failMustCollide = failMustCollide; }
    inline void setIgnoreFailure(bool ignoreFailure){ m_ignoreFailure = ignoreFailure; }
    inline bool getIgnoreFailure(){ return m_ignoreFailure; }
    inline bool getIgnoringFailure(){ return m_ignoringFailure; }
    inline void setCrouching(bool crouching){ m_crouching = crouching; }
    inline void setChangeStepTime(float changeStepTime) { m_roPacket.m_changeStepTime = changeStepTime; }
    inline void setUseComDirTurnVelThresh(float useComDirTurnVelThresh) { m_roPacket.m_useComDirTurnVelThresh = useComDirTurnVelThresh; }
    inline void setStepIfInSupport(bool stepIfInSupport){ m_footState.m_stepIfInSupport = stepIfInSupport; }
    inline void setAlwaysStepWithFarthest(bool alwaysStepWithFarthest){ m_footState.m_alwaysStepWithFarthest = alwaysStepWithFarthest; }
    inline void setStandUp(bool standUp){ m_standUp = standUp; }

#if DYNBAL_GIVEUP_RAMP
    inline void setGiveUpHeightEnd(float v) { m_giveUpHeightEnd = v; }; 
    inline void setGiveUpThresholdEnd(float v) { m_giveUpThresholdEnd = v; }; 
    inline void setGiveUpRampDuration(float v) { m_giveUpRampDuration = v; }; 
    inline void setLeanToAbort(float v) { m_leanToAbort = v; }; 
#endif

    void calibrateLowerBodyEffectors(NmRsHumanBody* body);
    void setLowerBodyGravityOpposition(NmRsHumanBody* body);

    void decrementTime(float amount) { if (amount < 0.0f) return; m_timer -= amount; if( m_timer < 0.f ) m_timer = 0.f; }
    void decrementSteps(int amount) 
    { 
      if (m_footState.m_numOfSteps4Max >=0 && amount > 0.0f)//If not the very first step
      {
        m_footState.m_numOfSteps4Max -= amount;
        if (m_footState.m_numOfSteps4Max < 0)
          m_footState.m_numOfSteps4Max = 0;
      }
    }

    // use to automatically set the 'lean-in-direction' value causing the balancer
    // to try and stumble in the calculated/chosen direction. setting a position or object
    // to walk to will recalculate a direction each update. 
    void autoLeanInDirection(const rage::Vector3& dir, float amount);
    void autoLeanRandom(float amountMin, float amountMax, float changeTimeMin, float changeTimeMax);
    void autoLeanToPosition(const rage::Vector3& pos, float amount);
    void autoLeanToObject(int objLevelIndex, int objBoundIndex, const rage::Vector3& offset, float amount);
    void autoLeanChangeAmountOverTime(float byAmt, float time);//mmmmNote This is currently unused
    void autoLeanCancel(); // stop any auto-lean processing
    // use to automatically set a different 'lean-in-direction' value causing the balancer
    // to try and stumble/Sway at hips in the calculated/chosen direction. setting a position or object
    // to walk to will recalculate a direction each update. 
    void autoLeanHipsInDirection(const rage::Vector3& dir, float amount);
    void autoLeanHipsRandom(float amountMin, float amountMax, float changeTimeMin, float changeTimeMax);
    void autoLeanHipsToPosition(const rage::Vector3& pos, float amount);
    void autoLeanHipsToObject(int objLevelIndex, int objBoundIndex, const rage::Vector3& offset, float amount);
    void autoLeanHipsChangeAmountOverTime(float byAmt, float time);//mmmmNote This is currently unused
    void autoLeanHipsCancel(); // stop any auto-lean processing

    void autoLeanForceInDirection(const rage::Vector3& dir, float amount, int bodyPart);
    void autoLeanForceRandom(float amountMin, float amountMax, float changeTimeMin, float changeTimeMax, int bodyPart);
    void autoLeanForceToPosition(const rage::Vector3& pos, float amount, int bodyPart);
    void autoLeanForceToObject(int objLevelIndex, int objBoundIndex, const rage::Vector3& offset, float amount, int bodyPart);
    void autoLeanForceChangeAmountOverTime(float byAmt, float time);//mmmmNote This is currently unused
    void autoLeanForceCancel(); // stop any auto-lean processing

    // body configuration functions
    void setLegCollision(bool enable);

#define BAL_STATES(_action) \
  _action(balOK) \
  _action(balFail_Leaning) \
  _action(balFail_Foot2HipHeight) \
  _action(balFail_LeaningAndHeight) \
  _action(balFail_Draped) \
  _action(balFail_General) \
  _action(balFail_Forced) \
  _action(balFail_BackArched)

    enum BalStates
    { 
#define BAL_ENUM_ACTION(_name) _name,
      BAL_STATES(BAL_ENUM_ACTION)
#undef BAL_ENUM_ACTION
    } m_failType;
    bool m_failedIfDefaultFailure;//Note doesn't get set to true if crouching

#if NM_STEP_UP
    // data packets that are not per-frame transient
    NmRsCBUDynBal_ReadOnly      m_roPacket;
#endif

    float m_distHeightRatio;
    float                       m_distKnee;
    float                       m_heightKnee;
    float                       m_distHeightKneeRatio;
    float                       m_dist;
    float                       m_height;
    float                       m_balanceInstability;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    int footState() const { return m_footState.m_footChoice; }

    void initialiseCustomVariables();
  protected:

    CBUTaskReturn tickPhase1(float timeStep);  // setup, kick foot/pelvis tasks
    CBUTaskReturn tickPhase2(float timeStep);  // block on foot/pelvis, kick solve task
    CBUTaskReturn tickPhase3(float timeStep);  // block on solve, use results

    void initialiseData();

    // put default values into the RO and module state packets
    void initializeDefaultDataPackets();

    // building parameter packets
    void populateBodyPacket();

    // accept changes from the modified body packet
    void readBodyPacketResults();

    // update posture control in character based on feet status
    void setPostureData(const NmRsCBUDynBal_FootState &foot);

    // apply any per-frame modifications to the module packets
    void updateModulePackets(NmRsCBUDynBal_FootState& foot, NmRsCBUDynBal_PelvisState& pelvis);

    // initialise the task parameter structures to defaults / empty
    void initialiseRAGETaskParams();

    // called per-frame to update task params before kicking off tasks
    void updateRAGETaskParams();

    // called per-frame to update the lean-in-direction if active
    void updateAutoLean(float timeStep);
    void updateAutoLeanHips(float timeStep);
    void updateAutoLeanForce(float timeStep);

    // we use RAGE to probe for the terrain heights near the characters
    // feet; because this talks to RAGE directly, we keep this phase local
    // to the manager rather than separating it into a isolated module
    // 
    // places results into RO packet member variable
    void probeForUnevenTerrain();

    // called every frame to sense if we are falling over and need the balance to disable;
    // test to see if we successfully balanced and fire feedback events if so.
    bool updateBalanceStatus();

    // data packets that are not per-frame transient
#if !NM_STEP_UP
    NmRsCBUDynBal_ReadOnly      m_roPacket;
#endif
    NmRsCBUDynBal_PelvisState   m_pelvisState;
    NmRsCBUDynBal_FootState     m_footState;
    NmRsCBUDynBal_BodyPacket    m_bodyPacket;

    // configurable values that get piped into the packets before processing
    rage::Vector3               m_customTurnDir;
    rage::Vector3               m_upOffset,m_upOffsetHips,m_upOffsetForce;
    float                       m_hipPitch;
    float                       m_hipRoll;
    float                       m_hipYaw;
    float                       m_legStraightnessModifier;
    float                       m_leanAgainstVelocity;
    float                       m_stepHeight;
#if NM_STEP_UP
    float                       m_stepHeightInc4Step;
#endif//#if NM_STEP_UP
    float                       m_kneeStrength;
    float                       m_stableSuccessMinimumRotSpeed;
    float                       m_stableSuccessMinimumLinSpeed;
    float                       m_leftLegStiffness;
    float                       m_rightLegStiffness;
    float                       m_leftLegSwingDamping;
    float                       m_rightLegSwingDamping;    
    float												m_opposeGravityLegs;
    float												m_opposeGravityAnkles;
    float                       m_leanAcc;
    float                       m_hipLeanAcc;
    float                       m_leanAccMax;
    float                       m_resistAcc;
    float                       m_resistAccMax;
    bool                        m_footSlipCompOnMovingFloor;

    float                       m_stepClampScale;
    float                       m_stepClampScaleIn;
    float                       m_stepClampScaleVariance;
    float                       m_balanceTimeIn;
    float                       m_balanceTimeVariance;
    float                       m_timer;
    float                       m_maximumBalanceTime;
    float                       m_balanceTimeAtRampDownStart;
    float                       m_giveUpHeight;
    float                       m_minKneeAngle;
    float                       m_fallMult;
#if DYNBAL_GIVEUP_RAMP
    float                       m_giveUpHeightEnd;
    float                       m_giveUpThresholdEnd;
    float                       m_giveUpRampDuration;
    float                       m_leanToAbort;
#endif
    int                         m_fallType;

    //int                         m_numAccs;

    struct autoLeanParams
    {
      autoLeanParams() : m_mode(eALNone), m_amountOverTimeMode(eAMNone),
        m_levelIndex(-1), m_boundIndex(-1), m_bodyPart(0), m_amount(0),
        m_amountMin(0),m_amountMax(0),m_changeTimeMin(0),m_changeTimeMax(0),
        m_amountDelta(0), m_timeRemaining(0) {}

      // direction vector if eALDirection
      // world-space position if eALToPosition 
      // offset from object position, if eALToObject
      rage::Vector3   m_vec;  

      enum
      {
        eALNone,
        eALDirection,
        eALToPosition,
        eALToObject,
        eALRandom,
        eALInvalid
      }               m_mode;
      enum
      {
        eAMNone,
        eAMModify,
        eAMInvalid
      }               m_amountOverTimeMode;

      int             m_levelIndex;
      int             m_boundIndex;
      int             m_bodyPart;
      float           m_amount;
      float           m_amountMin;
      float           m_amountMax;
      float           m_changeTimeMin;
      float           m_changeTimeMax;

      float           m_amountDelta;
      float           m_timeRemaining;

    }                           m_autoLeanParams, m_autoLeanHipsParams, m_autoLeanForceParams;

    enum
    {
      eDynBalTPInvalid,

      eDynBalTP_1,
      eDynBalTP_2,
      eDynBalTP_3
    }                           m_tickPhase;

    // task parameters
    rage::sysTaskParameters     m_footTaskParams;
    rage::sysTaskParameters     m_pelvisTaskParams;
    rage::sysTaskParameters     m_solveTaskParams;

    // task handles
    rage::sysTaskHandle         m_footTaskHandle;
    rage::sysTaskHandle         m_pelvisTaskHandle;
    rage::sysTaskHandle         m_solveTaskHandle;

    bool                      m_failed;           // true if we deactivated due to balance failure
    bool                      m_useCustomTurnDir;
    bool                      m_taperKneeStrength;
    bool                      m_stagger;
    bool                      m_plantLeg;
    bool                      m_airborneStep;
    bool                      m_rampHipPitchOnFail;
    bool                      m_balanceIndefinitely;
    bool                      m_rampDownBegun;
    bool                      m_failMustCollide;
    bool                      m_ignoreFailure;
    bool                      m_ignoringFailure;
    bool                      m_crouching;
    bool                      m_doneAVelocityProbe;
    bool                      m_deactivateMe;
    bool                      m_fallReduceGravityComp;
    bool                      m_standUp;
  };
}

#endif // NM_RS_CBU_DYNBAL_H

