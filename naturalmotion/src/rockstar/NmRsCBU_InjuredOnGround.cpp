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
 * Rolling motion. Apply when the character is to be tumbling, either down a hill,
 * after being blown by an explosion, or hit by a car.
 * The character is in a rough foetal position, he puts his arms out to brace against
 * collisions with the ground, and he will relax after he stops tumbling.
 *
 * Roll Up behaviour. This is a pre-condition type behaviour which has to occur
 * before the character starts rolling or tumbling. Rolling and tumbling are
 * largely dependent on the shape of the character before it hits the ground/terrain
 * and the slope etc. of the ground/terrain. Part of the post-ground impact suite of behaviours.
 *
 * TDL current implementation simple curls the character up in proportion to how fast he's rotating
 * This behaviour is designed to transition in from a fall behaviour as the character approaches the ground
 */


#include "NmRsInclude.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"
#include "NmRsCBU_InjuredOnGround.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_Pedal.h"
#include "NmRsCBU_Flinch.h"
#include "NmRsCBU_InjuredOnGround.h"

namespace ART
{
  NmRsCBUInjuredOnGround::NmRsCBUInjuredOnGround(ART::MemoryManager* services) : CBUTaskBase(services, bvid_injuredOnGround),
    m_painState(kRelaxed),
    m_reachType(kNone),
    m_braceWithLeftFrames(0),
    m_braceWithRightFrames(0),
    m_ChangerHeadTarget(false),
    m_LookingAtAttacker(false),
	m_wristLean2(0.6f),
    m_Initialized(false),
    m_doBrace(false)
  {
    initialiseCustomVariables();
  }

  NmRsCBUInjuredOnGround::~NmRsCBUInjuredOnGround()
  {
  }

  void NmRsCBUInjuredOnGround::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_injuredOnGroundTimer       = 0.0f;
    m_ChestConstraint.Reset();
    m_PelvisConstraint.Reset();
  }

  void NmRsCBUInjuredOnGround::onActivate()
  {
    Assert(m_character);

    // Turn on joint driving
    m_character->setBodyDriveState(rage::phJoint::DRIVE_STATE_ANGLE_AND_SPEED);

    // decide which hands to grab with
    m_reachType = kBoth;
    m_doBrace = true;
	m_wristLean2 = 0.6f;

    // check that collisions are enabled
    for (int iPart = 0; iPart < m_character->getNumberOfParts(); iPart++)
      m_character->getGenericPartByIndex(iPart)->setCollisionEnabled(true);

    // Reset friction values
    //static float legFrictionMultiplier = 1.0f;
    //static float footFrictionMultiplier = 1.0f;
    static float armFrictionMultiplier = 0.5f;
    //static float headFrictionMultiplier = 1.0f;
	  //static float spineFrictionMultiplier = 1.0f;
    static float frictionMultiplier = 1.0f;
    m_character->setFrictionPreScale(frictionMultiplier);

    //getLeftLeg()->getShin()->setFrictionMultiplier(legFrictionMultiplier);
    //getRightLeg()->getShin()->setFrictionMultiplier(legFrictionMultiplier);
    //getLeftLeg()->getThigh()->setFrictionMultiplier(legFrictionMultiplier);
    //getRightLeg()->getThigh()->setFrictionMultiplier(legFrictionMultiplier);

    //getLeftLeg()->getFoot()->setFrictionMultiplier(footFrictionMultiplier);
    //getRightLeg()->getFoot()->setFrictionMultiplier(footFrictionMultiplier);

    m_character->setFrictionPreScale(armFrictionMultiplier, bvmask_ArmLeft | bvmask_ArmRight);

    //getSpine()->getNeckPart()->setFrictionMultiplier(headFrictionMultiplier);
    //getSpine()->getHeadPart()->setFrictionMultiplier(headFrictionMultiplier);

	//getSpine()->getPelvisPart()->setFrictionMultiplier(spineFrictionMultiplier);
	//getSpine()->getSpine0Part()->setFrictionMultiplier(spineFrictionMultiplier);
	//getSpine()->getSpine1Part()->setFrictionMultiplier(spineFrictionMultiplier);
	//getSpine()->getSpine2Part()->setFrictionMultiplier(spineFrictionMultiplier);

    m_injuredOnGroundTimer = 0;

    m_reachLeft.arm = kLeftArm;
    m_reachRight.arm = kRightArm;
  }

  void NmRsCBUInjuredOnGround::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();

    m_character->ReleaseConstraintSafely(m_ChestConstraint);
    m_character->ReleaseConstraintSafely(m_PelvisConstraint);

    m_character->setFrictionPreScale(1.0f);
  }

  CBUTaskReturn NmRsCBUInjuredOnGround::onTick(float timeStep)
  {
    NM_RS_DBG_LOGF(L"during");


#if NM_RUNTIME_LIMITS
    // Loosen up the shoulder twist limit
    static float shoulder1 = 1.3f; 
    static float shoulder2 = 0.9f; 
    static float shoulder3 = 2.0f; 
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtShoulder_Left)))->setLimits(shoulder1, shoulder2, shoulder3);
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtShoulder_Right)))->setLimits(shoulder1, shoulder2, shoulder3);
#endif

    // Update the timer
    m_injuredOnGroundTimer += timeStep;

    // Gather state data
    GatherStateData();

    // Foetal
    Foetal();

    // Roll
    Roll(timeStep);

    // Breathe
    Breathe();

    // Pedal
    Pedal(timeStep);

    // Grab for wound
    GrabForWound();

    // Head control
    HeadControl();

    // limbs: this is awkward to support in the new system. it should
    // probably be moved to a character function.
    // Check that nothing is too stiff
    for (int iJoint = 0; iJoint < m_character->getNumberOfEffectors(); iJoint++)
    {
#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour("-stiff");
#endif
      float stiff = m_character->getEffectorDirect(iJoint)->getMuscleStrength();
      if (stiff != 0.0f)
        stiff = sqrt(stiff);
      static float stiffMax = 9.0f;
      if (stiff > stiffMax)
        m_character->getEffectorDirect(iJoint)->setStiffness(stiffMax, 1.0f);
#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]);
#endif
    }

    //#if ART_ENABLE_BSPY
    //	  m_character->setSkeletonVizMode(NmRsCharacter::kSV_DesiredAngles);
    //
    //	  m_character->setSkeletonVizRoot(15);
    //	  BehaviourMask mask = bvmask_ArmRight;
    //	  m_character->setSkeletonVizMask(mask);
    //	  m_character->drawSkeleton(NmRsCharacter::kSV_DesiredAngles);
    //
    //	  m_character->setSkeletonVizRoot(11);
    //	  mask = bvmask_ArmLeft;
    //	  m_character->setSkeletonVizMask(mask);
    //	  m_character->drawSkeleton(NmRsCharacter::kSV_DesiredAngles);
    //
    //	  m_character->setSkeletonVizMode(NmRsCharacter::kSV_None);
    //	  m_character->setSkeletonVizMask(bvmask_None);
    //#endif

    m_Initialized = true;

    return eCBUTaskComplete;
  } 

  void NmRsCBUInjuredOnGround::HeadControl()
  {
    // Only look if there's an injury
    if (m_parameters.m_numInjuries > 0 && !m_downFacing)
    {
      rage::Vector3 target;
      rage::Vector3 toAttacker = m_parameters.m_attackerPos - getSpine()->getHeadPart()->getPosition();
      toAttacker.Normalize();
      float facingAttackerDot = toAttacker.Dot(-m_bodyBack);
      static float facingAttackerDotMin = 0.0f;
      if (!(m_ChangerHeadTarget && !m_LookingAtAttacker) && ((m_painState == kRelaxed && facingAttackerDot > facingAttackerDotMin) || (m_ChangerHeadTarget && m_LookingAtAttacker)))
      {
        target = m_parameters.m_attackerPos;
        static float Zadjust = 1.2f;
        target.z += Zadjust;
        if (!m_LookingAtAttacker)
          m_ChangerHeadTarget = true;
        m_LookingAtAttacker = true;
      }
      else
      {
        rage::Vector3 injury1WorldPosition;
        rage::Matrix34 partMat;
        m_character->getGenericPartByIndex(m_parameters.m_injury1Component)->getMatrix(partMat);
        partMat.Transform(m_parameters.m_injury1LocalPosition, injury1WorldPosition);
        target = injury1WorldPosition;
        if (m_LookingAtAttacker)
          m_ChangerHeadTarget = true;
        m_LookingAtAttacker = false;
      }

      NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      if (!headLookTask->isActive())
      {
        headLookTask->updateBehaviourMessage(NULL); // sets values to defaults
        headLookTask->m_parameters.m_pos = target;
        headLookTask->m_parameters.m_alwaysEyesHorizontal = false;
        headLookTask->m_parameters.twistSpine = false;//m_painState != kRelaxed;
        headLookTask->m_parameters.m_alwaysLook = true;
        headLookTask->activate();
      }
      else
      {
        // If already active, just update the look position
        headLookTask->m_parameters.m_pos = target;
        headLookTask->m_parameters.twistSpine = false;//m_painState != kRelaxed;
      }
    }
  }

  void NmRsCBUInjuredOnGround::GrabForWound()
  {
    static bool doReach = true;
    if (!doReach)
      return;

#if NM_RUNTIME_LIMITS
    // Allow hand to bend back farther
    static float lean1wristDefault = 0.5236f;
    static float lean2wristDefault = 0.6f;
    static float twistwristDefault = 1.2f;
    static float lean2Wrate = 0.05f;
    m_wristLean2 = rage::Max(lean2wristDefault, m_wristLean2-lean2Wrate);

    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtWrist_Left)))->setLimits(lean1wristDefault, m_wristLean2, twistwristDefault);
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtWrist_Right)))->setLimits(lean1wristDefault, m_wristLean2, twistwristDefault);
#endif

    // Give a chest wound if the character somehow doesn't have an injury or if it's an arm shot
    if (m_parameters.m_numInjuries == 0 || (m_parameters.m_injury1Component >= 11 && m_parameters.m_injury1Component <= 18))
    {
      m_parameters.m_numInjuries = 1;
      m_parameters.m_injury1LocalPosition = rage::Vector3(0.0f, 0.0f, -0.14f);
      m_parameters.m_injury1Component = 8;
    }

    // Move leg shots to the front of that leg's thigh
    if (m_parameters.m_injury1Component >= 1 && m_parameters.m_injury1Component <= 6)
    {
      m_parameters.m_injury1LocalPosition = rage::Vector3(0.0f, 0.0f, -0.1f);
      if (m_parameters.m_injury1Component <= 3) // left leg
        m_parameters.m_injury1Component = 1;
      else
        m_parameters.m_injury1Component = 4;
    }

    // Turn back shots into front shots along the spine (including pelvis and neck)
    if ((m_parameters.m_injury1Component == 0 || m_parameters.m_injury1Component == 19 || 
      (m_parameters.m_injury1Component >= 7 && m_parameters.m_injury1Component <= 10)) 
      && m_parameters.m_injury1LocalPosition.z > 0.0f)
    {
      m_parameters.m_injury1LocalPosition.z *= -1.0f;
    }

    // Turn low-spine shots into mid-spine shots
    if (m_parameters.m_injury1Component == 0 || m_parameters.m_injury1Component == 7)
    {
      m_parameters.m_injury1Component = 8;
    }

    // The local normal is determined by the local position
    m_parameters.m_injury1LocalNormal = m_parameters.m_injury1LocalPosition;
    m_parameters.m_injury1LocalNormal.Normalize();

    // Get world space wound position and normal
    rage::Vector3 injury1WorldPosition, injury1WorldNormal;
    rage::Vector3 injury2WorldPosition, injury2WorldNormal;
    bool bracedWithLeft = false;
    bool bracedWithRight = false;
    if (m_parameters.m_numInjuries > 0)
    {
      rage::Matrix34 partMat;
      m_character->getGenericPartByIndex(m_parameters.m_injury1Component)->getMatrix(partMat);
      partMat.Transform(m_parameters.m_injury1LocalPosition, injury1WorldPosition);
      partMat.Transform3x3(m_parameters.m_injury1LocalNormal, injury1WorldNormal);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", injury1WorldPosition);
      bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", injury1WorldNormal);
#endif
      if (!m_Initialized)
      {
        // Make sure that there is always a reach hand
        if (m_parameters.m_dontReachWithLeft && m_reachType == kLeft)
          m_reachType = kRight;
        else if (m_parameters.m_dontReachWithRight && m_reachType == kRight)
          m_reachType = kLeft;

        // Avoid reaching shot shots too close to the reaching arm's shoulder
        rage::Vector3 toShoulder = injury1WorldPosition - getRightArm()->getShoulder()->getJointPosition();
        float targetDistFromRightShoulder = toShoulder.Mag();
        toShoulder = injury1WorldPosition - getLeftArm()->getShoulder()->getJointPosition();
        float targetDistFromLeftShoulder = toShoulder.Mag();
        static float minDistFromShoulder = 0.22f;
        m_ShotInLeftArm = m_parameters.m_dontReachWithLeft;
        m_ShotInRightArm = m_parameters.m_dontReachWithRight;
        if (targetDistFromRightShoulder < minDistFromShoulder)
          m_parameters.m_dontReachWithRight = true;
        if (targetDistFromLeftShoulder < minDistFromShoulder)
          m_parameters.m_dontReachWithLeft = true;
      }

      bool didSomethingWithLeftArm = false;

      // Reach with the left hand + gravity compensation
      //bool reachedWithLeft = false;
      m_character->m_posture.useZMP = false;
      static float gravCompen = 1.0f;
      static float armStiffness = 10.0f;
      static float armStiffnessInsideRoll = 4.0f;
      static float armOutFrontStiffness = 8.0f;
      static float shoulderLean1 = 1.0f;
      static float shoulderLean2 = 1.0f;
      static float shoulderTwist = 0.0f;
      static float injuredShoulderLean1 = 0.5f;
      static float injuredShoulderLean2 = 1.0f;
      static float injuredShoulderTwist = 1.0f;
      static float elbowAngle = 0.2f;
      static float injuredElbowAngle = 0.6f;
      static float threshold = 0.3f;
      static float cleverIKstrength = 0.0f;
      if (!m_parameters.m_dontReachWithLeft && (m_reachType == kLeft || m_reachType == kBoth || m_painState != kRelaxed))
      {
#if ART_ENABLE_BSPY 
        m_character->setCurrentSubBehaviour("-rfbpL");
#endif

        getLeftArm()->setBodyStiffness(getLeftArmInput(), m_painState == kRollRight ? armStiffnessInsideRoll : armStiffness, 1.0f);

        // Setup reach arms
        m_reachLeft.offset = m_parameters.m_injury1LocalPosition;
        m_reachLeft.normal = m_parameters.m_injury1LocalNormal;
        m_reachLeft.arm = kLeftArm;
        m_reachLeft.woundPart = m_parameters.m_injury1Component;
        //m_reachLeft.twist = 0.0f;
        m_reachLeft.direction = 0.0f;
        m_reachLeft.dist2Target = 0.0f;
        m_character->reachForBodyPart(m_body, &m_reachLeft, false, cleverIKstrength, false, 0.0f, false);

        //callMaskedEffectorFunctionFloatArg(m_character, "ul", gravCompen, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);

        //m_character->leftArmIK(injury1WorldPosition);
        //m_character->leftWristIK(injury1WorldPosition, injury1WorldNormal);
        //reachedWithLeft = true;
        didSomethingWithLeftArm = true;
#if ART_ENABLE_BSPY 
        m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif

      }

      // get left arm to the side if rolling right
      if (m_downFacing && m_painState == kRollRight)
      {
        rage::Vector3 toHand = getLeftArm()->getHand()->getPosition() - getSpine()->getSpine2Part()->getPosition();
        rage::Vector3 toElbow = getLeftArm()->getElbow()->getJointPosition() - getSpine()->getSpine2Part()->getPosition();
        toHand.Normalize();
        toElbow.Normalize();
        float leftHandBehindDot = m_bodyBack.Dot(toHand);
        float leftElbowBehindDot = m_bodyBack.Dot(toElbow);
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", leftHandBehindDot);
        bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", leftElbowBehindDot);
#endif
        if (leftHandBehindDot > threshold || leftElbowBehindDot > threshold)
        {
#if ART_ENABLE_BSPY 
          m_character->setCurrentSubBehaviour("-a2sideL");
#endif
          //callMaskedEffectorFunctionFloatArg(m_character, "ul", gravCompen, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);

          getLeftArm()->setBodyStiffness(getLeftArmInput(), armOutFrontStiffness, armOutFrontStiffness / 10.0f);
          getLeftArmInputData()->getShoulder()->setDesiredAngles(shoulderLean1, shoulderLean2, shoulderTwist);
          getLeftArmInputData()->getElbow()->setDesiredAngle(elbowAngle);

          didSomethingWithLeftArm = true;
#if ART_ENABLE_BSPY 
          m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif
        }
      }

      bool didSomethingWithRightArm = false;

      // Reach with the right hand + gravity compensation
      //bool reachedWithRight = false;
      if (!m_parameters.m_dontReachWithRight && (m_reachType == kRight || m_reachType == kBoth || m_painState != kRelaxed))
      {
#if ART_ENABLE_BSPY 
        m_character->setCurrentSubBehaviour("-rfbpR");
#endif

       getRightArm()->setBodyStiffness(getRightArmInput(), m_painState == kRollRight ? armStiffnessInsideRoll : armStiffness, 1.0f);

        // Setup reach arms
        m_reachRight.offset = m_parameters.m_injury1LocalPosition;
        m_reachRight.normal = m_parameters.m_injury1LocalNormal;
        m_reachRight.arm = kRightArm;
        m_reachRight.woundPart = m_parameters.m_injury1Component;
        //m_reachRight.twist = 0.0f;
        m_reachRight.direction = 0.0f;
        m_reachRight.dist2Target = 0.0f;
        m_character->reachForBodyPart(m_body, &m_reachRight, false, cleverIKstrength, false, 0.0f, false);

        //callMaskedEffectorFunctionFloatArg(m_character, "ur", gravCompen, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
        //m_character->setBodyStiffness(m_painState == kRollRight ? armStiffnessInsideRoll : armStiffness, 1.0f, "ur");
        //m_character->rightArmIK(injury1WorldPosition);
        //m_character->rightWristIK(injury1WorldPosition, injury1WorldNormal);
        //reachedWithRight = true;
        didSomethingWithRightArm = true;
#if ART_ENABLE_BSPY 
        m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif
     }

      // get right arm to the side if rolling left
      if (m_downFacing && m_painState == kRollLeft)
      {
        rage::Vector3 toHand = getRightArm()->getHand()->getPosition() - getSpine()->getSpine2Part()->getPosition();
        rage::Vector3 toElbow = getRightArm()->getElbow()->getJointPosition() - getSpine()->getSpine2Part()->getPosition();
        toHand.Normalize();
        toElbow.Normalize();
        float rightHandBehindDot = m_bodyBack.Dot(toHand);
        float rightElbowBehindDot = m_bodyBack.Dot(toElbow);
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", rightHandBehindDot);
        bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", rightElbowBehindDot);
#endif
        if (rightHandBehindDot > threshold || rightElbowBehindDot > threshold)
        {
#if ART_ENABLE_BSPY 
          m_character->setCurrentSubBehaviour("-a2sideR");
#endif

          getRightArm()->setBodyStiffness(getRightArmInput(), armOutFrontStiffness, armOutFrontStiffness / 10.0f);
          getRightArmInputData()->getShoulder()->setDesiredAngles(shoulderLean1, shoulderLean2, shoulderTwist);
          getRightArmInputData()->getElbow()->setDesiredAngle(elbowAngle);

          didSomethingWithRightArm = true;
#if ART_ENABLE_BSPY 
          m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif
       }
      }

#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour("-brace");
#endif
      // Brace against attacker if not reaching for wound for each hand
      if (m_doBrace)
      {
#if NM_RUNTIME_LIMITS
        static float lean1wrist = 0.5236f;
        static float twistwrist = 1.2f;
#endif
        static float lean2wrist = 2.0;
        static float braceRate = 0.5f;
        static float maxReachDistance = 0.55f;
        rage::Vector3 target = m_parameters.m_attackerPos;
        static float Zadjust = 0.5f;
        target.z += Zadjust;
        rage::Vector3 toAttacker = target - getSpine()->getHeadPart()->getPosition();
        toAttacker.Normalize();
        float facingAttackerDot = toAttacker.Dot(-m_bodyBack);
        static float facingAttackerDotMin = (bracedWithLeft || bracedWithRight) ? 0.1f : 0.4f;
        if (facingAttackerDot > facingAttackerDotMin && m_painState == kRelaxed && !m_downFacing)
        {
          if (!m_ShotInLeftArm && m_onRightSide)
          {
            getLeftArm()->setOpposeGravity(getLeftArmInput(), gravCompen);
            float stiffLeft = rage::Clamp(((float)m_braceWithLeftFrames) * braceRate, 4.0f, 10.0f);
            getLeftArm()->setBodyStiffness(getLeftArmInput(), stiffLeft, stiffLeft/10.0f);
#if NM_RUNTIME_LIMITS
            (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtWrist_Left)))->setLimits(lean1wrist, lean2wrist, twistwrist);
#endif
            NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);
            NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
            ikInputData->setTarget(target);
            ikInputData->setTwist(0.0f);
            ikInputData->setDragReduction(1.0f);
            ikInputData->setMaxReachDistance(maxReachDistance);
            ikInputData->setWristTarget(target);
            ikInputData->setWristNormal(-toAttacker);
            getLeftArm()->postInput(ikInput);

            bracedWithLeft = true;
            m_braceWithLeftFrames++;
            m_wristLean2 = lean2wrist;
            didSomethingWithLeftArm = true;
          }
          else if (!m_ShotInRightArm && m_onLeftSide)
          {
            getRightArm()->setOpposeGravity(getRightArmInput(), gravCompen);
            float stiffRight = rage::Clamp(((float)m_braceWithRightFrames) * braceRate, 4.0f, 10.0f);
            getRightArm()->setBodyStiffness(getLeftArmInput(), stiffRight, stiffRight/10.0f);

#if NM_RUNTIME_LIMITS
            (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(gtaJtWrist_Right)))->setLimits(lean1wrist, lean2wrist, twistwrist);
#endif
            NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);
            NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
            ikInputData->setTarget(target);
            ikInputData->setTwist(0.0f);
            ikInputData->setDragReduction(1.0f);
            ikInputData->setMaxReachDistance(maxReachDistance);
            ikInputData->setWristTarget(target);
            ikInputData->setWristNormal(-toAttacker);
            getRightArm()->postInput(ikInput);

            bracedWithRight = true;
            m_braceWithRightFrames++;
            m_wristLean2 = lean2wrist;
            didSomethingWithRightArm = true;
          }
        }

        //NmRsCBUFlinch* flinchTask = (NmRsCBUFlinch*)m_cbuParent->m_tasks[bvid_upperBodyFlinch];
        //if (!(bracedWithRight || bracedWithLeft))
        //{
        //	if (flinchTask->isActive())
        //		flinchTask->deactivate();
        //}
        //else if (!flinchTask->isActive())
        //{
        //	// Start a flinch task
        //	flinchTask->updateBehaviourMessage(NULL); // sets values to defaults
        //	flinchTask->m_parameters.m_hand_dist_lr = 0.1;
        //	flinchTask->m_parameters.m_hand_dist_fb = 0.06;
        //	flinchTask->m_parameters.m_hand_dist_vert = 0.1;
        //	flinchTask->m_parameters.m_bodyStiffness = 11.0;
        //	flinchTask->m_parameters.m_bodyDamping = 1.0;
        //	flinchTask->m_parameters.m_backBendAmount = 0.0;
        //	flinchTask->m_parameters.m_useRight = bracedWithRight;
        //	flinchTask->m_parameters.m_useLeft = bracedWithLeft;
        //	flinchTask->m_parameters.m_noiseScale = 0.0;
        //	flinchTask->m_parameters.m_newHit = false;
        //	flinchTask->m_parameters.m_protectHeadToggle = false;
        //	flinchTask->m_parameters.m_dontBraceHead = true;
        //	flinchTask->m_parameters.m_applyStiffness = false;
        //	flinchTask->m_parameters.m_headLookAwayFromTarget = false;
        //	flinchTask->m_parameters.m_useHeadLook = false;
        //	flinchTask->m_parameters.m_turnTowards = 0; 
        //	flinchTask->m_parameters.m_pos = target;
        //	flinchTask->activate();
        //}
        //else
        //{
        //	flinchTask->m_parameters.m_pos = target;
        //	flinchTask->m_parameters.m_useRight = bracedWithRight;
        //	flinchTask->m_parameters.m_useLeft = bracedWithLeft;
        //}
      }
#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif

#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour("-default");
#endif
      // Position left arm across the chest if nothing else
      if (!didSomethingWithLeftArm)
      {
        getLeftArm()->setBodyStiffness(getLeftArmInput(), armOutFrontStiffness, armOutFrontStiffness / 10.0f);
        getLeftArmInputData()->getShoulder()->setDesiredAngles(injuredShoulderLean1, injuredShoulderLean2, injuredShoulderTwist);	
        getLeftArmInputData()->getElbow()->setDesiredAngle(injuredElbowAngle);
      }

      // Position right arm across the chest if nothing else
      if (!didSomethingWithRightArm)
      {
        getRightArm()->setBodyStiffness(getRightArmInput(), armOutFrontStiffness, armOutFrontStiffness / 10.0f);
        getRightArmInputData()->getShoulder()->setDesiredAngles(injuredShoulderLean1, injuredShoulderLean2, injuredShoulderTwist);	
        getRightArmInputData()->getElbow()->setDesiredAngle(injuredElbowAngle);
      }
#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif

#if ART_ENABLE_BSPY
      //bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", reachedWithLeft);
      //bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", reachedWithRight);
      bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", bracedWithLeft);
      bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", bracedWithRight);
      bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", didSomethingWithLeftArm);
      bspyScratchpad(m_character->getBSpyID(),"InjuredOnGround", didSomethingWithRightArm);
#endif
    }

    if (!bracedWithLeft)
      m_braceWithLeftFrames = 0;
    if (!bracedWithRight)
      m_braceWithRightFrames = 0;

    // Keep the clavicles down
    static float clavLean2 = 1.0f;

    getLeftArmInputData()->getClavicle()->setDesiredAngles(nmrsGetDesiredLean1(getLeftArm()->getClavicle()), clavLean2, nmrsGetDesiredTwist(getLeftArm()->getClavicle()));
    getRightArmInputData()->getClavicle()->setDesiredAngles(nmrsGetDesiredLean1(getRightArm()->getClavicle()), clavLean2, nmrsGetDesiredTwist(getRightArm()->getClavicle()));
  }

  void NmRsCBUInjuredOnGround::Pedal(float timeStep)
  {
    static bool doPedal = true;
    if (doPedal)
    {
      NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];

      static float minSpeed = 2.0f;
      static float maxSpeed = 2.0f;

      // Disable when rolling or when facing down
      if (m_painState != kRelaxed || m_bodyBack.Dot(m_character->m_gUp) > 0.5f)
      {
        if (pedalTask->isActive())
          pedalTask->deactivate();
      }
      else if (!pedalTask->isActive())
      {
        // Start a light pedal
        static float stiffD = 4.0f;
        pedalTask->updateBehaviourMessage(NULL); // sets values to defaults
        pedalTask->m_parameters.pedalLeftLeg = m_parameters.m_injury1Component < 1 || m_parameters.m_injury1Component > 2;
        pedalTask->m_parameters.pedalRightLeg = m_parameters.m_injury1Component < 4 || m_parameters.m_injury1Component > 5;
        float stiff = m_character->getRandom().GetRanged(stiffD, stiffD);
        pedalTask->m_parameters.legStiffness = stiff;
        pedalTask->m_parameters.backPedal = m_character->getRandom().GetRanged(0.f, 1.f) > 0.5f;
        //pedalTask->m_parameters.speedAsymmetry = m_character->getRandom().GetRanged(-10.f, 10.f);
        pedalTask->m_parameters.radiusVariance = m_character->getRandom().GetRanged(0.f, 1.f);
        pedalTask->m_parameters.legAngleVariance = m_character->getRandom().GetRanged(0.f, 1.f);
        pedalTask->m_parameters.angularSpeed = m_character->getRandom().GetRanged(minSpeed, maxSpeed);
        static float cent = 0.0f;
        pedalTask->m_parameters.centreUp = cent;
        pedalTask->m_parameters.hula = false;//m_character->getRandom().GetRanged(0.f, 1.f) > 0.7f;
        pedalTask->activate();

        // determine time until next convulsion
        m_PedalStateChangeCountdown = m_character->getRandom().GetRanged(1.f, 3.f);
      }
      else
      {
        // Switch pedal state every few seconds to convey random convulsions
        m_PedalStateChangeCountdown -= timeStep;
        if (m_PedalStateChangeCountdown < 0.f)
        {
          m_PedalStateChangeCountdown = m_character->getRandom().GetRanged(1.f, 3.f);

          if (pedalTask->isActive())
          {
            pedalTask->m_parameters.hula = false;//m_character->getRandom().GetRanged(0.f, 1.f) > 0.7f;
            pedalTask->m_parameters.backPedal = m_character->getRandom().GetRanged(0.f, 1.f) > 0.5f;
            //pedalTask->m_parameters.speedAsymmetry = m_character->getRandom().GetRanged(-10.f, 10.f);
            pedalTask->m_parameters.radiusVariance = m_character->getRandom().GetRanged(0.f, 1.f);
            pedalTask->m_parameters.legAngleVariance = m_character->getRandom().GetRanged(0.f, 1.f);
            pedalTask->m_parameters.angularSpeed = m_character->getRandom().GetRanged(minSpeed, maxSpeed);
          }
        }
      }
    }
  }

  void NmRsCBUInjuredOnGround::Foetal(float severity)
  {
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour("-foetal");
#endif
    static bool doFoetal = true;
    if (doFoetal)
    {
      //float backTwist = m_character->getRandom().GetRanged(0.0f, 1.0f);
      static float spineSeverity = 0.2f;
      getSpineInputData()->getSpine3()->setDesiredLean1(spineSeverity);
      getSpineInputData()->getSpine2()->setDesiredLean1(spineSeverity);
      getSpineInputData()->getSpine1()->setDesiredLean1(spineSeverity);
      getSpineInputData()->getLowerNeck()->setDesiredLean1(spineSeverity);

      getLeftLegInputData()->getHip()->setDesiredLean1(severity * 2.f);
      getRightLegInputData()->getHip()->setDesiredLean1(severity * 2.f);
      getLeftLegInputData()->getKnee()->setDesiredAngle(severity * -2.f);
      getRightLegInputData()->getKnee()->setDesiredAngle(severity * -2.f);

      getRightArmInputData()->getShoulder()->setDesiredTwist(severity * -0.3f);
      getLeftArmInputData()->getShoulder()->setDesiredTwist(severity * -0.3f);
      getRightArmInputData()->getShoulder()->setDesiredLean1(severity * 1.57f);
      getLeftArmInputData()->getShoulder()->setDesiredLean1(severity * 1.57f);
      getRightArmInputData()->getElbow()->setDesiredAngle(severity * 2.f);
      getLeftArmInputData()->getElbow()->setDesiredAngle(severity * 2.f);
      getLeftLegInputData()->getAnkle()->setMuscleStrength(20.f);
      getLeftLegInputData()->getAnkle()->setMuscleDamping(5.f);
      getLeftLegInputData()->getAnkle()->setDesiredLean1(severity * -1.f);
      getRightLegInputData()->getAnkle()->setMuscleStrength(20.f);
      getRightLegInputData()->getAnkle()->setMuscleDamping(5.f);
      getRightLegInputData()->getAnkle()->setDesiredLean1(severity * -1.0f);
    }
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif

  }

  void NmRsCBUInjuredOnGround::Roll(float timeStep)
  {
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour("-roll");
#endif
    // check constraints
	static float constrLengthPelivs = 0.25f;
	static float constrLengthChest = 0.3f;
    if (!m_ChestConstraint.IsValid() && getSpine()->getSpine3Part()->collidedWithEnvironment() && 
		!m_PelvisConstraint.IsValid() && getSpine()->getPelvisPart()->collidedWithEnvironment() && m_upFacing)
    {
      rage::Vector3 partPos = getSpine()->getSpine3Part()->getPosition();
      m_character->constrainPart(m_ChestConstraint, getSpine()->getSpine3Part()->getPartIndex(), 0, constrLengthChest, partPos, partPos, -1, true, true, NM_MIN_STABLE_DISTANCECONSTRAINT_DISTANCE);//has to be a distance constraint      
	  partPos = getSpine()->getPelvisPart()->getPosition();
      m_character->constrainPart(m_PelvisConstraint, getSpine()->getPelvisPart()->getPartIndex(), 0, constrLengthPelivs, partPos, partPos, -1, true, true, NM_MIN_STABLE_DISTANCECONSTRAINT_DISTANCE);//has to be a distance constraint      
    }

    // Rolling in pain?
    if (m_painState == kRelaxed)
    {
      m_body->setStiffness(3.0f, 1.0f);

      // Use a cheat force to maintain the last "on side" angle
      if (m_onLeftSide || m_onRightSide)
      {
        static float maintainSideMultiplier1 = 0.0f;
        static float maintainSideMultiplier2 = 0.0f;
        static float maintainSideMultiplier3 = 5.0f;
        static float maintainSideMultiplier4 = 10.0f;
        static float maintainSideMultiplier5 = 15.0f;
        static float maintainSideMultiplierTotal = 4.0f;
        rage::Vector3 maintianOnSideTorque;
        maintianOnSideTorque.CrossSafe(m_bodySide, m_onRightSide ? -m_character->m_gUp : m_character->m_gUp);

        // Cheat torque to roll the hips
        getSpine()->getPelvisPart()->applyTorque(maintianOnSideTorque * maintainSideMultiplier1 * maintainSideMultiplierTotal);
        getSpine()->getSpine0Part()->applyTorque(maintianOnSideTorque * maintainSideMultiplier2 * maintainSideMultiplierTotal);
        getSpine()->getSpine1Part()->applyTorque(maintianOnSideTorque * maintainSideMultiplier3 * maintainSideMultiplierTotal);
        getSpine()->getSpine2Part()->applyTorque(maintianOnSideTorque * maintainSideMultiplier4 * maintainSideMultiplierTotal);
        getSpine()->getSpine3Part()->applyTorque(maintianOnSideTorque * maintainSideMultiplier5 * maintainSideMultiplierTotal);
      }

      // Update relaxed timer
      m_relaxTimer -= timeStep;
      if (m_relaxTimer < 0.0f)
      {
        if (m_onLeftSide)
          m_painState = kRollRight;
        else if (m_onRightSide)
          m_painState = kRollLeft;
        else 
          m_painState = m_character->getRandom().GetRanged(0.0f, 1.0f) > 0.5f ? kRollRight : kRollLeft;
        static float min = 7.0f;
        static float max = 13.0f;
        m_relaxTimer = m_character->getRandom().GetRanged(min, max);
        m_rollTimer = 0.0f;
        m_ChangerHeadTarget = false;
        m_LookingAtAttacker = false;
        m_CheatStrength = 0.0f;

        // Reset whether a brace is done
        float randf = m_character->getRandom().GetRanged(0.f, 1.f);
        m_doBrace = randf <= 0.6f ? true : false;
      }
    }
    else  
    {
      // Update the roll timer
      m_rollTimer += timeStep;

      // Abort roll if stuck
      static float timeOut = 60.0f;
      if (m_rollTimer > timeOut)
        m_painState = kRelaxed;

      // Stiffen the body but keep the arms loose
      m_body->setStiffness(7.0f, 1.0f, bvmask_Full &~ (bvmask_ArmLeft | bvmask_ArmRight));
      m_body->setStiffness(5.0f, 1.0f, bvmask_ArmLeft | bvmask_ArmRight);

      // Grab the side vector
      rage::Vector3 toSide;
      toSide.Cross(m_bodyUp, m_character->m_gUp);
      toSide.Normalize();

      static bool doIK = true;

      // Magnitude of the cheat torque on the hips
      static float rate1 = 8.0f;
      static float changeCheat = 1.0f;
      float rollDot = m_bodySide.Dot(m_character->m_gUp);
      if (m_painState == kRollRight)
        rollDot = -rollDot;
      float currAngle = rage::AcosfSafe(rollDot) * 180.0f / PI;
      float currentRate = abs(currAngle - m_lastAngle);
      m_lastAngle = currAngle;
      float desiredRate = rate1 * timeStep;
      if (currentRate < desiredRate)
        m_CheatStrength += changeCheat;
      else
        m_CheatStrength -= changeCheat;
      m_CheatStrength = rage::Clamp(m_CheatStrength, 0.0f, 30.0f);


      // Magnitude of leg strength
      static float rate2 = 10.0f;
      float IKLegStrength = rage::Clamp(m_rollTimer * rate2, 0.0f, 10.0f);
      float otherLegStrength = rage::Clamp(m_rollTimer * rate2, 0.0f, 4.5f);
      static float down = 0.4f;
 
      // Magnitude of spine twist
      static float rate3 = 5.0f;
      float spineTwistMag = rage::Clamp(m_rollTimer * rate3, 0.0f, 5.0f);

      // Magnitude of spine stiffness
      static float rate4 = 10.0f;
      float spineStiffnessMag = rage::Clamp(m_rollTimer * rate4, 3.0f, 20.0f);
      getSpine()->setBodyStiffness(getSpineInput(), spineStiffnessMag,1.0f, bvmask_LowSpine);

      // Rolling strategy - push that side's ankle into the ground
      if (m_painState == kRollLeft)
      {
        // Twist the back to the left
        getSpineInputData()->getSpine0()->setDesiredTwist(spineTwistMag);
        getSpineInputData()->getSpine1()->setDesiredTwist(spineTwistMag);
        getSpineInputData()->getSpine2()->setDesiredTwist(spineTwistMag);
        getSpineInputData()->getSpine3()->setDesiredTwist(spineTwistMag);

        // Cheat torque to roll the hips
        getSpine()->getPelvisPart()->applyTorque(m_CheatStrength * m_bodyUp);
        getSpine()->getSpine0Part()->applyTorque(m_CheatStrength * m_bodyUp);
        getSpine()->getSpine1Part()->applyTorque(m_CheatStrength * m_bodyUp);
        getSpine()->getSpine2Part()->applyTorque(m_CheatStrength * m_bodyUp);

        // Push on the ground with the outside foot
        if (!m_downFacing && doIK)
        {
          float fac = 0.1f * (cos(8.0f * m_injuredOnGroundTimer)-1.0f) - down;
          if (m_onRightSide && !m_upFacing)
            fac = -0.3f;
          rage::Vector3 goalPos = getSpine()->getPelvisPart()->getPosition() + 
            toSide * -0.3f + m_bodyUp * -0.4f + m_character->m_gUp * fac;

          NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
          NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
          ikInputData->setTarget(goalPos);
          getRightLeg()->postInput(ikInput);

          getRightLeg()->setBodyStiffness(getRightLegInput(), IKLegStrength, 1.0f);
        }

        // Curl and throw the inside leg in the roll direction
        if (!m_downFacing)
        {
          getLeftLegInputData()->getHip()->setDesiredLean2(-1.0f);
          getLeftLegInputData()->getKnee()->setDesiredAngle(-2.0f);
          getLeftLeg()->setBodyStiffness(getLeftLegInput(), otherLegStrength, 1.0f);
        }

        // Check exit conditions
        float dot = m_bodySide.Dot(m_character->m_gUp);
        if (dot > 0.9f)
          m_painState = kRelaxed;
      }
      else // m_painState == kRollRight
      {
        // Twist the back to the right
        getSpineInputData()->getSpine0()->setDesiredTwist(-spineTwistMag);
        getSpineInputData()->getSpine1()->setDesiredTwist(-spineTwistMag);
        getSpineInputData()->getSpine2()->setDesiredTwist(-spineTwistMag);
        getSpineInputData()->getSpine3()->setDesiredTwist(-spineTwistMag);

        // Cheat torque to roll the hips
        getSpine()->getPelvisPart()->applyTorque(-m_CheatStrength * m_bodyUp);
        getSpine()->getSpine0Part()->applyTorque(-m_CheatStrength * m_bodyUp);
        getSpine()->getSpine1Part()->applyTorque(-m_CheatStrength * m_bodyUp);
        getSpine()->getSpine2Part()->applyTorque(-m_CheatStrength * m_bodyUp);

        // Push on the ground with the outside foot
        if (!m_downFacing && doIK)
        {
          float fac = 0.1f * (cos(8.0f * m_injuredOnGroundTimer)-1.0f) - down;
          if (m_onLeftSide && !m_upFacing)
            fac = -0.3f;
          rage::Vector3 goalPos = getSpine()->getPelvisPart()->getPosition() + 
            toSide * 0.3f + m_bodyUp * -0.4f + m_character->m_gUp * fac;

          NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
          NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
          ikInputData->setTarget(goalPos);
          getLeftLeg()->postInput(ikInput);

          getLeftLeg()->setBodyStiffness(getLeftLegInput(), IKLegStrength, 1.0f);
        }

        // Curl and throw the inside leg in the roll direction
        if (!m_downFacing)
        {
          getRightLegInputData()->getHip()->setDesiredLean2(-1.0f);
          getRightLegInputData()->getKnee()->setDesiredAngle(-2.0f);

          getLeftLeg()->setBodyStiffness(getLeftLegInput(), otherLegStrength, 1.0f);
        }

        // Check exit conditions
        float dot = m_bodySide.Dot(m_character->m_gUp);
        if (dot < -0.9f)
          m_painState = kRelaxed;
      }
    }
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif

  }

  void NmRsCBUInjuredOnGround::Breathe()
  {
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour("-breathe");
#endif
    static bool doBreath = false;
    if (!doBreath)
      return;

    static float amp = 1.0f;
    static float freq = 4.0f;
    float breathDiff = amp*0.05f*(rage::Clamp(20.f-m_injuredOnGroundTimer,10.f,20.f))*
      rage::Sinf(freq*m_injuredOnGroundTimer);

    static float neckFactor = 0.1f;
    // todo check if this is an internal or an external blend.
    getSpineInputData()->getSpine3()->setDesiredLean1(nmrsGetDesiredLean1(getSpine()->getSpine3())- 0.16f*breathDiff);
    getSpineInputData()->getSpine2()->setDesiredLean1(nmrsGetDesiredLean1(getSpine()->getSpine2())+ 0.12f*breathDiff);
    getSpineInputData()->getSpine1()->setDesiredLean1(nmrsGetDesiredLean1(getSpine()->getSpine1())- 0.06f*breathDiff);
    getSpineInputData()->getLowerNeck()->setDesiredLean1(nmrsGetDesiredLean1(getSpine()->getLowerNeck()) + neckFactor * breathDiff);

    // Only change stiffnesses when relaxing and on the ground
    if (m_painState == kRelaxed && m_character->IsOnGround())
    {
      static float stiffness = 9.0f;
      static float neckStiffness = 10.0f;
      getSpineInputData()->getSpine1()->setStiffness(stiffness, 1.0f);
      getSpineInputData()->getSpine2()->setStiffness(stiffness, 1.0f);
      getSpineInputData()->getSpine3()->setStiffness(stiffness, 1.0f);
      getSpineInputData()->getLowerNeck()->setStiffness(neckStiffness, 1.0f);
    }
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif
  }

  void NmRsCBUInjuredOnGround::GatherStateData()
  {
    // determine character orientation
    rage::Matrix34 tmCom;
    static bool useCOMorient = true;
    if (useCOMorient) 
    {
      tmCom.Set(m_character->m_COMTM);
      m_bodySide = tmCom.a;  // faces right
      m_bodyUp = tmCom.b;
      m_bodyBack = tmCom.c;
    }
    else 
    {
      getSpine()->getSpine1Part()->getBoundMatrix(&tmCom); 
      m_bodyUp = tmCom.a;
      m_bodySide = -tmCom.b;
      m_bodyBack = tmCom.c;
    }

    // determine roll state (face up, face down, right side, left side)
    m_downFacing = (m_bodyBack.Dot(m_character->m_gUp) > 0.5f);
    m_upFacing = (m_bodyBack.Dot(m_character->m_gUp) < -0.6f);
    m_onLeftSide = (m_bodySide.Dot(m_character->m_gUp) > 0.5f);
    m_onRightSide = (m_bodySide.Dot(m_character->m_gUp) < -0.5f);
  }


#if ART_ENABLE_BSPY
  void NmRsCBUInjuredOnGround::sendParameters(NmRsSpy& spy)
  {
    static const char* painStateStrings[] = 
    {
      "kRelaxed",
      "kRollLeft",
      "kRollRight"
    };
    static const char*  reachTypeStrings[] =
    {
      "kNone",
      "kLeft",
      "kRight",
      "kBoth"
    };

    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_numInjuries, true);
    bspyTaskVar(m_parameters.m_injury1Component, true);
    bspyTaskVar(m_parameters.m_injury2Component, true);

    bspyTaskVar(m_parameters.m_injury1LocalPosition, true);
    bspyTaskVar(m_parameters.m_injury2LocalPosition, true);
    bspyTaskVar(m_parameters.m_injury1LocalNormal, true);
    bspyTaskVar(m_parameters.m_injury2LocalNormal, true);
    bspyTaskVar(m_parameters.m_attackerPos, true);

    bspyTaskVar(m_parameters.m_dontReachWithLeft, true);
    bspyTaskVar(m_parameters.m_dontReachWithRight, true);
    bspyTaskVar(m_parameters.m_strongRollForce, true);

    bspyTaskVar(m_injuredOnGroundTimer, false);
    bspyTaskVar_StringEnum(m_painState, painStateStrings, false); 
    bspyTaskVar_StringEnum(m_reachType, reachTypeStrings, false); 

    bspyTaskVar(m_character->IsOnGround(), false);

    if (!m_parameters.m_dontReachWithLeft && (m_reachType == kLeft || m_reachType == kBoth || m_painState != kRelaxed))
    {
      bspyTaskVar(m_reachLeft.offset, false);
      bspyTaskVar(m_reachLeft.normal, false);
      bspyTaskVar(m_reachLeft.woundPart, false);
      //bspyTaskVar(m_reachLeft.twist, false);
      bspyTaskVar(m_reachLeft.direction, false);
    }

    if (!m_parameters.m_dontReachWithRight && (m_reachType == kRight || m_reachType == kBoth || m_painState != kRelaxed))
    {
      bspyTaskVar(m_reachRight.offset, false);
      bspyTaskVar(m_reachRight.normal, false);
      bspyTaskVar(m_reachRight.woundPart, false);
      //bspyTaskVar(m_reachRight.twist, false);
      bspyTaskVar(m_reachRight.direction, false);
    }
    bspyTaskVar(m_bodyUp, false);
    bspyTaskVar(m_bodyBack, false);
    bspyTaskVar(m_bodySide, false);


    bspyTaskVar(m_relaxTimer, false);
    bspyTaskVar(m_rollTimer, false);
    bspyTaskVar(m_PedalStateChangeCountdown, false);
    bspyTaskVar(m_wristLean2, false);
    bspyTaskVar(m_lastAngle, false);
    bspyTaskVar(m_CheatStrength, false);

    bspyTaskVar(m_braceWithLeftFrames, false);
    bspyTaskVar(m_braceWithRightFrames, false);

    bspyTaskVar(m_downFacing, false);
    bspyTaskVar(m_upFacing, false);
    bspyTaskVar(m_onLeftSide, false);
    bspyTaskVar(m_onRightSide, false);

    bspyTaskVar(m_ChangerHeadTarget, false);
    bspyTaskVar(m_LookingAtAttacker, false);
    bspyTaskVar(m_ShotInLeftArm, false);
    bspyTaskVar(m_ShotInRightArm, false);
    bspyTaskVar(m_doBrace, false);
    bspyTaskVar(m_Initialized, false);
  }
#endif // ART_ENABLE_BSPY
}
