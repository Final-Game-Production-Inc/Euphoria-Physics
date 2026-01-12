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
#include "NmRsLimbInputBlendable.h"

#undef BLEND_EFFECTOR
#define BLEND_EFFECTOR(_prefix, _name)\
  result = m_##_name##Blendable.blend(input.m_data.m_##_name, task, weight) && result;

#define CAN_DO_IK(_name)\
  result = m_##_name##Blendable.canDoIK() && result;

#define CAN_DO_IK_GREEDY(_name)\
  result = m_##_name##Blendable.canDoIK(greedy) || result;

#define SEND_BLENDABLE_COMPONENTS(_prefix, _name)\
  m_##_name##Blendable.sendComponents(#_name);

namespace ART {

//
// NmRsArmInputBlendable
//---------------------------------------------------------------------------

// would do the inits with the .inl file, but can't figure out how to do a
// conditional comma.
NmRsArmInputBlendable::NmRsArmInputBlendable() :
  m_ClavicleBlendable(m_data.m_Clavicle),
  m_ShoulderBlendable(m_data.m_Shoulder),
  m_ElbowBlendable(m_data.m_Elbow),
  m_WristBlendable(m_data.m_Wrist)
{
  init();
}

// for 2bone IK to make sense, we should have control of the shoulder and elbow
// phase2 todo take an argument specifying which kind of IK we intend to do and
// make a better decision.
bool NmRsArmInputBlendable::canDoIK(bool greedy /*= false*/)
{
  return getShoulderBlendable()->canDoIK(greedy) && (getElbowBlendable()->canDoIK(greedy) || greedy);
}

#if ART_ENABLE_BSPY_LIMBS
// Create and send bSpy limb effector data debug packets. Overrides
// wrapper function to use blendable effectors.
void NmRsArmInputBlendable::sendComponents(const char* /* name */, BehaviourID /* task */)
{
  // Send individual effector component messages. E.g.:
  // m_ShoulderBlendable.sendComponents("Shoulder");
  // Auto-generate send components from inline file.
  #define NM_RS_PARAMETER(_prefix, _name) SEND_BLENDABLE_COMPONENTS(_prefix, _name)
  #include "common\NmRsHumanArm.inl"
  #undef NM_RS_PARAMETER
}
#endif


//
// NmRsLegInputBlendable
//---------------------------------------------------------------------------

NmRsLegInputBlendable::NmRsLegInputBlendable() :
  m_HipBlendable(m_data.m_Hip),
  m_KneeBlendable(m_data.m_Knee),
  m_AnkleBlendable(m_data.m_Ankle)
{
  init();
}

// for 2bone IK to make sense, we should have control of the hip and knee.
// phase2 todo take an argument specifying which kind of IK we intend to do and
// make a better decision.
bool NmRsLegInputBlendable::canDoIK(bool greedy /*= false*/)
{
  if(greedy)
  {
    return getHipBlendable()->canDoIK(greedy) || getKneeBlendable()->canDoIK(greedy);
  }
  else
  {
    return getHipBlendable()->canDoIK(greedy) && getKneeBlendable()->canDoIK(greedy);
  }
}

#if ART_ENABLE_BSPY_LIMBS
void NmRsLegInputBlendable::sendComponents(const char* /* name */, BehaviourID /* task */)
{
  // Auto-generate send components from inline file.
  #define NM_RS_PARAMETER(_prefix, _name) SEND_BLENDABLE_COMPONENTS(_prefix, _name)
  #include "common\NmRsHumanLeg.inl"
  #undef NM_RS_PARAMETER
}
#endif


//
// NmRsSpineInputBlendable
//---------------------------------------------------------------------------

NmRsSpineInputBlendable::NmRsSpineInputBlendable() :
  m_Spine0Blendable(m_data.m_Spine0),
  m_Spine1Blendable(m_data.m_Spine1),
  m_Spine2Blendable(m_data.m_Spine2),
  m_Spine3Blendable(m_data.m_Spine3),
  m_LowerNeckBlendable(m_data.m_LowerNeck),
  m_UpperNeckBlendable(m_data.m_UpperNeck)
{
}

bool NmRsSpineInputBlendable::canDoIK(bool greedy /*=false*/)
{
  bool result = true;

  if(greedy)
  {
    // Auto-generate can do IK from inline file.
    #define NM_RS_PARAMETER(_prefix, _name) CAN_DO_IK_GREEDY(_name)
    #include "common\NmRsHumanSpine.inl"
    #undef NM_RS_PARAMETER
  }
  else
  {
    // Auto-generate can do IK from inline file.
    #define NM_RS_PARAMETER(_prefix, _name) CAN_DO_IK(_name)
    #include "common\NmRsHumanSpine.inl"
    #undef NM_RS_PARAMETER
  }

  return result;
}

#if ART_ENABLE_BSPY_LIMBS
void NmRsSpineInputBlendable::sendComponents(const char* /* name */, BehaviourID /* task */)
{
  // Auto-generate sendComponents from inline file.
  #define NM_RS_PARAMETER(_prefix, _name) SEND_BLENDABLE_COMPONENTS(_prefix, _name)
  #include "common\NmRsHumanSpine.inl"
  #undef NM_RS_PARAMETER
}
#endif

} // namespace ART
