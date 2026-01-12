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
  NmRs3DofEffector::NmRs3DofEffector(ART::MemoryManager* services, rage::phJoint3Dof* joint, int jointIndex, int jointTypeIndex, NmRs3DofEffectorParams &info ) : NmRsEffectorBase(true, services, jointIndex, jointTypeIndex), 
    m_zeroPoseAngles(0, 0, 0),
    m_3DofJoint(joint),
    m_info(info)
  {
    initialiseData();

    // calculate and store (local and Lua) min/max on each dof
    m_MinLean1 = m_info.reverseFirstLeanMotor ? -m_info.maxFirstLeanAngle : m_info.minFirstLeanAngle;
    m_MaxLean1 = m_info.reverseFirstLeanMotor ? -m_info.minFirstLeanAngle : m_info.maxFirstLeanAngle;
    m_MinLean2 = m_info.reverseSecondLeanMotor ? -m_info.maxSecondLeanAngle : m_info.minSecondLeanAngle;
    m_MaxLean2 = m_info.reverseSecondLeanMotor ? -m_info.minSecondLeanAngle : m_info.maxSecondLeanAngle;
    m_MinTwist = m_info.reverseTwistMotor ? -m_info.maxTwistAngle : m_info.minTwistAngle;
    m_MaxTwist = m_info.reverseTwistMotor ? -m_info.minTwistAngle : m_info.maxTwistAngle;

    // work out mid and extents including motor reversal, store local and Lua
    m_MidLean1    = (m_MinLean1 + m_MaxLean1) * 0.5f;
    m_MidLean2    = (m_MinLean2 + m_MaxLean2) * 0.5f;
    m_MidTwist    = (m_MinTwist + m_MaxTwist) * 0.5f;
    m_Lean1Extent = (m_MaxLean1 - m_MinLean1) * 0.5f;
    m_Lean2Extent = (m_MaxLean2 - m_MinLean2) * 0.5f;
    m_TwistExtent = (m_MaxTwist - m_MinTwist) * 0.5f;

    joint->SetDriveState(rage::phJoint::DRIVE_STATE_ANGLE_AND_SPEED);
    joint->SetStiffness(0.2f);
  }

  void NmRs3DofEffector::initialiseData()
  {
    NmRsEffectorBase::initialiseData();

    // reset to defaults
    resetEffectorCalibrations();
    resetAngles();
    setInjured(0.f); 
    m_MuscleDampingScaling = 1.0;
    m_MuscleStrengthScaling = 1.0;
    m_MuscleStiffnessScaling = 1.0;
    state.m_zeroPoseStored = false;
    state.m_partOfGroundedChain = false;

    state.m_limitsSet = false;
    state.m_limitsSetThisFrame = false;

    m_zeroPoseAngles.Zero();

    m_ActualLean1 = 
    m_ActualLean2 = 
    m_ActualTwist = 
    m_ActualLean1Vel = 
    m_ActualLean2Vel = 
    m_ActualTwistVel = 0.0f;

    state.m_actualAnglesValid = false;

#if NM_RUNTIME_LIMITS
    cacheCurrentLimits();
#endif
  }

  void NmRs3DofEffector::init(NmRsCharacter *character)
  {
    m_character = character;
    initialiseData();
    m_3DofJoint->ComputeCurrentLeanAndTwist(m_character->getArticulatedBody());//Reset twist that is remembered from last performance
    updateCurrentAngles(); // HD: ensure current-angle values are valid on first frame

#if NM_RUNTIME_LIMITS
    setLimitsToPose(true);// open limits to accommodate actual starting pose.
#endif
  }

  void NmRs3DofEffector::term()
  {
    m_character = 0;
  }

  void NmRs3DofEffector::saveToShadow(Shadow3Dof& state) const
  {
    state.m_desiredLean1 = m_DesiredLean1;
    state.m_desiredLean2 = m_DesiredLean2;
    state.m_desiredTwist = m_DesiredTwist;
    state.m_actualLean1 = getActualLean1();
    state.m_actualLean2 = getActualLean2();
    state.m_actualTwist = getActualTwist();
    state.m_twistExtent = m_TwistExtent;
    state.m_lean1Extent = m_Lean1Extent;
    state.m_lean2Extent = m_Lean2Extent;
    state.m_midLean1 = m_MidLean1;
    state.m_midLean2 = m_MidLean2;
    state.m_midTwist = m_MidTwist;
    state.m_reverseLean1 = m_info.reverseFirstLeanMotor;
    state.m_reverseLean2 = m_info.reverseSecondLeanMotor;
    state.m_reverseTwist = m_info.reverseTwistMotor;
    state.m_muscleDamping = m_MuscleDamping;
    state.m_muscleStrength = m_MuscleStrength;
    state.m_position = getJointPosition();
    getMatrix1(state.m_matrix1);
    getMatrix2(state.m_matrix2);
  }

  void NmRs3DofEffector::loadFromShadow(Shadow3Dof& state)
  {
    nmrsSetAngles(this, rage::Clamp(state.m_desiredLean1,-10.f,10.f), rage::Clamp(state.m_desiredLean2,-10.f,10.f), rage::Clamp(state.m_desiredTwist,-10.f,10.f));
  }

  float NmRs3DofEffector::extentToLimit() const
  {
#if NM_RUNTIME_LIMITS_IK
    float l1 = (m_DesiredLean1 - m_MidLean1)/m_Lean1Extent;
    float l2 = (m_DesiredLean2 - m_MidLean2)/m_Lean2Extent;
    // reversal code
    if (m_info.reverseFirstLeanMotor)
      l1 *= -1.f;;
    if (m_info.reverseSecondLeanMotor)
      l2 *= -1.f;;
#else
    float midLean1 = (m_info.minFirstLeanAngle + m_info.maxFirstLeanAngle)*0.5f;
    float midLean2 = (m_info.minSecondLeanAngle + m_info.maxSecondLeanAngle)*0.5f;
    float extentLean1 = (m_info.maxFirstLeanAngle - m_info.minFirstLeanAngle)*0.5f;
    float extentLean2 = (m_info.maxSecondLeanAngle - m_info.minSecondLeanAngle)*0.5f;
    float realDesiredLean1 = m_DesiredLean1;
    float realDesiredLean2 = m_DesiredLean2;
    // reversal code
    if (m_info.reverseFirstLeanMotor)
      realDesiredLean1 = -realDesiredLean1;
    if (m_info.reverseSecondLeanMotor)
      realDesiredLean2 = -realDesiredLean2;
    float l1 = (realDesiredLean1 - midLean1)/extentLean1;
    float l2 = (realDesiredLean2 - midLean2)/extentLean2;
#endif
    float magSqr = l1*l1+l2*l2;
    return sqrtf(magSqr);
  }
  void NmRs3DofEffector::preStep(float dtClamped, float minMuscleDamping)
  {
#if ART_ENABLE_BSPY
    // todo repace with comparison to correctly generated map
    // when roadmap phase2 is complete.
    if(m_character->isPartInMask(bvmask_UpperArmRight, getJointIndex()))
    {
      int driveState = (int)getJoint()->GetDriveState();
      bspyScratchpad(m_character->getBSpyID(), "leftShoulder", driveState);
    }
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

    float realDesiredLean1, realDesiredLean2, realDesiredTwist;

    state.m_jointMatrixCacheValid = false;
    state.m_jointQuatFromITMValid = false;

    realDesiredLean1 = m_DesiredLean1;
    realDesiredLean2 = m_DesiredLean2;
    realDesiredTwist = m_DesiredTwist;


    m_MuscleDamping = rage::Max(m_MuscleDamping, minMuscleDamping); // can't go less than the natural damping of the joints

#if NM_RUNTIME_LIMITS_IK
    // work out mid and lean values, we can't continue to store these lean1Min etc in the joint, should go somewhere else.
    float midLean1 = m_MidLean1;
    float midLean2 = m_MidLean2;
    float midTwist = m_MidTwist;
    // reversal code
    if (m_info.reverseFirstLeanMotor)
    { 
      realDesiredLean1 = -realDesiredLean1;
      midLean1 = -midLean1;
    }
    if (m_info.reverseSecondLeanMotor)
    { 
      realDesiredLean2 = -realDesiredLean2;
      midLean2 = -midLean2;
    }
    if (m_info.reverseTwistMotor)
    { 
      realDesiredTwist = -realDesiredTwist;
      midTwist = -midTwist;
    }

    const float extendAmount = 1.f + 20.f/(m_MuscleStrength + 1e-10f);
    float l1 = (realDesiredLean1 - midLean1)/m_Lean1Extent;
    float l2 = (realDesiredLean2 - midLean2)/m_Lean2Extent;
    float magSqr = l1*l1+l2*l2;
    if (magSqr > extendAmount)
    {
      float scale = extendAmount/rage::SqrtfSafe(magSqr);
      realDesiredLean1 = midLean1 + l1*scale*m_Lean1Extent;
      realDesiredLean2 = midLean2 + l2*scale*m_Lean2Extent;
    }
    realDesiredTwist = rage::Clamp(realDesiredTwist, midTwist-m_TwistExtent*extendAmount, midTwist+m_TwistExtent*extendAmount);
#else

    // reversal code
    if (m_info.reverseFirstLeanMotor)
      realDesiredLean1 = -realDesiredLean1;
    if (m_info.reverseSecondLeanMotor)
      realDesiredLean2 = -realDesiredLean2;
    if (m_info.reverseTwistMotor)
      realDesiredTwist = -realDesiredTwist;
    // work out mid and lean values, we can't continue to store these lean1Min etc in the joint, should go somewhere else.
    float midLean1 = (m_info.minFirstLeanAngle + m_info.maxFirstLeanAngle)*0.5f;
    float midLean2 = (m_info.minSecondLeanAngle + m_info.maxSecondLeanAngle)*0.5f;
    float midTwist = (m_info.minTwistAngle + m_info.maxTwistAngle)*0.5f;

    // get extent from joint, as joint limits may be different from effector limits.
    // due to NM_RUNTIME_LIMITS code
    // extents are only used to determine extent clamping here.
    rage::phJoint3Dof* threeDof = (rage::phJoint3Dof*)getJoint();
    Assert(threeDof);
    float extentLean1, extentLean2 ,extentTwist;
    threeDof->GetThreeAngleLimits(extentLean1, extentLean2 ,extentTwist);

    const float extendAmount = 1.f + 20.f/(m_MuscleStrength + 1e-10f);
    float l1 = (realDesiredLean1 - midLean1)/extentLean1;
    float l2 = (realDesiredLean2 - midLean2)/extentLean2;
    float magSqr = l1*l1+l2*l2;
    if (magSqr > extendAmount)
    {
      float scale = extendAmount/rage::SqrtfSafe(magSqr);
      realDesiredLean1 = midLean1 + l1*scale*extentLean1;
      realDesiredLean2 = midLean2 + l2*scale*extentLean2;
    }
    realDesiredTwist = rage::Clamp(realDesiredTwist, midTwist-extentTwist*extendAmount, midTwist+extentTwist*extendAmount);
#endif

#if defined (NM_RS_VALIDATE_VITAL_VALUES) && ART_ENABLE_BSPY
    // desired Angles are extreme, or QNAN (there's a divide by 0) ?
	if (rage::Abs(realDesiredLean1) > 12.0f || realDesiredLean1 != realDesiredLean1)
      Displayf("NM 3Dof[%d], setBy %s: d-L1 = %.5f", getJointIndex(), s_bvIDNames[m_DesiredLean1SetBy + 1], realDesiredLean1);
	if (rage::Abs(realDesiredLean2) > 12.0f || realDesiredLean2 != realDesiredLean2)
      Displayf("NM 3Dof[%d], setBy %s: d-L2 = %.5f", getJointIndex(), s_bvIDNames[m_DesiredLean2SetBy + 1], realDesiredLean2);
	if (rage::Abs(realDesiredTwist) > 12.0f || realDesiredTwist != realDesiredTwist)
      Displayf("NM 3Dof[%d], setBy %s: d-Twist = %.5f", getJointIndex(), s_bvIDNames[m_DesiredTwistSetBy + 1], realDesiredTwist);
#endif // NM_RS_VALIDATE_VITAL_VALUES 

    //  Apply the injury to the joint. NOTE: m_Damping is calculated with unInjured strength!!
    float muscleStrengthAfterInjury = m_MuscleStrengthScaling*m_MuscleStrength;
    float muscleDampingAfterInjury = m_MuscleDampingScaling*m_MuscleDamping;
    float muscleStiffnessAfterInjury = m_MuscleStiffnessScaling*m_MuscleStiffness;
    if (m_injuryAmount != 0.f)
      applyInjuryToEffector(muscleStrengthAfterInjury,muscleDampingAfterInjury);

    muscleDampingAfterInjury = rage::Max(muscleDampingAfterInjury, minMuscleDamping); // can't go less than the natural damping of the joints
    // this calculation increases stiffness automatically with muscle strength and damping, allowing larger strength values without instability
    float f2 = (muscleStrengthAfterInjury + 8.f*muscleDampingAfterInjury)*0.5f*muscleStiffnessAfterInjury*dtClamped;
    float internalStiffness = rage::Max(0.1f*dtClamped*60.f, f2/(f2+1));
    internalStiffness = rage::Min(internalStiffness,0.9999f);//Otherwise internalStiffness will be > 1 if dtClamped/timeStep > 1/6. Should we clamp dtClamped [1/60,1/30] so this code would never be needed
#ifdef NM_RS_VALIDATE_VITAL_VALUES
    Assert((internalStiffness == internalStiffness));
#endif
    m_3DofJoint->SetStiffness(internalStiffness);
    m_3DofJoint->SetMuscleAngleStrength(rage::Vector3(muscleStrengthAfterInjury, muscleStrengthAfterInjury, muscleStrengthAfterInjury));
    m_3DofJoint->SetMuscleSpeedStrength(rage::Vector3(muscleDampingAfterInjury, muscleDampingAfterInjury, muscleDampingAfterInjury));
    m_3DofJoint->SetLean1TargetAngle(realDesiredLean1 - midLean1);
    m_3DofJoint->SetLean2TargetAngle(realDesiredLean2 - midLean2);
    m_3DofJoint->SetTwistTargetAngle(realDesiredTwist - midTwist);
    m_3DofJoint->SetLean1TargetSpeed(0.f);
    m_3DofJoint->SetLean2TargetSpeed(0.f);
    m_3DofJoint->SetTwistTargetSpeed(0.f);

    // this code means force is capped with distance from target (pos or vel) but not capped with respect to internal stiffness
    float leanTorqueMax = m_LeanForceCap / (1.f - internalStiffness);
    float twistTorqueMax = m_TwistForceCap / (1.f - internalStiffness);
    m_3DofJoint->SetMinAndMaxMuscleTorque(rage::Vector3(leanTorqueMax, leanTorqueMax, twistTorqueMax));
  }

  void NmRs3DofEffector::postStep()
  {
#if NM_RUNTIME_LIMITS
    restoreLimits(NM_RUNTIME_LIMITS_RECOVERY_TIME);
#endif
    state.m_limitsSetThisFrame = false;
    state.m_actualAnglesValid = false;
  }

#if 0
  // none of these should be here any more...
  void NmRs3DofEffector::setInjured(float /*injuryAmount*/ NM_RS_SETBY_PARAMS_UNUSED)
  {
    Assert(false);
  }

  void NmRs3DofEffector::applyInjuryToEffector(float &/*effeciveMStrength*/, float &/*effeciveMDamping*/)
  {
    Assert(false);
  }
#else
  void NmRs3DofEffector::setInjured(float injuryAmount NM_RS_SETBY_PARAMS_UNUSED)
  {
    //Assert(false);
    m_injuryAmount = rage::Clamp(injuryAmount,0.f,1.f);
  }

  void NmRs3DofEffector::applyInjuryToEffector(float &effeciveMStrength,float &effeciveMDamping)
  {
    //Assert(false);
    effeciveMStrength = rage::Max(0.1f, effeciveMStrength*(1.f - getInjuryAmount())*(1.f - getInjuryAmount()));
    effeciveMDamping = effeciveMDamping*(1.f-getInjuryAmount());
  }
#endif

  void NmRs3DofEffector::updateCurrentAngles() const
  {
    // TDL this block reverses the transformations on desired angle, so setting desiredLean1 = actualLean1 will set to the current pose
    {
		rage::Vec3V leanAndTwistAngles;

      // getActualX functions, which call updateCurrentAngles are effectively const
      // functions from a usage perspective.
	  float *actualLean1 = const_cast<float*>(&m_ActualLean1);
	  float *actualLean1Vel = const_cast<float*>(&m_ActualLean1Vel);
	  float *actualLean2 = const_cast<float*>(&m_ActualLean2);
	  float *actualLean2Vel = const_cast<float*>(&m_ActualLean2Vel);
	  float *actualTwist = const_cast<float*>(&m_ActualTwist);
	  float *actualTwistVel = const_cast<float*>(&m_ActualTwistVel);
	  EffectorBitField *_state = const_cast<EffectorBitField*>(&state);

#if CRAWL_LEARNING
		// Split the lean into two parts and get the lean1, lean2 and twist angles.
		rage::Vec3V leanAndTwistRates;
		m_3DofJoint->ComputeCurrentLeanAnglesAndRates(m_character->getArticulatedBody(), leanAndTwistAngles, leanAndTwistRates);
		*actualLean1Vel = leanAndTwistRates.GetXf();
		*actualLean2Vel = leanAndTwistRates.GetYf();
		*actualTwistVel = leanAndTwistRates.GetZf();
#else
      // Split the lean into two parts and get the lean1, lean2 and twist angles.
      leanAndTwistAngles = m_3DofJoint->ComputeCurrentLeanAngles(m_character->getArticulatedBody());
#endif
	  *actualLean1 = leanAndTwistAngles.GetXf();
	  *actualLean2 = leanAndTwistAngles.GetYf();
	  *actualTwist = leanAndTwistAngles.GetZf();

#if NM_RUNTIME_LIMITS_IK
      if (m_info.reverseFirstLeanMotor)
      {
        *actualLean1 -= m_MidLean1;
        *actualLean1 = -*actualLean1, *actualLean1Vel = -*actualLean1Vel;
      }
      else
      {
         *actualLean1 += m_MidLean1;
      }
      if (m_info.reverseSecondLeanMotor)
      {
        *actualLean2 -= m_MidLean2;
        *actualLean2 = -*actualLean2, *actualLean2Vel = -*actualLean2Vel;
      }
      else
      {
        *actualLean2 += m_MidLean2;
      }
      if (m_info.reverseTwistMotor)
      {
        *actualTwist -= m_MidTwist;
        *actualTwist = -*actualTwist, *actualTwistVel = -*actualTwistVel;
      }
      else
      {
        *actualTwist += m_MidTwist;
      }
#else
      *actualLean1 += (m_info.minFirstLeanAngle + m_info.maxFirstLeanAngle)*0.5f;
      *actualLean2 += (m_info.minSecondLeanAngle + m_info.maxSecondLeanAngle)*0.5f;
      *actualTwist += (m_info.minTwistAngle + m_info.maxTwistAngle)*0.5f;

      if (m_info.reverseFirstLeanMotor)
        *actualLean1 = -*actualLean1, *actualLean1Vel = -*actualLean1Vel;
      if (m_info.reverseSecondLeanMotor)
        *actualLean2 = -*actualLean2, *actualLean2Vel = -*actualLean2Vel;
      if (m_info.reverseTwistMotor)
        *actualTwist = -*actualTwist, *actualTwistVel = -*actualTwistVel;
#endif

	  _state->m_actualAnglesValid = true;
      }
  }

  void NmRs3DofEffector::activePose(int transformSource)
  {
    rage::Quaternion quat;
    NMutils::NMVector4 nmQ;
    if (!getJointQuaternionFromIncomingTransform(nmQ, (IncomingTransformSource)transformSource))
      return;
    quat.Set(nmQ[1], nmQ[2], nmQ[3], nmQ[0]);

    rage::Vector3 tts = rsQuatToRageDriveTwistSwing(quat);
    getTwistAndSwingFromRawTwistAndSwing(tts, tts);

    nmrsSetAngles(this, tts.y, tts.z, tts.x);
  }
  // TDL return back the angles / angle vels from the incoming transforms without setting them
  void NmRs3DofEffector::activeAnimInfo(float timeStep, float *lean1, float *lean2, float *twist, float *lean1Vel, float *lean2Vel, float *twistVel) const
  {
    rage::Quaternion quat;
    rage::Vector3 vel, rotVel;
    if (!getJointQuatPlusVelFromIncomingTransform(quat, rotVel))
      return;
    rage::Vector3 tts = rsQuatToRageDriveTwistSwing(quat);
    getTwistAndSwingFromRawTwistAndSwing(tts, tts);

    *lean1Vel = rotVel.y/timeStep;
    *lean2Vel =-rotVel.x/timeStep;
    *twistVel = rotVel.z/timeStep;
    if (m_info.reverseFirstLeanMotor)
      *lean1Vel = -*lean1Vel;
    if (m_info.reverseSecondLeanMotor)
      *lean2Vel = -*lean2Vel;
    if (m_info.reverseTwistMotor)
      *twistVel = -*twistVel;

    *lean1 = tts.y; 
    *lean2 = tts.z;
    *twist = tts.x;
  }

  void NmRs3DofEffector::holdPose()
  {
    nmrsSetAngles(this, nmrsGetActualLean1(this), nmrsGetActualLean2(this), nmrsGetActualTwist(this));
  }

  bool NmRs3DofEffector::storeZeroPose()
  {
    state.m_zeroPoseStored = false;

    NMutils::NMVector4 nmQ;
    if (!getJointQuaternionFromIncomingTransform_uncached(nmQ))
      return false;

    rage::Quaternion q;
    q.Set(nmQ[1], nmQ[2], nmQ[3], nmQ[0]);

    m_zeroPoseAngles = rsQuatToRageDriveTwistSwing(q);

    getTwistAndSwingFromRawTwistAndSwing(m_zeroPoseAngles, m_zeroPoseAngles);

    state.m_zeroPoseStored = true;

    return true;
  }

    void NmRs3DofEffector::blendToZeroPose(float t NM_RS_SETBY_PARAMS_UNUSED)
  {
    if (state.m_zeroPoseStored)
    {
      // do linear blend with current desired angles
      float clampT = rage::Clamp(t, 0.0f, 1.0f);
      float invT = 1.0f - clampT;

      float twistResult = (getDesiredTwist() * invT) + (m_zeroPoseAngles.x * clampT);
      float lean1Result = (getDesiredLean1() * invT) + (m_zeroPoseAngles.y * clampT);
      float lean2Result = (getDesiredLean2() * invT) + (m_zeroPoseAngles.z * clampT);

      nmrsSetAngles(this, lean1Result, lean2Result, twistResult);
    }
  }

  void NmRs3DofEffector::ApplyTorque(const rage::Vector3 &torque)
  {
	  rage::Vec3V vTorque = VECTOR3_TO_VEC3V(torque);
	  if (NmRsCharacter::sm_ApplyForcesImmediately)
		  get3DofJoint()->ApplyTorque(m_character->getArticulatedBody(), vTorque.GetIntrin128ConstRef(), rage::ScalarV(m_character->getLastKnownUpdateStep()).GetIntrin128ConstRef());
	  else
			m_character->AddDeferredForce(NmRsCharacter::kJointTorque3Dof, (rage::u8) m_jointIndex, torque, torque);
  }

	void NmRs3DofEffector::ApplyAngImpulse(const rage::Vector3 &angImpulse)
	{
		rage::Vec3V vImpulse = VECTOR3_TO_VEC3V(angImpulse);
		if (NmRsCharacter::sm_ApplyForcesImmediately)
			get3DofJoint()->ApplyAngImpulse(m_character->getArticulatedBody(), vImpulse.GetIntrin128ConstRef());
		else
			m_character->AddDeferredForce(NmRsCharacter::kJointAngImpulse3Dof, (rage::u8) m_jointIndex, angImpulse, angImpulse);
	}

  void NmRs3DofEffector::blendToPose(float twist, float lean1, float lean2, float t)
  {
    // do linear blend with current desired angles
    float clampT = rage::Clamp(t, 0.0f, 1.0f);
    float invT = 1.0f - clampT;

    float twistResult = (getDesiredTwist() * invT) + (twist * clampT);
    float lean1Result = (getDesiredLean1() * invT) + (lean1 * clampT);
    float lean2Result = (getDesiredLean2() * invT) + (lean2 * clampT);

    nmrsSetAngles(this, lean1Result, lean2Result, twistResult);
  }

    void NmRs3DofEffector::setRelaxation(float mult NM_RS_SETBY_PARAMS_UNUSED)
  {
    Assert(mult >= 0.0f && mult <= 1.0f);
    float clampedStrength = rage::Clamp(m_info.m_defaultMuscleStrength * mult*mult, 2.0f, 20.0f);
    setMuscleStrength(clampedStrength); // we square the multiplier as the natural frequency (strength*strength) is what we want to multiply by mult.
    setMuscleDamping(m_info.m_defaultMuscleDamping * mult);
  }

    void NmRs3DofEffector::setRelaxation_DampingOnly(float mult NM_RS_SETBY_PARAMS_UNUSED)
  {
    Assert(mult >= 0.0f && mult <= 1.0f);
    setMuscleDamping(m_info.m_defaultMuscleDamping * mult);
  }

  void NmRs3DofEffector::resetEffectorCalibrations()
  {
    setLeanForceCap(m_info.m_defaultLeanForceCap);
    setTwistForceCap(m_info.m_defaultTwistForceCap);
    setMuscleStiffness(m_info.m_defaultMuscleStiffness);
    setMuscleStrength(m_info.m_defaultMuscleStrength);
    setMuscleDamping(m_info.m_defaultMuscleDamping);
    setOpposeGravity(0.f);
  }    

  void NmRs3DofEffector::resetEffectorMuscleStiffness()
  {
    setMuscleStiffness(m_info.m_defaultMuscleStiffness);
  }


  void NmRs3DofEffector::resetAngles()
  {
    nmrsSetAngles(this, 0,0,0);
  }

  void NmRs3DofEffector::setStiffness(float stiffness, float dampingScale, float *muscleStiffness)
  {
    setMuscleStrength(stiffness*stiffness);
    setMuscleDamping(2.f*dampingScale*stiffness);
    if (muscleStiffness)
      setMuscleStiffness(*muscleStiffness);
  }

#if NM_RUNTIME_LIMITS
  void NmRs3DofEffector::cacheCurrentLimits()
  {
    rage::phJoint3Dof* threeDof = (rage::phJoint3Dof*)getJoint();
    Assert(threeDof);
    float maxLean1Angle, maxLean2Angle, maxTwistAngle;
    threeDof->GetThreeAngleLimits(maxLean1Angle, maxLean2Angle, maxTwistAngle);
    // compensate for SetAngleLimits(), which subtracts rage::phSimulator::GetAllowedAnglePenetration().
    m_lean1LimitCache = maxLean1Angle + rage::phSimulator::GetAllowedAnglePenetration();
    m_lean2LimitCache = maxLean2Angle + rage::phSimulator::GetAllowedAnglePenetration();
    m_twistLimitCache = maxTwistAngle + rage::phSimulator::GetAllowedAnglePenetration();
  }

  // step: max change in rad/s, set to 0 to disable step clamping.
  void NmRs3DofEffector::restoreLimits(float step)
  {
    if(state.m_limitsSet && !state.m_limitsSetThisFrame)
    {
      if(step == 0.f)
      {
        setLimits(m_lean1LimitCache,m_lean2LimitCache,m_twistLimitCache);
      }
      else
      {
        float adjustedStep = step * m_character->getLastKnownUpdateStep();
        float lean1Limit, lean2Limit, twistLimit;
        rage::phJoint3Dof* threeDof = (rage::phJoint3Dof*)getJoint();
        Assert(threeDof);
        threeDof->GetThreeAngleLimits(lean1Limit, lean2Limit, twistLimit);
        // compensate for SetAngleLimits(), which subtracts rage::phSimulator::GetAllowedAnglePenetration().
        lean1Limit += rage::phSimulator::GetAllowedAnglePenetration();
        lean2Limit += rage::phSimulator::GetAllowedAnglePenetration();
        twistLimit += rage::phSimulator::GetAllowedAnglePenetration();
        lean1Limit += rage::Clamp(m_lean1LimitCache - lean1Limit, -adjustedStep, adjustedStep);
        lean2Limit += rage::Clamp(m_lean2LimitCache - lean2Limit, -adjustedStep, adjustedStep);
        twistLimit += rage::Clamp(m_twistLimitCache - twistLimit, -adjustedStep, adjustedStep);

        setLimits(lean1Limit, lean2Limit, twistLimit);

        if(lean1Limit == m_lean1LimitCache && lean2Limit == m_lean2LimitCache && twistLimit == m_twistLimitCache)
        {
          state.m_limitsSet = false;
          state.m_limitsSetThisFrame = false;
        }
      }
    }
  }

  void NmRs3DofEffector::disableLimits()
  {
    const float limit = PI-0.01f;
    setLimits(limit, limit, limit);
  }

  void NmRs3DofEffector::setTwistLimit(float twist)
  {
    float lean1Limit, lean2Limit, twistLimit;
    rage::phJoint3Dof* threeDof = (rage::phJoint3Dof*)getJoint();
    Assert(threeDof);
    threeDof->GetThreeAngleLimits(lean1Limit, lean2Limit, twistLimit);
    // compensate for SetAngleLimits(), which subtracts rage::phSimulator::GetAllowedAnglePenetration().
    lean1Limit += rage::phSimulator::GetAllowedAnglePenetration();
    lean2Limit += rage::phSimulator::GetAllowedAnglePenetration();
    //twistLimit += rage::phSimulator::GetAllowedAnglePenetration();
    setLimits(lean1Limit, lean2Limit, twist);
  }

  void NmRs3DofEffector::setLean1Limit(float lean1)
  {
    float lean1Limit, lean2Limit, twistLimit;
    rage::phJoint3Dof* threeDof = (rage::phJoint3Dof*)getJoint();
    Assert(threeDof);
    threeDof->GetThreeAngleLimits(lean1Limit, lean2Limit, twistLimit);
    // compensate for SetAngleLimits(), which subtracts rage::phSimulator::GetAllowedAnglePenetration().
    //lean1Limit += rage::phSimulator::GetAllowedAnglePenetration();
    lean2Limit += rage::phSimulator::GetAllowedAnglePenetration();
    twistLimit += rage::phSimulator::GetAllowedAnglePenetration();
    setLimits(lean1, lean2Limit, twistLimit);
  }

  void NmRs3DofEffector::setLean2Limit(float lean2)
  {
    float lean1Limit, lean2Limit, twistLimit;
    rage::phJoint3Dof* threeDof = (rage::phJoint3Dof*)getJoint();
    Assert(threeDof);
    threeDof->GetThreeAngleLimits(lean1Limit, lean2Limit, twistLimit);
    // compensate for SetAngleLimits(), which subtracts rage::phSimulator::GetAllowedAnglePenetration().
    lean1Limit += rage::phSimulator::GetAllowedAnglePenetration();
    //lean2Limit += rage::phSimulator::GetAllowedAnglePenetration();
    twistLimit += rage::phSimulator::GetAllowedAnglePenetration();
    setLimits(lean1Limit, lean2, twistLimit);
  }

  void NmRs3DofEffector::setLimits(float lean1, float lean2, float twist)
  {
    state.m_limitsSet = true;
    state.m_limitsSetThisFrame = true;

    rage::phJoint3Dof* threeDof = (rage::phJoint3Dof*)getJoint();
    Assert(threeDof);
    Assert(m_info.softLimitFirstLeanMultiplier == m_info.softLimitSecondLeanMultiplier);
    Assert(m_info.softLimitSecondLeanMultiplier == m_info.softLimitTwistMultiplier);
#ifdef USE_SOFT_LIMITS //Modified by R*
    threeDof->SetSoftLimitRatio(m_info.softLimitFirstLeanMultiplier);
#endif
#if NM_RUNTIME_LIMITS_IK
    //so that IK and clamping use the rage joint limits i.e the runTime limits
    m_Lean1Extent = rage::Clamp(lean1, 0.f, PI-0.001f);
    m_Lean2Extent = rage::Clamp(lean2, 0.f, PI-0.001f);
    m_TwistExtent = rage::Clamp(twist, 0.f, PI-0.001f);
    //note m_MidLean1, m_MidLean2 and m_MidTwist stay the same as RunTimeLimits add symmetricaly to the min and max
    //min and max's are increased by difference from setup extents
    m_MinLean1 = m_info.reverseFirstLeanMotor ? -m_info.maxFirstLeanAngle : m_info.minFirstLeanAngle;
    m_MaxLean1 = m_info.reverseFirstLeanMotor ? -m_info.minFirstLeanAngle : m_info.maxFirstLeanAngle;
    m_MinLean2 = m_info.reverseSecondLeanMotor ? -m_info.maxSecondLeanAngle : m_info.minSecondLeanAngle;
    m_MaxLean2 = m_info.reverseSecondLeanMotor ? -m_info.minSecondLeanAngle : m_info.maxSecondLeanAngle;
    m_MinTwist = m_info.reverseTwistMotor ? -m_info.maxTwistAngle : m_info.minTwistAngle;
    m_MaxTwist = m_info.reverseTwistMotor ? -m_info.minTwistAngle : m_info.maxTwistAngle;
    float changeFromSetUpLean1Extent = m_Lean1Extent - m_lean1LimitCache;
    float changeFromSetUpLean2Extent = m_Lean2Extent - m_lean2LimitCache;
    float changeFromSetUpTwistExtent = m_TwistExtent - m_twistLimitCache;
    m_MinLean1 -= changeFromSetUpLean1Extent;
    m_MaxLean1 += changeFromSetUpLean1Extent;
    m_MinLean2 -= changeFromSetUpLean2Extent;
    m_MaxLean2 += changeFromSetUpLean2Extent;
    m_MinTwist -= changeFromSetUpTwistExtent;
    m_MaxTwist += changeFromSetUpTwistExtent;

    // clamp and set hard limits.
    threeDof->SetThreeAngleLimits(
      m_Lean1Extent,
      m_Lean2Extent,
      m_TwistExtent,
      true);  // this is to make sure that the type data is not modified.  Only adjusts for the fragInst.
#else

    // clamp and set hard limits.
    threeDof->SetThreeAngleLimits(
      rage::Clamp(lean1, 0.f, PI-0.001f),
      rage::Clamp(lean2, 0.f, PI-0.001f),
      rage::Clamp(twist, 0.f, PI-0.001f),
      true);  // this is to make sure that the type data is not modified.  Only adjusts for the fragInst.
#endif
#if ART_ENABLE_BSPY // double-check limits setting...
    if(m_jointIndex == NM_BSPY_JOINT_DEBUG_INDEX)
    {
      bspyScratchpad(m_character->getBSpyID(), "debug.limit", lean1);
      bspyScratchpad(m_character->getBSpyID(), "debug.limit", lean2);
      bspyScratchpad(m_character->getBSpyID(), "debug.limit", twist);

      float twistCheck, lean1Check, lean2Check;
      threeDof->GetThreeAngleLimits(lean1Check, lean2Check, twistCheck);
      bspyScratchpad(m_character->getBSpyID(), "debug.limit", lean1Check);
      bspyScratchpad(m_character->getBSpyID(), "debug.limit", lean2Check);
      bspyScratchpad(m_character->getBSpyID(), "debug.limit", twistCheck);

      int limitsExceeded = threeDof->ComputeNumHardJointLimitDofs(m_character->getArticulatedBody());
      bspyScratchpad(m_character->getBSpyID(), "debug.limit", limitsExceeded);

      float limitResponse = threeDof->GetJointLimitResponse(m_character->getArticulatedBody(), 3);
      bspyScratchpad(m_character->getBSpyID(), "debug.limit", limitResponse);
    }
#endif  

#if ART_ENABLE_BSPY && 0
    if (m_character->getBSpyID() != INVALID_AGENTID)
    {  
      // todo: account for effector offsets.

      EffectorModifyLimitsPacket eflp((bs_uint16)m_character->getBSpyID(), (bs_uint8)m_jointIndex);

      eflp.m_minAngles.v[0]   = -twist;
      eflp.m_minAngles.v[1]   = -lean1;
      eflp.m_minAngles.v[2]   = -lean2;

      eflp.m_maxAngles.v[0]   = twist;
      eflp.m_maxAngles.v[1]   = lean1;
      eflp.m_maxAngles.v[2]   = lean2;

      eflp.m_cacheUpdateID = (bs_uint32)bSpyServer::inst()->getFrameTicker();

      bspySendPacket(eflp);
    }
#endif // ART_ENABLE_BSPY

#if ART_ENABLE_BSPY && 0
    //if(m_jointIndex == NM_BSPY_JOINT_DEBUG_INDEX)
    //{
    //  rage::Vector3 cached(m_twistLimitCache, m_lean1LimitCache, m_lean2LimitCache); 
    //  bspyScratchpad(m_character->getBSpyID(), "debug.limit", cached);
    //  rage::Vector3 current(twist, lean1, lean2); 
    //  bspyScratchpad(m_character->getBSpyID(), "debug.limit", current);
    //}
#endif
  }

  void NmRs3DofEffector::setLimitsToPose(bool useActual /* = false */, float margin /* = 0.f */)
  {
    rage::Vector3 ts;
    if(useActual)
      ts.Set(getActualTwist(), getActualLean1(), getActualLean2());
    else
      ts.Set(getDesiredTwist(), getDesiredLean1(), getDesiredLean2());

#if ART_ENABLE_BSPY
    if(m_jointIndex == NM_BSPY_JOINT_DEBUG_INDEX)
    {
      rage::Vector3 unRaw(ts);
      bspyScratchpad(m_character->getBSpyID(), "debug.limit2pose", unRaw);
    }
#endif

    getRawTwistAndSwingFromTwistAndSwing(ts, ts);

#if ART_ENABLE_BSPY
    if(m_jointIndex == NM_BSPY_JOINT_DEBUG_INDEX)
    {
      rage::Vector3 raw(ts);
      bspyScratchpad(m_character->getBSpyID(), "debug.limit2pose", raw);
    }
#endif

    // todo does not take rage joint drive space oddity (strange twist
    // dependence) into account.  may not open lean limits wide enough
    // when twist is extreme.

    // abs and add margin.
    ts.x = rage::Abs(ts.x) + margin;
    ts.y = rage::Abs(ts.y) + margin;
    ts.z = rage::Abs(ts.z) + margin;

    // get joint limit.
    float lean1Limit = m_lean1LimitCache;
    float lean2Limit = m_lean2LimitCache;
    float twistLimit = m_twistLimitCache;

#if ART_ENABLE_BSPY && 0
    //  if(m_jointIndex == NM_BSPY_JOINT_DEBUG_INDEX)
    //{
    //    bspyScratchpad(m_character->getBSpyID(), "debug.limit2pose", ts);
    //    rage::Vector3 actual;
    //    actual.Set(getActualTwist(), getActualLean1(), getActualLean2());
    //    bspyScratchpad(m_character->getBSpyID(), "debug.limit2pose", actual);
    //}
#endif

    // adjust twist independently.
    if(ts.x > twistLimit)
      twistLimit = ts.x;
    else if(-ts.x > twistLimit)
      twistLimit = -ts.x;

#if ART_ENABLE_BSPY && 0
    //if(m_jointIndex == NM_BSPY_JOINT_DEBUG_INDEX)
    //  bspyScratchpad(m_character->getBSpyID(), "debug.limit2pose", ts);
#endif

    // squash lean limits into a unit circle.
    ts.y /= lean1Limit;
    ts.z /= lean2Limit;
    float radius = rage::Sqrtf(ts.y*ts.y+ts.z*ts.z);

#if ART_ENABLE_BSPY && 0
    //if(m_jointIndex == NM_BSPY_JOINT_DEBUG_INDEX)
    //  bspyScratchpad(m_character->getBSpyID(), "debug.limit2pose", radius);
#endif

    // if desired lean is out of limits...
    if(radius > 1.f)
    {
      float k = 2.f * rage::Asinf(ts.y / radius) / PI;

#if ART_ENABLE_BSPY && 0
      //if(m_jointIndex == NM_BSPY_JOINT_DEBUG_INDEX)
      //  bspyScratchpad(m_character->getBSpyID(), "debug.limit2pose", k);
#endif

      if(k > 0.5)
        lean1Limit *= radius;
      else
        lean1Limit += lean1Limit * (radius - 1.f) * k * 2;

      if(k < 0.5)
        lean2Limit *= radius;
      else
        lean2Limit += lean2Limit * (radius - 1.f) * (1.f - k) * 2;
    }

    setLimits(lean1Limit, lean2Limit, twistLimit);
  }
#endif // NM_RUNTIME_LIMITS

  /*
  * The following function gets the actual twist, swing1, and swing2 (used in 
  * Rockstar taking into account: reversing motors, drive mode relative, offsets, 
  * and limits) and converts it into the 'raw' twist, swing1, and swing2.
  */
  void NmRs3DofEffector::getTwistAndSwingFromRawTwistAndSwing(rage::Vector3 &ts, const rage::Vector3 &tsRaw) const
  {
    // Initialise.
    ts.x = tsRaw.x;
    ts.y = tsRaw.y;
    ts.z = tsRaw.z;

    // Reversal code.
    if (m_info.reverseTwistMotor)
    {
      ts.x = -ts.x;
    }
    if (m_info.reverseFirstLeanMotor)
    {
      ts.y = -ts.y;
    }
    if (m_info.reverseSecondLeanMotor)
    {
      ts.z = -ts.z;
    }

#if NM_RUNTIME_LIMITS_IK
    // Calculate twist, swing1, and swing2 relative to mid values.
    ts.x += m_MidTwist;
    ts.y += m_MidLean1;
    ts.z += m_MidLean2;
#else
    // Calculate mid and extent values for twist, swing1, and swing2 for the given 3dof.
    float midTwist = 0.5f*(m_info.minTwistAngle + m_info.maxTwistAngle);
    float midSwing1 = 0.5f*(m_info.minFirstLeanAngle + m_info.maxFirstLeanAngle);
    float midSwing2 = 0.5f*(m_info.minSecondLeanAngle + m_info.maxSecondLeanAngle);

    // Calculate twist, swing1, and swing2 relative to mid values.
    if (m_info.reverseTwistMotor)
    {
      ts.x -= midTwist;
    }
    else
    {
      ts.x += midTwist;
    }

    if (m_info.reverseFirstLeanMotor)
    {
      ts.y -= midSwing1;
    }
    else
    {
      ts.y += midSwing1;
    }
    if (m_info.reverseSecondLeanMotor)
    {
      ts.z -= midSwing2;
    }
    else
    {
      ts.z += midSwing2;
    }
#endif
  }

  /*
  * The following function gets the 'raw' twist, swing1, and swing2 (free and unconstrained)
  * and converts it into the actual twist, swing1, and swing2 (used in Rockstar taking
  * into account: reversing motors, drive mode relative, offsets, and limits).
  */
  void NmRs3DofEffector::getRawTwistAndSwingFromTwistAndSwing(rage::Vector3 &tsRaw,const rage::Vector3 &ts) const
  {
    tsRaw.x = ts.x;
    tsRaw.y = ts.y;
    tsRaw.z = ts.z;

#if NM_RUNTIME_LIMITS_IK
    // Calculate twist, swing1, and swing2 relative to zero values.
    tsRaw.x -= m_MidTwist;
    tsRaw.y -= m_MidLean1;
    tsRaw.z -= m_MidLean2;
#else
    // Calculate mid and extent values for twist, swing1, and swing2 for the given 3dof.
    float midTwist = 0.5f*(m_info.minTwistAngle + m_info.maxTwistAngle);
    float midSwing1 = 0.5f*(m_info.minFirstLeanAngle + m_info.maxFirstLeanAngle);
    float midSwing2 = 0.5f*(m_info.minSecondLeanAngle + m_info.maxSecondLeanAngle);

    // Calculate twist, swing1, and swing2 relative to zero values.
    if (m_info.reverseTwistMotor)
    {
      tsRaw.x += midTwist;
    }
    else
    {
      tsRaw.x -= midTwist;
    }

    if (m_info.reverseFirstLeanMotor)
    {
      tsRaw.y += midSwing1;
    }
    else
    {
      tsRaw.y -= midSwing1;
    }
    if (m_info.reverseSecondLeanMotor)
    {
      tsRaw.z += midSwing2;
    }
    else
    {
      tsRaw.z -= midSwing2;
    }
#endif
    // Reversal code.
    if (m_info.reverseTwistMotor)
    {
      tsRaw.x = -tsRaw.x;
    }
    if (m_info.reverseFirstLeanMotor)
    {
      tsRaw.y = -tsRaw.y;
    }
    if (m_info.reverseSecondLeanMotor)
    {
      tsRaw.z = -tsRaw.z;
    }
  }

  void NmRs3DofEffector::clamp(float amount /* = 1 */)
  {
    rage::Vector3 leanTwist(getDesiredTwist(), getDesiredLean1(), getDesiredLean2());
    clamp(leanTwist, amount);
    setDesiredTwist(leanTwist.x);
    setDesiredLean1(leanTwist.y);
    setDesiredLean2(leanTwist.z);
  }

  void NmRs3DofEffector::clamp(rage::Vector3& leanTwist, float amount /* = 1 */) const
  {
    getRawTwistAndSwingFromTwistAndSwing(leanTwist, leanTwist);
    clampRawLeanTwist(leanTwist, amount);
    getTwistAndSwingFromRawTwistAndSwing(leanTwist, leanTwist);
  }

  void NmRs3DofEffector::setDesiredLean1Relative(float angle)
  {
    setDesiredLean1( getMidLean1() + (getLean1Extent() * angle) );
  }

  void NmRs3DofEffector::setDesiredLean2Relative(float angle)
  {
    setDesiredLean2( getMidLean2() + (getLean2Extent() * angle) );
  }

  void NmRs3DofEffector::setDesiredTwistRelative(float angle)
  {
    setDesiredTwist( getMidTwist() + (getTwistExtent() * angle) );
  }

  float NmRs3DofEffector::getDesiredLean1FromRelative(float angle) const
  {
    return (getMidLean1() + (getLean1Extent() * angle));
  }

  float NmRs3DofEffector::getDesiredLean2FromRelative(float angle) const
  {
    return (getMidLean2() + (getLean2Extent() * angle));
  }

  float NmRs3DofEffector::getDesiredTwistFromRelative(float angle) const
  {
    return (getMidTwist() + (getTwistExtent() * angle));
  }

  void NmRs3DofEffector::setDesiredLean1ZeroRelative(float angle)
  {
    setDesiredLean1( m_zeroPoseAngles.x + angle );
  }

  void NmRs3DofEffector::setDesiredLean2ZeroRelative(float angle)
  {
    setDesiredLean2( m_zeroPoseAngles.y + angle );
  }

  void NmRs3DofEffector::setDesiredTwistZeroRelative(float angle)
  {
    setDesiredTwist( m_zeroPoseAngles.z + angle );
  }

  void NmRs3DofEffector::getQuaternionFromDesiredAngles(rage::Quaternion &q) const
  {
    rage::Vector3 tts(getDesiredTwist(), getDesiredLean1(), getDesiredLean2());
    q = rsRageDriveTwistSwingToQuat(tts);
  }

  void NmRs3DofEffector::getQuaternionFromDesiredRawAngles(rage::Quaternion &q) const
  {
    rage::Vector3 tts(getDesiredTwist(), getDesiredLean1(), getDesiredLean2());
    getRawTwistAndSwingFromTwistAndSwing(tts, tts);
    q = rsRageDriveTwistSwingToQuat(tts);
  }

  float NmRs3DofEffector::clampRawLeanTwist(rage::Quaternion& q, float amount /* = 1 */) const
  {
    // this will correctly clamp a quat in the rage limit
    // space. Drive and limit spaces are *not* the same.
    rage::Vector3 leanTwist = rsQuatToRageLimitTwistSwing(q);
    float result = clampRawLeanTwist(leanTwist, amount);
    q = rsRageLimitTwistSwingToQuat(leanTwist);
    return result;
  }

  // clamp RAW lean and twist values to limits.
  // not strictly accurate because rage 3dof *drives* 
  // with dependent twist and *limits* with free twist.
  float NmRs3DofEffector::clampRawLeanTwist(rage::Vector3& leanTwist, float amount /* = 1 */) const
  {
    float t  = leanTwist.x;
    float l1 = leanTwist.y;
    float l2 = leanTwist.z;

#if NM_RUNTIME_LIMITS_IK
    // clamp twist separately.
    float twistMax = amount * m_TwistExtent;
    t = rage::Clamp(t, -twistMax, twistMax);

    float l1Max = amount * m_Lean1Extent;
    float l2Max = amount * m_Lean2Extent;
#else
    // clamp twist separately.
    float twistMax = amount * (m_info.maxTwistAngle - m_info.minTwistAngle) / 2.f;
    t = rage::Clamp(t, -twistMax, twistMax);

    float l1Max = amount * (m_info.maxFirstLeanAngle - m_info.minFirstLeanAngle) / 2.f;
    float l2Max = amount * (m_info.maxSecondLeanAngle - m_info.minSecondLeanAngle) / 2.f;
#endif
    if(l1Max < 1e-3f || l2Max < 1e-3f)
    {
      l1 = l1 < -l1Max ? -l1Max : (l1 > l1Max ? l1Max : l1);
      l2 = l2 < -l2Max ? -l2Max : (l2 > l2Max ? l2Max : l2);
    }

    float l1unit = l1/l1Max;
    float l2unit = l2/l2Max;
    float violationFactor = l1unit*l1unit + l2unit*l2unit - 1.f;
    if (violationFactor > 0)
    {
      ART::closestPointOnEllipse(l1, l2, l1Max, l2Max);
    }

    leanTwist.Set(t, l1, l2);
    return violationFactor > 0 ? violationFactor : 0;
  }

#if ART_ENABLE_BSPY && NM_BSPY_JOINT_DEBUG
  void NmRs3DofEffector::addArrow(float l1, float l2, const rage::Matrix34 &mat, float scale, const rage::Vector3 &col)
  {
    float angle = rage::Sqrtf(rage::square(l1) + rage::square(l2));
    l1 /= (angle + NM_RS_FLOATEPS);
    l2 /= (angle + NM_RS_FLOATEPS);
    rage::Vector3 b(l1 * rage::Sinf(angle) * scale, l2 * rage::Sinf(angle) * scale, rage::Cosf(angle) * scale);
    mat.Transform(b);
    m_character->bspyDrawLine(mat.d, b, col);
  }

  void NmRs3DofEffector::renderDebugDraw()
  {
    BehaviourMask mask = NM_BSPY_JOINT_DEBUG_MASK;
    if(!m_character->isEffectorInMask(mask, m_jointIndex))
      return;

    const int tacoSides = 8;

    rage::Matrix34 mat;
    rage::Vector3 a, b, col(0, 1, 0), col2(0.0f, 0.3f, 1.0f), col3(1.0f, 0.3f, 0.0f);
    const float scale = 0.2f;

    // query joint for actual extents
    rage::phJoint3Dof* threeDof = (rage::phJoint3Dof*)getJoint();
    Assert(threeDof);
    float ext1, ext2, twist;
    threeDof->GetThreeAngleLimits(ext1, ext2, twist);

    getMatrix1(mat);
#if ART_ENABLE_BSPY && NM_BSPY_JOINT_DEBUG
    m_character->bspyDrawCoordinateFrame(0.2f, mat);
#endif
    a = mat.d;

    //  draw extents of taco
    b.Set(rage::Sinf(ext1) * scale, 0, rage::Cosf(ext1) * scale );
    mat.Transform(b);
    m_character->bspyDrawLine(a, b, col);

    b.Set(rage::Sinf(-ext1) * scale, 0, rage::Cosf(-ext1) * scale );
    mat.Transform(b);

    m_character->bspyDrawLine(a, b, col);

    b.Set(0, rage::Sinf(-ext2) * scale, rage::Cosf(-ext2) * scale );
    mat.Transform(b);
    m_character->bspyDrawLine(a, b, col);

    b.Set(0, rage::Sinf(ext2) * scale, rage::Cosf(ext2) * scale );
    mat.Transform(b);
    m_character->bspyDrawLine(a, b, col);

    // then draw cap
    for (int i=0; i<=tacoSides; i++)
    {
      float t = (float)i * (PI * 2.0f) / tacoSides;
      float l1 = rage::Sinf(t) * ext1;
      float l2 = rage::Cosf(t) * ext2;

      float angle = rage::Sqrtf(rage::square(l1) + rage::square(l2));
      l1 /= (angle + NM_RS_FLOATEPS);
      l2 /= (angle + NM_RS_FLOATEPS);

      a = b;
      b.Set(l1 * rage::Sinf(angle) * scale, l2 * rage::Sinf(angle) * scale, rage::Cosf(angle) * scale);

      mat.Transform(b);
      m_character->bspyDrawLine(a, b, col);
    }
    // TDL now draw desiredAngle as a line
    float dirL1 = m_info.reverseFirstLeanMotor ? -1.f : 1.f;
    float dirL2 = m_info.reverseSecondLeanMotor ? -1.f : 1.f;
    addArrow((getDesiredLean1() - m_MidLean1)*dirL1, (getDesiredLean2() - m_MidLean2)*dirL2, mat, scale, col2);
    addArrow((getActualLean1() - m_MidLean1)*dirL1, (getActualLean2() - m_MidLean2)*dirL2, mat, scale, col3);

    const int hingeSides = 8;
    float angMin = getMinTwist(), angMax = getMaxTwist(), t, angle;
    float angRange = angMax - angMin;
    //In MP3:
    //      float angMin = -twist, angMax = twist, t, angle;
    //      float angRange = twist  * 2.f;


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
    angle = getDesiredTwist();
    b.Set(rage::Sinf(-angle) * scale, rage::Cosf(-angle) * scale, 0);
    mat.Transform(b);
    m_character->bspyDrawLine(mat.d, b, col2);
    angle = getActualTwist();
    b.Set(rage::Sinf(-angle) * scale, rage::Cosf(-angle) * scale, 0);
    mat.Transform(b);
    m_character->bspyDrawLine(mat.d, b, col3);
  }
#endif

#if ART_ENABLE_BSPY
  void NmRs3DofEffector::sendDescriptor()
  {
    Effector3DofDescriptorPacket dp((bs_uint8)m_jointIndex, (bs_uint8)m_jointTypeIndex);

    dp.d.m_nameToken        = m_nameToken;

    dp.d.m_minAngles.v[0]   = getMinTwist();
    dp.d.m_minAngles.v[1]   = getMinLean1();
    dp.d.m_minAngles.v[2]   = getMinLean2();

    dp.d.m_maxAngles.v[0]   = getMaxTwist();
    dp.d.m_maxAngles.v[1]   = getMaxLean1();
    dp.d.m_maxAngles.v[2]   = getMaxLean2();

    dp.d.m_parentIndex      = (bs_uint8)m_info.parentIndex;
    dp.d.m_childIndex       = (bs_uint8)m_info.childIndex;

    dp.d.m_reverseMotor[0]  = getInfo().reverseTwistMotor;
    dp.d.m_reverseMotor[1]  = getInfo().reverseFirstLeanMotor;
    dp.d.m_reverseMotor[2]  = getInfo().reverseSecondLeanMotor;

    dp.d.m_positionChild    = bSpyVec3fromVector3(get3DofJoint()->GetPositionChild());
    dp.d.m_positionParent   = bSpyVec3fromVector3(get3DofJoint()->GetPositionParent());
    rage::Matrix34 orientChild;
    get3DofJoint()->GetOrientationChild(orientChild);
    dp.d.m_orientChild      = bSpyMat34fromMatrix34(orientChild);
    dp.d.m_orientParent     = bSpyMat34fromMatrix34(get3DofJoint()->GetOrientationParent());

    bspySendPacket(dp);
  }

  void NmRs3DofEffector::sendUpdate()
  {
    Effector3DofUpdatePacket pp((bs_uint8)m_jointIndex, (bs_uint8)m_jointTypeIndex);

    pp.d.m_desired.v[0]   = getDesiredTwist();
    pp.d.m_desired.v[1]   = getDesiredLean1();
    pp.d.m_desired.v[2]   = getDesiredLean2();

    pp.d.m_actual.v[0]    = getActualTwist();
    pp.d.m_actual.v[1]    = getActualLean1();
    pp.d.m_actual.v[2]    = getActualLean2();

    pp.d.m_actualVel.v[0] = getActualTwistVel();
    pp.d.m_actualVel.v[1] = getActualLean1Vel();
    pp.d.m_actualVel.v[2] = getActualLean2Vel();

    pp.d.m_zeroPose       = bSpyVec3fromVector3(m_zeroPoseAngles);

    pp.d.m_stiffness      = getMuscleStiffness();
    pp.d.m_strength       = getMuscleStrength();
    pp.d.m_damping        = getMuscleDamping();

    pp.d.m_stiffnessScale = m_MuscleStiffnessScaling;
    pp.d.m_strengthScale  = m_MuscleStrengthScaling;
    pp.d.m_dampingScale   = m_MuscleDampingScaling;
    pp.d.m_injury         = getInjuryAmount();

    pp.d.m_desiredTwistSetBy    = m_DesiredTwistSetBy;
    pp.d.m_desiredLean1SetBy    = m_DesiredLean1SetBy;
    pp.d.m_desiredLean2SetBy    = m_DesiredLean2SetBy;
    pp.d.m_muscleStiffnessSetBy    = m_MuscleStiffnessSetBy;
    pp.d.m_muscleStrengthSetBy    = m_MuscleStrengthSetBy;
    pp.d.m_muscleDampingSetBy    = m_MuscleDampingSetBy;

    pp.d.m_desiredTwistSetByFrame = m_DesiredTwistSetByFrame;
    pp.d.m_desiredLean1SetByFrame = m_DesiredLean1SetByFrame;
    pp.d.m_desiredLean2SetByFrame = m_DesiredLean2SetByFrame;
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

