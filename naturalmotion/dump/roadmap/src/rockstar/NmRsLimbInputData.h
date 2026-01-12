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

/**
 * -------------------------- IMPORTANT --------------------------
 * This file contains the minimum data for limb system messages.
 * It must be compilable on SPU, so no external references outside of the RAGE
 * vector/math libraries and our own math macros.
 */

#ifndef NM_RS_LIMB_INPUT_DATA_H
#define NM_RS_LIMB_INPUT_DATA_H

#undef NM_RS_RO_PARAMETER
#define NM_RS_RO_PARAMETER(_prefix, _type, _name)

#undef NM_RS_RO_PARAMETER_ACTUAL
#define NM_RS_RO_PARAMETER_ACTUAL(_prefix, _type, _name)

#undef NM_RS_PARAMETER_DIRECT
#define NM_RS_PARAMETER_DIRECT(_prefix, _type, _name, _min, _max, _default)

#undef NM_RS_PARAMETER

// declare member variable and accessor. accessor sets appropriate apply flag when member is set.
#define DECLARE_DATA(_type, _name)\
  inline void set##_name(const _type& _v) { m_##_name = _v; m_flags |= apply##_name; }\
  _type m_##_name;

// set counter offset correctly so apply flags enumerate correctly.
#define INIT_FLAG_OFFSET() static const int FlagOffset = __COUNTER__;

namespace ART
{

// Input types for all limbs
enum NmRsLimbInputType
{
  kIk,                    // standard two-bone ik input
  kArmPose,               // pose each effector
  kLegPose,               // pose each effector
  kSpinePose,             // pose each effector
  kSetStiffness,          // masked message, sets stiffness, damping, (muscle stiffness)
  kNumNmRsLimbInputTypes
};

enum NmRsIKType
{
  k2Bone,                 // 
  kNumNmRsIKTypes
};

enum NmRsClavicleMatchType
{
  kDontMatchClavicle,       // don't match clavicle at all
  kMatchClavicle,           // was NmRsCharacter::matchClavicleToShoulder()
  kMatchClavicleBetter,     // was NmRsCharacter::matchClavicleToShoulderBetter()
  kMatchClavicleUsingTwist, // was NmRsCharacter::matchClavicleToShoulderUsingTwist()
  kNumNmRsClavicleMatchTypes
};

// NmRsIkInputData
//-----------------------------------------------------------------------------
// todo this can probably be passed as-is to the SPU-able IK functions.
struct NmRsIKInputData
{
  // flags keep track of which data members have been set
  INIT_FLAG_OFFSET()
  enum Flags
  {
    // Auto-generate bit masks for input types from inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) apply##_name = 1 << (__COUNTER__ - FlagOffset - 1),
    #include "common\NmRsIK.inl"
    #undef NM_RS_PARAMETER

    // Auto-generate apply-all bitmask from inline file.
    applyAll = 0 
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) | apply##_name
    #include "common\NmRsIK.inl"
    #undef NM_RS_PARAMETER
  };
  unsigned int m_flags;

  // Auto-generated member data and accessors from inline file
  #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DECLARE_DATA(_type, _name)
  #include "common\NmRsIK.inl"
  #undef NM_RS_PARAMETER
};

// NmRs1DofEffectorInputData
//-----------------------------------------------------------------------------
struct NmRs1DofEffectorInputData
{
  // flags keep track of which data members have been set
  INIT_FLAG_OFFSET()
  enum Flags
  {
    // Auto-generate bit masks for input types from inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) apply##_name = 1 << (__COUNTER__ - FlagOffset - 1),
    #include "common\NmRs1DofEffector.inl"
    #undef NM_RS_PARAMETER

    // Auto-generate apply-all bitmask from inline file.
    applyAll = 0 
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) | apply##_name
    #include "common\NmRs1DofEffector.inl"
    #undef NM_RS_PARAMETER
  };
  unsigned int m_flags;

  // Auto-generated member data and accessors from inline file
  #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DECLARE_DATA(_type, _name)
  #include "common\NmRs1DofEffector.inl"
  #undef NM_RS_PARAMETER
};

// NmRs3DofEffectorInputData
//-----------------------------------------------------------------------------
struct NmRs3DofEffectorInputData
{
  // flags keep track of which data members have been set
  INIT_FLAG_OFFSET()
    enum Flags
  {
    // Auto-generate bit masks for input types from inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) apply##_name = 1 << (__COUNTER__ - FlagOffset - 1),
    #include "common\NmRs3DofEffector.inl"
    #undef NM_RS_PARAMETER

    // Auto-generate apply-all bitmask from inline file.
    applyAll = 0 
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) | apply##_name
    #include "common\NmRs3DofEffector.inl"
    #undef NM_RS_PARAMETER
  };
  unsigned int m_flags;

  // Auto-generated member data and accessors from inline file
  #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DECLARE_DATA(_type, _name)
  #include "common\NmRs3DofEffector.inl"
  #undef NM_RS_PARAMETER
};

// Set up inline macro for includes used in structs below.
#define NM_RS_PARAMETER(_type, _name)\
  _type##InputData* get##_name() { return &m_##_name; }\
  _type##InputData m_##_name;

// NmRsArmInputData
//-----------------------------------------------------------------------------
struct NmRsArmInputData
{
  // Struct members and accessors auto generated from inline using NM_RS_PARAMETER above
  #include "common\NmRsHumanArm.inl"
};

// NmRsLegInputData
//-----------------------------------------------------------------------------
struct NmRsLegInputData
{
  // Struct members and accessors auto generated from inline using NM_RS_PARAMETER above
  #include "common\NmRsHumanLeg.inl"
};

// NmRsSpineInputData
//-----------------------------------------------------------------------------
struct NmRsSpineInputData
{
  // Struct members and accessors auto generated from inline using NM_RS_PARAMETER above
  #include "common\NmRsHumanSpine.inl"
};

#undef NM_RS_PARAMETER

} // namespace ART

#endif // NM_RS_LIMB_INPUT_DATA_H
