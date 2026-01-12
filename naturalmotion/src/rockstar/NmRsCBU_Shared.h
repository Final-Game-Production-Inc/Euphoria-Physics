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

#ifndef NM_RS_CBU_SHARED_H
#define NM_RS_CBU_SHARED_H


#include "NmRsCBU_DBMCommon.h"

// define this to make CBUs use asynch probes; gives performance
// improvement on balancer and others that cast probes every frame
#define NM_RS_CBU_ASYNCH_PROBES


// macro for defining a variable and common accessors in the style
// get<cbu name><value name>() eg. getRollUpStiffness()
#define NM_RS_CBU_ACCESSOR(type_, cbuName_, name_)  \
    public:\
    inline void set##cbuName_##name_(type_ val) { m_##name_ = val; }\
    protected:\
    inline type_ get##cbuName_##name_() const { return m_##name_; }\
    type_ m_##name_;

#define NM_RS_CBU_ACCESSOR_TYPEREF(type_, typeByRef_, cbuName_, name_)  \
    public:\
    inline void set##cbuName_##name_(typeByRef_ val) { m_##name_ = val; }\
    protected:\
    inline type_ get##cbuName_##name_() const { return m_##name_; }\
    type_ m_##name_;

namespace ART
{
  //struct MemoryManager;

    struct CBURecord;

    // general shape test type
    typedef rage::phShapeTest<rage::phShapeProbe> RageShapeProbe;

    // the ordering in here is important, it defines task execution/update order
    // don't change it unless you know what you're doing; it is based on the HSM DB ordering.
    // If the order is changed also change the order of:
    //   const char* s_bvIDNames in naturalmotion\src\rockstar\NmRsCBU_Shared.cpp
    //   static char* getBvidNameSafe(bvid_Count) in naturalmotion\src\rockstar\NmRsCBU_TaskManager.cpp
    enum BehaviourID
    {
      bvid_Invalid = -1,

        bvid_buoyancy = 0,
        bvid_dynamicBalancer,
        bvid_bodyBalance,
        bvid_braceForImpact,
#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
        bvid_learnedCrawl,
#endif
        bvid_bodyFoetal,
        bvid_shot,
        bvid_staggerFall,
        bvid_teeter,
        bvid_armsWindmill,
        bvid_armsWindmillAdaptive,
        bvid_balancerCollisionsReaction,
        bvid_spineTwist,
        bvid_catchFall,
        bvid_injuredOnGround,
        bvid_carried,
		bvid_dangle,
        bvid_yanked,
#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
        bvid_dragged,
#endif
        bvid_bodyRollUp,
        bvid_upperBodyFlinch,
        bvid_fallOverWall,
        bvid_highFall,
#if ALLOW_TRAINING_BEHAVIOURS & 0//Needs changing to limb system
        bvid_landing,
#endif
        bvid_rollDownStairs,
        bvid_pedalLegs,
#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
        bvid_stumble,
#endif
        bvid_grab,
        bvid_animPose,
        bvid_onFire,
        bvid_bodyWrithe,
        bvid_pointGun,
		bvid_headLook,
        bvid_pointArm,
        bvid_electrocute,
        bvid_smartFall,
#if ALLOW_DEBUG_BEHAVIOURS
        //Debug only behaviours
        bvid_debugRig,
#endif //ALLOW_DEBUG_BEHAVIOURS

      bvid_Count,

      // tacking this on here to identify limb messages sent from direct invoke
      // without messing up bvid_Count
      bvid_DirectInvoke,

      bvid_NameCount
    };

#define USE_NEW_NM_RIG 1

    // This may need to be expanded to support
    // a greater number of body parts
    typedef unsigned int BehaviourMask;
    const BehaviourMask bvmask_Full = 0xffffffff;//"fb"
    const BehaviourMask bvmask_None = 0;
    // MP3-specific partial masks. Add more as needed.
    const BehaviourMask bvmask_Pelvis     = (1 << 0);
    const BehaviourMask bvmask_ThighLeft  = (1 << 1);
    const BehaviourMask bvmask_ShinLeft   = (1 << 2);
    const BehaviourMask bvmask_FootLeft   = (1 << 3);
    const BehaviourMask bvmask_ThighRight = (1 << 4);
    const BehaviourMask bvmask_ShinRight  = (1 << 5);
    const BehaviourMask bvmask_FootRight  = (1 << 6);
    const BehaviourMask bvmask_Spine0     = (1 << 7);
    const BehaviourMask bvmask_Spine1     = (1 << 8);
    const BehaviourMask bvmask_Spine2     = (1 << 9);
    const BehaviourMask bvmask_Spine3     = (1 << 10);
#if USE_NEW_NM_RIG
    const BehaviourMask bvmask_ClavicleLeft  = (1 << 11);
    const BehaviourMask bvmask_UpperArmLeft  = (1 << 12);
    const BehaviourMask bvmask_ForearmLeft  = (1 << 13);
    const BehaviourMask bvmask_HandLeft    = (1 << 14);
    const BehaviourMask bvmask_ClavicleRight= (1 << 15);
    const BehaviourMask bvmask_UpperArmRight= (1 << 16);
    const BehaviourMask bvmask_ForearmRight  = (1 << 17);
    const BehaviourMask bvmask_HandRight  = (1 << 18);
    const BehaviourMask bvmask_Neck      = (1 << 19);
    const BehaviourMask bvmask_Head      = (1 << 20);
#else
    const BehaviourMask bvmask_Neck      = (1 << 11);
    const BehaviourMask bvmask_Head      = (1 << 12);
    const BehaviourMask bvmask_ClavicleLeft  = (1 << 13);
    const BehaviourMask bvmask_UpperArmLeft  = (1 << 14);
    const BehaviourMask bvmask_ForearmLeft  = (1 << 15);
    const BehaviourMask bvmask_HandLeft    = (1 << 16);
    const BehaviourMask bvmask_ClavicleRight= (1 << 17);
    const BehaviourMask bvmask_UpperArmRight= (1 << 18);
    const BehaviourMask bvmask_ForearmRight  = (1 << 19);
    const BehaviourMask bvmask_HandRight  = (1 << 20);
#endif
    const BehaviourMask bvmask_CervicalSpine = (bvmask_Neck | bvmask_Head);
    const BehaviourMask bvmask_HighSpine  = (bvmask_Spine2 | bvmask_Spine3 | bvmask_Neck | bvmask_Head); 
    const BehaviourMask bvmask_Spine      = (bvmask_Pelvis | bvmask_Spine0 | bvmask_Spine1 | bvmask_Spine2 | bvmask_Spine3 | bvmask_Neck | bvmask_Head); 
    const BehaviourMask bvmask_JustSpine  = (bvmask_Spine0 | bvmask_Spine1 | bvmask_Spine2 | bvmask_Spine3);//"us"
    const BehaviourMask bvmask_LowSpine   = (bvmask_Spine & ~bvmask_CervicalSpine);
    const BehaviourMask bvmask_ArmLeft    = (bvmask_ClavicleLeft | bvmask_UpperArmLeft | bvmask_ForearmLeft | bvmask_HandLeft);//"ul"
    const BehaviourMask bvmask_ArmRight    = (bvmask_ClavicleRight | bvmask_UpperArmRight | bvmask_ForearmRight | bvmask_HandRight);//"ur"
    const BehaviourMask bvmask_Arms       = (bvmask_ArmLeft | bvmask_ArmRight);
    const BehaviourMask bvmask_LegLeft    = (bvmask_ThighLeft | bvmask_ShinLeft | bvmask_FootLeft);
    const BehaviourMask bvmask_LegRight    = (bvmask_ThighRight | bvmask_ShinRight | bvmask_FootRight);
    const BehaviourMask bvmask_Legs       = (bvmask_LegLeft | bvmask_LegRight);
    const BehaviourMask bvmask_Trunk      = (bvmask_Spine | bvmask_ClavicleLeft | bvmask_ClavicleRight);
    const BehaviourMask bvmask_UpperBody  = (bvmask_Spine | bvmask_ArmLeft | bvmask_ArmRight);
    const BehaviourMask bvmask_LowerBody  = (bvmask_Pelvis | bvmask_Legs);
    const BehaviourMask bvmask_UpperBodyExceptHands  = (bvmask_Spine | bvmask_ClavicleLeft | bvmask_UpperArmLeft | bvmask_ForearmLeft | bvmask_ClavicleRight | bvmask_UpperArmRight | bvmask_ForearmRight);
    const BehaviourMask bvmask_LowerBodyExceptFeet  = (bvmask_Pelvis | bvmask_ThighLeft | bvmask_ShinLeft| bvmask_ThighRight | bvmask_ShinRight);
    const BehaviourMask bvmask_BodyExceptHandsAndFeet  = (bvmask_UpperBodyExceptHands | bvmask_LowerBodyExceptFeet);

    enum CBUTaskReturn
    {
      eCBUTaskComplete,
      eCBUTaskMoreTicksRequired,
    };

//Needs to match definition in NmRsCBU_DBMCommon.h
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

    enum CBUTaskUpdatePhase
    {
      kInvalidPhase,
      kActivate,
      kTick,
      kDeactivate,
      kNumUpdatePhases
    };

    // phase2 todo make these bspy-only once feedback messages no longer need
    // to look up strings.
    extern const char* s_bvIDNames[bvid_NameCount];
    const char* getBvidNameSafe(BehaviourID bvid);
    extern const char* s_phaseNames[kNumUpdatePhases];

} // namespace ART

#if NM_RS_VALIDATE_VITAL_VALUES || ART_ENABLE_BSPY
# define NM_RS_VALIDATE_VAL(_v, _min, _max, _jointIndex, _setBy) { Assert(_v == _v); Assertf(_v <= _max, "NaturalMotion[j=%d, setBy %s]: " #_v " = %.3f (max = %.3f)", _jointIndex, _setBy, _v, _max); Assertf(_v >= _min, "NaturalMotion[j=%d,b=%s]: " #_v " = %.3f (min = %.3f)", _jointIndex, _setBy, _v, _min); }
# define NM_RS_VALIDATE_VAL_WARN(_v, _min, _max, _jointIndex, _setBy) { Assert(_v == _v); }
#else
# define NM_RS_VALIDATE_VAL(_v, _min, _max, _jointIndex, _behaviour)
# define NM_RS_VALIDATE_VAL_WARN(_v, _min, _max, _jointIndex, _behaviour)
#endif // NM_RS_VALIDATE_VITAL_VALUES

#endif // NM_RS_CBU_SHARED_H
