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
#include "NmRsShadows.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsUtils.h"
#include "NmRsTypeUtils.h"
#include "NmRsCBU_Shared.h"

namespace ART
{
  NmRs1DofEffector::NmRs1DofEffector(ART::MemoryManager* services, rage::phJoint1Dof* joint, int jointIndex, int jointTypeIndex, NmRs1DofEffectorParams &info ) : NmRsEffectorBase(false, services, jointIndex, jointTypeIndex), 
    m_info(info),
    m_1DofJoint(joint),
    m_zeroPoseAngle(0)
  {    
    // store angle information, including mid point and extents
    // this stuff never changes, so it can be safely cached
    m_MinAngle = m_info.minAngle;
    m_MaxAngle = m_info.maxAngle;

    float midAngle = (m_info.maxAngle + m_info.minAngle) * 0.5f;
    m_MidAngle = (midAngle);

    float extent = (m_info.maxAngle - m_info.minAngle) * 0.5f;
    m_Extent = (extent);

    joint->SetDriveState(rage::phJoint::DRIVE_STATE_ANGLE_AND_SPEED);
    joint->SetStiffness(0.2f);

    initialiseData();
  }

  void NmRs1DofEffector::initialiseData()
  {
    NmRsEffectorBase::initialiseData();

    // reset to defaults
    resetEffectorCalibrations();
    resetAngles();

    // reset the injuries
    setInjured(0.f); 
    m_MuscleDampingScaling = 1.0;
    m_MuscleStrengthScaling = 1.0;
    m_MuscleStiffnessScaling = 1.0;

    // reset the zeroPose
    state.m_zeroPoseStored = false;
    m_zeroPoseAngle = 0.f;
    //reset posture/gravityCompensation
    state.m_partOfGroundedChain = false;

    state.m_limitsSet = false;
    state.m_limitsSetThisFrame = false;

    m_ActualAngle = 0;
    m_ActualAngleVel = 0;
    state.m_actualAnglesValid = false;

#if NM_RUNTIME_LIMITS
    cacheCurrentLimits();
#endif
  }

  void NmRs1DofEffector::init(NmRsCharacter *character)
  {
    m_character = character;
    initialiseData();
    updateCurrentAngles(); // HD: ensure current-angle values are valid on first frame

#if NM_RUNTIME_LIMITS
    setLimitsToPose(true);// open limits to accommodate actual starting pose.
#endif
  }

  void NmRs1DofEffector::term()
  {
    m_character = 0;
  }

  void NmRs1DofEffector::saveToShadow(Shadow1Dof& state) const
  {
    state.m_desiredAngle = m_DesiredAngle;
    state.m_actualAngle = m_ActualAngle;
    state.m_extent = m_Extent;
    state.m_midAngle = m_MidAngle;
    state.m_muscleDamping = m_MuscleDamping;
    state.m_muscleStrength = m_MuscleStrength;
    state.m_position = getJointPosition();
    getMatrix1(state.m_matrix1);
    getMatrix2(state.m_matrix2);
  }

  void NmRs1DofEffector::loadFromShadow(Shadow1Dof& state)
  {
    setDesiredAngle(rage::Clamp(state.m_desiredAngle,-10.f,10.f));
    // HDD TODO should restore muscle values too?
  }

  void NmRs1DofEffector::preStep(float dtClamped, float minMuscleDamping)
  {    

#if ART_ENABLE_BSPY
    if ((m_MuscleStrengthScaling < 1.0f || m_MuscleDampingScaling < 1.0f || m_MuscleStiffnessScaling < 1.0f) ||
      (m_MuscleStrengthScaling > 1.01f || m_MuscleDampingScaling > 1.01f || m_MuscleStiffnessScaling > 1.01f))
    {
      char ctrStr [30];
      sprintf(ctrStr, "Looseness %i ", m_jointIndex);
      bspyScratchpad(m_character->getBSpyID(), ctrStr, m_MuscleStrengthScaling);
      bspyScratchpad(m_character->getBSpyID(), ctrStr, m_MuscleDampingScaling);
      bspyScratchpad(m_character->getBSpyID(), ctrStr, m_MuscleStiffnessScaling);
    }
#endif

    state.m_jointMatrixCacheValid = false;
    state.m_jointQuatFromITMValid = false;

#if defined (NM_RS_VALIDATE_VITAL_VALUES) && ART_ENABLE_BSPY
    // desiredAngles are extreme, or QNAN (there's a divide by 0)
	if (rage::Abs(m_DesiredAngle) > 12.0f || m_DesiredAngle != m_DesiredAngle)
      Displayf("NM 1Dof[%d], setBy %s: d-angle = %.5f", getJointIndex(), s_bvIDNames[m_DesiredAngleSetBy + 1], m_DesiredAngle);
#endif // NM_RS_VALIDATE_VITAL_VALUES

    const float extendAmount = 1.6f;
    // get extent and midAngle from joint, as joint limits may be different from effector limits.
    // due to NM_RUNTIME_LIMITS code
    // extents are only used to determine extent clamping here.
    rage::phJoint1Dof* oneDof = (rage::phJoint1Dof*)getJoint();
    Assert(oneDof);
    float minAngle;
    float maxAngle;
    oneDof->GetAngleLimits(minAngle,maxAngle);
    float extent = (maxAngle - minAngle) * 0.5f;
    float midAngle = (maxAngle + minAngle) * 0.5f;
    m_DesiredAngle = rage::Clamp(m_DesiredAngle, midAngle - extent * extendAmount, midAngle + extent * extendAmount);

    // this calculation increases stiffness automatically with muscle strength and damping, allowing larger strength values without instability
    m_MuscleDamping = rage::Max(m_MuscleDamping, minMuscleDamping);// can't go less than the natural damping of the joints


    //  Apply the injury to the joint. NOTE: m_Damping is calculated with unInjured strength!!
    float muscleStrengthAfterInjury = m_MuscleStrengthScaling*m_MuscleStrength;
    float muscleDampingAfterInjury = m_MuscleDampingScaling*m_MuscleDamping;
    float muscleStiffnessAfterInjury = m_MuscleStiffnessScaling*m_MuscleStiffness;
    if (m_injuryAmount != 0.f)
      applyInjuryToEffector(muscleStrengthAfterInjury,muscleDampingAfterInjury);

    muscleDampingAfterInjury = rage::Max(muscleDampingAfterInjury, minMuscleDamping);// can't go less than the natural damping of the joints
    float f2 = (muscleStrengthAfterInjury + 8.f*muscleDampingAfterInjury)*0.5f*muscleStiffnessAfterInjury*dtClamped;
    float internalStiffness = rage::Max(0.1f*dtClamped*60.f, f2/(f2+1));
    internalStiffness = rage::Min(internalStiffness,0.9999f);//Otherwise internalStiffness will be > 1 if dtClamped/timeStep > 1/6. Should we clamp dtClamped [1/60,1/30] so this code would never be needed 
#ifdef NM_RS_VALIDATE_VITAL_VALUES
    Assert((internalStiffness == internalStiffness));
#endif // NM_RS_VALIDATE_VITAL_VALUES

    // this code means force is capped with distance from target (pos or vel) but not capped with respect to internal stiffness
    Assert(1.f-internalStiffness > 0.f);

    m_1DofJoint->SetStiffness(internalStiffness);
    m_1DofJoint->SetMinAndMaxMuscleTorque(m_ForceCap / (1.f - internalStiffness)); 
    m_1DofJoint->SetMuscleAngleStrength(muscleStrengthAfterInjury);
    m_1DofJoint->SetMuscleSpeedStrength(muscleDampingAfterInjury);
    m_1DofJoint->SetMuscleTargetAngle(m_DesiredAngle);
    m_1DofJoint->SetMuscleTargetSpeed(0.f);
  }

  void NmRs1DofEffector::postStep()
  {
#if NM_RUNTIME_LIMITS
    restoreLimits(NM_RUNTIME_LIMITS_RECOVERY_TIME);
#endif
    state.m_limitsSetThisFrame = false;
    state.m_actualAnglesValid = false;
  }

  void NmRs1DofEffector::ApplyTorque(float torque)
  {
    if (NmRsCharacter::sm_ApplyForcesImmediately)
      get1DofJoint()->ApplyTorque(m_character->getArticulatedBody(), torque, rage::ScalarV(m_character->getLastKnownUpdateStep()).GetIntrin128ConstRef());
    else
    {
      rage::Vector3 vecInput(torque,torque,torque);
      m_character->AddDeferredForce(NmRsCharacter::kJointTorque1Dof, (rage::u8) m_jointIndex, vecInput, vecInput);
    }
  }


  void NmRs1DofEffector::ApplyAngImpulse(float impulse)
	{
		rage::ScalarV vMag;
		vMag.Setf(impulse);
		if (NmRsCharacter::sm_ApplyForcesImmediately)
			get1DofJoint()->ApplyAngImpulse(m_character->getArticulatedBody(), vMag);
		else
		{
			rage::Vector3 vecInput(impulse,impulse,impulse);
			m_character->AddDeferredForce(NmRsCharacter::kJointAngImpulse1Dof, (rage::u8) m_jointIndex, vecInput, vecInput);
		}
	}

  void NmRs1DofEffector::setInjured(float injuryAmount NM_RS_SETBY_PARAMS_UNUSED)
    {
      m_injuryAmount = rage::Clamp(injuryAmount,0.f,1.f);
    }

  void NmRs1DofEffector::applyInjuryToEffector(float &effeciveMStrength,float &effeciveMDamping)
  {
    effeciveMStrength = rage::Max(0.1f, effeciveMStrength*(1.f - getInjuryAmount())*(1.f - getInjuryAmount()));
    effeciveMDamping = effeciveMDamping*(1.f-getInjuryAmount());
  }

  void NmRs1DofEffector::updateCurrentAngles() const
  {
    m_1DofJoint->ComputeCurrentAngle(m_character->getArticulatedBody());

	// getActualX functions, which call updateCurrentAngles are effectively const
    // functions from a usage perspective.
	float *actualAngle = const_cast<float*>(&m_ActualAngle);
	EffectorBitField *_state = const_cast<EffectorBitField*>(&state);
	
	// these now assume that ->ComputeCurrentAngle() has been called during the solve
    *actualAngle = m_1DofJoint->GetComputedAngle();

#if CRAWL_LEARNING
	float *actualAngleVel = const_cast<float*>(&m_ActualAngleVel);
    *actualAngleVel = m_1DofJoint->GetAngularSpeed(m_character->getArticulatedBody());
#endif

	_state->m_actualAnglesValid = true;
  }

  void NmRs1DofEffector::setDesiredAngleRelative(float angle)
  {
    setDesiredAngle( getMidAngle() + (getExtent() * angle) );
  }

  float NmRs1DofEffector::getDesiredAngleFromRelative(float angle) const
  {
    return (getMidAngle() + (getExtent() * angle));
  }

  void NmRs1DofEffector::setDesiredAngleZeroRelative(float angle)
  {
    setDesiredAngle(m_zeroPoseAngle + angle );
  }

  void NmRs1DofEffector::getQuaternionFromDesiredAngles(rage::Quaternion &q) const
  {
    rage::Vector3 tts(getDesiredAngle(), 0, 0);
    q = rsRageDriveTwistSwingToQuat(tts);
  }

  void NmRs1DofEffector::getQuaternionFromDesiredRawAngles(rage::Quaternion &q) const
  {
    rage::Vector3 tts(getDesiredAngle(), 0, 0);
    // todo: adjust for extra leans?
    q = rsRageDriveTwistSwingToQuat(tts);
  }

  void NmRs1DofEffector::activePose(int transformSource)
  {
    rage::Quaternion quat;
    NMutils::NMVector4 nmQ;
    if (!getJointQuaternionFromIncomingTransform(nmQ, (IncomingTransformSource)transformSource))
      return;
    quat.Set(nmQ[1], nmQ[2], nmQ[3], nmQ[0]);
    rage::Vector3 tts = rsQuatToRageDriveTwistSwing(quat);      

    setDesiredAngle(tts.x);
  }
  // TDL return back the angles / angle vels from the incoming transforms without setting them
  void NmRs1DofEffector::activeAnimInfo(float timeStep, float *angle, float *angleVel) const
  {
    rage::Quaternion quat;
    rage::Vector3 rotVel;
    if (!getJointQuatPlusVelFromIncomingTransform(quat, rotVel))
      return;

    *angleVel = rotVel.z/timeStep;
    rage::Vector3 axis;
    rage::Vector3 tts = rsQuatToRageDriveTwistSwing(quat);      
    *angle = tts.x;
  }
  void NmRs1DofEffector::holdPose()
  {
    setDesiredAngle(getActualAngle());
  }

  bool NmRs1DofEffector::storeZeroPose()
  {
    state.m_zeroPoseStored = false;

    NMutils::NMVector4 nmQ;
    if (!getJointQuaternionFromIncomingTransform_uncached(nmQ))
      return false;

    rage::Quaternion q;
    q.Set(nmQ[1], nmQ[2], nmQ[3], nmQ[0]);
    rage::Vector3 tts = rsQuatToRageDriveTwistSwing(q);

    m_zeroPoseAngle = tts.x;

    // for some reason, calculateJointQuatFromITMCache()
    // is flipping at around 2 rad. 3dofs rarely (never?)
    // drive this far, so not normally seen except on 1dofs.
    // temporary fix.
    if(m_zeroPoseAngle > PI)
      m_zeroPoseAngle = m_zeroPoseAngle - 2 * PI;
    else if(m_zeroPoseAngle < -PI)
      m_zeroPoseAngle = m_zeroPoseAngle + 2 * PI;

    state.m_zeroPoseStored = true;

    return true;
  }

  void NmRs1DofEffector::blendToZeroPose(float t  NM_RS_SETBY_PARAMS_UNUSED)
  {
    if (state.m_zeroPoseStored)
    {
      // do linear blend with current desired angle
      float clampT = rage::Clamp(t, 0.0f, 1.0f);
      float invT = 1.0f - clampT;
      float resultingAngle = (getDesiredAngle() * invT) + (m_zeroPoseAngle * clampT);

      setDesiredAngle(resultingAngle);
    }
  }

  void NmRs1DofEffector::blendToPose(float angle, float t)
  {
    // do linear blend with current desired angle
    float clampT = rage::Clamp(t, 0.0f, 1.0f);
    float invT = 1.0f - clampT;
    float resultingAngle = (getDesiredAngle() * invT) + (angle * clampT);

    setDesiredAngle(resultingAngle);
  }

    void NmRs1DofEffector::setRelaxation(float mult NM_RS_SETBY_PARAMS_UNUSED)
  {
    Assert(mult >= 0.0f && mult <= 1.0f);
    float clampedStrength = rage::Clamp(m_info.m_defaultMuscleStrength * mult*mult, 2.0f, 20.0f);
    setMuscleStrength(clampedStrength); // we square the multiplier as the natural frequency (strength*strength) is what we want to multiply by mult.
    setMuscleDamping(m_info.m_defaultMuscleDamping * mult);
  }

    void NmRs1DofEffector::setRelaxation_DampingOnly(float mult NM_RS_SETBY_PARAMS_UNUSED)
  {
    Assert(mult >= 0.0f && mult <= 1.0f);
    setMuscleDamping(m_info.m_defaultMuscleDamping * mult);
  }

  void NmRs1DofEffector::resetEffectorCalibrations()
  {
    setForceCap(m_info.m_defaultForceCap);
    setMuscleStiffness(m_info.m_defaultMuscleStiffness);
    setMuscleStrength(m_info.m_defaultMuscleStrength);
    setMuscleDamping(m_info.m_defaultMuscleDamping);
    setOpposeGravity(0.f);
  }

  void NmRs1DofEffector::resetEffectorMuscleStiffness()
  {
    setMuscleStiffness(m_info.m_defaultMuscleStiffness);
  }

  void NmRs1DofEffector::resetAngles()
  {
    setDesiredAngle(0.0f);
  }

  void NmRs1DofEffector::setStiffness(float stiffness, float damping, float *muscleStiffness)
  {
    setMuscleStrength(stiffness*stiffness);
    setMuscleDamping(2.f*damping*stiffness);
    if (muscleStiffness)
      setMuscleStiffness(*muscleStiffness);
  }

#if NM_RUNTIME_LIMITS
  void NmRs1DofEffector::cacheCurrentLimits()
  {
    const rage::phJoint1Dof* oneDof = (rage::phJoint1Dof*)getJoint();
    Assert(oneDof);
    float minAngle, maxAngle;
    oneDof->GetAngleLimits(minAngle, maxAngle);
    // compensate for SetAngleLimits(), which subtracts rage::phSimulator::GetAllowedAnglePenetration().
    m_minLimitCache = minAngle - rage::phSimulator::GetAllowedAnglePenetration();
    m_maxLimitCache = maxAngle + rage::phSimulator::GetAllowedAnglePenetration();
  }

  void NmRs1DofEffector::disableLimits()
  {
    setLimits(-(PI-0.1f),PI-0.1f);
  }

  void NmRs1DofEffector::restoreLimits(float step)
  {
    if(state.m_limitsSet && !state.m_limitsSetThisFrame)
    {
      if(step == 0.f)
      {
        setLimits(m_minLimitCache, m_maxLimitCache);
      }
      else
      {
        float adjustedStep = step * m_character->getLastKnownUpdateStep();
        rage::phJoint1Dof* oneDof = (rage::phJoint1Dof*)getJoint();
        Assert(oneDof);
        float limitMin, limitMax;
        oneDof->GetAngleLimits(limitMin, limitMax);
        // compensate for SetAngleLimits(), which subtracts rage::phSimulator::GetAllowedAnglePenetration().
        limitMin -= rage::phSimulator::GetAllowedAnglePenetration();
        limitMax += rage::phSimulator::GetAllowedAnglePenetration();
        float limitMaxStep = m_maxLimitCache - limitMax;
        limitMaxStep = rage::Clamp(limitMaxStep, -adjustedStep, adjustedStep);
        limitMax += limitMaxStep;
        float limitMinStep = m_minLimitCache - limitMin;
        limitMinStep = rage::Clamp(limitMinStep, -adjustedStep, adjustedStep);
        limitMin += limitMinStep;

        setLimits(limitMin, limitMax);

        if(limitMin == m_minLimitCache && limitMax == m_maxLimitCache)
        {
          state.m_limitsSet = false;
          state.m_limitsSetThisFrame = false;
        }
      }
    }
  }

  void NmRs1DofEffector::setLimits(float min, float max)
  {
    state.m_limitsSet = true;
    state.m_limitsSetThisFrame = true;

    rage::phJoint1Dof* oneDof = (rage::phJoint1Dof*)getJoint();
    Assert(oneDof); 
    Assert(max >= min);
    oneDof->SetAngleLimits(min,max, 
      true);  // this is to make sure that the type data is not modified.  Only adjusts for the fragInst.
#if NM_RUNTIME_LIMITS_IK
    float extent = (max - min) * 0.5f;
    m_Extent = (extent);
    //note m_MidAngle stays the same as RunTimeLimits add symmetrically to the min and max
    m_MinAngle = min;
    m_MaxAngle = max;
#endif

#if ART_ENABLE_BSPY && 0
    if (m_character->getBSpyID() != INVALID_AGENTID)
    {  
      EffectorModifyLimitsPacket eflp((bs_uint16)m_character->getBSpyID(), (bs_uint8)m_jointIndex);

      eflp.m_minAngles.v[0]   = min;
      eflp.m_minAngles.v[1]   = 0;
      eflp.m_minAngles.v[2]   = 0;

      eflp.m_maxAngles.v[0]   = max;
      eflp.m_maxAngles.v[1]   = 0;
      eflp.m_maxAngles.v[2]   = 0;

      eflp.m_cacheUpdateID = (bs_uint32)bSpyServer::inst()->getFrameTicker();

      bspySendPacket(eflp);
    }
#endif // ART_ENABLE_BSPY

#if ART_ENABLE_BSPY
    if(this->getJointIndex() == gtaJtElbow_Left)
    {
      rage::Vector3 cached(m_minLimitCache, m_maxLimitCache, 0.f); 
      bspyScratchpad(m_character->getBSpyID(), "elbow.limit", cached);
      rage::Vector3 current(min, max, 0.f); 
      bspyScratchpad(m_character->getBSpyID(), "elbow.limit", current);
    }
#endif
  }

  void NmRs1DofEffector::setLimitsToPose(bool useActual /* = false */, float margin /* = 0.f */)
  {
    float angle;
    if(useActual)
      angle = getActualAngle();
    else
      angle = getDesiredAngle();
    Assert(angle > -PI && angle < PI);
    if(angle-margin < m_minLimitCache)
      setLimits(angle-margin, m_maxLimitCache);
    else if(angle+margin > m_maxLimitCache)
      setLimits(m_minLimitCache, angle+margin);
    else
      setLimits(m_minLimitCache, m_maxLimitCache);
  }
#endif // NM_RUNTIME_LIMITS

#if ART_ENABLE_BSPY && NM_BSPY_JOINT_DEBUG
  void NmRs1DofEffector::renderDebugDraw()
  {
    BehaviourMask mask = NM_BSPY_JOINT_DEBUG_MASK;
    if(!m_character->isEffectorInMask(mask, m_jointIndex))
      return;
    const int hingeSides = 4;

    rage::Matrix34 mat;
    rage::Vector3 a, b, col(0, 1, 0), col2(0.0f, 0.3f, 1.0f), col3(1.0f, 0.3f, 0.0f);
    const float scale = 0.2f;

#if 0
    float angMin = getMinAngle(), angMax = getMaxAngle(), t, angle;
    float angRange = angMax - angMin;
#else
    rage::phJoint1Dof* oneDof = (rage::phJoint1Dof*)getJoint();
    Assert(oneDof);
    float angMin, angMax, t, angle;
    oneDof->GetAngleLimits(angMin, angMax);
    float angRange = angMax - angMin;
#endif

    getMatrix1(mat);
    a = mat.d;

    // draw extents of hinge
    b.Set(rage::Sinf(-angMax) * scale, rage::Cosf(-angMax) * scale, 0);
    mat.Transform(b);
    m_character->bspyDrawLine(a, b, col);

    b.Set(rage::Sinf(-angMin) * scale, rage::Cosf(-angMin) * scale, 0);
    mat.Transform(b);
    m_character->bspyDrawLine(a, b, col);

    // then draw range
    for (int i=0; i<=hingeSides; i++)
    {
      t = (float)i * (PI / hingeSides);
      angle = angMin + (rage::Sinf(t) * angRange);

      a = b;
      b.Set(rage::Sinf(-angle) * scale, rage::Cosf(-angle) * scale, 0);

      mat.Transform(b);
      m_character->bspyDrawLine(a, b, col);
    }
    angle = getDesiredAngle();
    b.Set(rage::Sinf(-angle) * scale, rage::Cosf(-angle) * scale, 0);
    mat.Transform(b);
    m_character->bspyDrawLine(a, b, col2);
    angle = getActualAngle();
    b.Set(rage::Sinf(-angle) * scale, rage::Cosf(-angle) * scale, 0);
    mat.Transform(b);

    m_character->bspyDrawLine(a, b, col3);
  }
#endif

#if ART_ENABLE_BSPY
  void NmRs1DofEffector::sendDescriptor()
  {
    Effector1DofDescriptorPacket dp((bs_uint8)m_jointIndex, (bs_uint8)m_jointTypeIndex);

    dp.d.m_nameToken    = m_nameToken;
    dp.d.m_parentIndex  = (bs_uint8)m_info.parentIndex;
    dp.d.m_childIndex   = (bs_uint8)m_info.childIndex;

    dp.d.m_minAngle     = getMinAngle();
    dp.d.m_maxAngle     = getMaxAngle();

    dp.d.m_positionChild  = bSpyVec3fromVector3(get1DofJoint()->GetPositionChild());
    dp.d.m_positionParent = bSpyVec3fromVector3(get1DofJoint()->GetPositionParent());
    rage::Matrix34 orientChild;
    get1DofJoint()->GetOrientationChild(orientChild);
    dp.d.m_orientChild    = bSpyMat34fromMatrix34(orientChild);
    dp.d.m_orientParent   = bSpyMat34fromMatrix34(get1DofJoint()->GetOrientationParent());

    bspySendPacket(dp);
  }

  void NmRs1DofEffector::sendUpdate()
  {
    Effector1DofUpdatePacket pp((bs_uint8)m_jointIndex, (bs_uint8)m_jointTypeIndex);

    pp.d.m_desired        = getDesiredAngle();
    pp.d.m_actual         = getActualAngle();
    pp.d.m_actualVel      = getActualAngleVel();
    pp.d.m_zeroPose       = m_zeroPoseAngle;

    pp.d.m_stiffness      = getMuscleStiffness();
    pp.d.m_strength       = getMuscleStrength();
    pp.d.m_damping        = getMuscleDamping();

    pp.d.m_stiffnessScale = m_MuscleStiffnessScaling;
    pp.d.m_strengthScale  = m_MuscleStrengthScaling;
    pp.d.m_dampingScale   = m_MuscleDampingScaling;
    pp.d.m_injury         = getInjuryAmount();

    pp.d.m_desiredAngleSetBy    = m_DesiredAngleSetBy;
    pp.d.m_muscleStiffnessSetBy = m_MuscleStiffnessSetBy;
    pp.d.m_muscleStrengthSetBy  = m_MuscleStrengthSetBy;
    pp.d.m_muscleDampingSetBy   = m_MuscleDampingSetBy;

    pp.d.m_desiredAngleSetByFrame = m_DesiredAngleSetByFrame;
    pp.d.m_muscleStiffnessSetByFrame = m_MuscleStiffnessSetByFrame;
    pp.d.m_muscleStrengthSetByFrame = m_MuscleStrengthSetByFrame;
    pp.d.m_muscleDampingSetByFrame = m_MuscleDampingSetByFrame;

    pp.d.m_opposeGravity  = getOpposeGravity();

    NMutils::NMVector4 q;
    getJointQuaternionFromIncomingTransform(q);

    pp.d.m_itmDriveQuat[0] = q[0];
    pp.d.m_itmDriveQuat[1] = q[1];
    pp.d.m_itmDriveQuat[2] = q[2];
    pp.d.m_itmDriveQuat[3] = q[3];

    rage::Matrix34 mat1;
    getMatrix1(mat1);
    pp.d.m_matrix1        = bSpyMat34fromMatrix34(mat1);

    bspySendPacket(pp);
  }
#endif // ART_ENABLE_BSPY
}

