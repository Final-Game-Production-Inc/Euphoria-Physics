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
// Blendable input data for the various effectors.  Carries additional setBy
// and blend weight data required to blend and debug properly.


#include "NmRsInclude.h"

#ifndef NM_RS_EFFECTOR_INPUT_BLENDABLE_H
#define NM_RS_EFFECTOR_INPUT_BLENDABLE_H

#include "NmRsEffectorInputWrapper.h"
#include "NmRsCBU_Shared.h"

#if ART_ENABLE_BSPY_LIMBS
#include "NmRsSpy.h"
#include "NmRsCBU_Shared.h"
#include "bSpy\bSpyCommonPackets.h"
#endif // ART_ENABLE_BSPY_LIMBS


#if ART_ENABLE_BSPY_LIMBS
#define INIT_BLENDABLE_PARAMETER(_name)\
  m_##_name##Weight = 0.0f;\
  sprintf(m_##_name##SetBy, "");
#else
#define INIT_BLENDABLE_PARAMETER(_name)\
  m_##_name##Weight = 0.0f;
#endif

#if ART_ENABLE_BSPY_LIMBS
#define BLEND_PARAMETER(_prefix, _name)\
  if (input.m_flags & _prefix##InputData::apply##_name && !(m_data.m_flags & _prefix##InputData::apply##_name)){\
    m_data.m_##_name = input.m_##_name * (1.0f - m_##_name##Weight) + m_data.m_##_name * m_##_name##Weight;\
    float clampedWeight = nm_fsel_min_float(weight, 1.0f - m_##_name##Weight);\
    if (ART::bSpyServer::inst() && ART::bSpyServer::inst()->isClientConnected()) {\
      if(clampedWeight < 1.0f)\
        sprintf(m_##_name##SetBy +strlen(m_##_name##SetBy), "%s %s(%.1f) ", getBvidNameSafe(task), subTask, clampedWeight);\
      else\
        sprintf(m_##_name##SetBy +strlen(m_##_name##SetBy), "%s %s", getBvidNameSafe(task), subTask);\
    }\
    m_##_name##Weight += clampedWeight;\
    if(m_##_name##Weight == 1.0f)\
    m_data.m_flags |= _prefix##InputData::apply##_name;\
  }
#else
#define BLEND_PARAMETER(_prefix, _name)\
  if (canApply & _prefix##InputData::apply##_name){\
    m_data.m_##_name = input.m_##_name * (1.0f - m_##_name##Weight) + m_data.m_##_name * m_##_name##Weight;\
    m_##_name##Weight = nm_fsel_min_float(1.0f, m_##_name##Weight + weight);\
    if(m_##_name##Weight == 1.0f)\
      m_data.m_flags |= _prefix##InputData::apply##_name;\
  }
#endif

#if ART_ENABLE_BSPY_LIMBS
#define DECLARE_BLENDABLE_PARAMETER(_name)\
  float m_##_name##Weight;\
  char m_##_name##SetBy[1024];
#else
#define DECLARE_BLENDABLE_PARAMETER(_name)\
  float m_##_name##Weight;
#endif

namespace ART
{

  //
  // NmRs1DofEffectorInputBlendable
  //---------------------------------------------------------------------------
  class NmRs1DofEffectorInputBlendable : public NmRs1DofEffectorInputWrapper
  {
  public:
    NmRs1DofEffectorInputBlendable(NmRs1DofEffectorInputData& data) : NmRs1DofEffectorInputWrapper(data)
    {
      init();
    }

    void init()
    {
      // Initialise blendable-specific data. E.g.:
      // m_DesiredAngleWeight = 0.0f;
      // m_DesiredAngleSetBy = bvid_Invalid;
      // Auto-generate initialisation from inline file.
      #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) INIT_BLENDABLE_PARAMETER(_name)
      #include "common\NmRs1DofEffector.inl"
      #undef NM_RS_PARAMETER
    }

    // Blend individual effector parameters.if the input's apply flag is set
    // and ours is not. Returns true if all parameters have been set.
#if ART_ENABLE_BSPY_LIMBS
    bool blend(const NmRs1DofEffectorInputData& input, BehaviourID task, float weight, const char* subTask = 0)
#else
    bool blend(const NmRs1DofEffectorInputData& input, BehaviourID /*task*/, float weight)
#endif
    {
      unsigned int freePositions = ~m_data.m_flags & NmRs1DofEffectorInputData::applyAll;
      unsigned int canApply = input.m_flags & freePositions;

      // return if all of the positions we'd like to set are full
      if(canApply == 0)
      {
        // return true if there are no settable parameters
        return freePositions == 0;
      }

      // Blend each element of effector data. E.g.:
      // if(canApply & NmRs1DofEffectorInputData::applyDesiredAngle)
      // {
      //   m_data.m_DesiredAngle = input.m_DesiredAngle * (1.0f - m_DesiredAngleWeight) + m_data.m_DesiredAngle * m_DesiredAngleWeight;
      //   m_DesiredAngleWeight = rage::Clamp(m_DesiredAngleWeight + weight, 1.0f);
      //   nm_fsel_min_float(m_DesiredAngleWeight + weight, 1.0f, m_DesiredAngleWeight + weight);
      //   if(m_DesiredAngleWeight == 1.0f)
      //     m_data.m_flags |= NmRs1DofEffectorInputData::applyDesiredAngle;
      // }
      //
      // Auto-generate blend parameter from inline file.
      #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) BLEND_PARAMETER(_prefix, _name)
      #include "common\NmRs1DofEffector.inl"
      #undef NM_RS_PARAMETER

      return m_data.m_flags == NmRs1DofEffectorInputData::applyAll;
    }

    bool canDoIK(bool)
    {
      return !(m_data.m_flags & NmRs1DofEffectorInputData::applyDesiredAngle);
    }

#if ART_ENABLE_BSPY_LIMBS
    // Create and send bSpy limb effector data debug packet. Sets individual
    // parameter setBy values.
    void sendComponents(const char* name = 0, BehaviourID task = bvid_Invalid);
#endif

    // Declare additional values for each parameter to support blending and debug.
    //
    // float m_DesiredAngleWeight;
    // BehaviourID m_DesiredAngleSetBy;
    //
    // Auto-generate blend parameter member variables from inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DECLARE_BLENDABLE_PARAMETER(_name)
    #include "common\NmRs1DofEffector.inl"
    #undef NM_RS_PARAMETER
  };

  //
  // NmRs3DofEffectorInputBlendable
  //---------------------------------------------------------------------------
  class NmRs3DofEffectorInputBlendable : public NmRs3DofEffectorInputWrapper
  {
  public:
    NmRs3DofEffectorInputBlendable(NmRs3DofEffectorInputData& data) : NmRs3DofEffectorInputWrapper(data)
    {
      init();
    }

    void init()
    {
      // Auto-generate intialisation from inline file.
      #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) INIT_BLENDABLE_PARAMETER(_name)
      #include "common\NmRs3DofEffector.inl"
      #undef NM_RS_PARAMETER
    }

#if ART_ENABLE_BSPY_LIMBS
    bool blend(const NmRs3DofEffectorInputData& input, BehaviourID task, float weight, const char* subTask = 0)
#else
    bool blend(const NmRs3DofEffectorInputData& input, BehaviourID /*task*/, float weight)
#endif
    {
      unsigned int freePositions = ~m_data.m_flags & NmRs3DofEffectorInputData::applyAll;
      unsigned int canApply = input.m_flags & freePositions;

      // return if all of the positions we'd like to set are full
      if(canApply == 0)
      {
        // return true if there are no settable parameters
        return freePositions == 0;
      }

      // Blend each element of effector data. E.g.:
      //  if (canApply & NmRs3DofEffectorInputData::applyDesiredLean1)
      //  {
      //    m_data.m_DesiredLean1 = input.m_DesiredLean1 * (1.0f - m_DesiredLean1Weight) + m_data.m_DesiredLean1 * m_DesiredLean1Weight;
      //    m_DesiredLean1Weight = rage::Clamp(m_DesiredLean1Weight + weight, 0.0f, 1.0f);
      //    if(m_DesiredLean1Weight == 1.0f)
      //      m_data.m_flags |= NmRs3DofEffectorInputData::applyDesiredLean1;
      //  }
      //
      // Auto-generate blend parameter from inline file.
      #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) BLEND_PARAMETER(_prefix, _name)
      #include "common\NmRs3DofEffector.inl"
      #undef NM_RS_PARAMETER

      return m_data.m_flags == NmRs3DofEffectorInputData::applyAll;
    }

    bool canDoIK(bool greedy = false)
    {
      if(greedy)
      {
        // return true if *any* angles are free
        return !(m_data.m_flags & NmRs3DofEffectorInputData::applyDesiredLean1) |
               !(m_data.m_flags & NmRs3DofEffectorInputData::applyDesiredLean2) |
               !(m_data.m_flags & NmRs3DofEffectorInputData::applyDesiredTwist);
      }
      else
      {
        // return true if *all* angles are free
      return !(m_data.m_flags & (NmRs3DofEffectorInputData::applyDesiredLean1 | NmRs3DofEffectorInputData::applyDesiredLean2 | NmRs3DofEffectorInputData::applyDesiredTwist));
      }
    }

#if ART_ENABLE_BSPY_LIMBS
    void sendComponents(const char* name = 0, BehaviourID task = bvid_Invalid);
#endif

    // Auto-generate blend parameter member variables from inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DECLARE_BLENDABLE_PARAMETER(_name)
    #include "common\NmRs3DofEffector.inl"
    #undef NM_RS_PARAMETER
  };

} //namespace ART

#endif //NM_RS_EFFECTOR_INPUT_BLENDABLE_H
