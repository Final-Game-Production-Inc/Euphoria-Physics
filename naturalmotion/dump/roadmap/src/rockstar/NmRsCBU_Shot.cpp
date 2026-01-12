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
 * <DESCRIPTION OF BEHAVIOUR>
 *
 */


#include "NmRsInclude.h"
#include "NmRsCBU_Shot.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_ArmsWindmill.h"
#include "NmRsCBU_ArmsWindmillAdaptive.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_CatchFall.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_SpineTwist.h"
#include "NmRsCBU_BodyWrithe.h"


namespace ART
{
  NmRsCBUShot::NmRsCBUShot(ART::MemoryManager* services) : CBUTaskBase(services, bvid_shot),
    m_reachArm(&m_reachLeft)
  {
    initialiseCustomVariables();
  }

  NmRsCBUShot::~NmRsCBUShot()
  {
  }

  // also called at deactivate, so right place for everything that needs resetting
  void NmRsCBUShot::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_newHit = false;

    m_torqueSpin = 0.0f;
    m_torqueSpinTime = -1.0f;

    m_parameters.initialWeaknessRampDuration = 0.4f;
    m_parameters.initialWeaknessZeroDuration = 0.0f;
    m_parameters.initialNeckDuration = 0.0f; 
    m_parameters.initialNeckRampDuration = 0.0f;    
    m_parameters.useCStrModulation = false;
    m_parameters.cStrUpperMin = 0.1f;
    m_parameters.cStrUpperMax = 1.0f;
    m_parameters.cStrLowerMin = 0.1f;
    m_parameters.cStrLowerMax = 1.0f;

    //injuredArm parameters not part of behaviour message (hence not automatically being reset)
    // jrp looks like most ot the injured arm structure is used to pass around
    // these constants. is this somehow better than setting these up in the
    // injured arm substates?
    m_injuredArm.hipYaw = 1.5f;
    m_injuredArm.hipRoll = 0.f;
    m_injuredArm.shrugTime = 0.4f;
    m_injuredArm.forceStepExtraHeight = 0.07f;
    m_injuredArm.forceStep = true;
    m_injuredArm.stepTurn = true;
    m_injuredArm.velMultiplierStart = 1.f;
    m_injuredArm.velMultiplierEnd = 5.f;
    m_injuredArm.velForceStep = 0.8f;
    m_injuredArm.velStepTurn = 0.8f;     
    m_injuredArm.velScales = true;
    m_injuredArm.injuredArmTime = 0.25f;

    m_disableBalance = false;

    // Recently stripped out shot-reaction message parameters: as is currently this wouldn't be needed, 
    // because all shot params are still either being send explicitly or evaluate to default.
    // But once the broken out reaction type messages are called individually by the game, they need to be
    // reset, since otherwise they will still have the most recent value by the next time the behaviour is activated.
    m_parameters.addShockSpin = false;
    m_parameters.fallToKnees = false;
    m_parameters.shotFromBehind = false;
    m_parameters.shotInGuts = false;
    m_parameters.useHeadLook = false;
  }

  void NmRsCBUShot::onActivate()
  {
    Assert(m_character);
    m_snapDirection = 1.f;

    // all substates disabled at start
    m_crouchShotEnabled = false;
    m_injuredLeftArmEnabled = false;
    m_injuredRightArmEnabled = false;
    m_defaultArmMotionEnabled = false;
    m_reachLeftEnabled = false;
    m_reachRightEnabled = false;
    m_controlStiffnessEnabled = false;
    m_injuredLeftLegEnabled = false;
    m_injuredRightLegEnabled = false;
    m_headLookEnabled = false;
    m_justWhenFallenEnabled = false;
    m_chickenArmsEnabled = false;
    m_flingEnabled = false;
    m_meleeEnabled = false;
    m_fallToKneesEnabled = false;
    m_shotFromBehindEnabled = false;
    m_shotInGutsEnabled = false;
    m_hitFromBehind = false;
    m_onGroundEnabled = false;
    m_pointGunEnabled = false;

    //initialize subState variables
    //FallToKnees
    m_fTK.m_bendLegs = false;
    m_fTK.m_LkneeHasHit = false;
    m_fTK.m_RkneeHasHit = false;
    m_fTK.m_hitFloor = false;
    m_fTK.m_ftkTimer = 0.f;
    m_fTK.m_ftkLoosenessTimer = 0.f;
    m_fTK.m_LkneeHitLooseness = false;
    m_fTK.m_RkneeHitLooseness = false;
    m_fTK.m_fallingBack = false;
    m_fTK.m_squatting = false;

    NM_RS_DBG_LOGF(L"shot entry");
    m_reactionTime = 3.6f;
    if (m_parameters.fling)
      m_reactionTime /= 2.f;

    m_falling = false;
    m_newReachL = false;
    m_newReachR = false;
    m_injuredLArm = false;
    m_injuredRArm = false;
    m_injuredLLeg = false;
    m_injuredRLeg = false;
    m_injuredLegMask = bvmask_None;
    m_hitTime = 0.0f;
    m_hitTimeLeft = 0.0f;
    m_hitTimeRight = 0.0f;
    m_controlStiffnessTime = 0.0f;
    m_controlNeckStiffnessTime = 0.0f;
    m_shrug = false;
    m_spineLean1 = 0.0f;
    m_relaxTimeUpper = 0.0f;
    m_relaxPeriodUpper = 0.0f;
    m_relaxTimeLower = 0.0f;
    m_relaxPeriodLower = 0.0f;
    m_injuredArmElbowBend = 1.0f;
    m_defaultArmMotion.rightBrace = false;
    m_defaultArmMotion.leftBrace = false;
    m_defaultArmMotion.releaseLeftWound = false;
    m_defaultArmMotion.releaseRightWound = false;     
    m_defaultArmMotion.armsWindmillTimeLeft = m_character->getRandom().GetRanged(0.f,1.f);;
    m_defaultArmMotion.armsWindmillTimeRight = m_character->getRandom().GetRanged(0.f,1.f);;
    m_injuredLeftLeg.legLiftTimer = -0.1f;
    m_injuredLeftLeg.legInjuryTimer = -0.1f;
    m_injuredLeftLeg.forceStepTimer = -0.1f;  
    m_injuredRightLeg.legLiftTimer = -0.1f;
    m_injuredRightLeg.legInjuryTimer = -0.1f;
    m_injuredRightLeg.forceStepTimer = -0.1f;

    m_woundLPart = -1;
    m_woundRPart = -1;
    m_archBack = false;
    m_headLookAtWound = false;
    m_velForwards = 0;
    m_hitPointRight = 0;
    m_hitNormalLocal.Set(0,0,-1);
    m_hitPointLocal.Set(0,0,0);
    m_time = 0;
    m_woundLOffset.Set(0,0,0);
    m_woundROffset.Set(0,0,0);
    m_woundLNormal.Set(0,0,0);
    m_woundRNormal.Set(0,0,0);

    m_feedbackSent_FinishedLookingAtWound = false;

    // setup character's effector strengths
    m_upperBodyStiffness = 1.f;
    m_lowerBodyStiffness = 1.f;

    m_hipYaw = 0.f;
    m_hipRoll = 0.f;
    m_twistMultiplier = 1.f;

    m_exagLean1 = 0.f;
    m_exagLean2 = 0.f;
    m_exagTwist = 0.f;

    m_cleverIKStrengthMultLeft = 1.0f;
    m_cleverIKStrengthMultRight = 1.0f;

    // initial value for STIFFNESS ramp (not muscle strength). if stiffness is higher, 
    // impulses will travel further though body, destabilizing the balancer more.
    m_controlStiffnessStrengthScale = 0.01f;
    m_controlNeckStiffnessScale = 0.01f;

    m_body->resetEffectors(kResetCalibrations | kResetAngles);

    m_body->setOpposeGravity(1.0f);

    m_defaultBodyStiffness = 11.f;
    m_spineStiffness = m_parameters.bodyStiffness;
    m_spineDamping = m_parameters.spineDamping;//1.f;
    m_armsStiffness = m_parameters.bodyStiffness * 10.f/m_defaultBodyStiffness;
    m_armsDamping = 1.f;
    m_wristStiffness = 17.f;//m_parameters.bodyStiffness * 20.f/m_defaultBodyStiffness;//mmmmhere this is too high if bodyStiffness is above 11
    m_wristDamping = 1.f;//rage::Max(m_character->getLastKnownUpdateStep(), 1.f/60.f) * 60.f * 0.5f;////mmmmhere this is too low for low timesteps
    m_neckStiffness = m_parameters.neckStiffness; // = 14.f;//m_parameters.bodyStiffness * 20.f/m_defaultBodyStiffness;
    m_neckDamping = m_parameters.neckDamping; //1.f;//rage::Max(m_character->getLastKnownUpdateStep(), 1.f/60.f) * 60.f * 0.5f;

    m_torqueSpin = 0.0f;
    m_torqueSpinTime = -1.0f;


    // configure balancer
    if (!m_disableBalance)
    {
      NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      dynamicBalancerTask->setCrouching(m_parameters.crouching);
      dynamicBalancerTask->activate();
      if (m_character->m_underwater && dynamicBalancerTask->isActive())
      {
        dynamicBalancerTask->setLegCollision(true);
      }


      // !!! TODO: knee strength gets overwritten in controlStiffness tick,
      // so the following will never actually take effect
      //if (m_parameters.crouching)
      //{
      //  //This has no effect as the dynBalancer's leg muscle parameters are overwitten by shotControlStiffness
      //  //However I've commented it out because if there is no newhit shotControlStiffness is not called
      //  //MMMMtodo should shotControlStiffness always be called while standing?
      //  //dynamicBalancerTask->setKneeStrength(0.5f);       
      //}

      if (dynamicBalancerTask->isActive())
      {
        NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
        Assert(balColReactTask);

        if (!balColReactTask->isActive() || balColReactTask->m_balancerState == bal_Normal) 
        {
          dynamicBalancerTask->setLeftLegStiffness(12.f);
          dynamicBalancerTask->setRightLegStiffness(12.f);
        }
        dynamicBalancerTask->setOpposeGravityAnkles(1.f);
        dynamicBalancerTask->setOpposeGravityLegs(1.f);
        if (dynamicBalancerTask->isActive())
        {
          dynamicBalancerTask->calibrateLowerBodyEffectors(m_body);
          dynamicBalancerTask->setLowerBodyGravityOpposition(m_body);
        }
        //Force step if nearing a transition in the gait cycle
        //This is to get a stepping while airborne motion
        //Force step choice is overuled if one leg in contact other not so don't have to be too conservative in identifying transition
        //Only do it for gaits that launch the character in the air - i.e forwards run/sprint
        bool movingForwardsFast = (m_character->m_COMTM.c.Dot(m_character->m_COMvel) < 0.f) &&
          (m_character->m_COMvelMag > 3.3f);
        if (movingForwardsFast && nmrsGetActualLean1(getLeftLeg()->getHip()) < -0.03f && nmrsGetActualLean1(getRightLeg()->getHip()) > 0.7f)
          dynamicBalancerTask->setForceStep(1,0.2f,true);//step with left leg
        if (movingForwardsFast && nmrsGetActualLean1(getLeftLeg()->getHip()) < -0.2f && nmrsGetActualLean1(getRightLeg()->getHip()) > 0.6f)
          dynamicBalancerTask->setForceStep(1,0.2f,true);//step with left leg
        if (movingForwardsFast && nmrsGetActualLean1(getRightLeg()->getHip()) < -0.03f && nmrsGetActualLean1(getLeftLeg()->getHip()) > 0.7f)
          dynamicBalancerTask->setForceStep(2,0.2f,true);//step with right leg
        if (movingForwardsFast && nmrsGetActualLean1(getRightLeg()->getHip()) < -0.2f && nmrsGetActualLean1(getLeftLeg()->getHip()) > 0.6f)
          dynamicBalancerTask->setForceStep(2,0.2f,true);//step with right leg
      }
    }

    //Scale incoming part angularVelocities - for shot from running legs have some spurious angVel 
    for (int i = 0; i<m_character->getNumberOfParts(); i++)
    {
      if(m_character->isPartInMask(m_parameters.angVelScaleMask, i))
      {
        NmRsGenericPart *part = m_character->getGenericPartByIndex(i);
        rage::Vector3 rotVel = part->getAngularVelocity();
        rotVel *= m_parameters.angVelScale;
        part->setAngularVelocity(rotVel);
      }
    }

    //Set constant armswindmill parameters so that armsWindmill message may be called with no start parameter after
    //  shot has been activated and when the armsWindmill is run by shot it will use those new parameters not the ones below
    NmRsCBUArmsWindmill* armsWindmillTask = (NmRsCBUArmsWindmill*)m_cbuParent->m_tasks[bvid_armsWindmill];
    Assert(armsWindmillTask);
    armsWindmillTask->updateBehaviourMessage(NULL); // initialise parameters.
    armsWindmillTask->m_parameters.m_adaptiveMode = 0;
    armsWindmillTask->m_parameters.m_forceSync = true;
    armsWindmillTask->m_parameters.m_disableOnImpact = true;
    armsWindmillTask->m_parameters.m_shoulderStiffness = 9.f;
    armsWindmillTask->m_parameters.m_elbowStiffness = 9.f;//mmmmtodo set a good default
    armsWindmillTask->m_parameters.m_shoulderDamping = 1.f;
    armsWindmillTask->m_parameters.m_elbowDamping = 1.f;
    armsWindmillTask->m_parameters.m_phaseOffset = 60.f;
    armsWindmillTask->m_parameters.m_mirrorMode = 1;
    armsWindmillTask->m_parameters.m_useLeft = true;
    armsWindmillTask->m_parameters.m_useRight = true;
    armsWindmillTask->m_parameters.m_IKtwist = 0;
    armsWindmillTask->m_parameters.m_dragReduction = 0.2f;

    int partID = getSpine()->getSpine3Part()->getPartIndex();
    armsWindmillTask->m_parameters.m_leftCircleDesc.partID = partID;
    armsWindmillTask->m_parameters.m_leftCircleDesc.centre.Set(0.0f, 0.5f, -0.3f);
    armsWindmillTask->m_parameters.m_leftCircleDesc.normal.Set(0.0f, 0.2f, 0.2f);
    armsWindmillTask->m_parameters.m_leftCircleDesc.speed = -0.5f;
    armsWindmillTask->m_parameters.m_leftCircleDesc.radius1 = 0.15f;
    armsWindmillTask->m_parameters.m_leftCircleDesc.radius2 = 0.2f;
    armsWindmillTask->m_parameters.m_rightCircleDesc.partID = partID;
    armsWindmillTask->m_parameters.m_rightCircleDesc.centre.Set(0.0f, 0.5f, -0.1f);
    armsWindmillTask->m_parameters.m_rightCircleDesc.normal.Set(0.0f, -0.2f, -0.2f);//unnecessary as using mirroring
    armsWindmillTask->m_parameters.m_rightCircleDesc.speed = -0.3f;
    armsWindmillTask->m_parameters.m_rightCircleDesc.radius1 = 0.3f;
    armsWindmillTask->m_parameters.m_rightCircleDesc.radius2 = 0.1f;


    NmRsCBUArmsWindmillAdaptive* armsWindmillAdaptiveTask = (NmRsCBUArmsWindmillAdaptive*)m_cbuParent->m_tasks[bvid_armsWindmillAdaptive];
    Assert(armsWindmillAdaptiveTask);
    armsWindmillAdaptiveTask->updateBehaviourMessage(NULL); // initialise parameters.
    armsWindmillAdaptiveTask->m_parameters.bendLeftElbow = true;
    armsWindmillAdaptiveTask->m_parameters.bendRightElbow = true;
    armsWindmillAdaptiveTask->m_parameters.leftElbowAngle = 0.1f;
    armsWindmillAdaptiveTask->m_parameters.rightElbowAngle = 0.1f;			
    armsWindmillAdaptiveTask->m_parameters.phase = 0.8f;//try -0.8, 0.8 led to right leg facing down and straight alot
    armsWindmillAdaptiveTask->m_parameters.armDirection = -1;
    armsWindmillAdaptiveTask->m_parameters.setBackAngles = false;
    armsWindmillAdaptiveTask->m_parameters.effectorMask = bvmask_UpperBody;
    armsWindmillAdaptiveTask->m_parameters.angSpeed = 4.49f;
    armsWindmillAdaptiveTask->m_parameters.amplitude = 0.8f;
    armsWindmillAdaptiveTask->m_parameters.armStiffness = 11.f;
    armsWindmillAdaptiveTask->m_parameters.lean1mult = 0.9f;
    armsWindmillAdaptiveTask->m_parameters.lean1offset = 1.3f;
    armsWindmillAdaptiveTask->m_parameters.elbowRate = 4.0f;
    armsWindmillAdaptiveTask->m_parameters.leftElbowAngle = 0.1f;
    armsWindmillAdaptiveTask->m_parameters.rightElbowAngle = 0.1f;
    armsWindmillAdaptiveTask->m_parameters.disableOnImpact = false;
#if NM_STAGGERSHOT
    m_character->m_spineStrengthScale = 1.f;
    m_character->m_spineDampingScale = 1.f;
    m_character->m_spineMuscleStiffness = 1.5f;
#endif

  }

  void NmRsCBUShot::onDeactivate()
  {
    Assert(m_character);

    // deactivate substates
    //      if (m_crouchShotEnabled) // no exit needed for these
    //        crouchShot_exit();
    if (m_injuredLeftArmEnabled)
      injuredArm_exit();
    if (m_injuredRightArmEnabled)
      injuredArm_exit();
    if (m_defaultArmMotionEnabled)
      defaultArmMotion_exit();
    if (m_reachLeftEnabled)
      reachLeft_exit();
    if (m_reachRightEnabled)
      reachRight_exit();
    if (m_controlStiffnessEnabled)
      controlStiffness_exit();
    if (m_injuredLeftLegEnabled)
      injuredLeftLeg_exit();
    if (m_injuredRightLegEnabled)
      injuredRightLeg_exit();
    if (m_headLookEnabled)
      headLook_exit();
    if (m_justWhenFallenEnabled)
      justWhenFallen_exit();
    //      if (m_chickenArmsEnabled)
    //        chickenArms_exit();
    if (m_flingEnabled)
      fling_exit();
    if (m_meleeEnabled)
      melee_exit();
    if (m_onGroundEnabled)
      onGround_exit();
    if (m_crouchShotEnabled)
      crouchShot_exit();
    if (m_shotFromBehindEnabled)
      shotFromBehind_exit();
    if (m_shotInGutsEnabled)
      shotInGuts_exit();
    if (m_fallToKneesEnabled)
      fallToKnees_exit();
    if (m_pointGunEnabled)
      pointGun_exit();

    m_character->m_footSlipMult = 1.f;

#if 0
    // this is awkward to support in the limbs model. move to a character
    // or a limb function...

    // fix for the exorcist bug. if we are trying to exit with
    // stiffness set to low, reset. the power of christ compels you!
    const float minMuscleStiffness = 1.0f;
    for(int i = 0; i < m_character->getNumberOfEffectors(); ++i)
    {
      NmRsEffectorBase* effector = m_character->getEffector(i);
      if(effector->getMuscleStiffness() < minMuscleStiffness)
        effector->setMuscleStiffness(minMuscleStiffness);
    }
#endif

    // reset variables and non-message params
    initialiseCustomVariables();

    NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];      
    dynamicBalancerTask->setKneeStrength(1.f);//injuredleg and crouch
    dynamicBalancerTask->setHipPitch(0.f);//injuredleg and crouch

    if (dynamicBalancerTask->isActive() && m_character->noBehavioursUsingDynBalance())
      dynamicBalancerTask->requestDeactivate();
#if NM_STAGGERSHOT
    m_character->m_spineStrengthScale = 1.f;
    m_character->m_spineDampingScale = 1.f;
    m_character->m_spineMuscleStiffness = 1.5f;
#endif

  }

  CBUTaskReturn NmRsCBUShot::onTick(float timeStep)
  {

    NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);
    if (balColReactTask->isActive() && balColReactTask->m_balancerState == bal_Rebound && balColReactTask->m_parameters.reboundMode == 0)
      m_parameters.fallToKnees = true;
    m_bcrArms = balColReactTask->m_timeAfterImpact > 0.0001f && balColReactTask->m_timeAfterImpact < balColReactTask->m_parameters.reactTime && dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK;

    //Kill off the character - will be replaced by game sending message
    if (m_character->m_underwater)
    {
      //HACKERY!!!
      if (m_time > m_parameters.deathTime && m_parameters.deathTime > 0.f)
      {
        deactivate();
        NmRsCBUBodyWrithe* writheTask = (NmRsCBUBodyWrithe*)m_cbuParent->m_tasks[bvid_bodyWrithe];
        Assert(writheTask);
        if (writheTask->isActive())
          writheTask->deactivate();
        float muscleStiffness = 0.2f;
        m_body->setStiffness(3.f, 1.0f, bvmask_LegLeft | bvmask_LegRight, &muscleStiffness);
        muscleStiffness = 0.3f;
        m_body->setStiffness(4.f, 1.5f, bvmask_Spine | bvmask_ArmLeft | bvmask_ArmRight, &muscleStiffness);
        muscleStiffness = 4.f;
        getLeftArmInputData()->getWrist()->setStiffness(7.f, 1.5f, &muscleStiffness);
        getRightArmInputData()->getWrist()->setStiffness(7.f, 1.5f, &muscleStiffness);
        getLeftLegInputData()->getAnkle()->setStiffness(7.f, 1.5f, &muscleStiffness);
        getRightLegInputData()->getAnkle()->setStiffness(7.f, 1.5f, &muscleStiffness);

        return eCBUTaskComplete;
      }
    }

    float unSnapInterval = m_parameters.unSnapInterval;
    if (m_parameters.snap && m_hitTime>unSnapInterval && m_snapDirection < 0.0f && m_parameters.unSnapRatio>=0.f)
    {
      float unSnapRatio = m_parameters.unSnapRatio;
      rage::Vector3* snapDirection = NULL;
      if ( m_parameters.snapUseBulletDir)
        snapDirection = &m_parameters.bulletVel;
      int bodyPart = -1;//Don't apply snap just to bodyPart
      if ( m_parameters.snapHitPart)
        bodyPart = m_parameters.bodyPart;
      m_character->snap(m_parameters.snapMag*m_snapDirection*unSnapRatio,
        m_parameters.snapDirectionRandomness, 
        m_parameters.snapHipType,
        m_parameters.snapLeftArm,
        m_parameters.snapRightArm,
        m_parameters.snapLeftLeg,  
        m_parameters.snapRightLeg,  
        m_parameters.snapSpine,  
        m_parameters.snapNeck, 
        m_parameters.snapPhasedLegs, 
        m_parameters.snapUseTorques,
        1.f,
        bodyPart,
        snapDirection,
        m_parameters.snapMovingMult,
        m_parameters.snapBalancingMult,
        m_parameters.snapAirborneMult,
        m_parameters.snapMovingThresh);
      m_snapDirection *= -1.f;
    }       

    // shot relax: sets upper and lower body strength(!), not stiffness
    //------------------------------------------------------------------------------------------------------
    m_relaxTimeUpper = rage::Min(m_relaxTimeUpper + timeStep, m_relaxPeriodUpper);
    m_upperBodyStiffness = 1.f - (m_relaxTimeUpper / (m_relaxPeriodUpper + NM_RS_FLOATEPS));
    m_relaxTimeLower = rage::Min(m_relaxTimeLower + timeStep, m_relaxPeriodLower);
    m_lowerBodyStiffness = 1.f - (m_relaxTimeLower / (m_relaxPeriodLower + NM_RS_FLOATEPS));

    // apply characterStrength (implies upper/lowerBodyStiffness reset each step, as in lines above)
    //------------------------------------------------------------------------------------------------------
    if(m_parameters.useCStrModulation)
    {
      m_upperBodyStiffness *= m_parameters.cStrUpperMin + (m_parameters.cStrUpperMax - m_parameters.cStrUpperMin)*m_character->m_strength;
      m_lowerBodyStiffness *= m_parameters.cStrLowerMin + (m_parameters.cStrLowerMax - m_parameters.cStrLowerMin)*m_character->m_strength;
    }

    // !!! TODO: ??
    m_wristDamping = 1.f;//timeStep * 60.f * 0.5f;//mmmmhere this is too low for low timesteps
    m_neckDamping = 1.f;//timeStep * 60.f * 0.5f;

    m_time += timeStep;
    m_hitTime += timeStep;
    m_hitTimeLeft += timeStep;
    m_hitTimeRight += timeStep;
    m_controlStiffnessTime += timeStep;
    m_controlNeckStiffnessTime += timeStep;

    // feedback: check if character is falling
    //------------------------------------------------------------------------------------------------------
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    Assert(feedback); // should exist
    if (m_relaxTimeUpper == m_relaxPeriodUpper && m_relaxPeriodUpper != 0.f && !m_parameters.useExtendedCatchFall)
    {
      NM_RS_DBG_LOGF(L"----------------------> falling : relax");
      feedback->m_agentID = m_character->getID();
      feedback->m_argsCount = 0;
      strcpy(feedback->m_behaviourName, NMShotFeedbackName);
      feedback->onBehaviourSuccess();
      m_falling = true;
      //Reset the muscleStiffness changed by controlStiffness now that controlStiffness no longer has to exit if falling
      //This is so looseness for newBullet applies it to what the behaviours set the effectors (and a lot don't set muscleStiffness)
      m_body->resetEffectors(kResetMuscleStiffness);

      if (dynamicBalancerTask->isActive())//mmmmtodo assume that because it is relaxed it is dead therefore no handsKneesCatchFall?
        dynamicBalancerTask->forceFail();
    }

    if (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK /*&& !m_fallToKneesEnabled*/) 
    {
      NM_RS_DBG_LOGF(L"----------------------> falling : hasFailed");
      m_falling = true;
      //Reset the muscleStiffness changed by controlStiffness now that controlStiffness no longer has to exit if falling
      //This is so looseness for newBullet applies it to what the behaviours set the effectors (and a lot don't set muscleStiffness)
      m_body->resetEffectors(kResetMuscleStiffness);
    }
    NM_RS_DBG_LOGF(L"falling: %d", m_falling);

    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    //balancer should be left on if hands and knees is set.
    if (((m_falling && !catchFallTask->m_handsAndKnees) || m_disableBalance) && (dynamicBalancerTask->isActive()))
      dynamicBalancerTask->forceFail();

    ////Exaggeration/Reflex
    if ((m_hitTime < m_parameters.exagDuration + m_parameters.exagSmooth2Zero + m_parameters.exagZeroTime) && !m_parameters.melee)
    {
#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour("-Exag"); 
#endif
      float startTime = 0.f;
      float startTimeRampDown = startTime + m_parameters.exagDuration;
      float zeroTime = startTimeRampDown + m_parameters.exagSmooth2Zero;
      float painResponse = 0.f; 
      if ((m_hitTime >= startTime) && (m_hitTime < startTimeRampDown))
        painResponse = 1.f;
      if ((m_hitTime >= startTimeRampDown) && (m_hitTime < zeroTime))
        painResponse = (zeroTime - m_hitTime)/m_parameters.exagSmooth2Zero;//mmmmtodo possible div by zero
      if (m_hitTime >= zeroTime)
        painResponse = 0.f;

      NM_RS_DBG_LOGF(L"Exaggeration %f", painResponse);

      getSpineInputData()->setBackAngles(painResponse*m_parameters.exagMag*m_exagLean1, 
        painResponse*m_parameters.exagMag*m_exagLean2, 
        painResponse*m_parameters.exagTwistMag*m_exagTwist);

#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif
    }
    //Conscious Pain Response - Spine  
    //lean1 forward 
    //lean2 towards bullet impact side 
    //twist towards bullet impact side 
    //------------------------------------------------------------------------------------------------------
    else if (!m_parameters.melee)
    {
#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour("-CPain"); 
#endif
      float startTimeRampUp = m_parameters.exagDuration + m_parameters.exagSmooth2Zero + m_parameters.exagZeroTime;
      float startTime = startTimeRampUp + m_parameters.cpainSmooth2Time;
      float startTimeRampDown = startTime + m_parameters.cpainDuration;
      float endTime = startTimeRampDown + m_parameters.cpainSmooth2Zero;
      float painResponse = 0.f; 
      if ((m_hitTime >= startTimeRampUp) && (m_hitTime < startTime))
        painResponse = (m_hitTime - startTimeRampUp)/m_parameters.cpainSmooth2Time;//mmmmtodo possible div by zero
      if ((m_hitTime >= startTime) && (m_hitTime < startTimeRampDown))
        painResponse = 1.f;
      if ((m_hitTime >= startTimeRampDown) && (m_hitTime < endTime))
        painResponse = (endTime - m_hitTime)/m_parameters.cpainSmooth2Zero;//mmmmtodo possible div by zero

      NM_RS_DBG_LOGF(L"painResponse %f", painResponse);

      if (painResponse > 0.f)
      {
        int part = m_parameters.bodyPart;
        bool hitLowLeg = part == getLeftLeg()->getThigh()->getPartIndex() ||
          part == getRightLeg()->getThigh()->getPartIndex() ||
          part == getLeftLeg()->getShin()->getPartIndex() ||
          part == getRightLeg()->getShin()->getPartIndex();

        //! Check the range of the parameters (m_hitPointRight is checked in newHit())!
        Assert(m_parameters.cpainTwistMag >= 0.0f && m_parameters.cpainTwistMag <= 2.0f);
        float twistL2 = m_hitPointRight * -10.f * painResponse * m_parameters.cpainTwistMag;
        NM_RS_DBG_LOGF(L"twistL2 %f", twistL2);

        //! Check for NAN!
        Assert(twistL2 == twistL2);

        painResponse *= m_parameters.cpainMag;
        if (m_velForwards > 0.f)
          painResponse *= 0.75f;

        if (hitLowLeg)
          painResponse *= 1.25f;

        getSpineInputData()->setBackAngles(painResponse, m_twistMultiplier*twistL2, m_twistMultiplier*twistL2);
      }
      else if (m_hitTime < endTime + 2.f*timeStep)//switch off Exaggeration/Pain Once
        getSpineInputData()->setBackAngles(0.f, 0.f, 0.f);

#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif
    } 


    // retrieve hit point in world space
    rage::Matrix34 mat;
    m_character->getGenericPartByIndex(m_parameters.bodyPart)->getMatrix(mat);
    mat.Transform(m_hitPointLocal, m_hitPointWorld);//m_hitPointWorld used by headlook and Fling

    // shockSpin: adds an extra 'shock' of torque/lift to the spine to exaggerate impacts 
    //mmmmmtodo
    //Move functionality into bullet
    //Lift should be in up direction
    //  Should be scaled with character weight: liftForce *= m_setup.liftGain*m_character->getTotalMass()*9.81f
    //------------------------------------------------------------------------------------------------------
    if (m_parameters.addShockSpin)
    {
      int part = m_parameters.bodyPart;
      // only react if shot in trunk, or if always-on is set
      bool doShockSpin = m_parameters.alwaysAddShockSpin ||
        (part == getSpine()->getSpine0Part()->getPartIndex() ||
        part == getSpine()->getSpine1Part()->getPartIndex() ||
        part == getSpine()->getSpine2Part()->getPartIndex() ||
        part == getSpine()->getSpine3Part()->getPartIndex() ||
        part == getLeftArm()->getClaviclePart()->getPartIndex() ||
        part == getRightArm()->getClaviclePart()->getPartIndex());

      if (doShockSpin)
      {
        m_character->m_footSlipMult = 1.f;
        rage::Vector3 torqueVector;
        //Turn-off shockspin at a certain twist angvel
        if (m_torqueSpinTime >= 0.0f)
        {
          torqueVector = getSpine()->getSpine3()->getJointPosition() - getSpine()->getSpine0()->getJointPosition();
          torqueVector.Normalize();

          if (m_parameters.shockSpinMaxTwistVel > 0.f)
          {
            torqueVector.Normalize();
            float twistVelMag = rage::Abs(torqueVector.Dot(m_character->m_COMrotvel));
            if ((twistVelMag > m_parameters.shockSpinMaxTwistVel) && m_torqueSpin < 0.f)
              m_torqueSpinTime = -1.0f;
            if ((twistVelMag > m_parameters.shockSpinMaxTwistVel) && m_torqueSpin > 0.f)
              m_torqueSpinTime = -1.0f;
          }
#if ART_ENABLE_BSPY && 0
          bspyScratchpad(m_character->getBSpyID(), "shockSpin", twistVelMag);
          bspyScratchpad(m_character->getBSpyID(), "shockSpin", m_torqueSpin);
#endif
        }

        if (m_torqueSpinTime >= 0.0f)
        {
          float torqueSpinMult = 1.f;
          bool leftFootOffGround = !getLeftLeg()->getFoot()->collidedWithNotOwnCharacter();
          bool rightFootOffGround = !getRightLeg()->getFoot()->collidedWithNotOwnCharacter();
          if (leftFootOffGround && rightFootOffGround)
            torqueSpinMult = m_parameters.shockSpinAirMult;
          else if (leftFootOffGround || rightFootOffGround)
            torqueSpinMult = m_parameters.shockSpin1FootMult;

          m_character->m_footSlipMult = m_parameters.shockSpinFootGripMult;
          torqueVector.Scale(m_torqueSpin * m_torqueSpinTime * torqueSpinMult);

          if (m_parameters.shockSpinLiftForceMult > 0.0f)
          {
            rage::Vector3 liftForce;
            liftForce.Abs(torqueVector);//mmmm this is a bad idea if the torque vector is not world up or down you get very unpredictable results
            liftForce *= m_parameters.shockSpinLiftForceMult;
            getSpine()->getSpine2Part()->applyForce(liftForce);
            liftForce.Scale(m_parameters.shockSpinScalePerComponent);
            getSpine()->getSpine3Part()->applyForce(liftForce);
            liftForce.Scale(m_parameters.shockSpinScalePerComponent);
            getSpine()->getSpine0Part()->applyForce(liftForce);

            // [jrp] hrm... does this mean that we torque non-trunk body parts?
            // [jrp] further, does this mean we might torque a trunk part twice?
            // [jrp] m_character->getGenericPartByIndex(m_parameters.bodyPart)->applyForce(liftForce);
          }              

          torqueVector.Scale(m_parameters.shockSpinScalePerComponent);
          getSpine()->getSpine1Part()->applyTorque(torqueVector);
          getSpine()->getSpine3Part()->applyTorque(torqueVector);

          torqueVector.Scale(m_parameters.shockSpinScalePerComponent);
          getSpine()->getSpine0Part()->applyTorque(torqueVector);

          // [jrp] hrm... does this mean that we torque non-trunk body parts?
          // [jrp] further, does this mean we might torque a trunk part twice?
          // [jrp] m_character->getGenericPartByIndex(m_parameters.bodyPart)->applyTorque(torqueVector);

          m_torqueSpinTime += (-1.0f - m_torqueSpinTime) * (timeStep * m_parameters.shockSpinDecayMult);//almost linear decay
        }//if (m_torqueSpinTime >= 0.0f)

      }//if (doShockSpin)
    }//if (m_parameters.addShockSpin)

    // tick substates
    //------------------------------------------------------------------------------------------------------
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-Crouch"); 
#endif
    substateTick(&NmRsCBUShot::crouchShot_entryCondition, NULL, &NmRsCBUShot::crouchShot_tick, &NmRsCBUShot::crouchShot_exitCondition, &NmRsCBUShot::crouchShot_exit, m_crouchShotEnabled, timeStep);
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-CStiff"); 
#endif
    substateTick(&NmRsCBUShot::controlStiffness_entryCondition, &NmRsCBUShot::controlStiffness_entry, &NmRsCBUShot::controlStiffness_tick, &NmRsCBUShot::controlStiffness_exitCondition, &NmRsCBUShot::controlStiffness_exit, m_controlStiffnessEnabled, timeStep);
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-DefArms"); 
#endif
    substateTick(&NmRsCBUShot::defaultArmMotion_entryCondition, &NmRsCBUShot::defaultArmMotion_entry, &NmRsCBUShot::defaultArmMotion_tick, &NmRsCBUShot::defaultArmMotion_exitCondition, &NmRsCBUShot::defaultArmMotion_exit, m_defaultArmMotionEnabled, timeStep);
    m_armProperty.arm = getLeftArm();
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour("-injLArm"); 
#endif
    substateTick(&NmRsCBUShot::injuredLeftArm_entryCondition, &NmRsCBUShot::injuredLeftArm_entry, &NmRsCBUShot::injuredArm_tick, &NmRsCBUShot::injuredArm_exitCondition, &NmRsCBUShot::injuredArm_exit, m_injuredLeftArmEnabled, timeStep);
    m_armProperty.arm = getRightArm();
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-injRArm"); 
#endif
    substateTick(&NmRsCBUShot::injuredRightArm_entryCondition, &NmRsCBUShot::injuredRightArm_entry, &NmRsCBUShot::injuredArm_tick, &NmRsCBUShot::injuredArm_exitCondition, &NmRsCBUShot::injuredArm_exit, m_injuredRightArmEnabled, timeStep);
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour("-OnGround"); 
#endif
    substateTick(&NmRsCBUShot::onGround_entryCondition, &NmRsCBUShot::onGround_entry, &NmRsCBUShot::onGround_tick, &NmRsCBUShot::onGround_exitCondition, &NmRsCBUShot::onGround_exit, m_onGroundEnabled, timeStep);
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-Fallen"); 
#endif
    substateTick(&NmRsCBUShot::justWhenFallen_entryCondition, &NmRsCBUShot::justWhenFallen_entry, &NmRsCBUShot::justWhenFallen_tick, &NmRsCBUShot::justWhenFallen_exitCondition, &NmRsCBUShot::justWhenFallen_exit, m_justWhenFallenEnabled, timeStep);
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour("-chick"); 
#endif
    substateTick(&NmRsCBUShot::chickenArms_entryCondition, &NmRsCBUShot::chickenArms_entry, &NmRsCBUShot::chickenArms_tick, &NmRsCBUShot::chickenArms_exitCondition, NULL, m_chickenArmsEnabled, timeStep);
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-fling"); 
#endif
    substateTick(&NmRsCBUShot::fling_entryCondition, &NmRsCBUShot::fling_entry, &NmRsCBUShot::fling_tick, &NmRsCBUShot::fling_exitCondition, &NmRsCBUShot::fling_exit, m_flingEnabled, timeStep);
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-sfb"); 
#endif
    substateTick(&NmRsCBUShot::shotFromBehind_entryCondition, &NmRsCBUShot::shotFromBehind_entry, &NmRsCBUShot::shotFromBehind_tick, &NmRsCBUShot::shotFromBehind_exitCondition, &NmRsCBUShot::shotFromBehind_exit, m_shotFromBehindEnabled, timeStep);
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-sig"); 
#endif
    substateTick(&NmRsCBUShot::shotInGuts_entryCondition, &NmRsCBUShot::shotInGuts_entry, &NmRsCBUShot::shotInGuts_tick, &NmRsCBUShot::shotInGuts_exitCondition, NULL, m_shotInGutsEnabled, timeStep);
    m_reachArm = &m_reachLeft;
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-reachL"); 
#endif
    substateTick(&NmRsCBUShot::reachLeft_entryCondition, &NmRsCBUShot::reachLeft_entry, &NmRsCBUShot::reachArm_tick, &NmRsCBUShot::reachLeft_exitCondition, &NmRsCBUShot::reachLeft_exit, m_reachLeftEnabled, timeStep);
    m_reachArm = &m_reachRight;
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-reachR"); 
#endif
    substateTick(&NmRsCBUShot::reachRight_entryCondition, &NmRsCBUShot::reachRight_entry, &NmRsCBUShot::reachArm_tick, &NmRsCBUShot::reachRight_exitCondition, &NmRsCBUShot::reachRight_exit, m_reachRightEnabled, timeStep);
    m_injuredLegMask = bvmask_LegLeft;
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-injLegL"); 
#endif
    substateTick(&NmRsCBUShot::injuredLeftLeg_entryCondition, &NmRsCBUShot::injuredLeftLeg_entry, &NmRsCBUShot::injuredLeg_tick, &NmRsCBUShot::injuredLeftLeg_exitCondition, &NmRsCBUShot::injuredLeftLeg_exit, m_injuredLeftLegEnabled, timeStep);
    m_injuredLegMask = bvmask_LegRight;
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-injLegR"); 
#endif
    substateTick(&NmRsCBUShot::injuredRightLeg_entryCondition, &NmRsCBUShot::injuredRightLeg_entry, &NmRsCBUShot::injuredLeg_tick, &NmRsCBUShot::injuredRightLeg_exitCondition, &NmRsCBUShot::injuredRightLeg_exit, m_injuredRightLegEnabled, timeStep);
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour("-look"); 
#endif
    substateTick(&NmRsCBUShot::headLook_entryCondition, &NmRsCBUShot::headLook_entry, &NmRsCBUShot::headLook_tick, &NmRsCBUShot::headLook_exitCondition, &NmRsCBUShot::headLook_exit, m_headLookEnabled, timeStep);
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour("-melee"); 
#endif
    substateTick(&NmRsCBUShot::melee_entryCondition, &NmRsCBUShot::melee_entry, &NmRsCBUShot::melee_tick, &NmRsCBUShot::melee_exitCondition, &NmRsCBUShot::melee_exit, m_meleeEnabled, timeStep);
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-ftk"); 
#endif
    substateTick(&NmRsCBUShot::fallToKnees_entryCondition, &NmRsCBUShot::fallToKnees_entry, &NmRsCBUShot::fallToKnees_tick, &NmRsCBUShot::fallToKnees_exitCondition, NULL, m_fallToKneesEnabled, timeStep);
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("-pointGun"); 
#endif
    substateTick(&NmRsCBUShot::pointGun_entryCondition, &NmRsCBUShot::pointGun_entry, &NmRsCBUShot::pointGun_tick, &NmRsCBUShot::pointGun_exitCondition, &NmRsCBUShot::pointGun_exit, m_pointGunEnabled, timeStep);
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif

    //send feedback
    //send feedback about arm reaching state
    if (feedback)
    {
      feedback->m_agentID = m_character->getID();
      feedback->m_argsCount = 2;

      ART::ARTFeedbackInterface::FeedbackUserdata data;
      data.setBool(m_reachLeftEnabled);
      feedback->m_args[0] = data;
      data.setBool(m_reachRightEnabled);
      feedback->m_args[1] = data;

      strcpy(feedback->m_behaviourName, NMShotRFWFeedbackName);
      feedback->onBehaviourEvent();
    }
    if (m_parameters.spineBlendZero > 0.f && (m_parameters.spineBlendExagCPain || m_hitTime > 
      m_parameters.exagDuration + m_parameters.exagSmooth2Zero + m_parameters.exagZeroTime +
      m_parameters.cpainSmooth2Time + m_parameters.cpainDuration + m_parameters.cpainSmooth2Zero))
    {
      float blendFactor = 1.f;
      float averageSpeed = rage::Sqrtf(2.0f * m_character->getKineticEnergyPerKilo_RelativeVelocity()) * 0.5f;
      float motionMultiplier = rage::Clamp(2.f*averageSpeed, 0.001f, 1.0f); 
      float spine0Lean2 = getSpine()->getSpine0()->getDesiredLean2();
      blendFactor = m_parameters.spineBlendZero + ( (1.0f - motionMultiplier) * (1.f-m_parameters.spineBlendZero));
      getSpine()->blendToZeroPose(getSpineInput(), blendFactor);
      getSpineInputData()->getSpine0()->setDesiredLean2(spine0Lean2);
    }

    m_newHit = false; // ie so newHit is only ever true for one frame

    static bool dontPointToes = true;
    if (dontPointToes)
      return eCBUTaskComplete;

    rage::Vector3 bodyVel(m_character->m_COMvelRelative),bodyCom(m_character->m_COM);
    rage::Vector3 rightFootP(getRightLeg()->getFoot()->getPosition()),leftFootP(getLeftLeg()->getFoot()->getPosition());
    m_character->levelVector(bodyCom, 0.f); 
    m_character->levelVector(bodyVel, 0.f); 
    m_character->levelVector(rightFootP, 0.f);
    m_character->levelVector(leftFootP, 0.f);

    bodyVel.Normalize();

    rage::Vector3 l2Com(bodyCom),r2Com(bodyCom);
    l2Com -= leftFootP;
    r2Com -= rightFootP;

    float l2ComMag = l2Com.Mag(); 
    float r2ComMag = r2Com.Mag(); 
    l2Com.Normalize();
    r2Com.Normalize();
    float l2ComDotVel = l2Com.Dot(bodyVel);
    float r2ComDotVel = r2Com.Dot(bodyVel);     
    if ((dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep))// && (l2rDotVel < 0.3f))// && (getRightLeg()->getFoot()->collidedWithNotOwnCharacter()))
      //if ((dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)  && (getRightLeg()->getFoot()->collidedWithNotOwnCharacter()))
    {
      float addLean1 = 0.f;// = - m_character->m_COMvelRelativeMag*10.f*(getRightLeg()->getFoot()->getLinearVelocity() - m_character->getFloorVelocity()).Mag();
      if (r2ComDotVel > 0.3f)
      {
        addLean1 = -10.f*r2ComMag*m_character->m_COMvelRelativeMag;
        addLean1 = - m_character->m_COMvelRelativeMag*0.7f*(getRightLeg()->getFoot()->getLinearVelocity() - m_character->getFloorVelocity()).Mag();
      }

      //point right toes more
      rage::Vector3 footVel(m_character->m_COMvelRelative);
      rage::Vector3 footBack(m_character->m_COMTM.c); //back
      m_character->levelVector(footVel,0.f);
      m_character->levelVector(footBack,0.f);
      footVel.Normalize();
      footBack.Normalize();
      float dum = footVel.Dot(footBack);
      if (dum<0.5f)//don't point toes if going backwards

      // limbs todo ensure this is intended as an internal blend.
      getRightLegInputData()->getAnkle()->setDesiredLean1(rage::Clamp(getRightLegInputData()->getAnkle()->getDesiredLean1() + addLean1,-9.f,9.f));
    }
    if ((dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep) )//&& (l2rDotVel > -0.3f))// && (getLeftLeg()->getFoot()->collidedWithNotOwnCharacter()))
      //if ((dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)  && (getLeftLeg()->getFoot()->collidedWithNotOwnCharacter()))
    {
      float addLean1 = 0.f;// = - m_character->m_COMvelRelativeMag*10.f*(getLeftLeg()->getFoot()->getLinearVelocity() - m_character->getFloorVelocity()).Mag();
      if (l2ComDotVel > 0.3f)
      {
        addLean1 = -10.f*l2ComMag*m_character->m_COMvelRelativeMag;
        addLean1 =  - m_character->m_COMvelRelativeMag*0.7f*(getLeftLeg()->getFoot()->getLinearVelocity() - m_character->getFloorVelocity()).Mag();
      }

      //point left toes more
      rage::Vector3 footVel(m_character->m_COMvelRelative);
      rage::Vector3 footBack(m_character->m_COMTM.c); //back
      m_character->levelVector(footVel,0.f);
      m_character->levelVector(footBack,0.f);
      footVel.Normalize();
      footBack.Normalize();
      float dum = footVel.Dot(footBack);
      if (dum<0.5f)//don't point toes if going backwards

      // limbs todo ensure this is intended as an internal blend.
      getLeftLegInputData()->getAnkle()->setDesiredLean1(rage::Clamp(getLeftLegInputData()->getAnkle()->getDesiredLean1() + addLean1,-9.f,9.f));
    }


    return eCBUTaskComplete;
  }

  void NmRsCBUShot::substateTick(bool (NmRsCBUShot::*entryCondition)(),
    void (NmRsCBUShot::*entry)(),
    void (NmRsCBUShot::*tick)(float step),
    bool (NmRsCBUShot::*exitCondition)(),
    void (NmRsCBUShot::*exit)(), bool &enabled, float timeStep)
  {
    if (enabled && exitCondition && (this->*exitCondition)())
    {
      enabled = false;
      if (exit)
        (this->*exit)();
    }
    if (!enabled && entryCondition && (this->*entryCondition)())
    {
      enabled = true;
      if (entry)
        (this->*entry)();
    }
    if (enabled && tick)
      (this->*tick)(timeStep);
  }

  //----------------CROUCH SHOT ------------------------------------------------
  //-------------------------------------------------------------------------
  bool NmRsCBUShot::crouchShot_entryCondition()
  {
    return m_parameters.crouching && !m_falling;
  }

  void NmRsCBUShot::crouchShot_tick(float /*timeStep*/)
  {
    // if doing feedback! This is needed as a success scenario for crouching.
    float speed = m_character->m_COMvelRelativeMag;
    if (speed < 0.05f && m_time > 0.4f)
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      Assert(feedback); // should exist
      strcpy(feedback->m_behaviourName, NMShotFeedbackName);
      feedback->onBehaviourSuccess();
    }

    NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    //The balacer can be in a fail state from the crouch pose so ignore this failure
    //for a while to allow the charatcer to stand up from crouch.
    if (m_time < 0.5f)//MMMMMtodo should be shorter? i.e time to get above balancer failure height or made a param so that it is more of a scramble time aswell
      dynamicBalancerTask->setCrouching(true);
    else
      dynamicBalancerTask->setCrouching(false);

  }

  bool NmRsCBUShot::crouchShot_exitCondition()
  {
    return m_falling;
  }

  void NmRsCBUShot::crouchShot_exit()
  {
    NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    dynamicBalancerTask->setCrouching(false);
  }

#if ART_ENABLE_BSPY
  void NmRsCBUShot::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    static const char* failTypeStrings[] =
    {
#define BAL_NAME_ACTION(_name) #_name ,
      BAL_STATES(BAL_NAME_ACTION)
#undef BAL_NAME_ACTION
    };
    static const char* ftkArmTypeStrings[] = 
    {
      "useFallArms",
      "armsIn",
      "armsOut",
    };

    bspyTaskVar(m_parameters.localHitPointInfo, true);
    bspyTaskVar(m_parameters.normal, true);
    bspyTaskVar(m_parameters.hitPoint, true);
    bspyTaskVar(m_parameters.bulletVel, true);
    bspyTaskVar(m_parameters.headLookPos, true);

    bspyTaskVar(m_parameters.angVelScale, true);
    bspyTaskVar_Bitfield32(m_parameters.angVelScaleMask, true);
    bspyTaskVar(m_parameters.flingWidth, true);
    bspyTaskVar(m_parameters.timeBeforeReachForWound, true);
    bspyTaskVar(m_parameters.grabHoldTime, true);
    bspyTaskVar(m_parameters.alwaysReachTime, true);
    bspyTaskVar(m_parameters.reachWithOneHand, true);
    bspyTaskVar(m_parameters.allowLeftPistolRFW, true);
    bspyTaskVar(m_parameters.allowRightPistolRFW, true);
    bspyTaskVar(m_parameters.rfwWithPistol, true);
    bspyTaskVar(m_parameters.spineBlendExagCPain, true);    
    bspyTaskVar(m_parameters.spineBlendZero, true);     
    bspyTaskVar(m_parameters.bulletProofVest, true);
    bspyTaskVar(m_parameters.alwaysResetLooseness, true);
    bspyTaskVar(m_parameters.alwaysResetNeckLooseness, true);
    bspyTaskVar(m_parameters.minArmsLooseness, true);
    bspyTaskVar(m_parameters.minLegsLooseness, true);

    //BulletExaggeration/reflex - just affects spine at the mo
    bspyTaskVar(m_parameters.exagDuration, true);
    bspyTaskVar(m_parameters.exagMag, true);
    bspyTaskVar(m_parameters.exagTwistMag, true);
    bspyTaskVar(m_parameters.exagSmooth2Zero, true);
    bspyTaskVar(m_parameters.exagZeroTime, true);
    //Conscious pain - just affects spine at the mo
    bspyTaskVar(m_parameters.cpainSmooth2Time, true);
    bspyTaskVar(m_parameters.cpainDuration, true);
    bspyTaskVar(m_parameters.cpainMag, true);
    bspyTaskVar(m_parameters.cpainTwistMag, true);
    bspyTaskVar(m_parameters.cpainSmooth2Zero, true);

    bspyTaskVar(m_parameters.bodyStiffness, true);
    bspyTaskVar(m_parameters.initialNeckStiffness, true);
    bspyTaskVar(m_parameters.initialNeckDamping, true);
    bspyTaskVar(m_parameters.neckStiffness, true);
    bspyTaskVar(m_parameters.neckDamping, true);
    bspyTaskVar(m_parameters.armStiffness, true);
    bspyTaskVar(m_parameters.spineDamping, true);
    bspyTaskVar(m_parameters.kMultOnLoose, true);
    bspyTaskVar(m_parameters.kMult4Legs, true);
    bspyTaskVar(m_parameters.loosenessAmount, true);
    bspyTaskVar(m_parameters.looseness4Fall, true);
    bspyTaskVar(m_parameters.looseness4Stagger, true);
    bspyTaskVar(m_parameters.initialWeaknessZeroDuration, true);
    bspyTaskVar(m_parameters.initialWeaknessRampDuration, true);
    bspyTaskVar(m_parameters.initialNeckDuration, true);
    bspyTaskVar(m_parameters.initialNeckRampDuration, true);

    bspyTaskVar(m_parameters.useCStrModulation, true);
    bspyTaskVar(m_parameters.cStrUpperMin, true);
    bspyTaskVar(m_parameters.cStrUpperMax, true);
    bspyTaskVar(m_parameters.cStrLowerMin, true);
    bspyTaskVar(m_parameters.cStrLowerMax, true);

    bspyTaskVar(m_parameters.useHeadLook, true);
    bspyTaskVar(m_parameters.headLookAtWoundMinTimer, true);
    bspyTaskVar(m_parameters.headLookAtWoundMaxTimer, true);
    bspyTaskVar(m_parameters.headLookAtHeadPosMaxTimer, true);
    bspyTaskVar(m_parameters.headLookAtHeadPosMinTimer, true);
    bspyTaskVar(m_headLook.toggleTimer, false);
    //bspyTaskVar(m_headLook.toggleTimer, m_headLook.state);

    //Injured leg
    bspyTaskVar(m_parameters.allowInjuredLeg, true);
    bspyTaskVar(m_parameters.allowInjuredLowerLegReach, true);
    bspyTaskVar(m_parameters.allowInjuredThighReach, true);
    bspyTaskVar(m_parameters.legForceStep, true);
    bspyTaskVar(m_parameters.timeBeforeCollapseWoundLeg, true);
    bspyTaskVar(m_parameters.legInjuryTime, true);
    bspyTaskVar(m_parameters.legLiftTime, true);
    bspyTaskVar(m_parameters.legInjury, true);   
    bspyTaskVar(m_parameters.legInjuryLiftHipPitch, true);   
    bspyTaskVar(m_parameters.legInjuryHipPitch, true);   
    bspyTaskVar(m_parameters.legInjuryLiftSpineBend, true);   
    bspyTaskVar(m_parameters.legInjurySpineBend, true);   
    bspyTaskVar(m_parameters.legLimpBend, true);


    bspyTaskVar(m_parameters.melee, true);
    bspyTaskVar(m_parameters.fling, true);
    bspyTaskVar(m_parameters.crouching, true);
    bspyTaskVar(m_parameters.brace, true);
    bspyTaskVar(m_parameters.releaseWound, true);
    bspyTaskVar(m_parameters.pointGun, true);

    bspyTaskVar(m_parameters.useArmsWindmill, true);
    if (m_parameters.useArmsWindmill)
    {
      bspyTaskVar(m_parameters.AWSpeedMult, true);
      bspyTaskVar(m_parameters.AWRadiusMult, true);
      bspyTaskVar(m_parameters.AWStiffnessAdd, true);
    }

    bspyTaskVar(m_parameters.allowInjuredArm, true);     
    bspyTaskVar(m_parameters.bodyPart, true);
    bspyTaskVar(m_parameters.chickenArms, true);
    bspyTaskVar(m_parameters.reachForWound, true);
    bspyTaskVar(m_parameters.useCatchFallOnFall, true);
    bspyTaskVar(m_parameters.useExtendedCatchFall, true);
    bspyTaskVar(m_parameters.stableHandsAndNeck, true);
    bspyTaskVar(m_disableBalance, true);
    bspyTaskVar(m_newHit, true);

    if (m_parameters.addShockSpin)
    {
      bspyTaskVar(m_parameters.addShockSpin, true);
      bspyTaskVar(m_parameters.shockSpinMin, true);
      bspyTaskVar(m_parameters.shockSpinMax, true);
      bspyTaskVar(m_parameters.shockSpinLiftForceMult, true);
      bspyTaskVar(m_parameters.shockSpinDecayMult, true);
      bspyTaskVar(m_parameters.shockSpinScalePerComponent, true);
      bspyTaskVar(m_parameters.shockSpinMaxTwistVel, true);
      bspyTaskVar(m_parameters.shockSpinAirMult, true);
      bspyTaskVar(m_parameters.shockSpin1FootMult, true);
      bspyTaskVar(m_parameters.shockSpinFootGripMult, true);     
      bspyTaskVar(m_parameters.shockSpinScaleByLeverArm, true);
      bspyTaskVar(m_parameters.randomizeShockSpinDirection, true);
      bspyTaskVar(m_parameters.alwaysAddShockSpin, true);
	  bspyTaskVar(m_parameters.bracedSideSpinMult, true);
    }

    bspyTaskVar(m_parameters.fallToKnees, true);
    if(m_parameters.fallToKnees)
    {
      bspyTaskVar(m_parameters.ftkBalanceTime, true);
      bspyTaskVar(m_parameters.ftkHelperForce, true);
      bspyTaskVar(m_parameters.ftkHelperForceOnSpine, true);
      bspyTaskVar(m_parameters.ftkAlwaysChangeFall, true);       
      bspyTaskVar(m_parameters.ftkLeanHelp, true);
      bspyTaskVar(m_parameters.ftkSpineBend, true);      
      bspyTaskVar(m_parameters.ftkStiffSpine, true);       
      bspyTaskVar(m_parameters.ftkImpactLooseness, true);
      bspyTaskVar(m_parameters.ftkImpactLoosenessTime, true);
      bspyTaskVar(m_parameters.ftkBendRate, true);
      bspyTaskVar(m_parameters.ftkHipBlend, true);
      bspyTaskVar(m_parameters.ftkFricMult, true);
      bspyTaskVar(m_parameters.ftkHipAngleFall, true);
      bspyTaskVar(m_parameters.ftkPitchForwards, true);
      bspyTaskVar(m_parameters.ftkPitchBackwards, true);
      bspyTaskVar(m_parameters.ftkFallBelowStab, true);
      bspyTaskVar(m_parameters.ftkBalanceAbortThreshold, true);
      bspyTaskVar(m_parameters.ftkFailMustCollide, true); 
      bspyTaskVar(m_parameters.ftkReleaseReachForWound, true);
      bspyTaskVar(m_parameters.ftkReleasePointGun, true);        
      bspyTaskVar_StringEnum(m_parameters.ftkOnKneesArmType, ftkArmTypeStrings, true);  
    }
    //snap
    bspyTaskVar(m_parameters.snap, true);
    if(m_parameters.snap)
    {
      bspyTaskVar(m_parameters.snapMag, true);
      bspyTaskVar(m_parameters.snapMovingMult, true);
      bspyTaskVar(m_parameters.snapBalancingMult, true);
      bspyTaskVar(m_parameters.snapAirborneMult, true);
      bspyTaskVar(m_parameters.snapMovingThresh, true);
      bspyTaskVar(m_parameters.snapDirectionRandomness, true);
      bspyTaskVar(m_parameters.snapLeftArm, true);
      bspyTaskVar(m_parameters.snapRightArm, true);       
      bspyTaskVar(m_parameters.snapLeftLeg, true);
      bspyTaskVar(m_parameters.snapRightLeg, true);       
      bspyTaskVar(m_parameters.snapSpine, true);       
      bspyTaskVar(m_parameters.snapNeck, true);       
      bspyTaskVar(m_parameters.snapPhasedLegs, true);       
      bspyTaskVar(m_parameters.snapHipType, true);  
      bspyTaskVar(m_parameters.snapUseBulletDir, true);       
      bspyTaskVar(m_parameters.unSnapInterval, true);
      bspyTaskVar(m_parameters.unSnapRatio, true);
      bspyTaskVar(m_parameters.snapUseTorques, true);       
      bspyTaskVar(m_parameters.snapHitPart, true);		
    }

    bspyTaskVar(m_hitPointWorld, false);
    bspyTaskVar(m_hitPointLocal, false);
    bspyTaskVar(m_hitNormalLocal, false);
    rage::Vector3 hitNormalWorld;
    rage::Matrix34 mat;
    m_character->getGenericPartByIndex(m_parameters.bodyPart)->getMatrix(mat);
    mat.Transform3x3(m_hitNormalLocal, hitNormalWorld);
    bspyTaskVar(hitNormalWorld, false);
    bspyTaskVar(m_woundLOffset, false);
    bspyTaskVar(m_woundROffset, false);
    bspyTaskVar(m_woundLNormal, false);
    bspyTaskVar(m_woundRNormal, false);

    bspyTaskVar(m_hitTime, false);
    bspyTaskVar(m_hitTimeLeft, false);
    bspyTaskVar(m_hitTimeRight, false);   
    bspyTaskVar(m_controlStiffnessTime, false);
    bspyTaskVar(m_controlNeckStiffnessTime, false);
    bspyTaskVar(m_defaultBodyStiffness, false);
    bspyTaskVar(m_spineStiffness, false);
    bspyTaskVar(m_spineDamping, false);
    bspyTaskVar(m_armsStiffness, false);
    bspyTaskVar(m_armsDamping, false);   
    bspyTaskVar(m_neckStiffness, false);
    bspyTaskVar(m_neckDamping, false);
    bspyTaskVar(m_wristStiffness, false);
    bspyTaskVar(m_wristDamping, false);
    bspyTaskVar(m_reactionTime, false);   
    bspyTaskVar(m_upperBodyStiffness, false);
    bspyTaskVar(m_lowerBodyStiffness, false);
    bspyTaskVar(m_spineLean1, false);
    bspyTaskVar(m_relaxTimeUpper, false);
    bspyTaskVar(m_relaxPeriodUpper, false);
    bspyTaskVar(m_relaxTimeLower, false);
    bspyTaskVar(m_relaxPeriodLower, false);
    bspyTaskVar(m_velForwards, false);
    bspyTaskVar(m_hitPointRight, false);
    bspyTaskVar(m_time, false);
    bspyTaskVar(m_controlStiffnessStrengthScale, false);
    bspyTaskVar(m_controlNeckStiffnessScale, false);     
    bspyTaskVar(m_chickenArmsTimeSpaz, false);

    bspyTaskVar(m_exagLean1, false);
    bspyTaskVar(m_exagLean2, false);
    bspyTaskVar(m_exagTwist, false);

    if(m_parameters.addShockSpin)
    {
      bspyTaskVar(m_torqueSpin, false);
      bspyTaskVar(m_torqueSpinTime, false);
    }

    bspyTaskVar(m_defaultArmMotionEnabled,false);
    if(m_defaultArmMotionEnabled)
    {
      bspyTaskVar(m_defaultArmMotion.timer1, false);
      bspyTaskVar(m_defaultArmMotion.timer2, false);
      bspyTaskVar(m_defaultArmMotion.releaseLeftWound, false);
      bspyTaskVar(m_defaultArmMotion.releaseRightWound, false);
      bspyTaskVar(m_defaultArmMotion.armsWindmillTimeLeft, false);
      bspyTaskVar(m_defaultArmMotion.leftDefault, false);
      bspyTaskVar(m_defaultArmMotion.leftArmsWindmill, false);
      bspyTaskVar(m_defaultArmMotion.leftBrace, false);
      bspyTaskVar(m_defaultArmMotion.armsWindmillTimeRight, false);
      bspyTaskVar(m_defaultArmMotion.rightDefault, false);
      bspyTaskVar(m_defaultArmMotion.rightArmsWindmill, false);
      bspyTaskVar(m_defaultArmMotion.rightBrace, false);
    }

    bspyTaskVar(m_woundLPart, false);
    bspyTaskVar(m_woundRPart, false);
    bspyTaskVar(m_newReachL,false);
    bspyTaskVar(m_newReachR,false);
    bspyTaskVar(m_reachLeftEnabled,false);
    bspyTaskVar(m_reachRightEnabled,false);
    if(m_reachLeftEnabled)
    {
      bspyTaskVar(m_reachLeft.offset, false);
      bspyTaskVar(m_reachLeft.normal, false);
      bspyTaskVar(m_reachLeft.woundPart, false);
      bspyTaskVar(m_reachLeft.twist, false);
      bspyTaskVar(m_reachLeft.direction, false);
      bspyTaskVar(m_armTwist, false);
    }
    if(m_reachRightEnabled)
    {
      bspyTaskVar(m_reachRight.offset, false);
      bspyTaskVar(m_reachRight.normal, false);
      bspyTaskVar(m_reachRight.woundPart, false);
      bspyTaskVar(m_reachRight.twist, false);
      bspyTaskVar(m_reachRight.direction, false);
    }

    bspyTaskVar(m_injuredLArm,false);
    bspyTaskVar(m_injuredRArm,false);
    bspyTaskVar(m_injuredLLeg,false);
    bspyTaskVar(m_injuredRLeg,false);
    bspyTaskVar(m_injuredLeftArmEnabled,false);
    bspyTaskVar(m_injuredRightArmEnabled,false);
    bspyTaskVar(m_injuredArmElbowBend, false);
    bspyTaskVar(m_injuredLeftLegEnabled,false);
    bspyTaskVar(m_injuredRightLegEnabled,false);

    if(m_meleeEnabled)
    {
      bspyTaskVar(m_melee.motionMultiplier, false);
      bspyTaskVar(m_melee.neckTilt, false);
      bspyTaskVar(m_melee.LESwingMin, false);
      bspyTaskVar(m_melee.LESwingMax, false);
      bspyTaskVar(m_melee.RESwingMin, false);
      bspyTaskVar(m_melee.RESwingMax, false);
      bspyTaskVar(m_melee.hipYaw, false);
      bspyTaskVar(m_melee.headYaw, false);
      bspyTaskVar(m_melee.hipangle, false);
      bspyTaskVar(m_melee.bodyangle, false);
      bspyTaskVar(m_melee.headangle, false);
      bspyTaskVar(m_melee.headLookyTimer, false);
    }

    if(m_flingEnabled)
    {
      bspyTaskVar(m_fling.flingTimer, false);
      bspyTaskVar(m_fling.leftDir, false);
      bspyTaskVar(m_fling.rightDir, false);
      bspyTaskVar(m_fling.backLeanDir, false);
      bspyTaskVar(m_fling.period, false);
      bspyTaskVar(m_fling.sAngle, false);
      bspyTaskVar(m_fling.fAngle, false);
      bspyTaskVar(m_fling.bodyRegion, false);
      bspyTaskVar(m_fling.useLeft, false);
      bspyTaskVar(m_fling.useRight, false);
    }

    bspyTaskVar(m_falling,false);
    bspyTaskVar(m_archBack,false);
    bspyTaskVar(m_headLookAtWound,false);
    bspyTaskVar(m_newHitEnabled,false);
    bspyTaskVar(m_controlStiffnessEnabled,false);
    bspyTaskVar(m_headLookEnabled,false);
    bspyTaskVar(m_onGroundEnabled,false);
    bspyTaskVar(m_justWhenFallenEnabled,false);
    bspyTaskVar(m_chickenArmsEnabled,false);
    bspyTaskVar(m_flingEnabled,false);
    bspyTaskVar(m_meleeEnabled,false);
    bspyTaskVar(m_crouchShotEnabled,false);
    bspyTaskVar(m_shotFromBehindEnabled,false);
    bspyTaskVar(m_pointGunEnabled,false);

    bspyTaskVar(m_hitFromBehind,false);

    if(m_fallToKneesEnabled)
    {
      bspyTaskVar(m_fTK.m_ftkTimer, false);
      bspyTaskVar(m_fTK.m_bendLegs,false);
      bspyTaskVar(m_fTK.m_LkneeHasHit,false);
      bspyTaskVar(m_fTK.m_RkneeHasHit,false);
      bspyTaskVar(m_fTK.m_squatting,false);
      bspyTaskVar(m_fTK.m_hitFloor,false);
      bspyTaskVar(m_fTK.m_hipMoventBackwards,false);
      bspyTaskVar(m_fTK.m_ftkLoosenessTimer, false);
      bspyTaskVar(m_fTK.m_LkneeHitLooseness,false);
      bspyTaskVar(m_fTK.m_RkneeHitLooseness,false);
      bspyTaskVar(m_fTK.m_fallingBack,false);	

      //rage::Matrix34 toeTM;
      //float footLength = 0.26448f;
      //float footHeight = 0.07522f;
      //getLeftLeg()->getFoot()->getBoundMatrix(&toeTM);
      //rage::Vector3 ltoe = getLeftLeg()->getFoot()->getPosition();
      //ltoe -= toeTM.c*footLength*0.5f;
      //ltoe -= toeTM.b*footHeight*0.5f;
      //bspyTaskVar(ltoe, false);
      //getRightLeg()->getFoot()->getBoundMatrix(&toeTM);
      //rage::Vector3 rtoe = getRightLeg()->getFoot()->getPosition();
      //rtoe -= toeTM.c*footLength*0.5f;
      //rtoe -= toeTM.b*footHeight*0.5f;
      //bspyTaskVar(rtoe, false);
    }

    if(m_parameters.shotFromBehind)
    {
      bspyTaskVar(m_parameters.shotFromBehind,true);
      bspyTaskVar(m_parameters.sfbArmsOnset,true);
      bspyTaskVar(m_parameters.sfbForceBalancePeriod,true);
      bspyTaskVar(m_parameters.sfbHipAmount,true);
      bspyTaskVar(m_parameters.sfbKneeAmount,true);
      bspyTaskVar(m_parameters.sfbKneesOnset,true);
      bspyTaskVar(m_parameters.sfbNeckAmount,true);
      bspyTaskVar(m_parameters.sfbNoiseGain,true);
      bspyTaskVar(m_parameters.sfbPeriod,true);
      bspyTaskVar(m_parameters.sfbSpineAmount,true);

      bspyTaskVar(m_shotFromBehindEnabled,false);
    }
    if(m_shotFromBehindEnabled)
    {
      bspyTaskVar(m_shotFromBehind.timer,false);
      bspyTaskVar(m_shotFromBehind.sfbNoiseSeed,false);
      bspyTaskVar(m_shotFromBehind.sfbRandTimer,false);
    }

    if(m_parameters.shotInGuts)
    {
      bspyTaskVar(m_parameters.sigForceBalancePeriod,true);
      bspyTaskVar(m_parameters.sigHipAmount,true);
      bspyTaskVar(m_parameters.sigKneeAmount,true);
      bspyTaskVar(m_parameters.sigKneesOnset,true);
      bspyTaskVar(m_parameters.sigNeckAmount,true);
      bspyTaskVar(m_parameters.sigPeriod,true);
      bspyTaskVar(m_parameters.sigSpineAmount,true);

      bspyTaskVar(m_shotInGutsEnabled,false);
    }
    if(m_shotInGutsEnabled)
    {
      bspyTaskVar(m_shotInGuts.timer,true);
    }

    if (m_parameters.allowInjuredArm)
    {
      bspyTaskVar(m_injuredArm.hipYaw,true);
      bspyTaskVar(m_injuredArm.hipRoll,true);
      bspyTaskVar(m_injuredArm.forceStepExtraHeight,true);
      bspyTaskVar(m_injuredArm.shrugTime,true);
      bspyTaskVar(m_injuredArm.forceStep,true);
      bspyTaskVar(m_injuredArm.stepTurn,true);

      bspyTaskVar(m_injuredArm.stepTurn,true);
      bspyTaskVar(m_injuredArm.injuredArmTime,true);
      bspyTaskVar(m_injuredArm.velMultiplierStart,true);
      bspyTaskVar(m_injuredArm.velMultiplierEnd,true);
      bspyTaskVar(m_injuredArm.velForceStep,true);
      bspyTaskVar(m_injuredArm.velStepTurn,true);
      bspyTaskVar(m_injuredArm.velScales,true);
    }

    bspyTaskVar(m_injuredLeftLeg.legLiftTimer,false);
    bspyTaskVar(m_injuredLeftLeg.legInjuryTimer,false);
    bspyTaskVar(m_injuredLeftLeg.forceStepTimer,false);  
    bspyTaskVar(m_injuredRightLeg.legLiftTimer,false);
    bspyTaskVar(m_injuredRightLeg.legInjuryTimer,false);
    bspyTaskVar(m_injuredRightLeg.forceStepTimer,false);


    if (m_injuredLeftArmEnabled || m_injuredRightArmEnabled)
    {
      bspyTaskVar(m_turnTo, false);
    }

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    bspyTaskVar(dynamicBalancerTask->m_distKnee, false);
    bspyTaskVar(dynamicBalancerTask->m_heightKnee, false);
    bspyTaskVar(dynamicBalancerTask->m_distHeightKneeRatio, false);
    bspyTaskVar(dynamicBalancerTask->m_dist, false);
    bspyTaskVar(dynamicBalancerTask->m_height, false);
    bspyTaskVar(dynamicBalancerTask->m_distHeightRatio, false);
    bspyTaskVar_StringEnum(dynamicBalancerTask->m_failType, failTypeStrings, false);  
    bspyTaskVar(m_bcrArms,true);
  }
#endif // ART_ENABLE_BSPY
}
