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
#ifndef NM_RS_CBU_SHOTSTRENGTH_H 
#define NM_RS_CBU_SHOTSTRENGTH_H

#include "NmRsStateMachine.h"

namespace ART
{
     #define BulletBSpyDraw 1
    #define BulletBSpyScratchPad 1

    class BulletApplier : public AutoState
    {
    public:  

      enum T_Mode
      {
        TM_Disabled,
        TM_Strength,
        TM_Additive
      };

      enum T_SpinMode
      {
        TS_OriginalDir,
        TS_RandomDir,
        TS_FlipEachDir
      };

      enum T_FilterMode
      {
        TF_All,
        TF_LetFinish,
        TF_OnDirChange
      };

      BulletApplier(NmRsCharacter* character = NULL);
      ~BulletApplier(){};

      // helper to trigger this substate. returns proportion of impulse left after conversion to torque
      void newHit(int partID, const rage::Vector3 &imp, rage::Vector3 &position, float equalizer = 0.0f); 

      const rage::Vector3 &GetImpulse() const { return m_impulse; }

      // gets initialized in character's initializeData function by calling the configureBullet message without
      // parameters so that defaults are used as specified in NmRsMessageDefnitions. 
      struct Setup
      {
        // These values will be overwritten anyway (see above comment), so are here for safety just.
        Setup() : torqueDelay(0.0f), torqueGain(4.0f), torqueCutoff(0.5f), torquePeriod(0.12f), torqueReductionPerTick(0.9f),
          impulsePeriod(0.09f), impulseTorqueScale(1.0f), impulseDelay(0.018f), liftGain(0.0f), counterImpulseMag(0.5f), counterImpulseDelay(0.0334f), 
          counterImpulse2Hips(1.0f), impulseAirMult(1.0f ), impulseAirMultStart(10000.f), impulseAirMax(10000.f ), impulseAirApplyAbove(399.f), 
#if NM_ONE_LEG_BULLET
          impulseOneLegMult(1.0f ), impulseOneLegMultStart(10000.f), impulseOneLegMax(10000.f ), impulseOneLegApplyAbove(399.f), 
#endif//#if NM_ONE_LEG_BULLET
#if NM_RIGID_BODY_BULLET
          /*rbForce(1.f),*/rbRatio(0.f), rbLowerShare(0.5), rbMoment(1.f), rbMaxTwistMomentArm(0.5f), rbMaxBroomMomentArm(1.0f),
          rbRatioAirborne(0.f), rbMomentAirborne(1.f), rbMaxTwistMomentArmAirborne(0.5f), rbMaxBroomMomentArmAirborne(1.0f),
          rbRatioOneLeg(0.f), rbMomentOneLeg(1.f), rbMaxTwistMomentArmOneLeg(0.5f), rbMaxBroomMomentArmOneLeg(1.0f),
#endif
          torqueMode(TM_Disabled), torqueSpinMode(TS_OriginalDir), torqueFilterMode(TF_All),
#if NM_RIGID_BODY_BULLET
          rbTwistAxis(0),
#endif
          loosenessFix(false), torqueAlwaysSpine3(true), impulseSpreadOverParts(false), counterAfterMagReached(false), doCounterImpulse(false), impulseAirOn(false)
#if NM_ONE_LEG_BULLET
          , impulseOneLegOn(false)
#endif//#if NM_ONE_LEG_BULLET
#if NM_RIGID_BODY_BULLET
          , rbPivot(false) 
#endif
          {};

        float 
          torqueDelay,
          torqueGain,
          torqueCutoff,
          torquePeriod,
          torqueReductionPerTick,
          impulsePeriod,
          impulseTorqueScale,
          impulseDelay,
          liftGain,
          counterImpulseMag,
          counterImpulseDelay,
          counterImpulse2Hips,
          impulseNoBalMult,
          impulseBalStabStart, //100% <= Start to impulseBalStabMult*100% >= End
          impulseBalStabEnd, //100% <= Start to impulseBalStabMult*100% >= End
          impulseBalStabMult, //100% <= Start to impulseBalStabMult*100% >= End
          impulseSpineAngStart, //100% >= Start to impulseSpineAngMult*100% <= End
          impulseSpineAngEnd, //100% >= Start to impulseSpineAngMult*100% <= End
          impulseSpineAngMult, //100% >= Start to impulseSpineAngMult*100% <= End
          impulseVelStart, //100% <= Start to impulseVelMult*100% >= End
          impulseVelEnd, //100% <= Start to impulseVelMult*100% >= End
          impulseVelMult, //100% <= Start to impulseVelMult*100% >= End
          impulseAirMult,
          impulseAirMultStart,
          impulseAirMax,
          impulseAirApplyAbove
#if NM_ONE_LEG_BULLET
          ,impulseOneLegMult,
          impulseOneLegMultStart,
          impulseOneLegMax,
          impulseOneLegApplyAbove
#endif//#if NM_ONE_LEG_BULLET
#if NM_RIGID_BODY_BULLET
          ,/*rbForce,*/
          rbRatio,
          rbLowerShare,
          rbMoment,
          rbMaxTwistMomentArm,
          rbMaxBroomMomentArm,
          rbRatioAirborne,
          rbMomentAirborne,
          rbMaxTwistMomentArmAirborne,
          rbMaxBroomMomentArmAirborne,
          rbRatioOneLeg,
          rbMomentOneLeg,
          rbMaxTwistMomentArmOneLeg,
          rbMaxBroomMomentArmOneLeg
#endif
          ;
        int
          torqueMode,
          torqueSpinMode,
          torqueFilterMode
#if NM_RIGID_BODY_BULLET
          ,rbTwistAxis
#endif
          ;

        bool
          torqueAlwaysSpine3:1,
          impulseSpreadOverParts,
          counterAfterMagReached:1,
          loosenessFix:1,
          doCounterImpulse:1,
          impulseAirOn:1
#if NM_ONE_LEG_BULLET
          ,impulseOneLegOn:1
#endif//#if NM_ONE_LEG_BULLET
#if NM_RIGID_BODY_BULLET
          ,rbPivot:1
#endif
           ;
      };
      bool m_extra;
  
      Setup m_setup;  // configuration that this bullet will be running with
      static Setup s_nextSetup; // configuration set by game through message; used as buffer to not change current bullet while it's running
      static Setup s_nextSetupExtra; // configuration set by game through message; used as buffer to not change current bullet while it's running

#if ART_ENABLE_BSPY && BulletBSpyScratchPad
      void setBSpyID(int id) 
      {
        sprintf(m_impStr, "bullet - %i - impulse", id);
        sprintf(m_trqStr, "bullet - %i - torque", id);
        sprintf(m_ctrStr, "bullet - %i - counter", id);
        sprintf(m_idStr, "bullet - %i ", id);
      };
      char m_impStr [30];
      char m_trqStr[30];
      char m_ctrStr [30];
      char m_idStr [30];
#endif

    protected:

      // implemented base-class methods
      bool entryCondition();
      bool exitCondition();
      void update(float timeStep);
      void resetCustomVariables();

      void doPartMassCompensation(int partID, rage::Vector3 &impulse, float scale);
      void doTorqueCalculations(int partID, rage::Vector3 &impulse, const rage::Vector3 &position);
      void applyTorque();
      void applyImpulse();
      void applyCounterImpulse();
      void applyLift();
      
      // helpers
      bool partIsUpperBody(int partID);
      bool getNeighbours(int partID, int& n1, int& n2);
      float areaUnderTriangleProfile(float base, float a, float b);
      float areaUnderStraightProfile(float impulsePeriod, float timeNow, float timeStep);
      float areaUnderDownwardSlopeProfile(float base, float a, float b);
      float triangleAt(float x, float base);
      float rampDown(const float val, const float start, const float end, const float min);

      bool 
        m_doTorque,         // apply extended torques?
        m_doImpulse,        // apply extended impulse?
        m_doCounterImpulse, // apply counter impulse?
        m_doneCounterImpulse;// the counter Impulse has been applied

      float
        m_torqueTimer,
        m_moment,           // amount of torque in last step (reduces over time)
        m_impulseTot,       // the total impulse applied so far
        m_impulseTimer,
        m_counterImpulseTot,// the total counter impulse applied so far
        m_counterImpulseTimer,
        m_impulseMag,
        m_impulseScaleInAir;
#if NM_ONE_LEG_BULLET
      float m_impulseScaleOneLeg;
#endif//#if NM_ONE_LEG_BULLET
      // things shared by all bullets
      static int s_lastTorqueDir; 
      static int s_activeTorqueCount;

      int
        m_torquePartID,   // part selected for torque application
        m_impulsePartID;

      rage::Vector3 
        m_torque,
        m_torquePos,
        m_impulse,
        m_impulsePos;

	  enum BulletProfile
	  {
		  straightProfile = 0,
		  triangleProfile,
		  downwardSlopeProfile
	  };

	  static BulletProfile s_eBulletProfile;

    }; // class ShotStrengthState

} // NMS ART

#endif
