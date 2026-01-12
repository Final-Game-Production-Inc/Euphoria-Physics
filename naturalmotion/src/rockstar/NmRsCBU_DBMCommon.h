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

#ifndef NM_RS_CBU_DBMCOMMON_H
#define NM_RS_CBU_DBMCOMMON_H


#include "NmRsShadows.h"
#include "NmRsBodyLayout.h"
#include "NmRsUtils.h"

/**
 * NOTES
 *
 * This file is intended to be shared across the balancer manager and all
 * the isolated computation modules. As such it will be eventually referenced
 * by the SPU-compiled code and needs to have as few dependencies as possible.
 *
 * 'Parameter Packets' are defined that collect data used by each of the stages
 * of the balancer; each one is grouped to a particular usage/scope, so that we
 * don't necessarily have to upload the lot when running the modules on SPU.
 * Packets cannot contain pointers to our Effector classes, or Generic Parts, etc; 
 * use the shadow state structures to bake off the data components of these classes 
 * if you require their functionality.
 *
 */

// leave defined to have the balancer spew out debugging text to the logger, plus
// any profiler points defined in the modules
#if NM_RS_ENABLE_LOGGING
# include "ART/ARTInternal.h"
# endif // NM_RS_ENABLE_LOGGING


namespace ART
{ 
    // 'behaviour name' fed back to feedback interfaces
#define NMDynamicBalancerFeedbackName     "balance" 
#define NMDynamicBalancerAbortFeedbackName     "balanceAborted" 

    /**
     * Body Packet contains shadow states of all effectors and parts
     * used by balancer, plus any other data relating to the body.
     */
    MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
    NM_ALIGN_PREFIX(16) struct NmRsCBUDynBal_BodyPacket
    {
      // COM block, 128 bytes
      rage::Vector3   m_COM, 
                      m_COMvel, 
                      m_lvlCOMvelRelative,
                      m_COMrotvel;
      rage::Matrix34  m_COMTM;

      rage::Vector3 m_floorVelocity, m_floorAcceleration;
      //rage::Vector3 m_floorAcceleration_1, m_floorAcceleration_2;
      //rage::Vector3 m_floorSAcceleration_1, m_floorSAcceleration_2;

      ShadowGPart     m_leftHand;   
      ShadowGPart     m_rightHand;
      // left leg (816 bytes, 6.5 CL)
      Shadow3Dof      m_leftHip;    // 144
      ShadowGPart     m_leftThigh;  // 96
      Shadow1Dof      m_leftKnee;   // 176
      ShadowGPart     m_leftShin;   // 96
      Shadow3Dof      m_leftAnkle;  // 144
      ShadowGPart     m_leftFoot;   // 96 = 752
      rage::Matrix34  m_leftElbowMat;

      // right leg
      Shadow3Dof      m_rightHip;
      ShadowGPart     m_rightThigh;
      Shadow1Dof      m_rightKnee;
      ShadowGPart     m_rightShin;
      Shadow3Dof      m_rightAnkle;
      ShadowGPart     m_rightFoot;
      rage::Matrix34  m_rightElbowMat;

      Shadow3Dof      m_spine0;   // 144
      ShadowGPart     m_buttocks; // 96

      struct CollisionBitField
      {
        bool          m_leftFootCollided:1,
                      m_leftFootCollidedLast:1;
        bool          m_rightFootCollided:1,
                      m_rightFootCollidedLast:1;
        bool          m_leftHandCollided:1,
                      m_rightHandCollided:1;
      } cd;

      // --- 3 bytes padding 

      float           m_newWaistHeight;   // returned from balance solve, put back into
                                          // pelvis packet outside of task execution 

      // --- 8 bytes padding

    } NM_ALIGN_SUFFIX(16);
    MSVCEndWarningMacroBlock()

    /**
     * The RO Packet contains data provided by the manager or
     * configuration constants that never change / should not
     * be changed by any balancer module.
     * This is passed as a const reference.
     */
    MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
    NM_ALIGN_PREFIX(16) struct NmRsCBUDynBal_ReadOnly
    {
      //Needs to match definition in NmRsCBU_Shared.h
#define BCR_STATES(_action) \
  _action(bal_Normal)            /*0 normal balancer state*/  \
  _action(bal_Impact)            /*1 an impact has occured*/  \
  _action(bal_LeanAgainst)       /*2 Lean Against (sideways or backwards)*/  \
  _action(bal_LeanAgainstStable) /*3 Lean Against is stable - slump now or let the game recover to animation*/  \
  _action(bal_Slump)             /*4 Slump (slide down wall or just fall over)*/  \
  _action(bal_GlancingSpin)      /*5 Hit object at an angle therefore emphasize by adding some spin*/  \
  _action(bal_Rebound)           /*6 Moving away from impact - reduce strength and stop stepping soon*/  \
  _action(bal_Trip)              /*7 Used to stop stepping - e.g. after a predetermined number of steps after impact*/  \
  _action(bal_Drape)             /*8 Hit a table or something low - do a foetal for a while then catchfall/rollup*/  \
  _action(bal_DrapeForward)      /*9 Hit a table or something low - do a loose catchfall then rollup*/  \
  _action(bal_DrapeGlancingSpin) /*10 Hit a table or something low at an angle - add some spin*/  \
  _action(bal_Draped)            /*11 Triggered when balancer fails naturally during a drape or drapeGlancing spin or at end of drape (as drape can force the balancer to fail without the balancer assuming draped)*/  \
  _action(bal_End)               /*12 balancerCollisionsReaction no longer needed*/

      enum balancerState
      {
#define BCR_ENUM_ACTION(_name) _name,
        BCR_STATES(BCR_ENUM_ACTION)
#undef BCR_ENUM_ACTION
      };

      rage::Vector3   m_gUpReal;
      rage::Vector3   m_gUp;
      rage::Vector3   m_leanHipgUp;

      // feet-probe results from manager, updated per frame
      rage::Vector3   m_leftFootProbeHitPos,            // if no hit, set to feet position
                      m_leftFootProbeNormal,            // if no hit, set to m_gUp
                      m_leftFootHitPosVel,
                      m_rightFootProbeHitPos,
                      m_rightFootProbeNormal,
                      m_rightFootHitPosVel;
      
      //to offset feet pos in handsKnees
      rage::Vector3   m_left2right;
      
      //avoid stepping over a line
      rage::Vector3   m_pos1stContact,
                      m_normal1stContact;

      //avoid stepping into car
      rage::Vector3   m_carCorner1,
					            m_carCorner2,//Brace4Impact
					            m_carCorner3,//Brace4Impact
					            m_carCorner4;//Brace4Impact

      //avoid stepping over a line
      float           m_sideOfPlane;

      // time step fed in from game engine, updated per frame
      float           m_timeStep;
			float           m_timeTakenForStep;
			float           m_changeStepTime;

      // calculated from physical character
      float           m_fullLegLength;
      float           m_hipWidth;

      // from character config
      float           m_legStraightness;
      float           m_legSeparation;
      float           m_charlieChapliness;
      float           m_hipYaw;
      float           m_defaultHipPitch;

      //from configureBalance
	  float           m_extraFeetApart;
      // derived from leg separation, used by foot step calc
      float           m_lateralStepOffset;
      // to remove the chaplinesk walk you get if the hip twist limits are large and unsymmetrical
      float           m_hipTwistOffset;
      float           m_hipLean1Offset;
      float           m_hipLean2Offset;

      // some behavioural constants follow
      float           m_giveUpThreshold;                // used to decide if we're falling
      float           m_balanceTime;
      float           m_balanceTimeHip;
      float           m_dragReduction;
      float           m_stepDecisionThreshold;
      float           m_ankleEquilibrium;
      float           m_stepHeight;
      float           m_legsApartRestep;
      float           m_legsTogetherRestep;
      float           m_legsApartMax;
      float           m_stepClampScale;
      float           m_random;
      float           m_useComDirTurnVelThresh;
      float           m_avoidFootWidth;
      float           m_avoidFeedback;

      float           m_exclusionZone;//balancerCollsionsReaction
      int             m_balancerState;//balancerCollsionsReaction

      bool            m_taperKneeStrength;
      bool            m_probeHitLeft, m_probeHitRight;  // true if probes connected with something
      bool            m_stagger;
      bool            m_fallToKnees;
      bool            m_teeter;
      bool            m_plantLeg;
      bool            m_airborneStep;
      //avoid stepping over a line
      bool            m_impactOccurred;
      bool            m_flatterStaticFeet;
      bool            m_flatterSwingFeet;
      bool            m_movingFloor;
      //avoid stepping into car
      bool            m_avoidCar;
      //avoid stepping through the other leg
      bool            m_avoidLeg;
#if NM_STEP_UP
      bool            m_stepUp;
#endif//NM_STEP_UP
      bool            m_pushOff;
#if NM_SCRIPTING
      bool            m_pushOffBackwards;
#endif

      
      // adjust the component of rVec that represents the height (defined by gUp)
      inline void levelVectorReal(rage::Vector3& rVec, float height = 0.0f) const
      {
        float dot = m_gUpReal.Dot(rVec);
        rVec.AddScaled(rVec, m_gUpReal, (height - dot));
      }

      // return horizontal distance between two vectors, using gUp to factor out 'up' component
      inline float horizDistanceReal(const rage::Vector3& a, const rage::Vector3& b) const
      {
        rage::Vector3 diff(b - a);

        float dot = m_gUpReal.Dot(diff);
        diff.AddScaled(diff, m_gUpReal, -dot);
        return diff.Mag();
      }


      // adjust the component of rVec that represents the height (defined by gUp)
      inline void levelVector(rage::Vector3& rVec, float height = 0.0f) const
      {
        float dot = m_gUp.Dot(rVec);
        rVec.AddScaled(rVec, m_gUp, (height - dot));
      }

      // return horizontal distance between two vectors, using gUp to factor out 'up' component
      inline float horizDistance(const rage::Vector3& a, const rage::Vector3& b) const
      {
        rage::Vector3 diff(b - a);

        float dot = m_gUp.Dot(diff);
        diff.AddScaled(diff, m_gUp, -dot);
        return diff.Mag();
      }

    } NM_ALIGN_SUFFIX(16);
    MSVCEndWarningMacroBlock()

    /**
     * The state packet for the foot/lower leg computation module;
     * stored in manager across frames, fed into the balance solve module.
     */
    MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
    NM_ALIGN_PREFIX(16) struct NmRsCBUDynBal_FootState
    {
      enum FootChoice
      {
        kNotStepping = 0,
        kLeftStep,
        kRightStep,
      };

      rage::Vector3   m_leftFootPos,
                      m_rightFootPos;

      rage::Vector3   m_centreOfFeet;
      
      rage::Vector3   m_stepFootStart,
                      m_oldDesiredPos,
                      m_oldFootPos;

#if ART_ENABLE_BSPY
      //dynamicBalancer DebugDraw
      rage::Vector3 m_exclusionZonePt1;
      rage::Vector3 m_exclusionZonePt2;
#endif // ART_ENABLE_BSPY

      float           m_groundHeightLeft, 
                      m_groundHeightRight;
      float           m_leftDistance,
                      m_rightDistance;
      float           m_forceStepExtraHeight;//0.07f

      int             m_maxSteps,
                      m_numOfSteps4Max,
                      m_numOfSteps,
                      m_numOfStepsAtForceStep,
                      m_forceStep;//0 = don't, 1 = left, 2 = right

      FootChoice      m_footChoice;

      bool            m_forceBalance;
      bool            m_dontChangeStep;//don't change the stepping leg...was going to be used for hands and knees  
      bool            m_stepWithBoth;//Make the stance leg a stepping leg.  Used in catchfall.
      bool            m_newStep;
      bool            m_stepIfInSupport;
      bool            m_alwaysStepWithFarthest;

      struct FootStateBitField
      {
        bool          m_leftFootBalance:1,
                      m_rightFootBalance:1;

        bool          m_leftGround:1;
        bool          m_achievedGoal:1;
        bool          m_isInsideSupport:1;
        bool          m_isInsideSupportHonest:1;

      } state;
      

      // recalculate the centre of the feet
      inline void calculateCentreOfFeet()
      {
        m_centreOfFeet.Add(m_leftFootPos, m_rightFootPos);
#ifdef NM_PLATFORM_CELL_SPU
        m_centreOfFeet.Multiply(rage::VEC3_HALFc);
#else // NM_PLATFORM_CELL_SPU
        m_centreOfFeet.Multiply(rage::VEC3_HALF);
#endif // NM_PLATFORM_CELL_SPU
      }

    } NM_ALIGN_SUFFIX(16);
    MSVCEndWarningMacroBlock()

    /**
     * The state packet for the pelvis/hip computation module;
     * stored in manager across frames, fed into the balance solve module.
     */
    MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
    NM_ALIGN_PREFIX(16) struct NmRsCBUDynBal_PelvisState
    {
      rage::Vector3   m_pelvisSway;

      // configurable custom turn direction
      rage::Vector3   m_customTurnDir;

      // these are 1-frame-lagged from the foot step controller
      // so that we can try and retain some concurrency between feet/pelvis
      rage::Vector3   m_centreOfFeet;

      // configurable hip pitch value
      float           m_hipPitch;
      float           m_hipRoll;
      float           m_hipYaw;

      float           m_totalForwards;
      float           m_totalRight;

      float           m_twistLeft,
                      m_twistRight;

      float           m_waistHeight;

      // values returned from Balance Controller each frame to feed into IK solve
      float           m_hipRollCalc;
      float           m_ankleLean;

      bool            m_leftFootBalance,
                      m_rightFootBalance;

      bool            m_useCustomTurnDir;

    } NM_ALIGN_SUFFIX(16);
    MSVCEndWarningMacroBlock()
}

#endif // NM_RS_CBU_DBMCOMMON_H
