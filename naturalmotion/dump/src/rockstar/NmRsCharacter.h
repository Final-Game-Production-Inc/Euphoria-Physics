/*
* Copyright (c) 2005-2010 NaturalMotion Ltd. All rights reserved.
*
* Not to be copied, adapted, modified, used, distributed, sold,
* licensed or commercially exploited in any manner without 
* written consent of NaturalMotion.
*
* All non public elements of this software are the confidential
* information of NaturalMotion and may not be disclosed to any
* person nor used for any purpose not expressly approved by
* NaturalMotion in writing.
*
*/

#ifndef NM_RS_CHARACTER_H
#define NM_RS_CHARACTER_H

#include "physics/inst.h"
#include "fragment/instance.h"
#include "NmRsBodyLayout.h"
#include "NmRsLimbs.h"
#include "NmRsEffectorDefs.h"
#include "NmRsArticulatedWrapper.h"
#include "NmRsUtils.h"
#include "NmRsFrictionHelper.h"

#if NM_USE_1DOF_SOFT_LIMITS
#include "NmRs1DofSoftLimit.h"
#endif //NM_USE_1DOF_SOFT_LIMITS

#if ART_ENABLE_BSPY
#include "NmRsSpy.h"
#include "bspy/NmRsSpyPackets.h"
#endif // ART_ENABLE_BSPY

#include "NmRsBullet.h"
#include "physics/asyncshapetestmgr.h"
#include "physics/leveldefs.h"



#if NM_SCRIPTING
#include "art\messageparams.h"
#endif

// number of mask codes that can be pushed onto the mask code stack
#define RS_MASKCODESTACK_SZ (4)

#define NM_RS_MASKING_DEBUG 0

namespace rage
{
  class phSimulator;
  class phCollider;
  class phBound;
  class phArticulatedCollider;
  class phJoint1Dof;
  class phJoint3Dof;
  class Matrix34;
}

namespace ART
{
	// This list of inst types should match that used in gtaInst.h (game-side code)
	enum
	{
		PH_INST_GTA = rage::fragInst::PH_INST_GDF_LAST+1,	// marker for start of GTA phInst's
		PH_INST_MAPCOL,
		PH_INST_BUILDING,
		PH_INST_VEH,
		PH_INST_PED,
		PH_INST_OBJECT,
		PH_INST_PROJECTILE,
		PH_INST_EXPLOSION,
		PH_INST_PED_LEGS,

		PH_INST_FRAG_GTA,							// marker for start of GTA fragInst's
		PH_INST_FRAG_VEH,
		PH_INST_FRAG_PED,
		PH_INST_FRAG_OBJECT,
		PH_INST_FRAG_BUILDING, 
		PH_INST_FRAG_CACHE_OBJECT,

		PH_INST_FRAG_LAST
	};

#define MAX_NUM_LINKS 21

#define NMBalanceStateFeedbackName     "balanceState" 
  class NmRsGenericPart;
  class NmRsEffectorBase;
  class NmRsCBUTaskManager;
  class NmRsLimbManager; 
  class NmRsLimb;
#if ART_ENABLE_BSPY
  class NmRsSpy;
#endif // ART_ENABLE_BSPY

  struct CBURecord;

  typedef rage::phMaterialMgrImpl<rage::phMaterial>       PHMaterialManager;

  struct NmRsCharacterTypeData
  {
    char m_Identifier[8];
    int m_Num1DofJoints;
    int m_Num3DofJoints;
#if ART_ENABLE_BSPY
    bSpy::bSpyStringToken *m_PartTokens;
    bSpy::bSpyStringToken *m_JointTokens;
#endif
    rage::Matrix34 *m_InitialMatrices;
    rage::Matrix34 *m_ComponentToBoneMatrices;
    NmRs1DofEffectorParams *m_1DofEffectorParams;
    NmRs3DofEffectorParams *m_3DofEffectorParams;

    // Buoyancy modifiers
    float m_PartBuoyancyMultipliers[21];
    float m_BodyBuoyancyMultiplier;
    float m_DragMultiplier;
    float m_WeightBeltMultiplier;
  };

  /**
   * NmRsCharacter is the container for the entire simulated NM character. It contains
   * the physical description of the figure, joints, effectors, management code
   * and pretty much everything else to do with getting our characters acting on screen.
   * There is one NmRsCharacter per character instance in a simulator.
   */
  class NmRsCharacter
#if ART_ENABLE_BSPY
    // HDD; as a way to capture all feedback, we intercept calls to GetFeedbackInterface
    //      and return our own intermediary - in this case, the character itself, which can
    //      send back the calls as well as forwarding them onto the real, live feedback interface in the game
    : public ARTFeedbackInterface
#endif // ART_ENABLE_BSPY
  {
  public:
#if 0
    //EUPHORIA ONLY.  To run usercontrolled dragged behaviour in Euphoria)
    bool m_grabRight;
    bool m_grabLeft;
#endif
#if NM_SCRIPTING
    struct MMMessage
    {
      ART::MessageParams params;
    };
    FILE* m_pNMScriptFile;
    InvokeUID m_blockedUIDs[100];
    InvokeUID m_delayedUIDs[100];
    int m_delayForUIDs[100];
    int m_numOfBlockedUIDs;
    int m_numOfDelayedUIDs;
    InvokeUID m_delayedMessageUID[10];
    MMMessage m_delayedMessageParams[10];
    int m_delayedMessageDelay[10];
    int m_currentDelayedMessage;

    //Variables from NMBlockedAndVars.txt
    float m_bulletImpulseMag;
    float bulletImpulseMult;
    float rbRatio;
    float rbLowerShare;
    float rbForce;
    float rbMoment;
    float rbMaxBroomMomentArm;
    float rbMaxTwistMomentArm;
    float bulletTorqueMult;
    float impulseTorqueScale;
    float kMultForLoose;
    float m_minLegStraightness;
    float m_minLegSeperation;
    float m_maxLegSeperation;
    float minArmsLooseness;
    float minLegsLooseness;
	  float snapMag;

    int rbTwistAxis;

    float ctmTimeStep;
    int ctmStart;
    int ctmEnd;
    int ctmCurrent;
    char ctmAnimation[80];
    rage::Matrix34 *previousTM;
    rage::Matrix34 *currentTM;
    bool teleport;
    bool newBullet;
    bool m_newHitEachApplyBulletImpulseMessage;
    bool balIgnoreFailure;
    bool balLegCollisions;
    bool alwaysStepWithFarthest;
    bool stepIfInSupport;

    bool oldTripping;
    bool newTripping;
    bool stepUp;
    bool pushOff;
    bool pushOffBackwards;
    bool rbPivot;

    bool m_allowMeasureCharacter;
    bool getZeroPose;
    bool emergencyVelocityClamp;
    bool overideAnimPose;
    bool comAngularClamp;
    bool bulletDirection;
    bool angularClamp;
    bool oldClamp;
    bool bcrWrithe;
    bool m_allowLegShot;
    bool m_alwaysStepWithFarthest;
    bool m_behaviourMasking;
    bool m_standUp;
    bool alreadyHeldPose;
    bool scaleArmShot;
    bool allowArmShot;
    bool bulletMomA;
    bool bulletMomB;
    bool oldCOMCalcTime;
    bool m_newHitAsApplyBulletImpulseMessage;
#endif
#if NM_OBJECTS_IN_COLLISIONS
    float m_objectMass;
    rage::Vector3 m_objectSize;
    bool m_objectFixed;
#endif

    NmRsCharacter(ART::MemoryManager* services, class NmRsEngine *rsEngine, AssetID asset, AgentID agentID, unsigned randomSeed);
    virtual ~NmRsCharacter();

#if ART_ENABLE_BSPY
    // modes that control the rendering of skeleton overlays in Studio
    enum SkelVizMode
    {
      kSV_None,             // no skeleton visualization
      kSV_DesiredAngles,    // show joint skeleton for desired angles
      kSV_ActualAngles,     // ... or for actual angles
    };
#endif
    // A new pi_* should be associated with a async asyncProbeIndex by changing NmRsCharacter::rayIndexToProbeIndex(...)
#define PROBE_INDEX(_action) \
      _action(pi_balLeft) \
      _action(pi_balRight) \
      _action(pi_catchFallLeft) \
      _action(pi_catchFallRight) \
      /*_action(pi_rollDownStairs) unused*/\
      _action(pi_highFall) \
      _action(pi_learnedCrawl) \
        /*_action(pi_pointGun) unused*/\
      _action(pi_UseNonAsync) /*for crawl learning, landing, pointGun mmmmmtodo*/\
      _action(pi_highFallDown) \
      _action(pi_LightWeightAsync) \
      _action(pi_probeCount)

    enum rayProbeIndex
    {
#define ENUM_ACTION(_name) _name,
      PROBE_INDEX(ENUM_ACTION)
#undef ENUM_ACTION
    };

    enum asyncProbeIndex
    {
      api_One,
      api_Two,
      api_Three,
      api_probeCount,
    };

    enum NmRsHand
    {
      kLeftHand = 0,
      kRightHand
    };

    // called when a task manager has been initialised with this character; save the pointer
    inline void associateCBUTaskManager(NmRsCBUTaskManager* taskMgr, CBURecord* record) { m_cbuTaskManager = taskMgr; m_cbuRecord = record; }

    void calculateCoMValues();

#define ORIENTATION_STATES(_action) \
    _action(OS_Up) \
    _action(OS_Down) \
    _action(OS_Front) \
    _action(OS_Back) \
    _action(OS_Left) \
    _action(OS_Right) \
    _action(OS_NotSet)

    enum OrientationStates
    {
#define ENUM_ACTION(_name) _name,
      ORIENTATION_STATES(ENUM_ACTION)
#undef ENUM_ACTION
    };

    void getPredictedCOMOrientation(float predictTime, rage::Matrix34* predTM) const;
    OrientationStates getFacingDirectionFromCOMOrientation(const rage::Matrix34 &COMOrient, const rage::Vector3 *up = NULL) const;

    // Build the character based on the input collider and the engine's NmRsCharacterTypeData
    bool setupCharacter(rage::phArticulatedCollider *inputCollider);
    /**
     * add all the objects registered into this clump during a deserialize
     * into a RAGE physical simulation
     */
    bool addToScene(rage::phSimulator *sim, rage::phLevelNew* level);

    /**
     * remove and tidy all items used in addToScene
     */
    bool removeFromScene();

    //calculates the ComValues
    void prepareForSimulation();

    /**
    * true if the clump has been inserted into the virtual world
    */
    inline bool isInsertedInScene() const { return m_isInsertedInScene; }

    /**
     * update sensors
     */
    void preStep();

    /**
     * update effectors
     */
    void postStep(float deltaTime);

    /**
     * handle an invoke request from the game, via the NmRsEngine instance
     */
    bool handleDirectInvoke(InvokeUID iUID, const MessageParamsBase* const params);

    /**
     * called to pass through to the sensors and effectors to handle collision events
     */
    void handleCollision(rage::phContactIterator impact);

    /**
     * manages collision disabling based on hand collision exclusions. registers collisions within set
     * for use in restoring defaults. returns true if collision impact should be disabled.
     */
    bool disableHandCollisions(NmRsGenericPart* gpA, NmRsGenericPart* gpB);
    bool disableLegCollisions(NmRsGenericPart* gpA, NmRsGenericPart* gpB, float impactDepth, bool &justSoftenImpact);

    //Adds a NmRsGenericPart
    NmRsGenericPart* addArticulated(rage::phArticulatedBodyPart* artpart, int partIndex);

    /**
     * there are all sorts of ways to look up GenericParts
     */
    NmRsGenericPart* getGenericPartByIndex(int index) const;
#if NM_UNUSED_CODE
    NmRsGenericPart* lookupPartForInstance(rage::phInst *inst) const;
#endif

    bool getMatrixForPart(int index, rage::Matrix34& mtm) const;
    bool getMatrixForPartByComponentIdx(int index, rage::Matrix34& mtm) const;

    const rage::phBound* getBoundByComponentIdx(int idx) const;

    /**
     *
     */
    void setArticulatedPhysInstance(rage::phInst* inst);
    rage::phInst * getArticulatedPhysInstance();
    void updateArticulatedWrapperInertias();

    /**
     * utility functions acting on entire effector set
     */

private:
    void holdPoseAllEffectors();
    void storeZeroPoseAllEffectors();
    void recalibrateAllEffectors();
    void resetAllEffectors();

public:

    void setBodyDriveState(rage::phJoint::driveState state);

#if NM_RUNTIME_LIMITS
#if NM_UNUSED_CODE
    void disableAllLimits();
#endif
    void restoreAllLimits();
    void openAllLimitsToDesired();
#endif

    /**
     * mask code stack
     */

    BehaviourMask expressionToMask(const char* expression);
    BehaviourMask nameToMask(const char* const maskName);
    BehaviourMask partToMask(const int partIndex) const;
    BehaviourMask evaluateMaskExpression(const char* const expression, int start, int end);
    BehaviourMask hexToMask(const char* hex) const;
    BehaviourMask binToMask(const char* hex) const;

    bool isPartInMask(const BehaviourMask mask, int partIndex) const;
    bool isEffectorInMask(const BehaviourMask mask, int effectorIndex) const;

#if NM_FAST_COLLISION_CHECKING
    // functions to update collision cache
    void cacheEnvironmentCollision(int partIndex) { m_collidedEnvironmentMask |= partToMask(partIndex); }
    void cacheOwnCharacterCollision(int partIndex) { m_collidedOwnCharacterMask |= partToMask(partIndex); }
    void cacheOtherCharacterCollision(int partIndex) { m_collidedOtherCharactersMask |= partToMask(partIndex); }
#endif

    /**
     * create and register an effector
     */
    NmRsEffectorBase* add1DofEffector(
      rage::phJoint1Dof* joint, int jointIndex, int jointTypeIndex, NmRs1DofEffectorParams &info );

    NmRsEffectorBase* add3DofEffector(
      rage::phJoint3Dof* joint, int jointIndex, int jointTypeIndex, NmRs3DofEffectorParams &info );

    // body accessors, for convenience.
    // phase2 todo make body-agnostic?
  private:
    NmRsHumanArm *getLeftArmSetup()  { return m_body.getLeftArm();  }
    NmRsHumanArm *getRightArmSetup() { return m_body.getRightArm(); }
    NmRsHumanLeg *getLeftLegSetup()  { return m_body.getLeftLeg();  }
    NmRsHumanLeg *getRightLegSetup() { return m_body.getRightLeg(); }
    NmRsHumanSpine *getSpineSetup()  { return m_body.getSpine(); }

  public:

    /**
     * apply the supplied character configuration to this clump
     */
    void configureCharacter(bool useZeroPose, bool leftHandFree, bool rightHandFree, float stanceBias, float COMBias);//mmmmUnused leftHandFree, rightHandFree
    void configureTheCharacter(bool setZeroPose, bool setZeroPoseArms, bool configureBalancer, float stanceBias, float COMBias);
#if CRAWL_LEARNING
    void disableSelfCollision();
#endif
    void setIncomingTransforms(rage::Matrix34* ptr, IncomingTransformStatus statFlag, int tmcount, IncomingTransformSource source);
    void getIncomingTransforms(rage::Matrix34 **ptr, IncomingTransformStatus &statFlag, int &tmcount, IncomingTransformSource source) const;

    rage::Matrix34* GetWorldLastMatrices();
    rage::Matrix34* GetWorldCurrentMatrices();
#if NM_ANIM_MATRICES
    rage::Matrix34* GetBlendOutAnimationMatrices();
#endif

    /**
     * incoming transform support called by NmRsEngine
     */
    void applyIncomingTransforms(float deltaTime);
    void setIncomingTransformApplyMode(const IncomingTransformApplyMode mode);
#if ART_ENABLE_BSPY && NM_RS_MASKING_DEBUG
    IncomingTransformApplyMode getIncomingTransformApplyMode() const;
#endif
    void setIncomingTransformMask(BehaviourMask mask);
    void updateIncomingTransformApplyMode();

    bool setInitialTM(int componentIdx, const rage::Matrix34& newTm, const rage::Matrix34 *previousFrameTm = 0);

    rage::Vector3 getOpposeTorque(int i, const rage::Vector3 &gravity, rage::Vector3 &toCOM, rage::Vector3 &weight);
    float pointInsideFootSupport(const rage::Vector3 &point, int leftIndex, int rightIndex, float footWidth, float footLength, rage::Vector3 *nearestPoint);
    bool noBehavioursUsingDynBalance();
    void applyZMPPostureControl(float footWidth, float footLength);
    void applyPostureControl(float clampScale, rage::Vector3 *ZMP = NULL);

    void applyRootOrientationControl(NmRsGenericPart *partToControl, float forceScaler, float P, float D, float extraRotationAroundSide, bool scaleWithTime);

    void getIncomingTranformByBodyPart(rage::Matrix34 &matrix, int bodyPart, bool useInterpolation);

    //void hardConstraintOnCharacter(int *partToControl, rage::Vector3 *offset = NULL, bool *usePosition=NULL, bool *useOrientation=NULL);

    /**
     * called once per frame to .. well.. compensate for foot slip
     * By default, the orientation of the foot is initialized only if (!foot->previousCollided)
     * if forceInitializeFootOrientation is true, initialization of the orientation of the foot
     * will be done even if foot->previousCollided (for the both feet)
     */
    void applyFootSlipCompensation(float twistStrength, float twistDamping, float mult);

    /**
     * given the matrices for critical parts, returns useful measurements for things like leg separation
     */
    void measureCharacter(rage::Matrix34 &leftFootMat, rage::Matrix34 &rightFootMat,
      rage::Matrix34 &pelvisMat, rage::Matrix34 &headMat, rage::Matrix34 &thighLeftMat, rage::Matrix34 &thighRightMat,
      float *legSeparation, float *legStraightness, float *charlieChapliness, float *hipYaw, float *headYaw, float *defaultHipPitch);

    void initMaxAngSpeed(float multiplier = 1.f);

    /**
     * manage constraint between character's hands (mainly for gun pointing)
     */
      rage::phConstraintHandle m_handToHandConstraintHandle;
    bool isSupportHandConstraintActive();
    void constrainSupportHand(
      const NmRsHumanArm *primaryArm, 
      const NmRsHumanArm *supportArm,
      rage::Vector3 *constraintPosition, 
      const float minimumConstraintDistance,
      float constraintLengthReduction,
      float constraintStrength,
      bool distanceConstraint);
    void releaseSupportHandConstraint();

    /**
     * manage constraint between a gun and a hand
     */
    rage::phConstraintHandle m_gunToHandConstraintHandle[2];

    void ReleaseConstraintSafely(rage::phConstraintHandle &constraint);

    void snap(
      float snapMag, 
      float snapDirectionRandomness,
      int snapHipType,
      bool snapLeftArm, 
      bool snapRightArm, 
      bool snapLeftLeg, 
      bool snapRightLeg, 
      bool snapSpine, 
      bool snapNeck,
      bool snapPhasedLegs,
      bool snapUseTorques,
      float mult,
      int bodyPart,//if >0 snap this part only

      rage::Vector3 *snapDirection = NULL,
      float movingMult = 1.0f,
      float balancingMult = 1.0f,
      float airborneMult = 1.0f,
      float movingThresh = 1.0f);

      rage::Vector3 getRandomVec(rage::Vector3 &startVec, float deviation);
      void changeJointAngVel(NmRs3DofEffector* effector, const rage::Vector3 &angVel, float parentWeight);
      void changeJointAngVel(NmRs1DofEffector* effector, float angVel, float parentWeight);
      void controlStiffness(
        NmRsHumanBody& body,
        float timeStep,
        //globals
        float controlStiffnessStrengthScale,//modified
        //inputs
        float spineStiffness,//= m_parameters.bodyStiffness;
        float armsStiffness,//= m_parameters.armStiffness
        float armsDamping,
        float spineDamping,//= m_parameters.spineDamping
        float neckStiffness,
        float neckDamping,

        float upperBodyStiffness,
        float lowerBodyStiffness,

        //shot only inputs
        bool injuredLArm,
        bool injuredRArm,
        //shot params
        float loosenessAmount,
        float kMultOnLoose,
        float kMult4Legs,
        float minLegsLooseness,
        float minArmsLooseness,
        //shot only params
        bool bulletProofVest,
        bool allowInjuredArm,
        bool stableHandsAndNeck
      );

#if NM_STAGGERSHOT
    float m_spineStrengthScale;
    float m_spineDampingScale;
    float m_spineMuscleStiffness;
#endif

#if NM_RIGID_BODY_BULLET
    rage::Matrix34 m_characterInertiaAboutPivotInN;
    rage::Matrix34 m_characterInertiaAboutComInN;
    rage::Matrix34 m_bpInertiaMatrixAboutBpInN[21];
#endif
    rage::Matrix34 m_COMTM;
    rage::Vector3 m_COM, m_COMvel, m_COMrotvel, m_COMvelRelative, m_angMom;
    rage::Vector3 m_gUp, m_gUpReal;
    rage::Vector3 m_floorVelocity;
    rage::Vector3 m_floorAcceleration;//unused at the moment except for bSpy output but plan to have this in character rather than balancer
    rage::Vector3 m_groundNormal;
    float m_COMvelMag, m_COMrotvelMag, m_COMvelRelativeMag;
    float m_footSlipMult;

    //Constraints
    bool handCuffs;
    bool handCuffsBehindBack;
    bool legCuffs;
    bool rightDominant;
    int passiveMode;
    bool bespokeBehaviour;
    float blend2ZeroPose;

    // state variables for the foot slip compensation
    float m_fscLeftTwist,
      m_fscRightTwist;

    // [0..1] indicates character's overall strength on the dead-granny-to-healthy-terminator scale
    // used for modifying the character's reaction when being shot etc...
    float m_strength;
    // [0..1] indicates character's overall health on the dead-alive scale
    // used for modifying the character's catchFall/response to falling from dynamicBalancer fail
    float m_health;

    float m_depthFudge;
    float m_depthFudgeStagger;
    float m_footFriction;
    float m_footFrictionStagger;
    float m_minImpactFriction;
    float m_maxImpactFriction;
    bool m_applyMinMaxFriction;

    float m_viscosity;
    float m_stroke;
    bool m_linearStroke;
    bool m_underwater;
    bool m_movingFloor;

    float m_collision_spin;
    float m_collision_maxVelocity;
    int m_collision_vehicleClass;
    bool m_collision_applyToAll;
    bool m_collision_applyToSpine;
    bool m_collision_applyToThighs;
    bool m_collision_applyToClavicles;
    bool m_collision_applyToUpperArms;
    bool m_collision_footSlip;
    bool m_collision_withVehicle;

	// TODO there is a lot of repetition below. Consider a struct with the
	// common data.
    bool m_rememberSetStiffness;
    float m_rememberStiff;
    float m_rememberDamp;
    unsigned int m_rememberStiffnessMask;
    int m_rememberStiffnessPriority;
	float m_rememberStiffnessBlend;

    bool m_rememberSetMuscleStiffness;
    float m_rememberMuscleStiff;
    unsigned int m_rememberMuscleStiffnessMask;
    int m_rememberMuscleStiffnessPriority;
	float m_rememberMuscleStiffnessBlend;

    inline void setUnderwater(bool underwater) {m_underwater = underwater;}
    inline void setViscosity(float viscosity) {m_viscosity = viscosity;}
    inline void setStroke(float stroke) {m_stroke = stroke;}
    inline void setLinearStroke(bool linearStroke) {m_linearStroke = linearStroke;}
    inline void resetVehicleHit() {m_glancingVehicleHit = false;}
    inline void setVehicleHit(bool gsScale1Foot, float gsFricScale1, BehaviourMask gsFricMask1,float gsFricScale2, BehaviourMask gsFricMask2) 
      {m_glancingVehicleHit = true; m_gsScale1Foot = gsScale1Foot; m_gsFricScale1 = gsFricScale1; m_gsFricMask1 = gsFricMask1; m_gsFricScale2 = gsFricScale2; m_gsFricMask2 = gsFricMask2; }

	void ResetSavedVelocities();
	void AddDeferredImpulse(int link, rage::Vec3V_In trans, rage::Vec3V_In omega);
	void ApplyAccumulatedImpulses();

	static int sm_ImpulsePendingForLink[MAX_NUM_LINKS];
	static bool sm_ApplyForcesImmediately;

#if NM_CHECK_VALID_VALUES
	static bool CheckValidVector(const rage::Vector3 &vector, int checkID = -1);
	static bool CheckValidFloat(const float f, float minSize = -5000.0f, float maxSize = 5000.0f, int checkID = -1);
#endif

#define NUM_OF_PATCHES 10
#define NUM_OF_BULLETS 17
    friend class BulletApplier;
    BulletApplier m_bulletApplier[NUM_OF_BULLETS];
    int m_currentBulletApplier;
    float m_impulseReductionPerShot;// by how much are subsequent impulses reduced (e.g. 0.0: no reduction, 0.1: 10% reduction each new hit)
    float m_impulseRecovery;// recovery rate of impulse strength per second
    float m_impulseLeakage;// determines how much impulse is "leaked"
    float m_lastImpulseMultiplier;// Store the current impulse multiplier being used in case the character dies and is switched to a rage ragdoll
    bool m_impulseLeakageStrengthScaled;// for weaker characters subsequent impulses remain strong

    // upright constraint related variables
    struct UprightConstraint
    {
      bool
        forceActive,
        torqueActive,
        lastStandMode,
        velocityBased,
        torqueOnlyInAir,
        turnTowardsBullets;

      float
        lastStandSinkRate,
        lastStandHorizDamping,
        lastStandMaxTime,
        forceStrength,
        forceDamping,
        forceFeetMult,
        forceSpine3Share,
        forceLeanReduction,
        forceInAirShare,
        forceMin,
        forceMax,
        forceSaturationVel,
        forceThresholdVel,
        torqueStrength,
        torqueDamping,
        torqueSaturationVel,
        torqueThresholdVel,
        supportPosition,
        noSupportForceMult,
        stepUpHelp,
        stayUpAcc,
        stayUpAccMax;

      rage::Vector3 
        m_uprightPelvisPosition,
        m_uprightSpine2Position,
        m_uprightSpine3Position;
    } m_uprightConstraint;

    struct Posture
    {
      float damping;
      float clampLimit;
      int   alternateRoot;

      bool leftHandConnected;
      bool rightHandConnected;
      bool leftFootConnected;
      bool rightFootConnected;
      bool leftArmAutoSet;//autoSet based on impacts?
      bool rightArmAutoSet;//autoSet based on impacts?
      bool leftLegAutoSet;//autoSet based on impacts?
      bool rightLegAutoSet;//autoSet based on impacts?
      //mmmmTODO add in tech for pin and line constraints
      //rage::Vector3 leftArmPinPosition;
      //rage::Vector3 rightArmPinPosition;
      //rage::Vector3 leftLegPinPosition;
      //rage::Vector3 rightLegPinPosition;

      bool useZMP;
      void init(){ damping = 0.1f; leftLegAutoSet = true; rightLegAutoSet = true; leftArmAutoSet = false; rightArmAutoSet = false; 
      useZMP = true; clampLimit = 0.2f; alternateRoot = -1; 
      leftHandConnected = false; rightHandConnected = false; leftFootConnected = false; rightFootConnected = false;}
    } m_posture;

    struct AttachedObject
    {
      rage::Vector3 worldCOMPos;
      int partIndex;
      float mass;
      float massMultiplier;
      int levelIndex;
      //bool isColliding;//mmmmUnused
    } m_attachedObject;

    struct SpinDamping
    {
      float somersaultThresh;
      float somersaultDamp;
      float cartwheelThresh;
      float cartwheelDamp;
      float vehicleCollisionTime;
      bool v2;
      //variables
      float vehicleCollisionTimer;
      float somersaultDamping;
      float cartwheelDamping;
    } m_spinDamping;

    //struct AttachedWeapon : public AttachedObject
    //{
    //  float extraLean1;
    //  float extraLean2;
    //};

    /**
     * called to update attached object information
     */
    void updateAttachedObject(AttachedObject* object);

    // attached weapons
    WeaponMode m_weaponMode;
    BehaviourMask m_twoHandedWeaponMask;
    AttachedObject m_leftHandWeapon;
    AttachedObject m_rightHandWeapon;
    bool m_weaponModeChanged;

    // cached hand bounds
#if NM_SET_WEAPON_BOUND
	rage::pgRef<rage::phBound> m_handBoundCache[2];
#endif
    rage::Matrix34 m_gunToHandCurrent[2];
    rage::Matrix34 m_gunToHandAiming[2];
    bool m_registerWeaponCalled;

#if NM_SET_WEAPON_MASS
    float m_handMassCache[2];
#endif

    float m_minMuscleDamping; // natural damping of the joints
    unsigned int m_probeTypeIncludeFlags;
    unsigned int m_probeTypeExcludeFlags;

    bool m_zUp;
    bool m_calculateInertias; // do you calculate these before the simulation loop.

    // phase2 todo make body generic!
    inline NmRsHumanBody* getBody() { return &m_body; }
    NmRsHumanBody                 m_body;
    NmRsLimb*                     m_limbs[kNumNmRsHumanLimbs];

#if NM_EA
    struct Vector3LandG
    {
      rage::Vector3 local;
      rage::Vector3 global;
      void Zero(){local.Zero();global.Zero();};
    };

#if NM_EA_DEVEL
    //--------------------------------------------------------------------------------------------------
    // Declaration from 'Environment.types'
    // Data Payload: 112 Bytes
    // Alignment: 16
    struct State
    {

      rage::Vector3 boxCentre;                  /* (Position) */
      rage::Vector3 position;                   /* (Position) */
      rage::Vector3 velocity;                   /* (Velocity) */
      rage::Vector3 angularVelocity;            /* (AngularVelocity) */
      rage::Vector3 acceleration;               /* (Acceleration) */
      rage::Vector3 extents;                    /* (Extents) */
      float accSqr;
      float mass;                              /* (Mass) */
      int32_t id;

      // functions
      //State();
      rage::Vector3 getInstantVelocityAtPoint(const rage::Vector3& point) const;
      rage::Vector3 getVelocityAtPoint(const rage::Vector3& point) const;
      rage::Vector3 getAccelerationAtPoint(const rage::Vector3& point) const;
      //void adjustPathForAngularVelocity(SphereTrajectory& path) const;
      //bool getTrajectoryOverlapTimes(const SphereTrajectory& path, float& t1, float& t2);
      //float sphereTrajectoryCollides(const SphereTrajectory& path, bool forwards);
      //bool boundingBoxCull(const SphereTrajectory& path) const;
      //void debugDraw(const rage::Vector3& startColour, const rage::Vector3& endColour);
      rage::Vector3 rotateNormal(const rage::Vector3& normal, float time) const;
      rage::Vector3 getAverageAngularVel() const;
      rage::Vector3 getAveragedPointVel(const rage::Vector3& point, float timePeriod) const;
      bool isStatic() const;
    }; // struct State
#endif //#if NM_EA_DEVEL




    //--------------------------------------------------------------------------------------------------
    // Declaration from 'Environment.types'
    // Data Payload: 224 Bytes
    // Alignment: 16
    struct Patch
    {

      enum EOType
      {
        /* -1 */ EO_None = -1,
        /*  0 */ EO_UseGameGeometry,
        /*  1 */ EO_Point,
        /*  2 */ EO_Line,
        /*  3 */ EO_Corner,
        /*  4 */ EO_Edge,
        /*  5 */ EO_Plane,
        /*  6 */ EO_Disc,
        /*  7 */ EO_Capsule,
        /*  8 */ EO_Sphere,
        /*  9 */ EO_Box,
        /*  10*/ EO_ContactPlane,
        /*  11*/ EO_Max,
      };
      bool UseGameGeometry;
#if NM_EA_DEVEL
      /*Environment::*/State state; // 112 Bytes 
#endif//#if NM_EA_DEVEL
      Vector3LandG corner;                     /* (Position) */
      Vector3LandG faceNormals[3];             /* (PositionDelta) */
      Vector3LandG knownContactPoint;          /* (Position) */
      Vector3LandG knownContactPointFromCollision[3];          
      Vector3LandG knownContactNormalFromCollision[3];  

      float edgeLengths[3];                    /* (Length) */
      int numKnownEdgeLengths;
      float radius;                            /* (Length) */
      int type;
      int instLevelIndex;
      int boundIndex;
      bool inputIsLocal;

      // functions
      void Init();
#if NM_EA_DEVEL
      //bool sphereTrajectoryCollides(const SphereTrajectory& path, const rage::Vector3& lastNormal, /*Environment::*/CollideResult& result, bool adjustForAngularVelocity=bool(true)) const;
      //int32_t nearestPoint(const rage::Vector3& pointWorld, rage::Vector3& nearestPoint, bool getSurfacePoint=0) const;
      //void updateFromSweepResult(const ER::SweepResult& sweep);
      //bool isConnectedTo(const /*Environment::*/Patch& object, const /*Environment::*/LocalShape& shape, const /*Environment::*/LocalShape& objectShape, float epsilon=0.08f);
      void update(float timeStep);
      //void debugDraw(float size=0.5f) const;
      /*int32_t getEdgeDirections(rage::Vector3* edgeDirections) const;
      int32_t getNormal1Index(int32_t edgeIndex) const;
      int32_t getNormal2Index(int32_t edgeIndex) const;
      float getReliabilityOfPosition(const rage::Vector3& position, const rage::Vector3& pathPosition, bool clipped) const;
      void createAsPlane(const rage::Vector3& position, const rage::Vector3& normal, float radius, const rage::Vector3& velocity, const rage::Vector3& angularVelocity, float mass, int32_t id);
      void createAsSphere(const rage::Vector3& position, float radius, const rage::Vector3& velocity, float mass, int32_t id);*/
#endif//#if NM_EA_DEVEL
      void nearestPoint(const rage::Vector3 &pointWorld, rage::Vector3 &nearestPoint) const;
      void nearestPoint_Line(const rage::Vector3 &pointWorld, rage::Vector3 &nearestPoint) const;
      void nearestPoint_Disc(const rage::Vector3 &pointWorld, rage::Vector3 &nearestPoint) const;
      void nearestPoint_Plane(const rage::Vector3 &pointLocal, rage::Vector3 &nearestPoint) const;
      void getNearestPointOnLineSegment(const rage::Vector3 &point, rage::Vector3 &p0, rage::Vector3 &p1, float* mag2, rage::Vector3 *nearestPoint) const; 

    private:



#if NM_EA_DEVEL
      // functions
      bool nearestPointInternal(const rage::Vector3& point, rage::Vector3& nearestPoint, rage::Vector3* edgeTangents, bool getSurfacePoint=0) const;
      void getEdgeTangents(rage::Vector3* edgeTangents) const;
#endif//#if NM_EA_DEVEL
    }; // struct Patch

    Patch m_patches[NUM_OF_PATCHES];
    int m_currentPatch;
#if NM_EA_TEST_FROM_IMPACTS
    //add from contacts
    void Patch_Add(rage::phInst* inst, int boundIndex, bool localInput, const rage::Vector3 &pos, const rage::Vector3 &normal);
#endif//#if NM_EA_TEST_FROM_IMPACTS

    //add from message
    void Patch_Add(int geomType, 
      unsigned int action, 
      int instanceIndex, 
      int boundIndex, 
      const rage::Vector3 &corner, 
      const rage::Vector3 &faceNormal0,
      const rage::Vector3 &faceNormal1,
      const rage::Vector3 &faceNormal2,
      const rage::Vector3 &edgeLengths,
      float edgeRadius,
      bool localVectors);
    void Patch_Cull(int i);

#if ART_ENABLE_BSPY 
    void PatchSendUpdate(int index, NmRsSpy& spy);
#endif
#endif//#if NM_EA


    // physics helper tools - implemented in NmRsCharacterPhysicsHelpers.cpp
    // ------------------------------------------------------------------------------------------------
    //Below are for gravityCompensation
    void setLeftHandConnected(bool lArmConnected); 
    void setRightHandConnected(bool rArmConnected); 
    void setLeftFootConnected(bool lLegConnected); 
    void setRightFootConnected(bool rLegConnected);

    void applyInjuryMask(BehaviourMask mask, float injuryAmount = 0.f);

  private:
    // needed in character deserialize. phase2 todo review useage - may not need them after all.
    void setBodyStiffness(float stiffness, float damping, BehaviourMask mask = 0, float *muscleStiffness = NULL);
    void resetEffectorsToDefaults(BehaviourMask mask = bvmask_Full);

  public:
    void getInstMatrix(int instance, rage::Matrix34 *matrix);
    // PURPOSE: Get the world velocity (velocityInWorld) at a particular world point (posInWorld) on the given object with instanceIndex.
    void getVelocityOnInstance(int instanceIndex, rage::Vector3 &posInWorld, rage::Vector3 *velocityInWorld);

    bool IsOnGround();

    void stayUprightByComTorques(float stiffness, float damping);
    void applyUprightConstraintForces(float mult, float stiffness, float damping, float feetMult = 1.0, float leanReduction = 1.0, float inAirShare = 0.5, float min = -1, float max = -1);
    void applyLastStandUprightConstraintForces(float deltaTime);
    void turnTowardBullets(float deltaTime);
#if NM_STEP_UP
    void stepUpHelp();

#else
#define RecordFeetFrictionMult(_footFrictionMult);
#define RecordLeftFootFrictionMult(_footFrictionMult);
#define RecordRightFootFrictionMult(_footFrictionMult);
#endif//NM_STEP_UP

    void instanceToWorldSpace(rage::Vector3 *vecInWorld_Out, const rage::Vector3 &vecInInstance_In, int instanceIndex);
    void instanceToLocalSpace(rage::Vector3 *vecInInstance_Out, const rage::Vector3 &vecInWorld_In, int instanceIndex);
    void rotateInstanceToWorldSpace(rage::Vector3 *dest, rage::Vector3 &vec, int instanceIndex);

    void boundToWorldSpace(rage::Vector3 *dest, const rage::Vector3 &vec, int instanceIndex, int boundIndex);
    void rotateBoundToWorldSpace(rage::Vector3 *dest, const rage::Vector3 &vec, int instanceIndex, int boundIndex);
    void boundToLocalSpace(bool isDirection, rage::Vector3 *dest, const rage::Vector3 &vec, int instanceIndex, int boundIndex);

    rage::Vector3 targetPosition(rage::Vector3 &targetPos, rage::Vector3 &targetVel, rage::Vector3 &sourcePos, rage::Vector3 &sourceVel, rage::Vector3 &sourceAngVel, float deltaTime);
    void getNeckAngles(float *neckLowerAngle, float *neckUpperAngle, float angle, float neckLowerAngleMin, float neckLowerAngleMax, float neckUpperAngleMin, float neckUpperAngleMax);

#if NM_USE_IK_SELF_AVOIDANCE
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // BBDD Self avoidance tech
    struct SelfAvoidance
    {
      bool useSelfAvoidance;
      bool usePolarPathAlgorithm;
      bool overwriteDragReduction;

      struct PolarSelfAvoidanceParams
      {
        float radius;

        PolarSelfAvoidanceParams()
          : radius(0.3f)
        {}
      } m_polarSelfAvoidanceParams;

      struct SelfAvoidanceParams
      {
        float torsoSwingFraction;
        float maxTorsoSwingAngleRad;
        bool selfAvoidIfInSpineBoundsOnly;
        float selfAvoidAmount;
        bool overwriteTwist;

        SelfAvoidanceParams()
          : torsoSwingFraction(0.75f),
          maxTorsoSwingAngleRad(PI / 4.0f),
          selfAvoidIfInSpineBoundsOnly(false),
          selfAvoidAmount(0.3f),
          overwriteTwist(true)
        {}
      } m_selfAvoidanceParams;

      SelfAvoidance()
        : useSelfAvoidance(false),
        usePolarPathAlgorithm(false),
        overwriteDragReduction(false)
      {}
    } m_selfAvoidance;
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
#endif // NM_USE_IK_SELF_AVOIDANCE

    rage::Vector3 BraceArm(
      NmRsHumanLimbTypes limbType,
      NmRsHumanBody* body,
      NmRs3DofEffector* wrist,
      rage::Vector3 &desiredReachTarget,// = armsBracePos - rightOffset*direction*m_leftHandSeparation/2.f;
      rage::Vector3 &desiredHandTarget,// = m_target - rightOffset*direction*m_leftHandSeparation/2.f;
      float maxArmLength,
      float armTwist
      );

    // Given a brace target, decides whether character can brace for this target.
    // doBrace is true if *either* hand could brace.
    void DecideToBrace(
      float timeStep,
      const rage::Vector3& target,
      const rage::Vector3& targetVel,
      const float braceDistance,
      const float targetPredictionTime,
      const float minBraceTime,
      const float timeToBackwardsBrace,
      float& distanceToTarget,
      float& braceTime,
      float& backwardsBraceTimer,
      bool& shouldBrace,
      bool& doBrace,
      bool* overrideBraceDecision = NULL,
      const bool allowTimedBrace = true);

    // Decide which hands can brace for this target.
    // braceLeft and braceRight indicate which arm can brace.
    void DecideBraceHands(
      float timeStep,
      const rage::Vector3& target,
      const bool doBrace,
      bool& braceLeft,
      bool& braceRight,
      float& braceTime,
      float& handsDelay,
      const float handsDelayMin,
      const float handsDelayMax,
      bool& delayLeftHand,
      float& leftHandSeparation,
      float& rightHandSeparation);
    
    // Move arms to brace.
    void ArmsBrace(
      const rage::Vector3& target,
      const rage::Vector3& targetVel,
      const float reachAbsorbtionTime,
      const float braceDistance,
      const float armStiffness,
      const bool braceLeft,
      const bool braceRight,
      const float leftHandSeparation,
      const float rightHandSeparation,
      NmRsHumanBody* body,
      rage::Vector3& leftHandPos, 
      rage::Vector3& rightHandPos);
    
    //Reach arm to body part
    //returns m_armTwist and sets reachArm->dist2Target
    float reachForBodyPart(
      NmRsHumanBody* body,
      ReachArm* reachArm,
      bool delayMovement,
      float cleverIKStrength,
      bool useActualAnglesForWrist,
      float bustElbowLift,
      bool bust);
    float blendToSpecifiedPose(float oldDesiredAngle, float specifiedPoseAngle, float blendFactor);

    float getTotalMass() const;
    float getUpperBodyMass() const;
    float getLowerBodyMass() const;

    bool probeRay(rayProbeIndex index, const rage::Vector3 &startPos, const rage::Vector3 &endPos, rage::u8 stateIncludeFlags = rage::phLevelBase::STATE_FLAGS_ALL, unsigned int typeFlags = TYPE_FLAGS_ALL, unsigned int typeIncludeFlags = TYPE_FLAGS_ALL, unsigned int typeExcludeFlags = TYPE_FLAGS_NONE, bool includeAgent = false);
    bool probeRayNow(rayProbeIndex index, const rage::Vector3 &startPos, const rage::Vector3 &endPos, rage::u8 stateIncludeFlags = rage::phLevelBase::STATE_FLAGS_ALL, unsigned int typeFlags = TYPE_FLAGS_ALL, unsigned int typeIncludeFlags = TYPE_FLAGS_ALL, unsigned int typeExcludeFlags = TYPE_FLAGS_NONE, bool includeAgent = false);

#define PROBE_RESULT(_action) \
    _action(probe_Hit) \
    _action(probe_NoHit) \
    _action(probe_Late) \
    _action(probe_NoIntersection) \
    _action(probe_NoManager) \
    _action(probe_HandleKnownInvalid) \
    _action(probe_HandleInvalid) \
    _action(probe_NotRequested) \
    _action(probe_RayIndexInvalid) /*this probe was submitted by another rayProbeIndex.  e.g. the result has been requested by pi_highFall but the probe was submitted by pi_catchFallLeft*/\
    _action(probe_1stIgnored) /*We are ignoring the probe result this time as we have only requested an asynchronous probe on this step so have to wait untill the next step before it has good information*/

    enum probeResult
    {
#define ENUM_ACTION(_name) _name,
      PROBE_RESULT(ENUM_ACTION)
#undef ENUM_ACTION
    };

#ifdef NM_RS_CBU_ASYNCH_PROBES
    void	SubmitAsynchProbe(rage::Vector3 &start, rage::Vector3 &end, rayProbeIndex rayPrbeIndex, rage::u8 stateIncludeFlags = rage::phLevelBase::STATE_FLAGS_ALL, unsigned int typeFlags = TYPE_FLAGS_ALL, unsigned int typeIncludeFlags = TYPE_FLAGS_ALL, unsigned int typeExcludeFlags = TYPE_FLAGS_NONE, bool includeAgent = false);
    probeResult	GetAsynchProbeResult(rayProbeIndex rayPrbeIndex, 
      rage::Vector3* probeHitPos, rage::Vector3* probeHitNormal, int* probeHitComponent,
      const rage::phInst** probeHitInst, int* probeHitInstLevelIndex, int* probeHitInstGenID);
#endif

    void applyRollDownStairForces(float comVelMag, bool useCustomRollDir, bool useVelocityOfObjectBelowCharacter, bool useRelativeVelocity,const rage::Vector3 &forwardVelVecInput, float currentSlope, float rollDownStairsForceMag, bool rollDownStairsSpinWhenInAir, float totalMass, float zAxisSpinReduction, float linearVelocityTarget, float airbornReduction);
    void applyTorqueToRoll(float minAngVel, float maxAngVel, float magOfTorque, const rage::Vector3 &forwardVelVec);
    void roll2Velocity(const rage::Vector3 &velocity, float rollRadius);

    void antiSpinAroundVerticalAxisJuice(float zAxisSpinReduction, bool useWorldUp = true);

    void applyExtraWristLean(rage::Matrix34& extraLeanTM, float extraLean1, float extraLean2, float extraTwist = 0.f);

    bool getIsInLevel(int levelIndex);

    void setFrictionPreScale(float mult, BehaviourMask mask = bvmask_Full);
    void setFrictionPostScale(float mult, BehaviourMask mask = bvmask_Full);
    void setStepUpFriction(float left, float right);

    void setElasticityMultiplier(float mult, BehaviourMask mask = bvmask_Full);

    // adjust the component of rVec that represents the height (defined by gUp)
    inline void levelVector(rage::Vector3& rVec, float height = 0.0f) const
    {
      float dot = m_gUp.Dot(rVec);
      rVec.AddScaled(rVec, m_gUp, (height - dot));
    }

    // adjust the component of rVec that represents the height (defined by gUp)
    inline void levelVectorReal(rage::Vector3& rVec, float height = 0.0f) const
    {
      float dot = m_gUpReal.Dot(rVec);
      rVec.AddScaled(rVec, m_gUpReal, (height - dot));
    }

    // return horizontal distance between two vectors, using gUp to factor out 'up' component
    inline float horizDistance(rage::Vector3::Vector3Param a, rage::Vector3::Vector3Param b) const
    {
      rage::Vector3 diff(b);
      diff.Subtract(a);

      float dot = m_gUp.Dot(diff);
      diff.AddScaled(diff, m_gUp, -dot);
      return diff.Mag();
    }

    inline float vectorHeight(rage::Vector3::Vector3Param vec) const
    {
      return m_gUp.Dot(vec);
    }
#if NM_SCRIPTING
    void readBlockedBehaviourMessages();
    void readScript();
#endif

    // -----------------------------------------------------------------------------------

    bool cleverHandIK(rage::Vector3 &target,NmRsGenericPart* endPart,float direction,
      bool useHardConstraint, float strength, rage::phConstraintHandle *constraint,
      NmRsGenericPart *woundPart,int instanceIndex, float threshold, int partIndex, bool distanceContraint = false);

    void constrainPart(rage::phConstraintHandle &con, int indexA, int indexB, float partSeparation, rage::Vector3 &posA, rage::Vector3 &posB, int inputlevelIndexB, bool increaseArmMass, bool distance, const float minSeperation);
    void reduceConstraintSeperation(rage::phConstraintHandle &conHandle, rage::Vector3* worldPosB, float constraintLengthReduction, const float minSeparation);
    void setConstraintBreakable(rage::phConstraintHandle &conHandle, bool breakable=true, float breakingStrength=10000.0f);
    void AlterArmMasses(NmRsHand iType, bool increase);

    bool hasCollidedWithWorld(BehaviourMask mask = bvmask_Full) { return (collidedWithWorld(mask) == bvmask_None ? false : true); } // WithNotOwnCharacter
    bool hasCollidedWithOtherCharacters(BehaviourMask mask = bvmask_Full) { return (collidedWithOtherCharacters(mask) == bvmask_None ? false : true); }
    bool hasCollidedWithEnvironment(BehaviourMask mask = bvmask_Full) { return (collidedWithEnvironment(mask) == bvmask_None ? false : true); }

    BehaviourMask collidedWithWorld(BehaviourMask mask = bvmask_Full); // WithNotOwnCharacter
    BehaviourMask collidedWithOtherCharacters(BehaviourMask mask = bvmask_Full);
    BehaviourMask collidedWithEnvironment(BehaviourMask mask = bvmask_Full);

#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
    void reEnableCollision(NmRsGenericPart* part);
    void updateReEnableCollision();
    bool m_needToCheckReenableCollisionForGrab;
#endif

    bool m_kineticEnergyPerKiloValid;
    float m_kineticEnergyPerKilo;
    float getKineticEnergyPerKilo_RelativeVelocity();

    void fluidDamping(NmRsGenericPart* part, float viscosity);

    bool getITMForPart(
      int partIndex,
      rage::Matrix34* tm,
      IncomingTransformSource source = kITSourceCurrent);
#if 0//Removed because currently unused
    void getJointMatrix1FromParent(rage::Matrix34& jointMat1,
      NmRsEffectorBase* effector,
      NmRsGenericPart* parent);
    void getJointMatrix2FromChild(rage::Matrix34& jointMat2,
      NmRsEffectorBase* effector,
      NmRsGenericPart* child);
#endif
    void pdPartOrientationToITM(
      NmRsGenericPart *part,
      float stiffness,
      float damping,
      rage::Vector3* velCache = NULL,
      float velSmoothing = 0);
    void pdPartPositionToITM(NmRsGenericPart *part, float stiffness, float damping);

    void pdPartToM(
      NmRsGenericPart *part,
      rage::Matrix34& tm,
      rage::Vector3* angVel,
      rage::Vector3* linVel,
      float oriStiffness,
      float oriDamping,
      float posStiffness,
      float posDamping);
    void pdPartOrientationToM(
      NmRsGenericPart *part,
      rage::Matrix34& tm,
      rage::Vector3* angVel,
      float stiffness,
      float damping,
      rage::Vector3* velCache = NULL,
      float velSmoothing = 0,
      float limit = PI);
    void pdPartPositionToM(
      NmRsGenericPart *part, 
      rage::Matrix34& tm, 
      rage::Vector3* vel, 
      float stiffness, 
      float damping,
      rage::Vector3* velCache = NULL,
      float velSmoothing = 0,
      rage::Vector3* localPos = NULL);
    void pdPartPosition(
      NmRsGenericPart *part,
      rage::Vector3& pos,
      rage::Vector3* vel,
      float stiffness,
      float damping,
      rage::Vector3* velCache = NULL,
      float velSmoothing = 0,
      rage::Vector3* localPos = NULL);

    // ------------------------------------------------------------------------------------------------
    void sendFeedbackSuccess(const char *name);
    void sendFeedbackFailure(const char *name);
    void sendFeedbackFinish(const char *name);
#if NM_UNUSED_CODE
    void sendFeedbackEvent(const char *name);
#endif
    //inline bool usingUprightConstraint(){ return m_uprightConstraintActive; };
    inline void configureUprightConstraint(bool enabled, float stiffness, float damping, float inAirShare = 0.5, float min = -1, float max = -1)
    {
      m_uprightConstraint.forceActive = enabled;
      m_uprightConstraint.forceStrength = stiffness;
      m_uprightConstraint.forceDamping = damping;
      m_uprightConstraint.forceInAirShare = inAirShare;
      m_uprightConstraint.forceMin = min;
      m_uprightConstraint.forceMax = max;
    }
    inline void configureUprightByComTorques(bool enabled, float stiffness, float damping)
    {
      m_uprightConstraint.torqueActive = enabled;
      m_uprightConstraint.torqueStrength = stiffness;
      m_uprightConstraint.torqueDamping = damping;
    }
    inline void setDontRegisterCollsion(float dontRegisterCollsionMassBelow, float dontRegisterCollsionVolBelow)
    {  
      m_dontRegisterCollsionActive = false;
      m_dontRegisterCollsionMassBelow = dontRegisterCollsionMassBelow;
      m_dontRegisterCollsionVolBelow = dontRegisterCollsionVolBelow;
      if ((m_dontRegisterCollsionMassBelow > 0.f) || (m_dontRegisterCollsionVolBelow > 0.f))
        m_dontRegisterCollsionActive = true;
    }

    inline void setDontRegisterProbeVelocity(float dontRegisterProbeVelocityMassBelow, float dontRegisterProbeVelocityVolBelow)
    {  
      m_dontRegisterProbeVelocityActive = false;
      m_dontRegisterProbeVelocityMassBelow = dontRegisterProbeVelocityMassBelow;
      m_dontRegisterProbeVelocityVolBelow = dontRegisterProbeVelocityVolBelow;
      if ((m_dontRegisterProbeVelocityMassBelow > 0.f) || (m_dontRegisterProbeVelocityVolBelow > 0.f))
        m_dontRegisterProbeVelocityActive = true;
    }

    //By default, the orientation of the foot is initialized only if (!foot->previousCollided)
    //if forceInitializeFootOrientation is true, initialization of the orientation of the foot
    //will be done even if foot->previousCollided
    inline void enableFootSlipCompensationActive(bool enable, bool forceInitializeFootOrientation = false) 
    {
      m_footSlipCompensationActive = enable; 
      if (forceInitializeFootOrientation || !enable) 
      {
        m_footLOrientationInitialized = false; 
        m_footROrientationInitialized = false;
      }
    }
    inline void enableZMPPostureControlActive(bool enable) { m_ZMPPostureControlActive = enable; }

#if ART_ENABLE_BSPY
    inline void setSkeletonVizMode(SkelVizMode svm) { m_skeletonVizMode = svm; }
    inline SkelVizMode getSkeletonVizMode() const { return m_skeletonVizMode; }
    inline void setSkeletonVizRoot(int root) { m_skeletonVizRoot = root; }
    inline void setSkeletonVizMask(BehaviourMask mask) { m_skeletonVizMask = mask; }

    // implemented in NmRsCharacterDebugViz.cpp
    void drawSkeleton(bool desiredSkeleton);
    void drawBone(const rage::Vector3& a, const rage::Vector3& b, const rage::Vector3& axis, const rage::Vector3& col, float finScale);
    bool bspyDebugDrawIsInhibited();
    void bspyDrawLine(const rage::Vector3& start, const rage::Vector3& end, const rage::Vector3& colour);
    void bspyDrawPoint(const rage::Vector3& position, float size, const rage::Vector3& colour);
    void bspyDrawCoordinateFrame(float size, const rage::Matrix34& tm);
    void bspyDrawTorque(float scale, const rage::Vector3& position, const rage::Vector3& torque, const rage::Vector3& colour);
    void bspyDrawCircle(const rage::Vector3& pos, const rage::Vector3& axis, float radius, const rage::Vector3& colour, unsigned int segments = 8);
#if NM_UNUSED_CODE
    void bspyDrawArc(const rage::Vector3& pos, const rage::Vector3& axis, const rage::Vector3& start, const float length, const rage::Vector3& colour, unsigned int segments /* = 8 */);
    void bspyDrawSphere(const rage::Vector3& pos, float radius, const rage::Vector3& colour, unsigned int segments = 8);
#endif
#endif

#if ART_ENABLE_BSPY
    void setCurrentBehaviour(BehaviourID bvid) { Assert(bvid >= bvid_Invalid && bvid < bvid_Count); m_currentBehaviour = bvid; }
    void setCurrentSubBehaviour(const char* name) { m_currentSubBehaviour = name; }
    BehaviourID m_currentBehaviour;
    const char * m_currentSubBehaviour;
    int m_currentFrame;
#endif


#if CRAWL_LEARNING
    inline void setLearningCrawl(bool learningCrawl)              { m_learningCrawl = learningCrawl; }
    inline bool getLearningCrawl()                          const { return m_learningCrawl; }
#endif

    bool hasValidBodyIdentifier() const { return (m_bodyIdent > notSpecified && m_bodyIdent < endOfBodyId); }
    bool isBiped() const { return m_bodyIdent > notSpecified && m_bodyIdent < rdrHorse; };
#if NM_UNUSED_CODE
    void  getInstanceTM(rage::Matrix34* matrix) const;
#endif
    rage::phBound* getArticulatedBound() const;
    rage::phArticulatedBody *getArticulatedBody() const;
    rage::phInst *getFirstInstance() const;
    float getLastKnownUpdateStep() const;
    float getLastKnownUpdateStepClamped() const;

private:
    inline NmRsEffectorBase *getEffector(int index);

public:
    inline const NmRsEffectorBase* getConstEffector(int index) const;

    // effector accessor exposed to support a small number of cases that need
    // to sidestep the limbs system (eg runtime limits). do not use this
    // unless you are sure of what you are doing!
    inline NmRsEffectorBase *getEffectorDirect(int index);

    inline int getNumberOfParts()                           const { return m_genericPartCount; }
    inline int getNumberOfEffectors()                       const { return m_effectorCount; }
    inline int getNumberOf1Dofs()                           const { return m_1dofCount; }
    inline int getNumberOf3Dofs()                           const { return m_3dofCount; }

    // simple accessors
    inline rage::phSimulator *getSimulator()                const { return m_simulator;  }
    inline rage::phLevelNew *getLevel()                     const { return m_level; }
    inline rage::mthRandom& getRandom()                           { return m_random; }
    inline rage::Vector3 getUpVector()                      const { return m_gUp; }
    inline ART::AgentID getID()                             const { return m_agentID; }
#if ART_ENABLE_BSPY
    ////mmmmBSPY3INTEGRATION Remove getBSpyID() and replace with getID()
    inline ART::AgentID getBSpyID()                         const { return m_agentID; }
#endif//ART_ENABLE_BSPY
    inline const NmRsEngine* getEngine()                    const { return m_rsEngine; }
    inline NmRsEngine* getEngineNotConst()                        { return m_rsEngine; }
    inline NmRsArticulatedWrapper *getArticulatedWrapper()  const { return m_articulatedWrapper; }
    inline const CharacterConfiguration& getCharacterConfiguration() const { return m_characterConfig; }
    inline float getDontRegisterProbeVelocityMassBelow()    const { return m_dontRegisterProbeVelocityMassBelow;}
    inline float getDontRegisterProbeVelocityVolBelow()     const { return m_dontRegisterProbeVelocityVolBelow;}
    inline bool getDontRegisterProbeVelocityActive()        const { return m_dontRegisterProbeVelocityActive;}
    inline void setMovingFloor(bool movingFloor)                  { m_movingFloor = movingFloor; }
    inline rage::Vector3 getFloorVelocity()                 const { return m_floorVelocity; }
    void setFloorVelocityFromColliderRefFrameVel();

    BodyIdentifier getBodyIdentifier() const { return m_bodyIdent; }//is enum
    AssetID getAssetID() const;
    void setIdentifier(const char* ident);//is string
    const char* getIdentifier() const;//is string

    class NmRsLimbManager* getLimbManager() { return m_limbManager; }

    class CBUTaskBase *getTask(int bvid);
    class NmRsCBUTaskManager *getTaskManager() { return m_cbuTaskManager; }
    void deactivateTask(int bvid);

    /*
     * Register attached weapon for collision exception, com calculation and
     * gravity opposition. hand = 0 : left hand, hand = 1 : right hand
     *   Information is also used by pointGun.
     */
    void unregisterWeapon(int hand); 
    void registerWeapon(int hand, int levelIndex, 
      rage::phConstraintHandle *gunToHandConstraint,
      const rage::Matrix34& gunToHand, 
      const rage::Vector3& gunToMuzzleInGun, 
      const rage::Vector3& gunToButtInGun);  
    void registerWeapon(int hand, int levelIndex, float extraLean1, float extraLean2);
    void getAttachedWeaponIndices(int& leftHandWeaponIndex, int& rightHandWeaponIndex) { leftHandWeaponIndex = m_leftHandWeapon.levelIndex; rightHandWeaponIndex = m_rightHandWeapon.levelIndex; }
    void getAttachedWeapons(AttachedObject**leftHandWeapon, AttachedObject** rightHandWeapon)
    {
      *leftHandWeapon = &m_leftHandWeapon;
      *rightHandWeapon = &m_rightHandWeapon;
    }
    WeaponMode getWeaponMode() { return m_weaponMode; }
    void setWeaponMode(int weaponMode);
    bool weaponModeChanged() { return m_weaponModeChanged; }//mmmmtodo mmmmunused

    // mask to keep hands and weapons from colliding during point gun.
    struct CollisionExclusionData {
      void init() {
        a = bvmask_None;
        b = bvmask_None;
        bTarget = bvmask_None;
        colliding = bvmask_None;
      }
      // try to get bActual to match b, but don't disable exclusion for
      // parts that are currently colliding.
      void setB(BehaviourMask _b) { bTarget = _b; b = _b | colliding; }
      //update
      //b is union of bTarget and colliding(colliding = anything in bTarget that is colliding).
      //Colliding is then reset ready for the next wave of handleCollisions.
      void update() { b = bTarget | colliding; colliding = bvmask_None;}//Happens in character->poststep
      BehaviourMask a;          // Set of parts to ignore collisions with {b}
      BehaviourMask b;          // Set of parts to ignore collisions with {a}
      BehaviourMask bTarget;    // Target set of parts to ignore collisions with {a}
      BehaviourMask colliding;  // Parts (within b) that are colliding.
#if ART_ENABLE_BSPY
      void debug(ART::AgentID agentID, const char* name)
      {
        bspyScratchpad_Bitfield32(agentID, name, a);
        bspyScratchpad_Bitfield32(agentID, name, bTarget);
        bspyScratchpad_Bitfield32(agentID, name, colliding);
        bspyScratchpad_Bitfield32(agentID, name, b);
      }
#endif
    };
    CollisionExclusionData m_rightHandCollisionExclusion;
    CollisionExclusionData m_Leg2LegCollisionExclusion;

    void setHandBound(int /* hand */, rage::phBound* /* bound */);
#if NM_SET_WEAPON_BOUND
    void cacheHandBound(NmRsHand hand);
    void restoreHandBound(NmRsHand hand);
#endif

#if NM_SET_WEAPON_MASS
    void cacheHandMass(NmRsHand hand);
    void setHandMass(int hand, float mass, rage::Vector3* comOffset = 0);
    void restoreHandMass(NmRsHand hand);
#endif

    ARTFeedbackInterface* getFeedbackInterface();
    void setFeedbackInterface(ARTFeedbackInterface* iface) { m_feedbackInterface = iface; }
#if NM_UNUSED_CODE
    int feedbackBehaviourEvent(const char* name, int id);
#endif

#if ART_ENABLE_BSPY
    void sendDescriptor();
    void sendUpdate();
    void sendDirectInvoke(const char* msg, const MessageParamsBase* const params);

    void bSpyProcessInstanceOnContact(rage::phInst* inst, int collidingNMAgent);
    void bSpyProcessDynamicBoundOnContact(rage::phInst* inst, rage::phBound* bound, const rage::Matrix34& tm, int collidingNMAgent);
#endif // ART_ENABLE_BSPY


#ifdef NM_RS_CBU_ASYNCH_PROBES
    void InitializeProbe(rayProbeIndex rayPrbeIndex, bool useAsyncProbe1st = false);
    void ResetRayProbeIndex(rayProbeIndex rayPrbeIndex, bool useAsyncProbe1st = false);
    void ClearAsynchProbe_IfNotInUse(rayProbeIndex rayPrbeIndex);
#endif
    bool IsInstValid(const rage::phInst *inst, const int instGenID);
    bool IsInstValid(const rage::phInst *inst, const int instLevelIndex, const int instGenID);
    bool IsInstValid(const int levelIndex, const int instGenID);
    bool IsInstValid(const rayProbeIndex rayPrbeIndex);
    bool IsInstValid_NoGenIDCheck(const rage::phInst *inst);
    bool IsInstValid_NoGenIDCheck(const int levelIndex);

#define PROBE_RESULT_TYPE(_action) \
    _action(probeType_Probe) \
    _action(probeType_OldProbe) \
    _action(probeType_Impact) \
    _action(probeType_OldImpact) \
    _action(probeType_None)

    enum probeResultType
    {
#define ENUM_ACTION(_name) _name,
      PROBE_RESULT_TYPE(ENUM_ACTION)
#undef ENUM_ACTION
    };

    const rage::phInst *m_probeHitInst[pi_probeCount];//used only to decide to get velocity information
    rage::Vector3 m_probeHitPos[pi_probeCount];
    rage::Vector3 m_probeHitNormal[pi_probeCount];
    rage::Vector3 m_probeHitInstBoundingBoxSize[pi_probeCount];//used only to decide to get velocity information
    //int m_probeHitComponent[pi_probeCount];//currently unused
    int m_probeHitLateFrames[pi_probeCount];
    int m_probeHitNoProbeInfo[pi_probeCount];
    int m_probeHitInstLevelIndex[pi_probeCount];//used only to get velocity information
    int m_probeHitInstGenID[pi_probeCount];//only used temporarily to decide if Inst is still valid
    float m_probeHitNoHitTime[pi_probeCount];
    float m_probeHitInstMass[pi_probeCount];//used only to decide to get velocity information
    probeResultType m_probeHitResultType[pi_probeCount];
    bool m_probeHit[pi_probeCount];
    bool m_useAsyncProbe1st[pi_probeCount];//If false then use a non-asynch probe for the 1st result.
    bool m_is1stAsyncProbe[pi_probeCount];

  protected:
    ART::ARTFeedbackInterface *simpleFeedback(const char *name);

#if ART_ENABLE_BSPY
    void copyFeedbackDataToLiveInstance();
    // ARTFeedbackInterface
    virtual int onBehaviourStart();
    virtual int onBehaviourFailure();
    virtual int onBehaviourSuccess();
    virtual int onBehaviourFinish();
    virtual int onBehaviourEvent();
    virtual int onBehaviourRequest();
#endif // ART_ENABLE_BSPY
    bool prepareFeedback(const char* name, int id);

    void findMatrixOfBound(rage::Matrix34 &tmMat, int instanceIndex, int boundIndex);
    void findMatrixOfCollider(rage::Matrix34 &tmMat, int instanceIndex, int colliderIndex);

    // used during deserialize to either look-up or allocate a new material
    rage::phMaterial& getMaterial(const char* name);

    /**
     * called to scale the friction of an impact
     */
    void setImpactFriction(rage::phContactIterator impact, const float scale, const float min, const float max);
    void scaleElasticity(rage::phContactIterator impact, const float scale);

#if NM_RIGID_BODY_BULLET
    void calculateCoM(
      rage::Matrix34* comResult,
      rage::Vector3* comVelResult,
      rage::Vector3* comAngVelResult,
      rage::Vector3* angMomResult); 

    void getCharacterInertiaAboutPivot(rage::Matrix34* characterInertiaAboutPivotInN, rage::Vector3* pivotPoint);
#else
    void calculateCoM(
      rage::Matrix34* comResult,
      rage::Vector3* comVelResult,
      rage::Vector3* comAngVelResult,
      rage::Vector3* angMomResult) const;
#endif

    // used by applyFootSlipCompensation
    //By default, the orientation of the foot is initialized only if (!foot->previousCollided)
    //if forceInitializeFootOrientation is true, initialization of the orientation of the foot
    //will be done even if foot->previousCollided
    float twistCompensation(NmRsGenericPart* foot, float startTwist, float twistStrength, float twistDamping, float mult);

    // This must be used to reset persistent data every time the agent is inserted into the scene.
    // called in the constructor and in addToScene.
    void initialiseData();

    // call after setting m_volumeCount, etc, in the deserializer to allocate space to store
    // pointers to NmRsGenericParts and so on
    void allocateStorage();

    // free up any allocated storage; m_bodies, m_joints, etc
    void deallocateStorage();

    // frees all objects built during a deserialize, called by dtor
    void freeAllocatedResources();

#ifdef NM_RS_CBU_ASYNCH_PROBES
    bool replaceWithImpactInfo(NmRsGenericPart *part, const rage::Vector3& probeStart, float *distanceToProbeStart, rage::Vector3 *collisionPos, 
      rage::Vector3 *collisionNormal, const rage::phInst **collisionInst, bool *replacedWithImpact, bool *collided,
      int *probeHitInstLevelIndex, int *probeHitInstGenID, float *probeHitInstMass, rage::Vector3& probeHitInstBoundingBoxSize);

    asyncProbeIndex	rayIndexToProbeIndex(rayProbeIndex index);
    void	ClearAsynchProbe(rayProbeIndex rayPrbeIndex);
    void	ClearAllProbes();
    static rage::phAsyncShapeTestMgr *GetAsyncShapeTestMgr() { return sm_AsyncShapeTestMgr; }
public:
    static void SetAsyncShapeTestMgr(rage::phAsyncShapeTestMgr *mgr) { sm_AsyncShapeTestMgr = mgr; }

protected:
    static rage::phAsyncShapeTestMgr* sm_AsyncShapeTestMgr;
    rage::New_phAsyncShapeTestHandle m_AsyncProbeHandles[api_probeCount];
    NmRsCharacter::rayProbeIndex m_AsyncProbeRayProbeIndex[api_probeCount];
#if ART_ENABLE_BSPY
    int m_AsyncProbeSubmitFrame[api_probeCount];
#endif
#endif // NM_RS_CBU_ASYNCH_PROBES
    //So that a car as well as the character can be excluded from the probe
    rage::phInst *m_excludeInstList[api_probeCount][2];

    // number of items stored in the character, read in during deserialize and used
    // to preallocate buffers to hold objects
    int                         m_genericPartCount,
      m_effectorCount,
      m_1dofCount,
      m_3dofCount,
      m_collisionPairCount;

    NmRsGenericPart           **m_parts;

    NmRsArticulatedWrapper     *m_articulatedWrapper;

    NmRsEffectorBase          **m_effectors;      ///< flat array of all effectors
    NmRs1DofEffector          **m_1dofEffectors;  ///< just 1-dof effectors
    NmRs3DofEffector          **m_3dofEffectors;  ///< just 1-dof effectors

    int                        *m_collisionPairs; ///< a list of part indices, where n + (n+1) form a collision pair

    rage::phSimulator          *m_simulator;      ///< RAGE simulation environment
    rage::phLevelNew           *m_level;          ///< RAGE physics level


    // incoming transforms
    // current mode used to apply incoming transforms to the character
    IncomingTransformApplyMode m_applyMode;

    // mask determines what parts are applied
    BehaviourMask m_applyMask;
    BehaviourMask m_lastApplyMask;

#if NM_FAST_COLLISION_CHECKING
    // set of masks to cache part collisions.
    // updated during pre-step.
    BehaviourMask m_collidedMask;
    BehaviourMask m_collidedEnvironmentMask;
    BehaviourMask m_collidedOtherCharactersMask;
    BehaviourMask m_collidedOwnCharacterMask;
#endif

    // moved from engine because the number of nm ticks per incoming
    // transform is variable in MP3. during Split Body, one transform
    // is supplied per nm tick. otherwise, we receive one transform
    // per game tick (there could be several nm ticks in this span)
  public:
    void setIncomingAnimationVelocityScale(float scale) { m_incomingAnimationVelocityScale = scale; }
    float getIncomingAnimationVelocityScale()           { return m_incomingAnimationVelocityScale; }
    // add others here if needed...
  protected:
    float                       m_incomingAnimationVelocityScale;

#if NM_SCRIPTING
    float m_simTime;
    float m_scriptTime;
    float m_nextScriptTime;
    bool m_readScript;
#endif
    // high-level parameters controlling character setup
    CharacterConfiguration      m_characterConfig;

    // mask code stack (pairs of mask codes)
    BehaviourMask               m_maskCodeStack[RS_MASKCODESTACK_SZ];
    int                         m_maskCodeStackIdx;

    // [0..1] multiplier for shot forces to reduce impact of machine gun bursts
    //float m_shotImpulseScaler;
    char                        m_identifier[8];

  private:

    BodyIdentifier              m_bodyIdent; //directly related to m_identifier
    AssetID                     m_asset;//Asset id fred=0,wilma=1, etc

#if ART_ENABLE_BSPY
    SkelVizMode                 m_skeletonVizMode;
    int                         m_skeletonVizRoot;
    BehaviourMask               m_skeletonVizMask;
#endif // ART_ENABLE_BSPY

    rage::mthRandom             m_random;       ///< random number generator for this character

    ART::AgentID                m_agentID;

    NmRsEngine                 *m_rsEngine;
    NmRsCBUTaskManager         *m_cbuTaskManager;
    NmRsLimbManager            *m_limbManager;
    CBURecord                  *m_cbuRecord;
    ART::MemoryManager         *m_artMemoryManager;
    ARTFeedbackInterface       *m_feedbackInterface;

    NmRsFrictionHelper         m_frictionHelper;

    rage::Matrix34             *m_incomingTm[ART::KITSourceCount];

    IncomingTransformStatus     m_incomingTmStatus;
    int                         m_incomingTmCount;

    rage::Matrix34 *m_WorldCurrentMatrices;
    rage::Matrix34 *m_WorldLastMatrices;
#if NM_ANIM_MATRICES
    rage::Matrix34 *m_BlendOutAnimationMatrices;
#endif

#if NM_RIGID_BODY_BULLET
    float m_upperBodyMass;
    float m_lowerBodyMass;
#endif

#if NM_STEP_UP
    //HACK FootFrictionMultiplier set by behaviours (in activate/tick/deactivate) 
    // so that anything outside of a behaviour that wants to change footFriction knows what to reset it back to
    // Used only in stepUp()
    float m_leftFootFrictionMult;
    float m_rightFootFrictionMult;
#endif

    bool m_LeftArmMassIncreased;
    bool m_RightArmMassIncreased;
#if NM_UPSIDEDOWN_FEET
    rage::Vector3 m_FootSizeCacheLeft;
    rage::Vector3 m_FootSizeCacheRight;
    bool m_leftFootUpsideDown;
    bool m_rightFootUpsideDown;
#endif
    //used by balancerCollisionsReaction behaviour to not register collisions with non-fixed small objects like guns
    float                       m_dontRegisterCollsionMassBelow;
    float                       m_dontRegisterCollsionVolBelow;
    bool                        m_dontRegisterCollsionActive;

    //used by catchFall and dynamicBalancer behaviour to not register probed object's velocity with non-fixed small objects like guns
    float                       m_dontRegisterProbeVelocityMassBelow;
    float                       m_dontRegisterProbeVelocityVolBelow;
    bool                        m_dontRegisterProbeVelocityActive;

    bool                        m_footSlipCompensationActive;
    bool                        m_forceInitializeFootROrientation;
    bool                        m_forceInitializeFootLOrientation;
    bool                        m_footROrientationInitialized;
    bool                        m_footLOrientationInitialized;

    bool                        m_ZMPPostureControlActive;

    bool                        m_isInsertedInScene;
#if CRAWL_LEARNING
    bool                        m_learningCrawl;
#endif
    float                       m_gsFricScale1;
    float                       m_gsFricScale2;
    BehaviourMask               m_gsFricMask1;
    BehaviourMask               m_gsFricMask2;
    bool                        m_gsScale1Foot;
    bool                        m_glancingVehicleHit;

#if NM_USE_1DOF_SOFT_LIMITS
  public:
    // NmRsHumanLimbTypes::kNumNmRsHumanLimbs-1 because the spine doesn't need a soft limit.
    SoftLimitController m_softLimitCtrls[kNumNmRsHumanLimbs-1];
#endif //NM_USE_1DOF_SOFT_LIMITS

  public:
    bool stopAllBehavioursSent() { bool temp = m_stopAllBehavioursSent; m_stopAllBehavioursSent = true; return temp; }
  private:
    bool m_stopAllBehavioursSent;
  };

  inline NmRsEffectorBase *NmRsCharacter::getEffector(int index)
  {
    FastAssert(index >= 0 && index<m_effectorCount);
    return m_effectors[index];
  }

  inline const NmRsEffectorBase* NmRsCharacter::getConstEffector(int index) const
  {
    FastAssert(index >= 0 && index<m_effectorCount);
    return m_effectors[index];
  }

  inline NmRsEffectorBase *NmRsCharacter::getEffectorDirect(int index)
  {
    return getEffector(index);
  }

  template <typename T>
#if ART_ENABLE_BSPY_LIMBS
  inline NmRsLimbInput NmRsBody::createNmRsLimbInput(int subPriority, float weight, BehaviourMask mask, const char* subTask)
#else
  inline NmRsLimbInput NmRsBody::createNmRsLimbInput(int subPriority, float weight, BehaviourMask mask)
#endif
  {
	  Assert(m_character);
	  Assert(m_character->getLimbManager());
    Assert(m_bodyInput);

	  // Make sure the specified blend weight is equal to or less than the body-
	  // wide weight.
	  if(weight > m_bodyInput->m_blend)
		  weight = m_bodyInput->m_blend;

#if ART_ENABLE_BSPY_LIMBS
	  const char* _subTask = 0; 
	  if(subTask)
		  _subTask = subTask;
	  else if(m_bodyInput->m_subTask)
		  _subTask = m_bodyInput->m_subTask;

	  return m_character->getLimbManager()->createNmRsLimbInput<T>(
		  m_bodyInput->m_priority,
		  subPriority,
		  m_bodyInput->m_mask & mask,
		  weight,
		  m_bodyInput->m_bvid,
		  _subTask);
#else
	  return m_character->getLimbManager()->createNmRsLimbInput<T>(
		  m_bodyInput->m_priority,
		  subPriority,
		  m_bodyInput->m_mask & mask,
		  weight);
#endif
  }
}
#endif // NM_RS_CHARACTER_H
