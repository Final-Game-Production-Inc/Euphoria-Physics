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


#include "NmRsInclude.h"
#include "NmRsCBU_Shared.h"

namespace ART
{
  // phase 2 todo refactor feedback messages to remove non-bspy dependency on
  // this list.
#if ART_ENABLE_BSPY | 1
  // add the task names into the profiler
  // This list should mirror the ordering of enum BehaviourID in
  // naturalmotion\src\rockstar\NmRsCBU_Shared.h
  const char* s_bvIDNames[bvid_NameCount] = 
  {
    "buoyancy",
    "dynamicBalancer",
    "bodyBalance",
    "braceForImpact",
#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
    "learnedCrawl",
#endif
    "bodyFoetal",
    "shot",
    "staggerFall",
    "teeter",
    "armsWindmill",
    "armsWindmillAdaptive",
    "balancerCollisionsReaction",            
    "spineTwist",
    "catchFall",
    "injuredOnGround",
    "carried",
    "dangle",
    "yanked",
#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
    "dragged",
#endif
    "bodyRollUp",
    "upperBodyFlinch",
    "fallOverWall",
    "highFall",
#if ALLOW_TRAINING_BEHAVIOURS & 0//Needs changing to limb system
    "landing",
#endif
    "rollDownStairs",
    "pedalLegs",
#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
    "stumble",
#endif
    "grab",
    "animPose",
    "onFire",
    "bodyWrithe",
    "pointGun",
    "headLook",
    "pointArm",
    "electrocute",
    "smartFall",
#if ALLOW_DEBUG_BEHAVIOURS
    //Debug only behaviours
    "debugRig",
#endif //ALLOW_DEBUG_BEHAVIOURS

    "invalid", // corresponds to bvid_Count

    "directInvoke"
  };

  const char* getBvidNameSafe(BehaviourID bvid)
  {
    if(bvid > bvid_Invalid && bvid <= bvid_DirectInvoke)
      return s_bvIDNames[bvid];

    return "default";
  }

  const char* s_phaseNames[kNumUpdatePhases] = 
  {
    "Invalid",
    "Activate",
    "Tick",
    "Deactivate"
  };
#endif
} // namespace ART

