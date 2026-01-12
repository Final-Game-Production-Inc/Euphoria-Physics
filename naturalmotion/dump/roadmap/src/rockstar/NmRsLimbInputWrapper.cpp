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
#include "NmRsLimbInputWrapper.h"

#if ART_ENABLE_BSPY_LIMBS
#include "bSpy\NmRsSpyPackets.h"
#endif // ART_ENABLE_BSPY_LIMBS

namespace ART
{
#if ART_ENABLE_BSPY_LIMBS
// support for easy vector type conversion in limb macros when input type
// is not strictly known.
template <typename IN_TYPE, typename OUT_TYPE>
inline void convertForBspy3(IN_TYPE& in, OUT_TYPE& out)
{
  out = in;
}

// Template specialisation for converting rage vectors to bspy vectors
template <>
inline void convertForBspy3(rage::Vector3& in, bSpy::bSpyVec3& out)
{
  out = bSpyVec3fromVector3(in);
}
#endif

  // NmRsIkInputWrapper
  //---------------------------------------------------------------------------

#if ART_ENABLE_BSPY_LIMBS

#define SEND_BSPY_COMPONENT(_prefix, _type, _name)\
  if (m_data.m_flags & _prefix##InputData::apply##_name)\
  {\
    _type temp = get##_name();\
    convertForBspy3(temp, cp.m_##_name);\
    cp.m_parameterSetFlags |= LimbComponentIKPacket::apply##_name;\
  }

#endif // ART_ENABLE_BSPY_LIMBS

// todo remove use* values. the applyFlags make this redundant.
void NmRsIKInputWrapper::C_LimbIK(
  bool twistIsFixed, const rage::Vector3 *target /*= NULL*/, float *twist /*= NULL*/,
  float *dragReduction /*= NULL*/, float *maxReachDistance /*= NULL*/,
  const rage::Vector3 *targetVel /*= NULL*/, float *blend /*= NULL*/,
  float *straightness /*= NULL*/, float *maxSpeed /*= NULL*/,
  const rage::Vector3 *poleVector /*= NULL*/, int /*version = 0*/,
  const rage::Vector3 *effectorOffset /*= NULL*/)
{
  setTwistIsFixed(twistIsFixed);
  if (target) setTarget(*target);
  if (twist) setTwist(*twist);
  if (dragReduction) setDragReduction(*dragReduction);
  if (maxReachDistance) setMaxReachDistance(*maxReachDistance);
  if (targetVel)
  {
    setVelocity(*targetVel);
  }
  if (blend) setBlend(*blend);
  if (straightness) setAdvancedStaightness(*straightness);
  if (maxSpeed) setAdvancedMaxSpeed(*maxSpeed);
  if (poleVector)
  {
    setPoleVector(*poleVector);
  }
  if (effectorOffset)
  {
    setEffectorOffset(*effectorOffset);
  }
}

#if ART_ENABLE_BSPY_LIMBS
  void NmRsIKInputWrapper::sendComponents(const char* /* name */, BehaviourID /* task */)
  {
    if (!m_data.m_flags)
      return;

    bSpy::LimbComponentIKPacket cp(bSpyServer::inst()->getTokenForString("Hmmmm"));

    // Auto-generation of bspy send components via inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) SEND_BSPY_COMPONENT(_prefix, _type, _name)
    #include "common\NmRsIK.inl"
    #undef NM_RS_PARAMETER

    bspySendPacket(cp);
  }
#endif

} // namespace ART
