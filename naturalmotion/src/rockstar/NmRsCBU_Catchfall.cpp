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
 *
 * the character catches his fall when falling over. 
 * He will twist his spine and look at where he is falling. He will also relax after hitting the ground.
 * He always braces against a horizontal ground.
 */


#include "NmRsInclude.h"
#include "NmRsCBU_Catchfall.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_SpineTwist.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_RollDownStairs.h" 
#include "NmRsCBU_Rollup.h" 
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_Teeter.h"

#define CF_LOG_COLLISSIONS 0

namespace ART
{
  //#define CATCH_FALL_EVENTS
  NmRsCBUCatchFall::NmRsCBUCatchFall(ART::MemoryManager* services) : CBUTaskBase(services, bvid_catchFall),
    m_armReduceSpeed(2.5f),
    m_comVelRDSThresh(2.0f),
    m_armLength(0.6f),
    m_reachLength(m_armLength),
    m_reachLengthMultiplier(1.0f),
    m_inhibitRollingTime(0.2f),
    m_changeFrictionTime(0.2f),
    m_groundFriction(1.0f),
    m_groundFrictionMin(0.0f),
    m_stopManual(0.0f),
    m_stoppedStrengthDecay(5.0f),
    m_spineLean1Offset(0.0f),
    m_handsAndKnees(false),
    m_callRDS(false),
    m_resistRolling(false),
    m_stopOnSlopes(false),
    m_riflePose(false),
    m_antiPropClav(false),
    m_antiPropWeak(false),
    m_headAsWeakAsArms(true),
    m_successStrength(1.0f),
    m_probeLength(m_reachLength + 0.1f),
    m_predictionTime(1.f),
    m_behaviourTime(0.0f),
    m_headAvoidActive(false),
    m_onGround(false),
    m_rdsActivatedByCatchFall(false),
    m_allowFrictionChangeToExtremities(false)
  {
    initialiseCustomVariables();
  }

  NmRsCBUCatchFall::~NmRsCBUCatchFall()
  {
  }

  void NmRsCBUCatchFall::initialiseCustomVariables()
  {
    //Can be set to exclude either or both arms by shotReachForWound/shotFallToKnees.
    //Set back to bvmask_None after each tick so that shotReachForWound/shotFallToKnees doesn't have to worry about resetting it
    m_excludeMask = bvmask_None;

    m_stopOnSlopes = false;
    m_stopManual = 0.0f;
    m_stoppedStrengthDecay = 5.0f;
    m_spineLean1Offset = 0.0f;
  }

  float NmRsCBUCatchFall::getLeftArmLength() const
  {
    float armLength = (getLeftArm()->getElbow()->getJointPosition() - getLeftArm()->getShoulder()->getJointPosition()).Mag();
    armLength += (getLeftArm()->getElbow()->getJointPosition() - getLeftArm()->getWrist()->getJointPosition()).Mag();
    // Add distance between the wrist joint and centre of hand pos.
     armLength += (getLeftArm()->getHand()->getPosition() - getLeftArm()->getWrist()->getJointPosition()).Mag();
    return armLength;
  }

  // Function sets m_reachLengthMultiplier and updates reach length and the probe length.
  void NmRsCBUCatchFall::applyReachLengthMultiplier(float reachLengthMultiplier)
  {
    if (reachLengthMultiplier <= 0.000001f)
      reachLengthMultiplier = 1.0f;
      m_reachLengthMultiplier = reachLengthMultiplier;
      m_reachLength = m_armLength * reachLengthMultiplier;
      m_probeLength = m_reachLength + 0.1f;
    }

  void NmRsCBUCatchFall::onActivate()
  {
    Assert(m_character);

    m_inhibitRollingTimer = m_inhibitRollingTime;
    m_changeFrictionTimer = m_changeFrictionTime;
    m_floorTime = -0.1f;//MMMMHandsKnees
    m_restart = 2.f;//MMMMHandsKnees
    m_fall2Knees = false;
    m_ftk_armsIn = false;
    m_ftk_armsOut = false;

    m_mask = m_parameters.m_effectorMask;
    m_headAvoidActive = false;
    m_onGround = false;
    m_rdsActivatedByCatchFall = false;

    m_fallDirection.Zero();
    m_fallVelocityMag = 0.0f;
    m_forwardsAmount = 0;
    m_bodyStrength = 1.f;
    m_upwardsness = 0.f;
    m_behaviourTime = 0.0f;
    m_OnFloorTimer = 0.0f;

    m_slope = 0.0f;
    m_slopeAlignment = 0.0f;

    m_body->resetEffectors(kResetCalibrations);

    BehaviourMask spineMask = bvmask_LowSpine;
    if(m_parameters.m_useHeadLook)
      spineMask |= bvmask_CervicalSpine;
    getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_torsoStiffness, 0.5f, spineMask);

    m_body->setStiffness(m_parameters.m_armsStiffness-1.f, 1.f, bvmask_ArmLeft | bvmask_ArmRight);

    m_body->setStiffness(m_parameters.m_legsStiffness, 0.5f, bvmask_LegLeft | bvmask_LegRight);

    getLeftArmInputData()->getWrist()->setMuscleStrength(10.0f);
    getRightArmInputData()->getWrist()->setMuscleStrength(10.0f);

    m_body->setOpposeGravity(1.0f);

    if (m_handsAndKnees)//bend knees 
    {
      m_kneeBendL = m_character->getRandom().GetRanged(0.1f, 0.7f);
      m_kneeBendR = m_character->getRandom().GetRanged(0.1f, 0.7f);
    }
    else//have little or no knee bend (to stop character holding legs off ground in forward catch fall)
    {
      m_kneeBendL = m_character->getRandom().GetRanged(0.f, 0.3f);
      m_kneeBendR = m_character->getRandom().GetRanged(0.f, 0.3f);
    }

    m_randomSpineL2 = m_character->getRandom().GetRanged(-0.1f, 0.1f);

    getLeftLegInputData()->getAnkle()->setDesiredLean1(-m_kneeBendL);
    getRightLegInputData()->getAnkle()->setDesiredLean1(-m_kneeBendR);
    getLeftLegInputData()->getHip()->setDesiredLean1(m_kneeBendL*m_bodyStrength);
    getRightLegInputData()->getHip()->setDesiredLean1(m_kneeBendR*m_bodyStrength);
    getLeftLegInputData()->getKnee()->setDesiredAngle(-2*m_kneeBendL*m_bodyStrength);
    getRightLegInputData()->getKnee()->setDesiredAngle(-2*m_kneeBendR*m_bodyStrength);

    m_leftArmState.init(m_character, this);
    m_leftArmState.enter(getLeftArm(), true, bvmask_ArmLeft);
    m_rightArmState.init(m_character, this);
    m_rightArmState.enter(getRightArm(), false, bvmask_ArmRight);
    // Assuming here that the left and right arm have equal lengths.
    m_armLength = getLeftArmLength();

    // Set reachLengthMultiplier and update reach length and probe length.
    // NOTE: This function can also be called by setFallingReaction message.
    applyReachLengthMultiplier(m_reachLengthMultiplier);

    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist];
    spineTwistTask->initialiseCustomVariables();
    //spineTwistTask->setSpineTwistStiffness(m_parameters.m_torsoStiffness);
    spineTwistTask->setSpineTwistTwistClavicles(true);
    spineTwistTask->activate();

    if (m_parameters.m_useHeadLook)
    {
      NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)m_cbuParent->m_tasks[bvid_headLook];
      headLookTask->initialiseCustomVariables();
      headLookTask->updateBehaviourMessage(NULL); // initialise the parameters
      headLookTask->m_parameters.m_stiffness = m_parameters.m_torsoStiffness;
      headLookTask->activate();
    }

    NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
    Assert(rdsTask);
    rdsTask->updateBehaviourMessage(NULL); // sets values to defaults
    rdsTask->m_parameters.m_Stiffness = 8.f;
    rdsTask->m_parameters.m_ForceMag = 0.8f;
    rdsTask->m_parameters.m_UseArmsToSlowDown = -0.9f;
    rdsTask->m_parameters.m_ArmReachAmount = 1.4f;
    rdsTask->m_parameters.m_SpinWhenInAir = false;
    rdsTask->m_parameters.m_LegPush = 0.4f;
    rdsTask->m_parameters.m_ArmL = 0.6f;
    rdsTask->m_parameters.m_useVelocityOfObjectBelow = true;
    rdsTask->m_parameters.m_useRelativeVelocity = true;

#ifdef NM_RS_CBU_ASYNCH_PROBES
    m_character->InitializeProbe(NmRsCharacter::pi_catchFallLeft);
    m_character->InitializeProbe(NmRsCharacter::pi_catchFallRight);
#endif //NM_RS_CBU_ASYNCH_PROBES
    m_effectorMask = bvmask_Full;
#if ART_ENABLE_BSPY
    m_character->setSkeletonVizRoot(10);
    m_character->setSkeletonVizMode(NmRsCharacter::kSV_DesiredAngles);
    BehaviourMask mask = m_character->nameToMask("ur");
    mask |= m_character->nameToMask("ul");
    m_character->setSkeletonVizMask(mask);
#endif // ART_ENABLE_BSPY

    m_faceUpness = 0.0f;
  }

  void NmRsCBUCatchFall::onDeactivate()
  {
    Assert(m_character);

    if (m_parameters.m_useHeadLook)
    {
      m_cbuParent->m_tasks[bvid_headLook]->deactivate();
    }
    m_cbuParent->m_tasks[bvid_spineTwist]->deactivate();
    if (m_rdsActivatedByCatchFall)
      m_cbuParent->m_tasks[bvid_rollDownStairs]->deactivate();

#ifdef NM_RS_CBU_ASYNCH_PROBES
    m_character->ClearAsynchProbe_IfNotInUse(NmRsCharacter::pi_catchFallLeft);
    m_character->ClearAsynchProbe_IfNotInUse(NmRsCharacter::pi_catchFallRight);
#endif

    //Reset friction
    BehaviourMask mask = bvmask_Full;
    if(!m_allowFrictionChangeToExtremities)
      mask &= ~(bvmask_Head | bvmask_Neck | bvmask_HandLeft | bvmask_HandRight | bvmask_FootLeft | bvmask_FootRight); 

    m_character->setFrictionPreScale(1.0f, mask);

    m_character->m_applyMinMaxFriction = false; 

    m_allowFrictionChangeToExtremities = false;

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUCatchFall::onTick(float timeStep)
  {
#if NM_RUNTIME_LIMITS
    // Allow hands to bend back farther
    static float lean1wrist = 0.5236f;
    static float lean2wrist = 1.4f;
    static float twistwrist = 1.4f;
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtWrist_Left)))->setLimits(lean1wrist, lean2wrist, twistwrist);
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtWrist_Right)))->setLimits(lean1wrist, lean2wrist, twistwrist);
#endif

    NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
    Assert(rdsTask);

    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);

    if(m_character->hasCollidedWithWorld(bvmask_Full &~ (bvmask_FootRight | bvmask_FootLeft | bvmask_ShinRight | bvmask_ShinLeft)))
    {
      m_headAvoidActive = true;
      m_onGround = true; // this may (most likely is) naive. will trigger shot on ground reaction if it hits anything.
    }

    //if the character has landed already but continues to move quite quickly e.g. sliding down a slope
    // switch to a rollDownStairs behaviour, switch back to catchfall if the character slows down again.
    if (m_onGround)
    {
      if (m_inhibitRollingTimer >= 0.0f)
        m_inhibitRollingTimer -= timeStep;
      if (m_changeFrictionTimer > 0.0f)
        m_changeFrictionTimer -= timeStep;
    }

    // Bias slope normal a bit against velocity 
    //

    if(m_stopOnSlopes)
    {
      // m_slopeAlignment - Detect sloped ground and the body's alignment relative to the slope -1..1
      //
      rage::Vector3 bodyUp = m_character->m_COMTM.b;
      rage::Vector3 slopeAxis;
      slopeAxis.Cross(m_character->m_groundNormal, m_character->m_gUp);
      slopeAxis.Normalize();
      m_slopeAlignment = slopeAxis.Dot(bodyUp);

      // m_slope - Scale steepness to emphasize the first 15 degrees or so and get a range from 0..1 (flat..~25degrees)
      //
      m_slope = 1.0f - m_character->m_groundNormal.Dot(m_character->m_gUp);
      const float maxSteepness = 0.1f;
      m_slope = rage::Clamp(m_slope, 0.0f, maxSteepness) / maxSteepness;

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", bodyUp);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", m_character->m_groundNormal);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", m_character->m_COMvelMag);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", slopeAxis);
#endif
    }
    else
    {
      m_slope = 0.0f;
      m_slopeAlignment = 1.0f;
    }

    // m_slopeManual set as a minimum slope activation, regardless of the state of m_stopOnSlopes.
    //
    m_slope = rage::Max(m_slope, m_stopManual);

    if (m_changeFrictionTimer <= 0.0f/* && m_changeFrictionTimer > -2.0f*/)
    {
      // Allow hand and foot friction to be uniform (material value is 3x higher than other parts) when rolling or moving fast.
      BehaviourMask handsAndFeet = bvmask_HandLeft | bvmask_HandRight | bvmask_FootLeft | bvmask_FootRight;
      BehaviourMask neck = bvmask_Head | bvmask_Neck;
      m_character->setFrictionPreScale(m_groundFriction, bvmask_Full & ~(handsAndFeet | neck));
      if(m_allowFrictionChangeToExtremities)
      {
        if(m_character->m_COMvelRelativeMag > 5.0f)
        {
          m_character->setFrictionPreScale(m_groundFriction, neck);
          m_character->setFrictionPreScale(m_groundFriction / 3.0f, handsAndFeet);
        }
        else
        {
          m_character->setFrictionPreScale(m_groundFriction, (handsAndFeet | neck));
        }
      }
      m_character->m_minImpactFriction = m_groundFrictionMin;
      m_character->m_applyMinMaxFriction = true; 
    }

    const float velThreshSlopeOffset = 2.0f;
    float comVelRDSThresh = m_comVelRDSThresh + (m_slope * velThreshSlopeOffset);
    bool roll = (m_inhibitRollingTimer < 0.f) && m_callRDS && (m_character->m_COMvelRelativeMag > comVelRDSThresh );
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "CatchFall", comVelRDSThresh);
#endif

    if (m_resistRolling)
      roll = roll && m_character->hasCollidedWithWorld(bvmask_UpperBody);
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "CatchFall", roll);
#endif
    if (roll)
    {
      if (!rdsTask->isActive())
      {
        m_rdsActivatedByCatchFall = true;
        float legAssmetry = m_character->getRandom().GetRanged(0.2f, 0.4f);
        rdsTask->m_parameters.m_AsymmetricalLegs = legAssmetry;
        rdsTask->m_parameters.m_zAxisSpinReduction = m_parameters.m_zAxisSpinReduction;

        rdsTask->activate();
        //Just to be safe feed the arm state from shotReachForWound/shotFallToKnees to the rollup as it may have been deactivated in catchFall
        NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
        Assert(rollUpTask);
        rollUpTask->m_excludeMask = m_excludeMask;
      }

      // look at a point just forward of the pelvis
      if(m_parameters.m_useHeadLook)
      {
        NmRsGenericPart* part = getSpine()->getPelvisPart();
        Assert(part);
        rage::Matrix34 tm;
        part->getMatrix(tm);
        headLookTask->m_parameters.m_pos = tm.d - tm.c * 0.3f;
      }
    }
    else
    {
      m_mask = (m_parameters.m_effectorMask &~ m_excludeMask) & m_effectorMask;

      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      if (m_rdsActivatedByCatchFall && rdsTask->isActive())
      {
        m_rdsActivatedByCatchFall = false;
          rdsTask->deactivate();
      }
      if (m_handsAndKnees)
      {
        if (m_floorTime>0.f)
          m_floorTime += timeStep;
        if (m_floorTime>0.1f && m_floorTime<0.5f)
          dynamicBalancerTask->setHipPitch(-0.5f);//helps the character stand up (hip pitch should be set to zero to keep it standing up)
        //else
        //  dynamicBalancerTask->setHipPitch(0.f);
      }
      if (m_parameters.m_zAxisSpinReduction>0.f)
        m_character->antiSpinAroundVerticalAxisJuice(m_parameters.m_zAxisSpinReduction);

      //m_leftArmState.m_armStrength has been decayed/increased with speed inside the armstate tick
      //armStrengthLeft is max of 0.3,m_leftArmState.m_armStrength but is overridden for handsAndKnees
      //Left arm stiffness is a function of armStrengthLeft but can be overridden to stiffen the downward arm on slopes
      float armStrengthLeft = rage::Max(0.3f, m_leftArmState.m_armStrength);
      float armStrengthRight = rage::Max(0.3f, m_rightArmState.m_armStrength);
      float averageSpeed = rage::Sqrtf(2.f*m_character->getKineticEnergyPerKilo_RelativeVelocity());

      // Decay strength based on speed.
      //
      float add = (averageSpeed*2.f - m_bodyStrength)*m_stoppedStrengthDecay*timeStep;
      m_bodyStrength = rage::Clamp(m_bodyStrength + add, 0.2f, 1.f);

      if (rage::Min(armStrengthLeft, armStrengthRight) < 0.30001f)
        m_character->sendFeedbackFinish(NMCatchFallFeedbackName);
      if (rage::Min(armStrengthLeft, armStrengthRight) < 0.9f && m_bodyStrength < m_successStrength)
      {
        m_character->sendFeedbackSuccess(NMCatchFallFeedbackName);
        //NM_RS_CBU_DRAWPOINT(m_character->m_COM, 1.f, rage::Vector3(1,0,0));
      }

      float strength = m_bodyStrength;
      NM_RS_DBG_LOGF(L"strength: %.3f", strength);

      float effectorStiffnessArms = 0.5f;
      float effectorStiffnessSpine = 0.5f;
      float strengthLeftWrist = rage::Max(0.6f, armStrengthLeft);//stop wrist going too floppy
      float strengthRightWrist = rage::Max(0.6f, armStrengthRight);//stop wrist going too floppy
      if (m_handsAndKnees)
      {
        static float strengthLeftHand = 1.1f;//go lower than arms
        static float strengthRightHand = 1.1f;//go lower than arms
        strengthLeftWrist = strengthLeftHand;
        strengthRightWrist = strengthRightHand;
        if (m_floorTime>0.1f)//Gives a nice sink to the arm impact before pushing back
        {
          //Set joint stiffness parameters for a hands and knees type catchFall
          static float strengthLeftArm = 0.9f;
          static float strengthRightArm = 0.9f;
          static float strengthBody = 0.9f;//0.9f slightly too loose
          static float effectorStiffArms = 1.0f;
          static float effectorStiffSpine = 1.0f;

          armStrengthLeft = strengthLeftArm;
          armStrengthRight = strengthRightArm;
          strength = strengthBody;
          effectorStiffnessArms = effectorStiffArms;
          effectorStiffnessSpine = effectorStiffSpine;

        }
      }

      float spineStrengthscale = 1.0f;
      if(m_handsAndKnees)
        spineStrengthscale = 1.2f;

#define INCREASE_CHEST_STIFFNESS 1
#if INCREASE_CHEST_STIFFNESS
      // Keep spines 2 and 3 strong to avoid weird arched-back pose
      // when settling onto the ground.
      //
      BehaviourMask spineExceptions = bvmask_Spine3 | bvmask_Spine2;
      const float spineExceptionMinStiffness  = 12.0f;
      m_body->setStiffness(
        m_parameters.m_torsoStiffness*strength*spineStrengthscale,
        0.5f,
        bvmask_LowSpine &~ spineExceptions,
        &effectorStiffnessSpine);
      m_body->setStiffness(
        spineExceptionMinStiffness,
        0.5f,
        spineExceptions,
        &effectorStiffnessSpine);
#else
      getSpine()->setBodyStiffness(
        getSpineInput(),
        m_parameters.m_torsoStiffness*strength*spineStrengthscale,
        0.5f,
        bvmask_LowSpine,
        &effectorStiffnessSpine);
#endif

      // Enforce minimum arm stiffness to avoid getting arms wrapped around
      // torso when rolling.
      //
      // m_slopeAlignment measures body orientation to the slope. When positive,
      // the left side is down-slope. Up-slope minArmStrength will be driven
      // negative and Max() will not select it (other term is always positive).
      //
      float minArmStiffnessLeft = 3.0f + 12.0f * m_slope * m_faceUpness * -m_slopeAlignment;
      getLeftArm()->setBodyStiffness(getLeftArmInput(), rage::Max(m_parameters.m_armsStiffness*armStrengthLeft, minArmStiffnessLeft), 1.0f, bvmask_Full, &effectorStiffnessArms);
      float minArmStiffnessRight = 3.0f + 12.0f * m_slope * m_faceUpness * m_slopeAlignment;
      getRightArm()->setBodyStiffness(getRightArmInput(), rage::Max(m_parameters.m_armsStiffness*armStrengthRight, minArmStiffnessRight), 1.0f, bvmask_Full, &effectorStiffnessArms);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "CatchFall", armStrengthLeft);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall", armStrengthRight);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall", minArmStiffnessLeft);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall", minArmStiffnessRight);
      float setStrengthLeft = rage::Max(m_parameters.m_armsStiffness*armStrengthLeft, minArmStiffnessLeft);
      float setStrengthRight = rage::Max(m_parameters.m_armsStiffness*armStrengthRight, minArmStiffnessRight);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall", setStrengthLeft);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall", setStrengthRight);
#endif

      m_body->setStiffness(m_parameters.m_legsStiffness*strength, 0.5f, bvmask_LegLeft | bvmask_LegRight);
      float wristMuscleStiffness = 10.f;
      getLeftArmInputData()->getWrist()->setStiffness((13.f + m_parameters.m_armsStiffness)*0.5f*strengthLeftWrist, 1.f, &wristMuscleStiffness);
      getRightArmInputData()->getWrist()->setStiffness((13.f + m_parameters.m_armsStiffness)*0.5f*strengthRightWrist, 1.f, &wristMuscleStiffness);
      m_body->setStiffness(10.f, 1.f, bvmask_FootLeft | bvmask_FootRight);

      // set head look target
      rage::Vector3 averageFloorVel = (m_rightArmState.m_floorVel + m_leftArmState.m_floorVel) * 0.5f;
      m_character->setFloorVelocityFromColliderRefFrameVel();//This takes precedence over DynamicBalancer but not rollDownStairs

      m_fallDirection = getSpine()->getSpine3Part()->getLinearVelocity() - averageFloorVel;
      m_character->levelVector(m_fallDirection, m_character->vectorHeight(m_fallDirection) - 0.5f*9.8f*m_predictionTime); // prediction into future
      m_fallVelocityMag = m_fallDirection.Mag();
      m_fallDirection.Normalize();

      m_forwardsAmount = -m_fallDirection.Dot(m_character->m_COMTM.c);

      float velForwards = -(m_character->m_COMvelRelative).Dot(m_character->m_COMTM.c);

      NM_RS_DBG_LOGF(L"velForwards: %.3f", velForwards);
      float faceDown = rage::Clamp(m_character->m_COMTM.c.Dot(m_character->getUpVector()), 0.f, 1.f);
      m_upwardsness = rage::Clamp(m_character->m_COMTM.b.Dot(m_character->getUpVector()), 0.f, 1.f);

      NM_RS_DBG_LOGF(L"kneeBendL: %.3f", m_kneeBendL, L", kneeBendR: %.3f", m_kneeBendR);
      getLeftLegInputData()->getAnkle()->setDesiredLean1(-m_kneeBendL);
      getRightLegInputData()->getAnkle()->setDesiredLean1(-m_kneeBendR);

      float pitch = rage::Clamp(-velForwards*2.f, 0.f, 1.f) * m_upwardsness;
      NM_RS_DBG_LOGF(L"pitch: %.3f", pitch);

      float maxExtraSit = 1.f;
      float spineLean1Scale = 0.2f;
      if (/*(m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl) &&*/ !m_handsAndKnees)
      {
        maxExtraSit = 0.5f; // 0.2f;// Extra sit 
        spineLean1Scale = 0.1f;
      }

      //extra sit passed to the spine 
      float extraSit = m_parameters.extraSit * rage::Clamp(pitch + -0.5f*m_forwardsAmount, 0.f, maxExtraSit); 

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-ExtraSit", pitch);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-ExtraSit", m_forwardsAmount);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-ExtraSit", maxExtraSit);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-ExtraSit", extraSit);
#endif

      //extra sit passed to the hips
      float extraSitHips = extraSit;

      if(m_spineLean1Offset > 0.0f)
      {
        // Drastic measure to stop arched-back issue. If problem persists even
        // when extraSit parameter is zeroed, try enabling this.
        extraSitHips = 0.0f;
      }

      if (m_handsAndKnees)
      {
        extraSit=0.8f;
        extraSitHips = extraSit;
        rage::Vector3 bodyBack = m_character->m_COMTM.c;
        float onBackness = -bodyBack.Dot(m_character->m_gUp);
        if (onBackness > 0.f)
        {
          extraSit += rage::Min(2.f*onBackness*1.1f,1.1f/spineLean1Scale/strength);
          extraSitHips = 0.8f - 0.4f*onBackness;
        }
      }

#if ART_ENABLE_BSPY & 1
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Hips", m_spineLean1Offset);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Hips", extraSit);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Hips", spineLean1Scale);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Hips", strength);
#endif
      getSpineInputData()->getSpine0()->setDesiredLean1(m_spineLean1Offset + extraSit*spineLean1Scale*strength);
      getSpineInputData()->getSpine1()->setDesiredLean1(m_spineLean1Offset + extraSit*spineLean1Scale*strength);
      getSpineInputData()->getSpine2()->setDesiredLean1(m_spineLean1Offset + extraSit*spineLean1Scale*strength);
      getSpineInputData()->getSpine3()->setDesiredLean1(m_spineLean1Offset + extraSit*spineLean1Scale*strength);

      getSpineInputData()->getSpine0()->setDesiredLean2(m_randomSpineL2);
      getSpineInputData()->getSpine1()->setDesiredLean2(m_randomSpineL2);
      getSpineInputData()->getSpine2()->setDesiredLean2(m_randomSpineL2);
      getSpineInputData()->getSpine3()->setDesiredLean2(m_randomSpineL2);

      //doingTwist() should really not be used for this purpose
      bool doingTwist = ((NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist])->doingTwist();
      if ((!doingTwist) && (!m_handsAndKnees))
      {
#ifdef NM_COWBOY
        if (m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl)
        {
          // twist into fall
          rage::Matrix34 spine3TM;
          getSpine()->getSpine3Part()->getBoundMatrix(&spine3TM);

          // project fall direction on spine3 yz plane and normalize
          //NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, -spine3TM.c, rage::Vector3(0,0,1));
          rage::Vector3 fallDirProjection;
          fallDirProjection.Cross(spine3TM.a, m_fallDirection);
          fallDirProjection.Cross(spine3TM.a);
          fallDirProjection.Normalize();
          //NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, m_fallDirection, rage::Vector3(1,1,0));
          //NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, fallDirProjection, rage::Vector3(1,0,1));

          // cross with spine3 forward to get twist vector
          rage::Vector3 torqueVector;
          torqueVector.Cross(fallDirProjection, -spine3TM.c);
          //NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, torqueVector, rage::Vector3(1,0,0));

          // twist spine
          float spineTwistAmount = torqueVector.Mag() / -1.0f;
          if(torqueVector.Dot(spine3TM.a) < 0.0f)
            spineTwistAmount *= -1.0f;
          float twistSpeed = 0.7f;
          getSpineInputData()->getSpine0()->setDesiredTwist(getSpine()->getSpine0()->getDesiredTwist() + (spineTwistAmount - getSpine()->getSpine0()->getDesiredTwist())*twistSpeed);
          getSpineInputData()->getSpine1()->setDesiredTwist(getSpine()->getSpine1()->getDesiredTwist() + (spineTwistAmount - getSpine()->getSpine1()->getDesiredTwist())*twistSpeed);
          getSpineInputData()->getSpine2()->setDesiredTwist(getSpine()->getSpine2()->getDesiredTwist() + (spineTwistAmount - getSpine()->getSpine2()->getDesiredTwist())*twistSpeed);
          getSpineInputData()->getSpine3()->setDesiredTwist(getSpine()->getSpine3()->getDesiredTwist() + (spineTwistAmount - getSpine()->getSpine3()->getDesiredTwist())*twistSpeed);

          // apply torque to spine0 to counter spine twist
          float torqueScale = 1.0f/(1.0f+20.0f*m_behaviourTime);
          if(torqueScale > 0.1)
          {
            torqueScale *= -50.0f * torqueScale;
            getSpine()->getSpine0Part()->applyTorque(torqueVector * torqueScale);
            //NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, torqueVector * torqueScale, rage::Vector3(1,0,0));
          }
        } 
        else 
#endif
        {
          float timeScale = 1.f/(1.f + 0.4f*30.f*timeStep);
          getSpineInputData()->getSpine0()->setDesiredTwist(getSpine()->getSpine0()->getDesiredTwist()*timeScale);
          getSpineInputData()->getSpine1()->setDesiredTwist(getSpine()->getSpine1()->getDesiredTwist()*timeScale);
          getSpineInputData()->getSpine2()->setDesiredTwist(getSpine()->getSpine2()->getDesiredTwist()*timeScale);
          getSpineInputData()->getSpine3()->setDesiredTwist(getSpine()->getSpine3()->getDesiredTwist()*timeScale);
        }
      }

      getLeftLegInputData()->getHip()->blendToZeroPose((NmRsEffectorBase*)getLeftLeg()->getHip(), 1.f);
      getRightLegInputData()->getHip()->blendToZeroPose((NmRsEffectorBase*)getRightLeg()->getHip(), 1.f);

#if ART_ENABLE_BSPY & 1
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Hips", extraSitHips);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Hips", strength);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Hips", m_kneeBendL);
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Hips", m_kneeBendR);
      float lean1Left = (m_kneeBendL+extraSitHips)*strength;
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Hips", lean1Left);
      float lean1Right = (m_kneeBendR+extraSitHips)*strength;
      bspyScratchpad(m_character->getBSpyID(), "CatchFall-Hips", lean1Right);
#endif

      getLeftLegInputData()->getHip()->setDesiredLean1((m_kneeBendL+extraSitHips)*strength - m_spineLean1Offset);
      getRightLegInputData()->getHip()->setDesiredLean1((m_kneeBendR+extraSitHips)*strength - m_spineLean1Offset);

      if(m_stopOnSlopes || m_stopManual > 0.0f)
      {
        // Spread legs out a bit when moving slowly and on sloped ground.
        //
        const float velThreshold = 2.0f; // TODO threshold based on slope?
        
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", velThreshold);
        bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", m_character->m_COMvelMag);
        bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", m_resistRolling);
        bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", m_slope);
        bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", rage::Abs(m_slopeAlignment));
        bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", m_onGround);
#endif

        // Spread 
        //
        if((m_character->m_COMvelMag < velThreshold || m_resistRolling) && ((m_slope * rage::Abs(m_slopeAlignment)) > 0.0f) && m_onGround)
        {
          // Blend between current desired lean 2 and target based on combined
          // slope steepness, alignment and faceupdownness. Apply to hips.
          //
          m_faceUpness = -m_character->m_COMTM.c.Dot(m_character->m_groundNormal);
          float blend = m_slope * rage::Abs(m_slopeAlignment) * rage::Abs(m_faceUpness);
          float invBlend = 1.0f - blend;
          const float targetLean = -0.7f;
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", blend);
          bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", invBlend);
          bspyScratchpad(m_character->getBSpyID(), "CatchFall-Slope", targetLean);
#endif
          getLeftLegInputData()->getHip()->setDesiredLean2(invBlend * getLeftLegInputData()->getHip()->getDesiredLean2() + blend * targetLean);
          getRightLegInputData()->getHip()->setDesiredLean2(invBlend * getRightLegInputData()->getHip()->getDesiredLean2() + blend * targetLean);
        }
      }

      float kneeScale = (1.f+faceDown)*strength - faceDown; // this complex line is to get the guy to not have his knee at an extreme angle when falling on his front

      if (m_handsAndKnees)
      {
        kneeScale = 1.f;
        getLeftLegInputData()->getHip()->setDesiredLean2(-0.3f);
        getRightLegInputData()->getHip()->setDesiredLean2(-0.3f);

        getLeftLegInputData()->getHip()->setDesiredTwist(-0.0f);
        getRightLegInputData()->getHip()->setDesiredTwist(-0.0f);
      }
      //minimum kneebend below is enough to just keep knee on floor if lying flat onface. i.e. knee/ankle/toe triangle
      getLeftLegInputData()->getKnee()->setDesiredAngle(rage::Min(-2.f*m_kneeBendL*kneeScale,-0.3f));
      getRightLegInputData()->getKnee()->setDesiredAngle(rage::Min(-2.f*m_kneeBendR*kneeScale,-0.3f));

      rage::Vector3 headTarget = getSpine()->getHeadPart()->getPosition() + m_fallDirection;
      //NM_RS_CBU_DRAWPOINT(headTarget, 0.2f, rage::Vector3(1,0,0));

      if (m_parameters.m_useHeadLook)
      {
        rage::Vector3 headTargetVel = getSpine()->getSpine3Part()->getLinearVelocity();
        if (m_handsAndKnees)
        {
          //look horizontally forward
          rage::Matrix34 tmCom;
          getSpine()->getSpine2Part()->getBoundMatrix(&tmCom); 
          rage::Vector3 headUp = -tmCom.c;  //spine forward(-c)
          if (headUp.Dot(m_character->m_gUpReal) < 0.0f)//front down
            headUp += tmCom.a; //spine up(a)
          else//lying  on back/back down
            headUp -= tmCom.a; //spine down(-a)
          m_character->levelVector(headUp);
          headTarget = getSpine()->getHeadPart()->getPosition() + headUp;
        }
        headLookTask->m_parameters.m_pos = headTarget;
        headLookTask->m_parameters.m_instanceIndex = -1;
        float neckStrength = rage::Max(armStrengthLeft, armStrengthRight);
        if (!m_headAsWeakAsArms)
          neckStrength = rage::Max(neckStrength, m_bodyStrength);
        headLookTask->m_parameters.m_stiffness = (m_parameters.m_torsoStiffness+2.f)*neckStrength;
        headLookTask->m_parameters.m_vel = headTargetVel;
        if (!headLookTask->isActive())
          headLookTask->activate();
      }

      NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist];
      spineTwistTask->setSpineTwistPos(headTarget);
      if (!spineTwistTask->isActive())
        spineTwistTask->activate();//Point gun deactivation turns this off

      if (m_handsAndKnees)
      {
        NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
        Assert(balColReactTask);

        if ((m_character->hasCollidedWithWorld(bvmask_Arms) || !dynamicBalancerTask->isActive()) && 
          (!(dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK && (balColReactTask->m_balancerState == bal_Drape || balColReactTask->m_balancerState == bal_DrapeForward))))
        {
          m_effectorMask = bvmask_Full;
          //redundant lines below
          dynamicBalancerTask->taperKneeStrength(false);//in order that the character is to stand up from bent legs / bent legs should be strong enough
          dynamicBalancerTask->setStepWithBoth(true);
        }
        else
        {
          m_effectorMask = bvmask_UpperBody;
          //redundant line below
          dynamicBalancerTask->setStepWithBoth(false);
        }
      }

      m_leftArmState.tick(timeStep);
      m_rightArmState.tick(timeStep);

#if defined(CATCH_FALL_EVENTS)
      if (m_character->hasCollidedWithWorld(bvmask_UpperBody) || getSpine()->getPelvisPart()->collidedWithEnvironment()) // bit quicker, more readable (I had to look up this new fangled s5 mask)
      {
        ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
        if (feedback)
        {
          feedback->m_agentID = m_character->getID();
          feedback->m_argsCount = 1; // there is one argument – the type of “success”

          ART::ARTFeedbackInterface::FeedbackUserdata data;
          data.setInt(2);//Success state:0 = leaned over,  1= l+r arms have collided (and stayed collided for over 0.5secs), 2 = THIS ub or pelvis collided
          feedback->m_args[0] = data;

          strcpy(feedback->m_behaviourName, NMCatchFallFeedbackName);
          feedback->onBehaviourEvent();
        }
      }

      rage::Vector3 headPos = getSpine()->getHeadPart()->getPosition();
      rage::Vector3 pelvisPos = getSpine()->getPelvisPart()->getPosition();
      rage::Vector3 toHead = headPos - pelvisPos;
      toHead.Normalize();
      float lean = toHead.Dot(m_character->getUpVector());
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      // 0.3f threshold needs to be exposed
      if (feedback && lean < 0.3f)
      {
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 1; // Dana - there is one argument – the type of “success”
        ART::ARTFeedbackInterface::FeedbackUserdata data;
        data.setInt(0);//Success state:0 = THIS leaned over,  1= l+r arms have collided (and stayed collided for over 0.5secs), 2 = ub or pelvis collided
        feedback->m_args[0] = data;
        strcpy(feedback->m_behaviourName, NMCatchFallFeedbackName);
        feedback->onBehaviourEvent();
      }
#endif

      if (m_character->hasCollidedWithWorld(bvmask_ArmLeft) && m_character->hasCollidedWithWorld(bvmask_ArmRight))
      {
        m_OnFloorTimer += timeStep;
        if (m_OnFloorTimer > 0.5f) // && stationary?
        {
          ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
          if (feedback)
          {
            feedback->m_agentID = m_character->getID();
            feedback->m_argsCount = 1;

            ART::ARTFeedbackInterface::FeedbackUserdata data;
            data.setInt(1);//Success state:0 = dunno, 2 = hands+Knees, 3 = sitting down
            feedback->m_args[0] = data;
#if ART_ENABLE_BSPY
            strcpy(feedback->m_behaviourName, NMCatchFallFeedbackName);
#endif
            feedback->onBehaviourEvent();
          }
        }
      }
      else
      {
        m_OnFloorTimer = 0.f;
      }


      if(headLookTask->isActive())
      {
        headLookTask->m_parameters.m_keepHeadAwayFromGround = m_headAvoidActive && (m_hkHeadAvoid || !m_handsAndKnees);
      }
      else if (m_headAvoidActive && (m_hkHeadAvoid || !m_handsAndKnees))
      {
        const float blend = 1.0f;
        NmRsBodyStateHelper helper(m_body, m_bvid, m_priority, 1, blend, bvmask_Full DEBUG_LIMBS_PARAMETER("CatchFall-HeadAvoid"));
        NM_RS_DBG_LOGF(L"normalX: ", 0, ", avoid: ", 0.7f);
        rage::Vector3 down = -m_fallDirection;
        getSpine()->keepHeadAwayFromGround(getSpineInput(), 1.f, &down);//mmmmtodo this doesn't work if headlook on as headlook overides
      }

      //Hold rifle in a safe position to reduce complications with collision
      if (m_riflePose && m_character->getCharacterConfiguration().m_rightHandState == CharacterConfiguration::eHS_Rifle)
      {
        //case npsRifleFall:
        getRightArmInputData()->getShoulder()->setDesiredAngles(0.2f, 0.0f, 0.0f);
        getRightArmInputData()->getShoulder()->setDesiredAngles(0.53f, 0.35f, -0.2f);
        getRightArmInputData()->getElbow()->setDesiredAngle(2.0f);
        getRightArmInputData()->getShoulder()->setDesiredAngles(0.0f, 0.0f, 0.0f);
      }
      //if (m_character->getCharacterConfiguration().m_leftHandState == CharacterConfiguration::eHS_Rifle)
      //{
      //  //case npsRifleFall:
      //  getRightArmInputData()->getShoulder()->setDesiredAngles(0.2f, 0.0f, 0.0f);
      //  getRightArmInputData()->getShoulder()->setDesiredAngles(0.53f, 0.35f, -0.2f);
      //  getRightArmInputData()->getElbow()->setDesiredAngle(2.0f);
      //  getRightArmInputData()->getShoulder()->setDesiredAngles(0.0f, 0.0f, 0.0f);
      //}
      //Set by shot fallToKnees
      if (m_fall2Knees)
      {
        //Give the hips more strength as the fallToKnees shot goes from a Knee hit to landing on front
        //This is to try and have a flat landing on the chest to get a satisfiying impact.  
        //  (Otherwise the angle between thighs and spine dampens the landing)         
        getLeftLegInputData()->getHip()->setMuscleStrength(81.f);
        getRightLegInputData()->getHip()->setMuscleStrength(81.f);

        //Optionally get the arms to the side of the torso so they don't dampen the impact with the chest
        if (m_ftk_armsIn || m_ftk_armsOut)
        {
          if (m_ftk_armsIn)
          {
            getLeftArmInputData()->getShoulder()->setDesiredAngles(0.f,0.9f,0.7f);
            getRightArmInputData()->getShoulder()->setDesiredAngles(0.f,0.7f,0.7f);
          }
          else//fling out arms
          {
            getLeftArmInputData()->getShoulder()->setDesiredAngles(0.f,-0.5f,-0.7f);
            getRightArmInputData()->getShoulder()->setDesiredAngles(0.f,-0.9f,-0.7f);
            float minStrength = rage::Min(m_rightArmState.m_armStrength, m_leftArmState.m_armStrength);
            m_leftArmState.m_armStrength = minStrength;
            m_rightArmState.m_armStrength = minStrength;
          }
          getLeftArmInputData()->getElbow()->setDesiredAngle(0.9f);
          getRightArmInputData()->getElbow()->setDesiredAngle(0.6f);
        }
      }
      m_behaviourTime += timeStep;
    }
    //Assumes no early return from tick - which there isn't
    //Set back to bvmask_None after each tick so that shotReachForWound/shotFallToKnees doesn't have to worry about resetting it
    m_excludeMask = bvmask_None;

    return eCBUTaskComplete;
  }

  void NmRsCBUCatchFall::ArmState::armIK(NmRsIKInputWrapper* ikInputData, const rage::Vector3 &target, float armTwist, float dragReduction, const rage::Vector3 *vel)
  {
    float straightness = 0.4f;
    float maxSpeed = 200.f; // ie out of range

    //If teeter says so then restrict the hands to not reach over the edge 
    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_parent->m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);
    NmRsCBUTeeter* teeterTask = (NmRsCBUTeeter*)m_parent->m_cbuParent->m_tasks[bvid_teeter];
    Assert(teeterTask);
    rage::Vector3 targetCopy(target);
    if (teeterTask->isActive() && teeterTask->restrictCatchFallArms())
    {
      m_character->levelVector(targetCopy,m_character->m_gUp.Dot(balColReactTask->m_pos1stContact));
      float exclusionZone = 0.05f;
      if (balColReactTask->m_normal1stContact.Dot(targetCopy-balColReactTask->m_pos1stContact-balColReactTask->m_normal1stContact*exclusionZone)*balColReactTask->m_sideOfPlane > 0.f)//increase offset
      {
        targetCopy -= balColReactTask->m_normal1stContact.Dot(targetCopy - balColReactTask->m_pos1stContact-balColReactTask->m_normal1stContact*exclusionZone)* balColReactTask->m_normal1stContact;
        m_character->levelVector(targetCopy,balColReactTask->m_pos1stContact.z);
      }
      else
        targetCopy.Set(target);
    }

    ikInputData->setTarget(targetCopy);
    ikInputData->setTwist(armTwist);
    ikInputData->setDragReduction(dragReduction);
    if(vel)
    {
      ikInputData->setVelocity(*vel);
    }
    ikInputData->setTwistIsFixed(false);
    ikInputData->setAdvancedStaightness(straightness);
    ikInputData->setAdvancedMaxSpeed(maxSpeed);
    ikInputData->setUseAdvancedIk(true);
    ikInputData->setMaxReachDistance(m_parent->m_reachLength);
	  ikInputData->setMatchClavicle(kMatchClavicleUsingTwist);

#if ART_ENABLE_BSPY
    if (m_isLeftArm)
      m_parent->m_reachTargetL = target;
    else
      m_parent->m_reachTargetR = target;
#endif // ART_ENABLE_BSPY
  }

  void NmRsCBUCatchFall::ArmState::wristIK(NmRsIKInputWrapper* ikInputData, const rage::Vector3 &wristTarget, const rage::Vector3 &wristNormal)
  {
#if ART_ENABLE_BSPY
    m_character->m_currentSubBehaviour = "-wristIK"; 
#endif

    bool useActualAngles = false;
    float twistLimit = 2.4f;
    if (m_parent->m_handsAndKnees)//mmmmtodo recheck this makes a difference
    {
      useActualAngles = true;
      twistLimit = 1.f;
    }

    ikInputData->setWristUseActualAngles(useActualAngles);
    ikInputData->setWristTwistLimit(twistLimit);
    ikInputData->setWristTarget(wristTarget);
    ikInputData->setWristNormal(wristNormal);

#if ART_ENABLE_BSPY
    m_character->m_currentSubBehaviour = ""; 
#endif
  }

  void NmRsCBUCatchFall::ArmState::enter(NmRsHumanArm * armSetup, bool isLeftArm, BehaviourMask armMask)
  {
    m_armSetup = armSetup;
    m_isLeftArm = isLeftArm;
    m_armStrength = 1.f;
    m_armMask = armMask;
    m_onBackRandomL1 = m_character->getRandom().GetRanged(-0.4f, 0.f);
    m_onBackRandomL2 = m_character->getRandom().GetRanged(0.f, 0.5f);
    m_maxElbowAngleRandom = m_character->getRandom().GetRanged(0.7f, 1.3f);
  }

  void NmRsCBUCatchFall::ArmState::tick(float timeStep)
  {
    NmRsHumanArm* arm = NULL;
    NmRsArmInputWrapper* inputData = NULL;

    if(m_isLeftArm)
    {
      arm = m_parent->getLeftArm();
      inputData = m_parent->getLeftArmInputData();
    }
    else
    {
      arm = m_parent->getRightArm();
      inputData = m_parent->getRightArmInputData();
    }

    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)m_parent->m_cbuParent->m_tasks[bvid_spineTwist];
    float armTwist = rage::Max(0.0f, 0.5f - m_parent->m_forwardsAmount); // rotating elbows in on forwards and out on backwards
    NM_RS_DBG_LOGF_FROMPARENT(L">>>armTwist: %.3f", armTwist);
    rage::Vector3 bodyUpModified = m_character->m_COMTM.b;
    rage::Vector3 side;
    side.Cross(bodyUpModified, m_character->m_gUp);
    if (side.Mag2() > 1e-10f)
      side.Normalize();
    rage::Vector3 leanDir;
    leanDir.Cross(bodyUpModified, side);
    rage::Vector3 zAxis = m_character->m_COMTM.c;
    if (m_parent->m_forwardsAmount > 0.f)
    {
      bodyUpModified += leanDir; // -= zAxis;
      bodyUpModified.Normalize();
      bodyUpModified.Scale( 0.4f + 0.6f * rage::Clamp(m_parent->m_upwardsness, 0.f, 1.f) );
    }

    rage::Vector3 shoulderPos = m_armSetup->getShoulder()->getJointPosition();
    shoulderPos += bodyUpModified * rage::Clamp(m_parent->m_forwardsAmount, m_parent->m_parameters.m_backwardsMinArmOffset, m_parent->m_parameters.m_forwardMaxArmOffset);
    rage::Matrix34 tmCom;
    m_armSetup->getClaviclePart()->getBoundMatrix(&tmCom);


    // Put arms out more for better hands and knees stability unless on back or sitting up
    //
    float armOutMag = 0.2f;

#if 0 // Request for wider hand placement (to match getup). Try like this for a bit and make sure it doesn't break other aspects of the performance.
    if (!m_parent->m_handsAndKnees || (m_parent->m_forwardsAmount<-0.3f && m_character->hasCollidedWithWorld(m_armMask)))
      armOutMag = 0.0f;
#endif

    // Put arms out even more if we're on a slope and body axis is perpendicular to slope.
    //
    armOutMag += m_parent->m_slope * rage::Abs(m_parent->m_slopeAlignment) * 0.4f;

    rage::Vector3 armOut = -armOutMag*tmCom.b;

#if ART_ENABLE_BSPY
    char name[64];
    if(m_isLeftArm)
    {
      sprintf(name, "CatchFall-Left");
    }
    else
    {
      sprintf(name, "CatchFall-Right");
    }
    bspyScratchpad(m_character->getBSpyID(), name, armOut);
#endif

    //NM_RS_CBU_DRAWVECTORCOL(m_armSetup->getClaviclePart()->getPosition(), -tmCom.b, rage::Vector3(1,1,1)); 

    // probe the direction we are falling
    //
    rage::Vector3 probeEnd = shoulderPos + m_parent->m_fallDirection*m_parent->m_probeLength + armOut;
    rage::Vector3 probeEndHit;
    rage::Vector3 handPos = m_armSetup->getHand()->getPosition();
    rage::Vector3 groundNormal = m_character->getUpVector();
    rage::Vector3 groundNormalHit;
    m_timeToImpact = m_parent->m_probeLength * m_parent->m_fallVelocityMag;
    NmRsCharacter::rayProbeIndex armProbeIndex = NmRsCharacter::pi_catchFallRight;
    if (m_isLeftArm)
      armProbeIndex = NmRsCharacter::pi_catchFallLeft;
    bool hasHit = m_character->probeRay(armProbeIndex, shoulderPos, probeEnd, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);

    //m_floorVel.Set(0,0,0);
    m_floorVel = m_character->getFloorVelocity();
    //NM_RS_CBU_DRAWLINE(shoulderPos, probeEnd, rage::Vector3(1.0f,0.0f,0.0f));
    bool allowVelFromProbe = true;
    if (hasHit)
    {
      probeEndHit = m_character->m_probeHitPos[armProbeIndex];
      groundNormalHit = m_character->m_probeHitNormal[armProbeIndex];
      if (m_character->IsInstValid(armProbeIndex))
      {
        //ignore velocity if hit say a gun //catchFall only uses m_floorVel for the extraSit
        //  bool allowVelFromProbe = true;
        if (m_character->getDontRegisterProbeVelocityActive())
        {
          rage::Vector3 objectSize = m_character->m_probeHitInstBoundingBoxSize[armProbeIndex];
          float vol = objectSize.x * objectSize.y * objectSize.z;
          allowVelFromProbe = PHLEVEL->IsFixed(m_character->m_probeHitInstLevelIndex[armProbeIndex]) ||
            ((m_character->m_probeHitInstMass[armProbeIndex] >= m_character->getDontRegisterProbeVelocityMassBelow())
            && (vol >= m_character->getDontRegisterProbeVelocityVolBelow()));
#if ART_ENABLE_BSPY
          if (!allowVelFromProbe)
            bspyScratchpad(m_character->getBSpyID(), "CF DontRegisterProbeVelocity", m_character->m_probeHitInstLevelIndex[armProbeIndex]);
#endif
        }
        if (allowVelFromProbe)
        {
          m_character->getVelocityOnInstance(m_character->m_probeHitInstLevelIndex[armProbeIndex],probeEnd,&m_floorVel);
        }
        else
        {
          //ignore this hit - it usually has a strange normal mmmmtodo more explanation 
          hasHit = false;
        }
      }

      // time to impact?
      rage::Vector3 toImpact = probeEndHit - shoulderPos;
      m_timeToImpact = toImpact.Mag() / m_parent->m_fallVelocityMag;
    }

    // Function calculates m_reachLength value that is used in IK to clamp to maximum reaching distance.
    // NOTE: Function has to be called before armIK step.
    m_parent->updateReachLength(hasHit);

    if (hasHit)
    {
      probeEnd = probeEndHit;
      groundNormal = groundNormalHit;

      //If probe only just hit something make the probe longer next time to stop jitter
      rage::Vector3 probeVec = m_parent->m_fallDirection*m_parent->m_probeLength + armOut;
      rage::Vector3 probeHitVec = probeEndHit - shoulderPos;
      if (rage::Abs(probeVec.Mag() - probeHitVec.Mag()) < 0.005)
        m_parent->m_probeLength = m_parent->m_reachLength + 0.105f;
      else
        m_parent->m_probeLength = m_parent->m_reachLength + 0.1f;
#if ART_ENABLE_BSPY
      if (allowVelFromProbe)
      {
        m_character->bspyDrawLine(shoulderPos,probeEnd,rage::Vector3(1.0f,0.0f,0.0f));
        m_character->bspyDrawLine(probeEnd,probeEnd+groundNormal,rage::Vector3(1.0f,1.0f,1.0f));
      }
      else
      {
        m_character->bspyDrawLine(shoulderPos,probeEnd,rage::Vector3(0.0f,0.0f,1.0f));
        m_character->bspyDrawLine(probeEnd,probeEnd+groundNormal,rage::Vector3(0.4f,0.40f,0.4f));
      }
#endif
    }
#if ART_ENABLE_BSPY
    else
    {
      if (allowVelFromProbe)
      {
        m_character->bspyDrawLine(shoulderPos,probeEnd,rage::Vector3(0.0f,1.0f,0.0f));
        m_character->bspyDrawLine(probeEnd,probeEnd+groundNormal,rage::Vector3(0.8f,0.80f,0.8f));
      }
      else
      {
        m_character->bspyDrawLine(shoulderPos,probeEnd,rage::Vector3(0.0f,0.0f,1.0f));
        m_character->bspyDrawLine(probeEnd,probeEnd+groundNormal,rage::Vector3(0.8f,0.80f,0.8f));
      }
    }
#endif

    // adjust strength based on whether the arms have collided
    // not sure exactly what is going on here.
    //
    //Keep armStrength high if only arms hit the ground if falling forward (strength will ramp down when spine hits ground)
    //Set armStrength to minimum if falling backwards as soon as the arms hit the floor (stops the character getting stuck propped up on elbows)
    if (m_parent->m_antiPropWeak && m_parent->m_forwardsAmount<0.f)
      m_armStrength = rage::Min(0.3f, m_armStrength);
    float maxStrength = rage::Min(1.f, m_parent->m_bodyStrength*3.f);
    if (m_character->hasCollidedWithWorld(m_armMask))
    {
      //arm strength is <= bodyStrength
      m_armStrength = rage::Min(m_armStrength - m_parent->m_armReduceSpeed*timeStep, m_parent->m_bodyStrength);
    }
    else
    {
      m_armStrength = rage::Min(m_armStrength + 2.f*timeStep, maxStrength);
    }

    shoulderPos = m_armSetup->getShoulder()->getJointPosition();

    // hands and knees abuses the probe distance to push the character off the ground.
    //
    if (m_parent->m_handsAndKnees)
    {
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_parent->m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      //Lying on back 
      if (m_parent->m_forwardsAmount<-0.3f)// && m_character->hasCollidedWithWorld(m_armMask))
      {
        //push off with hands
        probeEnd = handPos-groundNormal*0.7f;
        //lean forward if balancer on
        dynamicBalancerTask->setHipPitch(-0.5f+3.5f*(m_parent->m_forwardsAmount+0.3f));
        m_parent->m_effectorMask = bvmask_Full;
        //below gives more colour to legs, but less stable and perhaps too many bad leg poses
        //if (m_character->hasCollidedWithWorld("ub"))
        //{
        //  //lean forward if balancer on
        //  dynamicBalancerTask->setHipPitch(-0.5f+3.5f*(m_parent->m_forwardsAmount+0.3f));
        //  m_parent->m_parameters.m_effectorMask[0] = 'f';
        //}

#if ART_ENABLE_BSPY
        const bool CFPushOff = true;
        bspyScratchpad(m_character->getBSpyID(), name, CFPushOff);
#endif
      }
    }
    else
    {
      if (m_character->hasCollidedWithWorld(m_armMask))
        probeEnd = (probeEnd + handPos)*0.5f;

      if (m_parent->m_forwardsAmount<-0.5f && m_character->hasCollidedWithWorld(m_armMask))
        probeEnd = handPos;
    }

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), name, shoulderPos);
    bspyScratchpad(m_character->getBSpyID(), name, probeEnd);
#endif
    rage::Vector3 target = probeEnd - shoulderPos;

    float dist = target.Mag();
    float pushOff = 0.4f;
    if (m_parent->m_handsAndKnees)
    {
      //if (m_parent->m_floorTime>0.1f)//to sink then pushoff
      //pushOff = 0.9f;//to spring up from arms
      pushOff = 0.7f;//to push up from arms
    }

    if (dist > m_parent->m_reachLength)
      target *= m_parent->m_reachLength / dist;
    else if (dist < pushOff)
      target *= pushOff / (dist + 0.0001f);
    target += shoulderPos;

    if (m_parent->m_handsAndKnees)
    {
      //hand orientated wrong lift it up - only lift off one hand at time
      //This code simulates taking a hands and knees arm step sometimes - maybe set this off another way to increase movement
      rage::Vector3 normal2BackofHand,fingers2Wrist;
      rage::Matrix34 handTM;
      m_armSetup->getHand()->getBoundMatrix(&handTM);
      normal2BackofHand = handTM.a;
      if (m_isLeftArm)
        normal2BackofHand *= -1.f;
      fingers2Wrist = handTM.b;
      bool otherHandIsLifted = (m_isLeftArm ? m_parent->m_rightArmState.m_liftArm : m_parent->m_leftArmState.m_liftArm);
      NM_RS_DBG_LOGF_FROMPARENT(L"BackOfHand %f", normal2BackofHand.Dot(groundNormal));
      NM_RS_DBG_LOGF_FROMPARENT(L"fingers2Wrist %f", fingers2Wrist.Dot(groundNormal));
      if (normal2BackofHand.Dot(groundNormal)<0.0f && fingers2Wrist.Dot(groundNormal)>0.7f && !otherHandIsLifted)
      {
        m_liftArm = true;
        m_liftArmTime = 5.f*0.0167f;
        target = probeEnd - shoulderPos;
        target += 0.6f*groundNormal;
        if (dist > m_parent->m_reachLength)
          target *= m_parent->m_reachLength / dist;
        target += shoulderPos;

        //handPos += 0.6f*groundNormal;
#if ART_ENABLE_BSPY
        const bool lifting = true;
        bspyScratchpad(m_character->getBSpyID(), name, lifting);
#endif

      }
      else
      {
        if (m_liftArm)
          m_liftArmTime -= timeStep;
        if (m_liftArmTime<0.f)
          m_liftArm = false;
      }
    }

    NmRsLimbInput ikInput = m_parent->createNmRsLimbInput<NmRsIKInputWrapper>(-1, 1.0f DEBUG_LIMBS_PARAMETER("IK"));
    NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();

    ikInputData->setCanDoIKGreedy(true);

    rage::Vector3 vel = m_armSetup->getClaviclePart()->getLinearVelocity();
    if (m_character->hasCollidedWithWorld(m_armMask))
    {
      // If probe has hit IK arm to target, otherwise IK arm to current hand
      // position (will drift!)
      //
      if (hasHit)
      {
        //crude way of stopping alot of the hands and knees handstanding and arms lifting whole body when lying on side on top of an arm
        if (m_parent->m_handsAndKnees)
        {
          float mag = (target-handPos).Mag();
          if (mag > 0.5f)
            target = handPos + 0.25f*(target-handPos);
        }
#if ART_ENABLE_BSPY && CF_LOG_COLLISSIONS
        if(m_isLeftArm) {
          bspyLogf(info, L"left arm IK: collided[X] probe[X]");
        } else {
          bspyLogf(info, L"right arm IK: collided[X] probe[X]");
        }
#endif
        armIK(ikInputData, target, armTwist, 0);//collided and hit
        if (m_parent->m_handsAndKnees)
        {
          if (m_parent->m_floorTime <= 0.f)
          {
            m_parent->m_floorTime = 0.00001f;
            m_parent->m_restart = 0.1f;
          }
          if (m_parent->m_restart<1.f)
          {
            m_parent->m_restart = 0.1f;
          }
        }
      }
      else
      {
#if ART_ENABLE_BSPY && CF_LOG_COLLISSIONS
        if(m_isLeftArm) {
          bspyLogf(info, L"left arm IK: collided[X] probe[ ]");
        } else {
          bspyLogf(info, L"right arm IK: collided[X] probe[ ]");
        }
#endif
        armIK(ikInputData, handPos, armTwist, 0);//collided and nohit
      }
    }
    else //(m_character->hasCollidedWithWorld(m_armMask))
    {
      if (m_parent->m_handsAndKnees)
      {
        if (m_parent->m_restart<1.f)
          m_parent->m_restart -= timeStep;
        if (m_parent->m_restart<0.f)
          m_parent->m_floorTime = -1.f;
      }
#ifdef NM_COWBOY
      if ((m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl) && !m_parent->m_handsAndKnees) 
      {
        // determine if the character is falling backwards
        rage::Matrix34 spine0TM;
        m_parent->getSpine()->getSpine0Part()->getBoundMatrix(&spine0TM);
        rage::Vector3 fallDirProjection;
        fallDirProjection.Cross(spine0TM.a, m_parent->m_fallDirection);
        fallDirProjection.Cross(spine0TM.a);
        fallDirProjection.Normalize();
        float angleOfAcceptance = 0.6f;

        // debug draw arc of acceptance
//#if NM_RS_ENABLE_DEBUGDRAW//mmmmTodo convert to bspy
//        if (rage::NMRenderBuffer::getInstance())
//        {
//          float angle = angleOfAcceptance;
//          float size = 0.5f;
//          int segments = 10;
//          float stepAngle = 2.0f*angle/(float)segments;
//          rage::Vector3 axis = spine0TM.a;        // "hinge axis"
//          rage::Vector3 sweep = spine0TM.c*size;  // initialise sweep to center of arc
//          rage::Vector3 lastSweep;
//          rage::Vector3 location = spine0TM.d;
//          rage::Quaternion rot;
//          rot.FromRotation(axis*-angle);
//          rot.Transform(sweep);
//          //NM_RS_CBU_DRAWVECTORCOL(location, sweep, rage::Vector3(1,1,0));
//          for(int i = 0; i < segments; i++) {
//            lastSweep = sweep;
//            rot.FromRotation(axis*stepAngle);
//            rot.Transform(sweep);
//            //NM_RS_CBU_DRAWLINE(location+lastSweep, location+sweep, rage::Vector3(1,1,0));
//          }
//          //NM_RS_CBU_DRAWVECTORCOL(location, sweep, rage::Vector3(1,1,0));
//          //NM_RS_CBU_DRAWARROW(location, location+fallDirProjection*size, rage::Vector3(0,0,1));
//        }
//#endif
        if(fallDirProjection.Dot(spine0TM.c) > rage::Cosf(angleOfAcceptance))
        {
          // put arms up, cowboy-style
          //NM_RS_CBU_DRAWVECTORCOL(spine0TM.d, spine0TM.a, rage::Vector3(1,0,0));
          //NM_RS_CBU_DRAWVECTORCOL(spine0TM.d, spine0TM.b, rage::Vector3(0,1,0));
          //NM_RS_CBU_DRAWVECTORCOL(spine0TM.d, spine0TM.c, rage::Vector3(0,0,1));
          //NM_RS_CBU_DRAWVECTORCOL(spine0TM.d, fallDirProjection, rage::Vector3(1,1,0));
          rage::Matrix34 spine3TM;
          m_parent->getSpine()->getSpine0Part()->getBoundMatrix(&spine3TM);
          float yOffset = 0.7f;
          if(m_isLeftArm)
            yOffset = -0.7f;
          armIK(ikInputData, spine3TM.d+yOffset*spine3TM.b+0.7f*spine3TM.a, 0.0f, 0.5f, &vel);//cowboy
        }
        else
        {
          armIK(ikInputData, target, armTwist, 0.5f, &vel); // rotational velocity compensation only//cowboy
        }
      }
      else//((m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl) && !m_parent->m_handsAndKnees) 
#endif //#ifdef NM_COWBOY
      {
#if ART_ENABLE_BSPY && CF_LOG_COLLISSIONS
        if (hasHit)
        {
          if(m_isLeftArm) {
            bspyLogf(info, L"left arm IK: collided[ ] probe[X]");
          } else {
            bspyLogf(info, L"right arm IK: collided[ ] probe[X]");
          }
        }
        else
        {
        if(m_isLeftArm) {
          bspyLogf(info, L"left arm IK: collided[ ] probe[ ]");
        } else {
          bspyLogf(info, L"right arm IK: collided[ ] probe[ ]");
        }
        }
#endif
        armIK(ikInputData, target, armTwist, 0.5f, &vel); // rotational velocity compensation only
      }
    }

    if (m_parent->m_forwardsAmount < -0.3f)
      ikInputData->setMaximumElbowAngle(m_maxElbowAngleRandom);
      //inputData->getElbow()->setDesiredAngle(rage::Min(m_armSetup->getElbow()->getDesiredAngle(), m_maxElbowAngleRandom));


    // danger... will not work as expected due to getDesiredTwist. expects ik output to already have happened.
    // can we clarify the intention of this twist change?
    if (hasHit && m_parent->m_handsAndKnees && m_armSetup->getShoulder()->getDesiredTwist() > 0.6f) 
    {
      inputData->getShoulder()->setDesiredTwist(0.5f*(m_armSetup->getShoulder()->getDesiredTwist()+ 0.6f));
    }

    // danger... will not work as expected due to getDesired*. expects ik output to already have happened.
    // can we clarify the intention of this twist change?
    if (m_parent->m_forwardsAmount<0.f || !m_character->hasCollidedWithWorld(m_armMask))
    {
      if (rage::Abs(spineTwistTask->getTwist()) < 2.0f) // this moves clavicles with shoulders if falling backwards, if falling sideways, they match the back twist
      {
        ikInputData->setMatchClavicle(kMatchClavicleUsingTwist);
        inputData->getClavicle()->setDesiredLean1(0.0f);
        inputData->getClavicle()->setDesiredLean2(0.0f);
      }
      else
      {
        ikInputData->setMatchClavicle(kDontMatchClavicle);
        inputData->getClavicle()->setDesiredTwist(0.3f);
        inputData->getClavicle()->setDesiredLean1(m_onBackRandomL1);
        inputData->getClavicle()->setDesiredLean2(m_onBackRandomL2);
      }
    }
    else
    {
      inputData->getClavicle()->setDesiredLean1(0.0f);
      inputData->getClavicle()->setDesiredLean2(0.0f);
      inputData->getClavicle()->setDesiredTwist(m_armSetup->getClavicle()->getActualTwist());
      NmRsSpineInputWrapper* spineData = m_parent->getSpineInputData();
      spineData->getSpine3()->setDesiredLean1(-1);
      spineData->getSpine2()->setDesiredLean1(-1);
#if NMHandAndKneesAngryCat
      if(m_handsAndKnees)
      {
        spineData->getSpine3()->setDesiredLean1(1);
        spineData->getSpine2()->setDesiredLean1(1);
      }
#endif
    }

    if (m_parent->m_antiPropClav && m_parent->m_forwardsAmount<0.f)
    {
      ikInputData->setMatchClavicle(kDontMatchClavicle);
      inputData->getClavicle()->setDesiredTwist(0.0f);
      inputData->getClavicle()->setDesiredLean1(0.0f);
      inputData->getClavicle()->setDesiredLean2(0.0f);
    }

    // Wrist orientation.
    rage::Vector3 wristNormal;
    getDesiredWristNormal(groundNormal, probeEndHit, hasHit, wristNormal);
    // Step wrist IK solver.
    wristIK(ikInputData, target, wristNormal);

    arm->postInput(ikInput);

#if ART_ENABLE_BSPY
  if (m_isLeftArm)
    m_parent->m_reachTargetL = target;
  else
    m_parent->m_reachTargetR = target;
#endif // ART_ENABLE_BSPY

  }

#define NM_NEW_WRISTS 1
#if NM_NEW_WRISTS
  // Function calculates wristNormal for wristIK.
  void NmRsCBUCatchFall::ArmState::getDesiredWristNormal(const rage::Vector3 &groundNormal, const rage::Vector3 &probeEndHit, bool hasProbeHit, rage::Vector3 &wristNormal) const
#else
  // Function calculates wristNormal for wristIK.
  void NmRsCBUCatchFall::ArmState::getDesiredWristNormal(const rage::Vector3 &groundNormal, const rage::Vector3 &/*probeEndHit*/, bool hasProbeHit, rage::Vector3 &wristNormal) const
#endif//NM_NEW_WRISTS
  {
    rage::Matrix34 lowerArm;
    m_armSetup->getLowerArm()->getMatrix(lowerArm);

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "Wrists", m_isLeftArm);
#endif // ART_ENABLE_BSPY
#if NM_NEW_WRISTS
    //1) Default alignment of wrists to be Frankenstein's monster.
    //2) If the wrist is in contact or about to contact floor then align wrist to floor.
    //TODO (although it seems to work already): blend the orientation? Is there too much switching?
    //This stops the swan/crane pose.
    //This doesn't need to know that the character is falling over a railing

    //Default alignment of wrists to be Frankenstein's monster.
    //  this is closer to the mean wrist orientation of catchfall meaning wrists (more often) orientate quicker to floor normal once requested
    wristNormal = -lowerArm.c;

    // Ray-probe hit
    if (hasProbeHit)
    {
      rage::Vector3 handVel = m_armSetup->getHand()->getLinearVelocity();
      rage::Vector3 hand2Hit = probeEndHit;
      hand2Hit -= m_armSetup->getHand()->getPosition();
      float dist2Hit = hand2Hit.Mag();
      hand2Hit.Normalize();
      float vel2Hit = hand2Hit.Dot(handVel);
      float handLength = 0.2f;
      float time2Impact = 999.0f;
      if (vel2Hit > 0.0001f)
        time2Impact = dist2Hit/vel2Hit;
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "Wrists", dist2Hit);
      bspyScratchpad(m_character->getBSpyID(), "Wrists", time2Impact);
#endif // ART_ENABLE_BSPY
      //TODO: also check hand not in contact with probe inst
      bool handOrWristCollided = m_armSetup->getHand()->collidedWithEnvironment();
      //Is the contact of the lower arm close to the wrist?
      if (!handOrWristCollided && m_armSetup->getLowerArm()->collidedWithEnvironment())
      {
        rage::Vector3 pos, collisionNormal;
        float depth = 0;
        m_armSetup->getLowerArm()->getCollisionZMPWithEnvironment(pos, collisionNormal,&depth);
        pos -= m_armSetup->getWrist()->getJointPosition();
        if (pos.Mag() < 0.05)
          handOrWristCollided = true;
      }
      if ( handOrWristCollided || 
        (vel2Hit > 0.0001f && (time2Impact< 0.5f || dist2Hit < handLength)))
        wristNormal = groundNormal;
    }
#else
    //1) Default wrists aligned with forearms.
    //2) If the character has hit the floor (is in contact with environment) or the probe has hit then align wrist to floor.
    
    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_parent->m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);

    //when falling or slumped over a railing don't match wrists to the groundNormal unless the probes have hit
    bool railing = (balColReactTask->isActive() && balColReactTask->m_obstacleType == NmRsCBUBalancerCollisionsReaction::BCR_Railing);

    // TODO: We may need to blend to ground normal when using handsAndKnees.
    if (hasProbeHit || (m_character->hasCollidedWithEnvironment() && !railing)) // Ray-probe hit or Agent collided with environment.
    {
      // Ray-probe hit i.e. Agent is low and there is no time for blend.
      if (hasProbeHit)
      {
        wristNormal = groundNormal;
      }
        else // Agent collided with the environment but probes did not fire i.e. Agent possibly landed vertically, feet first. Blend in this case.
      {
        // Lerp from lower arm frame x-axis to the ground normal depending on how upright Agent is.
        const rage::Vector3 lerpTo = (m_armSetup->getType() == kLeftArm) ? (-lowerArm.a) : (lowerArm.a);

        const float w = rage::Clamp(m_parent->m_upwardsness - 0.25f, 0.0f, 1.0f);
        wristNormal.Lerp(w, groundNormal, lerpTo);
        wristNormal.NormalizeSafe(groundNormal);
      }//if (hasProbeHit)
    }
    else // Agent hasn't collided with the environment and neither probe fired.
      {
      // Keep wrists aligned with forearms.
      wristNormal = (m_armSetup->getType() == kLeftArm) ? (-lowerArm.a) : (lowerArm.a);
    }
#endif //NM_NEW_WRISTS


#if ART_ENABLE_BSPY
    if (m_isLeftArm)
    {
      m_parent->m_groundNormalL = groundNormal;
      m_parent->m_wristNormalL = wristNormal;
    }
    else
    {
      m_parent->m_groundNormalR = groundNormal;
      m_parent->m_wristNormalR = wristNormal;
    }
#endif // ART_ENABLE_BSPY
      }

  // Function calculates m_reachLength value that is used in IK to clamp to maximum reaching distance.
  void NmRsCBUCatchFall::updateReachLength(bool hasProbeHit)
  {
    if (hasProbeHit || m_character->hasCollidedWithEnvironment()) // Ray-probe hit or Agent collided with environment.
      {
      // Reach length blend.
      const float w = rage::Clamp(m_upwardsness - 0.5f, 0.0f, 1.0f);
      m_reachLength = (m_armLength * m_reachLengthMultiplier * 0.7f) +
                      (m_armLength * m_reachLengthMultiplier * 0.3f * (1.0f - w));
      }
    else // Agent hasn't collided with the environment and neither probe fired.
    {
      // Keep arms slightly bended.
      m_reachLength = (m_armLength * m_reachLengthMultiplier) * 0.9f;
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUCatchFall::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_legsStiffness, true);
    bspyTaskVar(m_parameters.m_torsoStiffness, true);
    bspyTaskVar(m_parameters.m_armsStiffness, true);
    bspyTaskVar(m_parameters.m_forwardMaxArmOffset, true);
    bspyTaskVar(m_parameters.m_backwardsMinArmOffset, true);
    bspyTaskVar(m_parameters.m_zAxisSpinReduction, true);
    bspyTaskVar(m_parameters.extraSit, true);
    bspyTaskVar(m_callRDS, true);
    bspyTaskVar(m_comVelRDSThresh, true);  
    bspyTaskVar(m_resistRolling, true);
    bspyTaskVar(m_armReduceSpeed, true);
    bspyTaskVar(m_reachLengthMultiplier, true);
    bspyTaskVar(m_inhibitRollingTime, true);
    bspyTaskVar(m_changeFrictionTime, true);
    bspyTaskVar(m_groundFriction, true);
    bspyTaskVar(m_stopOnSlopes, true);
    bspyTaskVar(m_stopManual, true);
    bspyTaskVar(m_stoppedStrengthDecay, true);
    bspyTaskVar(m_spineLean1Offset, true);
    bspyTaskVar(m_riflePose, true);
    bspyTaskVar(m_antiPropClav, true);
    bspyTaskVar(m_antiPropWeak, true);
    bspyTaskVar(m_headAsWeakAsArms, true);    
    bspyTaskVar(m_successStrength, true);    

    bspyTaskVar(m_parameters.m_useHeadLook, true);
    bspyTaskVar_Bitfield32(m_parameters.m_effectorMask, true);

    bspyTaskVar_Bitfield32(m_effectorMask, false);

    bspyTaskVar(m_fallDirection, false);
    bspyTaskVar(m_probeLength, false);
    bspyTaskVar(m_reachLength, false);
    bspyTaskVar(m_armLength, false);
    bspyTaskVar(m_forwardsAmount, false);
    bspyTaskVar(m_bodyStrength, false);
    bspyTaskVar(m_predictionTime, false);
    bspyTaskVar(m_upwardsness, false);
    bspyTaskVar(m_kneeBendL, false);
    bspyTaskVar(m_kneeBendR, false);
    bspyTaskVar(m_randomSpineL2, false);

    bspyTaskVar(m_headAvoidActive, false);
    bspyTaskVar(m_onGround, false);
    bspyTaskVar(m_rdsActivatedByCatchFall, false);

    bspyTaskVar(m_inhibitRollingTimer, false);
    bspyTaskVar(m_changeFrictionTimer, false);
    
    bspyTaskVar(m_leftArmState.m_liftArm, false);
    bspyTaskVar(m_rightArmState.m_liftArm, false);

    bspyTaskVar(m_leftArmState.m_timeToImpact, false);
    bspyTaskVar(m_rightArmState.m_timeToImpact, false);

    bspyTaskVar(m_leftArmState.m_armStrength, false);
    bspyTaskVar(m_leftArmState.m_onBackRandomL1, false);
    bspyTaskVar(m_leftArmState.m_onBackRandomL2, false);
    bspyTaskVar(m_leftArmState.m_maxElbowAngleRandom, false);
    bspyTaskVar(m_leftArmState.m_floorVel, false);
    bspyTaskVar(m_leftArmState.m_isLeftArm, false);
    bspyTaskVar_Bitfield32(m_leftArmState.m_armMask, false);

    bspyTaskVar(m_rightArmState.m_armStrength, false);
    bspyTaskVar(m_rightArmState.m_onBackRandomL1, false);
    bspyTaskVar(m_rightArmState.m_onBackRandomL2, false);
    bspyTaskVar(m_rightArmState.m_maxElbowAngleRandom, false);
    bspyTaskVar(m_rightArmState.m_floorVel, false);
    bspyTaskVar(m_rightArmState.m_isLeftArm, false);
    bspyTaskVar_Bitfield32(m_rightArmState.m_armMask, false);

    //Hands and Knees specific
    bspyTaskVar(m_handsAndKnees, true);
    bspyTaskVar(m_floorTime, false);
    //Shot Fall2Knees
    bspyTaskVar(m_fall2Knees, true);     
    bspyTaskVar(m_ftk_armsOut, true);
    bspyTaskVar(m_ftk_armsIn, true);

    bspyTaskVar (m_groundNormalL, false);
    bspyTaskVar (m_wristNormalL, false);
    bspyTaskVar (m_reachTargetL, false);
    bspyTaskVar (m_groundNormalR, false);
    bspyTaskVar (m_wristNormalR, false);
    bspyTaskVar (m_reachTargetR, false);

    bspyTaskVar (m_groundFrictionMin, false);

    bspyTaskVar(m_slope, false);
    bspyTaskVar(m_slopeAlignment, false);
    bspyTaskVar(m_faceUpness, false);
  }
#endif // ART_ENABLE_BSPY
}
