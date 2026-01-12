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
      rage::phInst *pInst = getLevel()->GetInstance(levelIndex);//mmmmNote doesn't check if instance is added, only if levelIndex is between 0 and 100.  If levelIndex not existing then produces a matrix of QNANS not NULL
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

    //vector3:subtract(dR, targetPos, sourcePos)
    //vector3:cross(relVel, dR, sourceAngVel)
    //vector3:subtract(relVel, relVel, sourceVel)
    //vector3:add(relVel, relVel, targetVel)
    //vector3:scaleAndAdd(target, targetPos, deltaTime, relVel)  //a=b+c.d

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
    float weightLowerAngle = 0.0f;              // Must have: alpha + beta = 1.0
    float weightUpperAngle = 0.0f;

#ifdef NM_HAS_FSEL_INTRINSIC
    weightLowerAngle = (float)__fsel(angle, neckLowerAngleMax * neckAngleMax, neckLowerAngleMin * neckAngleMin);
    weightUpperAngle = (float)__fsel(angle, neckUpperAngleMax * neckAngleMax, neckUpperAngleMin * neckAngleMin);
#else
    if (angle > 0.0)
    {
      weightLowerAngle = neckLowerAngleMax * neckAngleMax;
      weightUpperAngle = neckUpperAngleMax * neckAngleMax;
    }
    else if (angle < 0.0)
    {
      weightLowerAngle = neckLowerAngleMin * neckAngleMin;
      weightUpperAngle = neckUpperAngleMin * neckAngleMin;
    }
#endif // NM_PLATFORM_X360

    *neckLowerAngle = weightLowerAngle*angle;
    *neckUpperAngle = weightUpperAngle*angle;
  }

  //Reach arm to body part
  //returns armTwistOut and sets reachArm->dist2Target
  float NmRsCharacter::reachForBodyPart(
    NmRsHumanBody* body,
    ReachArm* reachArm,
    bool delayMovement,
    float cleverIKStrength,
    bool /*shrug*/,
    bool useActualAnglesForWrist)
  {

    float armTwistOut;

    Assert(reachArm);
    Assert(reachArm->offset.x == reachArm->offset.x);
    Assert(reachArm->normal.x == reachArm->normal.x);

#if ART_ENABLE_BSPY
    bspyLogf(info, L"REACHING");
#endif

    NmRsArmInputWrapper* reachArmInput;
    NmRsHumanArm* reachArmLimb;
    if(reachArm->arm == kLeftArm)
    {
      reachArmInput = body->getLeftArmInputData();
      reachArmLimb = body->getLeftArm();
    }
    else
    {
      reachArmInput = body->getRightArmInputData();
      reachArmLimb = body->getRightArm();
    }


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
    float normalAdd = -0.05f;// 0.05 = push into wound a little
    //the distance < 0.45f is because a shot to the top of the head could not be reached with an offset
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "rfbp", avoid);
    bspyScratchpad(getBSpyID(), "rfbp", outOfRange);
    bspyScratchpad(getBSpyID(), "rfbp", distance);
#endif 
    if ((distance > 0.15f && distance < 0.45f) || (avoid && !outOfRange /*&& (distance2 > 0.1f)*/) )//distance 0.15 cleverWrist starts at 0.2
    {
      normalAdd = rage::Min(distance*0.5f, 0.25f); 
      handPos = grabPoint + pushNormal*normalAdd;
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
        handPos += 12.f*pelvis2hip*normalAdd;
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

    const float twistOffset = 0.0f;
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
      ikInputData->setTwist(armTwist - twistOffset);
    ikInputData->setDragReduction(0.0f);
    ikInputData->setWristTarget(grabPoint);
    ikInputData->setWristNormal(worldNormal);
    ikInputData->setWristUseActualAngles(useActualAnglesForWrist);

    reachArmLimb->postInput(ikInput);

    float tempDistance = 0.f;
    //loosen the wrist if using cleverHandIK helper forces (stops the wrist fighting with the forces causing jitter)
    if (cleverHandIK(handPos, reachArmLimb->getHand(), reachArm->direction, false, cleverIKStrength, NULL, tempDistance, pPart, -1, 0.1f,0))

    reachArmInput->getWrist()->setStiffness(10.f,1.f);

    return armTwistOut;
  }

  void NmRsCharacter::blendWithDesiredPose(NmRs3DofEffector* effector, float lean1, float lean2, float twist, float blendFactor)
  {
    nmrsSetAngles(effector, 
      blendToSpecifiedPose(nmrsGetDesiredLean1(effector), lean1, blendFactor),
      blendToSpecifiedPose(nmrsGetDesiredLean2(effector), lean2, blendFactor),
      blendToSpecifiedPose(nmrsGetDesiredTwist(effector), twist, blendFactor));
  }

  void NmRsCharacter::blendWithDesiredPose(NmRs1DofEffector* effector, float angle, float blendFactor)
  {
    nmrsSetAngle(effector, 
      blendToSpecifiedPose(nmrsGetDesiredAngle(effector), angle, blendFactor));
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
        bool steppingUpTo = false;
        rage::Vector3 steppingTo = rightLeg->getFoot()->getPosition() - leftLeg->getFoot()->getPosition();;
        rage::Vector3 comVelLevelled = m_COMvel;
        levelVector(comVelLevelled);
        if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep) 
        {
          //will give force when say walking along a curb with one foot down one foot up.
          // perhaps steppingUp &= probe leftToProbeRight is in dir of velocity?  Stil probs with that
          steppingUp = dynamicBalancerTask->m_roPacket.m_leftFootProbeHitPos.z > dynamicBalancerTask->m_roPacket.m_rightFootProbeHitPos.z + 0.1f;              
          steppingUpTo = leftLeg->getFoot()->getPosition().z + 0.05f < rightLeg->getFoot()->getPosition().z;              
          levelVector(steppingTo);
          steppingUpTo &= steppingTo.Dot(comVelLevelled) > 0.1f;
        }
        else if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
        {
          steppingUp = dynamicBalancerTask->m_roPacket.m_rightFootProbeHitPos.z > dynamicBalancerTask->m_roPacket.m_leftFootProbeHitPos.z + 0.1f;              
          steppingUpTo = rightLeg->getFoot()->getPosition().z + 0.05f < leftLeg->getFoot()->getPosition().z;              
          steppingTo *= -1.f;
          levelVector(steppingTo);
          steppingUpTo &= steppingTo.Dot(comVelLevelled) > 0.1f;
        }        
        if (steppingUp)
        {
          spine->getSpine3Part()->applyForce(m_gUp*getTotalMass()*m_uprightConstraint.stepUpHelp);
        }

        if (steppingUpTo)
        {
          //spine->getSpine3Part()->applyForce(m_gUp*getTotalMass()*m_uprightConstraint.torqueDamping);
        }

        //if steppingUpTo is trigger for a particular step then apply frictionMultiplier of 0 
        //  to the whole of that step
        getRightLegSetup()->getFoot()->setFrictionMultiplier(1.f);
        getLeftLegSetup()->getFoot()->setFrictionMultiplier(1.f);
        //if (steppingUpToStep)
        {
          //spine->getSpine3Part()->applyForce(m_gUp*getTotalMass()*m_uprightConstraint.torqueDamping);
          if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kRightStep)
            getRightLegSetup()->getFoot()->setFrictionMultiplier(0.f);
          if (dynamicBalancerTask->footState() == NmRsCBUDynBal_FootState::kLeftStep)
            getLeftLegSetup()->getFoot()->setFrictionMultiplier(0.f);

        }
#if ART_ENABLE_BSPY
        bool ID = steppingUp;
        bspyScratchpad(getBSpyID(), "ID_HelpStepUp", ID);
        ID = steppingUpTo;
        bspyScratchpad(getBSpyID(), "ID_HelpStepUpTo", ID);
#endif
      }
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

	  // Abandon this mode if the character isn't balanced
	  NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuTaskManager->getTaskByID(getID(), bvid_dynamicBalancer);
	  if(dynamicBalancerTask->m_failedIfDefaultFailure)
	  {
		  m_uprightConstraint.m_uprightPelvisPosition.Zero();
		  return;
	  }

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
      rage::Vector3 force = strength*(projCOM  + (m_COMvel - m_floorVelocity)*legAheadTime - footCentre) + damping*(m_COMvel - footCentreVel);      force *= getLastKnownUpdateStep()*60.f;             // normalize with respect to frame rate
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
  void NmRsCharacter::updateReEnableCollision(){
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
  * if NM_NEWCLEVERHANDIK 1, target should always be in word space even if instanceIndex != -1 
  * if NM_NEWCLEVERHANDIK 0, it doesn't matter because this parameter is not used
  */
  bool NmRsCharacter::cleverHandIK(rage::Vector3 &target,NmRsGenericPart* endPart, float direction,
    bool useHardConstraint, float strength, rage::phConstraintHandle *constraint,float &maxDistance,
    NmRsGenericPart *woundPart,int instanceIndex, float threshold, int partIndex)
  {
    bool applyingForce = false;
    rage::phConstraintHandle *cPtr = 0;
    if (constraint)
      cPtr = constraint;
    bool existingConstraint = cPtr && cPtr->IsValid();
    //release an existing constraint if useHardConstraint=false
    if (existingConstraint && (!useHardConstraint))
      ReleaseConstraintSafetly(*cPtr);
    existingConstraint = cPtr && cPtr->IsValid();

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
    if (useHardConstraint && (distance < threshold)) 
    {
      Assert(constraint);
      // if constraint isn't created yet, do it now
      if (!existingConstraint)
      {
        maxDistance = distance;
        if (instanceIndex == -1)
        {
          // fix to world?
#if NM_NEWCLEVERHANDIK
          rage::Vector3 toGo = target - handPos;
          toGo.Normalize();
          rage::Vector3 newTarget = handPos;
          toGo.Scale(0.6f*getLastKnownUpdateStep());
          newTarget.Add(toGo);
          if (target.Dist(handPos)<0.6f*getLastKnownUpdateStep())
          {
            newTarget=target;
          }
#if ART_ENABLE_BSPY
          bspyDrawPoint(newTarget, 0.1f, rage::Vector3(1,0,1));
          bspyScratchpad(getBSpyID(), "grab.first", newTarget);
          bspyScratchpad(getBSpyID(), "grab.first", target);
          bspyScratchpad(getBSpyID(), "grab.first", toGo);
#endif
          fixPart(endPart->getPartIndex(), newTarget, handPos, distance, *constraint);
#else
          fixPart(endPart->getPartIndex(), target, handPos, distance, *constraint);
#endif
        }
        else
        {
          // fix to environment part?
#if NM_NEWCLEVERHANDIK
          rage::Vector3 toGo = target - handPos;
          toGo.Normalize();
          rage::Vector3 newTarget = handPos;
          toGo.Scale(0.6f*getLastKnownUpdateStep());
          newTarget.Add(toGo);
          if (target.Dist(handPos)<0.6f*getLastKnownUpdateStep())
          {
            newTarget=target;
          }
#if ART_ENABLE_BSPY
          bspyDrawPoint(newTarget, 0.1f, rage::Vector3(1,0,1));
          bspyScratchpad(getBSpyID(), "grab", newTarget);
          bspyScratchpad(getBSpyID(), "grab", target);
          bspyScratchpad(getBSpyID(), "grab", toGo);
#endif
          fixPartsTogether2(endPart->getPartIndex(),partIndex, distance, handPos, newTarget, instanceIndex, *constraint, true);
#else

          fixPartsTogether2(endPart->getPartIndex(),partIndex, distance, handPos, target, instanceIndex, *constraint, true);
#endif
        }
      }
    }
    // if constraint already exists, just update it
    if (existingConstraint)
    {
#if NM_NEWCLEVERHANDIK
      /* NEW METHOD : Modifications because apparently hard constraint is broken  */
      //slowly moves constraint from the hand to the real target
      if (instanceIndex == -1)
      {
        rage::Vector3 toGo = target - handPos;
        toGo.Normalize();
        rage::Vector3 newTarget = handPos;
        toGo.Scale(0.6f*getLastKnownUpdateStep());
        newTarget.Add(toGo);
        if (target.Dist(handPos)<0.6f*getLastKnownUpdateStep())
        {
          newTarget=target;
          toGo = target - handPos;
        }
#if ART_ENABLE_BSPY
        bspyDrawPoint(newTarget, 0.1f, rage::Vector3(1,0,1));
        bspyScratchpad(getBSpyID(), "grab", newTarget);
        bspyScratchpad(getBSpyID(), "grab", target);
        bspyScratchpad(getBSpyID(), "grab", toGo);
#endif
        cPtr->MoveWorldPosition(toGo);
      }
      else
      {
        rage::Vector3 localHandPos;
        instanceToLocalSpace(&localHandPos,handPos,instanceIndex);
        rage::Vector3 localTarget;
        instanceToLocalSpace(&localTarget,target,instanceIndex);
        rage::Vector3 toGo = localTarget - localHandPos;
        toGo.Normalize();
        rage::Vector3 newTarget = localHandPos;
        toGo.Scale(0.6f*getLastKnownUpdateStep());
        newTarget.Add(toGo);
        if (localTarget.Dist(localHandPos)<0.6f*getLastKnownUpdateStep())
        {
          newTarget=localTarget;
          toGo = localTarget - localHandPos;
        }
#if ART_ENABLE_BSPY
        bspyScratchpad(getBSpyID(), "grab.locally", newTarget);
        bspyScratchpad(getBSpyID(), "grab.locally", target);
        bspyScratchpad(getBSpyID(), "grab.locally", localTarget);
        bspyScratchpad(getBSpyID(), "grab.locally", toGo);
#endif
        cPtr->SetPositionB(newTarget);
      }
      updateConstraintSeparation(*constraint, 0.f);
#else
      // slowly moves constraint distance from initial to 0, to safely glue hand to target
      maxDistance = rage::Clamp(maxDistance - 0.6f*getLastKnownUpdateStep(), 0.0f, distance);
      updateConstraintSeparation(*constraint, maxDistance);
#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "cleverConstraint", maxDistance);
#endif
#endif
    }
    return applyingForce;
  }

  bool NmRsCharacter::hasCollidedWithWorld(BehaviourMask mask)
  {
    bool collided = false;
    for (int i = 0; i < getNumberOfParts(); i++)
      if(isPartInMask(mask, i))
        collided |= getGenericPartByIndex(i)->collidedWithNotOwnCharacter();
#if NM_FAST_COLLISION_CHECKING & 0
    bool result = 0;
    result = hasCollidedWithOtherCharacters(mask) || hasCollidedWithEnvironment(mask);
    Assert(result == collided);
#endif
    return collided;
  }

  bool NmRsCharacter::hasCollidedWithOtherCharacters(BehaviourMask mask)
  {
    bool collided = false;
    for (int i = 0; i < getNumberOfParts(); i++)
      if(isPartInMask(mask, i))
        collided |= getGenericPartByIndex(i)->collidedWithOtherCharacter();
#if NM_FAST_COLLISION_CHECKING & 0
    bool result = 0;
    result = (m_collidedOtherCharactersMask & mask) != bvmask_None;
    Assert(result == collided);
#endif
    return collided;
  }

  bool NmRsCharacter::hasCollidedWithEnvironment(BehaviourMask mask)
  {
    bool collided = false;
    for (int i = 0; i < getNumberOfParts(); i++)
      if(isPartInMask(mask, i))
        collided |= getGenericPartByIndex(i)->collidedWithEnvironment();
#if NM_FAST_COLLISION_CHECKING & 0
    bool result = 0;
    result = (m_collidedEnvironmentMask & mask) != bvmask_None;
    Assert(result == collided);
#endif
    return collided;
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
        rage::phArticulatedBodyPart &part = body->GetLink(i);
        totalKE += 0.5f * getPartMass(part) * (VEC3V_TO_VECTOR3(body->GetLinearVelocityNoProp(i)) - m_floorVelocity).Mag2(); // NoProp
        totalMass += getPartMass(part);
        rage::Vector3 inertia = VEC3V_TO_VECTOR3(part.GetAngInertia());
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
  * Fixes a part to its current position. First parameter is part index (integer, zero-based index),
  * second argument is the location of the pin constraint in world space. Optional third arguement allows different attachment point in world
  */
  void NmRsCharacter::fixPart(int component, rage::Vector3 &pos, rage::Vector3 &pos2, float partSeparation, rage::phConstraintHandle &constraint, bool increaseArmMass)
  {
    // Setting this <= 0.0 would be setting it to the internal seperation internally
    if (partSeparation <= 0.0f)
      partSeparation = 0.001f;

    rage::phConstraintDistance::Params distanceConstraint;
    distanceConstraint.instanceA = getFirstInstance();
    distanceConstraint.componentA = (rage::u16) component;
    distanceConstraint.worldAnchorA = VECTOR3_TO_VEC3V(pos);
    distanceConstraint.worldAnchorB = VECTOR3_TO_VEC3V(pos2);
    distanceConstraint.minDistance = 0.0f;
    distanceConstraint.maxDistance = partSeparation;
    distanceConstraint.allowedPenetration = 0.0f;
    constraint = getSimulator()->GetConstraintMgr()->Insert(distanceConstraint);

	  // Check if the arm mass/stiffness needs to be increased to improve the constraint stability
	  if (increaseArmMass && (component == gtaHand_Left || component == gtaHand_Right))
	  {
		  AlterArmMasses(component == gtaHand_Left ? 1 : 2, true);

		  // Set the breaking strength
		  static float breakStrength = 40.0f;
		  rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(constraint) );
		  if (distConstraint)
			  distConstraint->SetBreakable(true, breakStrength);
	  }
  }

  /**
  * OVERVIEW
  * Fixes two parts together. First parameter is part 1 index (integer, zero-based index),
  * second parameter is part 2 index (integer, zero-based index), third parameter
  * is separation (float), the fourth parameter is world space part 1 constraint position,
  * the fifth parameter is world space part 2 constraint position. The 6th param
  * levelIndex is optional, it looks up an instance set in-game.
  */
  void NmRsCharacter::fixPartsTogether2(int component1, int component2, float partSeparation, rage::Vector3 &worldPos1, rage::Vector3 &worldPos2, int inputlevelIndex, rage::phConstraintHandle &constraint, bool increaseArmMass)
  {
    // Setting this <= 0.0 would be setting it to the internal seperation internally
    if (partSeparation <= 0.0f)
      partSeparation = 0.001f;

    int levelIndex = inputlevelIndex;
    Assert(levelIndex>-1);
    rage::phInst *pInst = NULL;
    // Extra level of checks to prevent crashes caused by trying to grab objects
    // if the instance we're trying to grab doesn't exist anymore don't attach
    if (IsInstValid_NoGenIDCheck(levelIndex))
      pInst = getEngine()->getLevel()->GetInstance(levelIndex);
    else
        return;
    Assert(pInst);

    rage::phConstraintDistance::Params distanceConstraint;
    distanceConstraint.instanceA = getFirstInstance();
    distanceConstraint.instanceB = pInst;
    distanceConstraint.componentA = (rage::u16) component1;
    distanceConstraint.componentB = (rage::u16) component2;
    distanceConstraint.worldAnchorA = VECTOR3_TO_VEC3V(worldPos1);
    distanceConstraint.worldAnchorB = VECTOR3_TO_VEC3V(worldPos2);
    distanceConstraint.minDistance = 0.0f;
    distanceConstraint.maxDistance = partSeparation;
    distanceConstraint.allowedPenetration = 0.0f;
    constraint = getSimulator()->GetConstraintMgr()->Insert(distanceConstraint);

	  // Check if the arm mass/stiffness needs to be increased to improve the constraint stability
	  if (increaseArmMass && getFirstInstance() != pInst && (component1 == gtaHand_Left || component1 == gtaHand_Right))
	  {
		  AlterArmMasses(component1 == gtaHand_Left ? 1 : 2, true);

		  // Set the breaking strength
		  static float breakStrength = 40.0f;
		  rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(constraint) );
		  if (distConstraint)
			  distConstraint->SetBreakable(true, breakStrength);
	  }
  }

  // 1 for left, 2 for right
  void NmRsCharacter::AlterArmMasses(int iType, bool increase)
  {
	  // Check for valid input
	  if (!(
		  (iType == 1 && increase && m_LeftArmMassIncreased == false) || 
		  (iType == 2 && increase && m_RightArmMassIncreased == false) ||
		  (iType == 1 && !increase && m_LeftArmMassIncreased == true) ||
		  (iType == 2 && !increase && m_RightArmMassIncreased == true)))
	  {
		  return;
	  }

	  // Update the state
	  if (iType == 1)
		  m_LeftArmMassIncreased = increase;
	  else
		  m_RightArmMassIncreased = increase;

	  // Alter the arm mass
	  static float massMultHandMax = 5.0f;
	  static float massMultArmMax = 3.0f;
	  float massMultHand = increase ? massMultHandMax : 1.0f / massMultHandMax;
	  float massMultArm = increase ? massMultArmMax : 1.0f / massMultArmMax;
	  int component = iType == 1 ? gtaHand_Left : gtaHand_Right;
	  rage::phArticulatedBody *body = getArticulatedBody();
	  float mass = body->GetLink(component).GetMass().Getf();
	  body->GetLink(component).SetMassAndAngInertia(massMultHand*mass, 
		  massMultHand*VEC3V_TO_VECTOR3(body->GetLink(component).GetAngInertia()));
	  mass = body->GetLink(component-1).GetMass().Getf();
	  body->GetLink(component-1).SetMassAndAngInertia(massMultArm*mass, 
		  massMultArm*VEC3V_TO_VECTOR3(body->GetLink(component-1).GetAngInertia()));
	  mass = body->GetLink(component-2).GetMass().Getf();
	  body->GetLink(component-2).SetMassAndAngInertia(massMultArm*mass, 
		  massMultArm*VEC3V_TO_VECTOR3(body->GetLink(component-2).GetAngInertia()));
	  body->CalculateInertias();

	  // Up the min stiffness if attaching, otherwise revert to default min stiffness
	  static float minStiff = 0.8f;
	  int jointIndex = (component == gtaHand_Left) ? gtaJtWrist_Left : gtaJtWrist_Right;
	  body->GetJoint(jointIndex).SetMinStiffness(increase ? minStiff : 0.1f);
  }

  /**
  * updates the separation of constraints
  */
  void NmRsCharacter::updateConstraintSeparation(rage::phConstraintHandle &con, float seperation)
  {
    if (con.IsValid())
    {
      rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(con) );
      if (distConstraint)
        distConstraint->SetMaxDistance(seperation);
    }
  }

#if 0
  void NmRsCharacter::matchClavicleToShoulderBetter(NmRs3DofEffector *clavicle, NmRs3DofEffector *shoulder)
  {
#ifdef NM_PLATFORM_X360

    // fsel instead of fp branch, plus half number of divides

    float totalLean1 = clavicle->getActualLean1() + shoulder->getDesiredLean1();
    float totalLean2 = clavicle->getActualLean2() + shoulder->getDesiredLean2();

    float totalMaxL1 = 1.0f / (clavicle->getMaxLean1() + shoulder->getMaxLean1());
    float totalMinL1 = 1.0f / (clavicle->getMinLean1() + shoulder->getMinLean1());
    float totalMaxL2 = 1.0f / (clavicle->getMaxLean2() + shoulder->getMaxLean2());
    float totalMinL2 = 1.0f / (clavicle->getMinLean2() + shoulder->getMinLean2());

    float gte_clavL1 = totalLean1 * (clavicle->getMaxLean1()) * totalMaxL1;
    float gte_shldL1 = totalLean1 * (shoulder->getMaxLean1()) * totalMaxL1;
    float gte_clavL2 = totalLean2 * (clavicle->getMaxLean2()) * totalMaxL2;
    float gte_shldL2 = totalLean2 * (shoulder->getMaxLean2()) * totalMaxL2;

    float lt_clavL1 = totalLean1 * (clavicle->getMinLean1()) * totalMinL1;
    float lt_shldL1 = totalLean1 * (shoulder->getMinLean1()) * totalMinL1;
    float lt_clavL2 = totalLean2 * (clavicle->getMinLean2()) * totalMinL2;
    float lt_shldL2 = totalLean2 * (shoulder->getMinLean2()) * totalMinL2;

    clavicle->setDesiredLean1( (float) __fsel(totalLean1, gte_clavL1, lt_clavL1) );
    shoulder->setDesiredLean1( (float) __fsel(totalLean1, gte_shldL1, lt_shldL1) );

    clavicle->setDesiredLean2( (float) __fsel(totalLean2, gte_clavL2, lt_clavL2) );
    shoulder->setDesiredLean2( (float) __fsel(totalLean2, gte_shldL2, lt_shldL2) );

#else

    float totalLean1 =  clavicle->getActualLean1() + shoulder->getDesiredLean1();
    if (totalLean1 > 0 )
    {
      float totalMax = clavicle->getMaxLean1() + shoulder->getMaxLean1();
      nmrsSetLean1(clavicle, totalLean1 * (clavicle->getMaxLean1())/totalMax);
      nmrsSetLean1(shoulder, totalLean1 * (shoulder->getMaxLean1())/totalMax);
    }
    else
    {
      float totalMin = clavicle->getMinLean1()+shoulder->getMinLean1();
      nmrsSetLean1(clavicle, totalLean1 * (clavicle->getMinLean1())/totalMin);
      nmrsSetLean1(shoulder, totalLean1 * (shoulder->getMinLean1())/totalMin);
    }

    float totalLean2 =  clavicle->getActualLean2()+shoulder->getDesiredLean2();
    if (totalLean2 > 0 )
    {
      float totalMax = clavicle->getMaxLean2()+shoulder->getMaxLean2();
      nmrsSetLean2(clavicle, totalLean2 * (clavicle->getMaxLean2())/totalMax);
      nmrsSetLean2(shoulder, totalLean2 * (shoulder->getMaxLean2())/totalMax);
    }
    else
    {
      float totalMin = clavicle->getMinLean2()+shoulder->getMinLean2();
      nmrsSetLean2(clavicle, totalLean2 * (clavicle->getMinLean2())/totalMin);
      nmrsSetLean2(shoulder, totalLean2 * (shoulder->getMinLean2())/totalMin);
    }

#endif // NM_PLATFORM_X360
  }

  void NmRsCharacter::matchClavicleToShoulderUsingTwist(NmRs3DofEffector *clavicle, NmRs3DofEffector *shoulder)
  {
    float lean1XS =  shoulder->getDesiredLean1() - shoulder->getMaxLean1();
    if (lean1XS > 0.0f )
    {
      nmrsSetTwist(clavicle, -lean1XS);
    }
    else
    {
      float lean1XS =  shoulder->getDesiredLean1() - shoulder->getMinLean1();
      if (lean1XS < 0.0f )
      {
        nmrsSetTwist(clavicle, -lean1XS);
      }
      else
      {
        nmrsSetTwist(clavicle, 0.0f);
      }
    }
  }
#endif

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
      m_probeHitComponent[rayPrbeIndex] = isect.GetComponent();
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

    getResultsFromCollisionsOnly  = ((rayPrbeIndex == pi_balLeft || rayPrbeIndex == pi_balRight) && m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_catchFall));
    getResultsFromCollisionsOnly = getResultsFromCollisionsOnly || 
      ((rayPrbeIndex == pi_balLeft || rayPrbeIndex == pi_catchFallLeft)  && ((highFallTask->isActive() && highFallTask->GetState() == NmRsCBUHighFall::HF_Falling) || m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_learnedCrawl)));       
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
          m_is1stAsyncProbe[rayPrbeIndex] = false;//Set to true in activate of the behaviours that use them so that 1st probe is non-async or if async the result is not rquested for the 1st time
        }
        else
          probeReslt = GetAsynchProbeResult(rayPrbeIndex, 
          &m_probeHitPos[rayPrbeIndex],
          &m_probeHitNormal[rayPrbeIndex],
          &m_probeHitComponent[rayPrbeIndex],
          &m_probeHitInst[rayPrbeIndex],
          &m_probeHitInstLevelIndex[rayPrbeIndex],
          &m_probeHitInstGenID[rayPrbeIndex]);

        if (probeReslt == probe_Hit)
        {
          m_probeHit[rayPrbeIndex] = true;
        }
        else /*if (probeReslt == probe_NoHit)*/  //if there is an error with the probeReslt then call it not hit
          m_probeHit[rayPrbeIndex] = false;

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
      else //i.e. OK ,error or ignored
      {
        m_probeHitLateFrames[rayPrbeIndex] = 0;
        m_probeHitNoProbeInfo[rayPrbeIndex] = 0;
        m_probeHitNoHitTime[rayPrbeIndex] = 0.f;
        m_probeHitResultType[rayPrbeIndex] = probeType_Probe;
      }
      if ((rayPrbeIndex != pi_UseNonAsync) && ((probeReslt != probe_Late) || (m_probeHitLateFrames[rayPrbeIndex] > numLateFramesBeforeSubmitNewProbe && probeReslt == probe_Late)))
      {
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
    //geometry can be old but ray is currrent
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

    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], startPos);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], endPos);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], probeResultStrings[probeReslt]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHit[rayPrbeIndex]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHitPos[rayPrbeIndex]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHitNormal[rayPrbeIndex]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHitInstLevelIndex[rayPrbeIndex]);
    bspyScratchpad(getBSpyID(), probeIndexStrings[rayPrbeIndex], m_probeHitComponent[rayPrbeIndex]);
	
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

    bool dontClearProbe = false;
    dontClearProbe = ((rayPrbeIndex == pi_balLeft || rayPrbeIndex == pi_balRight) && m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_catchFall));
    dontClearProbe = dontClearProbe || 
      ((rayPrbeIndex == pi_balLeft || rayPrbeIndex == pi_catchFallLeft)  && ((highFallTask->isActive() && highFallTask->GetState() == NmRsCBUHighFall::HF_Falling) || m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_learnedCrawl)));

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
          if (intersection)
              {
            intersection = intersection->IsAHit() ? intersection: NULL;
            if (intersection)
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
            }
            else
              return probe_NoHit;
          }
          return probe_NoIntersection;
        }
        else
        {
          // BAD - it took more than a frame to get results
          return probe_Late;
        }
      }
      else
      {
        // BAD - your probe handle is bad
        return probe_HandleInvalid;
      }
    }
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
    bool contacts = hasCollidedWithEnvironment(bvmask_Full);
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
      float partMass = getPartMass(getArticulatedBody()->GetLink(partIndex));\
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
  void NmRsCharacter::antiSpinAroundVerticalAxisJuice(float zAxisSpinReduction)
  {
    float zAxisSpinReductionClamp = rage::Clamp(zAxisSpinReduction,0.f,1.f);
    rage::Vector3 comrvn;
    comrvn = m_COMrotvel;
    float currRotAxisOntoUp = comrvn.Dot(getUpVector());
    rage::Vector3 correctRotVelTorqueAxis = getUpVector();
    correctRotVelTorqueAxis.Normalize();
    correctRotVelTorqueAxis.Scale(-getTotalMass()*(0.2f)*currRotAxisOntoUp*zAxisSpinReductionClamp);

    getSpineSetup()->getPelvisPart()->applyTorque(correctRotVelTorqueAxis);
    getSpineSetup()->getSpine0Part()->applyTorque(correctRotVelTorqueAxis);
    getSpineSetup()->getSpine1Part()->applyTorque(correctRotVelTorqueAxis);
    getSpineSetup()->getSpine2Part()->applyTorque(correctRotVelTorqueAxis);
    getSpineSetup()->getSpine3Part()->applyTorque(correctRotVelTorqueAxis);
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
#define CONSTRAINT_FIX 0 // [jrp] this "fix" does *not* work when constraining two moveable parts!!!
#if CONSTRAINT_FIX
  void NmRsCharacter::constrainSupportHand(const NmRsHumanArm *primaryArm, const NmRsHumanArm *supportArm, rage::Vector3 *constraintPosition, float* /* constraintDistance */, float minimumConstraintDistance, float /* constraintLengthReduction */, float constraintStrength)
#else
  void NmRsCharacter::constrainSupportHand(const NmRsHumanArm *primaryArm, const NmRsHumanArm *supportArm, rage::Vector3 *constraintPosition, float* constraintDistance, float minimumConstraintDistance /* = 0 */, float constraintLengthReduction, float constraintStrength)
#endif
  {
    // If an already active constraint.
    if (m_handToHandConstraintHandle.IsValid())
    {
#if CONSTRAINT_FIX
#if 0
      m_handToHandConstraintPos.Set(*constraintPosition);
      rage::Vector3 lhPos = supportArm->getHand()->getPosition();
      rage::Vector3 toPosition(m_handToHandConstraintPos);
      toPosition.Subtract(lhPos);
      const float maxStep = 0.0025f;
      toPosition.ClampMag(0.f, maxStep);
      rage::Vector3 positionB(lhPos);
      positionB.Add(toPosition);
      if(toPosition.Mag() > 0.f)
        m_handToHandConstraint->SetWorldPositionB(VECTOR3_TO_VEC3V(positionB));
#endif
#else//CONSTRAINT_FIX
      //reduce the constraint distance if the hands have moved closer together
      rage::Vector3 lhPos = supportArm->getHand()->getPosition();
      float posibleSmallerConstraintDistance = m_handToHandConstraintPos.Dist(lhPos);
      float clampMax = 10.f;
      if (posibleSmallerConstraintDistance > minimumConstraintDistance)
        clampMax = posibleSmallerConstraintDistance;
      // slowly reduce the distance that we are away from the desired position.
      m_constraintDistance = rage::Clamp((*constraintDistance - constraintLengthReduction), minimumConstraintDistance, clampMax);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_agentID, "", m_constraintDistance);
      bspyScratchpad(getBSpyID(), "constrainSupportHand", m_constraintDistance);
#endif
      rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_handToHandConstraintHandle) );
      if (distConstraint)
      {
        //update WorldPositionA of the constraint as the support hand is really being set to the gun not the gunHand and gunHandToGun is now changing from current to aiming
        distConstraint->SetWorldPosA(VECTOR3_TO_VEC3V(*constraintPosition));
        distConstraint->SetMaxDistance(m_constraintDistance);
      }

#endif//CONSTRAINT_FIX
    }//if (m_handToHandConstraint)       
    else // otherwise, make a new constraint.
    {
#if CONSTRAINT_FIX
      // create a constraint of zero-length at the current
      // part relationship.
      m_handToHandConstraintPos.Set(*constraintPosition);
      rage::Vector3 lhPos = supportArm->getHand()->getPosition();
      rage::Vector3 rhPos = primaryArm->getHand()->getPosition();
      rage::Vector3 toPosition(m_handToHandConstraintPos);
      toPosition.Subtract(lhPos);
      const float maxStep = 0.01f;
      toPosition.ClampMag(0.f, maxStep);
      rage::Vector3 positionB(m_handToHandConstraintPos);
      //positionB.Subtract(toPosition);
      fixPartsTogether2(
        primaryArm->getHand()->getPartIndex(),
        supportArm->getHand()->getPartIndex(),
        minimumConstraintDistance, //0.f,
        m_handToHandConstraintPos,
        positionB,
        getFirstInstance()->GetLevelIndex(),
        &m_handToHandConstraintHandle);
#else
      // update constraint info
      Assert(constraintDistance && constraintPosition);
      m_constraintDistance = *constraintDistance;
      m_handToHandConstraintPos.Set(*constraintPosition);

      // Create the constraint.
      rage::Vector3 lhPos = supportArm->getHand()->getPosition();
      m_constraintDistance = m_handToHandConstraintPos.Dist(lhPos);

#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "constrainSupportHand", m_handToHandConstraintPos);
#endif

      fixPartsTogether2(
        primaryArm->getHand()->getPartIndex(),
        supportArm->getHand()->getPartIndex(),
        m_constraintDistance,
        m_handToHandConstraintPos,
        lhPos,
        getFirstInstance()->GetLevelIndex(),
        m_handToHandConstraintHandle, false);
      //if (m_handConstraint)//For when the weird parallel pointing and release arm for shot/catchfall code is written
      //  m_handConstraint->SetBreakingStrength(m_breakConstraint);
#endif
    }

    rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_handToHandConstraintHandle) );
#if ART_ENABLE_BSPY
    // draw the constraint radius.
    if (distConstraint)
    {
      rage::Vector3 worldPosA(VEC3V_TO_VECTOR3(distConstraint->GetWorldPosA()));
      rage::Vector3 worldPosB(VEC3V_TO_VECTOR3(distConstraint->GetWorldPosB()));
      // draw a cross representing the radius of the constraint.
      float radius = distConstraint->GetMaxDistance();
      bspyDrawPoint(worldPosB+rage::Vector3(0.0001f, 0.0001f, 0.0001f), 0.05f, rage::Vector3(0,0,1));
      if(radius > 0.f)
        bspyDrawSphere(worldPosA, radius, rage::Vector3(1,0,0));
      bspyDrawPoint(worldPosA, 0.05f, rage::Vector3(1,0,0));
    }
#endif

    if (distConstraint)
    {
      *constraintPosition = VEC3V_TO_VECTOR3(distConstraint->GetWorldPosA());
      if (constraintStrength < -0.01f)
        distConstraint->SetBreakable(false);
      else
        distConstraint->SetBreakable(true, constraintStrength);

    }
  }

  void NmRsCharacter::releaseSupportHandConstraint()
  {
    ReleaseConstraintSafetly(m_handToHandConstraintHandle);
  }



  //Safetly Releases a constraint if it is an active constraint
  void NmRsCharacter::ReleaseConstraintSafetly(rage::phConstraintHandle &constraint)  
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
				  AlterArmMasses(1, false);
			  else if (relevantComponent == gtaHand_Right && m_RightArmMassIncreased)
				  AlterArmMasses(2, false);
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
        snapMagValue *= -1.0f;
        parentEffector = getEffector(gtaJtHip_Left);
        parentEffector2 = getEffector(gtaJtHip_Right);
        childEffector = getEffector(gtaJtSpine_0);
        break;
      case gtaSpine0:
        snapMagValue *= -1.0f;
        parentEffector = getEffector(gtaJtSpine_0);
        childEffector = getEffector(gtaJtSpine_1);
        break;
      case gtaSpine1:
        snapMagValue *= -1.0f;
        parentEffector = getEffector(gtaJtSpine_1);
        childEffector = getEffector(gtaJtSpine_2);
        break;
      case gtaSpine2:
        snapMagValue *= -1.0f;
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
        snapMagValue *= -1.0f;
        parentEffector = getEffector(gtaClav_Left);
        childEffector = getEffector(gtaJtShoulder_Left);
        break;
      case gtaClav_Right:
        snapMagValue *= -1.0f;
        parentEffector = getEffector(gtaClav_Right);
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
          ((NmRs1DofEffector*)getEffector(gtaJtElbow_Left))->ApplyAngImpulse(-snapMagValue*2.f);
          ((NmRs3DofEffector*)getEffector(gtaJtShoulder_Left))->ApplyAngImpulse(-snapMagValue*4.f*getRandomVec(bodyBack, snapDirectionRandomness));
          ((NmRs3DofEffector*)getEffector(gtaJtClav_Jnt_Left))->ApplyAngImpulse(snapMagValue*4.f*getRandomVec(bodyBack, snapDirectionRandomness));
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
    NM_RS_DBG_LOGF(L"Control Stiffness During");
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
    NM_RS_DBG_LOGF(L"strengthSCale: ", controlStiffnessStrengthScale);
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

      if (balTimer>balMaximumBalanceTime && !dynamicBalancerTask->getBalanceIndefinitely())
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

      if (balTimer>balMaximumBalanceTime && !dynamicBalancerTask->getBalanceIndefinitely())
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
      if (balTimer>=balMaximumBalanceTime && !dynamicBalancerTask->getBalanceIndefinitely()  && dynamicBalancerTask->getFallReduceGravityComp())
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
      body.setStiffness(neckStiffness, neckDamping, bvmask_HighSpine, &stiff);
    }
    else
    {
      body.setStiffness(rage::Min(neckStiffness*strengthScale, maxStrength), neckDamping*scale, bvmask_HighSpine, &stiff);
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
  void NmRsCharacter::disableAllLimits()
  {
    for(int i = 0; i < m_effectorCount; ++i)
    {
      NmRsEffectorBase* effector = getEffector(i);
      Assert(effector);
      effector->disableLimits();
    }
  }

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
    NMutils::NMMatrix4 *itPtr = 0;
    IncomingTransformStatus itmStatusFlags = kITSNone;
    getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, source);
    if (incomingComponentCount == 0 || itPtr == 0)
    {
      //mmmmMP3 uncommented in MP3 tm->Identity();
      //Assert(false);
      return false;
    }
    NMutils::NMMatrix4 &tmPel = itPtr[partIndex];
    tm->Set(tmPel[0][0], tmPel[0][1], tmPel[0][2],
      tmPel[1][0], tmPel[1][1], tmPel[1][2],
      tmPel[2][0], tmPel[2][1], tmPel[2][2],
      tmPel[3][0], tmPel[3][1], tmPel[3][2]);
    return true;
  }

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
    joint->GetOrientationChild(oriChild);
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

  void NmRsCharacter::setFrictionMultiplier(float mult, BehaviourMask mask /* = bvmask_Full */)
  {
    int i;
    for(i = 0; i < m_genericPartCount; ++i)
      if(isPartInMask(mask, i))
        m_parts[i]->setFrictionMultiplier(mult);
  }

  void NmRsCharacter::setElasticityMultiplier(float mult, BehaviourMask mask /* = bvmask_Full */)
  {
    int i;
    for(i = 0; i < m_genericPartCount; ++i)
      if(isPartInMask(mask, i))
        m_parts[i]->setElasticityMultiplier(mult);
  }

} // namespace ART
