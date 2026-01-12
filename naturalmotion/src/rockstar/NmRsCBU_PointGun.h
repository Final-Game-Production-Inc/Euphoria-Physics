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

#ifndef NM_RS_CBU_POINTGUN_H
#define NM_RS_CBU_POINTGUN_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsCBU_Shared.h"
#include "NmRsBodyLayout.h"
#include "NmRsLimbs.h"

namespace ART
{
  class NmRsCharacter;

#define NMPointGunFeedbackName      "pointGun"
#define PointGunBSpyDraw 1
#define PointGunIKBSpyDraw 1
#define USE_CLAVICLEIK2 0

  class NmRsCBUPointGun : public CBUTaskBase
  {

  public:

#define PG_STATES(_action) \
  _action(kUnused)             /*0 Arm is not enabled via parameter*/  \
  _action(kNotSet)             /*0 kUnused clone at moment - something has gone wrong*/  \
  _action(kShotUsing)          /*1 kUnused clone at moment mmmmtodo*/  \
  _action(kPointing)           /*2 Arm is pointing at target*/  \
  _action(kSupporting)         /*3 Arm is supporting the gun*/  \
  _action(kCantPoint)          /*4 Arm cannot point at target*/  \
  _action(kPointingWithError)        /*5 Arm cannot point at target*/  \
  _action(kNeutral)            /*3 Only for desiredStatus*/  \
  _action(kNeutralBroken)      /*7 Only for desiredStatus*/  \
  _action(kNeutral4Constrain)  /*6 Arm cannot point at target*/  \
  _action(kNeutralPointing)         /*3 Arm is supporting the gun*/  \
  _action(kNeutralPose)        /*8 Arm is doing a neutral pose*/

    enum ArmStatus
    { 
#define PG_ENUM_ACTION(_name) _name,
      PG_STATES(PG_ENUM_ACTION)
#undef PG_ENUM_ACTION
    };

//Point Gun Neutral Pose States
#define PGNP_STATES(_action) \
  _action(npsNone)               /*0 */  \
  _action(npsNotRequested)       /*1 */  \
  _action(npsByFace)             /*2 */  \
  _action(npsRifleFall)          /*3 */  \
  _action(npsRifleFallBySide)    /*4 */  \
  _action(npsByHip)              /*5 */  \
  _action(npsAcrossFront)        /*6 */  \
  _action(npsPointing2Connect)   /*7 */  \
  _action(npsByFaceSupport)      /*8 */  \
  _action(npsByHipSupport)       /*9 */  \
  _action(npsAcrossFrontSupport) /*10*/  \
  _action(npsBySide)             /*11*/  \
  _action(npsPointingDontConnect)/*12*/

    enum NeutralPoseState
    { 
#define PGNP_ENUM_ACTION(_name) _name,
      PGNP_STATES(PGNP_ENUM_ACTION)
#undef PGNP_ENUM_ACTION
    };

    enum FeedbackType {
      pgfbArmStatus = 0,
      pgfbAnimation = 1
    };

    NmRsCBUPointGun(ART::MemoryManager* services);
    ~NmRsCBUPointGun();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      rage::Vector3 leftHandTarget;
      int           leftHandTargetIndex;
      int           leftHandParentEffector;
      rage::Vector3 leftHandParentOffset;

      rage::Vector3 rightHandTarget;
      int           rightHandTargetIndex;
      int           rightHandParentEffector;
      rage::Vector3 rightHandParentOffset;

      float         leadTarget;

      bool          targetValid;

      float         primaryHandWeaponDistance;
      float         armStiffness;
      float         armStiffnessDetSupport;
      float         armDamping;

      int           supportConstraint;
      int           oneHandedPointing;
      float         constraintMinDistance;
      float         makeConstraintDistance;
      float         constraintStrength;
      float         constraintThresh;
      float         reduceConstraintLengthVel;
      float         breakingStrength;
      float         brokenSupportTime;
      float         brokenToSideProb;
      float         connectAfter;
      float         connectFor;

      float         clavicleBlend;
      float         elbowAttitude;

      bool          useIncomingTransforms;
      bool          measureParentOffset;

      int           weaponMask;

      float         oriStiff;
      float         oriDamp;
      float         posStiff;
      float         posDamp;

      float         fireWeaponRelaxTime;
      float         fireWeaponRelaxAmount;
      float         fireWeaponRelaxDistance;

      float         gravityOpposition;
      float         gravOppDetachedSupport;
      float         massMultDetachedSupport;

      bool          constrainRifle;
      float         rifleConstraintMinDistance;

      bool          enableRight;
      bool          enableLeft;
      bool          disableArmCollisions;
      bool          disableRifleCollisions;
      bool          poseUnusedGunArm;
      bool          poseUnusedSupportArm;
      bool          poseUnusedOtherArm;

      bool          timeWarpActive;
      float         timeWarpStrengthScale;

      float         maxAngleAcross;
      float         maxAngleAway;

      int           fallingLimits;
      float         acrossLimit;
      float         awayLimit;
      float         upLimit;
      float         downLimit;

      float         errorThreshold;

      bool          usePistolIK;

      bool          useSpineTwist;
      bool          useTurnToTarget;
      bool          useHeadLook;
      bool          alwaysSupport;
      bool          allowShotLooseness;

      int           pistolNeutralType;
      bool          neutralPoint4Pistols;
      bool          neutralPoint4Rifle;
      bool          checkNeutralPoint;
      int           rifleFall;
      int           fallingSupport;
      int           fallingTypeSupport;
      rage::Vector3 point2Side;
      rage::Vector3 point2Connect;
      float         add2WeaponDistSide;
      float         add2WeaponDistConnect;

    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

    void fireWeapon(NmRsHumanBody& body, int hand, float fireStrength, bool applyFireGunForceAtClavicle, float inhibitTime, rage::Vector3& direction, float split /* = 0.5 */);

    bool isFiring() { return(m_armData[0].relaxScale < 1.0f && m_armData[1].relaxScale < 1.0f); }

    bool m_forceNeutral;//e.g. fall2Knees forces pointGun into neutral pose.  NB> only forces neutral on gunHand  mmmmtodo break constraint choice, supportingHand.
    void enableLeftArm(bool enable) { m_enableLeft = enable; }//if false - Shot is taking control of the left arm to reachForWound 

  protected:

    struct ArmData
    {
      NmRsHumanArm* arm;
      NmRsLimbInput* input;
      NmRsArmInputWrapper* inputData;

      rage::Matrix34 desiredHandToWorld;
      rage::Matrix34 desiredSupportHandToWorld;
      rage::Matrix34 gunToHand;
      rage::Matrix34 currentGunSupportInWorld;
      rage::Matrix34 desiredGunSupportInWorld;
      rage::Vector3 pointingFromPosition;
      rage::Vector3 target;//target used to move arm - can be actual target or a neutral target
      rage::Vector3 currentTarget;//the actual target sent in by game
      rage::Vector3 lastTarget;
      rage::Vector3 leadTarget;
      rage::Vector3 targetDirection;
      rage::Vector3 targetVel;
      rage::Vector3 lastHandTargetPos;
      rage::Vector3 handTargetVel;
      float relaxTimer;
      float relaxScale;
      float weaponDistance;//distance from pointingFromPosition to gunHand.  This is modulated for the pistol as aiming comes across the chest
      float palmScaleDir;
      float error;
      float fireInhibitTimer;
      float currentGunSupportDistanceFromShoulder;
      float desiredGunSupportDistanceFromShoulder;
      float desiredTargetGunSupportDistanceFromShoulder;
      float currentGunSupportDistanceFromHand;
      ArmStatus status;
      ArmStatus desiredStatus;
      NeutralPoseState neutralPoseType;
      bool armStatusChanged;
      bool currentGunSupportable;
      bool desiredGunSupportable;
      bool desiredTargetGunSupportable;//mmmmtodo support variables should be moved out of ArmData?
      
      bool currentGunSupportBehindBack;
      bool supportHandBehindBack;
      bool supportBehindBack;
#if ART_ENABLE_BSPY
      bool desiredGunSupportBehindBack;//is this needed?
#endif

    };

    void initialiseCustomVariables();

    //GunArm
    bool isGunHandTargetReachable(ArmData& armData);
    ArmStatus computeGunArmState(ArmData& armData);
    void calculateGunHand(ArmData& armData, const ArmStatus gunArmStatus = kNotSet);
    ArmStatus moveGunArm(ArmData& armData, const ArmStatus gunArmStatus, bool calculate);
    ArmStatus pointGunHand(ArmData& armData, bool calculate, const ArmStatus gunArmStatus = kNotSet);
    void pointSingleHandedGun(ArmData& armData);

    //SupportArm
    bool isSupportHandTargetReachable(ArmData& primaryArmData, ArmData& supportArmData);
    void pointDoubleHandedGun(ArmData& primaryArmData, ArmData& supportArmData);

    //SupportArm to Weapon(GunHand) Constraint
    bool isConstraintBehindBack(ArmData& primaryArmData, ArmData& supportArmData);
    bool isConstraintBroken(ArmData& primaryArmData, ArmData& supportArmData);
    bool supportConstraintActive(ArmData& supportArmData, float distToConstraint);
    bool supportConstraintEffective(ArmData& supportArmData);
    void supportConstraintUpdate(ArmData& primaryArmData, ArmData& supportArmData);

    //Rifle constraint
    void rifleConstraintTick(ArmData& primaryArmData);
    void updateRifleConstraint(
      const NmRsHumanArm *primaryArm,
      rage::Vector3& handConstraintPos,
      rage::Vector3& spine3ConstraintPos,
      float minimumDistance = 0);
    void releaseRifleConstraint();

    //When not pointing at target or supporting
    bool poseUnusedArm(ArmData& armData);
    ArmStatus setNeutralPoseOrNeutralPoint(ArmData& armData);
    bool selectNeutralTarget(ArmData& armData);
    ArmStatus setNeutralPose(ArmData& armData);
    ArmStatus setNeutralPoseType(ArmData& armData, NeutralPoseState pose, bool checkAllowed = false);

    void updateArmData(float timeStep);
    void setArmMuscles(ArmData& armData);
    void setHandCollisionExclusion();
#if 0
    void intoWorldTest(const NmRsHumanArm *arm, rage::Vector3 &target);
#endif
    bool armEnabled(const NmRsHumanArm* arm);

    void setArmStatus(ArmData& armData, ArmStatus status);
    void armStatusFeedback();
    void animationFeedback();
    void handAnimationFeedback();

    //returns desiredGunToWorld
    void getDesiredGunToWorldOrientation(rage::Matrix34 &desiredGunToWorld, const rage::Vector3 &pointingFromPosition, const rage::Vector3 &target);
    void pistolIk(
      ArmData& armData,
      float /* twist */,
      rage::Matrix34& target);
    void ik(
      ArmData& armData,
      float twist,
      rage::Matrix34& target,
      float wristBlend);
    void clavicleIK(
      const NmRsHumanArm* arm,
      NmRsLimbInput& armInput,
      const ArmData& armData,
      rage::Vector3& target,
      float limitAmount = 0.9f);
#if USE_CLAVICLEIK2
    rage::Quaternion clavicleIK2(
      const NmRsHumanArm* arm,
      NmRsLimbInput* armInput,
      rage::Vector3& targetDirection,
      float limitAmount = 0.95f);
#endif//USE_CLAVICLEIK2
    void twoBoneIK(
      const NmRsHumanArm* arm,
      NmRsLimbInput& armInput,
      rage::Vector3& target,
      float twist,
      rage::Matrix34& shoulderMatrix1);

    void getDesiredHandTM(rage::Matrix34& handTM);
    float clampAndDriveQuat(const NmRs3DofEffector* eff, NmRs3DofEffectorInputWrapper* effData, rage::Quaternion& q, float amount);

  protected:

    ArmData m_armData[2];

    rage::phConstraintHandle m_rifleConstraint;

    rage::Matrix34 m_supportHandToGunHand; // store offset computed from ITMs
    rage::Matrix34 m_supportHandToGun;
    rage::Vector3 m_measuredRightHandParentOffset;
    rage::Vector3 m_measuredLeftHandParentOffset;

    float m_maxArmReach;
    float m_rifleConstraintDistance;
    float m_timeWarpScale;

    float m_neutralSidePoseTimer;//Randomly set when ConstraintInhibited if m_supportConstraintInhibitTimer>0 then this timer chooses a sidePose(non-connecting)pose for the gunArm instead of a connecting pose
    float m_supportConstraintInhibitTimer;//Constraints will be inhibited and poses taken to reflect this. At the moment set when a constraint is broken
    float m_gunArmNotPointingTime;
    float m_reConnectTimer;
    float m_add2WeaponDist;//Only applied to gunArm

    NeutralPoseState m_rifleFallingPose;
    HandAnimationType m_handAnimationType;
    WeaponMode m_weaponMode;

    bool  m_probeHitSomeThing;
    bool  m_forceConstraintMade;
    bool m_enableLeft;//if false - Shot is taking control of the left arm to reachForWound 
    bool m_rifleFalling;
    bool m_decideFallingSupport;
    bool m_allowFallingSupportArm;
  };
}
#endif // NM_RS_CBU_POINTGUN_H
