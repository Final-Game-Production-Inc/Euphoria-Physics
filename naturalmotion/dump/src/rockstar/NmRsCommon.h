/*
* Copyright (c) 2005-2010 NaturalMotion Ltd. All rights reserved. 
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

#ifndef NM_RS_COMMON_H
#define NM_RS_COMMON_H

/**
 * core defines follow - if you need to force debug draw / logging on
 * then only change these and it will configure the rest for you
 */

namespace ART 
{
  // TG - NM Integration 14 june 2010 - Moved this here from NmRsEngine.h so that other classes could
  // safely access it without compilation issues on the precompiled header creation.
  const int NUM_ASSETS = 5;//Number of NM assets the game uses
  const unsigned int MAX_BSPY_AGENTS = 24;//DO NOT EXCEED 31
} // namespace ART

#define ON_DEMAND_AGENT

#define NM_TEST_ENVIRONMENT 0//ALWAYS CHECK IN AS 0.  Set to 1 so that bankRelease can be used which is not __DEV but still loads the characters from a path that exists for the NM Test Environment drop

#if HACK_GTA4 // Integrating NM code drop (22/7/09)
#if __DEV
#define CRAWL_LEARNING 0
#define ALLOW_DEBUG_BEHAVIOURS 0
#define ALLOW_TRAINING_BEHAVIOURS 0
#else
#define CRAWL_LEARNING 0
#define ALLOW_DEBUG_BEHAVIOURS 0
#define ALLOW_TRAINING_BEHAVIOURS 0
#endif
#else //HACK_GTA4
#define CRAWL_LEARNING 0
#define ALLOW_DEBUG_BEHAVIOURS 0
#define ALLOW_TRAINING_BEHAVIOURS 0
#endif //HACK_GTA4

#define ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V 0

#define NM_UNUSED_CODE 0

#define NM_BULLETIMPULSEFIX 1

#define NM_CHECK_VALID_VALUES 1

#define USE_NEW_BALANCE_SUCCESS 0
#define DYNBAL_GIVEUP_RAMP 0

#define NM_TESTING_NEW_REGISTERWEAPON_MESSAGE 0
#define NM_SET_WEAPON_BOUND 0
#define NM_SET_WEAPON_MASS 0
#define NM_POINTGUN_RECOIL_IK 1 // Weapon distance and clavicle lean are driven based on recoil relax scale to produce recoil effect without having to weaken arms quite so much.
#define NM_POINTGUN_COLLISIONS_OFF 0 //mmmmtodo - this switch can be removed once R* checks that pointGun is not currently being called with disableArmCollisions =true or disableRifleCollisions=true otherwise the behaviour will change.  if 1 turn off collisions for the lower arm and hand - possible with spine3/2 if using rifle.  I prefer the collision left on otherwise gun points through body alot for rifle.
#define NM_NEW_LEG_COLLISION_CODE 1

#define NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING 0//mmmmRDR check

#define NM_USE_IK_SELF_AVOIDANCE 1 // Self avoidance tech;
#define NM_USE_1DOF_SOFT_LIMITS 1 // Soft limits tech; 1dof joint soft limits for character elbows and knees.

#define NM_FALL2KNEESPOINTGUN 0
#define NM_HANDSANDKNEES_FIX 1

#define NM_FAST_COLLISION_CHECKING 0

#define NM_RUNTIME_LIMITS_IK 1 //runTimeLimits are taken into account in effector angle clamping and inside IK routines 
#define NM_RUNTIME_LIMITS 1
#define NM_RUNTIME_LIMITS_RECOVERY_TIME 10.f

// new hit decrements balancer timer and step counter
// keeping characters on their feet longer when shot
// multiple times.
#define NM_NEW_HIT_EXTENDS_BALANCE 0
//If shot from behind then NM code probeRays through the body to find the exit wound and this is used for the reachFor wound point
#define NM_FIND_EXITWOUND 1

#define NM_EA_TEST_FROM_IMPACTS 0
#define NM_EA_DEVEL 0
#define NM_EA 0
//configureTheCharacter should be called instead of configureCharacter by the game.  
// Until this is so NM_PRE_INTEGRATION 1 will make the code act quite similar to if that had been done
// i.e. it modifies configureCharacter to act like configureTheCharacter
#define NM_PRE_INTEGRATION 1//Never check this in as 1
#define NM_SCRIPTING 0//Doesn't compile in Release for PS3 if = 1
#define NM_SCRIPTING_VARIABLES 0
//#define NM_ANIM_MATRICES 0 // set in art.h
#define NM_NEW_BALANCER 1
#define STOP_TRIPPING 1 
#define NM_SIDE_STEP_FIX 1
#define NM_STEP_UP 1//WIP Please leave as 0
#define STOP_TRIPPING_OLD 0 
//If the character is falling forwards off a cliff or is upsidedown:
// Makes the length of the feet smaller (so heels are inside lowerLeg) and sets foot friction to zero
//  Stops pivoting around feet when falling off a cliff.
//  Stops heels getting stuck to geometry when upsidedown.
#define NM_UPSIDEDOWN_FEET 1

#define NM_ONE_LEG_BULLET 1

#define NM_TEST_NEW_INSIDESUPPORT 0

#define NM_STAGGERSHOT 1//None of the variables are actually used in staggerFall therefore staggerFall sets the spine without the damping defined by shot looseness
#define NM_RIGID_BODY_BULLET 1

// Identify objects that collide with the hands to be ignored.
// RDR-era hack around missing weapon masks.
#define NM_OBJECTS_IN_COLLISIONS 0

#ifndef NM_RS_ENABLE_LOGGING
# define NM_RS_ENABLE_LOGGING 0
#endif // NM_RS_ENABLE_LOGGING


#if NM_RS_ENABLE_LOGGING

// wchar expansion of __FILE__ & __LINE__ used for logging
# define NM_RS_WIDEN2(x) L ## x
# define NM_RS_WIDEN(x) NM_RS_WIDEN2(x)
# define NM__WFILE__ NM_RS_WIDEN(__FILE__)

# define NM_RS_WLINE2(x) #x 
# define NM_RS_WLINE(x) NM_RS_WLINE2(x)
# define NM__WLINE__ NM_RS_WIDEN(NM_RS_WLINE( __LINE__ ))

//# define NM_RS_DBG_LOGF(...) m_artMemoryManager->logger->logf(NMutils::BasicLogger::kInfo, NM__WFILE__ L"(" NM__WLINE__ L") - " __VA_ARGS__);
//# define NM_RS_DBG_LOGF_FROMPARENT(...) m_parent->m_services->logger->logf(NMutils::BasicLogger::kInfo, NM__WFILE__ L"(" NM__WLINE__ L") - " __VA_ARGS__);
//# define NM_RS_LOGERROR(...) m_services->logger->logf(NMutils::BasicLogger::kError, NM__WFILE__ L"(" NM__WLINE__ L") - " __VA_ARGS__);
# define NM_RS_DBG_LOGF(...) Displayf("NMLOG |" NM__WFILE__ L"(" NM__WLINE__ L") - " __VA_ARGS__);//mmmmnoART check this mmmmtodo
# define NM_RS_DBG_LOGF_FROMPARENT(...) Displayf("NMLOG |" NM__WFILE__ L"(" NM__WLINE__ L") - " __VA_ARGS__);
# define NM_RS_LOGERROR(...) Errorf("NMLOG |" NM__WFILE__ L"(" NM__WLINE__ L") - " __VA_ARGS__);//mmmmnoART check this mmmmtodo

#else

# define NM_RS_DBG_LOGF(...)
# define NM_RS_DBG_LOGF_FROMPARENT(...)
# define NM_RS_LOGERROR(...)

#endif // NM_RS_ENABLE_LOGGING


// set to have sections of the code validate their inputs constantly -
// eg. effectors check their desired angles, stiffnesses, etc to ensure that
// they are not NANs, outside of acceptable range, etc.
// shall remove once we're approaching final release
#define NM_RS_VALIDATE_VITAL_VALUES 1

// used to identify an invalid level index
#define NM_RS_INVALID_LVL_INDEX   (-1)


#define NM_RS_FLOATEPS      0.000001f
#define NM_MIN_STABLE_DISTANCECONSTRAINT_DISTANCE 0.001f

//There is no ART_PROFILING.  Leave the macros in so code can be diffed
# define BEGIN_PROFILING(name)
# define BEGIN_PROFILING_TAGGED(name, numtag)
# define END_PROFILING()

#endif // NM_RS_COMMON_H

