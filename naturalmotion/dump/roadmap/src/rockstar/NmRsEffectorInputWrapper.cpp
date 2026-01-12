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
#include "NmRsEffectorInputWrapper.h"
#include "NmRsEffectors.h"

#undef NM_RS_RO_PARAMETER
#define NM_RS_RO_PARAMETER(_prefix, _type, _name)
#undef NM_RS_RO_PARAMETER_ACTUAL
#define NM_RS_RO_PARAMETER_ACTUAL(_prefix, _type, _name)
#undef NM_RS_PARAMETER_DIRECT
#define NM_RS_PARAMETER_DIRECT(_prefix, _type, _name, _min, _max, _default)

namespace ART
{
#if ART_ENABLE_BSPY_LIMBS
  // support for easy vector type conversion in limb macros when input type
  // is not strictly known.
  template <typename IN_TYPE, typename OUT_TYPE>
  inline void convertForBspy2(IN_TYPE& in, OUT_TYPE& out)
  {
    out = in;
  }

  template <>
  inline void convertForBspy2(rage::Vector3& in, bSpy::bSpyVec3& out)
  {
    out = bSpyVec3fromVector3(in);
  }

#define SET_COMPONENT_PARAMETER(name)\
  if(m_flags & apply##name)\
  {\
  convertForBspy2(m_##name, cp.m_##name);\
  cp.m_##name##SetBy = 0;\
  cp.m_parameterSetFlags |= apply##name;\
  }
#endif

  // NmRs1DofEffectorInputDataBase
  //---------------------------------------------------------------------------

#if ART_ENABLE_BSPY_LIMBS
#undef SEND_BSPY_COMPONENT
#define SEND_BSPY_COMPONENT(_prefix, _type, _name)\
  if(m_data.m_flags & _prefix##InputData::apply##_name)\
  {\
  _type temp = get##_name();\
  convertForBspy2(temp, cp.m_##_name);\
  cp.m_##_name##SetBy = BSPY_INVALID_MSGTOKEN;\
  cp.m_parameterSetFlags |= _prefix##InputData::apply##_name;\
}
#endif

#define DEFINE_SET_CURRENT(_name)\
  m_data.set##_name(dof->get##_name());

  // NmRs1DofEffectorInputWrapper
  //---------------------------------------------------------------------------
#if ART_ENABLE_BSPY_LIMBS
  void NmRs1DofEffectorInputWrapper::sendComponents(const char* name, BehaviourID /* task */)
  {
    if (!m_data.m_flags)
      return;

    bSpy::LimbComponent1DofPacket cp(bSpyServer::inst()->getTokenForString(name));

    // Auto-generate send component from inline file.
    #undef NM_RS_PARAMETER
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) SEND_BSPY_COMPONENT(_prefix, _type, _name)
    #include "common\NmRs1DofEffector.inl"
    #undef NM_RS_PARAMETER

    bspySendPacket(cp);
  }
#endif

  void NmRs1DofEffectorInputWrapper::setToCurrent(const NmRsEffectorBase* eff)
  {
    Assert(!eff->is3DofEffector());
    const NmRs1DofEffector* dof = (const NmRs1DofEffector*)eff;
    // m_data.setDesiredAngle(dof->getDesiredAngle());

    // Auto-generate set current from inline file.
    #undef NM_RS_PARAMETER
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DEFINE_SET_CURRENT(_name)
    #include "common\NmRs1DofEffector.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRs1DofEffectorInputWrapper::setStiffness(float stiffness, float damping, float *muscleStiffness /*= 0*/)
  {
    setMuscleStrength(stiffness * stiffness);
    setMuscleDamping(2.0f * stiffness * damping);
    if (muscleStiffness)
      setMuscleStiffness(*muscleStiffness);
  }

  void NmRs1DofEffectorInputWrapper::setRelaxation(const NmRsEffectorBase* effector, float mult, float *pMultDamping /*= 0*/)
  {
    Assert(mult >= 0.0f && mult <= 1.0f);
    Assert(!effector->is3DofEffector());
    NmRs1DofEffector* eff1Dof = (NmRs1DofEffector*)effector;

    // we square the multiplier as the natural frequency (strength*strength) is what we want to multiply by mult.
    float clampedStrength = rage::Clamp(eff1Dof->getInfo().m_defaultMuscleStrength * mult*mult, 2.0f, 20.0f);
    setMuscleStrength(clampedStrength);

    if (pMultDamping)
    {
      Assert(*pMultDamping >= 0.0f && *pMultDamping <= 1.0f);
      setMuscleDamping(eff1Dof->getInfo().m_defaultMuscleDamping * *pMultDamping);
    }
    else
    {
      setMuscleDamping(eff1Dof->getInfo().m_defaultMuscleDamping * mult);
    }
  }

  void NmRs1DofEffectorInputWrapper::blendToZeroPose(const NmRsEffectorBase* effector, float t)
  {
    Assert(!effector->is3DofEffector());
    const NmRs1DofEffector* eff1Dof = (const NmRs1DofEffector*)effector;

    if (effector->hasStoredZeroPose())
      blendToSpecifiedPose(eff1Dof->getZeroPoseAngle(), t);
  }

  void NmRs1DofEffectorInputWrapper::blendToSpecifiedPose(float angle, float t)
  {
    t = rage::Clamp(t, 0.f, 1.f);
    float invT = 1.0f - t;
    float newDesiredAngle;

    // only blend if the message data has this value set. otherwise,
    // leave it alone.
      if (m_data.m_flags & NmRs1DofEffectorInputData::applyDesiredAngle)
    {
      newDesiredAngle = (getDesiredAngle() * invT) + (angle * t);
        setDesiredAngle(newDesiredAngle);
    }
  }

  void NmRs1DofEffectorInputWrapper::reset(const NmRsEffectorBase* effector, ResetEffectorsType resetType)
  {
    Assert(!effector->is3DofEffector());
    const NmRs1DofEffector* eff1Dof = (const NmRs1DofEffector*)effector;

    if (resetType & kResetAngles)
    {
      setDesiredAngle(0.0f);
    }
    if (resetType & kResetCalibrations)
    {
      setMuscleStiffness(eff1Dof->getInfo().m_defaultMuscleStiffness);
      setMuscleStrength(eff1Dof->getInfo().m_defaultMuscleStrength);
      setMuscleDamping(eff1Dof->getInfo().m_defaultMuscleDamping);
      setOpposeGravity(0.f);
    }
    if (resetType & kResetMuscleStiffness)
    {
      setMuscleStiffness(eff1Dof->getInfo().m_defaultMuscleStiffness);
    }
  }

  void NmRs1DofEffectorInputWrapper::activePose(const NmRsEffectorBase* effector, int transformSource)
  {
    Assert(effector);

    rage::Quaternion quat;
    NMutils::NMVector4 nmQ;
    if (!effector->getJointQuaternionFromIncomingTransform(nmQ, (IncomingTransformSource)transformSource))
      return;
    quat.Set(nmQ[1], nmQ[2], nmQ[3], nmQ[0]);

    rage::Vector3 tts = rsQuatToRageDriveTwistSwing(quat);

    setDesiredAngle(tts.x);
  }

  void NmRs1DofEffectorInputWrapper::holdPose(const NmRsEffectorBase* effector)
  {
    Assert(effector);
    Assert(!effector->is3DofEffector());
    const NmRs1DofEffector* eff1Dof = (const NmRs1DofEffector*)effector;
    setDesiredAngle(eff1Dof->getActualAngle());
  }

  // NmRs3DofEffectorInputWrapper
  //---------------------------------------------------------------------------

#if ART_ENABLE_BSPY_LIMBS
  void NmRs3DofEffectorInputWrapper::sendComponents(const char* name, BehaviourID /* task */)
  {
    if(!m_data.m_flags)
      return;

    bSpy::LimbComponent3DofPacket cp(bSpyServer::inst()->getTokenForString(name));

    // Auto-generate send bspy component from inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) SEND_BSPY_COMPONENT(_prefix, _type, _name)
    #include "common\NmRs3DofEffector.inl"
    #undef NM_RS_PARAMETER

    bspySendPacket(cp);
  }
#endif //ART_ENABLE_BSPY_LIMBS

  void NmRs3DofEffectorInputWrapper::setToCurrent(const NmRsEffectorBase* eff)
  {
    Assert(eff->is3DofEffector());
    const NmRs3DofEffector* dof = (const NmRs3DofEffector*)eff;
    //m_data.setDesiredLean1(dof->getDesiredLean1());

    // Auto-generate set current from inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DEFINE_SET_CURRENT(_name)
    #include "common\NmRs3DofEffector.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRs3DofEffectorInputWrapper::setStiffness(float stiffness, float damping, float *muscleStiffness /*= 0*/)
  {
    setMuscleStrength(stiffness * stiffness);
    setMuscleDamping(2.0f * stiffness * damping);
    if (muscleStiffness)
      setMuscleStiffness(*muscleStiffness);
  }

  void NmRs3DofEffectorInputWrapper::setRelaxation(const NmRsEffectorBase* effector, float mult, float *pMultDamping /*= 0*/)
  {
    Assert(mult >= 0.0f && mult <= 1.0f);
    Assert(effector->is3DofEffector());
    const NmRs3DofEffector* eff3Dof = (const NmRs3DofEffector*)effector;

    // we square the multiplier as the natural frequency (strength*strength) is what we want to multiply by mult.
    float clampedStrength = rage::Clamp(eff3Dof->getInfo().m_defaultMuscleStrength * mult*mult, 2.0f, 20.0f);
    setMuscleStrength(clampedStrength);

    if (pMultDamping)
    {
      Assert(*pMultDamping >= 0.0f && *pMultDamping <= 1.0f);
      setMuscleDamping(eff3Dof->getInfo().m_defaultMuscleDamping * *pMultDamping);
    }
    else
    {
      setMuscleDamping(eff3Dof->getInfo().m_defaultMuscleDamping * mult);
    }
  }

  void NmRs3DofEffectorInputWrapper::blendToZeroPose(const NmRsEffectorBase* effector, float t)
  {
    Assert(effector->is3DofEffector());
    const NmRs3DofEffector* eff3Dof = (const NmRs3DofEffector*)effector;

    if (effector->hasStoredZeroPose())
    {
      rage::Vector3 zeroPoseAngles = eff3Dof->getZeroPoseAngles();
      blendToSpecifiedPose(zeroPoseAngles, t);
    }
  }

  void NmRs3DofEffectorInputWrapper::reset(const NmRsEffectorBase* effector, ResetEffectorsType resetType)
  {
    Assert(effector->is3DofEffector());
    const NmRs3DofEffector* eff3Dof = (const NmRs3DofEffector*)effector;

    if (resetType & kResetAngles)
    {
      setDesiredAngles(0.0f ,0.0f ,0.0f);
    }
    if (resetType & kResetCalibrations)
    {
      setMuscleStiffness(eff3Dof->getInfo().m_defaultMuscleStiffness);
      setMuscleStrength(eff3Dof->getInfo().m_defaultMuscleStrength);
      setMuscleDamping(eff3Dof->getInfo().m_defaultMuscleDamping);
      setOpposeGravity(0.f);
    }
    if (resetType & kResetMuscleStiffness)
    {
      setMuscleStiffness(eff3Dof->getInfo().m_defaultMuscleStiffness);
    }
  }

  // twist, lean, lean
  void NmRs3DofEffectorInputWrapper::blendToSpecifiedPose(rage::Vector3& angles, float t)
  {
    float clampT = rage::Clamp(t, 0.0f, 1.0f);
    float invT = 1.0f - clampT;

    float twistResult, lean1Result, lean2Result;

    // only blend if the message data has this value set. otherwise,
    // leave it alone.
      if (m_data.m_flags & NmRs3DofEffectorInputData::applyDesiredTwist)
    {
      twistResult = (getDesiredTwist() * invT) + (angles.x * clampT);
        setDesiredTwist(twistResult);
    }
      if (m_data.m_flags & NmRs3DofEffectorInputData::applyDesiredTwist)
    {
      lean1Result = (getDesiredLean1() * invT) + (angles.y * clampT);
        setDesiredLean1(lean1Result);
    }
      if (m_data.m_flags & NmRs3DofEffectorInputData::applyDesiredTwist)
    {
      lean2Result = (getDesiredLean2() * invT) + (angles.z * clampT);
        setDesiredLean2(lean2Result);
    }
  }

  void NmRs3DofEffectorInputWrapper::activePose(const NmRsEffectorBase* effector, int transformSource)
  {
    Assert(effector);
    Assert(effector->is3DofEffector());
    const NmRs3DofEffector* eff3Dof = (const NmRs3DofEffector*)effector;

    rage::Quaternion quat;
    NMutils::NMVector4 nmQ;
    if (!effector->getJointQuaternionFromIncomingTransform(nmQ, (IncomingTransformSource)transformSource))
      return;
    quat.Set(nmQ[1], nmQ[2], nmQ[3], nmQ[0]);

    rage::Vector3 tts = rsQuatToRageDriveTwistSwing(quat);
    eff3Dof->getTwistAndSwingFromRawTwistAndSwing(tts, tts);

    setDesiredAngles(tts.y, tts.z, tts.x);
  }

  void NmRs3DofEffectorInputWrapper::holdPose(const NmRsEffectorBase* effector)
  {
    Assert(effector);
    Assert(effector->is3DofEffector());
    const NmRs3DofEffector* eff3Dof = (const NmRs3DofEffector*)effector;
    setDesiredAngles(eff3Dof->getActualLean1(), eff3Dof->getActualLean2(),eff3Dof->getActualTwist());
  }

  void NmRs3DofEffectorInputWrapper::clamp(const NmRsEffectorBase* effector, float amount)
  {
    Assert(effector);
    Assert(effector->is3DofEffector());
    const NmRs3DofEffector* eff3Dof = (const NmRs3DofEffector*)effector;

    rage::Vector3 tll(getDesiredTwist(), getDesiredLean1(), getDesiredLean2());
    eff3Dof->clamp(tll, amount);
    setDesiredAngles(tll.y, tll.z, tll.x);
  }


#if ART_ENABLE_BSPY_LIMBS
  void NmRsSetStiffnessInputWrapper::sendComponents(const char* /*name*/, BehaviourID /*task*/)
  {
    LimbComponentSetStiffnessPacket cp;

    cp.m_stiffness = m_stiffness;
    cp.m_damping = m_damping;
    cp.m_muscleStiffness = m_muscleStiffness;
    cp.m_applyMuscleStiffness = m_applyMuscleStiffness;

    bspySendPacket(cp);
  }
#endif //ART_ENABLE_BSPY_LIMBS

} // namespace ART
