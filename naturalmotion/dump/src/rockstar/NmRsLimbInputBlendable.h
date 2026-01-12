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

//
// Blendable limb input data for the various limbs.  The limb system uses
// these types during the tick to receive the output as it bakes the message
// stack into a single set of drive signals.
//---------------------------------------------------------------------------

#include "NmRsInclude.h"

#ifndef NM_RS_LIMB_INPUT_BLENDABLE_H
#define NM_RS_LIMB_INPUT_BLENDABLE_H

#include "NmRsLimbInputWrapper.h"
#include "NmRsEffectorInputBlendable.h"

#define DECLARE_BLENDABLE_EFFECTOR_DATA(_prefix, _name)\
public:\
  _prefix##InputBlendable* get##_name##Blendable() { return &m_##_name##Blendable; }\
protected:\
  _prefix##InputBlendable m_##_name##Blendable;

#define INIT_BLENDABLE_EFFECTOR_DATA(_prefix, _name)\
  m_##_name##Blendable.init();

namespace ART
{

//
// NmRsArmInputBlendable
//---------------------------------------------------------------------------
class NmRsArmInputBlendable : public NmRsArmInputWrapper
{
public:
  NmRsArmInputBlendable();

  void init()
  {
    NmRsArmInputWrapper::init();

    // Auto-generate initialization code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) INIT_BLENDABLE_EFFECTOR_DATA(_prefix, _name)
    #include "common\NmRsHumanArm.inl"
    #undef NM_RS_PARAMETER
  }

  bool canDoIK(bool greedy = false);

#if ART_ENABLE_BSPY_LIMBS
  // Create and send bSpy limb effector data debug packets. Overrides
  // wrapper function to use blendable effectors.
  void sendComponents(const char* /* name */, BehaviourID /* task */);
#endif

  // Declare individual effector blendables. Eg:
  //  public:
  //    NmRs3DofInputBlendable* getClavicleBlendable() { return &m_ClavicleBlendable; }
  //  protected:
  //    NmRs3DofInputBlendable m_ClavicleBlendable;
  #define NM_RS_PARAMETER(_prefix, _name) DECLARE_BLENDABLE_EFFECTOR_DATA(_prefix, _name)
  #include "common\NmRsHumanArm.inl"
  #undef NM_RS_PARAMETER
};

//
// NmRsLegInputBlendable
//---------------------------------------------------------------------------
class NmRsLegInputBlendable : public NmRsLegInputWrapper
{
public:
  NmRsLegInputBlendable();

  void init()
  {
    NmRsLegInputWrapper::init();

    // Auto-generate initialization code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) INIT_BLENDABLE_EFFECTOR_DATA(_prefix, _name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

  bool canDoIK(bool greedy = false);

#if ART_ENABLE_BSPY_LIMBS
  void sendComponents(const char* /* name */, BehaviourID /* task */);
#endif

  #define NM_RS_PARAMETER(_prefix, _name) DECLARE_BLENDABLE_EFFECTOR_DATA(_prefix, _name)
  #include "common\NmRsHumanLeg.inl"
  #undef NM_RS_PARAMETER
};

//
// NmRsSpineInputBlendable
//---------------------------------------------------------------------------
class NmRsSpineInputBlendable : public NmRsSpineInputWrapper
{
public:
  NmRsSpineInputBlendable();

  void init()
  {
    NmRsSpineInputWrapper::init();

    // Auto-generate initialization code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) INIT_BLENDABLE_EFFECTOR_DATA(_prefix, _name)
    #include "common\NmRsHumanSpine.inl"
    #undef NM_RS_PARAMETER
  }

  bool canDoIK(bool greedy = false);

#if ART_ENABLE_BSPY_LIMBS
  void sendComponents(const char* name, BehaviourID task);
#endif

  #define NM_RS_PARAMETER(_prefix, _name) DECLARE_BLENDABLE_EFFECTOR_DATA(_prefix, _name)
  #include "common\NmRsHumanSpine.inl"
  #undef NM_RS_PARAMETER
};

} // namespace ART

#endif // NM_RS_LIMB_INPUT_BLENDABLE_H