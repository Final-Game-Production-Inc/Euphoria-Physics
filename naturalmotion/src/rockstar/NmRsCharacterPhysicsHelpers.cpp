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
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsIK.h"
#include "NmRsGenericPart.h"
#include "NmRsEffectors.h"
#include "NmRsCBU_TaskManager.h"
#include "NmRsBodyLayout.h"

#include "NmRsCBU_BalancerCollisionsReaction.h" 
#include "NmRsCBU_BraceForImpact.h" 
#include "NmRsCBU_CatchFall.h" 
#include "NmRsCBU_DynamicBalancer.h" 
#include "NmRsCBU_HighFall.h" 
#include "NmRsCBU_Shot.h" 
#include "NmRsCBU_StaggerFall.h" 

#include "physics/constraintspherical.h"
#define NM_PHYSHELPER_TORQUE_ARROWS 1

namespace ART
{

  void NmRsCharacter::applyInjuryMask(BehaviourMask mask, float injuryAmount)
  {
    callMaskedEffectorFunctionFloatArg(
      this,
      mask,
      injuryAmount,
      &NmRs1DofEffector::setInjured,
      &NmRs3DofEffector::setInjured);
  }

  void NmRsCharacter::setLeftFootConnected(bool lLegConnected)
  {
    m_posture.leftFootConnected = lLegConnected;

    getEffector(gtaJtHip_Left)->setIsPartOfGroundedChain(lLegConnected);
    getEffector(gtaJtKnee_Left)->setIsPartOfGroundedChain(lLegConnected);
    getEffector(gtaJtAnkle_Left)->setIsPartOfGroundedChain(lLegConnected);

    m_posture.leftLegAutoSet = false;
  }
  void NmRsCharacter::setRightFootConnected(bool rLegConnected)
  {
    m_posture.rightFootConnected = rLegConnected;

    getEffector(gtaJtHip_Right)->setIsPartOfGroundedChain(rLegConnected);
    getEffector(gtaJtKnee_Right)->setIsPartOfGroundedChain(rLegConnected);
    getEffector(gtaJtAnkle_Right)->setIsPartOfGroundedChain(rLegConnected);

    m_posture.rightLegAutoSet = false;
  }
  void NmRsCharacter::setLeftHandConnected(bool lArmConnected)
  {
    m_posture.leftHandConnected = lArmConnected;

    getEffector(gtaJtClav_Jnt_Left)->setIsPartOfGroundedChain(lArmConnected);
    getEffector(gtaJtShoulder_Left)->setIsPartOfGroundedChain(lArmConnected);
    getEffector(gtaJtElbow_Left)->setIsPartOfGroundedChain(lArmConnected);
    getEffector(gtaJtWrist_Left)->setIsPartOfGroundedChain(lArmConnected);

    m_posture.leftArmAutoSet = false;
  }
  void NmRsCharacter::setRightHandConnected(bool rArmConnected)
  {
    m_posture.rightHandConnected = rArmConnected;

    getEffector(gtaJtClav_Jnt_Right)->setIsPartOfGroundedChain(rArmConnected);
    getEffector(gtaJtShoulder_Right)->setIsPartOfGroundedChain(rArmConnected);
    getEffector(gtaJtElbow_Right)->setIsPartOfGroundedChain(rArmConnected);
    getEffector(gtaJtWrist_Right)->setIsPartOfGroundedChain(rArmConnected);

    m_posture.rightArmAutoSet = false;
  }


  //
  // old body functions - now private, deprecated
  //---------------------------------------------------------------------------


  void NmRsCharacter::setBodyStiffness(float stiffness, float damping, BehaviourMask mask, float *muscleStiffness)
  {
    callMaskedEffectorFunctionFloatArg(
      this,
      mask,
      stiffness*stiffness,
      &NmRs1DofEffector::setMuscleStrength,
      &NmRs3DofEffector::setMuscleStrength);

    callMaskedEffectorFunctionFloatArg(
      this,
      mask,
      2.f*stiffness*damping,
      &NmRs1DofEffector::setMuscleDamping,
      &NmRs3DofEffector::setMuscleDamping);

    if (muscleStiffness)
    {
      callMaskedEffectorFunctionFloatArg(
        this,
        mask,
        *muscleStiffness,
        &NmRs1DofEffector::setMuscleStiffness,
        &NmRs3DofEffector::setMuscleStiffness);
    }
  }

  void NmRsCharacter::getInstMatrix(int levelIndex, rage::Matrix34 *matrix)
  {
    if (!getLevel()->IsInLevel(levelIndex))
    {
      Assert(levelIndex >= -1);
      matrix->Identity();
    }
    else
    {
      rage::phInst *pInst = getLevel()->GetInstance(levelIndex);
      if (pInst)
        *matrix = RCC_MATRIX34(pInst->GetMatrix());
      else
      {
        matrix->Identity();
#ifdef NM_PHYSHELPER_DEBUG_LOGGING
        NM_RS_LOGERROR(L"getInstMatrix: invalid level index passed in, phInst == NULL");
#endif
      }
    }
  }

  bool NmRsCharacter::IsOnGround()
  {
    int numCollParts = 0;
    for (int i = 0; i<getNumberOfParts(); i++)
    {
      NmRsGenericPart* part = getGenericPartByIndex(i);
      if (part->collidedWithEnvironment())
        numCollParts++;
    }
    static int maxCollParts = 9;
    return numCollParts > maxCollParts;
  }

  // PURPOSE: Get the world velocity (velocityInWorld) at a particular world point (posInWorld) on the given object with instanceIndex.
  void NmRsCharacter::getVelocityOnInstance(int instanceIndex, rage::Vector3 &posInWorld, rage::Vector3 *velocityInWorld)
  {
    Assert(instanceIndex >= -1);
    velocityInWorld->Set(0.0f, 0.0f, 0.0f);
    if (PHLEVEL->IsInLevel(instanceIndex))
    {
      if (rage::phInst *pInst = PHLEVEL->GetInstance(instanceIndex))
      {
        rage::Vector3 vel;
        getSimulator()->GetObjectLocalVelocity(vel, *pInst, posInWorld);
        velocityInWorld->Set(vel.x, vel.y, vel.z);
      }
    }
  }

  /**
  * NmRsCharacter::resetEffectorsToDefaults(BehaviourMask mask)
  *
  * OVERVIEW
  * reset all effector desired angles / leans / twists to 0
  */
  void NmRsCharacter::resetEffectorsToDefaults(BehaviourMask mask)
  {
    if(mask == bvmask_Full)
      resetAllEffectors();
    else
    {
      callMaskedEffectorFunctionNoArgs(
        this,
        mask,
        &NmRs1DofEffector::resetEffectorCalibrations,
        &NmRs3DofEffector::resetEffectorCalibrations);

      callMaskedEffectorFunctionNoArgs(
        this,
        mask,
        &NmRs1DofEffector::resetAngles,
        &NmRs3DofEffector::resetAngles);
    }
  }

  void NmRsCharacter::instanceToWorldSpace(rage::Vector3 *vecInWorld_Out, const rage::Vector3 &vecInInstance_In, int instanceIndex)
  {
    rage::Matrix34 instanceToWorld;
    getInstMatrix(instanceIndex, &instanceToWorld);
    vecInWorld_Out->Dot(vecInInstance_In, instanceToWorld);
  }

  void NmRsCharacter::instanceToLocalSpace(rage::Vector3 *vecInInstance_Out, const rage::Vector3 &vecInWorld_In, int instanceIndex)
  {
    rage::Matrix34 worldToInstance;
    getInstMatrix(instanceIndex, &worldToInstance);
    worldToInstance.Inverse();
    vecInInstance_Out->Dot(vecInWorld_In,worldToInstance);
  }

  void NmRsCharacter::rotateInstanceToWorldSpace(rage::Vector3 *dest, rage::Vector3 &vec, int instanceIndex )
  {
    rage::Matrix34 toWorld;
    getInstMatrix(instanceIndex, &toWorld);
    toWorld.MakeTranslate(0.0f,0.0f,0.0f);
    dest->Dot(vec,toWorld);
  }

  void NmRsCharacter::boundToWorldSpace(rage::Vector3 *dest, const rage::Vector3 &vec, int instanceIndex, int boundIndex )
  {
    rage::Matrix34 toWorld;

    if ((boundIndex<1)||(instanceIndex<0))
      getInstMatrix(instanceIndex, &toWorld);
    else
    {
      findMatrixOfBound(toWorld, instanceIndex, boundIndex);
    }
    dest->Dot(vec, toWorld);
  }

  void NmRsCharacter::boundToLocalSpace(bool isDirection, rage::Vector3 *dest, const rage::Vector3 &vec, int instanceIndex, int boundIndex )
  {
    rage::Matrix34 toWorld;
    if ((boundIndex<1)||(instanceIndex<0))
      getInstMatrix(instanceIndex, &toWorld);
    else
    {
      findMatrixOfBound(toWorld, instanceIndex, boundIndex);
    }
    if (isDirection)
      toWorld.MakeTranslate(0.0f,0.0f,0.0f);
    toWorld.Inverse();
    dest->Dot(vec,toWorld);
  }

  void NmRsCharacter::rotateBoundToWorldSpace(rage::Vector3 *dest, const rage::Vector3 &vec, int instanceIndex, int boundIndex )
  {
    rage::Matrix34 toWorld;

    if ((boundIndex<1)||(instanceIndex<0))
      getInstMatrix(instanceIndex, &toWorld);
    else
    {
      findMatrixOfBound(toWorld, instanceIndex, boundIndex);
    }

    toWorld.MakeTranslate(0.0f,0.0f,0.0f);
    dest->Dot(vec,toWorld);
  }


  void NmRsCharacter::findMatrixOfBound(rage::Matrix34 &tmMat, int instanceIndex, int boundIndex)
  {
    Assert(instanceIndex > -1 && boundIndex > -1);

    rage::phInst* const pInst = getEngine()->getLevel()->GetInstance(instanceIndex);

    if (pInst)
    {
      tmMat = RCC_MATRIX34(pInst->GetMatrix());
#if HACK_GTA4 // RA: The orientation of the bounds weren't being taken into account in the transformation to world space.
      const rage::Matrix34 instMatrix = tmMat;
#endif //HACK_GTA4

      //rage::phBound* const pBound = pInst->GetArchetype()->GetBound();
      rage::phBound* const pBound = pInst->GetArchetype() ? pInst->GetArchetype()->GetBound() : NULL;//RageMP3

      if (pBound && (pBound->GetType() == rage::phBound::COMPOSITE)) 
      {
        rage::phBoundComposite* const pBoundComposite = static_cast<rage::phBoundComposite*>(pBound);

        if (boundIndex < pBoundComposite->GetNumBounds()) 
        {
			tmMat.Dot(RCC_MATRIX34(pBoundComposite->GetCurrentMatrix(boundIndex)), RCC_MATRIX34(pInst->GetMatrix()));
        }
        else 
        {
          Assert(false);
        }
      }
      else 
      {
        Assert(false);
      }
    }
  }

  /*
  void NmRsCharacter::findMatrixOfCollider(rage::Matrix34 &tmMat, int instanceIndex, int colliderIndex)
  {
  Assert(instanceIndex > -1 && colliderIndex > -1);

  rage::phInst* const pInst = getEngine()->getLevel()->GetInstance(instanceIndex);

  if (pInst){
  tmMat = pInst->GetMatrix();

  rage::phCollider* const pCol= getEngine()->getSimulator()->GetCollider(pInst);

  if (pCol && pCol->IsArticulated()){
  rage::phArticulatedCollider* const pArtCol = static_cast<rage::phArticulatedCollider*>(pCol);
  rage::phArticulatedBody* const pArtBody = pArtCol->GetBody();

  if (colliderIndex < pArtBody->GetNumBodyParts()){
  const rage::phArticulatedBodyPart& bPart = pArtBody->GetLink(colliderIndex);

  tmMat = bPart.GetMatrix();
  tmMat.Transpose();

  rage::Matrix34 baseMat = pInst->GetMatrix();
  rage::Vector3 matP = tmMat.d;
  rage::Vector3 baseMatP = baseMat.d;
  matP = matP + baseMatP;
  tmMat.d = matP;
  }
  else {
  Assert(false);
  }
  }
  else {
  Assert(false);
  }
  }
  }
  */





  /**
  * Function for calculating the actual target given the point being looked at, the
  * velocity of the point being looked at, the position of the source, the velocity
  * of the source, the angular velocity of the source, and time step.
  * Used in HeadLook and Spine Twist
  */
  rage::Vector3 NmRsCharacter::targetPosition(rage::Vector3 &targetPos, rage::Vector3 &targetVel, rage::Vector3 &sourcePos, rage::Vector3 &sourceVel, rage::Vector3 &sourceAngVel, float deltaTime)
  {
    rage::Vector3 dR;
    rage::Vector3 relVel;
    rage::Vector3 target;

    dR = targetPos - sourcePos;
    relVel.Cross(dR, sourceAngVel);
    relVel -= sourceVel;
    relVel += targetVel;
    target.AddScaled(targetPos, relVel, deltaTime);
    return target;
  }

  /**
  * Function for getting the twist(or swing) for the upper and lower neck joints from the overall twist(or swing) via a weighting
  * approach. In this function both neck joints reach their twist(or swing) limits at the same time.
  * Used in HeadLook and Head Avoid Direction
  *
  * Returns:  neckLowerAngle, neckUpperAngle
  */
  void NmRsCharacter::getNeckAngles(float *neckLowerAngle, float *neckUpperAngle, float angle, float neckLowerAngleMin, float neckLowerAngleMax, float neckUpperAngleMin, float neckUpperAngleMax)
  {
    float neckAngleMin = 1.0f / (neckLowerAngleMin + neckUpperAngleMin);
    float neckAngleMax = 1.0f / (neckLowerAngleMax + neckUpperAngleMax);

    float weightLowerAngle = rage::Selectf(angle, neckLowerAngleMax * neckAngleMax, neckLowerAngleMin * neckAngleMin);
    float weightUpperAngle = rage::Selectf(angle, neckUpperAngleMax * neckAngleMax, neckUpperAngleMin * neckAngleMin);

    *neckLowerAngle = weightLowerAngle*angle;
    *neckUpperAngle = weightUpperAngle*angle;
  }

  rage::Vector3 NmRsCharacter::BraceArm(
    NmRsHumanLimbTypes limbType,
    NmRsHumanBody* body,
    NmRs3DofEffector* wrist,
    rage::Vector3 &desiredReachTarget,// = armsBracePos - rightOffset*direction*m_leftHandSeparation/2.f;
    rage::Vector3 &desiredHandTarget,// = m_target - rightOffset*direction*m_leftHandSeparation/2.f;
    float maxArmLength,
    float armTwist
    )
  {
#if NM_RUNTIME_LIMITS
    // Allow hands to bend back farther
    static float lean1wrist = 0.51f;
    static float lean2wrist = 3.0;
    static float twistwrist = 0.91f;
    if (wrist)
      wrist->setLimits(lean1wrist, lean2wrist, twistwrist);
#else
    (void) wrist;
#endif
    Assert(limbType == kLeftArm || limbType == kRightArm);
    //NmRsArmInputWrapper* reachArmInput = body->getInput((ART::NmRsHumanLimbTypes)reachArm->arm).getData<NmRsArmInputWrapper>();
    //NmRsHumanArm* reachArmLimb = (NmRsHumanArm*)body->getLimb((ART::NmRsHumanLimbTypes) reachArm->arm);
    NmRsArmInputWrapper* armInputData = body->getInput(limbType).getData<NmRsArmInputWrapper>();
    NmRsHumanArm* arm = (NmRsHumanArm*)body->getLimb(limbType);

    // clamp arm not to reach too far
    rage::Vector3 reachTarget(desiredReachTarget);
    rage::Vector3 shoulderPos = arm->getShoulder()->getJointPosition();
    reachTarget -= shoulderPos;
    float mag = reachTarget.Mag();
    reachTarget.Normalize();
    //try to keep hands in contact by reaching a bit further.
    //if hands are not in contact don't move them towards the contact as this causes switching between contact/non contact hand positions and orientations
    float extension = 0.0f;
    float pastTargetExtension = 0.05f;
    if (arm->getHand()->collidedWithNotOwnCharacter()) 
      extension = pastTargetExtension;
    //
    reachTarget *= rage::Min(mag + extension, maxArmLength);
    reachTarget += shoulderPos;
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(),"BraceForImpact.ArmsBrace",mag);
    bspyScratchpad(getBSpyID(),"BraceForImpact.de-sink hands",extension);
    bspyScratchpad(getBSpyID(),"BraceForImpact.bracing",maxArmLength);
#endif

    float dragReduction = 1.f;
    rage::Vector3 targetVel = arm->getClaviclePart()->getLinearVelocity();
    NmRsLimbInput ikInput = body->createNmRsLimbInput<NmRsIKInputWrapper>(-1, 1.0f, bvmask_Full DEBUG_LIMBS_PARAMETER("ArmsBrace"));
    NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
    ikInputData->setTarget(reachTarget);
    ikInputData->setTwist(armTwist);
    ikInputData->setDragReduction(dragReduction);
    ikInputData->setVelocity(targetVel);
    ikInputData->setTwistIsFixed(false);
    ikInputData->setMatchClavicle(kMatchClavicleUsingTwist);

    //matchClavicleToShoulderUsingTwist only sets twist of clavicles 
    // If we don't clavicle lean1/2 here then it will use the last default arm ones
    armInputData->getClavicle()->setDesiredLean1(0.0f);
    armInputData->getClavicle()->setDesiredLean2(0.0f);

    rage::Vector3 handPos = arm->getHand()->getPosition();
    rage::Vector3 handNorm;

    bool handCollided = arm->getHand()->collidedWithEnvironment();
    if (handCollided || arm->getLowerArm()->collidedWithEnvironment())
    {
      rage::Vector3 collisionPos;
      float depth = 0.0f;

#if ART_ENABLE_BSPY
      rage::phInst *collisionInst = NULL;
      int collisionInstGenID = -1;
      if (handCollided)
        arm->getHand()->getCollisionZMPWithEnvironment(collisionPos, handNorm,&depth,&collisionInst, &collisionInstGenID);
      else
        arm->getLowerArm()->getCollisionZMPWithEnvironment(collisionPos, handNorm,&depth,&collisionInst, &collisionInstGenID);
#else
      if (handCollided)
        arm->getHand()->getCollisionZMPWithEnvironment(collisionPos, handNorm,&depth);
      else
        arm->getLowerArm()->getCollisionZMPWithEnvironment(collisionPos, handNorm,&depth);
#endif
      bool useActualAngles = true;
      float twistLimit = 1.f;
      ikInputData->setWristTarget(reachTarget);
      ikInputData->setWristNormal(handNorm);
      ikInputData->setWristUseActualAngles(useActualAngles);
      ikInputData->setWristTwistLimit(twistLimit);

      //collisionPos from contact could be used to update grab position
#if ART_ENABLE_BSPY
      bspyDrawPoint(collisionPos,0.1f,rage::Vector3(1.0f,0.0f,0.0f));
      int collisionObjectLevelIndex = 0;
      if (IsInstValid(collisionInst, collisionInstGenID))
        collisionObjectLevelIndex = collisionInst->GetLevelIndex();
      bspyScratchpad(getBSpyID(), "BraceArm", limbType == kLeftArm)
      bspyScratchpad(getBSpyID(), "BraceArm", collisionObjectLevelIndex);
#endif
    } 
    else //hand or forearm hasn't collided
    {
      rage::Vector3 shoulder2Target = desiredHandTarget - shoulderPos;
      rage::Vector3 hand2Target = desiredHandTarget - handPos;
      float hand2TargetLen = handPos.Dist(desiredHandTarget);
      bool handNotPastTarget = hand2Target.Dot(shoulder2Target) > 0.0f;
      //if hand is close enough to target but not too close and not past target then target2hand is the normal
      if (hand2TargetLen < 0.8f && hand2TargetLen > 0.05f && handNotPastTarget)
      {
        handNorm = handPos - desiredHandTarget;
        handNorm.Normalize();

        bool useActualAngles = true;
        float twistLimit = 1.f;
        ikInputData->setWristTarget(reachTarget);
        ikInputData->setWristNormal(handNorm);
        ikInputData->setWristUseActualAngles(useActualAngles);
        ikInputData->setWristTwistLimit(twistLimit);

        //blend from 0.5 full ik to 0.8 full default
        if (hand2TargetLen > 0.5f)
        {
          float blend = 1.0f - (0.8f - hand2TargetLen)/0.3f;
          NmRsLimbInput input = body->createNmRsLimbInput<NmRsArmInputWrapper>(1, blend, bvmask_Full DEBUG_LIMBS_PARAMETER("hand blend"));

          input.getData<NmRsArmInputWrapper>()->getWrist()->setDesiredAngles(0.4f, -0.4f, 1.0f);
          arm->postInput(input);
        }
      }//(hand2TargetLen < 0.8f)
      else
        // by default, characters tip hands up in preparation for connecting with object
        armInputData->getWrist()->setDesiredAngles(0.4f, -0.4f, 1.0f);

    }//hand or forearm hasn't collided

    arm->postInput(ikInput);
    return reachTarget;
  }

  void NmRsCharacter::DecideToBrace(
    float timeStep, 
    const rage::Vector3& target,
    const rage::Vector3& targetVel,
    const float braceDistance,
    const float targetPredictionTime,
    const float minBraceTime,
    const float timeToBackwardsBrace,
    float& distanceToTarget,
    float& braceTime,
    float& backwardsBraceTimer,
    bool& shouldBrace,
    bool& doBrace,
    bool* overrideBraceDecision,
    const bool allowTimedBrace)//(m_zone==toFront || m_zone==toRear)
  {
    //shouldBrace - tells you that an impact is going to occur
    //doBrace - brace with the arms.  Impact occurring:if they look good.  Impact not occurring: to not look switchy (we should probably keep the old target relative to the shoulders when this happens - we don't because it generally looks ok from all angles)
    // decide when and when not to be bracing
    rage::Vector3 chest = getSpineSetup()->getSpine2Part()->getPosition();
    rage::Vector3 dir = target - chest;
    dir.Normalize();
    float speedTowards = dir.Dot(m_COMvel - targetVel);
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "DecideToBrace.doing Before Impact", speedTowards);
#endif
    float dist = chest.Dist(target);
    float blendFac = 2.0f;
    float blend = blendFac * timeStep;

    // TDL this is one-sided smoothing. We can get close to target quickly, but smooth away
    float tempDistToTarget = rage::Max(0.1f, dist - speedTowards*targetPredictionTime);
    distanceToTarget = rage::Min(tempDistToTarget, distanceToTarget*(1.f-blend) + tempDistToTarget*blend);

#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "DecideToBrace", tempDistToTarget);
    bspyScratchpad(getBSpyID(), "DecideToBrace", dist);
#endif

    // exit a little further away. So we avoid the going in->out->in->out.... behaviour
    float extraDistWhenBracing = 0.f;
    if (doBrace)
      extraDistWhenBracing = 0.1f;


    doBrace = (distanceToTarget < (braceDistance + extraDistWhenBracing));//mmmmNote doBrace set here
    if (overrideBraceDecision)
      doBrace = *overrideBraceDecision;

    shouldBrace = doBrace;
    if (shouldBrace && braceTime < -99.0f)
      braceTime = minBraceTime;

    //Don't do a minTime brace if at the side of the car - to stop reaching when car passes the character - should be done using target moving away fast but complicated as target updates continuously to near the character
    if (!shouldBrace && braceTime > 0.0f && allowTimedBrace)
      doBrace = true;

    //Reset backwardsBraceTimer if not bracing
    //  timeToBackwardsBrace can then be used to give the impression of not bracing for targets from behind 
    //  unless you follow them round from the front.
    //  Another (better?) way to do this would be to have hysteresis in the acceptance angle in DecideBraceHands
    if (!doBrace)
      backwardsBraceTimer = 0.0f;

    backwardsBraceTimer += timeStep;
    if (backwardsBraceTimer < timeToBackwardsBrace) // TDL, if hit from behind, we only do brace after a period of time
    {
      rage::Vector3 toTarget = target - m_COM;
      levelVector(toTarget);
      float targetForwards = -m_COMTM.c.Dot(toTarget);
      if (targetForwards < 0.f)
        doBrace = false;
    }

  }

  void NmRsCharacter::DecideBraceHands(
    float timeStep,
    const rage::Vector3& target,
    const bool doBrace,
    bool& braceLeft,
    bool& braceRight,
    float& braceTime,
    float& handsDelay,
    const float handsDelayMin,
    const float handsDelayMax,
    bool& delayLeftHand,
    float& leftHandSeparation,
    float& rightHandSeparation)
  {
    //in:
    //m_parameters
    //  candidates: acceptance angle and hysteresis on angle
    bool wasReachingWithLeft = braceLeft;
    bool wasReachingWithRight = braceRight;
    bool wasNotReaching = !braceLeft && !braceRight;
    if (doBrace)
    {
      if (braceTime > 0.0f)
        braceTime -= timeStep;
      // decide on what arms to brace with
      float oneHandAngle = 0.25f; 
      float sideangle;
      rage::Matrix34 spine2Mat;
      getSpineSetup()->getSpine2Part()->getBoundMatrix(&spine2Mat); 
      rage::Vector3 bodyLeftLevelled = spine2Mat.b;
      levelVector(bodyLeftLevelled);
      rage::Vector3 shoulderToTarget;
      // left hand
      shoulderToTarget = target - getLeftArmSetup()->getShoulder()->getJointPosition();
      levelVector(shoulderToTarget);
      shoulderToTarget.Normalize();
      sideangle = rage::AcosfSafe(bodyLeftLevelled.Dot(shoulderToTarget));
      float test = (PI-oneHandAngle) - sideangle;
      if (!braceLeft && test > 0.2f && getCharacterConfiguration().m_leftHandState != CharacterConfiguration::eHS_Rifle)
        braceLeft = true;
      else if (braceLeft && test < -0.2f)//hysteresis - off condition is wider - this caould also help the character follow a target behind but not start from behind
        braceLeft = false;
#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "DecideBraceHands.do Brace. left hand", sideangle);
#endif
      // right hand
      shoulderToTarget = getRightArmSetup()->getShoulder()->getJointPosition();
      shoulderToTarget.Subtract(target,shoulderToTarget);
      levelVector(shoulderToTarget);
      shoulderToTarget.Normalize();
      sideangle = rage::AcosfSafe(bodyLeftLevelled.Dot(shoulderToTarget));

      test = sideangle - oneHandAngle;
      if (!braceRight && test > 0.2f && getCharacterConfiguration().m_rightHandState != CharacterConfiguration::eHS_Rifle)
        braceRight = true;
      else if (braceRight && test < -0.2f)
        braceRight = false;
#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "DecideBraceHands.do Brace. right hand", sideangle);
#endif
    }
    else
    {
      braceRight = false;
      braceLeft = false;
      braceTime = -100.f;
    }

    //Reset delay if wasNotReaching or one->other
    if (wasNotReaching || (wasReachingWithLeft && braceRight && !braceLeft) || (wasReachingWithRight && braceLeft && !braceRight))
      handsDelay = getRandom().GetRanged(handsDelayMin, handsDelayMax);

    if (braceRight && braceLeft) 
    {
      if (wasNotReaching)//choose nearest hand to reach with first
      {
        //Choose the hand not holding something
        //  won't be reaching already if holding a rifle so this is for holding other things that can be reached with but would rather not
        if ((getCharacterConfiguration().m_rightHandState != CharacterConfiguration::eHS_Free) && (getCharacterConfiguration().m_leftHandState == CharacterConfiguration::eHS_Free))
          delayLeftHand = false;
        else if ((getCharacterConfiguration().m_rightHandState == CharacterConfiguration::eHS_Free) && (getCharacterConfiguration().m_leftHandState != CharacterConfiguration::eHS_Free))
          delayLeftHand = true;
        else //both hands are holding something or both hands are free - choose nearest hand to reach with first
        {
          rage::Vector3 wristToTarget;
          // left hand
          wristToTarget = getLeftArmSetup()->getWrist()->getJointPosition();
          wristToTarget.Subtract(target,wristToTarget);
          float distToTarget;
          distToTarget = wristToTarget.Mag();
          // right hand
          wristToTarget = getRightArmSetup()->getWrist()->getJointPosition();
          wristToTarget.Subtract(target,wristToTarget);
          delayLeftHand = false;
          if (distToTarget > wristToTarget.Mag())//right hand closest to target
            delayLeftHand = true;
        }
      }//if (wasNotReaching)//choose nearest hand to reach with first
      //de-sink hands
      braceLeft = (handsDelay < 0 || !delayLeftHand);
      braceRight = (handsDelay < 0 || delayLeftHand);
    }//if (braceRight && braceLeft)
    if (handsDelay >= 0.f)
    {
      handsDelay = handsDelay - timeStep;
    }
    if (!doBrace)
    {
      //not bracing
      handsDelay = 0.0f;
      //Randomize hand offsets from target
      if (getBodyIdentifier() == gtaWilma /*|| getBodyIdentifier() == gtaWilmaLarge*/)
      {
        //hands together to little more than shoulder width
        leftHandSeparation = getRandom().GetRanged(0.075f, 0.23f);
        rightHandSeparation = getRandom().GetRanged(0.075f, 0.23f);
      }
      else //(getBodyIdentifier() == gtaFred || getBodyIdentifier() == rdrCowboy )
      {
        //little less than shoulder width to wide
        leftHandSeparation = getRandom().GetRanged(0.17f, 0.40f);
        rightHandSeparation = getRandom().GetRanged(0.17f, 0.40f);
      } 
    }

  }

  void NmRsCharacter::ArmsBrace(
    const rage::Vector3& target,
    const rage::Vector3& targetVel,
    const float reachAbsorbtionTime,
    const float braceDistance,
    const float armStiffness,
    const bool braceLeft,
    const bool braceRight,
    const float leftHandSeparation,
    const float rightHandSeparation,
    NmRsHumanBody* body,
    rage::Vector3& leftHandPos, 
    rage::Vector3& rightHandPos 
    )
  {
    leftHandPos.Zero();
    rightHandPos.Zero();
    rage::Vector3 armsBracePos = target;
    rage::Vector3 hipPos = body->getSpine()->getPelvisPart()->getPosition();
    rage::Vector3 toTarget = armsBracePos - hipPos;//use hipPos not chestPos to stop arms folding when character bends forward over car (making the target behind spine2)
    rage::Vector3 rightOffset;
    rightOffset.Cross(toTarget, m_gUp); // hand offset perpendicular to direction to target
    rightOffset.Normalize();

    //Work out the absorb offset
    rage::Vector3 chestPos = body->getSpine()->getSpine2Part()->getPosition();
    rage::Vector3 toTargetDir = target - chestPos;
    toTargetDir.Normalize();
    float speedTowards = toTargetDir.Dot(m_COMvel - targetVel);
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "ArmsBrace.doing Before Impact", speedTowards);
#endif
    float dist = chestPos.Dist(target);
    // this stops target going on other side of body due to the forward prediction
    float scale = rage::Clamp(speedTowards*reachAbsorbtionTime, dist-braceDistance, dist-0.3f); // /(speedTowards + 0.00001)
    rage::Vector3 absorbOffset = -toTargetDir*scale;

    armsBracePos += absorbOffset;
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(),"ArmsBrace",absorbOffset);
    bspyScratchpad(getBSpyID(),"ArmsBrace",rightOffset);
    bspyScratchpad(getBSpyID(),"ArmsBrace",armsBracePos);
#endif

    // this bit of code compensates for spine rotational velocity (just adjusts armsBracePos)
    toTarget = armsBracePos - chestPos;
    rage::Vector3 spineRotVel = body->getSpine()->getSpine2Part()->getAngularVelocity();
    dist = toTarget.Mag();
    rage::Vector3 temp;
    temp.Cross(toTarget, spineRotVel);
    temp *= 0.1f;
    toTarget += temp;
    toTarget.Normalize();
    toTarget *= dist;
    armsBracePos = chestPos + toTarget;

#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(),"ArmsBrace",armsBracePos);
#endif

    // find hand positions
    float handSeperationDirection = 1.0f;
    float armTwist = 0.f;
    float maxArmLength = 0.55f;
    //target is behind the character
    if (braceLeft && braceRight)
    {
      rage::Vector3 hipToHead = body->getSpine()->getHeadPart()->getPosition() - body->getSpine()->getPelvisPart()->getPosition();
      hipToHead.Normalize();
      rage::Matrix34 tmCom;
      body->getSpine()->getSpine2Part()->getBoundMatrix(&tmCom); 
      rage::Vector3 bodyBack;
      bodyBack.Cross(hipToHead,tmCom.b);//(up x Left)
      //NmRsCBUSpineTwist* spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
      //Assert(spineTwistTask);
      //if (spineTwistTask->doingTwist() == false)
      toTarget.Normalize();
      static float dotVal = 0.8f;
      if (bodyBack.Dot(toTarget) > dotVal)//Target is in 72deg cone behind spine (similar to can twist zone of spineTwist)
      {
        handSeperationDirection = -1.0f;
        armTwist = 1.f;
        maxArmLength = maxArmLength + 0.1f;
#if ART_ENABLE_BSPY
        bspyScratchpad(getBSpyID(),"ArmsBrace:Behind",true);
        bspyScratchpad(getBSpyID(),"ArmsBrace:Behind",bodyBack.Dot(toTarget));
#endif
      }
    }
    else
      handSeperationDirection = 0.0f;


    NmRsHumanArm *arm = body->getLeftArm();
    NmRsArmInputWrapper *armInputData = body->getLeftArmInputData();

    rage::Vector3 offsetRight = rightOffset*handSeperationDirection*leftHandSeparation/2.0f;
    rage::Vector3 desiredReachTarget;
    rage::Vector3 desiredHandTarget;
    NmRs3DofEffector* wrist = NULL;
    if (braceLeft)
    {
      arm->setBodyStiffness(body->getLeftArmInput(), armStiffness, 1.0f);
      armInputData->getElbow()->setStiffness(armStiffness, 0.75f);
      armInputData->getWrist()->setStiffness(armStiffness - 1.0f, 1.75f);

      desiredReachTarget = armsBracePos - offsetRight;
      desiredHandTarget = target - offsetRight;
#if NM_RUNTIME_LIMITS
      wrist = static_cast<NmRs3DofEffector*>(getEffectorDirect(gtaJtWrist_Left));
#endif
      leftHandPos = BraceArm(kLeftArm, body, wrist, desiredReachTarget, desiredHandTarget, maxArmLength, armTwist);

    }
    if (braceRight)
    {
      arm = body->getRightArm();
      armInputData = body->getRightArmInputData();
      arm->setBodyStiffness(body->getRightArmInput(), armStiffness, 1.0f);
      armInputData->getElbow()->setStiffness(armStiffness, 0.75f);
      armInputData->getWrist()->setStiffness(armStiffness - 1.0f, 1.75f);

      offsetRight = rightOffset*handSeperationDirection*rightHandSeparation;
      desiredReachTarget = armsBracePos + offsetRight;
      desiredHandTarget = target + offsetRight;
#if NM_RUNTIME_LIMITS
      wrist = static_cast<NmRs3DofEffector*>(getEffectorDirect(gtaJtWrist_Right));
#endif
      rightHandPos = BraceArm(kRightArm, body, wrist, desiredReachTarget, desiredHandTarget, maxArmLength, armTwist);
    }
  }


  //Reach arm to body part
  //returns armTwistOut and sets reachArm->dist2Target
  float NmRsCharacter::reachForBodyPart(
    NmRsHumanBody* body,
    ReachArm* reachArm,
    bool delayMovement,
    float cleverIKStrength,
    bool useActualAnglesForWrist,
    float bustElbowLift,
    bool bust
    )
  {
    float twistOffset = 0.0f;
    float pushIn = 0.05f;
    if (bust)
    {
      if(isPartInMask(bvmask_Spine2 | bvmask_Spine3, reachArm->woundPart))
  {
        twistOffset = bustElbowLift;//This could be less the less across the body it is, but forV we always reach across about at least to the midline (the nearest arm is inhibited > midline for spine2/spine3)
        pushIn = 0.0f;
      }
      else if(isPartInMask(bvmask_Spine1, reachArm->woundPart))
      {
        pushIn = 0.0f;
      }
    }


    float armTwistOut;

    Assert(reachArm);
    Assert(reachArm->offset.x == reachArm->offset.x);
    Assert(reachArm->normal.x == reachArm->normal.x);

#if ART_ENABLE_BSPY
    bspyLogf(info, L"REACHING");
#endif

    Assert(reachArm->arm == kLeftArm || reachArm->arm == kRightArm);
    NmRsArmInputWrapper* reachArmInput = body->getInput((ART::NmRsHumanLimbTypes)reachArm->arm).getData<NmRsArmInputWrapper>();
    NmRsHumanArm* reachArmLimb = (NmRsHumanArm*)body->getLimb((ART::NmRsHumanLimbTypes) reachArm->arm);

    NmRsGenericPart *pPart = getGenericPartByIndex(reachArm->woundPart);
    if(isPartInMask(bvmask_Pelvis | bvmask_Spine0 | bvmask_Spine1, reachArm->woundPart))
    {
      reachArmInput->getClavicle()->setDesiredLean1(reachArmLimb->getClavicle()->getMaxLean1());
      reachArmInput->getClavicle()->setDesiredLean2(reachArmLimb->getClavicle()->getMaxLean2()*0.5f);
    }
    else
    {
      reachArmInput->getClavicle()->setDesiredLean1(reachArmLimb->getClavicle()->getMaxLean1()*0.5f);
      reachArmInput->getClavicle()->setDesiredLean2(reachArmLimb->getClavicle()->getMinLean2()*0.6f);
    }

    /*
    *  get grab point in world space
    */
    rage::Matrix34 mat;
    pPart->getMatrix(mat);
    rage::Vector3 grabPoint;
    mat.Transform(reachArm->offset, grabPoint);

    //Work out a clavicle orientation
    rage::Vector3 shoulder2Grab = grabPoint - reachArmLimb->getShoulder()->getJointPosition();

    rage::Vector3 shoulder2centre = body->getSpine()->getSpine3Part()->getPosition() - reachArmLimb->getShoulder()->getJointPosition();

    shoulder2centre.Normalize();
    float clavDistanceL1 = shoulder2Grab.Dot(shoulder2centre);
    clavDistanceL1 = rage::Clamp(clavDistanceL1 - 0.1f, -0.3f, 0.6f)*3.f;

    reachArmInput->getClavicle()->setDesiredLean1(reachArmLimb->getClavicle()->getMaxLean1()*clavDistanceL1);
    reachArmInput->getClavicle()->setDesiredLean2(0.f);

    reachArmInput->getShoulder()->setOpposeGravity(1.f);
    reachArmInput->getClavicle()->setOpposeGravity(1.f);
    reachArmInput->getElbow()->setOpposeGravity(1.f);

    // ##debugDrawPoint(grabPoint, 0.2, 0,1,1)
    rage::Vector3 worldNormal;
    mat.Transform3x3(reachArm->normal, worldNormal);
    //NM_RS_CBU_DRAWVECTOR(grabPoint, worldNormal);
    //worldNormal *= reachArm->direction;
    rage::Vector3 pushNormal = /*worldNormal*/ - m_COMTM.c;//bodyBack
    pushNormal.Normalize();

#if ART_ENABLE_BSPY
    if(isPartInMask(bvmask_HandLeft, reachArmLimb->getHand()->getPartIndex()))
      bspyDrawLine(grabPoint, grabPoint+worldNormal, rage::Vector3(1,0.5,0));
    else
      bspyDrawLine(grabPoint, grabPoint+worldNormal, rage::Vector3(1,0.0f,0));
#endif
    /*
    *  compute hand target
    */
    rage::Vector3 handPos = reachArmLimb->getHand()->getPosition() - grabPoint;//hand2Grab
    //handPos -= pushNormal * rage::Max(0.f, handPos.Dot(pushNormal));//hand2grab - averageOfNormalW_and_BodyForward
    //NM_RS_CBU_DRAWPOINT(handPos + grabPoint, 0.2f, rage::Vector3(0,0,0));
    float distance = handPos.Mag();

    //jittering if use hand pos therefore use wrist + handlength/2(0.18/2) down elbow2wrist
    rage::Vector3 elbow2wrist = reachArmLimb->getWrist()->getJointPosition()-reachArmLimb->getElbow()->getJointPosition();
    elbow2wrist.Normalize();
    rage::Vector3 handPos2 = reachArmLimb->getWrist()->getJointPosition()+0.09f*elbow2wrist;
    handPos2 -= grabPoint;//hand2Grab
    //float distance2 = handPos2.Mag();
    handPos2.Normalize();
    float dot = handPos2.Dot(pushNormal);
    bool avoid = (dot < 0.0f);
    rage::Vector3 shoulder2grab = grabPoint - reachArmLimb->getShoulder()->getJointPosition(); 
    float reachDist = shoulder2grab.Mag();
    bool outOfRange = (reachDist > 0.65f);
    float normalAdd = -pushIn;//-0.05f;// 0.05 = push into wound a little
    //the distance < 0.45f is because a shot to the top of the head could not be reached with an offset
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "rfbp", avoid);
    bspyScratchpad(getBSpyID(), "rfbp", outOfRange);
    bspyScratchpad(getBSpyID(), "rfbp", distance);
    bspyScratchpad(getBSpyID(), "rfbp", dot);
#endif 
    if ((distance > 0.15f && distance < 0.45f) || (avoid && !outOfRange /*&& (distance2 > 0.1f)*/) )//distance 0.15 cleverWrist starts at 0.2
    {
      if (distance > 0.15f && distance < 0.45f)
      dot = 1.0f;
      else
        dot = rage::Abs(rage::Clamp(dot, -0.1f, 0.1f)*10.0f);//Blend when handPosition close to stop jittering
      normalAdd = rage::Min(distance*0.5f, 0.25f); 
      handPos = grabPoint + pushNormal*normalAdd*dot;
#if ART_ENABLE_BSPY
      if(isPartInMask(bvmask_HandLeft, reachArmLimb->getHand()->getPartIndex()))
        bspyDrawPoint(handPos, 0.025f, rage::Vector3(0,0.7f,0));
      else
        bspyDrawPoint(handPos, 0.025f, rage::Vector3(0.7f,0,0));
#endif 
      if (avoid)
      {
        rage::Vector3 pelvis2hip = body->getRightLeg()->getHip()->getJointPosition() - body->getSpine()->getPelvisPart()->getPosition();

        if(isPartInMask(bvmask_HandLeft, reachArmLimb->getHand()->getPartIndex()))
          pelvis2hip *=-1;
        handPos += 12.f*pelvis2hip*normalAdd*dot;
#if ART_ENABLE_BSPY
        if(isPartInMask(bvmask_HandLeft, reachArmLimb->getHand()->getPartIndex()))
          bspyDrawPoint(handPos, 0.025f, rage::Vector3(0,0.4f,0));
        else
          bspyDrawPoint(handPos, 0.025f, rage::Vector3(0.4f,0,0));
#endif 
      }
    }
    else
    {
      handPos = grabPoint + worldNormal*normalAdd;
    }
#if ART_ENABLE_BSPY
    if(isPartInMask(bvmask_HandLeft, reachArmLimb->getHand()->getPartIndex()))
      bspyDrawPoint(handPos, 0.025f, rage::Vector3(0,1,0));
    else
      bspyDrawPoint(handPos, 0.025f, rage::Vector3(1,0,0));
    bspyScratchpad(getBSpyID(), "rfbp", normalAdd);
    bspyScratchpad(getBSpyID(), "rfbp", dot);
#endif

    // [jrp] what on earth does this do?!?
    //float dir = 1.f;
    //float twistDistance = rage::Min(distance / 0.4f, 1.f);
    //rage::Vector3 spine3 = spine->getSpine3Part()->getPosition();
    //rage::Vector3 buttocks = spine->getPelvisPart()->getPosition();
    //spine3 -= buttocks;
    //float toSpine3 = spine3.Dot(grabPoint - buttocks);
    //Assert(spine3.Mag2() > 0.f);
    //toSpine3 = toSpine3 / spine3.Mag2();
    //float clampVal = rage::Clamp(toSpine3, 0.f, 0.8f);
    //NM_RS_DBG_LOGF(L"toSpine3: %.3f", toSpine3);

    //mmmmReachForWoundfloat armTwist = (0.f-1.0f*clampVal);
    //float armTwist = (1.f-2.5f*clampVal);

    shoulder2centre = body->getSpine()->getSpine2Part()->getPosition() - reachArmLimb->getShoulder()->getJointPosition();
    shoulder2centre.Normalize();

    clavDistanceL1 = shoulder2Grab.Dot(shoulder2centre);
    clavDistanceL1 = rage::Clamp(clavDistanceL1 - 0.15f, -0.3f, 0.3f)*1.5f;
    float armTwist = clavDistanceL1;
    armTwistOut = armTwist;
    //if (armTwist < 0.f)
    //  armTwist *= 1.f-twistDistance;

    if(isPartInMask(bvmask_ThighLeft | bvmask_ThighRight | bvmask_ShinLeft | bvmask_ShinRight, reachArm->woundPart))
      armTwist = 1.0f;

#if ART_ENABLE_BSPY
    if (reachArmLimb->getType() == kLeftArm)
    {
      bspyScratchpad(getBSpyID(), "rfbpL", armTwist);
    }
    else
    {
      bspyScratchpad(getBSpyID(), "rfbpR", armTwist);
    }
    bspyScratchpad(getBSpyID(), "rfbp", twistOffset);
    bspyScratchpad(getBSpyID(), "rfbp", pushIn);
#endif
    reachArm->dist2Target = (reachArmLimb->getHand()->getPosition() - handPos).Mag();

    //Stop the non-injured arm beating the injured arm to it's destination
    if (delayMovement)
      handPos = reachArmLimb->getHand()->getPosition();

    NmRsLimbInput ikInput = body->createNmRsLimbInput<NmRsIKInputWrapper>(0, 1.0f, bvmask_Full DEBUG_LIMBS_PARAMETER("reachForBodyPart"));
    NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();

    ikInputData->setTarget(handPos);
    if (reachArmLimb->getType() == kLeftArm)
      ikInputData->setTwist(armTwist + twistOffset);
    else
      ikInputData->setTwist(armTwist + twistOffset);
    ikInputData->setDragReduction(0.0f);

    //Don't start moving wrist until close to target
    const float wristIKDistance = 0.4f;//don't make too small as could be in centre of bound and keeping large makes jitter very unlikely
    if (distance < wristIKDistance)
    {
      ikInputData->setWristTarget(grabPoint);
      ikInputData->setWristNormal(worldNormal);
      ikInputData->setWristUseActualAngles(useActualAnglesForWrist);
    }
    else
      reachArmInput->getWrist()->setDesiredAngles(0.0f, 0.0f, 0.0f);

    reachArmLimb->postInput(ikInput);

    //loosen the wrist if using cleverHandIK helper forces (stops the wrist fighting with the forces causing jitter)
    //We are not using a hard constraint
    //  Note if constraints are to be used: to use spherical or distance constraint then code will have to be added to allow on the fly changing of constraint type
    if (cleverHandIK(handPos, reachArmLimb->getHand(), reachArm->direction, false, cleverIKStrength, NULL, pPart, -1, 0.1f,0, true))
    
    reachArmInput->getWrist()->setStiffness(10.f,1.f);

    return armTwistOut;
  }

  float NmRsCharacter::blendToSpecifiedPose(float oldDesiredAngle, float specifiedPoseAngle, float blendFactor)
  {
    blendFactor = rage::Clamp(blendFactor,0.f,1.f);
    float invBlendFactor = 1.0f - blendFactor;
    float newDesiredAngle = (oldDesiredAngle * invBlendFactor) + (specifiedPoseAngle * blendFactor);
    return newDesiredAngle;
  }

#if NM_STEP_UP
  //Apply cheat forces to help the character go up steps
  // Also sets friction of swing foot to zero - mmmmtodo this needs to be done just for a stepUptoStep
  void NmRsCharacter::stepUpHelp()
  {
    NmRsHumanLeg* leftLeg = getLeftLegSetup();
    NmRsHumanLeg* rightLeg = getRightLegSetup();
    NmRsHumanSpine* spine = getSpineSetup();

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuTaskManager->getTaskByID(getID(), bvid_dynamicBalancer);
    Assert(dynamicBalancerTask);

    if (dynamicBalancerTask->isActive() && (m_uprightConstraint.stepUpHelp > 0.01f) && leftLeg && rightLeg && spine && (leftLeg->getFoot()->collidedWithEnvironment() || rightLeg->getFoot()->collidedWithEnvironment()) )
    {
      if (m_probeHit[NmRsCharacter::pi_balLeft] && m_probeHit[NmRsCharacter::pi_balRight])
      {
        bool steppingUp = false;
        //bool steppingUpTo = false;
        //rage::Vector3 steppingTo = rightLeg->getFoot()->getPosition() - leftLeg->getFoot()->getPosition();;
        //rage::Vector3 comVelLevelled = m_COMvel;
        //levelVector(comVelLevelled);
        if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep) 
        {
          //will give force when say walking along a curb with one foot down one foot up.
          // perhaps steppingUp &= probe leftToProbeRight is in dir of velocity?  Stil probs with that
          steppingUp = dynamicBalancerTask->m_roPacket.m_leftFootProbeHitPos.z > dynamicBalancerTask->m_roPacket.m_rightFootProbeHitPos.z + 0.1f;              
          //steppingUpTo = leftLeg->getFoot()->getPosition().z + 0.05f < rightLeg->getFoot()->getPosition().z;              
          //levelVector(steppingTo);
          //steppingUpTo &= steppingTo.Dot(comVelLevelled) > 0.1f;
        }
        else if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
        {
          steppingUp = dynamicBalancerTask->m_roPacket.m_rightFootProbeHitPos.z > dynamicBalancerTask->m_roPacket.m_leftFootProbeHitPos.z + 0.1f;              
          //steppingUpTo = rightLeg->getFoot()->getPosition().z + 0.05f < leftLeg->getFoot()->getPosition().z;              
          //steppingTo *= -1.f;
          //levelVector(steppingTo);
          //steppingUpTo &= steppingTo.Dot(comVelLevelled) > 0.1f;
        }        
        if (steppingUp)
        {
          spine->getSpine3Part()->applyForce(m_gUp*getTotalMass()*m_uprightConstraint.stepUpHelp);
        }

        //if steppingUpTo is trigger for a particular step then apply frictionMultiplier of 0 
        //  to the whole of that step.  We reset the frictionMultiplier

        // if balancer active and  m_uprightConstraint.stepUpHelp > 0.01f and a foot has collided with the environment
        //if (steppingUpToStep)
        {
          //spine->getSpine3Part()->applyForce(m_gUp*getTotalMass()*m_uprightConstraint.torqueDamping);
          if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)
            setStepUpFriction(0.0f, 1.0f);
          if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
            setStepUpFriction(1.0f, 0.0f);
        }
#if ART_ENABLE_BSPY
        bool ID = steppingUp;
        bspyScratchpad(getBSpyID(), "ID_HelpStepUp", ID);
        //ID = steppingUpTo;
        //bspyScratchpad(getBSpyID(), "ID_HelpStepUpTo", ID);
#endif
      }
      else
      {
        setStepUpFriction(1.0f, 1.0f);
      }
    }
    else
    {
      setStepUpFriction(1.0f, 1.0f);
    }
  }
#endif//NM_STEP_UP


  void NmRsCharacter::turnTowardBullets(float deltaTime)
  {
	  // Update the desired facing direction 
	  rage::Vector3 facingDir = -m_COMTM.c;
	  facingDir.z = 0.0f;
	  facingDir.NormalizeSafe();
	  rage::Vector3 toAttacker = -m_bulletApplier[m_currentBulletApplier].GetImpulse();
	  toAttacker.z = 0.0f;
	  toAttacker.NormalizeSafe();
	  float angle = rage::AcosfSafe(toAttacker.Dot(facingDir));
	  static float rotSpeed = 10.0f;
	  rage::Vector3 vTemp;
	  vTemp.Cross(facingDir, toAttacker);
	  if (vTemp.z < 0.0f)
		  toAttacker.RotateZ(rage::Max(-rotSpeed*deltaTime, -angle));
	  else
		  toAttacker.RotateZ(rage::Min(rotSpeed*deltaTime, angle));

	  // Determine the deviations
	  rage::Vector3 toFacingController1, toFacingController2, toFacingController3;

    NmRsHumanSpine* spine = getSpineSetup();

	  Assert(spine);
	  rage::Matrix34 tmCom;
	  spine->getPelvisPart()->getMatrix(tmCom);		
	  toFacingController1.Cross(-tmCom.c, toAttacker);
	  spine->getSpine2Part()->getMatrix(tmCom);		
	  toFacingController2.Cross(-tmCom.c, toAttacker);
	  spine->getSpine3Part()->getMatrix(tmCom);		
	  toFacingController3.Cross(-tmCom.c, toAttacker);

	  // Apply corrective torques
	  static float torqueMag = 20.0f;
	  spine->getPelvisPart()->applyTorque(toFacingController1 * torqueMag);
	  spine->getSpine2Part()->applyTorque(toFacingController2 * torqueMag);
	  spine->getSpine3Part()->applyTorque(toFacingController3 * torqueMag);
  }

  void NmRsCharacter::applyLastStandUprightConstraintForces(float deltaTime)
  {
    NmRsHumanSpine* spine = getSpineSetup();
    Assert(spine);

	  // Add linear and angular controllers to the pelvis and spine2
	  if (m_uprightConstraint.m_uprightPelvisPosition.Mag2() < 0.001f)
	  {
		  // Store the current pelvis and spine2 positions
		  // These positions and the direction are updated in new bullet
		  m_uprightConstraint.m_uprightPelvisPosition = spine->getPelvisPart()->getPosition();
		  m_uprightConstraint.m_uprightSpine2Position = spine->getSpine2Part()->getPosition();
		  m_uprightConstraint.m_uprightSpine3Position = spine->getSpine3Part()->getPosition();
	  }
	  else
	  {
		  // Update and damp the horizontal desired pos
		  float lastZ0 = m_uprightConstraint.m_uprightPelvisPosition.z;
		  float lastZ1 = m_uprightConstraint.m_uprightSpine2Position.z;
		  float lastZ2 = m_uprightConstraint.m_uprightSpine3Position.z;
		  rage::Vector3 diff0 = spine->getPelvisPart()->getPosition() - m_uprightConstraint.m_uprightPelvisPosition;
		  rage::Vector3 diff1 = spine->getSpine2Part()->getPosition() - m_uprightConstraint.m_uprightSpine2Position;
		  rage::Vector3 diff2 = spine->getSpine3Part()->getPosition() - m_uprightConstraint.m_uprightSpine3Position;
		  float syncRate = 1.0f - m_uprightConstraint.lastStandHorizDamping;
		  m_uprightConstraint.m_uprightPelvisPosition += (diff0 * syncRate);
		  m_uprightConstraint.m_uprightSpine2Position += (diff1 * syncRate);
		  m_uprightConstraint.m_uprightSpine3Position += (diff2 * syncRate);

		  // Lower height based on time
		  m_uprightConstraint.m_uprightPelvisPosition.z = lastZ0 - m_uprightConstraint.lastStandSinkRate*deltaTime;
		  m_uprightConstraint.m_uprightSpine2Position.z = lastZ1 - m_uprightConstraint.lastStandSinkRate*deltaTime;
		  m_uprightConstraint.m_uprightSpine3Position.z = lastZ2 - m_uprightConstraint.lastStandSinkRate*deltaTime;

		  rage::Vector3 toSpine2Controller = m_uprightConstraint.m_uprightSpine2Position - spine->getSpine2Part()->getPosition();
		  rage::Vector3 toSpine3Controller = m_uprightConstraint.m_uprightSpine3Position - spine->getSpine3Part()->getPosition();
		  rage::Vector3 toPelvisController = m_uprightConstraint.m_uprightPelvisPosition - spine->getPelvisPart()->getPosition();

#if ART_ENABLE_BSPY
		  bspyScratchpad(getBSpyID(),"stayUprightHard", m_uprightConstraint.m_uprightSpine2Position);
		  bspyScratchpad(getBSpyID(),"stayUprightHard", m_uprightConstraint.m_uprightSpine3Position);
		  bspyScratchpad(getBSpyID(),"stayUprightHard", m_uprightConstraint.m_uprightPelvisPosition);
#endif

		  // Apply corrective linear forces
		  static float linMult = 3000.0f;
		  spine->getSpine3Part()->applyForce(toSpine3Controller * linMult);
		  spine->getSpine2Part()->applyForce(toSpine2Controller * linMult);
		  spine->getPelvisPart()->applyForce(toPelvisController * linMult);
	  }

	  return;
  }

  //#define OLD_CHEAT_FORCE
  // #define OLD_CHEAT_FORCE_NO_CHEST_FORCE
#define NEW_CHEAT_FORCE
  void NmRsCharacter::applyUprightConstraintForces(float mult, float stiffness, float damping, float feetMult, float leanReduction, float inAirShare, float min, float max)
  {
    NmRsHumanLeg* leftLeg = getLeftLegSetup();
    NmRsHumanLeg* rightLeg= getRightLegSetup();
    NmRsHumanSpine* spine = getSpineSetup();

    if (leftLeg && rightLeg && spine)
    {
      //Work out the share of the feet force the airborne foot should get(to reduce large straight legged steps)
      float leftLegForceShare = 0.5f;//share force equally between the feet
      bool leftFootInAir = !leftLeg->getFoot()->collidedWithNotOwnCharacter();
      bool rightFootInAir = !rightLeg->getFoot()->collidedWithNotOwnCharacter();
      if (leftFootInAir)
        leftLegForceShare = inAirShare;
      else if (rightFootInAir)
        leftLegForceShare = 1.f - inAirShare;

      // character strength scaling option (off by default, min=max=-1)
      if(min>0 && max>0 && max>min)
      {
        stiffness = min + (max - min)*m_strength;
      }

      rage::Vector3 lFoot = leftLeg->getFoot()->getPosition();
      rage::Vector3 lFootVel = leftLeg->getFoot()->getLinearVelocity();
      rage::Vector3 rFoot = rightLeg->getFoot()->getPosition();
      rage::Vector3 rFootVel = rightLeg->getFoot()->getLinearVelocity();

      NmRsCBUShot* shotTask = (NmRsCBUShot*)m_cbuTaskManager->getTaskByID(getID(), bvid_shot);
      Assert(shotTask);
      if (shotTask->isActive() && shotTask->getFallToKneesIsRunning())
      {
        //the upright forces aim for the front balance line between the toes - to help shot fall2Knees over balance forwards
        rage::Matrix34 toeTM;
        float footLength = 0.265f;
        float footHeight = 0.07522f;
        leftLeg->getFoot()->getBoundMatrix(&toeTM);
        lFoot -= toeTM.c*footLength*0.5f;
        lFoot -= toeTM.b*footHeight*0.5f;
        rightLeg->getFoot()->getBoundMatrix(&toeTM);
        rFoot -= toeTM.c*footLength*0.5f;
        rFoot -= toeTM.b*footHeight*0.5f;
      }

      rage::Vector3 footCentre = (lFoot + rFoot)*0.5f;
      rage::Vector3 dir;
      dir.Subtract(m_COM, footCentre);
      dir.Normalize();
      // get height of COM above feet for reducing constraint strength if character goes over
      float dUp = dir.Dot(m_gUp);
      float h = rage::Max(0.0f, dUp);
      h = 1.f - (1.f - h)*leanReduction;
      float h2 = rage::square(h);
      stiffness *= rage::square(h2);

#if defined(OLD_CHEAT_FORCE)
      rage::Vector3 footCentreVel = (lFootVel + rFootVel)*0.5f*0.5f; // TDL the second multiply means the constraint is semi-world space, trying to slow the character

      // TDL the 0.5 below just reduces the spring strength without having to reparametise all that uses this function
      rage::Vector3 force = (m_COM - footCentre)*0.5f*rage::square(stiffness) + (m_COMvel-footCentreVel)*stiffness*damping;
      force -= getUpVector() * force.Dot(getUpVector());
      force *= getLastKnownUpdateStep()*60.f;

      rage::Vector3 footImpulse;
      footImpulse = force*0.5f;
      leftLeg->getFoot()->applyImpulse(footImpulse, leftLeg->getFoot()->getPosition());
      rightLeg->getFoot()->applyImpulse(footImpulse, rightLeg->getFoot()->getPosition());

      spine->getPelvisPart()->applyImpulse(-force*0.5f, spine->getPelvisPart()->getPosition());
      rage::phArticulatedBodyPart *spine3 = spine->getSpine3()->getChildPart();
      spine3->ApplyImpulse(-force*0.5f, spine3->GetPosition());
      Assert(0); // want to know if this code is ever used as this is not a safe way to apply impulse to a part.
#else
      ((void)damping);
#endif
#if defined(OLD_CHEAT_FORCE_NO_CHEST_FORCE)
      rage::Vector3 footCentreVel = (lFootVel + rFootVel)*0.5f*0.5f; // TDL the second multiply means the constraint is semi-world space, trying to slow the character

      // TDL the 0.5 below just reduces the spring strength without having to re-parameterize all that uses this function
      rage::Vector3 force = (m_COM - footCentre)*0.5f*rage::square(stiffness) + (m_COMvel-footCentreVel)*stiffness*damping;
      force -= getUpVector() * force.Dot(getUpVector());
      force *= getLastKnownUpdateStep()*60.f;

      leftLeg->getFoot()->applyImpulse(force*0.5f, leftLeg->getFoot()->getPosition());
      rightLeg->getFoot()->applyImpulse(force*0.5f, rightLeg->getFoot()->getPosition());
      spine->getPelvisPart()->applyImpulse(-force, spine->getPelvisPart()->getPosition());
#endif
#if defined(NEW_CHEAT_FORCE)

      float strength = rage::square(stiffness);
      float legAheadTime = 0.1f;
      // this in on by default !
      if(damping < 0)
      {        
        float balanceTime = 0.3f;          
        damping *= -(balanceTime - legAheadTime)*strength;
      }
      // explicit. sort of. relative to strength.
      else
      {
        damping *= strength;
      }
      rage::Vector3 footCentreVel = (lFootVel + rFootVel)*0.5f;
      //Old way underestimated the effect of a leaned gravity vector
      //rage::Vector3 force = strength*(m_COM  + (m_COMvel - m_floorVelocity)*legAheadTime - footCentre) + damping*(m_COMvel - footCentreVel);
      //force -= getUpVector() * force.Dot(getUpVector());  // project onto horizontal plane
      //force *= getLastKnownUpdateStep()*60.f;             // normalize with respect to frame rate

      rage::Vector3 projCOM = m_COM;//COM position projected onto floor
      projCOM += m_gUp * ((footCentre - m_COM).Dot(m_gUpReal))/(m_gUp.Dot(m_gUpReal));
      rage::Vector3 force = strength*(projCOM  + (m_COMvel - m_floorVelocity)*legAheadTime - footCentre) + damping*(m_COMvel - footCentreVel);

      //level the force using the real up otherwise
      //if leaned gravity vector used gives:
      //  a downward component to the force applied to the pelvis/spine – causing the character to lose hip height
      //  an upward component to the forces at the feet causing higher steps and delaying the steping leg from footstrike
      levelVectorReal(force);
      
      force *= getLastKnownUpdateStep()*60.f;             // normalize with respect to frame rate

      leftLeg->getFoot()->applyImpulse(mult*force*leftLegForceShare*feetMult, lFoot);
      rightLeg->getFoot()->applyImpulse(mult*force*(1.f - leftLegForceShare)*feetMult, rFoot);

      //if (leftFootInAir) 
      //{
      //  leftLeg->getFoot()->applyImpulse(force*leftLegForceShare*0.5f, leftLeg->getAnkle()->getJointPosition());
      //  leftLeg->getShin()->applyImpulse(force*leftLegForceShare*0.5f, leftLeg->getKnee()->getJointPosition());
      //}
      //else
      //  leftLeg->getFoot()->applyImpulse(force*leftLegForceShare, leftLeg->getAnkle()->getJointPosition());
      //if (rightFootInAir)
      //{
      //  rightLeg->getFoot()->applyImpulse(force*(1.f - leftLegForceShare)*0.5f, rightLeg->getAnkle()->getJointPosition());
      //  rightLeg->getShin()->applyImpulse(force*(1.f - leftLegForceShare)*0.5f, rightLeg->getKnee()->getJointPosition());
      //}
      //else
      //  rightLeg->getFoot()->applyImpulse(force*(1.f - leftLegForceShare), rightLeg->getAnkle()->getJointPosition());

      Assert(m_uprightConstraint.forceSpine3Share<=1.f);
      Assert(m_uprightConstraint.forceSpine3Share>=0.f);
      spine->getPelvisPart()->applyImpulse(-force*mult*(1.f - m_uprightConstraint.forceSpine3Share), spine->getPelvisPart()->getPosition());
      spine->getSpine3Part()->applyImpulse(-force*mult*m_uprightConstraint.forceSpine3Share, spine->getSpine3Part()->getPosition());


#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "UprightConstraint", stiffness);
      bspyScratchpad(getBSpyID(), "UprightConstraint", min);
      bspyScratchpad(getBSpyID(), "UprightConstraint", max);
      bspyScratchpad(getBSpyID(), "UprightConstraint", strength);
      bspyScratchpad(getBSpyID(), "UprightConstraint", damping);
      bspyScratchpad(getBSpyID(), "UprightConstraint", force);
#endif

#endif
    }
  }

#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
  void NmRsCharacter::reEnableCollision(NmRsGenericPart* part){
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "character.reEnableCollision", part->getPartIndex());
    bspyScratchpad(getBSpyID(), "grab.reEnableCollision", part->isCollisionEnabled());
    bspyScratchpad(getBSpyID(), "grab.reEnableCollision", part->collidedWithEnvironment());
#endif

    if (!(part->isCollisionEnabled())&&!part->collidedWithEnvironment())
    {
      part->setCollisionEnabled(true);
    }
    updateReEnableCollision();
  }
  void NmRsCharacter::updateReEnableCollision()
  {
    const LeftArmSetup* leftArmSetup = getLeftArmSetup();
    const RightArmSetup* rightArmSetup = getRightArmSetup();
    m_needToCheckReenableCollisionForGrab = 
      !leftArmSetup->getLowerArm()->isCollisionEnabled() ||
      !rightArmSetup->getLowerArm()->isCollisionEnabled() || 
      !leftArmSetup->getHand()->isCollisionEnabled()||
      !rightArmSetup->getHand()->isCollisionEnabled();
  }
#endif
  /* NOT IK!!!! 
  * Applies forces to move hand towards a target and optionally creates a hard constraint when close enough
  *   If a constraint is present and useHardConstraint=false then the contraint is released
  * strength: scales forces
  */
  bool NmRsCharacter::cleverHandIK(rage::Vector3 &target,NmRsGenericPart* endPart, float direction,
    bool useHardConstraint, float strength, rage::phConstraintHandle *constraintHandle,
    NmRsGenericPart *woundPart,int instanceIndex, float threshold, int partIndex, bool distanceContraint)
  {
#if 0
    //Note if grab is changed to be able to use spherical or distance constraint then code will have to be added to allow on the fly changing of constraint type
    //If constraint type has changed and constraint exists, release it so that one of the correct type can be created
    rage::phConstraintBase* constraint = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(constraintHandle) );
    if (constraint)
    {
      //GetType() 1=distance, 2=spherical, (5=attachment, 8=fixed)
      //m_parameters.supportConstraint 0=none, 1=distance, 2=forceBased, 3=spherical
      if ((constraint->GetType() == rage::phConstraintBase::DISTANCE && !distanceConstraint) ||
        (constraint->GetType() == rage::phConstraintBase::SPHERICAL && distanceConstraint) )
        ReleaseConstraintSafely(constraintHandle);
    }
#endif

    bool applyingForce = false;
    bool existingConstraint = constraintHandle && constraintHandle->IsValid();
    //release an existing constraint if useHardConstraint=false
    if (existingConstraint && (!useHardConstraint))
      ReleaseConstraintSafely(*constraintHandle);
    existingConstraint = constraintHandle && constraintHandle->IsValid();

    // port of the cleverHandIk from: body Helpers
    rage::Vector3 handPos = endPart->getPosition();
    rage::Matrix34 handMat;
    //offset hand helper force to infront of the palm
    //gives a more stable response and possibly a helping turning moment
    endPart->getMatrix(handMat);
    rage::Vector3 handOffset = handMat.GetVector(0);

    //mmmmtodo if you find that the hand hovers above the wound point or away from grab position:
    //handDepth = 0.055 
    //so to put the palm on the rfw point use offset of 0.0277 + skin depth (bound to skin) for handPos to calculate distance and toHand.
    //and an offset of 0.04 for force application point on hand.
    handOffset.Scale(0.04f*direction);
    handPos = handPos + handOffset;
    float distance = target.Dist(handPos);

    // magnetic force pulling hands towards target
    // starts earlier than the hard constraint
    if ((distance < threshold*3.0f) && !existingConstraint)
    {
      applyingForce = true;
      float damping = 0.1f;
      float stiffness = (threshold*3.0f - distance) / (threshold*2.0f);
      stiffness = rage::Clamp(stiffness, 0.0f, 1.0f) * strength;

      rage::Vector3 toHand(target - handPos);
      toHand.Scale(stiffness*stiffness);

      rage::Vector3 bodyVel;
      bodyVel.Set(0);
      if (woundPart)
      { 
        bodyVel = woundPart->getLinearVelocity();
      }

      rage::Vector3 handVel = endPart->getLinearVelocity();
      bodyVel = bodyVel - handVel;
      bodyVel.Scale(2.0f*stiffness*damping);
      toHand = toHand + bodyVel;

      if (getLastKnownUpdateStep() < (1.0f/60.0f))//mmmmtodo why not scaled by timestep as is impulse?
      {
        toHand.Scale(getLastKnownUpdateStep()*60.0f);  // make it work for slow motion by scaling the impulse appropriately
      }

      //This used to be applied at the wrist for stability? but works better at the hand
      endPart->applyImpulse(toHand,handPos);
      toHand.Scale(-1.0f);
      if (woundPart) 
      {
        woundPart->applyImpulse(toHand,target);
      }
    }

    // when even closer, create hard constraint
    if ((!existingConstraint) && useHardConstraint && (distance < threshold)) 
    {
      Assert(constraintHandle);
      // if constraint isn't created yet, do it now
      constrainPart(*constraintHandle, endPart->getPartIndex(),partIndex, distance, handPos, target, instanceIndex, true, distanceContraint, NM_MIN_STABLE_DISTANCECONSTRAINT_DISTANCE);        
    }
    // if constraint already exists, just update it
    if (existingConstraint)
    {
      // slowly moves constraint distance from initial to 0, to safely glue hand to target
      reduceConstraintSeperation(*constraintHandle, &target, 0.6f*getLastKnownUpdateStep(), NM_MIN_STABLE_DISTANCECONSTRAINT_DISTANCE);
    }
    return applyingForce;
  }

  BehaviourMask NmRsCharacter::collidedWithWorld(BehaviourMask mask)
  {
    BehaviourMask result = 0;
    for (int i = 0; i < getNumberOfParts(); i++)
      if(isPartInMask(mask, i) && getGenericPartByIndex(i)->collidedWithNotOwnCharacter())
        result |= (1 << i);
    return result;
  }

  BehaviourMask NmRsCharacter::collidedWithOtherCharacters(BehaviourMask mask)
  {
    BehaviourMask result = 0;
    for (int i = 0; i < getNumberOfParts(); i++)
      if(isPartInMask(mask, i) && getGenericPartByIndex(i)->collidedWithOtherCharacter())
        result |= (1 << i);
    return result;
  }

  BehaviourMask NmRsCharacter::collidedWithEnvironment(BehaviourMask mask)
  {
    BehaviourMask result = 0;
    for (int i = 0; i < getNumberOfParts(); i++)
      if(isPartInMask(mask, i) && getGenericPartByIndex(i)->collidedWithEnvironment())
        result |= (1 << i);
    return result;
  }

  float NmRsCharacter::getKineticEnergyPerKilo_RelativeVelocity()
  {
    if(!m_kineticEnergyPerKiloValid)
    {
      rage::phArticulatedBody *body = getArticulatedBody();
      float totalKE = 0.f;
      float totalMass = 0.f;
      for (int i = 0; i<body->GetNumBodyParts(); i++)
      {
        totalKE += 0.5f * body->GetMass(i).Getf() * (VEC3V_TO_VECTOR3(body->GetLinearVelocityNoProp(i)) - m_floorVelocity).Mag2(); // NoProp
        totalMass += body->GetMass(i).Getf();
        rage::Vector3 inertia = VEC3V_TO_VECTOR3(body->GetAngInertia(i));
        float averageInertia = (inertia.x + inertia.y + inertia.z)*0.3333f;
        totalKE += 0.5f * averageInertia * VEC3V_TO_VECTOR3(body->GetAngularVelocityNoProp(i)).Mag2(); // NoProp
      }
      m_kineticEnergyPerKilo = totalKE / totalMass;
      m_kineticEnergyPerKiloValid = true;
    }
    Assert(m_kineticEnergyPerKilo == m_kineticEnergyPerKilo);
    return m_kineticEnergyPerKilo;
  }

  void NmRsCharacter::fluidDamping(NmRsGenericPart* part, float viscosity)
  {
    rage::Vector3 force;
    float magnitude;
    force = part->getLinearVelocity();
    magnitude = -force.Mag2();
    force.Normalize();
    force *= viscosity*magnitude;
    part->applyForce(force);
  }

  float NmRsCharacter::getTotalMass() const
  {
    return m_articulatedWrapper->getArchetype()->GetMass();
  }
#if NM_RIGID_BODY_BULLET
  float NmRsCharacter::getUpperBodyMass() const
  {
    return m_upperBodyMass;
  }
  float NmRsCharacter::getLowerBodyMass() const
  {
    return m_lowerBodyMass;
  }
#endif

  /**
  * OVERVIEW
  * Constrains a character part using either a distance constraint (distance=true) or a spherical constraint (distance = false) to:
  *  1) the world - inputlevelIndexB = -1
  *  2) to another part of the character, or part of an instance
  * The A variables are always for the character
  * componentA - character partIndex to constrain
  * componentB - character other partIndex to constrain to or partIndex of another character to constrain to or component index of another inst to constrain to 
  * partSeperation - if distance = true only - length of constraint between parts
  * worldPosA - worldPosition of the point on componentA to constrain
  * worldPosB - worldPosition of the point on componentB/inputlevelIndexB to constrain.  If spherical constraint then this is ignored and the constraint point is made at worldPosA.  
  * inputlevelIndexB - level index of inst to constrain to (can be the character's)
  * constraintHandle - container for the constraint
  * increaseArmMass - make constraints between hand and world/hand and inst more stable by increasing the arm mass and the wrist min stiffness - also sets a breakable limit
  * distance - true=distanceConstraint, false = sphericalConstraint
  * minSeparation - if distance = true only - sets a minimum constraint length
  */
  void NmRsCharacter::constrainPart(rage::phConstraintHandle &constraintHandle, int componentA, int componentB, float partSeparation, rage::Vector3 &worldPosA, rage::Vector3 &worldPosB, int inputlevelIndexB, bool increaseArmMass, bool distance, const float minSeparation)
  {
    float minSep = rage::Max(NM_MIN_STABLE_DISTANCECONSTRAINT_DISTANCE, minSeparation);
    // partSeparation <= 0.0 would cause rage to set the seperation as the distance between worldPosA and worldPosB
    // We don't let that happen
    partSeparation = rage::Max(partSeparation,minSep);

    rage::phInst *pInstToConstrainTo = NULL;
    if (inputlevelIndexB != -1)
    {
    // Extra level of checks to prevent crashes caused by trying to grab objects
    // if the instance we're trying to grab doesn't exist anymore don't attach
      if (IsInstValid_NoGenIDCheck(inputlevelIndexB))
        pInstToConstrainTo = getEngine()->getLevel()->GetInstance(inputlevelIndexB);
    else
        return;
      Assert(pInstToConstrainTo);
    }
    

    if (distance)
    {
      rage::phConstraintDistance::Params distanceConstraint;
      distanceConstraint.instanceA = getFirstInstance();
      distanceConstraint.instanceB = pInstToConstrainTo;
      distanceConstraint.componentA = (rage::u16) componentA;
      distanceConstraint.componentB = (rage::u16) componentB;
      distanceConstraint.worldAnchorA = VECTOR3_TO_VEC3V(worldPosA);
      distanceConstraint.worldAnchorB = VECTOR3_TO_VEC3V(worldPosB);
      distanceConstraint.minDistance = minSep;
      distanceConstraint.maxDistance = partSeparation;
      distanceConstraint.allowedPenetration = 0.0f;
      constraintHandle = PHCONSTRAINT->Insert(distanceConstraint);
    }
    else
    {
      rage::phConstraintSpherical::Params  sphericalConstraint;
      sphericalConstraint.instanceA = getFirstInstance();//A is always the character
      sphericalConstraint.instanceB = pInstToConstrainTo;//B can be world if inputlevelIndexB == -1, the character, or another character, or an object
      sphericalConstraint.componentA = (rage::u16) componentA;
      sphericalConstraint.componentB = (rage::u16) componentB;
      sphericalConstraint.worldPosition = VECTOR3_TO_VEC3V(worldPosA);
      constraintHandle = PHCONSTRAINT->Insert(sphericalConstraint);
    }

	  // Check if the arm mass/stiffness needs to be increased to improve the constraint stability
	  if (increaseArmMass && getFirstInstance() != pInstToConstrainTo && (componentA == gtaHand_Left || componentA == gtaHand_Right))
	  {
		  AlterArmMasses(componentA == gtaHand_Left ? kLeftHand : kRightHand, true);

		  // Set the breaking strength
		  static float breakStrength = 40.0f;
		  //mmmmtodo mmmmConstraint this will be overwritten
      //this should be the maxBreaking strength of a constraint needing extra stability.
      setConstraintBreakable(constraintHandle, true, breakStrength);
	  }
  }

  void NmRsCharacter::AlterArmMasses(NmRsHand iType, bool increase)
  {
	  // Check for valid input
	  if (!(
		  (iType == kLeftHand && increase && m_LeftArmMassIncreased == false) || 
		  (iType == kRightHand && increase && m_RightArmMassIncreased == false) ||
		  (iType == kLeftHand && !increase && m_LeftArmMassIncreased == true) ||
		  (iType == kRightHand && !increase && m_RightArmMassIncreased == true)))
	  {
		  return;
	  }

	  // Update the state
	  if (iType == kLeftHand)
		  m_LeftArmMassIncreased = increase;
	  else
		  m_RightArmMassIncreased = increase;

	  // Alter the arm mass
	  static float massMultHandMax = 5.0f;
	  static float massMultArmMax = 3.0f;
	  float massMultHand = increase ? massMultHandMax : 1.0f / massMultHandMax;
	  float massMultArm = increase ? massMultArmMax : 1.0f / massMultArmMax;
	  int component = iType == kLeftHand ? gtaHand_Left : gtaHand_Right;
	  rage::phArticulatedBody *body = getArticulatedBody();
	  float mass = body->GetMass(component).Getf();
	  body->SetMassAndAngInertia(component, massMultHand*mass, 
		  massMultHand*VEC3V_TO_VECTOR3(body->GetAngInertia(component)));
	  mass = body->GetMass(component-1).Getf();
	  body->SetMassAndAngInertia(component-1, massMultArm*mass, 
		  massMultArm*VEC3V_TO_VECTOR3(body->GetAngInertia(component-1)));
	  mass = body->GetMass(component-2).Getf();
	  body->SetMassAndAngInertia(component-2, massMultArm*mass, 
		  massMultArm*VEC3V_TO_VECTOR3(body->GetAngInertia(component-2)));
	  body->CalculateInertias();

	  // Up the min stiffness if attaching, otherwise revert to default min stiffness
	  static float minStiff = 0.8f;
	  int jointIndex = (component == gtaHand_Left) ? gtaJtWrist_Left : gtaJtWrist_Right;
	  body->GetJoint(jointIndex).SetMinStiffness(increase ? minStiff : 0.1f);
  }

  void NmRsCharacter::setConstraintBreakable(rage::phConstraintHandle &conHandle, bool breakable, float breakingStrength)
  {
    if (conHandle.IsValid())
    {
      rage::phConstraintBase* baseConstraint = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(conHandle) );
      if (baseConstraint)
        baseConstraint->SetBreakable(breakable, breakingStrength);
    }

  }

  /**
  * OVERVIEW
  * For distance constraint:
  *   Reduces the constraint length if the constraint is a  (i.e. brings WorldPosA and WorldPosB together).
  *   Sets WorldPosB to be worldPosBTarget
  *   Respects NM_MIN_STABLE_DISTANCECONSTRAINT_DISTANCE and minSeparation 
  * For spherical constraint
  *   Moves the WorldPosB closer to worldPosBTarget  
  */
  void NmRsCharacter::reduceConstraintSeperation(rage::phConstraintHandle &con, rage::Vector3* worldPosBTarget, float constraintLengthReduction, const float minSeparation)
  {
    if (con.IsValid())
    {
      rage::phConstraintBase* constraint = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(con) );
      if (constraint)
      {
        if (constraint->GetType() == rage::phConstraintBase::DISTANCE)//distance
        {
          rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( constraint );
          if (worldPosBTarget)
            distConstraint->SetWorldPosB(VECTOR3_TO_VEC3V(*worldPosBTarget));
          //if the constrained bodies are closer than the current separation then set the separation to that
          float newLength = rage::Min(VEC3V_TO_VECTOR3(distConstraint->GetWorldPosB()).Dist(VEC3V_TO_VECTOR3(distConstraint->GetWorldPosA())), 
                                          distConstraint->GetMaxDistance());
          if (newLength > constraintLengthReduction)
            newLength -= constraintLengthReduction;
          newLength = rage::Max(NM_MIN_STABLE_DISTANCECONSTRAINT_DISTANCE, newLength);
          newLength = rage::Max(minSeparation, newLength);
          distConstraint->SetMaxDistance(newLength);
        }
        if (constraint->GetType() == rage::phConstraintBase::SPHERICAL)//spherical
        {
          rage::Vector3 worldPA(0,0,0);
          rage::Vector3 worldPB(0,0,0);
          rage::phConstraintSpherical* sphericalConstraint = static_cast<rage::phConstraintSpherical*>( constraint );

          if (worldPosBTarget && constraintLengthReduction)
          {
            rage::phConstraintSpherical* sphericalConstraint = static_cast<rage::phConstraintSpherical*>( PHCONSTRAINT->GetTemporaryPointer(con) );
            rage::Vector3 newTarget = VEC3V_TO_VECTOR3(sphericalConstraint->GetWorldPosB());
            if (newTarget.Dist(*worldPosBTarget) > constraintLengthReduction)
            {
              rage::Vector3 posError = *worldPosBTarget - newTarget;
              posError.Normalize();
              posError.Scale(constraintLengthReduction);
              newTarget.Add(posError);
            }
            else
              newTarget = *worldPosBTarget;
            sphericalConstraint->SetWorldPosB(VECTOR3_TO_VEC3V(newTarget));
          }

          worldPA = VEC3V_TO_VECTOR3(sphericalConstraint->GetWorldPosA()); 
          worldPB = VEC3V_TO_VECTOR3(sphericalConstraint->GetWorldPosB()); 
        }
      }
    }
  }


  bool NmRsCharacter::probeRayNow(rayProbeIndex rayPrbeIndex, const rage::Vector3 &startPos, const rage::Vector3 &endPos, rage::u8 stateIncludeFlags, unsigned int typeFlags, unsigned int typeIncludeFlags, unsigned int typeExcludeFlags, bool includeAgent)
  {
    rage::phSegment segment;
    segment.Set(startPos, endPos);
    rage::phIntersection isect;

    m_probeHit[rayPrbeIndex] = getLevel()->TestProbe(segment, &isect, includeAgent ? NULL : getFirstInstance(), typeIncludeFlags, typeFlags, stateIncludeFlags, DEFAULT_NUM_ISECTS, typeExcludeFlags) ? true : false;

    if (m_probeHit[rayPrbeIndex])
    {
      m_probeHitPos[rayPrbeIndex] = RCC_VECTOR3(isect.GetPosition());
      m_probeHitNormal[rayPrbeIndex] = RCC_VECTOR3(isect.GetNormal());
      //m_probeHitComponent[rayPrbeIndex] = isect.GetComponent();
      m_probeHitInst[rayPrbeIndex] = isect.GetInstance();
      m_probeHitInstLevelIndex[rayPrbeIndex] = isect.GetLevelIndex();
      m_probeHitInstGenID[rayPrbeIndex] = isect.GetGenerationID();

      if (IsInstValid(rayPrbeIndex))
      {
        m_probeHitInstMass[rayPrbeIndex] = m_probeHitInst[rayPrbeIndex]->GetArchetype()->GetMass();
        m_probeHitInstBoundingBoxSize[rayPrbeIndex] = VEC3V_TO_VECTOR3(m_probeHitInst[rayPrbeIndex]->GetArchetype()->GetBound()->GetBoundingBoxSize());
      }
      else
      {
        m_probeHitInst[rayPrbeIndex] = NULL;
      }
    }
    m_probeHitLateFrames[rayPrbeIndex] = 0;
    return m_probeHit[rayPrbeIndex];
  }

  bool NmRsCharacter::probeRay(rayProbeIndex rayPrbeIndex, const rage::Vector3 &startPos, const rage::Vector3 &endPos, rage::u8 stateIncludeFlags, unsigned int typeFlags, unsigned int typeIncludeFlags, unsigned int typeExcludeFlags, bool includeAgent)
  {
#if ART_ENABLE_BSPY

    static const char* probeResultStrings[] = 
    {
#define NAME_ACTION(_name) #_name ,
      PROBE_RESULT(NAME_ACTION)
#undef NAME_ACTION
    };

#ifdef NM_RS_CBU_ASYNCH_PROBES     
    static const char* probeResultTypeStrings[] = 
    {
#define NAME_ACTION(_name) #_name ,
      PROBE_RESULT_TYPE(NAME_ACTION)
#undef NAME_ACTION
    };
#endif//#ifdef NM_RS_CBU_ASYNCH_PROBES

    static const char* probeIndexStrings[] =
    {
#define NAME_ACTION(_name) #_name ,
      PROBE_INDEX(NAME_ACTION)
#undef NAME_ACTION
    };

#endif//ART_ENABLE_BSPY

    if (rayPrbeIndex >= pi_probeCount)
    {
      Assertf(rayPrbeIndex < pi_probeCount, "RayProbeIndex is out of range");
    }
    bool hit = false;
    bool getResultsFromCollisionsOnly = false;
    NmRsCharacter::probeResult probeReslt;
    bool replacedWithImpact = false;
    bool collided = false;
#ifdef NM_RS_CBU_ASYNCH_PROBES

    NmRsCBUHighFall* highFallTask = (NmRsCBUHighFall*)m_cbuTaskManager->getTaskByID(getID(), bvid_highFall);
    Assert(highFallTask);

    // Catch Fall overrides Balancer probes.
    getResultsFromCollisionsOnly  = ((rayPrbeIndex == pi_balLeft || rayPrbeIndex == pi_balRight) && m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_catchFall));

    // High/Smart fall overrides Catch Fall 
#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
    const bool highFallActive = (highFallTask->isActive() && highFallTask->GetState() == NmRsCBUHighFall::HF_Falling) || m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_smartFall) || m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_learnedCrawl);
#else
    const bool highFallActive = (highFallTask->isActive() && highFallTask->GetState() == NmRsCBUHighFall::HF_Falling) || m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_smartFall);
#endif
    getResultsFromCollisionsOnly |= ((rayPrbeIndex == pi_balLeft || rayPrbeIndex == pi_catchFallLeft) && highFallActive);


    //NB: makes no sense to get highFall probe from impacts
    //mmmmtodo list implications of activating (and therefore initializing the probes) eg.catchFall if highFall is running.       
    bool getProbeNow = (m_is1stAsyncProbe[rayPrbeIndex] && !m_useAsyncProbe1st[rayPrbeIndex]) || rayPrbeIndex == pi_UseNonAsync;
    if (rayPrbeIndex >= pi_probeCount)
      getProbeNow = true;
    if (getProbeNow)
    {
#endif //NM_RS_CBU_ASYNCH_PROBES
      hit = probeRayNow(rayPrbeIndex, startPos, endPos, stateIncludeFlags, typeFlags, typeIncludeFlags, typeExcludeFlags, includeAgent);
      probeReslt = probe_NoHit;
      if (hit)
        probeReslt = probe_Hit;
#ifdef NM_RS_CBU_ASYNCH_PROBES
      if (rayPrbeIndex != pi_UseNonAsync)
        m_is1stAsyncProbe[rayPrbeIndex] = false;//Set to true in activate of the behaviours that use them so that 1st probe is non-async or if async the result is not requested for the 1st time
      m_probeHitNoProbeInfo[rayPrbeIndex] = 0;
      m_probeHitNoHitTime[rayPrbeIndex] = 0.f;
    }
    else
    {
      probeReslt = probe_NotRequested;
      if (!getResultsFromCollisionsOnly)
      {
        if (m_is1stAsyncProbe[rayPrbeIndex])
        {
          probeReslt = probe_1stIgnored;
          m_is1stAsyncProbe[rayPrbeIndex] = false;//Set to true in activate of the behaviours that use them so that 1st probe is non-async or if async the result is not requested for the 1st time
        }
        else
          probeReslt = GetAsynchProbeResult(rayPrbeIndex,
          &m_probeHitPos[rayPrbeIndex],
          &m_probeHitNormal[rayPrbeIndex],
          NULL, //&m_probeHitComponent[rayPrbeIndex],
          &m_probeHitInst[rayPrbeIndex],
          &m_probeHitInstLevelIndex[rayPrbeIndex],
          &m_probeHitInstGenID[rayPrbeIndex]);

        if (probeReslt == probe_Hit)
        {
          m_probeHit[rayPrbeIndex] = true;
        }
        //if probeReslt is Late or error go with last good probe hit/noHit result
        else if (probeReslt == probe_NoHit)
          m_probeHit[rayPrbeIndex] = false;
        //if probeReslt is an error or ignored should we call it not hit?
        //if probeReslt is late should we call it not hit for highFall?

      }//if (!getResultsFromCollisionsOnly)
    }//if (getProbeNow)

    static const int numLateFramesBeforeSubmitNewProbe = 1;
    static const int numFailedProbesBeforeUsingCollisions = 2;
    if (!getResultsFromCollisionsOnly)
    {
      //mmmm handle probe errors ie probeReslt > probe_Late, probe_NoIntersection should fire new probe the rest are bad big style
      if (probeReslt == probe_Late)
      {
        m_probeHitLateFrames[rayPrbeIndex] ++;
        m_probeHitNoProbeInfo[rayPrbeIndex] ++;
        m_probeHitNoHitTime[rayPrbeIndex] += getLastKnownUpdateStep();
      }
      else if (probeReslt == probe_Hit || probeReslt == probe_NoHit || probeReslt == probe_RayIndexInvalid)
      {
        m_probeHitLateFrames[rayPrbeIndex] = 0;
        m_probeHitNoProbeInfo[rayPrbeIndex] = 0;
        m_probeHitNoHitTime[rayPrbeIndex] = 0.f;
        m_probeHitResultType[rayPrbeIndex] = probeType_Probe;
      }
      else//probe error
      {
        m_probeHitNoProbeInfo[rayPrbeIndex] ++;
        m_probeHitNoHitTime[rayPrbeIndex] += getLastKnownUpdateStep();
      }

      //Submit a new probe unless we are waiting for a probe_Late
      if ((rayPrbeIndex != pi_UseNonAsync) && ((probeReslt != probe_Late) || (m_probeHitLateFrames[rayPrbeIndex] > numLateFramesBeforeSubmitNewProbe && probeReslt == probe_Late)))
      {
#if ART_ENABLE_BSPY
        if (m_probeHitLateFrames[rayPrbeIndex] > numLateFramesBeforeSubmitNewProbe && probeReslt == probe_Late)
          bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], numLateFramesBeforeSubmitNewProbe);
#endif //ART_ENABLE_BSPY
        rage::phSegment probe;
        probe.Set(startPos, endPos);
        SubmitAsynchProbe(probe.A, probe.B, rayPrbeIndex, rage::phLevelBase::STATE_FLAGS_ALL, typeFlags, m_probeTypeIncludeFlags, m_probeTypeExcludeFlags, includeAgent);
        m_probeHitLateFrames[rayPrbeIndex] = 0;
      }
    }//if (!getResultsFromCollisionsOnly)

    if (getResultsFromCollisionsOnly && m_probeHitResultType[rayPrbeIndex] == probeType_Probe)
    {
      m_probeHitResultType[rayPrbeIndex] = probeType_OldProbe;
    }
    if (m_probeHitNoProbeInfo[rayPrbeIndex] <= numFailedProbesBeforeUsingCollisions && m_probeHitNoProbeInfo[rayPrbeIndex] > 0 && m_probeHitResultType[rayPrbeIndex] == probeType_Probe && !getResultsFromCollisionsOnly)
    {
      m_probeHitResultType[rayPrbeIndex] = probeType_OldProbe;
    }
    else if ((m_probeHitNoProbeInfo[rayPrbeIndex] > numFailedProbesBeforeUsingCollisions && rayPrbeIndex != pi_highFall) || getResultsFromCollisionsOnly)//probe not good or collision only requested.  Try getting info from collisions
    {
      //mmmmtodo nohit from impact
      //will info from an impact be more suitable?

      //2 get info from lowerbody collision
      //order from least important
      float distanceToProbeStart = 10000.f;
      if (m_probeHit[rayPrbeIndex])//mmmmtodo check   mmmmtodo add old impact info even if noHit?
        distanceToProbeStart = (m_probeHitPos[rayPrbeIndex]-startPos).Mag();//mmmmtodo this could be really old information.
      if (hasCollidedWithWorld(bvmask_Full))//dunno whether this saves much time
      {
        //catchFall
        if (rayPrbeIndex == pi_catchFallLeft || rayPrbeIndex == pi_catchFallRight || rayPrbeIndex == pi_learnedCrawl)
        {
          replaceWithImpactInfo(getLeftLegSetup()->getFoot(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex], m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getLeftLegSetup()->getShin(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getLeftLegSetup()->getThigh(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getLeftArmSetup()->getClaviclePart(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getLeftArmSetup()->getUpperArm(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getLeftArmSetup()->getLowerArm(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getLeftArmSetup()->getHand(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);

          replaceWithImpactInfo(getRightLegSetup()->getFoot(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getRightLegSetup()->getShin(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getRightLegSetup()->getThigh(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getRightArmSetup()->getClaviclePart(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getRightArmSetup()->getUpperArm(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getRightArmSetup()->getLowerArm(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getRightArmSetup()->getHand(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);

          replaceWithImpactInfo(getSpineSetup()->getPelvisPart(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getSpineSetup()->getSpine0Part(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getSpineSetup()->getSpine1Part(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getSpineSetup()->getSpine2Part(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          replaceWithImpactInfo(getSpineSetup()->getSpine3Part(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
        }

        //dynamicBalancer
        if (rayPrbeIndex == pi_balLeft || rayPrbeIndex == pi_balRight)
        {
          //dynamicBalancer if catchFall is running pi_balLeft,pi_balRight
          //mmmmTodo Only do catchFall block below if running handsAndKnees catchFall 
          //  otherwise if the balancer is running and catchFall is running say just on upperBody
          //  e.g. this might happen in future braceForImpact then knees impact bumpers and we don't want to use this info
          if (m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_catchFall))//mmmmtodo may give block face info if in drape
          {
            //get info from lowerbody collisions
            replaceWithImpactInfo(getLeftLegSetup()->getFoot(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
            replaceWithImpactInfo(getLeftLegSetup()->getShin(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
            replaceWithImpactInfo(getLeftLegSetup()->getThigh(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
            replaceWithImpactInfo(getRightLegSetup()->getFoot(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
            replaceWithImpactInfo(getRightLegSetup()->getShin(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
            replaceWithImpactInfo(getRightLegSetup()->getThigh(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
            replaceWithImpactInfo(getSpineSetup()->getPelvisPart(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
          }
          else
          {
            //get info from feet collisions
            if (rayPrbeIndex == pi_balLeft)
            {
              replaceWithImpactInfo(getLeftLegSetup()->getFoot(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
              bool collidedIgnore = false;
              replaceWithImpactInfo(getRightLegSetup()->getFoot(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collidedIgnore, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
            }
            else
            {
              replaceWithImpactInfo(getRightLegSetup()->getFoot(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collided, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
              bool collidedIgnore = false;
              replaceWithImpactInfo(getLeftLegSetup()->getFoot(), startPos, &distanceToProbeStart, &m_probeHitPos[rayPrbeIndex], &m_probeHitNormal[rayPrbeIndex], &m_probeHitInst[rayPrbeIndex], &replacedWithImpact, &collidedIgnore, &m_probeHitInstLevelIndex[rayPrbeIndex], &m_probeHitInstGenID[rayPrbeIndex], &m_probeHitInstMass[rayPrbeIndex],  m_probeHitInstBoundingBoxSize[rayPrbeIndex]);
            }
          }

        }
      }
      if (collided)
        m_probeHitNoHitTime[rayPrbeIndex] = 0.f;
      if (replacedWithImpact)
      {
        //use impact
        m_probeHitResultType[rayPrbeIndex] = probeType_Impact;
        m_probeHit[rayPrbeIndex] = true;
      }
      else
      {
        //no impact suitable
        if (m_probeHitResultType[rayPrbeIndex] == probeType_Impact)
          m_probeHitResultType[rayPrbeIndex] = probeType_OldImpact;
        //else leave as is
        if (probeReslt != probe_Late && !collided)
          m_probeHitNoHitTime[rayPrbeIndex] += getLastKnownUpdateStep();
      }
      if (m_probeHitNoHitTime[rayPrbeIndex] > 0.2f)
        m_probeHit[rayPrbeIndex] = false;


    }


    if (probeReslt != probe_Hit && probeReslt != probe_NoHit && m_probeHitResultType[rayPrbeIndex] != probeType_Impact) //i.e. late or error
    {//we are using old information 
      //check that the instance hasn't been removed
      if (m_probeHit[rayPrbeIndex] && !IsInstValid(rayPrbeIndex))
      {
        m_probeHitInst[rayPrbeIndex] = NULL;
      }
    }

    //update probe information from old/current ray to current ray
    //geometry can be old but ray is current
    if (m_probeHit[rayPrbeIndex] == true)
    {
      float PAdotN = m_probeHitNormal[rayPrbeIndex].Dot(startPos - endPos);          
      float PBdotN = m_probeHitNormal[rayPrbeIndex].Dot(endPos - m_probeHitPos[rayPrbeIndex]);  
      if ((PAdotN >= 0.f && PBdotN >= 0.f) || (PAdotN <= 0.f && PBdotN <= 0.f))//both probe points above plane
        m_probeHit[rayPrbeIndex] = false;
      else //probe ray segment intersects with plane
      {
        rage::Vector3 AB = endPos;
        AB -= startPos;

        // Throw out probe hits with normals orthogonal to the probe direction
        float normDotAB = m_probeHitNormal[rayPrbeIndex].Dot(AB);
        if (rage::Abs(normDotAB) < 0.0001f)
          m_probeHit[rayPrbeIndex] = false;
        else
        {
          float u = (m_probeHitNormal[rayPrbeIndex].Dot(m_probeHitPos[rayPrbeIndex] - startPos)) / normDotAB;
          m_probeHitPos[rayPrbeIndex] = startPos + u*AB;
        }
      }
    }

#endif //NM_RS_CBU_ASYNCH_PROBES
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], startPos);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], endPos);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], probeResultStrings[probeReslt]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHit[rayPrbeIndex]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHitPos[rayPrbeIndex]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHitNormal[rayPrbeIndex]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHitInstLevelIndex[rayPrbeIndex]);
    //bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHitComponent[rayPrbeIndex]);
#ifdef NM_RS_CBU_ASYNCH_PROBES     
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], probeResultTypeStrings[m_probeHitResultType[rayPrbeIndex]]);
#endif 
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHitLateFrames[rayPrbeIndex]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHitNoHitTime[rayPrbeIndex]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHitNoProbeInfo[rayPrbeIndex]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], getResultsFromCollisionsOnly);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], collided);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], replacedWithImpact);

#endif //ART_ENABLE_BSPY
    return m_probeHit[rayPrbeIndex];
  }

  bool NmRsCharacter::IsInstValid(const rayProbeIndex rayPrbeIndex)
  {
    Assertf(rayPrbeIndex < pi_probeCount && rayPrbeIndex >= 0, "NmRsCharacter::IsInstValid. index is %d", rayPrbeIndex);
    return (rayPrbeIndex < pi_probeCount && rayPrbeIndex >= 0 && 
      PHLEVEL->IsInLevel(m_probeHitInstLevelIndex[rayPrbeIndex]) && 
      IsInstValid(m_probeHitInst[rayPrbeIndex], m_probeHitInstLevelIndex[rayPrbeIndex],m_probeHitInstGenID[rayPrbeIndex]));
  }

  bool NmRsCharacter::IsInstValid(const rage::phInst *inst, const int instLevelIndex, const int instGenID)
  {
    return (PHLEVEL->IsInLevel(instLevelIndex) && inst &&
      PHLEVEL->IsLevelIndexGenerationIDCurrent(instLevelIndex, instGenID) &&
      inst->GetArchetype() && inst->GetArchetype()->GetBound());
  }

  bool NmRsCharacter::IsInstValid(const rage::phInst *inst, const int instGenID)
  {
    return (inst && PHLEVEL->IsInLevel(inst->GetLevelIndex()) &&
      PHLEVEL->IsLevelIndexGenerationIDCurrent(inst->GetLevelIndex(), instGenID) &&
      inst->GetArchetype() && inst->GetArchetype()->GetBound());
  }

  bool NmRsCharacter::IsInstValid(const int levelIndex, const int instGenID)
  {
    if (PHLEVEL->IsInLevel(levelIndex))
    {
      rage::phInst* pInst = NULL;
      pInst = PHLEVEL->GetInstance(levelIndex);
      return (pInst && pInst->GetArchetype() && pInst->GetArchetype()->GetBound() && PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndex, instGenID));
    }
    else
      return false;
  }

  bool NmRsCharacter::IsInstValid_NoGenIDCheck(const rage::phInst *inst)
  {
    return (inst && PHLEVEL->IsInLevel(inst->GetLevelIndex()) &&
      inst->GetArchetype() && inst->GetArchetype()->GetBound());
  }

  bool NmRsCharacter::IsInstValid_NoGenIDCheck(const int levelIndex)
  {
    if (PHLEVEL->IsInLevel(levelIndex))
    {
      rage::phInst* pInst = NULL;
      pInst = PHLEVEL->GetInstance(levelIndex);
      return (pInst && pInst->GetLevelIndex()==levelIndex && pInst->GetArchetype() && pInst->GetArchetype()->GetBound());
    }
    else
      return false;
  }


#ifdef NM_RS_CBU_ASYNCH_PROBES
  NmRsCharacter::asyncProbeIndex	NmRsCharacter::rayIndexToProbeIndex(rayProbeIndex rayPrbeIndex)
  {
    switch (rayPrbeIndex)
    {
    case pi_balLeft:
    case pi_catchFallLeft:
    case pi_highFall:
      return api_One;
    case pi_balRight:
    case pi_catchFallRight:
    case pi_learnedCrawl:
      return api_Two;
    case pi_highFallDown:
    case pi_LightWeightAsync:
      return api_Three;
    default:
    case pi_probeCount:
      //case pi_UseNonAsync:
      AssertMsg(false,"Unmapped rayIndexToProbeIndex");
      return api_Two; //Will be wrong but won't crash
    } 

  }

  /*
  Replaces collisionPos, collisionNormal, collisionInst, distanceToProbeStart, with information from the part collision information
  if there is a collision with that part and that collisionPos is closer to probeStart than the distanceToProbeStart.

  Caution: replacedWithImpact is either unchanged or changed to true.  This is so you can use this function one after the other
  without changing the input arguments and know whether at least one of them has replacedWithImpactInfo.  
  It does however mean that initially you have to set replacedWithImpact to false.

  Returns whether it has replaced the input vectors with collision information.
  */
  bool NmRsCharacter::replaceWithImpactInfo(NmRsGenericPart *part, const rage::Vector3& probeStart, float *distanceToProbeStart, 
    rage::Vector3 *collisionPos, rage::Vector3 *collisionNormal, const rage::phInst **collisionInst, bool *replacedWithImpact, bool *collided,
    int *probeHitInstLevelIndex, int *probeHitInstGenID, float *probeHitInstMass, rage::Vector3& probeHitInstBoundingBoxSize)
  {
    if (part->collidedWithNotOwnCharacter())
    {
      *collided = true;
      rage::Vector3 tempCollisionPos;
      rage::Vector3 tempCollisionNormal;
      float depth = 0;
      rage::phInst *tempCollisionInst = NULL;
      int tempCollisionInstGenID = -1;
      part->getCollisionZMPWithNotOwnCharacter(tempCollisionPos, tempCollisionNormal,&depth,&tempCollisionInst, &tempCollisionInstGenID);
      float tempDistanceToProbeStart = (tempCollisionPos-probeStart).Mag();
      if ( tempDistanceToProbeStart < *distanceToProbeStart)
      {
        *collisionPos = tempCollisionPos;
        *collisionNormal = tempCollisionNormal;
        *collisionInst = tempCollisionInst;
        *distanceToProbeStart = tempDistanceToProbeStart;
        *replacedWithImpact = true;
        *probeHitInstGenID = tempCollisionInstGenID;
        //if tempCollisionInst is no longer valid we still use the Pos/Normal info which is in global space
        //We update the pos later using the returned surface normal
        //Should we do this though as it means the inst has been removed from the scene?
        //Also should we check the input collisionInst aswell?
        if (*collisionInst)
          *probeHitInstLevelIndex = (*collisionInst)->GetLevelIndex();
        if (IsInstValid(*collisionInst, *probeHitInstLevelIndex, *probeHitInstGenID))  // make another check inst func that extracts level index and genID to use with tempCollisionInst 
        {
          *probeHitInstMass = (*collisionInst)->GetArchetype()->GetMass();
          probeHitInstBoundingBoxSize = VEC3V_TO_VECTOR3((*collisionInst)->GetArchetype()->GetBound()->GetBoundingBoxSize());
        }
        else
        {
          *collisionInst = NULL;
        }

        return true;
      }
    }
    return false;
  }

  void NmRsCharacter::ClearAsynchProbe(rayProbeIndex rayPrbeIndex)
  {
    if (rayPrbeIndex == pi_UseNonAsync)//for crawl learning, landing
      return;

    asyncProbeIndex  index = rayIndexToProbeIndex(rayPrbeIndex);
    rage::phAsyncShapeTestMgr* mgr = GetAsyncShapeTestMgr();
    if (!mgr) return;
    if (m_AsyncProbeHandles[index] != rage::phAsyncShapeTestMgr::InvalidShapeTestHandle)
    {
      if (mgr->New_IsInUse(m_AsyncProbeHandles[index]))
      {
        mgr->New_ReleaseProbe(m_AsyncProbeHandles[index]);
      }
    }
    m_AsyncProbeHandles[index] = rage::phAsyncShapeTestMgr::InvalidShapeTestHandle;
  }

  //To be safe this function should be called when any behaviour using probes is deactivated
  // and should be called after the behaviours active variable has been set to false
  void NmRsCharacter::ClearAsynchProbe_IfNotInUse(rayProbeIndex rayPrbeIndex)
  {
    //Using the logic from probeRay:
    //getResultsFromCollisionsOnly  = ((rayPrbeIndex == pi_balLeft || rayPrbeIndex == pi_balRight) && m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_catchFall));
    //getResultsFromCollisionsOnly = getResultsFromCollisionsOnly || 
    //	((rayPrbeIndex == pi_balLeft || rayPrbeIndex == pi_catchFallLeft)  && ((highFallTask->isActive() && highFallTask->GetState() == NmRsCBUHighFall::HF_Falling) || m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_learnedCrawl)));       
    //
    //Clear the rayProbeIndex only if the probeIndex this maps to isn't being used.
    //NB. clear e.g. the  pi_catchFallRight probe even if pi_balRight is being used (they are the same probeIndex) otherwise pi_balRight will access the info for pi_catchFallRight next step.  
    NmRsCBUHighFall* highFallTask = (NmRsCBUHighFall*)m_cbuTaskManager->getTaskByID(getID(), bvid_highFall);
    Assert(highFallTask);

    bool dontClearProbe = ((rayPrbeIndex == pi_balLeft || rayPrbeIndex == pi_balRight) && m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_catchFall));
    dontClearProbe = dontClearProbe || 
      ((rayPrbeIndex == pi_balLeft || rayPrbeIndex == pi_catchFallLeft)  && ((highFallTask->isActive() && highFallTask->GetState() == NmRsCBUHighFall::HF_Falling)
#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
	  || m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_learnedCrawl)
#endif
	  ));

    if (!dontClearProbe)
      ClearAsynchProbe(rayPrbeIndex);

  }

  //////////////////////////////////////////////////////////////////////////

  void NmRsCharacter::ClearAllProbes()
  {
    for (int i=0; i<pi_probeCount; i++) 
      ClearAsynchProbe((rayProbeIndex) i);
  }

  //Initializes the asynch probe called by the rayProbeIndex
  void NmRsCharacter::InitializeProbe(rayProbeIndex rayPrbeIndex, bool useAsyncProbe1st)
  {
    ClearAsynchProbe(rayPrbeIndex);
    ResetRayProbeIndex(rayPrbeIndex, useAsyncProbe1st);
  }

  //Resets only the RayProbeIndex information without initializing the asynch probe (another rayProbeIndex may still be able to use the last bit of info in the asynch probe)
  void NmRsCharacter::ResetRayProbeIndex(rayProbeIndex rayPrbeIndex, bool useAsyncProbe1st)
  {
    m_useAsyncProbe1st[rayPrbeIndex] = useAsyncProbe1st;
    m_is1stAsyncProbe[rayPrbeIndex] = true;
    m_probeHit[rayPrbeIndex] = false;
    m_probeHitInst[rayPrbeIndex] = NULL;
    m_probeHitLateFrames[rayPrbeIndex] = 0;
    m_probeHitNoProbeInfo[rayPrbeIndex] = 0;
    m_probeHitNoHitTime[rayPrbeIndex] = 0.f;
    m_probeHitResultType[rayPrbeIndex] = probeType_None;
    m_probeHitInstLevelIndex[rayPrbeIndex] = -1;
    m_probeHitInstGenID[rayPrbeIndex] = -1;
  }

  NmRsCharacter::probeResult NmRsCharacter::GetAsynchProbeResult(rayProbeIndex rayPrbeIndex, 
    rage::Vector3* probeHitPos, rage::Vector3* probeHitNormal, int* probeHitComponent,
    const rage::phInst** probeHitInst, int* probeHitInstLevelIndex, int* probeHitInstGenID) 
  {
    asyncProbeIndex  index = rayIndexToProbeIndex(rayPrbeIndex);
#if ART_ENABLE_BSPY & 0
    bspyScratchpad(getBSpyID(), "GetAsynchProbeResult", rayPrbeIndex);
    bspyScratchpad(getBSpyID(), "GetAsynchProbeResult", index);
    bspyScratchpad(getBSpyID(), "GetAsynchProbeResult", m_currentFrame);
    bspyScratchpad(getBSpyID(), "GetAsynchProbeResult", m_AsyncProbeSubmitFrame[index]);
#endif
    //If the probe was called using a different RayIndex/behaviour than the one trying to access the result then return
    if (m_AsyncProbeRayProbeIndex[index] != rayPrbeIndex)
      return probe_RayIndexInvalid;
    if (m_AsyncProbeHandles[index] != rage::phAsyncShapeTestMgr::InvalidShapeTestHandle)
    {
      rage::phAsyncShapeTestMgr* mgr = GetAsyncShapeTestMgr();
      if (!mgr) 
      {
        return probe_NoManager;
      }
      if (mgr->New_IsInUse(m_AsyncProbeHandles[index]))
      {
        if (mgr->New_IsResultReady(m_AsyncProbeHandles[index]))
        {
          const rage::phIntersection* intersection = mgr->New_GetDestIsect(m_AsyncProbeHandles[index]);
          if (intersection)//1
          {
            intersection = intersection->IsAHit() ? intersection: NULL;
            if (intersection)//2
            {
              if (probeHitPos)
                 *probeHitPos = RCC_VECTOR3(intersection->GetPosition());
              if (probeHitNormal)
                *probeHitNormal = RCC_VECTOR3(intersection->GetNormal());
              if (probeHitComponent)
                *probeHitComponent = intersection->GetComponent();
              if (probeHitInst) 
                *probeHitInst = intersection->GetInstance();
              if (probeHitInstLevelIndex)
                *probeHitInstLevelIndex = intersection->GetLevelIndex();
              if (probeHitInstGenID)
                *probeHitInstGenID = intersection->GetGenerationID();

              if (IsInstValid(rayPrbeIndex))
              {
                m_probeHitInstMass[rayPrbeIndex] = m_probeHitInst[rayPrbeIndex]->GetArchetype()->GetMass();
                m_probeHitInstBoundingBoxSize[rayPrbeIndex] = VEC3V_TO_VECTOR3(m_probeHitInst[rayPrbeIndex]->GetArchetype()->GetBound()->GetBoundingBoxSize());
              }
              else
              {
                m_probeHitInst[rayPrbeIndex] = NULL;
              }
              return probe_Hit;
            }//if (intersection)//2
            else
              return probe_NoHit;
          }//if (intersection)//1
          return probe_NoIntersection;
        }
        else//if (mgr->New_IsResultReady(m_AsyncProbeHandles[index]))
        {
#if ART_ENABLE_BSPY
          if (m_AsyncProbeSubmitFrame[index] == m_currentFrame)
            bspyScratchpad(getBSpyID(), "GetAsynchProbeResult - Retrieved on same frame", true);
#endif
          // BAD - it took more than a frame to get results
          return probe_Late;
        }
      }//if (mgr->New_IsInUse(m_AsyncProbeHandles[index]))
      else
      {
        // BAD - your probe handle is bad
        return probe_HandleInvalid;
      }
    }//if (m_AsyncProbeHandles[index] != rage::phAsyncShapeTestMgr::InvalidShapeTestHandle)
    else
    {
      return probe_HandleKnownInvalid;
    }

  }
  //includeAgent,
  void NmRsCharacter::SubmitAsynchProbe(rage::Vector3 &start, rage::Vector3 &end, rayProbeIndex rayPrbeIndex, rage::u8 stateIncludeFlags, unsigned int typeFlags, unsigned int typeIncludeFlags, unsigned int typeExcludeFlags, bool includeAgent)
  {

#if ART_ENABLE_BSPY
    static const char* probeIndexStrings[] =
    {
#define NAME_ACTION(_name) #_name ,
      PROBE_INDEX(NAME_ACTION)
#undef NAME_ACTION
    };
    bspyScratchpad(getBSpyID(), "SubmitAsynchProbe", probeIndexStrings[rayPrbeIndex]);

    bspyScratchpad(getBSpyID(), "SubmitAsynchProbe", start);
    bspyScratchpad(getBSpyID(), "SubmitAsynchProbe", end);
#endif
    asyncProbeIndex  index = rayIndexToProbeIndex(rayPrbeIndex);
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "SubmitAsynchProbe", index);
#endif
    // If async probes are done, set to reference pointers and request a new one
    rage::phAsyncShapeTestMgr* mgr = GetAsyncShapeTestMgr();
    if (!mgr) return;
    rage::Vec3V vin1 = RCC_VEC3V(start);
    rage::Vec3V vin2 = RCC_VEC3V(end);

    // Release old probe handle if any
    if (m_AsyncProbeHandles[index] != rage::phAsyncShapeTestMgr::InvalidShapeTestHandle)
    {
      if (mgr->New_IsInUse(m_AsyncProbeHandles[index]))
      {
        mgr->New_ReleaseProbe(m_AsyncProbeHandles[index]);
      }
      m_AsyncProbeHandles[index] = rage::phAsyncShapeTestMgr::InvalidShapeTestHandle;
    }


    const rage::phInst *excludeInstList = NULL;
    int excludeInstCount = 0;
    m_AsyncProbeRayProbeIndex[index] = rayPrbeIndex;
#if ART_ENABLE_BSPY
    m_AsyncProbeSubmitFrame[index] = m_currentFrame;
#endif
#if 0
    if (!includeAgent)
    {
      excludeInstList = getFirstInstance();
      excludeInstCount = 1;
    }

    m_AsyncProbeHandles[index] = mgr->New_SubmitProbe(NULL, 1, vin1, vin2, stateIncludeFlags, typeFlags, typeIncludeFlags, typeExcludeFlags, &excludeInstList, excludeInstCount, 0);
#else //Exclude car from balancer probes test  //mmmmtodo also do this if balancerCollisionsReaction against car?
	  if (!includeAgent)
	  {
		  m_excludeInstList[index][0] = getFirstInstance();
		  excludeInstCount = 1;
	  }
    NmRsCBUBraceForImpact* bfiTask = (NmRsCBUBraceForImpact*)m_cbuTaskManager->getTaskByID(getID(), bvid_braceForImpact);
    Assert(bfiTask);
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuTaskManager->getTaskByID(getID(), bvid_catchFall);
    Assert(catchFallTask);

	  //exclude car from balancer probes but not from catchFall probes if braceForImpact is running
    if (bfiTask->isActive() && (!catchFallTask->isActive()))
	  {
      int carInstanceIndex = bfiTask->m_parameters.instanceIndex;
      int carInstGenId = bfiTask->getCarInstGenID();
      if (IsInstValid(carInstanceIndex, carInstGenId))
	    {
        m_excludeInstList[index][excludeInstCount] = getLevel()->GetInstance(bfiTask->m_parameters.instanceIndex);
		  excludeInstCount ++;
	    }
	  }
	  if (excludeInstCount)
		  m_AsyncProbeHandles[index] = mgr->New_SubmitProbe(NULL, 1, vin1, vin2, stateIncludeFlags, typeFlags, typeIncludeFlags, typeExcludeFlags, &m_excludeInstList[index][0], excludeInstCount, 0);
	  else
		  m_AsyncProbeHandles[index] = mgr->New_SubmitProbe(NULL, 1, vin1, vin2, stateIncludeFlags, typeFlags, typeIncludeFlags, typeExcludeFlags, &excludeInstList, excludeInstCount, 0);

#endif
  }
#endif // NM_RS_CBU_ASYNCH_PROBES

  void NmRsCharacter::applyRollDownStairForces(float comVelMag, bool useCustomRollDir, bool /*useVelocityOfObjectBelowCharacter*/, bool /*useRelativeVelocity*/,const rage::Vector3 &forwardVelVecInput, float currentSlope, float rollDownStairsForceMag, bool rollDownStairsSpinWhenInAir, float totalMass, float zAxisSpinReduction, float linearVelocityTarget, float airborneReduction)
  {
    //  Exclude extremities from collision measure.  Stops excessive "airborne" torquing.
    //
    const BehaviourMask exceptions = bvmask_FootLeft | bvmask_FootRight | bvmask_HandLeft | bvmask_HandRight | bvmask_Head | bvmask_Neck;
    bool contacts = hasCollidedWithEnvironment(bvmask_Full &~ exceptions);

    float slopeScaleScaler = -50.f;
    rage::Vector3 forwardVelVec = forwardVelVecInput;

    //mmmmtodo so we don't have to use a probe for this velocity //mmmmtodo do only from impact probe index 
    // get this from dynamicBalancer or catchFall probe or from collision 
    //bool hit = false;
    //if (useVelocityOfObjectBelowCharacter)
    //{
    //  rage::Vector3 velocityOfObjectBelowCharacter(0.0f,0.0f,0.0f);
    //  rage::Vector3 pStart = m_COM;
    //  rage::Vector3 pEnd = m_gUp;
    //  pEnd.Scale(2.0f);
    //  pEnd = pStart - pEnd;
    //  rage::phSegment segment;
    //  segment.Set(pStart, pEnd);
    //  rage::phIntersection isect;
    //  rage::Vector3 hitPos;
    //  hit = probeRay(pi_rollDownStairs, segment.A, segment.B, &hitPos, NULL, NULL, &isect, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_probeTypeIncludeFlags, TYPE_FLAGS_NONE, false);
    //  NM_RS_DBG_LOGF(L"hit = : %s",  hit?L"true":L"false");
    //  if (hit)
    //  {
    //    rage::phInst *inst = isect.GetInstance();
    //    //rage::phCollider *hitCollider = getSimulator()->GetCollider(hitInstance);
    //    //if (hitCollider)
    //    //  hitCollider->GetLocalVelocity(VECTOR3_TO_INTRIN(hitPos),RC_VEC3V(velocityOfObjectBelowCharacter),0);
    //    getVelocityOnInstance(inst->GetLevelIndex(),hitPos,&velocityOfObjectBelowCharacter);

    //    setFloorVelocityFromColliderRefFrameVel(velocityOfObjectBelowCharacter);//This takes precedent over CatchFall and DynamicBalancer
    //    float velMag;
    //    //Can't use relativeVelocity here
    //    //if (useRelativeVelocity)
    //    //  velMag = m_COMvelRelativeMag;
    //    //else
    //    velMag = velocityOfObjectBelowCharacter.Mag();

    //    if (velMag > 1.0f)
    //    {
    //      slopeScaleScaler = 0.0f;//Zero torque will be applied
    //    }
    //  }
    //}

    float linVM;
    //if (hit && useRelativeVelocity)//Could have not hit && to allow floorVelocity to come from dynamicBalancer or catchFall?
    //{
    //  linVM = rage::Max(m_COMvelRelativeMag,linearVelocityTarget);
    //}
    //else
    linVM = rage::Max(comVelMag,linearVelocityTarget);

    float rotVM = m_COMrotvelMag;

    const float bodyRadius = 0.35f; // approximate radius of rolling body.
    float rotDifference = linVM - rotVM * bodyRadius;
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "rdsForces", comVelMag);
    bspyScratchpad(getBSpyID(), "rdsForces", useCustomRollDir);
    bspyScratchpad(getBSpyID(), "rdsForces", forwardVelVecInput);
    bspyScratchpad(getBSpyID(), "rdsForces", currentSlope);
    bspyScratchpad(getBSpyID(), "rdsForces", rollDownStairsForceMag);
    bspyScratchpad(getBSpyID(), "rdsForces", rollDownStairsSpinWhenInAir);
    bspyScratchpad(getBSpyID(), "rdsForces", totalMass);
    bspyScratchpad(getBSpyID(), "rdsForces", zAxisSpinReduction);
    bspyScratchpad(getBSpyID(), "rdsForces", linearVelocityTarget);
    bspyScratchpad(getBSpyID(), "rdsForces", airborneReduction);
    bspyScratchpad(getBSpyID(), "rdsForces", linVM);
    bspyScratchpad(getBSpyID(), "rdsForces", rotVM);
#endif

    //Only apply helper torques if we need to 
    if ((rotDifference > 0.0f) && (linVM > 0.5f))
    {
      rotDifference = rage::Clamp(rotDifference, -1.0f, 1.0f);

#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "rdsForces", rotDifference);
#endif
#if 0//Unused
      // add some asymmetry into the motion
      rage::Vector3 assymVec(m_COMTM.c);//bodyBack
      float dotOntoAx = forwardVelVec.Dot(assymVec);

      if (rage::Abs(dotOntoAx) > 0.5f)
      {
        assymVec.Set(m_COMTM.a);//bodyRight
      }

    //forwardVelVec.AddScaled(assymVec, m_rds->getRollDownStairsAsymmetricalForces() * 0.25f * rage::Sinf(2.3f * m_forceTime));
#endif

    // work out the rotation axis
    rage::Vector3 zeros; zeros.Zero();
    rage::Vector3 rotAx;

    // if we are going with gravity, work out the axis from the current rotAx or
    // just take the side one if we are not rotating
    if (rage::Abs(currentSlope) > 1.4f && !useCustomRollDir)
    {
        rotAx.Set(m_COMTM.a);//bodyRight

      // if already rotating then needs to be the current rotation axis,
      // projected into the horizontal plane
      if (rotVM > 0.5f)
      {
        rotAx.Set(m_COMrotvel);
        levelVector(rotAx);
        rotAx.NormalizeSafeV(zeros);
        rotAx.Scale(-1.0f);
      }
    }
    else
    {
      rotAx.Cross(forwardVelVec, getUpVector());
      rotAx.NormalizeSafeV(zeros);
    }

    // maintain the linear and rotational velocity link
    // slopeScaleScaler = -50 normally
    float slopeScale = rollDownStairsForceMag * (slopeScaleScaler);
    NM_RS_DBG_LOGF(L" XXXslopeScale = %.2f", slopeScale)
#if ART_ENABLE_BSPY
      float _slopeScale = slopeScale;
      bspyScratchpad(getBSpyID(), "rdsForces", _slopeScale);
      bspyScratchpad(getBSpyID(), "rdsForces", contacts);
      bspyScratchpad(getBSpyID(), "rdsForces", rollDownStairsSpinWhenInAir);
#endif

    if (!contacts && rollDownStairsSpinWhenInAir)
    {
      //Reduce the applied torque [was 85% now airborneReduction*100 % if airborne and not rotating , 100% if airborne and rotating a lot]   
      slopeScale = slopeScale * (1.f-airborneReduction) * (1.0f - rage::Clamp(rotVM / 10.0f, 0.0f, 1.0f));
#if ART_ENABLE_BSPY
      float airborneScale = (1.f-airborneReduction);
      float rotationScale = (1.0f - rage::Clamp(rotVM / 10.0f, 0.0f, 1.0f));
      bspyScratchpad(getBSpyID(), "rdsForces", airborneScale);
      bspyScratchpad(getBSpyID(), "rdsForces", rotationScale);
#endif
      contacts = true;
    }

#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "rdsForces", slopeScale);
      bspyScratchpad(getBSpyID(), "rdsForces", rotDifference);
#endif
      
      Assert(rage::Abs(totalMass) > 1e-10f);
      float fMagCL = slopeScale * (1.0f / totalMass) * rage::Abs(rotDifference);
      NM_RS_DBG_LOGF(L" XXXfMagCL = %.2f", fMagCL)
        fMagCL = rage::Clamp(fMagCL, -2.0f, 2.0f);

      // apply the torques to various body parts
#define NM_RS_RDS_UNROLLED_TORQUE_TO_PART(_part_) \
      { int partIndex = getSpineSetup()->get##_part_()->getPartIndex();\
      float partMass = getArticulatedBody()->GetMass(partIndex).Getf();\
      rage::Vector3 partTorque;\
      float fMag = 10.0f * fMagCL * partMass;\
      partTorque.SetScaled(rotAx, fMag);\
      getSpineSetup()->get##_part_()->applyTorque(partTorque); }

      if (contacts)
      {
        NM_RS_RDS_UNROLLED_TORQUE_TO_PART(PelvisPart);
        NM_RS_RDS_UNROLLED_TORQUE_TO_PART(Spine0Part);
        NM_RS_RDS_UNROLLED_TORQUE_TO_PART(Spine1Part);
        NM_RS_RDS_UNROLLED_TORQUE_TO_PART(Spine2Part);
        NM_RS_RDS_UNROLLED_TORQUE_TO_PART(Spine3Part);

        if (zAxisSpinReduction > 0.f)
          antiSpinAroundVerticalAxisJuice(zAxisSpinReduction);
      }

#undef NM_RS_RDS_UNROLLED_TORQUE_TO_PART
  }


  }

  void NmRsCharacter::roll2Velocity(const rage::Vector3 &velocity, float rollRadius)
  {
    static float torqueMult = 10.f;
    static float torqueMultSlow = 3.f;
    bool contacts = hasCollidedWithEnvironment(bvmask_Full);

    // work out the rotation axis
    rage::Vector3 zeros; zeros.Zero();
    rage::Vector3 rotAx;
    rotAx.Cross(velocity, getUpVector());
    rotAx.NormalizeSafeV(zeros);
    rotAx.Negate();
    float rotVM = m_COMrotvel.Dot(rotAx);
    //scale rollRadius up for rotation not about twist and scale for head2foot/head2pelvis length 
    float rotDifference = velocity.Mag() - rotVM * rollRadius;

#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "roll2Velocity", rotAx);
    bspyScratchpad(getBSpyID(), "roll2Velocity", rotVM);
    bspyScratchpad(getBSpyID(), "roll2Velocity", velocity);
    bspyScratchpad(getBSpyID(), "roll2Velocity", velocity.Mag());
    bspyScratchpad(getBSpyID(), "roll2Velocity", rotVM * rollRadius);
    bspyScratchpad(getBSpyID(), "roll2Velocity", rotDifference);
#endif

    //Only apply helper torques if we need to 
    if ((rotDifference > 0.0f) && contacts)
    {
      rage::Vector3 torqueVec = rotAx;
      float torqueMag;
      if (rotVM > 0.0f)//increase spin 
      {
        torqueMag = torqueMult*rotDifference;
      }
      else//slow down spin
      {
        torqueMag = -torqueMultSlow;
      }
      torqueVec.Scale(torqueMag);
      getSpineSetup()->getPelvisPart()->applyTorque(torqueVec);
      getSpineSetup()->getSpine0Part()->applyTorque(torqueVec);
      getSpineSetup()->getSpine1Part()->applyTorque(torqueVec);
      torqueVec.Scale(0.6f);
      getSpineSetup()->getSpine2Part()->applyTorque(torqueVec);
      getSpineSetup()->getSpine3Part()->applyTorque(torqueVec);

    }


  }

  void NmRsCharacter::applyTorqueToRoll(float minAngVel, float maxAngVel, float magOfTorque,const rage::Vector3 &forwardVelVec)
  {
    rage::Vector3 upVec = getUpVector();

    rage::Vector3 torqueVec;
    torqueVec.Cross(forwardVelVec, upVec);
    torqueVec.Normalize();
    torqueVec.Negate();
    float currentAngVel = torqueVec.Dot(m_COMrotvel);
    float currentAngVelAbs = rage::Abs(currentAngVel); 
    if((currentAngVelAbs< maxAngVel) && (currentAngVelAbs>minAngVel))
    {
      torqueVec.Scale(magOfTorque);

      getSpineSetup()->getSpine0Part()->applyTorque(torqueVec);
      getSpineSetup()->getSpine1Part()->applyTorque(torqueVec);

      torqueVec.Scale(0.6f);
      getSpineSetup()->getSpine2Part()->applyTorque(torqueVec);
      getSpineSetup()->getSpine3Part()->applyTorque(torqueVec);
    }
  }

  // stops spinning around the vertical axis.
  void NmRsCharacter::antiSpinAroundVerticalAxisJuice(float zAxisSpinReduction, bool useWorldUp /* = true */)
  {
    float zAxisSpinReductionClamp = rage::Clamp(zAxisSpinReduction,0.f,1.f);
    rage::Vector3 comrvn;
    comrvn = m_COMrotvel;

    rage::Vector3 axis;
    if(useWorldUp)
    {
      // stabilize around world up axis
      axis = getUpVector();
    }
    else
    {
      // stabilize around character front/back
      axis = m_COMTM.c;
    }

#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "antiSpinAroundVerticalAxisJuice", axis);
#endif

    float currRotAxisOntoUp = comrvn.Dot(axis);
    axis.Normalize();
    axis.Scale(-getTotalMass()*(0.2f)*currRotAxisOntoUp*zAxisSpinReductionClamp);

    getSpineSetup()->getPelvisPart()->applyTorque(axis);
    getSpineSetup()->getSpine0Part()->applyTorque(axis);
    getSpineSetup()->getSpine1Part()->applyTorque(axis);
    getSpineSetup()->getSpine2Part()->applyTorque(axis);
    getSpineSetup()->getSpine3Part()->applyTorque(axis);

#if ART_ENABLE_BSPY
    rage::Vector3 torque(axis);
    bspyScratchpad(getBSpyID(), "antiSpinAroundVerticalAxisJuice", comrvn);
    bspyScratchpad(getBSpyID(), "antiSpinAroundVerticalAxisJuice", currRotAxisOntoUp);
    bspyScratchpad(getBSpyID(), "antiSpinAroundVerticalAxisJuice", torque);
#endif
  }

  // PD part orientation to transform
  void NmRsCharacter::pdPartOrientationToM(
    NmRsGenericPart *part,
    rage::Matrix34& tm,
    rage::Vector3* angVel,
    float stiffness,
    float damping,
    rage::Vector3* velCache,
    float velSmoothing,
    float /*limit*/)
  {
    Assert(part);
    rage::Matrix34 partTM;
    part->getMatrix(partTM);

#if ART_ENABLE_BSPY && 0
    bspyDrawCoordinateFrame(0.1f, tm);
    bspyDrawCoordinateFrame(0.05f, partTM);
#endif

    // compute error
    rage::Vector3 error;
    {
      rage::Matrix34 diffM;
      rage::Matrix34 tmCopy(tm);
      tmCopy.Transpose();
      diffM.Dot(tmCopy, partTM);
      rage::Quaternion diffQuat;
      diffM.ToQuaternion(diffQuat);

      float angle = 0;
      diffQuat.ToRotation(error, angle);
      if(angle > PI)
        angle -= 2.f*PI;
      else if(angle < -PI)
        angle += 2.f*PI;
      error.Scale(-angle);
#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "pdPartOrientationToM", error);
#endif		
    }

    // apply the torque
    rage::Vector3 torqueImpulse(stiffness * stiffness * getLastKnownUpdateStep() * error);
    part->applyTorqueImpulse(torqueImpulse);
#if ART_ENABLE_BSPY && NM_PHYSHELPER_TORQUE_ARROWS
    bspyDrawTorque(0.15f, partTM.d, torqueImpulse, rage::Vector3(1,0,0.75f));
    bspyDrawPoint(partTM.d+torqueImpulse, 0.01f, rage::Vector3(1.f,1.f,0.f)); // mark the primary torque with a little yellow cross.
#endif

    // compute dError
    rage::Vector3 dError = part->getAngularVelocity();

    // if we have a velocity cache, smooth the part velocity
    if(velCache)
    {
      velCache->Scale(velSmoothing);
      velCache->AddScaled(dError, (1-velSmoothing));
      dError.Set(*velCache);
    }

    if(angVel)
      dError -= *angVel;
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "pdPartOrientationToM", dError);
#endif		

#if 0
    // project dError on error.
    rage::Vector3 errorNormalized(error);
    errorNormalized.Normalize();
    float k = errorNormalized.Dot(dError);
    dError.SetScaled(error, k);
#endif

    // apply the torque
    torqueImpulse.Set(stiffness * damping * getLastKnownUpdateStep() * -dError);
    part->applyTorqueImpulse(torqueImpulse);
#if ART_ENABLE_BSPY && NM_PHYSHELPER_TORQUE_ARROWS
    bspyDrawTorque(0.15f, partTM.d, torqueImpulse, rage::Vector3(0.75f,0,1));
#endif
  }

  // PD part position to transform
  void NmRsCharacter::pdPartPositionToITM(NmRsGenericPart *part, float stiffness, float damping)
  {
    rage::Matrix34 currentITM, lastITM;

    // get target ITM
    getITMForPart(part->getPartIndex(), &currentITM, kITSourceCurrent);

    // if we have a previous ITM, compute target linear velocity
    rage::Vector3 velITM; velITM.Zero();
    if(getITMForPart(part->getPartIndex(), &lastITM, kITSourcePrevious))
      velITM = currentITM.d - lastITM.d;

    pdPartPositionToM(part, currentITM, &velITM, stiffness, damping);
  }

  // PD part position to transform
  void NmRsCharacter::pdPartPositionToM(
    NmRsGenericPart *part, 
    rage::Matrix34& tm, 
    rage::Vector3* vel, 
    float stiffness, 
    float damping,
    rage::Vector3* velCache,
    float velSmoothing,
    rage::Vector3* localPos)
  {
    pdPartPosition(part, tm.d, vel, stiffness, damping, velCache, velSmoothing, localPos);
  }

  void NmRsCharacter::pdPartPosition(
    NmRsGenericPart *part, 
    rage::Vector3& pos, 
    rage::Vector3* vel, 
    float stiffness, 
    float damping,
    rage::Vector3* /*velCache*/,
    float /*velSmoothing*/,
    rage::Vector3* localPos)
  {
#if ART_ENABLE_BSPY 
    m_currentSubBehaviour = "-pdPart"; 
#endif
    Assert(part);
    rage::Matrix34 partTM;
    part->getMatrix(partTM);

    // apply local offset if provided.
    rage::Vector3 partPos(partTM.d);
    if(localPos)
    {
      rage::Vector3 offset;
      partTM.Transform3x3(*localPos, offset);
      partPos.Set(partTM.d+offset);
    }

    // compute error
    rage::Vector3 error;
    error = partPos - pos;
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "pdPartPosition", error);
#endif

#if ART_ENABLE_BSPY
    {
      rage::Vector3 start(partPos + rage::Vector3(0.001f, 0, 0));
      bspyDrawLine( start,
        start + stiffness * stiffness * getLastKnownUpdateStep() * -error,
        rage::Vector3(1,0,0));
    }
#endif	

    rage::Vector3 impulse(stiffness * stiffness * getLastKnownUpdateStep() * -error);
#if 0
    // apply the impulse
    part->applyImpulse(impulse, partPos);
#endif

    // compute dError
    // apply part velocity. 
    rage::Vector3 dError = part->getLinearVelocity(&partPos);

#if 0 // not certain this works as designed.
    // if we have a velocity cache, smooth the part velocity
    if(velCache)
    {
      velCache->Scale(velSmoothing);
      velCache->AddScaled(dError, (1-velSmoothing));
      dError.Set(*velCache);
    }
#endif

    // apply target velocity.
    if(vel)
      dError -= *vel;
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "pdPartPosition", dError);
#endif	

#if 0
    // project dError on error direction
    rage::Vector3 errorDirection;
    errorDirection.Normalize(error);
    float dErrorMag = error.Dot(dError);
    dError.SetScaled(error, dErrorMag);
#endif

    // apply the impulse
#if ART_ENABLE_BSPY
    {
      rage::Vector3 start(partPos + rage::Vector3(0.002f, 0, 0));
      bspyDrawLine( start,
        start -2.f * stiffness * damping * getLastKnownUpdateStep() * dError,
        rage::Vector3(1,1,0));
    }
#endif	

#if 1
    impulse.Add(-2.f * stiffness * damping * getLastKnownUpdateStep() * dError);
#else
    impulse.Set(-2.f * stiffness * damping * getLastKnownUpdateStep() * dError);
#endif
    part->applyImpulse(impulse, partPos);

#if ART_ENABLE_BSPY 
    m_currentSubBehaviour = ""; 
#endif
  }

  // check level index is good.
  bool NmRsCharacter::getIsInLevel(int levelIndex)
  {
    return (getEngine()->getLevel()->IsInLevel(levelIndex));
  }

  // makes a point constraint between the hands.  supports point gun two-handed weapons.
  // if constraint already exists, reduces the constraint distance incrementally.
  // -primaryArm, supportArm are self-explanatory.
  // -constraintPosition is world space position of the constraint on the primary hand.
  //  position on support hand is assumed to be the part position.
  // -constraintDistance is the starting distance between constraint positions (why is
  //  this not calculated internally?)
  // -minimumConstraintDistance can be used to keep the constraint from fully closing.
  //  very small (but non-zero) values can cause instability.
  void NmRsCharacter::constrainSupportHand(const NmRsHumanArm *primaryArm, const NmRsHumanArm *supportArm, rage::Vector3 *constraintPosition, const float minimumConstraintDistance, float constraintLengthReduction, float constraintStrength, bool distanceConstraint)
  {
    //If constraint type has changed and constraint exists, release it so that one of the correct type can be created
    rage::phConstraintBase* constraint = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(m_handToHandConstraintHandle) );
    if (constraint)
  {
      //GetType() 1=distance, 2=spherical, (5=attachment, 8=fixed)
      //m_parameters.supportConstraint 0=none, 1=distance, 2=forceBased, 3=spherical
      if ((constraint->GetType() == rage::phConstraintBase::DISTANCE && !distanceConstraint) ||
        (constraint->GetType() == rage::phConstraintBase::SPHERICAL && distanceConstraint) )
        releaseSupportHandConstraint();
    }
    // If an already active constraint.
    if (m_handToHandConstraintHandle.IsValid())
    {
      reduceConstraintSeperation(m_handToHandConstraintHandle, constraintPosition, constraintLengthReduction, minimumConstraintDistance);
    }//if (m_handToHandConstraint)       
    else // otherwise, make a new constraint.
    {
      // update constraint info
      Assert(constraintPosition);
      // Create the constraint.
      float constraintDistance;//note if this is <=0.0 then it would be worked out by rage anyway
      rage::Vector3 lhPos = supportArm->getHand()->getPosition();
      constraintDistance = lhPos.Dist(*constraintPosition);

      constrainPart(
        m_handToHandConstraintHandle, 
        supportArm->getHand()->getPartIndex(),
        primaryArm->getHand()->getPartIndex(),
        constraintDistance,
        lhPos,
        *constraintPosition,
        getFirstInstance()->GetLevelIndex(),
        false, 
        distanceConstraint, 
        minimumConstraintDistance);
      //if (m_handConstraint)//For when the weird parallel pointing and release arm for shot/catchfall code is written
      //  m_handConstraint->SetBreakingStrength(m_breakConstraint);
    }
    setConstraintBreakable(m_handToHandConstraintHandle,(constraintStrength > -0.01f), constraintStrength);

#if ART_ENABLE_BSPY
    if (distanceConstraint)
    {
      rage::phConstraintDistance* constraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_handToHandConstraintHandle) );
    // draw the constraint radius.
      if (constraint)
    {
        rage::Vector3 worldPosA(VEC3V_TO_VECTOR3(constraint->GetWorldPosA()));
        rage::Vector3 worldPosB(VEC3V_TO_VECTOR3(constraint->GetWorldPosB()));
      // draw a cross representing the radius of the constraint.
        float radius = constraint->GetMaxDistance();
        bspyDrawPoint(worldPosB, 0.05f, rage::Vector3(0,1,0));//green - point on gun
        rage::Vector3 worldPosB2A = worldPosA - worldPosB;
        worldPosB2A.Normalize();
        rage::Vector3 worldPosError = worldPosB + radius*worldPosB2A;
        bspyDrawLine(worldPosError, worldPosA, rage::Vector3(0,1,0));//constraint length
       //if(radius > 0.f)
        //  bspyDrawSphere(worldPosA, radius, rage::Vector3(1,0,0));
        bspyDrawLine(worldPosError, worldPosB, rage::Vector3(1,0,0));//constraint error
        bspyDrawPoint(worldPosA, 0.05f, rage::Vector3(1,0,0));//red - point on hand
    }
    }
    else
    {
      rage::phConstraintSpherical* constraint = static_cast<rage::phConstraintSpherical*>( PHCONSTRAINT->GetTemporaryPointer(m_handToHandConstraintHandle) );
      // draw the constraint radius.
      if (constraint)
    {
        rage::Vector3 worldPosA(VEC3V_TO_VECTOR3(constraint->GetWorldPosA()));
        rage::Vector3 worldPosB(VEC3V_TO_VECTOR3(constraint->GetWorldPosB()));
        // draw a cross representing the radius of the constraint.
        bspyDrawPoint(*constraintPosition, 0.05f, rage::Vector3(0,0,1));//green - point on gun
        bspyDrawLine(*constraintPosition, worldPosB, rage::Vector3(0,0,1));//constraint length to go
        bspyDrawPoint(worldPosB, 0.05f, rage::Vector3(0,0,1));//blue - current constraint target
        bspyDrawLine(worldPosA, worldPosB, rage::Vector3(1,0,0));//red error in constraint
        bspyDrawPoint(worldPosA, 0.05f, rage::Vector3(1,0,0));//red - point on hand
      }

    }
#endif
  }

  void NmRsCharacter::releaseSupportHandConstraint()
  {
    ReleaseConstraintSafely(m_handToHandConstraintHandle);
  }

  //Safely Releases a constraint if it is an active constraint
  void NmRsCharacter::ReleaseConstraintSafely(rage::phConstraintHandle &constraint)  
  {
    if (constraint.IsValid())
    {
		  // Check if the constraint is attached to a arm that was made heavier, and if so revert to default weight.
		  // This is an assumption currently and will require extra logic if two constraints are being attached to one hand at the same time.
		  rage::phConstraintBase* baseConstraint = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(constraint) );
		  if (baseConstraint)
		  {
			  int relevantComponent = baseConstraint->GetInstanceA() == getFirstInstance() ? baseConstraint->GetComponentA() : baseConstraint->GetComponentB();
			  if (relevantComponent == gtaHand_Left && m_LeftArmMassIncreased)
				  AlterArmMasses(kLeftHand, false);
			  else if (relevantComponent == gtaHand_Right && m_RightArmMassIncreased)
				  AlterArmMasses(kRightHand, false);
		  }

      getSimulator()->GetConstraintMgr()->Remove(constraint);
      constraint.Reset();
    }
  }

  //if bodyPart>0 snap this part only
  void NmRsCharacter::snap(
    float snapMag, 
    float snapDirectionRandomness,
    int snapHipType,
    bool snapLeftArm, 
    bool snapRightArm, 
    bool snapLeftLeg, 
    bool snapRightLeg, 
    bool snapSpine, 
    bool snapNeck,
    bool snapPhasedLegs,
    bool snapUseTorques,
    float mult,
    int bodyPart,
    rage::Vector3 *snapDirection,
    float movingMult,
    float balancingMult,
    float airborneMult,
    float movingThresh)
  {
    static float angVelMult = 250.0f;
    if (!snapUseTorques)
      snapMag *= angVelMult;
    //mmmmtodo modulate on airborne and moving?
    //mmmmtodo modulate each part if moving?
    bool airborne = !hasCollidedWithWorld(bvmask_Full);
    bool balancing = !hasCollidedWithWorld(bvmask_UpperBody);
    bool moving = balancing && (m_COMvelMag > movingThresh);
    if (airborne)
      snapMag *= airborneMult;//?;
    else if (moving)
      snapMag *= movingMult;//4.0f;
    else if (balancing)
      snapMag *= balancingMult;//2.0f;
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "Snap", airborne);
    bspyScratchpad(getBSpyID(), "Snap", balancing);
    bspyScratchpad(getBSpyID(), "Snap", moving);
#endif
    rage::Matrix34 tmCom1(m_COMTM);
    rage::Vector3 bodySide = tmCom1.a;
    rage::Vector3 bodyBack = tmCom1.c;

    Assert(snapUseTorques);

    rage::Vector3 direction;
    if (snapDirection)
    {
      direction.Set(*snapDirection);
      direction.Normalize();
      bodyBack = direction;
      bodySide.Cross(direction,m_gUp);

    }

    float snapMagValue = snapMag;
    //Snap only around body Part
    if (bodyPart >= 0 && bodyPart < getNumberOfParts())
    {
      NmRsEffectorBase *parentEffector = NULL;
      NmRsEffectorBase *childEffector = NULL;
      NmRsEffectorBase *parentEffector2 = NULL;
      switch (bodyPart)
      {
      case gtaButtocks:
        parentEffector = getEffector(gtaJtHip_Left);
        parentEffector2 = getEffector(gtaJtHip_Right);
        childEffector = getEffector(gtaJtSpine_0);
        break;
      case gtaSpine0:
        parentEffector = getEffector(gtaJtSpine_0);
        childEffector = getEffector(gtaJtSpine_1);
        break;
      case gtaSpine1:
        parentEffector = getEffector(gtaJtSpine_1);
        childEffector = getEffector(gtaJtSpine_2);
        break;
      case gtaSpine2:
        parentEffector = getEffector(gtaJtSpine_2);
        childEffector = getEffector(gtaJtSpine_3);
        break;
      case gtaSpine3:
        parentEffector = getEffector(gtaJtSpine_3);
        childEffector = getEffector(gtaJtNeck_Lower);
        break;
      case gtaNeck:
        parentEffector = getEffector(gtaJtNeck_Lower);
        childEffector = getEffector(gtaJtNeck_Upper);
        break;
      case gtaHead:
        parentEffector = getEffector(gtaJtNeck_Lower);
        childEffector = NULL;
        break;
      case gtaClav_Left:
        parentEffector = getEffector(gtaJtClav_Jnt_Left);
        childEffector = getEffector(gtaJtShoulder_Left);
        break;
      case gtaClav_Right:
        parentEffector = getEffector(gtaJtClav_Jnt_Right);
        childEffector = getEffector(gtaJtShoulder_Right);
        break;
      case gtaUpper_Arm_Left:
        parentEffector = getEffector(gtaJtShoulder_Left);
        childEffector = getEffector(gtaJtElbow_Left);
        break;
      case gtaUpper_Arm_Right:
        parentEffector = getEffector(gtaJtShoulder_Right);
        childEffector = getEffector(gtaJtElbow_Right);
        break;
      case gtaLower_Arm_Left:
        parentEffector = getEffector(gtaJtElbow_Left);
        childEffector = getEffector(gtaJtWrist_Left);
        break;
      case gtaLower_Arm_Right:
        parentEffector = getEffector(gtaJtElbow_Right);
        childEffector = getEffector(gtaJtWrist_Right);
        break;
      case gtaThigh_Left:
        parentEffector = getEffector(gtaJtHip_Left);
        childEffector = getEffector(gtaJtKnee_Left);
        break;
      case gtaThigh_Right:
        parentEffector = getEffector(gtaJtHip_Right);
        childEffector = getEffector(gtaJtKnee_Right);
        break;
      case gtaShin_Left:
        parentEffector = getEffector(gtaJtKnee_Left);
        childEffector = getEffector(gtaJtAnkle_Left);
        break;
      case gtaShin_Right:
        parentEffector = getEffector(gtaJtKnee_Right);
        childEffector = getEffector(gtaJtAnkle_Right);
        break;
      default://no feet/hands response
        break;
      }
      if (parentEffector2)
        snapMagValue *= 0.5f;
      if (parentEffector)
      {
        if (parentEffector->is3DofEffector())
        {
          NmRs3DofEffector *threedof = (NmRs3DofEffector *)parentEffector;
          if (snapUseTorques)
            threedof->ApplyAngImpulse(snapMagValue*15.f*getRandomVec(bodySide, snapDirectionRandomness));
          else
            changeJointAngVel(threedof, snapMagValue*0.1f*getRandomVec(bodySide, snapDirectionRandomness), 0.5f);

        }
        else
        {
          NmRs1DofEffector *onedof = (NmRs1DofEffector *)parentEffector;
          if (snapUseTorques)
            onedof->ApplyAngImpulse(-snapMagValue*2.f);
          else
            changeJointAngVel(onedof, -1.f*snapMagValue*0.05f, 0.5f);
        }
      }

      if (parentEffector2)
      {
        if (parentEffector2->is3DofEffector())
        {
          NmRs3DofEffector *threedof = (NmRs3DofEffector *)parentEffector2;
          if (snapUseTorques)
            threedof->ApplyAngImpulse(snapMagValue*15.f*getRandomVec(bodySide, snapDirectionRandomness));
          else
            changeJointAngVel(threedof, snapMagValue*0.1f*getRandomVec(bodySide, snapDirectionRandomness), 0.5f);
        }
        else
        {
          NmRs1DofEffector *onedof = (NmRs1DofEffector *)parentEffector2;
          if (snapUseTorques)
            onedof->ApplyAngImpulse(-snapMagValue*2.f);
          else
            changeJointAngVel(onedof, -1.f*snapMagValue*0.05f, 0.5f);
        }
      }
      if (parentEffector2)
        snapMagValue *= 2.0f;

      if (childEffector)
      {
        if (childEffector->is3DofEffector())
        {
          NmRs3DofEffector *threedof = (NmRs3DofEffector *)childEffector;
          if (snapUseTorques)
            threedof->ApplyAngImpulse(snapMagValue*15.f*getRandomVec(bodySide, snapDirectionRandomness));
          else
            changeJointAngVel(threedof, snapMagValue*0.1f*getRandomVec(bodySide, snapDirectionRandomness), 0.5f);
        }
        else
        {
          NmRs1DofEffector *onedof = (NmRs1DofEffector *)childEffector;
          if (snapUseTorques)
            onedof->ApplyAngImpulse(-snapMagValue*2.f);
          else
            changeJointAngVel(onedof, -1.f*snapMagValue*0.05f, 0.5f);
        }
      }

    }//Snap only around body Part
    else
    {
      //Snap parts defined by parameters
      float phase = snapPhasedLegs ? -1.f:1.f;
      if (snapUseTorques)
      {
        if (snapLeftArm)
        {
          ((NmRs1DofEffector*)getEffector(gtaJtElbow_Left))->ApplyAngImpulse(-snapMagValue*2.f);
          ((NmRs3DofEffector*)getEffector(gtaJtShoulder_Left))->ApplyAngImpulse(-snapMagValue*4.f*getRandomVec(bodyBack, snapDirectionRandomness));
          ((NmRs3DofEffector*)getEffector(gtaJtClav_Jnt_Left))->ApplyAngImpulse(snapMagValue*4.f*getRandomVec(bodyBack, snapDirectionRandomness));
        }
        if (snapRightArm)
        {
          ((NmRs1DofEffector*)getEffector(gtaJtElbow_Right))->ApplyAngImpulse(-snapMagValue*2.f);
          ((NmRs3DofEffector*)getEffector(gtaJtShoulder_Right))->ApplyAngImpulse(-snapMagValue*4.f*getRandomVec(bodyBack, snapDirectionRandomness));
          ((NmRs3DofEffector*)getEffector(gtaJtClav_Jnt_Right))->ApplyAngImpulse(snapMagValue*4.f*getRandomVec(bodyBack, snapDirectionRandomness));
        }
        if (snapNeck)
        {
          ((NmRs3DofEffector*)getEffector(gtaJtNeck_Upper))->ApplyAngImpulse(+snapMagValue*2.f*getRandomVec(bodySide, snapDirectionRandomness));
          ((NmRs3DofEffector*)getEffector(gtaJtNeck_Lower))->ApplyAngImpulse(-snapMagValue*2.f*getRandomVec(bodyBack, snapDirectionRandomness));
        }
        if (snapSpine)
        {
          ((NmRs3DofEffector*)getEffector(gtaJtSpine_3))->ApplyAngImpulse(snapMagValue*15.f*getRandomVec(bodySide, snapDirectionRandomness));
          ((NmRs3DofEffector*)getEffector(gtaJtSpine_2))->ApplyAngImpulse(snapMagValue*15.f*getRandomVec(bodySide, snapDirectionRandomness));
          ((NmRs3DofEffector*)getEffector(gtaJtSpine_1))->ApplyAngImpulse(snapMagValue*15.f*getRandomVec(bodySide, snapDirectionRandomness));
          ((NmRs3DofEffector*)getEffector(gtaJtSpine_0))->ApplyAngImpulse(snapMagValue*25.f*getRandomVec(bodySide, snapDirectionRandomness));
        }
        if (snapLeftLeg)
        {
          if (snapHipType > 1)
            ((NmRs3DofEffector*)getEffector(gtaJtHip_Left))->ApplyAngImpulse(snapMagValue*20.f*getRandomVec(bodyBack, snapDirectionRandomness));
          else if (snapHipType > 0)
            ((NmRs3DofEffector*)getEffector(gtaJtHip_Left))->ApplyAngImpulse(snapMagValue*20.f*getRandomVec(bodySide, snapDirectionRandomness));
          ((NmRs1DofEffector*)getEffector(gtaJtKnee_Left))->ApplyAngImpulse(-snapMagValue*10.f);
        }
        if (snapRightLeg)
        {
          if (snapHipType > 1)
            ((NmRs3DofEffector*)getEffector(gtaJtHip_Right))->ApplyAngImpulse(phase*snapMagValue*20.f*getRandomVec(bodyBack, snapDirectionRandomness));
          else if (snapHipType > 0)
            ((NmRs3DofEffector*)getEffector(gtaJtHip_Right))->ApplyAngImpulse(phase*snapMagValue*20.f*getRandomVec(bodySide, snapDirectionRandomness));
          ((NmRs1DofEffector*)getEffector(gtaJtKnee_Right))->ApplyAngImpulse(snapMagValue*10.f);
        }
#if ART_ENABLE_BSPY & 0
        if (snapLeftArm)
        {
                  bspyDrawLine(getLeftArmSetup()->getElbow()->getJointPosition(), getLeftArmSetup()->getElbow()->getJointPosition() +  getLeftArmSetup()->getElbow()->get1DofJoint()->GetRotationAxis()*(-snapMagValue*2.f), rage::Vector3(1,0.5,0));
				  bspyDrawLine(getLeftArmSetup()->getShoulder()->getJointPosition(), getLeftArmSetup()->getShoulder()->getJointPosition() +  (-snapMagValue*4.f*getRandomVec(bodyBack, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
				  bspyDrawLine(getLeftArmSetup()->getClavicle()->getJointPosition(), getLeftArmSetup()->getClavicle()->getJointPosition() +  (snapMagValue*4.f*getRandomVec(bodyBack, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
        }
        if (snapRightArm)
        {
				  bspyDrawLine(getRightArmSetup()->getElbow()->getJointPosition(), getRightArmSetup()->getElbow()->getJointPosition() +  getRightArmSetup()->getElbow()->get1DofJoint()->GetRotationAxis()*(snapMagValue*2.f), rage::Vector3(1,0.5,0));
				  bspyDrawLine(getRightArmSetup()->getShoulder()->getJointPosition(), getRightArmSetup()->getShoulder()->getJointPosition() +  (-snapMagValue*4.f*getRandomVec(bodyBack, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
				  bspyDrawLine(getRightArmSetup()->getClavicle()->getJointPosition(), getRightArmSetup()->getClavicle()->getJointPosition() +  (+snapMagValue*4.f*getRandomVec(bodyBack, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
        }
        if (snapNeck)
        {
				  bspyDrawLine(getSpineSetup()->getUpperNeck()->getJointPosition(), getSpineSetup()->getUpperNeck()->getJointPosition() +  (+snapMagValue*2.f*getRandomVec(bodySide, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
				  bspyDrawLine(getSpineSetup()->getLowerNeck()->getJointPosition(), getSpineSetup()->getLowerNeck()->getJointPosition() +  (-snapMagValue*2.f*getRandomVec(bodyBack, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
        }
        if (snapSpine)
        {
				  bspyDrawLine(getSpineSetup()->getSpine3()->getJointPosition(), getSpineSetup()->getSpine3()->getJointPosition() +  (snapMagValue*15.f*getRandomVec(bodySide, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
				  bspyDrawLine(getSpineSetup()->getSpine2()->getJointPosition(), getSpineSetup()->getSpine2()->getJointPosition() +  (snapMagValue*15.f*getRandomVec(bodySide, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
				  bspyDrawLine(getSpineSetup()->getSpine1()->getJointPosition(), getSpineSetup()->getSpine1()->getJointPosition() +  (snapMagValue*15.f*getRandomVec(bodySide, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
				  bspyDrawLine(getSpineSetup()->getSpine0()->getJointPosition(), getSpineSetup()->getSpine0()->getJointPosition()+  (snapMagValue*25.f*getRandomVec(bodySide, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
        }
        if (snapLeftLeg)
        {
          if (snapHipType > 1)
					  bspyDrawLine(getLeftLegSetup()->getHip()->getJointPosition(), getLeftLegSetup()->getHip()->getJointPosition() +  (snapMagValue*20.f*getRandomVec(bodyBack, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
          else if (snapHipType > 0)
					  bspyDrawLine(getLeftLegSetup()->getHip()->getJointPosition(), getLeftLegSetup()->getHip()->getJointPosition() +  (snapMagValue*20.f*getRandomVec(bodySide, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
				  bspyDrawLine(getLeftLegSetup()->getKnee()->getJointPosition(), getLeftLegSetup()->getKnee()->getJointPosition() +  getLeftLegSetup()->getKnee()->get1DofJoint()->GetRotationAxis()*(-snapMagValue*10.f), rage::Vector3(1,0.5,0));
        }
        if (snapRightLeg)
        {
          if (snapHipType > 1)
					  bspyDrawLine(getRightLegSetup()->getHip()->getJointPosition(), getRightLegSetup()->getHip()->getJointPosition() +  (phase*snapMagValue*20.f*getRandomVec(bodyBack, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
          else if (snapHipType > 0)
					  bspyDrawLine(getRightLegSetup()->getHip()->getJointPosition(), getRightLegSetup()->getHip()->getJointPosition() +  (phase*snapMagValue*20.f*getRandomVec(bodySide, snapDirectionRandomness)), rage::Vector3(1,0.5,0));
				  bspyDrawLine(getRightLegSetup()->getKnee()->getJointPosition(), getRightLegSetup()->getKnee()->getJointPosition() +  getRightLegSetup()->getKnee()->get1DofJoint()->GetRotationAxis()*(snapMagValue*10.f), rage::Vector3(1,0.5,0));
        }
#endif
      }
      else
      {
        if (snapLeftArm)
        {
          changeJointAngVel((NmRs1DofEffector*)getEffector(gtaJtElbow_Left), -1.f*snapMagValue*0.05f, 0.5f);
          changeJointAngVel((NmRs3DofEffector*)getEffector(gtaJtShoulder_Left), snapMagValue*0.1f*getRandomVec(bodyBack, snapDirectionRandomness), 0.5f);
        }
        if (snapRightArm)
        {
          changeJointAngVel((NmRs1DofEffector*)getEffector(gtaJtElbow_Right), +1.f*snapMagValue*0.05f, 0.5f);
          changeJointAngVel((NmRs3DofEffector*)getEffector(gtaJtShoulder_Right), snapMagValue*0.1f*getRandomVec(bodyBack, snapDirectionRandomness), 0.5f);
        }

        if (snapNeck)
        {
          changeJointAngVel((NmRs3DofEffector*)getEffector(gtaJtNeck_Upper), -snapMagValue*0.15f*getRandomVec(bodySide, snapDirectionRandomness), 0.5f);
          changeJointAngVel((NmRs3DofEffector*)getEffector(gtaJtNeck_Lower), snapMagValue*0.15f*getRandomVec(bodyBack, snapDirectionRandomness), 0.5f);
        }
        if (snapSpine)
        {
          changeJointAngVel((NmRs3DofEffector*)getEffector(gtaJtSpine_3), -snapMagValue*0.1f*getRandomVec(bodySide, snapDirectionRandomness), 0.5f);
          changeJointAngVel((NmRs3DofEffector*)getEffector(gtaJtSpine_1), -snapMagValue*0.1f*getRandomVec(bodySide, snapDirectionRandomness), 0.5f);
        }

        if (snapLeftLeg)
        {
          if (snapHipType > 1)
            changeJointAngVel((NmRs3DofEffector*)getEffector(gtaJtHip_Left), mult*snapMagValue*0.02f*getRandomVec(bodyBack, snapDirectionRandomness),2.f);
          else if (snapHipType > 0)
            changeJointAngVel((NmRs3DofEffector*)getEffector(gtaJtHip_Left), mult*snapMagValue*0.02f*getRandomVec(bodySide, snapDirectionRandomness),2.f);
          changeJointAngVel((NmRs1DofEffector*)getEffector(gtaJtKnee_Left), -mult*snapMagValue*0.05f, 0.5f);
        }
        if (snapRightLeg)
        {
          if (snapHipType > 1)
            changeJointAngVel((NmRs3DofEffector*)getEffector(gtaJtHip_Right),  mult*phase*snapMagValue*0.02f*getRandomVec(bodyBack, snapDirectionRandomness),2.f);
          else if (snapHipType > 0)
            changeJointAngVel((NmRs3DofEffector*)getEffector(gtaJtHip_Right),  mult*phase*snapMagValue*0.02f*getRandomVec(bodySide, snapDirectionRandomness),2.f);
          changeJointAngVel((NmRs1DofEffector*)getEffector(gtaJtKnee_Right), mult*snapMagValue*0.05f, 0.5f);
        }
      }
    }
#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "Snap", bodySide);
      bspyScratchpad(getBSpyID(), "Snap", bodyBack);
      bspyScratchpad(getBSpyID(), "Snap", snapMagValue);
#endif
    }

  void NmRsCharacter::changeJointAngVel(NmRs3DofEffector* effector, const rage::Vector3 &angVel, float parentWeight)
  {
    rage::Vector3 partAngvel = VEC3V_TO_VECTOR3(getArticulatedBody()->GetAngularVelocityNoProp(effector->getParentIndex()))-parentWeight*angVel; // NoProp
    getArticulatedBody()->SetAngularVelocity(effector->getParentIndex(), RCC_VEC3V(partAngvel));
    partAngvel = VEC3V_TO_VECTOR3(getArticulatedBody()->GetAngularVelocityNoProp(effector->getChildIndex()))+angVel; // NoProp
    getArticulatedBody()->SetAngularVelocity(effector->getChildIndex(), RCC_VEC3V(partAngvel));    
  }
  void NmRsCharacter::changeJointAngVel(NmRs1DofEffector* effector, float angVel, float parentWeight)
  {
    rage::Vector3 jointAxis = effector->get1DofJoint()->GetRotationAxis();
    rage::Vector3 partAngvel = VEC3V_TO_VECTOR3(getArticulatedBody()->GetAngularVelocityNoProp(effector->getParentIndex()))-parentWeight*angVel*jointAxis; // NoProp
    getArticulatedBody()->SetAngularVelocity(effector->getParentIndex(), RCC_VEC3V(partAngvel));
    partAngvel = VEC3V_TO_VECTOR3(getArticulatedBody()->GetAngularVelocityNoProp(effector->getChildIndex()))+angVel*jointAxis; // NoProp
    getArticulatedBody()->SetAngularVelocity(effector->getChildIndex(), RCC_VEC3V(partAngvel));    
  }
  rage::Vector3 NmRsCharacter::getRandomVec(rage::Vector3 &startVec, float deviation)
  {
    rage::Vector3 retVec = startVec;
    retVec.x += getRandom().GetRanged(-deviation, deviation);
    retVec.y += getRandom().GetRanged(-deviation, deviation);
    retVec.z += getRandom().GetRanged(-deviation, deviation);
    retVec.Normalize();
    return retVec;
  }

  void NmRsCharacter::controlStiffness(
    NmRsHumanBody& body,
    float timeStep,
    //globals
    float controlStiffnessStrengthScale,
    //inputs
    float spineStiffness,//= m_parameters.bodyStiffness;
    float armsStiffness,//= m_parameters.armStiffness
    float armsDamping,
    float spineDamping,//= m_parameters.spineDamping
    float neckStiffness,
    float neckDamping,

    float upperBodyStiffness,
    float lowerBodyStiffness,

    //shot only inputs
    bool injuredLArm,
    bool injuredRArm,
    //shot params
    float loosenessAmount,
    float kMultOnLoose,
    float kMult4Legs,
    float minLegsLooseness,
    float minArmsLooseness,
    //shot only params
    bool bulletProofVest,
    bool allowInjuredArm,
    bool stableHandsAndNeck
    )
  {
    NmRsCBUStaggerFall* staggerFallTask = (NmRsCBUStaggerFall*)m_cbuTaskManager->getTaskByID(getID(), bvid_staggerFall);
    Assert(staggerFallTask);
    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuTaskManager->getTaskByID(getID(), bvid_balancerCollisionsReaction);
    Assert(balColReactTask);

    // set gravity opposition
    //----------------------------------------------------------------------------------------------------------
    float opposeGravStiffness = 0.5f + 0.5f*upperBodyStiffness;

    body.getLeftArmInputData()->getClavicle()->setOpposeGravity(opposeGravStiffness);
    body.getLeftArmInputData()->getShoulder()->setOpposeGravity(opposeGravStiffness);
    body.getLeftArmInputData()->getElbow()->setOpposeGravity(opposeGravStiffness);

    body.getRightArmInputData()->getClavicle()->setOpposeGravity(opposeGravStiffness);
    body.getRightArmInputData()->getShoulder()->setOpposeGravity(opposeGravStiffness);
    body.getRightArmInputData()->getElbow()->setOpposeGravity(opposeGravStiffness);

    body.getLeftArmInputData()->getWrist()->setOpposeGravity(opposeGravStiffness);
    body.getRightArmInputData()->getWrist()->setOpposeGravity(opposeGravStiffness);

    body.getSpineInputData()->getSpine0()->setOpposeGravity(1.f + upperBodyStiffness);
    body.getSpineInputData()->getSpine1()->setOpposeGravity(1.f + upperBodyStiffness);
    body.getSpineInputData()->getSpine2()->setOpposeGravity(1.f + upperBodyStiffness);
    body.getSpineInputData()->getSpine3()->setOpposeGravity(1.f + upperBodyStiffness);
    body.getSpineInputData()->getLowerNeck()->setOpposeGravity(1.f);
    body.getSpineInputData()->getUpperNeck()->setOpposeGravity(1.f);

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuTaskManager->getTaskByID(getID(), bvid_dynamicBalancer);
    Assert(dynamicBalancerTask);

    // This line makes him less likely to step when he's weaker. 
    //TODO      _g.stepDecisionThreshold = -0.05 + (0.0 - -0.05)*stiffness


    // upper body strength and stiffness
    //----------------------------------------------------------------------------------------------------------      
    // calculate ramped muscle stiffness
    float scale = (1.f-loosenessAmount) + controlStiffnessStrengthScale*loosenessAmount;

    // calculate upper body strength (can be modified by shotRelax and characterStrength)
    float strengthScale = 0.5f + 0.5f*upperBodyStiffness;
    strengthScale *= 1.f+(1.f-scale)*kMultOnLoose;

    float maxStrength = 10.f + 10.f/(60.f*timeStep);

    scale = rage::Clamp(scale,0.05f,1.f);
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "ControlStiffness", scale);
#endif

    float stiff = scale;
    //if (!staggerFallTask->isActive())//Fix for zombie walk during stagger - removed as causes stagger to go stationary more often after being shot 
    {
      body.setStiffness(spineStiffness*strengthScale, spineDamping*scale, bvmask_LowSpine, &stiff);
    }
#if NM_STAGGERSHOT
    m_spineStrengthScale = strengthScale;
    m_spineDampingScale = scale;
    m_spineMuscleStiffness = stiff;
#endif

    if (bulletProofVest)
    {
      float stiff1 = rage::Clamp(2.f*scale,0.2f,1.f);
      float strengthScale1 = 0.5f + 0.5f*upperBodyStiffness;
      strengthScale1 *= 1.f+(1.f-scale)*kMultOnLoose;

      body.setStiffness(spineStiffness*strengthScale1, spineDamping*scale, bvmask_Spine1 | bvmask_Spine2 | bvmask_Spine3, &stiff1);

#if NM_STAGGERSHOT
      m_spineStrengthScale = strengthScale1;
      m_spineDampingScale = scale;
      m_spineMuscleStiffness = stiff1;
#endif
    }

    float scale4B = scale;
    if (minLegsLooseness > 0.f)
      scale4B = rage::Clamp(scale,minLegsLooseness,2.f);
    float stiff4B = scale4B;
    // lower body MUSCLE STRENGTH and gravity opposition
    //----------------------------------------------------------------------------------------------------------
    float lbsLeft = dynamicBalancerTask->getLeftLegStiffness();
    float lbsRight = dynamicBalancerTask->getRightLegStiffness();

    //left leg
    float lowerStrength = 5.f + (lbsLeft-5.f)*lowerBodyStiffness;
    float kneeStiffnessL = lowerStrength;
    float kneeMStiffnessL = 1.5f*stiff4B;
    float hipStiffnessL = lowerStrength;
    float hipMStiffnessL = 1.5f*stiff4B;
    lowerStrength = 5.f + (lbsRight-5.f)*lowerBodyStiffness;
    float kneeStiffnessR = lowerStrength;
    float kneeMStiffnessR = 1.5f*stiff4B;
    float hipStiffnessR = lowerStrength;
    float hipMStiffnessR = 1.5f*stiff4B;

    body.getLeftLegInputData()->getKnee()->setMuscleStiffness(kneeMStiffnessL);
    body.getRightLegInputData()->getKnee()->setMuscleStiffness(kneeMStiffnessR);

    body.getLeftLegInputData()->getHip()->setMuscleStiffness(hipMStiffnessL);
    body.getRightLegInputData()->getHip()->setMuscleStiffness(hipMStiffnessR);

    //taper knee strength
    //add to stiffness if loose
    if (dynamicBalancerTask->isActive())
    {
      float strengthScale4B = 0.5f + 0.5f*lowerBodyStiffness;
      strengthScale4B *= 1.f+(1.f-scale4B)*kMult4Legs;

      float stiffness = dynamicBalancerTask->getLeftLegStiffness()*strengthScale4B;
      float strength = stiffness * stiffness;

      body.getLeftLegInputData()->getHip()->setMuscleStrength(strength);

      hipStiffnessL = stiffness;
      //float kneeStrength = m_kneeStrength * m_leftLegStiffness; // ie the kneeStrength is multiplier on bodyStiffness
      if (dynamicBalancerTask->getTaperKneeStrength() && dynamicBalancerTask->footState() != NmRsCBUDynBal_FootState::kLeftStep)//if (m_footState.state.m_leftFootBalance)
      {
        float angle = -(getLeftLegSetup()->getKnee()->getActualAngle());
        strength *= rage::Clamp( (1.f - 2.f*(angle-1.5f)/(PI - 1.5f)), 0.2f, 1.f);
      }

      body.getLeftLegInputData()->getKnee()->setMuscleStrength(strength);

      kneeStiffnessL = rage::SqrtfSafe(strength);

      stiffness = dynamicBalancerTask->getRightLegStiffness()*strengthScale4B;
      strength = stiffness * stiffness;

      body.getRightLegInputData()->getHip()->setMuscleStrength(strength);

      hipStiffnessR = stiffness;
      if (dynamicBalancerTask->getTaperKneeStrength() && dynamicBalancerTask->footState() != NmRsCBUDynBal_FootState::kRightStep)//if (m_footState.state.m_leftFootBalance)
      {
        float angle = -(getRightLegSetup()->getKnee()->getActualAngle());
        //float strength = kneeStrength * (m_taperKneeStrength ? rage::Clamp( (1.f - 2.f*(angle-1.5f)/(PI - 1.5f)), 0.2f, 1.f) : 1.f);
        strength *= rage::Clamp( (1.f - 2.f*(angle-1.5f)/(PI - 1.5f)), 0.2f, 1.f);
      }

      body.getRightLegInputData()->getKnee()->setMuscleStrength(strength);

      kneeStiffnessR = rage::SqrtfSafe(strength);
    }

    //mmmmmhere test allowing this if bcr
    if (!staggerFallTask->isActive() && !(balColReactTask->isActive() && (balColReactTask->m_balancerState != bal_Normal)))
    {
      //mmmmOpposeGravity
      //mmmmmHere this overwrites the balancer values set in configure balance
      //  A quick fix would be to not allow this unless lowerBodyStiffness or upperBodyStiffness is < 1 
      if(upperBodyStiffness < 1.f)
      {
        body.getLeftLegInputData()->getHip()->setOpposeGravity(opposeGravStiffness);
        body.getLeftLegInputData()->getKnee()->setOpposeGravity(opposeGravStiffness);

        body.getRightLegInputData()->getHip()->setOpposeGravity(opposeGravStiffness);
        body.getRightLegInputData()->getKnee()->setOpposeGravity(opposeGravStiffness);
      }
      if(lowerBodyStiffness < 1.f)
      {
        body.getLeftLegInputData()->getAnkle()->setOpposeGravity(2.f*lowerBodyStiffness);
        body.getRightLegInputData()->getAnkle()->setOpposeGravity(2.f*lowerBodyStiffness);
      }

      float balTimer = dynamicBalancerTask->getTimer();
      float balMaximumBalanceTime = dynamicBalancerTask->getMaximumBalanceTime();

      float lbsLeft = dynamicBalancerTask->getLeftLegStiffness();
      float lbsRight = dynamicBalancerTask->getRightLegStiffness();
      float stiffnessReductionTime;

      //left leg
      float lowerStrength = 5.f + (lbsLeft-5.f)*lowerBodyStiffness;
      float lowerDamping = 0.5f + (1.f-0.5f)*lowerBodyStiffness;
      float lowerDampingHip = lowerDamping;
      if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)
        lowerDampingHip *= dynamicBalancerTask->getLeftLegSwingDamping();
      float ankleStrength = 3.f + (lbsLeft-3.f)*lowerBodyStiffness;

      bool weakenBalancer = (balTimer>balMaximumBalanceTime && !dynamicBalancerTask->getBalanceIndefinitely());
      if (weakenBalancer)
      {
        stiffnessReductionTime = 1.f/(dynamicBalancerTask->getFallMult()*0.4f*(balTimer - balMaximumBalanceTime)+1.f);
        lowerStrength = rage::Min(lowerStrength,stiffnessReductionTime*lbsLeft);
        ankleStrength = rage::Min(ankleStrength,stiffnessReductionTime*lbsLeft);

        body.getLeftLegInputData()->getAnkle()->setStiffness(ankleStrength,1.f);
        body.getLeftLegInputData()->getHip()->setStiffness(lowerStrength,1.f);
        stiffnessReductionTime = 1.f/(dynamicBalancerTask->getFallMult()*0.3f*(balTimer - balMaximumBalanceTime)+1.f);
        lowerStrength = rage::Min(lowerStrength,stiffnessReductionTime*lbsLeft);
        body.getLeftLegInputData()->getKnee()->setStiffness(lowerStrength,1.f);
      }
      else if(lowerBodyStiffness < 1.f)
      {
        body.getLeftLegInputData()->getHip()->setStiffness(rage::Min(lowerStrength,hipStiffnessL), lowerDampingHip);
        body.getLeftLegInputData()->getKnee()->setStiffness(rage::Min(lowerStrength,kneeStiffnessL), lowerDamping);
        body.getLeftLegInputData()->getAnkle()->setStiffness(ankleStrength, 1);
      }

      //right leg
      lowerStrength = 5.f + (lbsRight-5.f)*lowerBodyStiffness;
      lowerDamping = 0.5f + (1.f-0.5f)*lowerBodyStiffness;
      lowerDampingHip = lowerDamping;
      if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
        lowerDampingHip *= dynamicBalancerTask->getLeftLegSwingDamping();
      ankleStrength = 3.f + (lbsRight-3.f)*lowerBodyStiffness;

      if (weakenBalancer)
      {
        stiffnessReductionTime = 1.f/(dynamicBalancerTask->getFallMult()*0.6f*(balTimer - balMaximumBalanceTime)+1.f);
        lowerStrength = rage::Min(lowerStrength,stiffnessReductionTime*lbsRight);
        ankleStrength = rage::Min(ankleStrength,stiffnessReductionTime*lbsRight);

        body.getRightLegInputData()->getAnkle()->setStiffness(ankleStrength,1.f);
        body.getRightLegInputData()->getHip()->setStiffness(lowerStrength,1.f);
        stiffnessReductionTime = 1.f/(dynamicBalancerTask->getFallMult()*0.4f*(balTimer - balMaximumBalanceTime)+1.f);
        lowerStrength = rage::Min(lowerStrength,stiffnessReductionTime*lbsRight);
        body.getRightLegInputData()->getKnee()->setStiffness(lowerStrength,1.f);
      }
      else if(lowerBodyStiffness < 1.f)
      {
        body.getRightLegInputData()->getHip()->setStiffness(rage::Min(lowerStrength,hipStiffnessR), lowerDampingHip);
        body.getRightLegInputData()->getKnee()->setStiffness(rage::Min(lowerStrength,kneeStiffnessR), lowerDamping);
        body.getRightLegInputData()->getAnkle()->setStiffness(ankleStrength, 1);
      }

      //Reduce gravity opposition aswell mmmmhere parameterize and add to balancer
      if (weakenBalancer  && dynamicBalancerTask->getFallReduceGravityComp())
      {
        if (dynamicBalancerTask->isActive())
        {
          stiffnessReductionTime = 1.f/(dynamicBalancerTask->getFallMult()*0.4f*(balTimer - balMaximumBalanceTime)+1.f);
          dynamicBalancerTask->setOpposeGravityLegs(stiffnessReductionTime);
          dynamicBalancerTask->setOpposeGravityAnkles(stiffnessReductionTime);
          dynamicBalancerTask->setLowerBodyGravityOpposition(&body);
        }
      }
    }

    stiff = scale;
    if (minArmsLooseness > 0.f)
      stiff = rage::Clamp(scale,minArmsLooseness,2.f);

    float leftStiffMod = 0.0f;
    float rightStiffMod = 0.0f;
    float leftDampMod = 0.0f;
    float rightDampMod = 0.0f;

    if (injuredLArm && allowInjuredArm)
    {
      leftStiffMod = 0.5f;
      rightStiffMod = -0.5f;
      // limbs note damping modifiers inverted from old code, but think this was a bug.
      leftDampMod = 0.2f;
      rightDampMod = -0.2f;
    }
    if (injuredRArm && allowInjuredArm)
    {
      leftStiffMod = -0.5f;
      rightStiffMod = 0.5f;
      leftDampMod = -0.2f;
      rightDampMod = 0.2f;
    }

    body.setStiffness((armsStiffness+rightStiffMod)*strengthScale, (armsDamping+rightDampMod)*scale, bvmask_ArmLeft, &stiff);
    body.setStiffness((armsStiffness+leftStiffMod)*strengthScale, (armsDamping+leftDampMod)*scale, bvmask_ArmRight, &stiff);

    stiff = scale*1.5f;//default muscle stiffness of neck is 1.5
    if (stableHandsAndNeck)
    {
      stiff = 1.5f;
      body.setStiffness(neckStiffness, neckDamping, bvmask_CervicalSpine, &stiff);
    }
    else
    {
      body.setStiffness(rage::Min(neckStiffness*strengthScale, maxStrength), neckDamping*scale, bvmask_CervicalSpine, &stiff);
    }

    // moved from stableHandsAndNeck block above as we *never* want wrists to participate in
    // shot looseness.
    stiff = 1.f;//New Shot prototype
    body.getLeftArmInputData()->getWrist()->setStiffness(13.f, 1.f, &stiff);
    body.getRightArmInputData()->getWrist()->setStiffness(13.f, 1.f, &stiff);

    // ensure minimum knee stiffness (for stability?)
    if (upperBodyStiffness < .3f)
    {
      body.getLeftLegInputData()->getKnee()->setMuscleStiffness(0.5f + 2.f*upperBodyStiffness);
      body.getRightLegInputData()->getKnee()->setMuscleStiffness(0.5f + 2.f*upperBodyStiffness);
    }

  }

  bool NmRsCharacter::isSupportHandConstraintActive()
  { 
    if(m_handToHandConstraintHandle.IsValid())
    {
      return (static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(m_handToHandConstraintHandle) )) != NULL;
    }
    else
      return false;
  }

#if NM_RUNTIME_LIMITS
#if NM_UNUSED_CODE
  void NmRsCharacter::disableAllLimits()
  {
    for(int i = 0; i < m_effectorCount; ++i)
    {
      NmRsEffectorBase* effector = getEffector(i);
      Assert(effector);
      effector->disableLimits();
    }
  }
#endif
  void NmRsCharacter::restoreAllLimits()
  {
    for(int i = 0; i < m_effectorCount; ++i)
    {
      NmRsEffectorBase* effector = getEffector(i);
      Assert(effector);
      effector->restoreLimits();
    }
  }

  void NmRsCharacter::openAllLimitsToDesired()
  {
    for(int i = 0; i < m_effectorCount; ++i)
    {
      NmRsEffectorBase* effector = getEffector(i);
      Assert(effector);
      effector->setLimitsToPose();
    }
  }
#endif // NM_RUNTIME_LIMITS

  bool NmRsCharacter::getITMForPart(
    int partIndex,
    rage::Matrix34* tm,
    IncomingTransformSource source)
  {
    Assert(tm);
    int incomingComponentCount = 0;

    rage::Matrix34 *itPtr = 0;

    IncomingTransformStatus itmStatusFlags = kITSNone;
    getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, source);
    if (incomingComponentCount == 0 || itPtr == 0)
    {
      //mmmmMP3 uncommented in MP3 tm->Identity();
      //Assert(false);
      return false;
    }

    *tm = itPtr[partIndex];

    return true;
  }

#if 0//Removed because currently unused
  // find jointMatrix1 from current partMatrix of the parent
  void NmRsCharacter::getJointMatrix1FromParent(rage::Matrix34& jointMat1, NmRsEffectorBase* effector, NmRsGenericPart* parent)
  {
    rage::phJoint* joint = effector->getJoint();
    rage::Matrix34 oriParent;
    rage::Vector3  posParent;
    oriParent = joint->GetOrientationParent();
    posParent = joint->GetPositionParent();
    getITMForPart(parent->getPartIndex(), &jointMat1);
    rage::Vector3 pos;
    jointMat1.Transpose();
    jointMat1.UnTransform3x3(posParent, pos);
    jointMat1.Dot3x3(oriParent);
    jointMat1.d += pos;
    jointMat1.Transpose();
#if ART_ENABLE_BSPY && 0
    bspyDrawCoordinateFrame(0.025f, jointMat1);
#endif
  }

  // find jointMatrix2 from current partMatrix of the child
  void NmRsCharacter::getJointMatrix2FromChild(rage::Matrix34& jointMat2, NmRsEffectorBase* effector, NmRsGenericPart* child)
  {
    rage::phJoint* joint = effector->getJoint();
    rage::Matrix34 oriChild;
    rage::Vector3  posChild;
    oriChild = joint->GetOrientationChild();
    posChild = joint->GetPositionChild();
    getITMForPart(child->getPartIndex(), &jointMat2);
    rage::Vector3 pos;
    jointMat2.Transpose();
    jointMat2.UnTransform3x3(posChild, pos);
    jointMat2.Dot3x3(oriChild);
    jointMat2.d += pos;
    jointMat2.Transpose();
#if ART_ENABLE_BSPY && 0
    bspyDrawCoordinateFrame(0.025f, jointMat2);
#endif
  }
#endif//#if 0//Removed because currently unused

  void NmRsCharacter::setFrictionPreScale(float mult, BehaviourMask mask /*= bvmask_Full*/)
  {
#if ART_ENABLE_BSPY
    m_frictionHelper.setPreScale(mult, mask, m_currentBehaviour);
#else
    m_frictionHelper.setPreScale(mult, mask);
#endif
  }

  void NmRsCharacter::setFrictionPostScale(float mult, BehaviourMask mask /*= bvmask_Full*/)
  {
    m_frictionHelper.setPostScale(mult, mask);
  }

  void NmRsCharacter::setStepUpFriction(float left, float right)
  {
    m_frictionHelper.setStepUpFriction(left, right);
  }

  void NmRsCharacter::setElasticityMultiplier(float mult, BehaviourMask mask /* = bvmask_Full */)
  {
    int i;
    for(i = 0; i < m_genericPartCount; ++i)
      if(isPartInMask(mask, i))
        m_parts[i]->setElasticityMultiplier(mult);
  }

} // namespace ART
