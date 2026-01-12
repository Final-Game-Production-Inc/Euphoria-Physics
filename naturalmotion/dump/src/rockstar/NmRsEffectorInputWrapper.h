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

#ifndef NM_RS_EFFECTOR_INPUT_WRAPPER_H
#define NM_RS_EFFECTOR_INPUT_WRAPPER_H

#include "NmRsLimbInputData.h"
#include "NmRsUtils.h"

#if ART_ENABLE_BSPY_LIMBS
#include "NmRsSpy.h"
#include "NmRsCBU_Shared.h"
#include "bSpy\bSpyCommonPackets.h"
#endif // ART_ENABLE_BSPY_LIMBS

#undef DECLARE_ACCESSORS
#define DECLARE_ACCESSORS(_prefix, _type, _name)\
  void set##_name(const _type& _v) { m_data.set##_name(_v); m_data.m_flags |= _prefix##InputData::apply##_name; }\
  _type get##_name() const { return m_data.m_##_name; }

#define INITIALIZE_PARAMETER(_name, _default)\
  m_data.m_##_name = _default;

namespace ART
{

class NmRsEffectorBase;

//
// NmRsEffectorInputWrapper
//
// Provides task-level support for effector input data. Implements many of
// the functions previously accessed through effectors directly.  Aims to
// provide a task-level usage nearly identical to the pre-limbs code.
//---------------------------------------------------------------------------
class NmRsEffectorInputWrapper
{
public:
  virtual ~NmRsEffectorInputWrapper() {}

  virtual void init() = 0;
  virtual void setToCurrent(const NmRsEffectorBase* eff) = 0;
  virtual void setStiffness(float stiffness, float damping, float *muscleStiffness = 0) = 0;
  virtual void setRelaxation(const NmRsEffectorBase* effector, float mult, float *pMultDamping = 0) = 0;
  virtual void blendToZeroPose(const NmRsEffectorBase* effector, float t) = 0;
  virtual void reset(const NmRsEffectorBase* effector, ResetEffectorsType resetType, float scale = 1.0f) = 0;
  virtual void activePose(const NmRsEffectorBase* effector, int transformSource) = 0;
  virtual void holdPose(const NmRsEffectorBase* effector) = 0;
  virtual void clamp(const NmRsEffectorBase* effector, float amount) = 0;

#if ART_ENABLE_BSPY_LIMBS
  virtual void sendComponents(const char* name = 0, BehaviourID task = bvid_Invalid) = 0;
#endif
};

//
// NmRs1DofEffectorInputWrapper
//---------------------------------------------------------------------------
class NmRs1DofEffectorInputWrapper : public NmRsEffectorInputWrapper
{
public:
  NmRs1DofEffectorInputWrapper(NmRs1DofEffectorInputData& data) :
  m_data(data)
  {
    init();
  }

  void init()
  {
    m_data.m_flags = 0;

    // Auto-generate parameter initialization from inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) INITIALIZE_PARAMETER(_name, _default)
    #include "common\NmRs1DofEffector.inl"
    #undef NM_RS_PARAMETER
  }

  virtual ~NmRs1DofEffectorInputWrapper() {}

  void setToCurrent(const NmRsEffectorBase* eff);
  void setStiffness(float stiffness, float damping, float *muscleStiffness = 0);
  void setRelaxation(const NmRsEffectorBase* effector, float mult, float *pMultDamping = 0);
  void blendToZeroPose(const NmRsEffectorBase* effector, float t);
  void blendToSpecifiedPose(float angle, float t);
  void reset(const NmRsEffectorBase* effector, ResetEffectorsType resetType, float scale = 1.0f);
  void activePose(const NmRsEffectorBase* effector, int transformSource);
  void holdPose(const NmRsEffectorBase* effector);
  void clamp(const NmRsEffectorBase* /*effector*/, float /*amount*/) {};

#if ART_ENABLE_BSPY_LIMBS
  void sendComponents(const char* name = 0, BehaviourID task = bvid_Invalid);
#endif

  #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DECLARE_ACCESSORS(_prefix, _type, _name)
  #include "common\NmRs1DofEffector.inl"
  #undef NM_RS_PARAMETER

NmRs1DofEffectorInputData& m_data;
};

//
// NmRs3DofEffectorInputWrapper
//---------------------------------------------------------------------------
class NmRs3DofEffectorInputWrapper : public NmRsEffectorInputWrapper
{
public:
  NmRs3DofEffectorInputWrapper(NmRs3DofEffectorInputData& data) :
    m_data(data)
  {

  }

  void init()
  {
    m_data.m_flags = 0;

    // Auto-generate parameter initialization from inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) INITIALIZE_PARAMETER(_name, _default)
    #include "common\NmRs3DofEffector.inl"
    #undef NM_RS_PARAMETER
  }

  void setDesiredAngles(float lean1, float lean2, float twist)
  {
    setDesiredTwist(twist);
    setDesiredLean1(lean1);
    setDesiredLean2(lean2);
  }

  void setToCurrent(const NmRsEffectorBase* eff);
  void setStiffness(float stiffness, float damping, float *muscleStiffness = 0);
  void setRelaxation(const NmRsEffectorBase* effector, float mult, float *pMultDamping = 0);
  void blendToZeroPose(const NmRsEffectorBase* effector, float t);
  void reset(const NmRsEffectorBase* effector, ResetEffectorsType resetType, float scale = 1.0f);
  void blendToSpecifiedPose(rage::Vector3& twistLean, float t);
  void activePose(const NmRsEffectorBase* effector, int transformSource);
  void holdPose(const NmRsEffectorBase* effector);
  void clamp(const NmRsEffectorBase* effector, float amount);

#if ART_ENABLE_BSPY_LIMBS
  void sendComponents(const char* name = 0, BehaviourID task = bvid_Invalid);
#endif

  // Auto-generate member accessors from inline file.
  #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DECLARE_ACCESSORS(_prefix, _type, _name)
  #include "common\NmRs3DofEffector.inl"
  #undef NM_RS_PARAMETER

  NmRs3DofEffectorInputData& m_data;
};

//
// Member function typedef for calling arbitrary effector functions either
// with or without a float argument
//---------------------------------------------------------------------------
typedef void (NmRs3DofEffectorInputWrapper::*Effector3DofDataFunctionNoArgs)();
typedef void (NmRs3DofEffectorInputWrapper::*Effector3DofDataFunctionFloatArg)(float arg);
typedef void (NmRs3DofEffectorInputWrapper::*Effector3DofDataFunctionIntArg)(int arg);
typedef void (NmRs1DofEffectorInputWrapper::*Effector1DofDataFunctionNoArgs)();
typedef void (NmRs1DofEffectorInputWrapper::*Effector1DofDataFunctionFloatArg)(float arg);
typedef void (NmRs1DofEffectorInputWrapper::*Effector1DofDataFunctionIntArg)(int arg);

} // namespace ART

#endif // NM_RS_EFFECTOR_INPUT_WRAPPER_H
