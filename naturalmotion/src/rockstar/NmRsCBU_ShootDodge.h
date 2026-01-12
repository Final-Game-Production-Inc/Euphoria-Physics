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

#ifndef NM_RS_CBU_SHOOTDODGE_H 
#define NM_RS_CBU_SHOOTDODGE_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"
#include "NmRsStateMachine.h"

// somewhat experimental code to shrink foot boxes to avoid snags.
#define NM_SHOOTDODGE_SMALL_FEET 1
#define NM_SHOOTDODGE_SMALL_FEET_SCALE 0.8f
// some smoothing for velocity input to orientation controllers.
#define NM_SHOOTDODGE_BUTTERWORTH 0
// support for caching and restoring ITMs as input to point gun.
#define NM_SHOOTDODGE_CACHE_ITMS 0

namespace ART
{
    class NmRsCharacter;
    #define NMShootDodgeFeedbackName      "ShootDodge"
    #define ShootDodgeBSpyDraw 1

    class NmRsCBUShootDodge : public CBUTaskBase
    {
    public:

      NmRsCBUShootDodge(ART::MemoryManager* services);
      ~NmRsCBUShootDodge();

      void onActivate();
      void onDeactivate();
      CBUTaskReturn onTick(float timeStep);

      void updateBehaviourMessage(const MessageParamsBase* const params);

      void fireWeapon(int hand = 1);

#if ART_ENABLE_BSPY
      virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

      // enum to indicate what orientation the character is currently in. 
      enum OrientationStatus {
        kFront = 0,
        kRightSide,
        kLeftSide,
        kBack,
        kHead
      };

      // enum to indicate what phase of the SD are in. 
      enum SDState{
        kInAir = 0,
        kCollideGround, // kEndCollision
        kOnGround,
        kLaunch,
        kCollideWall,
        kBail,
        kNumStates
      };

      enum CollisionType{
        kCollisionTypeNone = -1,
        kCollisionGround = 0,
        kCollisionWall,
        kCollisionObject,
        kCollisionFragmentable,
        kCollisionDoor,
        kCollisionGlass,
        kNumCollisionTypes
      };

      enum FeedbackType {
        kCollided = 0,
        kStableOnGround,
        kArmStatus,
        kUnSupported,
        kHardKey,
        kPreCollision
      };
     
      struct Parameters
      {
        rage::Vector3 rightArmTarget;
        rage::Vector3 leftArmTarget;
        bool targetValid;
        
        rage::Vector3 collisionPoint;
        rage::Vector3 collisionNormal;
        rage::Vector3 collisionObjectVelocity;

        float trunkStiffness;
        float trunkDamping;

        float legStiffness;
        float legDamping;
        float legTaper;

        float aimingArmStiffness;
        float aimingArmDamping;

        float notAimingArmStiffness;
        float notAimingArmDamping;
        float notAimingArmTaper;

        float headStiffness;
        float headDamping;

        float wristMuscleStiffness;

        float collisionTime;
        float collisionTimeThreshold;
        float collisionAccelThreshold;
        float collisionVelThreshold;
        int   collisionIndex;

        float supportedHeightThreshold;

        float legNoiseScale;
        float breathingScale;

        int collisionReaction; // EndCollisionType
        int state;             // SDState

        bool armAnimTask;

        int leftArmAnimTaskParent;
        int rightArmAnimTaskParent;

        float oriStiff;
        float oriDamp;
        float oriSplit;
        float velSmooth;

        float stickyRootThresh;
        float stickyRootRadius;

        float recoilRelaxAmount;
        float recoilRelaxTime;

        bool pointGunUseHelperTorques;
        bool pointGunUseHelperTorquesSupport;
        bool pointGunUseHelperForces;
        bool pointGunUseHelperForcesSupport;
        bool pointGunUseConstraint;
        float pointGunElbowAttitude;
        float pointGunOriStiff;
        float pointGunOriDamp;
        float pointGunPosStiff;
        float pointGunPosDamp;
        float pointGunClavicleBlend;
        float pointGunErrorThreshold;
        bool pointGunConstrainRifle;
        float pointGunRifleConstraintMinDistance;
        float pointGunConstraintMinDistance;
        float pointGunGravityOpposition;
        float pointGunFireWeaponRelaxDistance;

        bool timeWarpActive;
        float timeWarpStrengthScale;
        bool reversingDirection;

        rage::Vector3 groundVelocity;
        int groundInstance;

        float frictionScale;
        float restitutionScale;

        float spineBlend;

        float tuneWeaponMass;

        float reOrientScale;
        float reOrientTime;

        bool useHeadLook;

        bool hardKey;
        int hardKeyMask;

        float runtimeLimitsMargin;

      } m_parameters;

protected:

      void initialiseCustomVariables();

      void doOrientationControl();
      void orientationControl(
        NmRsGenericPart *part,
        float kp,
        float kd);
      void legPerlinNoise();
      void breathing();
      void startPointGun();
      float getWeightedCOMVel(float weight);
      OrientationStatus getOrientation();
      OrientationStatus getCollisionOrientation();
      float getCollisionVelocity();
      bool hasCollided();

      void driveSpine(float offsetAngle = 0);
      void driveSpineEffector(NmRs3DofEffector* effector, rage::Quaternion& rot);
      void driveHead();

      void successFeedback();
      void collisionFeedback();
      void stableFeedback();
      void armStatusFeedback(bool leftBracing, bool rightBracing);
      void unSupportedFeedback();

      bool probeTrajectory(rage::Vector3* pos, rage::Vector3* normal, float* time, float maxTime = 2.f);
      bool probeDown(rage::Vector3* pos, rage::Vector3* normal, float* height, float distance);

      void passThroughPointGunParams();

      bool isSupported(); /* { return m_isSupported; } // m_supported > 0.5; } */

      void setArmStrength(const ArmSetup* arm, float stiffness, float damping, float taper);
      void setLegStrength(const LegSetup* leg, float stiffness, float damping, float taper);

      // adapted from NmRsCBUShootDodge::calculateCoM(). takes a
      // part mask so it can be used to calculate COM info for 
      // subsets of the character parts. used to determine trunk
      // velocity for major collision detection.
      void calculateCoM(
        rage::Matrix34* comResult,
        rage::Vector3* comVelResult,
        rage::Vector3* comAngVelResult,
        rage::Vector3* angMomResult,
        BehaviourMask partMask) const;

      class SDStateBase : public State
      {
      public:
        SDStateBase() : m_character(0), m_fsm(0) {}
        void Update(float /*timeStep*/) {}
        NmRsCBUShootDodge* getContext() { return (NmRsCBUShootDodge*)m_context; }
        void setCharacter(NmRsCharacter* character) { m_character = character; }
        void setFSM(FSM* fsm) { m_fsm = fsm; }
      protected:
        NmRsCharacter* m_character;
        FSM* m_fsm;
      };

      void initStateMachine(FSM* fsm, SDStateBase** states, int numStates, SDStateBase* globalState = NULL);

      // todo: each fsm needs one of each of these.
      SDStateBase* enumToState(SDState stateEnum); 
      SDState stateToEnum(SDStateBase* state);
      SDState getState();

      friend class SDStateBase;

      // state machine for overall flow control
      class SDGlobalState : public SDStateBase {
      public:
        SDGlobalState() : m_bail(0) {}
        void Enter();
        void Update(float timeStep);
        void Exit();
      protected:
        float m_time;
        float m_bail;
      } m_sdGlobalState;
      class InAirState : public SDStateBase {
      public:
        InAirState() : m_takeoffAngle(0) {}
        void Enter();
        void Update(float timeStep);
        void Exit();
        float m_takeoffAngle;
        bool m_aboutToCrash;
        rage::Vector3 m_reOrientAxis;
        float m_reOrientAngle;
        BehaviourMask m_hardKeyMask;
      } m_inAirState;
      class CollideGroundState : public SDStateBase {
      public:
        CollideGroundState() : 
          m_timer(0), 
          m_timerOnGround(0),
          m_comVelMag(0) {}
        void Enter();
        void Update(float timeStep);
        void Exit();
        float m_timer;
        float m_timerOnGround;
        float m_comVelMag;
      } m_collideGroundState;
      class CollideWallState : public SDStateBase {
      public:
        CollideWallState() : 
          m_timer(0),
          m_timerAfterCollide(0),
          m_timerSupported(0),
          m_protectHead(false),
          m_headCollided(false) {}
        void Enter();
        void Update(float timeStep);
        void Exit();
      protected:
        float m_timer;
        float m_timerAfterCollide;
        float m_timerSupported;
        bool m_protectHead;
        bool m_headCollided;
      } m_collideWallState;
      class OnGroundState : public SDStateBase {
      public:
        OnGroundState() : m_stickyRootVelCache(0), m_timer(0), m_stickyRootActive(false) {}
        void Enter();
        void Update(float timeStep);
        void Exit();
        void AdjustLegHeight(const LegSetup* leg);
        float getITMRotVel(NmRsGenericPart* part, rage::Vector3* partVel = 0);
        rage::Vector3 m_stickyRootPosition;
        float m_stickyRootVelCache;
        float m_timer;
        bool m_stickyRootActive;
        rage::phConstraint *m_rootConstraint;
      } m_onGroundState;
      class BailState : public SDStateBase {
      public:
        void Enter();
        void Update(float timeStep);
        void Exit();
      } m_BailState;
      FSM m_stateMachine;
      SDStateBase* m_states[kNumStates];

      // state machine for pointing
      enum PointingStates{
        kPointing = 0,
        kNotPointing,
        kReloading,
        kNumPointingStates
      };
      class PointingGlobalState : public SDStateBase {
      public:
        void Enter();
        void Update(float timeStep);
        void Exit();
      } m_pointingGlobalState;
      class PointingState : public SDStateBase {
      public:
        void Enter();
        void Update(float timeStep);
        void Exit();
      } m_pointingState;
      class NotPointingState : public SDStateBase {
      public:
        void Enter();
        void Update(float timeStep);
        void Exit();
      } m_notPointingState;
      class ReloadState : public SDStateBase {
      public:
        void Enter();
        void Update(float timeStep);
        void Exit();
      protected:
        void reloadAction(int parentIndex, const ArmSetup* arm);
      } m_reloadState;
      SDStateBase* m_pointingStates[kNumPointingStates];
      FSM m_pointingStateMachine;

      // state machine for bracing
      enum BracingStates{
        kBracing = 0,
        kDefending,
        kIdling,
        kNumBracingStates
      };
      // state to actively defend against crash
      // by extending arms towards the predicted
      // collision.
      class BracingState : public SDStateBase {
      public:
        BracingState() :
          m_leftBracing(false),
          m_rightBracing(false),
          m_leftContact(false),
          m_rightContact(false)
        {}
        void Enter();
        void Update(float timeStep);
        void Exit();
      protected:
        void armIK(const ArmSetup* arm);
        bool m_leftBracing;
        bool m_rightBracing;
        bool m_leftContact;
        bool m_rightContact;
      } m_bracingState;
      // state to tuck head and shoulders in an
      // effort to protect head and roll through
      // crash.
      class DefendingState : public SDStateBase {
      public:
        DefendingState() {}
        void Enter();
        void Update(float timeStep);
        void Exit();
      protected:
      } m_defendingState;
      // inactive state. allows pointing and other
      // arm actions to take precedence.
      class IdlingState : public SDStateBase {
      public:
        void Enter();
        void Update(float timeStep);
        void Exit();
      } m_idlingState;
      // global state watches transition criteria
      // and switches between the three above states.
      class BracingGlobalState : public SDStateBase {
      public:
        BracingGlobalState() :
          m_timeThresholdOffset(0.f)
        {}
        void Enter() {}
        void Update(float timeStep);
        void Exit() {};
      protected:
        float m_timeThresholdOffset;
      } m_bracingGlobalState;
      SDStateBase* m_bracingStates[kNumPointingStates];
      FSM m_bracingStateMachine;

      rage::Vector3 m_groundVelocity; // velocity of ground under character pelvis.
      
      rage::Vector3 m_launchDirection;
      rage::Vector3 m_orientationTargetOffset;

      OrientationStatus m_orientation;
      OrientationStatus m_collisionOrientation;
      rage::Vector3 m_collisionDirection;

      rage::Vector3 m_trunkCOMVel;  // velocity metric that takes only trunk (pelvis through spine3) into account.
      rage::Vector3 m_trunkCOMAngVel;
      rage::Vector3 m_trunkCOMacc;
      rage::Vector3 m_trunkAngMom;

      float m_accelDotLaunchDir; // acceleration along leveled launch direction.
      float m_velDotLaunchDir;   // velocity along leveled launch direction.

      float m_timer;

      float m_oriStiffness;
      float m_oriDamping;
      float m_oriSplit;

      float m_timeWarpScale;

      bool m_hardKeyed;

      bool m_collisionFeedbackSent;
      bool m_stableFeedbackSent;

      rage::Vector3 m_pelvisRotVelCache;
      rage::Vector3 m_spine3RotVelCache;

      bool m_isSupported;
      float m_supported;

      float m_pelvisHeight;

#if NM_SHOOTDODGE_SMALL_FEET
      rage::Vector3 m_footSizeCache[2];
#endif

#if NM_SHOOTDODGE_BUTTERWORTH
      unsigned int getNextIndex(int diff);
      void updateAccelerationSmoothing(float timeStep);
      rage::Vector3 m_acceleration[3];
      unsigned int m_accelerationIndex;
      bool m_accelerationValid;
#endif

#if ART_ENABLE_BSPY
      public:
      BehaviourMask m_mask;
      protected:
#endif

#if NM_SHOOTDODGE_CACHE_ITMS
      // remove when we no longer need to hardcode itms.
      void cacheITMS();
      void writeITM(rage::Matrix34 *itPtr, int index);
#endif
    };
}

#endif // NM_RS_CBU_SHOOTDODGE_H


