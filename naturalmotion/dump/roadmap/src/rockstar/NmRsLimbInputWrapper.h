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

/*
 * Wrapper classes provide task-level support for the various limb input messages.
 */

#include "NmRsInclude.h"

#ifndef NM_RS_EFFECTOR_WRAPPER_H
#define NM_RS_EFFECTOR_WRAPPER_H

#if ART_ENABLE_BSPY_LIMBS
#include "NmRsSpy.h"
#include "bSpy\bSpyCommonPackets.h"
#endif // ART_ENABLE_BSPY_LIMBS

#include "NmRsCBU_Shared.h"
#include "NmRsLimbInputData.h"
#include "NmRsEffectorInputWrapper.h"

namespace ART
{

#define DECLARE_LIMB_INPUT_DATA(enumId) \
  static const NmRsLimbInputType TYPE = enumId;\

#undef DECLARE_ACCESSORS
#define DECLARE_ACCESSORS(_prefix, _type, _name)\
  void set##_name(const _type& _v) { m_data.set##_name(_v); m_data.m_flags |= _prefix##InputData::apply##_name; }\
  _type get##_name() const { return m_data.m_##_name; }

#define INITIALIZE_PARAMETER(_name, _default)\
  m_data.m_##_name = _default;

  //
  // NmRsInputWrapperBase
  //
  // Supports bSpy debugging of the various input wrappers.
  // TODO consider renaming to reflect actual usage.
  //---------------------------------------------------------------------------
  struct NmRsInputWrapperBase
  {
    virtual ~NmRsInputWrapperBase() {}

#if ART_ENABLE_BSPY_LIMBS
    virtual void sendComponents(const char* name = 0, BehaviourID task = bvid_Invalid) = 0;
    BehaviourID m_task;
#endif
  };

  //
  // NmRsIKInputWrapper
  //---------------------------------------------------------------------------
  struct NmRsIKInputWrapper : public NmRsInputWrapperBase
  {
    DECLARE_LIMB_INPUT_DATA(kIk)

    NmRsIKInputWrapper()
    {
      m_data.m_flags = 0;

      // Auto-generate code for input wrapper parameter initialisation via inline file
      #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) INITIALIZE_PARAMETER(_name, _default)
      #include "common\NmRsIK.inl"
      #undef NM_RS_PARAMETER
    }

    const NmRsIKInputData getData() { return m_data; }

    // Imitates old character function for easy compatibility.
    void C_LimbIK(bool twistIsFixed, const rage::Vector3 *target = NULL, float *twist = NULL, float *dragReduction = NULL,
      float *maxReachDistance = NULL, const rage::Vector3 *targetVel = NULL,
      float *blend = NULL, float *straightness = NULL, float *maxSpeed = NULL,
      const rage::Vector3 *poleVector = NULL, int version = 0,
      const rage::Vector3 *effectorOffset = NULL);

#if ART_ENABLE_BSPY_LIMBS
    void sendComponents(const char* name = 0, BehaviourID task = bvid_Invalid);
#endif

    // Auto-generate members and accessors for input wrapper via inline file
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DECLARE_ACCESSORS(_prefix, _type, _name)
    #include "common\NmRsIK.inl"
    #undef NM_RS_PARAMETER

    NmRsIKInputData m_data;
  };

//
// Pose inputs for the various limbs.  These form the backbone of the limb
// system as they carry all of the data we could possibly want to set on a
// given effector. We can bake any other input type down into the pose input
// for the given limb.
//
// Aim to provide a task-level usage nearly identical to the pre-limbs code.
//---------------------------------------------------------------------------
#define DECLARE_EFFECTOR_DATA(_prefix, _name)\
  _prefix##InputWrapper m_##_name;\
  _prefix##InputWrapper * get##_name() { return &m_##_name; }

#define INIT_EFFECTOR_DATA(_name)\
  m_##_name.init();

#define SEND_COMPONENTS(_prefix, _name)\
  m_##_name.sendComponents(#_name);

#define SET_EFFECTOR_PARAMS(_prefix, _name)\
  m_##_name.setStiffness(strength, damping, stiffness);

#define SET_OPPOSE_GRAVITY(_prefix, _name)\
  m_##_name.setOpposeGravity(oppose);

//
// NmRsArmInputWrapper
//---------------------------------------------------------------------------
struct NmRsArmInputWrapper : public NmRsInputWrapperBase
{
  DECLARE_LIMB_INPUT_DATA(kArmPose)

  NmRsArmInputWrapper() :
    m_Clavicle(m_data.m_Clavicle),
    m_Shoulder(m_data.m_Shoulder),
    m_Elbow(m_data.m_Elbow),
    m_Wrist(m_data.m_Wrist)
  {
    init();
  }

  void init()
  {
    // Auto-generate effector data initialization...
    #define NM_RS_PARAMETER(_prefix, _name) INIT_EFFECTOR_DATA(_name)
    #include "common\NmRsHumanArm.inl"
    #undef NM_RS_PARAMETER
  }

  // mimics existing character function...
  void setStiffness(float strength, float damping, float *stiffness = 0)
  {
    // Auto-generate setting effector params code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) SET_EFFECTOR_PARAMS(_prefix, _name)
    #include "common\NmRsHumanArm.inl"
    #undef NM_RS_PARAMETER
  }

  void setOpposeGravity(float oppose)
  {
    // Auto-generate oppose gravity code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) SET_OPPOSE_GRAVITY(_prefix, _name)
    #include "common\NmRsHumanArm.inl"
    #undef NM_RS_PARAMETER
  }

#if ART_ENABLE_BSPY_LIMBS
  void sendComponents(const char* /* name */, BehaviourID /* task */)
  {
    // Auto-generate send components code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) SEND_COMPONENTS(_prefix, _name)
    #include "common\NmRsHumanArm.inl"
    #undef NM_RS_PARAMETER
  }
#endif

  // Auto-generate members and accessors via inline file.
  #define NM_RS_PARAMETER(_prefix, _name) DECLARE_EFFECTOR_DATA(_prefix, _name)
  #include "common\NmRsHumanArm.inl"
  #undef NM_RS_PARAMETER

  NmRsArmInputData m_data;
};

//
// NmRsLegInputWrapper
//---------------------------------------------------------------------------
struct NmRsLegInputWrapper : public NmRsInputWrapperBase
{
  DECLARE_LIMB_INPUT_DATA(kLegPose)

  NmRsLegInputWrapper() :
    m_Hip(m_data.m_Hip),
    m_Knee(m_data.m_Knee),
    m_Ankle(m_data.m_Ankle)
  {
    init();
  }

  void init()
  {
    // Auto-generate effector data initialization...
    #define NM_RS_PARAMETER(_prefix, _name) INIT_EFFECTOR_DATA(_name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

  void setStiffness(float strength, float damping, float *stiffness = 0)
  {
    // Auto-generate setting effector params code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) SET_EFFECTOR_PARAMS(_prefix, _name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

  void setOpposeGravity(float oppose)
  {
    // Auto-generate oppose gravity code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) SET_OPPOSE_GRAVITY(_prefix, _name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

#if ART_ENABLE_BSPY_LIMBS
  void sendComponents(const char* /*name = 0*/, BehaviourID /* task = bvid_Invalid */)
  {
    // Auto-generate send components code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) SEND_COMPONENTS(_prefix, _name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }
#endif

  // Auto-generate members and accessors via inline file.
  #define NM_RS_PARAMETER(_prefix, _name) DECLARE_EFFECTOR_DATA(_prefix, _name)
  #include "common\NmRsHumanLeg.inl"
  #undef NM_RS_PARAMETER

  NmRsLegInputData m_data;
};

//
// NmRsSpineInputWrapper
//---------------------------------------------------------------------------
struct NmRsSpineInputWrapper : public NmRsInputWrapperBase
{
  DECLARE_LIMB_INPUT_DATA(kSpinePose)

  NmRsSpineInputWrapper() :
    m_Spine0(m_data.m_Spine0),
    m_Spine1(m_data.m_Spine1),
    m_Spine2(m_data.m_Spine2),
    m_Spine3(m_data.m_Spine3),
    m_LowerNeck(m_data.m_LowerNeck),
    m_UpperNeck(m_data.m_UpperNeck)
  {
    init();
  }

  void init()
  {
    // Auto-generate effector data initialization...
    #define NM_RS_PARAMETER(_prefix, _name) INIT_EFFECTOR_DATA(_name)
    #include "common\NmRsHumanSpine.inl"
    #undef NM_RS_PARAMETER
  }

  void setStiffness(float strength, float damping, float *stiffness = 0)
  {
    // Auto-generate setting effector params code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) SET_EFFECTOR_PARAMS(_prefix, _name)
    #include "common\NmRsHumanSpine.inl"
    #undef NM_RS_PARAMETER
  }

  void setOpposeGravity(float oppose)
  {
    // Auto-generate oppose gravity code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) SET_OPPOSE_GRAVITY(_prefix, _name)
    #include "common\NmRsHumanSpine.inl"
    #undef NM_RS_PARAMETER
  }

  void setBackAngles(float lean1, float lean2, float twist)
  {
    // div by 4 to spread value over 4 effectors
    lean1 *= 0.25f;
    lean2 *= 0.25f;
    twist *= 0.25f;

    twist = rage::Clamp(twist, -9.9f, 9.9f);
    lean1 = rage::Clamp(lean1, -9.9f, 9.9f);
    lean2 = rage::Clamp(lean2, -9.9f, 9.9f);

    m_Spine0.setDesiredAngles(lean1, lean2, twist);
    m_Spine1.setDesiredAngles(lean1, lean2, twist);
    m_Spine2.setDesiredAngles(lean1, lean2, twist);
    m_Spine3.setDesiredAngles(lean1, lean2, twist);
  }

  void setNeckAngles(float lean1, float lean2, float twist)
  {
    // div by 2 to spread value over 2 effectors
    lean1 *= 0.5f;
    lean2 *= 0.5f;
    twist *= 0.5f;

    getLowerNeck()->setDesiredAngles(lean1, lean2, twist);
    getUpperNeck()->setDesiredAngles(lean1, lean2, twist);
  }

  void applySpineLean(float lean1, float lean2)
  {
    float amount = lean1 * (1.0f / 6.0f);

    // set lean1s
    m_Spine0.setDesiredLean1(amount * 2.0f);
    m_Spine1.setDesiredLean1(amount * 2.0f);
    m_Spine2.setDesiredLean1(amount);
    m_Spine3.setDesiredLean1(amount);

    amount = lean2 * 0.25f;

    // set lean2s
    m_Spine0.setDesiredLean2(amount * 2.0f);
    m_Spine1.setDesiredLean2(amount * 2.0f);
    m_Spine2.setDesiredLean2(amount);
    m_Spine3.setDesiredLean2(amount);
  }

#if ART_ENABLE_BSPY_LIMBS
  void sendComponents(const char* /* name = 0 */, BehaviourID /* task = bvid_Invalid */)
  {
    // Auto-generate send components code via inline file
    #define NM_RS_PARAMETER(_prefix, _name) SEND_COMPONENTS(_prefix, _name)
    #include "common\NmRsHumanSpine.inl"
    #undef NM_RS_PARAMETER
  }
#endif

  // Auto-generate members and accessors via inline file.
  #define NM_RS_PARAMETER(_prefix, _name) DECLARE_EFFECTOR_DATA(_prefix, _name)
  #include "common\NmRsHumanSpine.inl"
  #undef NM_RS_PARAMETER

  NmRsSpineInputData m_data;
};

  //
  // NmRsSetStiffnessInputWrapper
  //---------------------------------------------------------------------------
  struct NmRsSetStiffnessInputWrapper : public NmRsInputWrapperBase
  {
    DECLARE_LIMB_INPUT_DATA(kSetStiffness)

    NmRsSetStiffnessInputWrapper()
    {
      m_stiffness = 9.0f;
      m_damping = 1.0f;
      m_muscleStiffness = 1.0f;
      m_applyMuscleStiffness = false;
    }

    void setValues(float stiffness, float damping, float* muscleStiffness = 0)
    {
      m_stiffness = stiffness;
      m_damping = damping;
      if(muscleStiffness)
      {
        m_muscleStiffness = *muscleStiffness;
        m_applyMuscleStiffness = true;
      }
    }

#if ART_ENABLE_BSPY_LIMBS
    void sendComponents(const char* name = 0, BehaviourID task = bvid_Invalid);
#endif

    float m_stiffness;
    float m_damping;
    float m_muscleStiffness;
    bool  m_applyMuscleStiffness;
  };

} // namespace ART

#endif // NM_RS_EFFECTOR_WRAPPER_H
