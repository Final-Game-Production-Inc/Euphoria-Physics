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
#include "NmRsEffectorInputBlendable.h"

#if ART_ENABLE_BSPY_LIMBS
#undef NM_RS_RO_PARAMETER
#define NM_RS_RO_PARAMETER(_prefix, _type, _name)

#undef NM_RS_RO_PARAMETER_ACTUAL
#define NM_RS_RO_PARAMETER_ACTUAL(_prefix, _type, _name)

#undef NM_RS_PARAMETER_DIRECT
#define NM_RS_PARAMETER_DIRECT(_prefix, _type, _name, _min, _max, _default)

#undef NM_RS_PARAMETER
#include "bspy/bSpyCommonPackets.h"
#endif

#if ART_ENABLE_BSPY_LIMBS
#undef SEND_BSPY_COMPONENT
#define SEND_BSPY_COMPONENT(_prefix, _type, _name)\
if ((m_data.m_flags & _prefix##InputData::apply##_name) || m_##_name##Weight > 0.0f )\
  {\
  _type temp = get##_name();\
  convertForBspy1(temp, cp.m_##_name);\
  cp.m_##_name##SetBy = bSpyServer::inst()->getTokenForString(m_##_name##SetBy);\
  cp.m_parameterSetFlags |= _prefix##InputData::apply##_name;\
}
#endif // ART_ENABLE_BSPY_LIMBS

namespace ART
{
#if ART_ENABLE_BSPY_LIMBS
  // support for easy vector type conversion in limb macros when input type
  // is not strictly known.
  template <typename IN_TYPE, typename OUT_TYPE>
  inline void convertForBspy1(IN_TYPE& in, OUT_TYPE& out)
  {
    out = in;
  }

  template <>
  inline void convertForBspy1(rage::Vector3& in, bSpy::bSpyVec3& out)
  {
    out = bSpyVec3fromVector3(in);
  }
#endif

// NmRs1DofEffectorInputBlendable
//---------------------------------------------------------------------------

#if ART_ENABLE_BSPY_LIMBS
void NmRs1DofEffectorInputBlendable::sendComponents(const char* name, BehaviourID /* task */)
{
  if (!m_data.m_flags)
    return;
  bSpy::LimbComponent1DofPacket cp(bSpyServer::inst()->getTokenForString(name));
  // Reference macro expansion for NmRs1DofEffector, float, DesiredAngle
  //
  //if(m_data.m_flags & NmRs1DofEffectorInputData::applyDesiredAngle)
  //{
  //  float temp = getDesiredAngle();
  //  convertForBspy(temp, cp.m_DesiredAngle);
  //  cp.m_DesiredAngleSetBy = bSpyServer::inst()->getTokenForString(m_DesiredAngleSetBy);
  //  cp.m_parameterSetFlags |= NmRs1DofEffectorInputData::applyDesiredAngle;
  //}

  // Auto-generate send components from inline file.
  #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) SEND_BSPY_COMPONENT(_prefix, _type, _name)
  #include "common\NmRs1DofEffector.inl"
  #undef NM_RS_PARAMETER

  bspySendPacket(cp);
}
#endif

// NmRs3DofEffectorInputBlendable
//---------------------------------------------------------------------------
#if ART_ENABLE_BSPY_LIMBS
void NmRs3DofEffectorInputBlendable::sendComponents(const char* name, BehaviourID /* task */)
{
  if (!m_data.m_flags)
    return;
  bSpy::LimbComponent3DofPacket cp(bSpyServer::inst()->getTokenForString(name));

  // Copy data and setBy value to bSpy packet e.g.:
  //if (m_data.m_flags & NmRs3DofEffectorInputData::applyDesiredLean1)
  //{
  //  float temp = getDesiredLean1();
  //  convertForBspy1(temp, cp.m_DesiredLean1);
  //  cp.m_DesiredLean1SetBy = bSpyServer::inst()->getTokenForString(m_DesiredLean1SetBy);
  //  cp.m_parameterSetFlags |= NmRs3DofEffectorInputData::applyDesiredLean1;
  //}
  // Auto-generate send components from inline file.
  #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) SEND_BSPY_COMPONENT(_prefix, _type, _name)
  #include "common\NmRs3DofEffector.inl"
  #undef NM_RS_PARAMETER

  bspySendPacket(cp);
}
#endif

} // namespace ART
