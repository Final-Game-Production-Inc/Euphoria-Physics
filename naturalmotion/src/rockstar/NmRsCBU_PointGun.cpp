/*
* Copyright (c) 2005-2010 NaturalMotion Ltd. All rights reserved. 
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
#include "NmRsCBU_PointGun.h"
#include "ART/ARTFeedback.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "NmRsEngine.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_AnimPose.h"
//#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_SpineTwist.h"
#include "NmRsCBU_Shot.h"

#include "NmRsIkHelpers.h"

#define HINGE_DIRECTION(arm) arm==getRightArm() ? 1.0f : -1.0f

namespace ART
{
  NmRsCBUPointGun::NmRsCBUPointGun(ART::MemoryManager* services) : CBUTaskBase(services, bvid_pointGun)
  {
    initialiseCustomVariables();
  }

  NmRsCBUPointGun::~NmRsCBUPointGun()
  {
  }

  void NmRsCBUPointGun::initialiseCustomVariables()
  {
    m_mask = bvmask_Arms;

    int i;
    ArmData* armData = NULL;
    for(i = 0; i < 2; ++i)
    {
      armData = &m_armData[i];
      armData->desiredHandToWorld.Identity();
      armData->desiredSupportHandToWorld.Identity();
      armData->handTargetVel.Zero();
      armData->target.Zero();
      armData->lastTarget.Zero();
      armData->targetVel.Zero();
      armData->targetDirection.Zero();
      armData->pointingFromPosition.Zero();
      armData->relaxTimer = 0.0f;
      armData->relaxScale = 1.0f;
      armData->weaponDistance = 0.0f;
      armData->lastHandTargetPos.Zero();
      armData->error = 0.0f;
      armData->fireInhibitTimer = 0.0f;
      armData->status = kUnused;
      armData->desiredStatus = kUnused;
      armData->neutralPoseType = npsNotRequested;
      armData->armStatusChanged = false;
    }

    m_armData[NmRsCharacter::kLeftHand].palmScaleDir = 1;
    m_armData[NmRsCharacter::kRightHand].palmScaleDir = -1;

    m_armData[NmRsCharacter::kLeftHand].arm = 0;
    m_armData[NmRsCharacter::kLeftHand].input = 0;
    m_armData[NmRsCharacter::kLeftHand].inputData = 0;

    m_armData[NmRsCharacter::kRightHand].arm = 0;
    m_armData[NmRsCharacter::kRightHand].input = 0;
    m_armData[NmRsCharacter::kRightHand].inputData = 0;

    m_supportHandToGunHand.Identity();//mmmmShould be in activate
    m_supportHandToGun.Identity();//mmmshould be in activate

    m_weaponMode = kPistolRight;

    m_probeHitSomeThing = false;

    m_parameters.targetValid = false;

    m_rifleConstraint.Reset();
    m_rifleConstraintDistance = 0;

    m_timeWarpScale = 1.0f;
    m_enableLeft = true;
  }

  void NmRsCBUPointGun::onActivate()
  {

    Assert(m_character);
    m_weaponMode = m_character->m_weaponMode;
    m_handAnimationType = haNone;
    updateArmData(-1.0f);

    m_gunArmNotPointingTime = 0.0f;
    m_reConnectTimer = 0.0f;

    m_supportConstraintInhibitTimer = 0.0f;
    m_neutralSidePoseTimer = 0.0f;
    m_forceConstraintMade = false;
    m_rifleFalling = false;
    m_rifleFallingPose = npsNotRequested;
    m_allowFallingSupportArm = true;
    m_decideFallingSupport = false;

    // Estimate the maximum reach of the support arm.  mp3Large is a bit bigger, but the
    // others have just about the same reach.
    m_maxArmReach = 0.65f;
    if( m_character->getBodyIdentifier() == mp3Large)
      m_maxArmReach = 0.75f;

    //Turn off collisions between the hands - the two handed pistol animation has the hands penetrating
    //if m_parameters.disableArmCollisions=true: also disable collisions between right hand/forearm and the torso/legs.
    //if m_parameters.disableRifleCollisions=true and m_weaponMode == kRifle: also disable collisions between right hand/forearm and spine3/spine2.
    setHandCollisionExclusion();

    m_measuredRightHandParentOffset.Zero();
    m_measuredLeftHandParentOffset.Zero();

    //From incoming transforms:
    //Compute m_measuredRightHandParentOffset and m_measuredLeftHandParentOffset
    //and
    //Compute m_supportHandToGun (and m_supportHandToGunHand)
    //to get support position on gun.
    //NB: m_supportHandToGunHand is recalculate each tick in isSupportHandTargetReachable
    if(m_parameters.useIncomingTransforms)
    {
      //This measures the m_parameters.rightHandParentOffset from the itms's
      //it only gives the up offset the other directions are made naturally from the weaponDistance
      //Currently only done for right hand
      //Currently this assumes:
      //  that the parent effector is the right shoulder,
      //  Spine3.x/Spine3Up) is worldUp(z) - this is an ok assumption for all the itms I've seen 
      if (m_parameters.measureParentOffset)
      {
        rage::Matrix34 shoulderItm;
        rage::Matrix34 handITM;
        rage::Matrix34 clavicleITM;
        m_character->getITMForPart(getRightArm()->getHand()->getPartIndex(), &handITM);
        m_character->getITMForPart(getRightArm()->getClaviclePart()->getPartIndex(), &clavicleITM);
        getEffectorMatrix1FromParentPartTM(clavicleITM, getRightArm()->getShoulder(), shoulderItm);
        m_measuredRightHandParentOffset.x += (handITM.d - shoulderItm.d).z;

        m_character->getITMForPart(getLeftArm()->getHand()->getPartIndex(), &handITM);
        m_character->getITMForPart(getLeftArm()->getClaviclePart()->getPartIndex(), &clavicleITM);
        getEffectorMatrix1FromParentPartTM(clavicleITM, getLeftArm()->getShoulder(), shoulderItm);
        m_measuredLeftHandParentOffset.x += (handITM.d - shoulderItm.d).z;
      }


      rage::Matrix34 worldToGunHand, supportHandToWorld;
      m_character->getITMForPart(getRightArm()->getHand()->getPartIndex(), &worldToGunHand);//gunHandToWorld
      m_character->getITMForPart(getLeftArm()->getHand()->getPartIndex(), &supportHandToWorld);
      worldToGunHand.Inverse();
      m_supportHandToGunHand.Dot(supportHandToWorld, worldToGunHand);//supportHandToWorld.worldToGunHand

      //Find supportHandToGun
      rage::Matrix34 worldToGunAiming, handToGun;
      handToGun = m_character->m_gunToHandAiming[NmRsCharacter::kRightHand];
      handToGun.Inverse();
      worldToGunAiming.Dot(worldToGunHand,handToGun);
      m_supportHandToGun.Dot(supportHandToWorld, worldToGunAiming);


    }
    else
    {
      //Set up some good defaults
      if (m_weaponMode == kRifle)
      {
        //rifle
        m_supportHandToGun.a.Set(-0.50882f, -0.118068f, 0.85275f);
        m_supportHandToGun.b.Set(-0.746846f, 0.55321f, -0.36904f);
        m_supportHandToGun.c.Set(-0.428169f, -0.824645f, -0.36966f);
        m_supportHandToGun.d.Set(0.202187f, -0.01494f, -0.0313562f);
        //m_measuredRightHandParentOffset	[0.00319624, 0, 0]
      }
      else//pistols
      {
        m_supportHandToGun.a.Set(-0.480766f, -0.860692f, -0.167617f);
        m_supportHandToGun.b.Set(-0.785369f, 0.507683f, -0.354212f);
        m_supportHandToGun.c.Set(0.389955f, -0.0386521f, -0.920036f);
        m_supportHandToGun.d.Set(-0.0703171f, 0.0198295f, -0.0675663f);

      }
    }

    if (!m_character->m_registerWeaponCalled)
    {
      //Set default transformations between gun and gunHand so that the hand will orientate
      //properly in pointGun if a gunToHand is not sent in - i.e if registerWeapon has not been called 
      //(Identity matrix does not work as gun axes are swapped)
      if (m_weaponMode == kRifle)
      {
        m_character->m_gunToHandAiming[NmRsCharacter::kLeftHand].a.Set(0.0836874f, -0.992887f, 0.0846575f);
        m_character->m_gunToHandAiming[NmRsCharacter::kLeftHand].b.Set(-0.98635f, -0.0946182f, -0.134663f);
        m_character->m_gunToHandAiming[NmRsCharacter::kLeftHand].c.Set(0.141716f, -0.0722336f, -0.987254f);
        m_character->m_gunToHandAiming[NmRsCharacter::kLeftHand].d.Set(0.00314653f, -0.0790734f, -0.0385407f);
        m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].a.Set(0.161185f, -0.972997f, 0.165194f);
        m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].b.Set(-0.977264f, -0.180705f, -0.110807f);
        m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].c.Set(0.137667f, -0.143581f, -0.980005f);
        m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].d.Set(0.00388393f, -0.0772404f, -0.024249f);
        m_character->m_gunToHandCurrent[NmRsCharacter::kLeftHand].Set(m_character->m_gunToHandAiming[NmRsCharacter::kLeftHand]);
        m_character->m_gunToHandCurrent[NmRsCharacter::kRightHand].Set(m_character->m_gunToHandAiming[NmRsCharacter::kRightHand]);
      }
      else//Pistols //NB may be wrong for left pistol
      {
        m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].a.Set(0.0836874f, -0.992887f, 0.0846575f);
        m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].b.Set(-0.98635f, -0.0946182f, -0.134663f);
        m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].c.Set(0.141716f, -0.0722336f, -0.987254f);
        m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].d.Set(0.00314653f, -0.0790734f, -0.0385407f);
        m_character->m_gunToHandCurrent[NmRsCharacter::kLeftHand].Set(m_character->m_gunToHandAiming[NmRsCharacter::kRightHand]);
        m_character->m_gunToHandAiming[NmRsCharacter::kLeftHand].Set(m_character->m_gunToHandAiming[NmRsCharacter::kRightHand]);
        m_character->m_gunToHandCurrent[NmRsCharacter::kRightHand].Set(m_character->m_gunToHandAiming[NmRsCharacter::kRightHand]);
      }
    }

    //#ifdef NM_RS_CBU_ASYNCH_PROBES
    //    m_character->InitializeProbe(NmRsCharacter::pi_pointGun);
    //#endif //NM_RS_CBU_ASYNCH_PROBES
    m_forceNeutral = false;
  }

  void NmRsCBUPointGun::onDeactivate()
  {
    Assert(m_character);
    animationFeedback();
#if ART_ENABLE_BSPY & PointGunBSpyDraw
    m_character->setSkeletonVizMode(NmRsCharacter::kSV_None);
    m_character->setSkeletonVizMask(bvmask_None);
#endif

    // switch off turn in direction 
    if (m_parameters.useTurnToTarget)
    {
      NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      rage::Vector3 toTarget;
      toTarget.Zero();
      dynamicBalancerTask->useCustomTurnDir(false, toTarget);
    }
    if(m_parameters.useSpineTwist)
    {
      NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
      Assert(spineTwistTask);
      spineTwistTask->deactivate();
    }

    m_character->releaseSupportHandConstraint();

    releaseRifleConstraint();

    m_character->setFrictionPreScale(1.0f, bvmask_ArmRight | bvmask_ArmLeft);

    m_character->m_rightHandCollisionExclusion.setB(bvmask_None);
#if NM_RUNTIME_LIMITS
    //Restore limits as much as possible. We assume that neutralPoses and neutralPointing are ok with the normal wrist limits
    m_character->getEffectorDirect(gtaJtWrist_Left)->setLimitsToPose(true);
    m_character->getEffectorDirect(gtaJtWrist_Right)->setLimitsToPose(true);
#endif


    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUPointGun::onTick(float timeStep)
  {

    m_add2WeaponDist = 0.0f;

    m_weaponMode = m_character->m_weaponMode;
    if(m_weaponMode >= kNumWeaponModes)
      m_weaponMode = kPistolRight;

    if (m_character->m_gunToHandConstraintHandle[NmRsCharacter::kRightHand].IsValid())
    {

#if 0//This code is not active:
      //Blend the gunToHand constraint from m_gunToHandCurrent[hand] to m_gunToHandAiming[hand]
      rage::Matrix34 gunToHandBlended = m_character->m_gunToHandAiming[NmRsCharacter::kRightHand];//as a test go straight to the desired gunToHand

      ////Modify the constraint (m_gunToHandConstraintHandle[hand]) between the gun and the hand to be gunToHandBlended
      //rage::phConstraintFixed* fixedConstraint = static_cast<rage::phConstraintFixed*>( PHCONSTRAINT->GetTemporaryPointer(m_character->m_gunToHandConstraintHandle[NmRsCharacter::kRightHand]) );
      //if (fixedConstraint)
      //{
      // // Calling this isn't working - and it might not be needed
      // //fixedConstraint->SetLocalPosA(VECTOR3_TO_VEC3V(gunToHandBlended.d));

      // rage::Quaternion quat;
      // gunToHandBlended.ToQuaternion(quat);
      // fixedConstraint->SetRelativeRotation(QuatV(quat.x, quat.y, quat.z, quat.w));
      //}

      //Decide whether to take m_gunToHandCurrent from the gun level index or from what we think we've set it to 
      m_character->m_gunToHandCurrent[NmRsCharacter::kRightHand] = gunToHandBlended;
#endif
      //if the gun is constrained to the hand and it's level index exists then update m_character->m_gunToHandCurrent
      //This shouldn't need to be done every tick as the gunToHand orientation shouldn't change
      //If the gun has disapeared then m_gunToHandCurrent is remembered as the last good one
      //left
      int gunLevelIndex = m_character->m_leftHandWeapon.levelIndex;
      if (m_character->IsInstValid_NoGenIDCheck(gunLevelIndex))
      {
        rage::phInst* pInst = NULL;
        pInst = m_character->getLevel()->GetInstance(gunLevelIndex);
        if(pInst)
        {
          rage::Matrix34 worldToHand;
          m_body->getLeftArm()->getHand()->getMatrix(worldToHand);//handToWorld
          worldToHand.Inverse();
          rage::Matrix34 gunToWorld = RCC_MATRIX34(pInst->GetMatrix());
          m_character->m_gunToHandCurrent[NmRsCharacter::kLeftHand].Dot(gunToWorld, worldToHand);
        }
      }
      //right
      gunLevelIndex = m_character->m_rightHandWeapon.levelIndex;
      if (m_character->IsInstValid_NoGenIDCheck(gunLevelIndex))
      {
        rage::phInst* pInst = NULL;
        pInst = m_character->getLevel()->GetInstance(gunLevelIndex);
        if(pInst)
        {
          rage::Matrix34 worldToHand;
          m_body->getRightArm()->getHand()->getMatrix(worldToHand);//handToWorld
          worldToHand.Inverse();
          rage::Matrix34 gunToWorld = RCC_MATRIX34(pInst->GetMatrix());
          m_character->m_gunToHandCurrent[NmRsCharacter::kRightHand].Dot(gunToWorld, worldToHand);
        }
      }
    }

    // compute time warp scale.
    if(m_parameters.timeWarpActive)
      m_timeWarpScale = rage::Sqrtf(m_parameters.timeWarpStrengthScale / (30.0f * timeStep));
    else
      m_timeWarpScale = 1.0f;

    updateArmData(timeStep);

    // drop friction so active arms will smoothly slip over the torso.
    //This can interfere with higher friction for catchFall leading to the arms stretching out and up when on the floor
    //mmmmtodo this properly either
    //  only when pointing
    //  only reduce for collisions with self
    //Workaround1 Turn off if a ground behaviour is running
    //Workaround2 Turn off if dynamicBalancer is not OK
    NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    if (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
    {
      if(m_parameters.enableRight)
        m_character->setFrictionPreScale(0.1f, bvmask_ArmRight);
      if(m_parameters.enableLeft && m_enableLeft && (m_weaponMode != kPistolRight))
        m_character->setFrictionPreScale(0.1f, bvmask_ArmLeft);
    }

#if ART_ENABLE_BSPY & PointGunBSpyDraw
    m_character->setSkeletonVizRoot(10);
    m_character->setSkeletonVizMode(NmRsCharacter::kSV_DesiredAngles);
    BehaviourMask mask = bvmask_ArmRight;
    mask |= (1 << gtaJtSpine_3);
    if(m_weaponMode == kPistol ||
      m_weaponMode == kRifle ||
      m_weaponMode == kDual)
      mask |= bvmask_ArmLeft;
    m_character->setSkeletonVizMask(mask);
#endif

    if(m_weaponMode != kRifle)
      releaseRifleConstraint();
    if(!(m_weaponMode == kRifle || m_weaponMode == kPistol))
      m_character->releaseSupportHandConstraint();
    ArmStatus localArmStatus;
    switch(m_weaponMode) 
    {
    case kRifle:
    case kPistol:
      //Assumes right arm is primary
      pointDoubleHandedGun(m_armData[NmRsCharacter::kRightHand], m_armData[NmRsCharacter::kLeftHand]);
      break;
    case kPistolRight:
      pointSingleHandedGun(m_armData[NmRsCharacter::kRightHand]);
      //Gives neutral pose if allowed by m_parameters.poseUnusedOtherArm
      localArmStatus = setNeutralPoseType(m_armData[NmRsCharacter::kLeftHand], npsBySide, true); 
      setArmStatus(m_armData[NmRsCharacter::kLeftHand], localArmStatus);
      break;
    case kSidearm:
      pointSingleHandedGun(m_armData[NmRsCharacter::kRightHand]);
      //Always gives neutral pose
      localArmStatus = setNeutralPoseType(m_armData[NmRsCharacter::kLeftHand], npsBySide); 
      setArmStatus(m_armData[NmRsCharacter::kLeftHand], localArmStatus);
      break;
    case kPistolLeft:
      pointSingleHandedGun(m_armData[NmRsCharacter::kLeftHand]);
      //Gives neutral pose if allowed by m_parameters.poseUnusedOtherArm
      localArmStatus = setNeutralPoseType(m_armData[NmRsCharacter::kRightHand], npsBySide, true); 
      setArmStatus(m_armData[NmRsCharacter::kLeftHand], localArmStatus);
      break;
    case kDual:
      pointSingleHandedGun(m_armData[NmRsCharacter::kRightHand]);
      pointSingleHandedGun(m_armData[NmRsCharacter::kLeftHand]);
      break;
    default:
      break;
    }

    if (m_armData[NmRsCharacter::kLeftHand].status != kUnused &&
      m_armData[NmRsCharacter::kLeftHand].status != kNotSet &&
      m_armData[NmRsCharacter::kLeftHand].status != kShotUsing)
      setArmMuscles(m_armData[NmRsCharacter::kLeftHand]);

    if (m_armData[NmRsCharacter::kRightHand].status != kUnused &&
      m_armData[NmRsCharacter::kRightHand].status != kNotSet &&
      m_armData[NmRsCharacter::kRightHand].status != kShotUsing)
      setArmMuscles(m_armData[NmRsCharacter::kRightHand]);

    //mmmmTODO: see if we're actually using the arms before turning collisions off?
    //Turn off collisions between the hands - the two handed pistol animation has the hands penetrating
    //if m_parameters.disableArmCollisions=true: also disable collisions between right hand/forearm and the torso/legs.
    //if m_parameters.disableRifleCollisions=true and m_weaponMode == kRifle: also disable collisions between right hand/forearm and spine3/spine2.
    setHandCollisionExclusion();

    //Target for headlook/spineTwist and balancer turnInDirection
    //needs to come after updateArmData is called.
    rage::Vector3 target = m_armData[NmRsCharacter::kRightHand].currentTarget;
    bool leftPointingOK = m_armData[NmRsCharacter::kLeftHand].status == kPointing || m_armData[NmRsCharacter::kLeftHand].status == kPointingWithError;
    bool rightPointingOK = m_armData[NmRsCharacter::kRightHand].status == kPointing || m_armData[NmRsCharacter::kRightHand].status == kPointingWithError;
    bool leftPointing = m_weaponMode == kPistolLeft ||
      (m_weaponMode == kDual && (leftPointingOK && !rightPointingOK));
    bool pointing = leftPointingOK || rightPointingOK;
    if (leftPointing)
      target = m_armData[NmRsCharacter::kLeftHand].currentTarget;

    if(m_parameters.useHeadLook)
    {
      NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      Assert(headLookTask);
      if(!headLookTask->isActive())
      {
        headLookTask->updateBehaviourMessage(NULL);
        headLookTask->activate();
      }
      headLookTask->m_parameters.m_pos.Set(target);
    }

    //don't turn or twist to target if in neutral
    if (!pointing)
    {
      rage::Matrix34 pelvisTM;
      getSpine()->getPelvisPart()->getMatrix(pelvisTM);
      target = getSpine()->getPelvisPart()->getPosition() - 3.0f*pelvisTM.c;
    }
    float twistOffset  = 0.0f;
    if (pointing)
    {
      if(m_weaponMode == kRifle)
        twistOffset  = 1.0f;
      else if(m_weaponMode == kPistolLeft || (m_weaponMode == kDual && leftPointing))
        twistOffset  = 0.5f;
      else if(m_weaponMode == kPistol || m_weaponMode == kPistolRight)
        twistOffset  = -0.5f;
    }
    // drive spine twist.
    if(m_parameters.useSpineTwist)
    {
      NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
      Assert(spineTwistTask);
      spineTwistTask->setSpineTwistPos(target);
      spineTwistTask->setSpineTwistAllwaysTwist(true);
      spineTwistTask->setSpineTwistTwistClavicles(false);
      spineTwistTask->setSpineTwistOffset(twistOffset);

      if (!spineTwistTask->isActive())
        spineTwistTask->activate();

      NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
      Assert(headLookTask);
      headLookTask->m_parameters.twistSpine = false;
    }
    if(m_parameters.useTurnToTarget)
    {
      NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      rage::Vector3 toTarget(target - m_body->getSpine()->getSpine3Part()->getPosition());
      toTarget.Normalize();
      rage::Vector3 bodyUp = m_character->m_COMTM.b;
      rage::Quaternion q;
      q.FromRotation(bodyUp, -twistOffset);
      q.Transform(toTarget);
      dynamicBalancerTask->useCustomTurnDir(true, toTarget);
    }

    // clear target flags.
    m_parameters.targetValid = false;

    // if the gun has been fired recently ( timer > 0 ) then run the timer down by the time step.
    for(int i = 0; i < 2; ++i)
      m_armData[i].fireInhibitTimer = rage::Clamp(m_armData[i].fireInhibitTimer - m_character->getLastKnownUpdateStep(), 0.0f, 10.0f);

    // report the arm status to the game, if changed.
    if (m_armData[NmRsCharacter::kLeftHand].armStatusChanged || m_armData[NmRsCharacter::kRightHand].armStatusChanged)
      armStatusFeedback();

#if NM_RUNTIME_LIMITS
    //Restore limits as much as possible. We assume that neutralPoses and neutralPointing are ok with the normal wrist limits
    if (m_armData[NmRsCharacter::kLeftHand].armStatusChanged && 
      (m_armData[NmRsCharacter::kLeftHand].status != kPointing && m_armData[NmRsCharacter::kLeftHand].status != kPointingWithError && m_armData[NmRsCharacter::kLeftHand].status != kSupporting))
      m_character->getEffectorDirect(gtaJtWrist_Left)->setLimitsToPose(true);
    if (m_armData[NmRsCharacter::kRightHand].armStatusChanged && 
      (m_armData[NmRsCharacter::kRightHand].status != kPointing && m_armData[NmRsCharacter::kRightHand].status != kPointingWithError))
      m_character->getEffectorDirect(gtaJtWrist_Right)->setLimitsToPose(true);
#endif

    //Interfaces with other behaviours (behaviour execution Hack): 
    //mmmmTODO1 cancel any other cheat forces on arms - e.g grab.
    NmRsCBUShot* shotTask = (NmRsCBUShot*)m_cbuParent->m_tasks[bvid_shot];
    Assert(shotTask);
    if (shotTask->isActive())
    {
      shotTask->setCleverIKStrengthMultLeft(0.0f);
      shotTask->setCleverIKStrengthMultRight(0.0f);
      if (m_armData[NmRsCharacter::kLeftHand].status == kUnused)
        shotTask->setCleverIKStrengthMultLeft(1.0f);
      if (m_armData[NmRsCharacter::kRightHand].status == kUnused)
        shotTask->setCleverIKStrengthMultRight(1.0f);
    }

    handAnimationFeedback();
    return eCBUTaskComplete;
  }
#if 0
  void NmRsCBUPointGun::intoWorldTest(const NmRsHumanArm *arm, rage::Vector3 &target)
  { 
    // probes the world to see if we are aiming into some kind of scenery.
    // NOTE: Game is setting flags of what to include/exclude in these probes per character.
    rage::Vector3 probeStart = arm->getShoulder()->getJointPosition();
    rage::Vector3 targetDirection = target - probeStart;
    targetDirection.Normalize();

    // a bit of hysteresis
    float probeL = 0.75f;
    if(m_weaponMode == kRifle)
      probeL = 1.1f; // rifle needs more clearance
    if (m_probeHitSomeThing)
      probeL = probeL*1.2f;

    rage::Vector3 probeEnd = probeStart + targetDirection*probeL;

    m_probeHitSomeThing = m_character->probeRay(NmRsCharacter::pi_UseNonAsync,
      probeStart, probeEnd,
      rage::phLevelBase::STATE_FLAGS_ALL,
      TYPE_FLAGS_ALL,
      TYPE_FLAGS_ALL,
      m_character->m_probeTypeExcludeFlags | ((1 << 10) | (1 << 11) | (1 << 12)) /*m_parameters.weaponMask*/,
      false);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    bspyScratchpad(m_character->getBSpyID(), "pointGun", probeStart);
    bspyScratchpad(m_character->getBSpyID(), "pointGun", probeEnd);
    rage::Vector3 probeHit; probeHit.Zero();
    if(m_probeHitSomeThing)
      probeHit = m_character->m_probeHitPos[m_character->pi_UseNonAsync];
    bspyScratchpad(m_character->getBSpyID(), "pointGun", probeHit);
#endif

    //Ignore Ray collisions with the targets instances
    if (m_probeHitSomeThing && m_character->m_probeHitInst[NmRsCharacter::pi_UseNonAsync])
    {
      int hitThingsIndex = m_character->m_probeHitInst[NmRsCharacter::pi_UseNonAsync]->GetLevelIndex();
      if (m_parameters.rightHandTargetIndex != -1)
        if (m_parameters.rightHandTargetIndex == hitThingsIndex)
          m_probeHitSomeThing = false;
      if (m_parameters.leftHandTargetIndex != -1)
        if (m_parameters.leftHandTargetIndex == hitThingsIndex)
          m_probeHitSomeThing = false;
    }
  }
#endif

  void NmRsCBUPointGun::setArmMuscles(ArmData& armData)
  {
    //mmmmTODO  Should we have different muscle settings for neutral pose?  For pointingArm vs supportingArm
    //set the arm stiffnesses
    float armStiffness = m_parameters.armStiffness;
    float gravityOpposition = m_parameters.gravityOpposition;
    m_character->m_rightHandWeapon.massMultiplier = 1.0f;
    //reduce the strength of the rightHand if the left hand isn't helping.
    if (armData.arm->getType() == kRightArm && (m_weaponMode == kRifle || m_weaponMode == kPistol) &&
      !supportConstraintEffective(m_armData[NmRsCharacter::kLeftHand]))
    {
      m_character->m_rightHandWeapon.massMultiplier = m_parameters.massMultDetachedSupport;
      gravityOpposition = m_parameters.gravOppDetachedSupport;
      armStiffness = m_parameters.armStiffnessDetSupport;
    }

    armData.arm->setBodyStiffness(*armData.input, armData.relaxScale*armStiffness*m_timeWarpScale, m_parameters.armDamping);

    float muscleStiffness = 1.0f;
    if (!m_parameters.allowShotLooseness)
    {
      armData.inputData->getClavicle()->setMuscleStiffness(muscleStiffness);
      muscleStiffness = 0.5f;
      armData.inputData->getShoulder()->setMuscleStiffness(muscleStiffness);
      armData.inputData->getElbow()->setMuscleStiffness(muscleStiffness);
    }
    else
    {
      //Done this way so it will still look to setBy as if shot set it
      if (armData.arm->getClavicle()->getMuscleStiffness() >= 1.0f)
        armData.inputData->getClavicle()->setMuscleStiffness(muscleStiffness);
      if (armData.arm->getShoulder()->getMuscleStiffness() >= 0.5f)
        armData.inputData->getShoulder()->setMuscleStiffness(muscleStiffness);
      if (armData.arm->getElbow()->getMuscleStiffness() >= 0.5f)
        armData.inputData->getElbow()->setMuscleStiffness(muscleStiffness);
    }
    muscleStiffness = 10.0f;
    armData.inputData->getWrist()->setMuscleStiffness(muscleStiffness);
    if (armData.arm->getType() == kRightArm && m_weaponMode == kRifle && (armData.status != kPointing || armData.status != kPointingWithError))
      armData.inputData->getWrist()->setStiffness(armStiffness*m_timeWarpScale, m_parameters.armDamping);//don't apply fireWeapon reduction to the wrists if not pointing
    // enable gravity opposition on the pointing arm 
    // Note this used to be set after the block below with the return
    armData.arm->setOpposeGravity(*armData.input, gravityOpposition);
    getSpineInputData()->getSpine3()->setOpposeGravity(gravityOpposition);

  }

  //Use when pointing a gun that is not being supported by the other hand
  void NmRsCBUPointGun::pointSingleHandedGun(ArmData& armData)
  {
    ArmStatus gunArmStatus;
    gunArmStatus = computeGunArmState(armData);//returns kUnused,kNeutral or kPointing
    armData.desiredStatus = gunArmStatus;
    gunArmStatus = moveGunArm(armData, gunArmStatus, true);//returns kUnused, kPointingWithError, kPointing, kNeutralPose, kNeutralPointing
    setArmStatus(armData, gunArmStatus);//We do this only at the end so that it is changed only once
  }

  //returns kUnused, kPointingWithError, kPointing, kNeutralPose, kNeutralPointing
  NmRsCBUPointGun::ArmStatus  NmRsCBUPointGun::moveGunArm(ArmData& armData, const NmRsCBUPointGun::ArmStatus gunArmStatus, bool calculate)
  {
    switch (gunArmStatus)
    {
    case kPointing:
      return pointGunHand(armData, calculate);//returns kPointingWithError or kPointing
    case kNeutralPointing:
      pointGunHand(armData, calculate, kNeutralPointing);//NB: armData must contain the pointing target (as the target).  returns kPointingWithError or kPointing which we ignore as we know we are kNeutralPointing
      return kNeutralPointing;
    case kNeutral:
      return setNeutralPoseOrNeutralPoint(armData);//returns kUnused, kNeutralPose or kNeutralPointing
    case kNeutralPose: //This is included for completeness but generally moveGunArm is not called with gunArmStatus=kNeutralPose
      return setNeutralPose(armData);//returns kUnused or kNeutralPose
    default:
      return kUnused;
    }
  }

  //returns kUnused, kNeutral or kPointing
  NmRsCBUPointGun::ArmStatus NmRsCBUPointGun::computeGunArmState(ArmData& armData)
  {
    //Updates:
    //armData.pointingFromPosition
    //armData.targetDirection
    //target for ik = armData.target + vel comp + 

    //Early outs
    if(!armEnabled(armData.arm))
      return kUnused;//We assume that something else is controlling the arm

    // check to see if we're trying to point directly into
    // some static geo. if so apply neutral pose and return.
    //MMMMINT - mmmmTODO comment out to push away with gun
    //mmmmTODO Removed untill I sort out the probes
#if 0
    intoWorldTest(arm, armData.currentTarget);
#endif
    if (m_rifleFalling)
      return kNeutralPose;

    if(m_probeHitSomeThing || m_forceNeutral)
      return kNeutral;

    if (isGunHandTargetReachable(armData))
      return kPointing;
    else//Do a neutral pose or neutral point
      return kNeutral;
  }

  bool NmRsCBUPointGun::poseUnusedArm(ArmData& armData)
  {
    switch(m_weaponMode) 
    {
    case kRifle:
      //Assumes right arm is primary
      //m_parameters.poseUnusedGunArm doesn't apply to the rifle hand - we always want pointGun to control it.
      if (armData.arm->getType() == kRightArm/*&& ((m_parameters.oneHandedPointing == 0 || m_parameters.oneHandedPointing == 1) && m_reConnectTimer > 0.0f)*/)
        return true;
      else
        return m_parameters.poseUnusedSupportArm;
    case kPistol:
      //Assumes right arm is primary - after ||| means if constraint broken and not oneHanded pointing allowed let point gun take control of it
      if (armData.arm->getType() == kRightArm)
        return m_parameters.poseUnusedGunArm || ((m_parameters.oneHandedPointing == 0 || m_parameters.oneHandedPointing == 2) && m_reConnectTimer > 0.0f);
      else
        return m_parameters.poseUnusedSupportArm;
    case kPistolRight:
      if (armData.arm->getType() == kRightArm)
        return m_parameters.poseUnusedGunArm;
      else
        return m_parameters.poseUnusedOtherArm;
    case kSidearm:
      if (armData.arm->getType() == kRightArm)
        return m_parameters.poseUnusedGunArm;
      else
        //m_parameters.poseUnusedOtherArm doesn't apply to kSidearm - we always want pointGun to control it.
        return true;
    case kPistolLeft:
      if (armData.arm->getType() == kLeftArm)
        return m_parameters.poseUnusedGunArm;
      else
        return m_parameters.poseUnusedOtherArm;
    case kDual:
      return m_parameters.poseUnusedGunArm;
    default:
      //This a good default if another behaviour is controlling the arms but will lead to t-pose if only pointGun setting arms
      //(If we identify that no other behaviour is setting the arm then we could return true)
      return false;
    }
  }

  bool NmRsCBUPointGun::selectNeutralTarget(ArmData& armData)
  {
    //if (arm == getLeftArm()) 
    //could give neutral pose to supporting hand if called pointGunHand on it as pointGunHand not a function of m_weaponMode now
    // select supporting if rifle/pistol
    if (m_weaponMode == kRifle && armData.arm == getRightArm())
      m_add2WeaponDist = 0.0f;//m_add2WeaponDist only applied to gunArm
    bool pointAtNeutral = (m_parameters.neutralPoint4Pistols && m_weaponMode != kRifle) ||
      (m_parameters.neutralPoint4Rifle && m_weaponMode == kRifle);
    if (poseUnusedArm(armData) && pointAtNeutral)//Try to point at a neutral target
    {
      rage::Vector3 bodyArmSide = m_character->m_COMTM.a;
      if (armData.arm == getLeftArm() && (m_weaponMode == kPistolLeft || m_weaponMode == kDual))
        bodyArmSide *= -1.0f;
      rage::Vector3 bodyUp = m_character->m_COMTM.b;
      rage::Vector3 bodyBack = m_character->m_COMTM.c;
      if (m_neutralSidePoseTimer > 0.0f && m_supportConstraintInhibitTimer > 0.0f)//to side - ONLY DO IF CONSTRAINT BROKEN.  IT GIVES A BAD LOOKING GUNARM IF CONSTRAINT ON.
      {
        armData.target = m_character->m_COM +
          bodyArmSide*m_parameters.point2Side.x +
          bodyUp*m_parameters.point2Side.y +
          bodyBack*m_parameters.point2Side.z;
        if (m_weaponMode == kRifle && armData.arm->getType() == kRightArm)//So that the rifle isn't held near shoulder/so that the arm straightens to make the gun look heavier
          m_add2WeaponDist = m_parameters.add2WeaponDistSide;
        m_neutralSidePoseTimer -= m_character->getLastKnownUpdateStep();
        armData.neutralPoseType = npsPointingDontConnect;
      }
      else//gun across front - atm these targets are assumed to be reachable by the gunHand
      {
        armData.target = m_character->m_COM +
          bodyArmSide*m_parameters.point2Connect.x +
          bodyUp*m_parameters.point2Connect.y +
          bodyBack*m_parameters.point2Connect.z;
        if (m_weaponMode == kRifle && armData.arm == getRightArm())//So that the rifle isn't held near shoulder/so that the arm straightens to make the gun look heavier
          m_add2WeaponDist = m_parameters.add2WeaponDistConnect;
        armData.neutralPoseType = npsPointing2Connect;
      }
      armData.targetDirection.Normalize(armData.target - armData.pointingFromPosition);

      if (m_parameters.checkNeutralPoint)
      {
        //if this target was relative to the shoulder matrix1 - it would have to be checked only once
        if (isGunHandTargetReachable(armData))
          return true;
        else
        {
          ////clean up? or zero the target? so bSpy would know whats happening. Would need to for rifleConstraintTick?
          //armData.target = armData.currentTarget;
          //armData.targetDirection.Normalize(armData.target - armData.pointingFromPosition);    
          return false;
        }
      }
      else//!m_parameters.checkNeutralPoint
        return true;
    }
    else //!pointAtNeutral
      return false;

  }

  //Also calculates armData.pointingFromPosition, armData.targetDirection
  bool NmRsCBUPointGun::isGunHandTargetReachable(ArmData& armData)
  {
    // set up arm-specific info
    int parentEffector;
    rage::Vector3 parentEffectorPos_To_PointingFromPos_In_Spine3;
    if(armData.arm->getType() == kRightArm)
    {
      parentEffector = m_parameters.rightHandParentEffector;
      parentEffectorPos_To_PointingFromPos_In_Spine3.Set(m_parameters.rightHandParentOffset + m_measuredRightHandParentOffset);
    }
    else//this is probably not needed as only right hand is primary.
    {
      parentEffector = m_parameters.leftHandParentEffector;
      parentEffectorPos_To_PointingFromPos_In_Spine3.Set(m_parameters.leftHandParentOffset + m_measuredLeftHandParentOffset);
    }
    // default to pointing from shoulder if not specified
    if(parentEffector == -1)
    {
      parentEffector = armData.arm->getShoulder()->getJointIndex();
    }

    // calculate pointing from position
    rage::Vector3 parentJointPosition = m_character->getConstEffector(parentEffector)->getJointPosition();
    rage::Vector3 parentEffectorPos_To_PointingFromPos_In_World;
    rage::Matrix34 spine3TM;
    getSpine()->getSpine3Part()->getMatrix(spine3TM);
    spine3TM.Transform3x3(parentEffectorPos_To_PointingFromPos_In_Spine3, parentEffectorPos_To_PointingFromPos_In_World);
    armData.pointingFromPosition.Add(parentJointPosition, parentEffectorPos_To_PointingFromPos_In_World);
#if ART_ENABLE_BSPY & PointGunBSpyDraw
    m_character->bspyDrawLine(parentJointPosition, armData.pointingFromPosition, rage::Vector3(1,0,0));
#endif

    armData.targetDirection.Normalize(armData.target - armData.pointingFromPosition);

    //Is target angle reachable?
    //Project toTarget onto plane defined by spine3Up normal //(a,b,c) (Up, left, back)
    rage::Vector3 toTarget = armData.target - spine3TM.d;
    toTarget.AddScaled(toTarget, spine3TM.a, -spine3TM.a.Dot(toTarget));//spine3TM.a = up
    toTarget.Normalize();    
    //Get angle of target from spine3 forward (-spine3TM.c).   +ve is left (for the right arm)
    float targetAngleHorizontal = rage::AcosfSafe(toTarget.Dot(-spine3TM.c));
    if (toTarget.Dot(spine3TM.b) < 0.0f)//-spine3TM.b = right
      targetAngleHorizontal *= -1.0f;
#if ART_ENABLE_BSPY & PointGunBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", RtoD*targetAngleHorizontal);
#endif
    NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    NmRsCBUShot *shotTask = (NmRsCBUShot*)m_cbuParent->m_tasks[bvid_shot];
    Assert(shotTask);
    //spine can twist 32deg=0.558rads left/right
    //Don't allow extra aiming angle from available spineTwist if falling or fallToKnees is dropping to knees.
    if (m_parameters.useSpineTwist && (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK) && (!(shotTask->isActive() && shotTask->getFallToKneesIsFalling())))
    {
      float currentSpineTwist = nmrsGetActualTwist(getSpine()->getSpine0())+nmrsGetActualTwist(getSpine()->getSpine1())+nmrsGetActualTwist(getSpine()->getSpine2())+nmrsGetActualTwist(getSpine()->getSpine3());
      float spineTwistMax = 0.5584f;//mmmm RigIndependence get form actual limits, +ve twist to left
      float twistAvailableLeft = spineTwistMax - currentSpineTwist;
      float twistAvailableRight = -spineTwistMax - currentSpineTwist;
      if (targetAngleHorizontal >= 0.0f && twistAvailableLeft > 0.0f)
        targetAngleHorizontal -= twistAvailableLeft;
      if (targetAngleHorizontal <= 0.0f && twistAvailableRight < 0.0f)
        targetAngleHorizontal -= twistAvailableRight;
#if ART_ENABLE_BSPY & PointGunBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", twistAvailableLeft);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", twistAvailableRight);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", RtoD*targetAngleHorizontal);
#endif
    }

    bool horizontalAngleOK = targetAngleHorizontal < m_parameters.maxAngleAway*DtoR && targetAngleHorizontal > -m_parameters.maxAngleAcross*DtoR;
    if (armData.arm->getType() == kLeftArm)//The angle should be the same but the maxAngleAway and maxAngleAcross should be swapped
      horizontalAngleOK = targetAngleHorizontal < m_parameters.maxAngleAcross*DtoR && targetAngleHorizontal > -m_parameters.maxAngleAway*DtoR;


    // is target angle reachable?
    //Is the target direction within the arms workspace cone?
    //cone centre line is 22deg 
    float targetAngleThreshold = -0.5f;//for pistols and small guns (i.e. can reach 210deg cone - for right  90+22+30 = 142deg right, 90-22+30 = 98 left
    rage::Vector3 centerOfCone;
    centerOfCone.Set(0.0f, armData.palmScaleDir * rage::Sinf(22.0f*DtoR), -rage::Cosf(22.0f*DtoR)); //spine3 (Up, Left, Back).i.e. 22 deg to left/right for Left/Right respectively
    centerOfCone.Normalize(); // remove me
    spine3TM.Transform3x3(centerOfCone);
    float dotOntoTDir = centerOfCone.Dot(armData.targetDirection);
#if ART_ENABLE_BSPY & PointGunBSpyDraw
    m_character->bspyDrawLine(armData.pointingFromPosition, armData.pointingFromPosition+centerOfCone*0.3f, rage::Vector3(1,1,1));
    //mmmmTODO display limit cone? Also add better debug graphics in general
    bspyScratchpad(m_character->getBSpyID(), "PointGun", dotOntoTDir);
#endif
    bool insideArmWorkSpace = dotOntoTDir > targetAngleThreshold;
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "PointGun", insideArmWorkSpace);
    bspyScratchpad(m_character->getBSpyID(), "PointGun", horizontalAngleOK);
#endif

    bool targetReachable = horizontalAngleOK;
    //UpDown and leftRight angle limits
    // horizontalAngleOK is removed from the equation
    // m_parameters.fallingLimits = 1 apply them only if the character is falling 
    // m_parameters.fallingLimits = 2 apply them always 
    if (
		(m_parameters.fallingLimits == 1 && ((dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK) || (shotTask->isActive() && shotTask->getFallToKneesIsFalling())) )
      || m_parameters.fallingLimits == 2)
    {
      float upAngleLimit = m_parameters.upLimit;
      float downAngleLimit = m_parameters.downLimit;
      float awayAngleLimit = m_parameters.awayLimit;
      float acrossAngleLimit = m_parameters.acrossLimit;
      if (armData.arm->getType() == kLeftArm)//The angle should be the same but the maxAngleAway and maxAngleAcross should be swapped
      {
        awayAngleLimit = m_parameters.acrossLimit;
        acrossAngleLimit = m_parameters.awayLimit;
      }
      //Rotate spine3TM to give the centre of the limit cone for asymmetrical limits
      rage::Matrix34 limitFrame = spine3TM;//(a,b,c) (Up, left, back)
      float upOffset = DtoR*(downAngleLimit - upAngleLimit)*0.5f; 
      float awayOffset = DtoR*(acrossAngleLimit - awayAngleLimit)*0.5f;
      limitFrame.RotateLocalX(awayOffset);
      limitFrame.RotateLocalY(upOffset);
      toTarget = armData.target - spine3TM.d;
      toTarget.Normalize();    
#if ART_ENABLE_BSPY
      m_character->bspyDrawLine(spine3TM.d, spine3TM.d+toTarget, rage::Vector3(1,1,1));
      m_character->bspyDrawCoordinateFrame(0.7f, spine3TM);
      m_character->bspyDrawCoordinateFrame(1.3f, limitFrame);
#endif
      //Get upDownAngle inside limit cone//(a,b,c) (Up, left, back)
      //up from forward = +ve (0 forward to 180 back)
      //down from forward = -ve (0 forward to -180 back)
      float toTargetDotFront = toTarget.Dot(-limitFrame.c);
      float toTargetDotDown = toTarget.Dot(-limitFrame.a);
      float upDownAngle = rage::AcosfSafe(toTargetDotDown) - PI*0.5f;//toTargetDotDown
      if (toTargetDotFront < 0)
      {
        if (upDownAngle>0)
          upDownAngle = PI-upDownAngle;
        else
          upDownAngle = -PI-upDownAngle;
      }

      //Get leftRightAngle inside limit cone//(a,b,c) (Up, left, back)
      //left from forward = -ve (0 forward to -180 back)
      //right from forward = +ve (0 forward to 180 back)
      float toTargetDotLeft = toTarget.Dot(limitFrame.b);
      float leftRightAngle = rage::AcosfSafe(toTargetDotLeft) - PI*0.5f;//toTargetDotDown
      if (toTargetDotFront < 0)
      {
        if (leftRightAngle>0)
          leftRightAngle = PI-leftRightAngle;
        else
          leftRightAngle = -PI-leftRightAngle;
      }

      float l1unit = leftRightAngle/(DtoR*0.5f*(awayAngleLimit + acrossAngleLimit));
      float l2unit = upDownAngle/(DtoR*0.5f*(upAngleLimit + downAngleLimit));
      float violationFactor = l1unit*l1unit + l2unit*l2unit - 1.f;
      targetReachable = (violationFactor <= 0.0f);

#if ART_ENABLE_BSPY & PointGunBSpyDraw
      float approxleftRightAngle = leftRightAngle - awayOffset;//might not be correct for left hand 
      float approxupDownAngle = upDownAngle - upOffset;//might not be correct for left hand 
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", upAngleLimit);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", downAngleLimit);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", awayAngleLimit);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", acrossAngleLimit);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", violationFactor);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", l1unit);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", l2unit);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", (violationFactor<0.0f));
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", toTargetDotLeft);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", toTargetDotFront);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", toTargetDotDown);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", RtoD*upDownAngle);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", RtoD*leftRightAngle);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", targetReachable);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", RtoD*approxleftRightAngle);
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", RtoD*approxupDownAngle)
#endif

    }


    if (m_parameters.leadTarget > 0.0f && !(m_weaponMode == kPistol || m_weaponMode == kRifle))
      armData.targetDirection.Normalize(armData.leadTarget - armData.pointingFromPosition);

    return (insideArmWorkSpace && targetReachable);


  }

  //Calculates armData.desiredHandToWorld
  //Also armData.pointingFromPosition, armData.targetDirection,  armData.weaponDistance
  void NmRsCBUPointGun::calculateGunHand(ArmData& armData, const ArmStatus gunArmStatus)
  {
    // set up arm-specific info
    int parentEffector;
    rage::Vector3 parentEffectorPos_To_PointingFromPos_In_Spine3;
    if(armData.arm->getType() == kRightArm)
    {
      parentEffector = m_parameters.rightHandParentEffector;
      parentEffectorPos_To_PointingFromPos_In_Spine3.Set(m_parameters.rightHandParentOffset + m_measuredRightHandParentOffset);
    }
    else
    {
      parentEffector = m_parameters.leftHandParentEffector;
      parentEffectorPos_To_PointingFromPos_In_Spine3.Set(m_parameters.leftHandParentOffset + m_measuredLeftHandParentOffset);
    }
    // default to pointing from shoulder if not specified
    if(parentEffector == -1)
    {
      parentEffector = armData.arm->getShoulder()->getJointIndex();
    }

    //Note: this has usually already been calculated in isGunHandTargetReachable 
    //  but isConstraintBroken isn't guaranteed to have called isGunHandTargetReachable so repeat calc here (where it should be anyway)
    // calculate pointing from position
    rage::Vector3 parentJointPosition = m_character->getConstEffector(parentEffector)->getJointPosition();
    rage::Vector3 parentEffectorPos_To_PointingFromPos_In_World;
    rage::Matrix34 spine3TM;
    getSpine()->getSpine3Part()->getMatrix(spine3TM);
    spine3TM.Transform3x3(parentEffectorPos_To_PointingFromPos_In_Spine3, parentEffectorPos_To_PointingFromPos_In_World);
    armData.pointingFromPosition.Add(parentJointPosition, parentEffectorPos_To_PointingFromPos_In_World);

    // Get armData.weaponDistance:  The distance from pointingFromPosition to desiredPrimaryHandTarget
    if (m_parameters.useIncomingTransforms && m_parameters.primaryHandWeaponDistance < 0.0f)
    {
      // Measure the weaponDistance from the incoming transforms.
      //  Get spine3 Position, primaryHand position and parentJoint position
      rage::Matrix34 primaryHandToWorld, jointChildBodyToWorld, spine3ToWorld;
      m_character->getITMForPart(getSpine()->getSpine3Part()->getPartIndex(), &spine3ToWorld);
      m_character->getITMForPart(armData.arm->getHand()->getPartIndex(), &primaryHandToWorld);
      const NmRsEffectorBase* effector = m_character->getConstEffector(parentEffector);
      m_character->getITMForPart(effector->getChildIndex(), &jointChildBodyToWorld); // get joint's child body matrix
      //    Get parentEffectorPositionFromITM
      rage::phJoint* jt = effector->getJoint();
      rage::Vector3 parentEffectorPositionFromITM = jt->GetPositionChild();//PositionChild = position from JointChildBody_to_Joint in ChildBody space
      jointChildBodyToWorld.Transform3x3(parentEffectorPositionFromITM);//parentEffectorPositionFromITM = JointChildBody_to_Joint in world
      parentEffectorPositionFromITM += jointChildBodyToWorld.d;

      rage::Vector3 pointingFromPosition;
      pointingFromPosition.Set(parentEffectorPositionFromITM);
      spine3ToWorld.Transform3x3(parentEffectorPos_To_PointingFromPos_In_Spine3);
      pointingFromPosition.Add(parentEffectorPos_To_PointingFromPos_In_Spine3); 
      rage::Vector3 offsetD = primaryHandToWorld.d  - pointingFromPosition;
      armData.weaponDistance = offsetD.Mag();
#if ART_ENABLE_BSPY
      //Output what primaryHandWeaponDistance would have to be if not measured
      float primaryHandWeaponDistanceWouldBe = armData.weaponDistance;
      bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", primaryHandWeaponDistanceWouldBe);
#endif
    }
    else
    {
      //badInput_primaryHandWeaponDistance is true if m_parameters.primaryHandWeaponDistance is negative here
      armData.weaponDistance = m_parameters.primaryHandWeaponDistance;
      armData.weaponDistance = rage::Clamp(armData.weaponDistance, 0.2f, m_maxArmReach);
    }
#define REDUCE_WEAPON_DISTANCE 0 
#if REDUCE_WEAPON_DISTANCE//If anything this makes the non-pistol IK worse.  At best doesn't really do much.
    // If target direction is across the body reduce the weaponDistance to allow the elbow to bend.
    // TODO: Limit these effects to zero all but the extreme angles
    if(m_weaponMode != kRifle)
    {
      rage::Vector3 centerOfCone;
      rage::Matrix34 spine3TM;
      getSpine()->getSpine3Part()->getMatrix(spine3TM);
      centerOfCone.Set(0.0f, armData.palmScaleDir * 0.71f, -0.71f); //spine3 (Up, Left, Back).i.e. 45 deg to left/right for Left/Right respectively
      centerOfCone.Normalize(); // remove me
      spine3TM.Transform3x3(centerOfCone);
#if ART_ENABLE_BSPY & PointGunBSpyDraw
      if(arm == getLeftArm())
        m_character->bspyDrawLine(spine3TM.d, spine3TM.d+centerOfCone*0.3f, rage::Vector3(0,1,0));
      else
        m_character->bspyDrawLine(spine3TM.d, spine3TM.d+centerOfCone*0.3f, rage::Vector3(1,0,0));
      m_character->bspyDrawLine(spine3TM.d, spine3TM.d-spine3TM.c*0.3f, rage::Vector3(1,1,1));
#endif
      float targetDotCenter = armData.targetDirection.Dot(centerOfCone);
      const float threshold = 0.7f;
      const float scale = 0.2f;
      if(targetDotCenter < threshold)
      {
        float max = m_maxArmReach - (threshold - targetDotCenter) * scale;
        float min = m_maxArmReach * 0.7f;//
        armData.weaponDistance = rage::Clamp(armData.weaponDistance, min, max);
      }
    }
#endif//#if REDUCE_WEAPON_DISTANCE
    // try shortening the weapon distance slightly as a function
    // of the recoil relax timer.
#if NM_POINTGUN_RECOIL_IK
    if(m_parameters.fireWeaponRelaxTime > 0.0f)
    {
      armData.weaponDistance -= (armData.relaxTimer/m_parameters.fireWeaponRelaxTime)*m_parameters.fireWeaponRelaxDistance;
    }
#endif
    armData.weaponDistance += m_add2WeaponDist;//add in extra weapon distance to straighten a rifle arm neutral pointing to side.
    armData.weaponDistance = rage::Clamp(armData.weaponDistance, 0.2f, m_maxArmReach);//0.2 from shoulder is probably too close (except if recoiling briefly). When REDUCE_WEAPON_DISTANCE was on min was m_maxArmReach * 0.7

    //mmmmTODO leadTarget here or added after target out of range calcs?
    rage::Vector3 target(armData.leadTarget);//	if (m_parameters.leadTarget > 0.0f && !(m_weaponMode == kPistol || m_weaponMode == kRifle)) then armData.leadTarget = armData.target
    if (gunArmStatus == kNeutralPointing)
      target.Set(armData.target);
    rage::Matrix34 desiredGunToWorld;
    getDesiredGunToWorldOrientation(desiredGunToWorld, armData.pointingFromPosition, target);//returns desiredGunToWorld
    // apply local weapon offset to generate hand target
    // orientation from weapon target orientation
    rage::Matrix34 handToGun;
    handToGun.Transpose(armData.gunToHand);
    armData.desiredHandToWorld.Dot3x3(handToGun,desiredGunToWorld);//desiredHandToWorld = handToGun . desiredGunToWorld
    armData.desiredHandToWorld.d = armData.pointingFromPosition + armData.targetDirection * armData.weaponDistance;
  }
  //status changed to kPointingWithError or kPointing;
  NmRsCBUPointGun::ArmStatus NmRsCBUPointGun::pointGunHand(ArmData& armData, bool calculate, const ArmStatus gunArmStatus)
  {
    if (calculate)
      calculateGunHand(armData,gunArmStatus);
#if ART_ENABLE_BSPY & PointGunBSpyDraw
    m_character->bspyDrawCoordinateFrame(0.2f, armData.desiredHandToWorld);
#endif

#if NM_RUNTIME_LIMITS
    // open wrist limits for better pointing.
    // todo test if this is necessary for pistol.
    (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(armData.arm->getWrist()->getJointIndex())))->setLimits(1.5f, 1.5f, 1.5f);
#endif

    // do the ik based on setup in armData
    float twist = m_parameters.elbowAttitude;
    if(m_weaponMode != kRifle && m_parameters.usePistolIK)
      pistolIk(armData, 0, armData.desiredHandToWorld);
    else
      ik(armData, ((HINGE_DIRECTION(armData.arm)) * twist), armData.desiredHandToWorld, 1.0f);

    //
    //
    // various external controllers to help with hand positioning.
    //
    //
    if(m_parameters.oriStiff > 0 && armData.relaxScale > 0)
    {
      rage::Matrix34 handTM(armData.desiredHandToWorld);

      // help pointing hand out a bit with gentle pd to target orientation
      m_character->pdPartOrientationToM(
        armData.arm->getHand(),
        handTM,
        NULL,
        armData.relaxScale*m_parameters.oriStiff*m_timeWarpScale,
        m_parameters.oriDamp,
        0,
        0,
        PI/2);
    }

    // part offsets from wrist to inform helper forces.
    // we control wrist joint position instead of hand effector position.
    // this means applying impulse to the forearm offset by the wrist's parent position.
    rage::Vector3 wristParentPos, wristChildPos;
    wristChildPos = getRightArm()->getWrist()->get3DofJoint()->GetPositionChild();
    wristParentPos = getRightArm()->getWrist()->get3DofJoint()->GetPositionParent();

    // update hand target velocity.
    // TODO work out target velocity in updateArmData. work on original target, not hand target? use this space to convert target vel to hand target vel.
    //mmmmTODO Needs to be looked at again  - calculates armData.handTargetVel. Only used by pdPart if m_parameters.posStiff > 0 && armData.relaxScale > 0
    //m_parameters.targetValid is set to true only on a frame when a pointGun message is received.
    if(m_parameters.targetValid)
    {
      // this may be our first time through.
      // do we have previous position?
      if(armData.lastHandTargetPos.Mag2() < 0.1f)//i.e. if armData.lastHandTargetPos is zero as set in initialiseCustomVariables
        // pass wrist velocity so pd controller doesn't create drag.
        armData.handTargetVel = armData.arm->getLowerArm()->getLinearVelocity(&wristParentPos);
      else
      {
        float timeStepClamped = rage::Max(m_character->getLastKnownUpdateStep(), SMALLEST_TIMESTEP);//protect against division by zero
        armData.handTargetVel = armData.desiredHandToWorld.d - armData.lastHandTargetPos;
        armData.handTargetVel.Scale(1.0f/timeStepClamped);
        // incoming anim velocity scale is equivalent to target updates per nm tick.
        // this will normally be 0.5, as we are double stepped in relationship to the game.
        armData.handTargetVel.Scale(m_character->getEngine()->getIncomingAnimationVelocityScale());
      }

      // copy position for next time through.
      armData.lastHandTargetPos = armData.desiredHandToWorld.d;
    }

    if(m_parameters.posStiff > 0 && armData.relaxScale > 0)
    {
      // offset target by wrist child pos.
      rage::Vector3 offset, offsetTarget;
      armData.desiredHandToWorld.Transform3x3(wristChildPos, offset); 
      offsetTarget.Add(armData.desiredHandToWorld.d, offset);
      m_character->pdPartPosition(
        armData.arm->getLowerArm(),
        offsetTarget,
        &armData.handTargetVel,
        armData.relaxScale*m_parameters.posStiff*m_timeWarpScale,
        m_parameters.posDamp,
        NULL,
        0,
        &wristParentPos);
    }

    // compute error between pointing direction and target direction.
    rage::Matrix34 gunToWorld;
    armData.arm->getHand()->getMatrix(gunToWorld);//handToWorld
    gunToWorld.Dot3x3FromLeft(armData.gunToHand); // adjust for weapon offset in hand  gunToHand X handToWorld
    armData.error = rage::AcosfSafe(armData.targetDirection.Dot(gunToWorld.a)); 
#if ART_ENABLE_BSPY & PointGunBSpyDraw
    rage::Vector3 shoulderPos(armData.arm->getShoulder()->getJointPosition());
    m_character->bspyDrawLine(shoulderPos, shoulderPos+armData.targetDirection*2.0f, rage::Vector3(0.0f,1.0f,0.0f));
    m_character->bspyDrawLine(gunToWorld.d, gunToWorld.d-gunToWorld.b, rage::Vector3(1.0f,1.0f,0.0f));
#endif // ART_ENABLE_BSPY

    if(armData.error > m_parameters.errorThreshold)
      return kPointingWithError;
    else
      return kPointing;
  }

  //Checks to see if the currentGunSupportPos and current support hand pos are behind the back
  //This is to stop the constraints from tying the hands behind the back e.g. if support hand is shot backwards.
  //Note: we don't check desiredGunSupportPos as we constrain to the currentGunSupportPos although we output it to bSpy just in case this becomes an issue
  // Don't make the constraint and release the constraint if already exists if the support hand is behind the back
  bool NmRsCBUPointGun::isConstraintBehindBack(ArmData& /*primaryArmData*/, ArmData& supportArmData)
  {
    rage::Vector3 left2Right = getRightArm()->getShoulder()->getJointPosition() - getLeftArm()->getShoulder()->getJointPosition();
    rage::Vector3 hip2Head = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
    rage::Vector3 backwards;
    backwards.Cross(left2Right,hip2Head); 
    backwards.Normalize();

    supportArmData.currentGunSupportBehindBack = false;
    supportArmData.supportHandBehindBack = false;
    rage::Vector3 leftShoulder2Point = supportArmData.currentGunSupportInWorld.d - getLeftArm()->getShoulder()->getJointPosition();
    if (leftShoulder2Point.Dot(backwards)>0.0f)
      supportArmData.currentGunSupportBehindBack = true;

    leftShoulder2Point = getLeftArm()->getHand()->getPosition() - getLeftArm()->getShoulder()->getJointPosition();
    if (leftShoulder2Point.Dot(backwards)>0.0f)
      supportArmData.supportHandBehindBack = true;

#if ART_ENABLE_BSPY
    supportArmData.desiredGunSupportBehindBack = false;
    leftShoulder2Point = supportArmData.desiredGunSupportInWorld.d - getLeftArm()->getShoulder()->getJointPosition();
    if (leftShoulder2Point.Dot(backwards)>0.0f)
      supportArmData.desiredGunSupportBehindBack = true;
#endif
    supportArmData.supportBehindBack = supportArmData.supportHandBehindBack || supportArmData.currentGunSupportBehindBack;
    return supportArmData.supportBehindBack;

  }

  //Decides whether to use or disable a rage point hard constraint forceConstraint.  Updates them if using them.
  void NmRsCBUPointGun::supportConstraintUpdate(ArmData& primaryArmData, ArmData& supportArmData)
  {
    (void) primaryArmData;
    //NB make sure useConstraint is false if supportArmData.status == kUnused
    bool useConstraint = !supportArmData.supportBehindBack && supportArmData.status == kSupporting;
    //Forced based constraint
    //threshold=0.1 for shot, 0.25f for grab.  The constraint begins to act when the distance is < 3.0*threshold
    float threshold = m_parameters.constraintThresh;
    if (m_parameters.supportConstraint == 2 && useConstraint)
    {
      //Current supportHand to gunHand (current or desired)
      rage::Vector3 target1(supportArmData.currentGunSupportInWorld.d);
      //primaryArmData.desiredHandToWorld.Transform(m_supportHandToGunHand.d, target1);

      float strength = m_parameters.constraintStrength;
      int direction = 1;//left
      // port of the cleverHandIk from: body Helpers
      rage::Vector3 handPos = getLeftArm()->getHand()->getPosition();

      float distance = target1.Dist(handPos);

      // starts earlier than the hard constraint
      if ((distance < threshold*3.0f))
      {
        float damping = 0.1f;
        float stiffness = (threshold*3.0f - distance) / (threshold*2.0f);
        stiffness = rage::Clamp(stiffness, 0.0f, 1.0f) * strength;

        rage::Vector3 toHand(target1 - handPos);
        toHand.Scale(stiffness*stiffness);

        rage::Vector3 bodyVel;
        bodyVel.Set(0);
        bodyVel = getRightArm()->getHand()->getLinearVelocity();

        rage::Vector3 handVel = getLeftArm()->getHand()->getLinearVelocity();
        bodyVel = bodyVel - handVel;
        bodyVel.Scale(2.0f*stiffness*damping);
        toHand = toHand + bodyVel;

        if (m_character->getLastKnownUpdateStep() < (1.0f/60.0f))//mmmmTODO why not scaled by timestep as is impulse?
        {
          toHand.Scale(m_character->getLastKnownUpdateStep()*60.0f);  // make it work for slow motion by scaling the impulse appropriately
        }

        //offset hand helper force to infront of the palm
        //gives a more stable response and possibly a helping turning moment
        rage::Matrix34 handMat;
        getLeftArm()->getHand()->getMatrix(handMat);
        rage::Vector3 handOffset = handMat.GetVector(0);
        handOffset.Scale(0.04f*direction);
        handPos = handPos + handOffset;
        //This used to be applied at the wrist for stability? but works better at the hand
        getLeftArm()->getHand()->applyImpulse(toHand,handPos);
        toHand.Scale(-1.0f);
        getRightArm()->getHand()->applyImpulse(toHand,target1);

        //only help gun point at the target if helping support hand aswell
        //DesiredSupportHand to Hip - helps ensure a good aim if supportHand ik wrong
        target1 = supportArmData.desiredGunSupportInWorld.d;
        //Below slows down pointing as it draws the pointing arm to the desired support target which can be the currentGunSupportInWorld 
        //  but gives the impression that the hand points only when helped - perhaps have if oneHandedPointing = false?
        //target1 = supportArmData.desiredSupportHandToWorld.d;
        //primaryArmData.desiredHandToWorld.Transform(m_supportHandToGunHand.d, target1);

        strength = m_parameters.constraintStrength;
        // port of the cleverHandIk from: body Helpers
        handPos = supportArmData.currentGunSupportInWorld.d;
        distance = target1.Dist(handPos);

        // starts earlier than the hard constraint
        if ((distance < threshold*3.0f))
        {
          float damping = 0.1f;
          float stiffness = (threshold*3.0f - distance) / (threshold*2.0f);
          stiffness = rage::Clamp(stiffness, 0.0f, 1.0f) * strength;

          rage::Vector3 toHand(target1 - handPos);
          toHand.Scale(stiffness*stiffness);

          rage::Vector3 bodyVel;
          bodyVel.Set(0);
          bodyVel = getSpine()->getPelvisPart()->getLinearVelocity();

          rage::Vector3 handVel = getRightArm()->getHand()->getLinearVelocity();
          bodyVel = bodyVel - handVel;
          bodyVel.Scale(2.0f*stiffness*damping);
          toHand = toHand + bodyVel;

          if (m_character->getLastKnownUpdateStep() < (1.0f/60.0f))//mmmmTODO why not scaled by timestep as is impulse?
          {
            toHand.Scale(m_character->getLastKnownUpdateStep()*60.0f);  // make it work for slow motion by scaling the impulse appropriately
          }

          //This used to be applied at the wrist for stability? but works better at the hand
          getRightArm()->getHand()->applyImpulse(toHand,handPos);
          toHand.Scale(-1.0f);
          getSpine()->getPelvisPart()->applyImpulse(toHand,target1);
        }
      }

    }

    bool supportPositionClose = supportArmData.currentGunSupportDistanceFromHand < m_parameters.makeConstraintDistance;
    if (useConstraint && (m_parameters.supportConstraint == 1 || m_parameters.supportConstraint == 3)  && supportPositionClose)
    {
      m_character->constrainSupportHand(
        primaryArmData.arm,
        supportArmData.arm,
        &supportArmData.currentGunSupportInWorld.d, 
        m_parameters.constraintMinDistance,
        m_parameters.reduceConstraintLengthVel*m_character->getLastKnownUpdateStep(),
        m_parameters.breakingStrength,
        (m_parameters.supportConstraint == 1));

    }
    else 
    {
      if(m_character->isSupportHandConstraintActive())
        m_character->releaseSupportHandConstraint();
    }	

#if 1 // need to sort out support target velocity before this can be used.
    if(m_parameters.posStiff > 0.0f && m_character->isSupportHandConstraintActive())
      m_character->pdPartPosition(supportArmData.arm->getHand(), supportArmData.desiredSupportHandToWorld.d, NULL, m_parameters.posStiff, m_parameters.posDamp);
#endif
  }

  bool NmRsCBUPointGun::isConstraintBroken(ArmData& primaryArmData, ArmData& supportArmData)
  {
    ArmStatus supportArmStatus = kUnused;
    ArmStatus gunArmStatus = kUnused;

    if (m_supportConstraintInhibitTimer <= 0.0f)//Constraint was not broken last frame
    {
      //Release constraint if it is broken
      if (m_character->m_handToHandConstraintHandle.IsValid()) 
      {
        rage::phConstraintBase* baseConstraint = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(m_character->m_handToHandConstraintHandle) );
        if (baseConstraint && baseConstraint->IsBroken())
        {
          m_character->releaseSupportHandConstraint();
          m_supportConstraintInhibitTimer = m_parameters.brokenSupportTime; //Delay attempts to re-constrain by 1 second
          if (m_parameters.brokenToSideProb >= m_character->getRandom().GetFloat())
            m_neutralSidePoseTimer = m_supportConstraintInhibitTimer;
        }
      }

      //Break constraint if constraint has previously been good but hands are now too far apart
      bool constraintActive = supportConstraintActive(supportArmData, 0.17f);
      if (m_forceConstraintMade && (!constraintActive) && supportArmData.status == kSupporting)
      {
        m_supportConstraintInhibitTimer = m_parameters.brokenSupportTime; //Delay attempts to re-constrain by 1 second
        if (m_parameters.brokenToSideProb >= m_character->getRandom().GetFloat())
          m_neutralSidePoseTimer = m_supportConstraintInhibitTimer;
        m_forceConstraintMade = false;
      }
      else if (supportArmData.status != kSupporting)
        m_forceConstraintMade = false;
      else if (!(m_forceConstraintMade && supportArmData.status == kSupporting))
        m_forceConstraintMade = supportConstraintActive(supportArmData, 0.13f);
    }

    //m_supportConstraintInhibitTimer is set above so don't be tempted to make below an else.
    if (m_supportConstraintInhibitTimer > 0.0f)
    {
      m_supportConstraintInhibitTimer -= m_character->getLastKnownUpdateStep();
      supportArmData.desiredStatus = kNeutralBroken;
      supportArmStatus = setNeutralPose(supportArmData);

      if (m_weaponMode == kRifle)//Never point even if oneHandedPointing OK
      {
        primaryArmData.desiredStatus = kNeutralBroken;
        if (m_rifleFalling)
        {
          gunArmStatus = kNeutralPose;
          moveGunArm(primaryArmData, gunArmStatus, false);
        }
        else
          gunArmStatus = setNeutralPoseOrNeutralPoint(primaryArmData);
      }
      else// kPistol only point if oneHandedPointing allowed
      {
        gunArmStatus = computeGunArmState(primaryArmData);//returns kUnused,kNeutral or kPointing
        primaryArmData.desiredStatus = gunArmStatus;

        //change gunArmStatus == kPointing to kNeutral if oneHanded pointing not allowed
        if (gunArmStatus == kPointing && !(m_parameters.oneHandedPointing == 1 || m_parameters.oneHandedPointing == 3))
        {
          gunArmStatus = kNeutral;
        }
        gunArmStatus = moveGunArm(primaryArmData, gunArmStatus, true);//returns kUnused, kPointingWithError, kPointing, kNeutralPose, kNeutralPointing
      }
      rifleConstraintTick(primaryArmData);

      setArmStatus(supportArmData, supportArmStatus);
      setArmStatus(primaryArmData, gunArmStatus);
      return true;//OUTPUT
    }

    return false;

  }
  // uses primaryArmData.desiredHandToWorld
  //Calculates:
  // supportArmData.currentGunSupportInWorld
  // supportArmData.desiredGunSupportInWorld
  // supportArmData.currentGunSupportDistanceFromHand
  // supportArmData.currentGunSupportDistanceFromShoulder
  // supportArmData.desiredGunSupportDistanceFromShoulder
  // supportArmData.currentGunSupportable
  // supportArmData.desiredGunSupportable
  bool NmRsCBUPointGun::isSupportHandTargetReachable(ArmData& primaryArmData, ArmData& supportArmData)
  {
    //SupportArmStatus
    //Can't support if either arm not being used
    if(!(armEnabled(supportArmData.arm) && armEnabled(primaryArmData.arm)))
    {
      setArmStatus(supportArmData, kUnused);
      if(m_character->isSupportHandConstraintActive())
        m_character->releaseSupportHandConstraint();
      return false;//OUTPUT
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
#if NM_TESTING_NEW_REGISTERWEAPON_MESSAGE
    //The new resisterWeapon message is sent in by script after the pointGun has activated
    //so this code needs to be run in the tick when testing
    //Find supportHandToGun  
    rage::Matrix34 worldToGunHand, supportHandToWorld;
    m_character->getITMForPart(getRightArm()->getHand()->getPartIndex(), &worldToGunHand);//gunHandToWorld
    m_character->getITMForPart(getLeftArm()->getHand()->getPartIndex(), &supportHandToWorld);
    worldToGunHand.Inverse();

    rage::Matrix34 worldToGunAiming;
    rage::Matrix34 handToGun;
    handToGun = m_character->m_gunToHandAiming[NmRsCharacter::kRightHand];
    handToGun.Inverse();
    worldToGunAiming.Dot(worldToGunHand,handToGun);
    m_supportHandToGun.Dot(supportHandToWorld, worldToGunAiming);
#endif//#if NM_TESTING_NEW_REGISTERWEAPON_MESSAGE

    //update the m_supportHandToGunHand using m_supportHandToGun
    m_supportHandToGunHand.Dot(m_supportHandToGun, primaryArmData.gunToHand);

    // *constraint* target is always relative to the *hand*.
    rage::Matrix34 gunHandToWorld;
    primaryArmData.arm->getHand()->getMatrix(gunHandToWorld);
    supportArmData.currentGunSupportInWorld.Dot(m_supportHandToGunHand, gunHandToWorld);
    supportArmData.desiredGunSupportInWorld.Dot(m_supportHandToGunHand, primaryArmData.desiredHandToWorld);
    //gunHandToWorld.Transform(m_supportHandToGunHand.d, primaryArmData.currentGunSupportPosInWorld);
    //primaryArmData.desiredHandToWorld.Transform(m_supportHandToGunHand.d, primaryArmData.desiredGunSupportPosInWorld);
    supportArmData.currentGunSupportDistanceFromHand = supportArmData.currentGunSupportInWorld.d.Dist(supportArmData.arm->getHand()->getPosition());

    // calculate support target distances
    supportArmData.currentGunSupportDistanceFromShoulder = supportArmData.currentGunSupportInWorld.d.Dist(supportArmData.arm->getShoulder()->getJointPosition());
    supportArmData.desiredGunSupportDistanceFromShoulder = supportArmData.desiredGunSupportInWorld.d.Dist(supportArmData.arm->getShoulder()->getJointPosition());
    //Very simple metric to work out whether the support target is reachable
    supportArmData.currentGunSupportable = supportArmData.currentGunSupportDistanceFromShoulder < m_maxArmReach;
    supportArmData.desiredGunSupportable = supportArmData.desiredGunSupportDistanceFromShoulder < m_maxArmReach;
    //The above HAS to have the correct geometry otherwize support arm will be left hanging in space looking bad
#if 0 //Restrict support
    //Does clamped IK solution get near the target?
    //Approximate above by
    //  For right side of body targets:
    //    angle<max lean2 and currentGunSupportDistance < m_maxArmReach
    //    angle>max && angle <max+angle between upperArm and hand when elbow 90deg && distance(elbow at max 2 target) < forearm length
    //Is target angle reachable?
    //Project toTarget onto plane defined by spine3Up normal //(a,b,c) (Up, left, back)
    rage::Matrix34 spine3TM;
    getSpine()->getSpine3Part()->getMatrix(spine3TM);
    rage::Vector3 toTarget = supportArmData.desiredSupportHandToWorld.d - supportArm->getShoulder()->getJointPosition();
    toTarget.AddScaled(toTarget, spine3TM.a, -spine3TM.a.Dot(toTarget));//spine3TM.a = up
    toTarget.Normalize();
    //Get angle of target from spine3 forward (-spine3TM.c).   +ve is left (for the right arm)//should just see whether inside shoulder joint range
    float supportTargetAngle = rage::AcosfSafe(toTarget.Dot(-spine3TM.c));
    if (toTarget.Dot(spine3TM.b) < 0.0f)//-spine3TM.b = right
      supportTargetAngle *= -1.0f;
#if ART_ENABLE_BSPY & PointGunBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "NmRsCBUPointGun", supportTargetAngle);
#endif
    //static float supportTargetAngleMax = 
#endif //Restrict support

    return (supportArmData.desiredGunSupportable);//return for kPointing decision
  }

  //Are the hands working together to reach the target?
  bool NmRsCBUPointGun::supportConstraintActive(ArmData& supportArmData, float distToConstraint)
  {
    return (m_character->isSupportHandConstraintActive()) //mmmmTODO perhaps just make this <0.15 and have another function for connectingUsingConstraint
      || (m_parameters.supportConstraint == 2 && supportArmData.currentGunSupportDistanceFromHand < distToConstraint);//ForceConstraint or no constraint
    //Note if the gun is not physical then supporting without a constraint (m_parameters.supportConstraint == 0)
    // will lead to the supporting arm passing upwards through the gun if supportConstraintActive.
    // If gun physical m_parameters.supportConstraint == 2 -> !=1 above but needs testing
  }

  //Used to determine when gunHand should get the benefit of the lower arms strength
  bool NmRsCBUPointGun::supportConstraintEffective(ArmData& supportArmData)
  {
    static float distToConstraint = 0.1f;
    return (supportArmData.status == kSupporting && supportArmData.currentGunSupportDistanceFromHand < distToConstraint);

  }

  void NmRsCBUPointGun::pointDoubleHandedGun(ArmData& primaryArmData, ArmData& supportArmData)
  {

    ArmStatus supportArmStatus = kNeutral;
    ArmStatus gunArmStatus;
    if (!(m_parameters.enableRight))//nothing to do
    {
      gunArmStatus = kUnused;
      supportArmStatus = kUnused;
    }
    else
    {
     //supportArmData.desiredTargetGunSupportable = false; done at start of tick
      //Decide what to do with the gunHand
      NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      NmRsCBUShot* shotTask = (NmRsCBUShot*)m_cbuParent->m_tasks[bvid_shot];
      Assert(shotTask);
      bool falling = ((dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK) || (shotTask->isActive() && shotTask->getFallToKneesIsFalling()));
      rage::Matrix34 predCOMTM;
      NmRsCharacter::OrientationStates orientationState = NmRsCharacter::OS_NotSet;
      if (!m_rifleFalling)
      {
        bool rifleFalling = falling && m_weaponMode == kRifle && (m_parameters.rifleFall != 0);
        if (rifleFalling && m_parameters.rifleFall == 2)
        {
          rifleFalling = false;
          // Calculate character predicted facing direction.
          m_character->getPredictedCOMOrientation(0.2f, &predCOMTM);
          orientationState = m_character->getFacingDirectionFromCOMOrientation(predCOMTM);
          if (!(orientationState == NmRsCharacter::OS_Up || orientationState == NmRsCharacter::OS_Back || orientationState == NmRsCharacter::OS_Down))
            rifleFalling = true;
  #if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "pgfalling", orientationState);
  #endif
        }
        m_rifleFalling = rifleFalling;
        if (m_rifleFalling)//this is the 1st time we start to fall only.  
        {
          //If the gunHand pose is to the side then keep it there for falling otherwise set it to a sort of pistol neutral pose 
          m_rifleFallingPose = npsRifleFall;
          if (m_neutralSidePoseTimer > 0.0f && m_supportConstraintInhibitTimer > 0.0f) //broken neutral pose will be by the side
            m_rifleFallingPose = npsRifleFallBySide;//If this connects it will look bad but at the moment this pose is out of range for the support arm
        }
      }    
      //fallingTypeSupport //(What is considered a fall by fallingSupport). 
      //Apply fallingSupport 0=never(will support if allowed), 1 = falling, 2 = falling except if falling backwards, 3 = falling and collided, 4 = falling and collided except if falling backwards, 5 = falling except if falling backwards until collided
      if (!m_decideFallingSupport && m_parameters.fallingTypeSupport != 0)
      {
        bool decideFallingSupport = falling && (m_weaponMode == kRifle || m_weaponMode == kPistol);//i.e. fallingTypeSupport == 1
        //do we have to not be falling backwards?
        bool fallingBackWards = false;
        if (decideFallingSupport && (m_parameters.fallingTypeSupport == 2 || m_parameters.fallingTypeSupport == 4 || m_parameters.fallingTypeSupport == 5))
        {
          // Calculate character predicted facing direction.
          if (orientationState == NmRsCharacter::OS_NotSet)
          {
            m_character->getPredictedCOMOrientation(0.2f, &predCOMTM);
            orientationState = m_character->getFacingDirectionFromCOMOrientation(predCOMTM);
  #if ART_ENABLE_BSPY
            bspyScratchpad(m_character->getBSpyID(), "pgfalling", orientationState);
  #endif
          }
          if (orientationState == NmRsCharacter::OS_Up || orientationState == NmRsCharacter::OS_Back || orientationState == NmRsCharacter::OS_Down)
            fallingBackWards = true;
        }
        //do we have to have collided with the upperbody?
        bool collided = true;
        if (decideFallingSupport && (m_parameters.fallingTypeSupport == 3 || m_parameters.fallingTypeSupport == 4 || m_parameters.fallingTypeSupport == 5))
        {
            collided = m_character->hasCollidedWithEnvironment(bvmask_UpperBody);
            if (collided && m_parameters.fallingTypeSupport == 5)
              fallingBackWards = false;
        }
  #if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "pgfalling", decideFallingSupport);
        bspyScratchpad(m_character->getBSpyID(), "pgfalling", fallingBackWards);
        bspyScratchpad(m_character->getBSpyID(), "pgfalling", collided);
  #endif
        m_decideFallingSupport = decideFallingSupport && (!fallingBackWards) && (collided);
      }

      //Special case.  Don't support or constrain for a while after the constraint has been broken.
      //Arms are set in isConstraintBroken if the constraint is broken and status updated.
      //mmmmTODO use output of this routine but move the arms here
      if (isConstraintBroken(primaryArmData, supportArmData))
        return;

      //Change the gunArmStatus based on what the support hand can do
      bool calculate = true;//for speed
      bool isSupportHandTargetReachable_StillTheSame = false;//for speed
      bool oneHandedPoint = false;
      gunArmStatus = computeGunArmState(primaryArmData);//returns kUnused,kNeutral or kPointing
      //save this gun hand status to bSpy as this may be changed later by what the support can do
      primaryArmData.desiredStatus = gunArmStatus;

      //fallingSupport //Allow supporting of a rifle(or two handed pistol) when falling. 
      //0 = false, 1 = support if allowed, 2 = support until constraint not active (don't allow support to restart), 3 = support until constraint not effective (support hand to support distance must be less than 0.15 - don't allow support to restart).
      if (m_allowFallingSupportArm && m_decideFallingSupport && 
        (m_parameters.fallingSupport == 0 ||
        (m_parameters.fallingSupport == 2 &&  (!supportConstraintActive(supportArmData, 0.15f))) ||
        (m_parameters.fallingSupport == 3 &&  (!supportConstraintEffective(supportArmData))) ))
        m_allowFallingSupportArm = false;//Can never go back to true

      if ((!m_allowFallingSupportArm) || (!(m_parameters.enableLeft && m_enableLeft)))
      {
        supportArmData.desiredStatus = kUnused;
        supportArmStatus = kUnused;
      }

      //If the gun hand can point then it may still go to kNeutral if the support hand cannot support it.
      if (gunArmStatus == kPointing)
      {
        //Calculates primaryArmData.weaponDistance, primaryArmData.desiredHandToWorld
        calculateGunHand(primaryArmData);//mmmm unnecessary if supportArmStatus = kUnused but left in for clearness 
        //NB isSupportHandTargetReachable returns true if supportArmData.desiredGunSupportable true
        bool pointTheGunHand = (supportArmStatus != kUnused && m_parameters.enableLeft && m_enableLeft && isSupportHandTargetReachable(primaryArmData, supportArmData)) || 
          (m_parameters.oneHandedPointing == 1 && m_weaponMode == kPistol) ||
          (m_parameters.oneHandedPointing == 2 && m_weaponMode == kRifle) ||
          m_parameters.oneHandedPointing == 3;
        //pointing target and support pos for that has been calculated
        supportArmData.desiredTargetGunSupportable = supportArmData.desiredGunSupportable;
        supportArmData.desiredTargetGunSupportDistanceFromShoulder = supportArmData.desiredGunSupportDistanceFromShoulder;
        if (pointTheGunHand)
        {
          if (!supportArmData.desiredGunSupportable)
            oneHandedPoint = true;
          isSupportHandTargetReachable_StillTheSame = true;//it is still kPointing
          calculate = false;
        }
        else
        {
          //change gunArmStatus == kPointing to kNeutral if oneHanded pointing not allowed
          gunArmStatus = kNeutral;
        }
      }    
      //need to know at this point whether kNeutral -> kUnused
      //should have pistol connect option here as well that stops it going to unused?
      if (gunArmStatus == kNeutral && !(poseUnusedArm(primaryArmData)))
        gunArmStatus = kUnused;

      //Can't be sure what an non-enabled right arm is doing therefore don't try to support it
      if (gunArmStatus == kUnused)
        supportArmStatus = kUnused;//Should have the same effect as setting m_parameters.enableLeft = false
    
      if (supportArmStatus != kUnused)
      {
        //Calculate the gun hand again if neutral pointing.  rifleFalling gunArmStatus is kNeutralPose
        if (gunArmStatus ==  kNeutral)
        {
          if (selectNeutralTarget(primaryArmData))//fills in armData.target
          {
            gunArmStatus = kNeutralPointing;
            calculateGunHand(primaryArmData, kNeutralPointing);
          }
        }
        //Calculates:
        // supportArmData.currentGunSupportInWorld
        // supportArmData.desiredGunSupportInWorld
        // supportArmData.currentGunSupportDistanceFromHand
        // supportArmData.currentGunSupportDistanceFromShoulder
        // supportArmData.desiredGunSupportDistanceFromShoulder
        // supportArmData.currentGunSupportable
        // supportArmData.desiredGunSupportable
        // uses primaryArmData.desiredHandToWorld
        //returns kUnused or supportArmData.currentGunSupportable + supportArmData.desiredGunSupportable 
        //kNeutralPointing gives a desiredGunSupportInWorld only if isSupportHandTargetReachable called after moveGunArm
        if (!isSupportHandTargetReachable_StillTheSame)
          isSupportHandTargetReachable(primaryArmData, supportArmData);

        //Uses supportArmData.currentGunSupportInWorld
        supportArmData.supportBehindBack = isConstraintBehindBack(primaryArmData, supportArmData);

        bool constraintActive = supportConstraintActive(supportArmData, 0.15f);

        bool supportTargetReachable = (constraintActive && gunArmStatus == kPointing && supportArmData.desiredGunSupportable) ||
          (supportArmData.currentGunSupportable && !oneHandedPoint);//mmmmTODO Does it let go of a neutralPointing (This is done for NeutralPose below)

        if( (gunArmStatus == kPointing || m_parameters.alwaysSupport) &&
          !supportArmData.supportBehindBack && 
          supportTargetReachable)
          supportArmStatus = kSupporting;

        if (supportArmStatus != kSupporting)
        {
          //change gunArmStatus == kPointing to kNeutral if oneHanded pointing not allowed
          if (gunArmStatus == kPointing &&
            !(          (m_parameters.oneHandedPointing == 1 && m_weaponMode == kPistol) ||
            (m_parameters.oneHandedPointing == 2 && m_weaponMode == kRifle) ||
            m_parameters.oneHandedPointing == 3))
          {
            gunArmStatus = kNeutral;
            calculate = true;
          }
        }

        // Point the primary arm with the singleHanded version.
        // gunArmStatus in = kUnused,kNeutralPointing,kNeutralPose or kPointing
        gunArmStatus = moveGunArm(primaryArmData, gunArmStatus, calculate);//returns kUnused, kPointingWithError, kPointing, kNeutralPose, kNeutralPointing

        supportArmData.desiredStatus = supportArmStatus;

        //Decide not to support if the desired support target of a neutralPose is not reachable
        // so that unreachable neutralPoses disconnect.
        //fk the desired arm to get desiredGunSupportInWorld - only needed for kNeutralPose as already set for kPointing, kPointingWithError, kNeutralPointing 
        if (gunArmStatus == kNeutralPose && supportArmStatus == kSupporting)
        {
          rage::Matrix34 desiredHandTM;
          getDesiredHandTM(desiredHandTM);//We could just fk the neutral pose (without applying it as desired)
  #if ART_ENABLE_BSPY
          m_character->bspyDrawCoordinateFrame(0.2f, desiredHandTM);
  #endif
          primaryArmData.desiredHandToWorld = desiredHandTM;
          supportArmData.desiredGunSupportInWorld.Dot(m_supportHandToGunHand, desiredHandTM);
          // calculate support target distances
          supportArmData.desiredGunSupportDistanceFromShoulder = supportArmData.desiredGunSupportInWorld.d.Dist(supportArmData.arm->getShoulder()->getJointPosition());
          //Very simple metric to work out whether the support target is reachable
          supportArmData.desiredGunSupportable = supportArmData.desiredGunSupportDistanceFromShoulder < m_maxArmReach;
          supportTargetReachable = (supportArmData.currentGunSupportable && supportArmData.desiredGunSupportable);
          if (!supportTargetReachable)
            supportArmStatus = kNeutral;
        }

        //supportArmStatus should now be either kUnused,kNeutral or kSupporting
        switch (supportArmStatus)
        {
        case kSupporting:
          {
            //if constraint has been made, *support* target is relative to primary *hand target*...
            // unless the primary hand can't point (therefore is in neutral pose) make it relative to the primary *hand*
            //  isSupportHandTargetReachable sets desiredGunSupportInWorld and currentGunSupportInWorld
            bool currentGunSupport = false;
            //The || gunArmStatus == kPointingWithError below helps aim the rifles better and inhibits (duration of and) straightness of the support arm
            //  This may cause bad gunHand pose or wrists as the support hand tries to pull the gun into aim
            if(constraintActive && (gunArmStatus == kPointing || gunArmStatus == kPointingWithError || 
              //Neutral desired pointing position is not supportable therefore either only support currentGunSupportInWorld (or you could set a neutralPose on the gunHand)
              (gunArmStatus == kNeutralPointing && supportArmData.desiredGunSupportable)))
              supportArmData.desiredSupportHandToWorld = supportArmData.desiredGunSupportInWorld;
            else
            {
              //rifleFall (kNeutralPose) will drop out here
              // ...otherwise, it is relative to the current primary *hand*
              supportArmData.desiredSupportHandToWorld = supportArmData.currentGunSupportInWorld;
              currentGunSupport = true;
            }

            //lerp between supportArmData.desiredSupportHandToWorld and 0,0,0 of hand (joint axis of wrist)
            // based on currentGunSupportDistanceFromHand
            static float beginWristBlendDist = 0.2f;
            static float gunOrientationWristDist = 0.1f;
            float blend = 0.0f;
            if (supportArmData.currentGunSupportDistanceFromHand > beginWristBlendDist)
              blend = 0.0f;
            else if (supportArmData.currentGunSupportDistanceFromHand < gunOrientationWristDist)
              blend = 1.0f;
            else
              blend = (beginWristBlendDist - supportArmData.currentGunSupportDistanceFromHand)/(beginWristBlendDist-gunOrientationWristDist); 
  #if ART_ENABLE_BSPY & PointGunBSpyDraw
            bspyScratchpad(m_character->getBSpyID(), "pointGun", blend);
            bspyScratchpad(m_character->getBSpyID(), "pointGun", supportArmData.desiredSupportHandToWorld.d);
  #endif

            static float velScale = 0.1f;
            rage::Matrix34 movingTarget(supportArmData.desiredSupportHandToWorld);
            //compensate for velocity only if moving to constraint
            //When we are aiming for the desiredGunSupport we are kind of compensating for velocity anyway
            if (currentGunSupport)
              movingTarget.d += velScale*getRightArm()->getHand()->getLinearVelocity(&movingTarget.d);

            float twist = m_parameters.elbowAttitude;
            ik(supportArmData, ((HINGE_DIRECTION(supportArmData.arm)) * twist), movingTarget, blend);
            //armData above used for recoil in clavicleIK only (for current ik setup) 
            supportArmStatus = kSupporting;
  #if NM_RUNTIME_LIMITS
            // open wrist limits for better pointing.
            // todo test if this is necessary for pistol.
            if (blend > 0.0f)
              (static_cast<NmRs3DofEffector*>(m_character->getEffectorDirect(supportArmData.arm->getWrist()->getJointIndex())))->setLimits(1.5f, 1.5f, 1.5f);
  #endif
          }
          break;
        case kNeutral:
          m_forceConstraintMade = false;
          supportArmStatus = setNeutralPose(supportArmData);//returns kUnused or kNeutralPose
          break;
        default:
          break;
        }

      }//if (supportArmStatus != kUnused)
      else
      {
        gunArmStatus = moveGunArm(primaryArmData, gunArmStatus, false);//returns kUnused, kPointingWithError, kPointing, kNeutralPose, kNeutralPointing
      }
    }

    //We update arm status only at the end so that it is changed only once
    setArmStatus(supportArmData, supportArmStatus);
    setArmStatus(primaryArmData, gunArmStatus);

    //Uses primaryArmData.gunToHand, primaryArmData.weaponDistance, primaryArmData.targetDirection, primaryArmData.pointingFromPosition
    // constrain rifle stock to chest
    //Therefore should only be used if calculateGunHand has been called prior
    rifleConstraintTick(primaryArmData);
    
    supportConstraintUpdate(primaryArmData, supportArmData);

    if (primaryArmData.status == kUnused && primaryArmData.desiredStatus == kPointing && m_reConnectTimer <=0.0f && supportArmData.desiredTargetGunSupportable)
    {
      m_gunArmNotPointingTime += m_character->getLastKnownUpdateStep();
      m_reConnectTimer = 0.0f;
    }
    else
      m_gunArmNotPointingTime = 0.0f;

    if (m_gunArmNotPointingTime > m_parameters.connectAfter)
      m_reConnectTimer = m_parameters.connectFor;

    if (m_reConnectTimer > 0.0f)
      m_reConnectTimer -= m_character->getLastKnownUpdateStep();

  }

  void NmRsCBUPointGun::rifleConstraintTick(ArmData& primaryArmData)
  {
    //mmmmTODO decide whether to have rifleConstraint even for neutralPose?
    //NB: Release the constraint if primaryArmData.status = kUnused
    //  weaponDistance only calculated in pointGunHand
    //Uses primaryArmData.gunToHand, primaryArmData.weaponDistance, primaryArmData.targetDirection, primaryArmData.pointingFromPosition
    if(m_weaponMode == kRifle && m_parameters.constrainRifle /*&& m_character->isSupportHandConstraintActive()*/ && (primaryArmData.status == kPointing || primaryArmData.status == kNeutralPointing || primaryArmData.status == kPointingWithError))
    {
      rage::Matrix34 gunToWorld;
      primaryArmData.arm->getHand()->getMatrix(gunToWorld);//handToWorld

      //handConstraintPos = From hand centre go back weaponDistance in the barrel direction.
      rage::Vector3 handConstraintPos;
      gunToWorld.Dot3x3FromLeft(primaryArmData.gunToHand); // adjust for weapon offset in hand. gunToHand.handToWorld
      rage::Vector3 offset(-primaryArmData.weaponDistance, 0.0f, 0.0f);
      gunToWorld.Transform(offset, handConstraintPos);

      //spine3ConstraintPos = pointingFromPosition + small distance in the target direction
      rage::Vector3 spine3ConstraintPos;
      float offsetDistance = m_parameters.rifleConstraintMinDistance - 0.05f; // offset constraint target towards target.
      spine3ConstraintPos = primaryArmData.pointingFromPosition + (offsetDistance*primaryArmData.targetDirection);
#if ART_ENABLE_BSPY & PointGunBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "rifle", handConstraintPos);
      bspyScratchpad(m_character->getBSpyID(), "rifle", spine3ConstraintPos);
#endif
      updateRifleConstraint(primaryArmData.arm, handConstraintPos, spine3ConstraintPos, m_parameters.rifleConstraintMinDistance);

    }
    else
    {
      releaseRifleConstraint();
    }
  }

  // Manage a distance constraint between the the gunHand and spine3
  // Either create the constraint or
  // if constraint has already been made, slowly reduce the distance 
  // that we are away from the desired position.
  void NmRsCBUPointGun::updateRifleConstraint(
    const NmRsHumanArm *primaryArm,
    rage::Vector3& handConstraintPos,
    rage::Vector3& spine3ConstraintPos,
    float minimumDistance /* = 0 */)
  {
    if (m_rifleConstraint.IsValid())
    {
      // if constraint has already been made, slowly reduce the distance 
      // that we are away from the desired position.
      m_rifleConstraintDistance = rage::Clamp((0.7f*m_rifleConstraintDistance), minimumDistance, 10.0f);
      m_rifleConstraintDistance = rage::Max(NM_MIN_STABLE_DISTANCECONSTRAINT_DISTANCE, m_rifleConstraintDistance);
      rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_rifleConstraint) );
      if (distConstraint)
      {
        Assert(distConstraint->GetType() == rage::phConstraintBase::DISTANCE);
        distConstraint->SetMaxDistance(m_rifleConstraintDistance);
        distConstraint->SetWorldPosB(VECTOR3_TO_VEC3V(spine3ConstraintPos));
      }
    }
    else
    {
      // Create the constraint.
      m_rifleConstraintDistance = handConstraintPos.Dist(spine3ConstraintPos);
      m_character->constrainPart(
        m_rifleConstraint, 
        primaryArm->getHand()->getPartIndex(),
        getSpine()->getSpine3Part()->getPartIndex(),
        m_rifleConstraintDistance,
        handConstraintPos,
        spine3ConstraintPos,
        m_character->getFirstInstance()->GetLevelIndex(),
        false,
        true, //This has to be a distance constraint
        NM_MIN_STABLE_DISTANCECONSTRAINT_DISTANCE);
    }
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    // draw the constraint radius on clavicle.
    rage::Vector3 worldPosA(VEC3V_TO_VECTOR3(m_rifleConstraint->GetWorldPosA()));
    rage::Vector3 worldPosB(VEC3V_TO_VECTOR3(m_rifleConstraint->GetWorldPosB()));
    // draw a cross representing the radius of the constraint.
    float radius = m_rifleConstraint->GetLength();
    m_character->bspyDrawPoint(worldPosA, 0.05f, rage::Vector3(0,0,1));
    m_character->bspyDrawSphere(worldPosB, radius, rage::Vector3(1,1,1));
    m_character->bspyDrawPoint(worldPosB, 0.05f, rage::Vector3(1,0,0));
#endif
  }

  void NmRsCBUPointGun::releaseRifleConstraint()
  {
    m_character->ReleaseConstraintSafely(m_rifleConstraint);
  }

  NmRsCBUPointGun::ArmStatus NmRsCBUPointGun::setNeutralPoseType(ArmData& armData, NeutralPoseState pose, bool checkAllowed)
  {
    NmRsArmInputWrapper* armInpuData = armData.inputData;

    if (!checkAllowed || poseUnusedArm(armData))
    {
      switch (pose)
      {
      case npsBySide:
      case npsRifleFallBySide:
        nmrsSetTwist(armInpuData->getClavicle(),-0.01543f);
        nmrsSetLean1(armInpuData->getClavicle(),0.05176f);
        nmrsSetLean2(armInpuData->getClavicle(),-0.03992f);
        nmrsSetTwist(armInpuData->getShoulder(),0.53650f);
        nmrsSetLean1(armInpuData->getShoulder(),-0.00119f);
        nmrsSetLean2(armInpuData->getShoulder(),0.18666f);
        nmrsSetAngle(armInpuData->getElbow(),0.63664f);
        nmrsSetTwist(armInpuData->getWrist(),-0.1965f);
        nmrsSetLean1(armInpuData->getWrist(),0.1214f);
        nmrsSetLean2(armInpuData->getWrist(),0.041f);
        break;
      case npsByFace:
      case npsRifleFall:
      case npsByFaceSupport://mmmmTODO Should be complimentary to this: looking like hands are together.  Should allow hands to connect
        nmrsSetTwist(armInpuData->getClavicle(),0.0f);
        nmrsSetLean1(armInpuData->getClavicle(),.2f);
        nmrsSetLean2(armInpuData->getClavicle(),0.0f);
        nmrsSetTwist(armInpuData->getShoulder(),-0.2f);
        nmrsSetLean1(armInpuData->getShoulder(),0.53f);
        nmrsSetLean2(armInpuData->getShoulder(),0.35f);
        nmrsSetAngle(armInpuData->getElbow(),2.0f);
        nmrsSetTwist(armInpuData->getWrist(),0.0f);
        nmrsSetLean1(armInpuData->getWrist(),0.0f);
        nmrsSetLean2(armInpuData->getWrist(),0.0f);
        break;
      case npsAcrossFront:
      case npsByHip://mmmmTODO
        nmrsSetTwist(armInpuData->getClavicle(),0.2f);
        nmrsSetLean1(armInpuData->getClavicle(),-0.16f);
        nmrsSetLean2(armInpuData->getClavicle(),-0.12f);
        nmrsSetTwist(armInpuData->getShoulder(),0.583f);
        nmrsSetLean1(armInpuData->getShoulder(),0.318f);
        nmrsSetLean2(armInpuData->getShoulder(),0.07f);
        nmrsSetAngle(armInpuData->getElbow(),1.78f);
        nmrsSetTwist(armInpuData->getWrist(),-0.3f);
        nmrsSetLean1(armInpuData->getWrist(),-0.37f);
        nmrsSetLean2(armInpuData->getWrist(),0.68f);
        break;
      case npsAcrossFrontSupport:
      case npsByHipSupport://mmmmTODO
        nmrsSetTwist(armInpuData->getClavicle(),0.04f);
        nmrsSetLean1(armInpuData->getClavicle(),0.33f);
        nmrsSetLean2(armInpuData->getClavicle(),0.04f);
        nmrsSetTwist(armInpuData->getShoulder(),0.31f);
        nmrsSetLean1(armInpuData->getShoulder(),0.17f);
        nmrsSetLean2(armInpuData->getShoulder(),0.35f);
        nmrsSetAngle(armInpuData->getElbow(),0.66f);
        nmrsSetTwist(armInpuData->getWrist(),0.0f);//-1.7
        nmrsSetLean1(armInpuData->getWrist(),0.0f);//-0.18
        nmrsSetLean2(armInpuData->getWrist(),0.0f);//0.4
        break;
      default://npsNone, npsNotRequested
        return kUnused;
      }
      armData.neutralPoseType = pose;
      return kNeutralPose;
    }
    return kUnused;    
  }

  //status changed to kUnused or kNeutralPose or kNeutralPointing
  NmRsCBUPointGun::ArmStatus NmRsCBUPointGun::setNeutralPoseOrNeutralPoint(ArmData& armData)
  {
    if (selectNeutralTarget(armData))//fills in armData->target
    {
      pointGunHand(armData, true, kNeutralPointing);//we ignore the kPointing/kPointingWithError return values.  As we want to tell the game we are not pointing at the target
      return kNeutralPointing;
    }
    else
    {
      return setNeutralPose(armData);//returns kUnused or kNeutralPose
    }
  }

  //status changed to kUnused or kNeutralPose
  NmRsCBUPointGun::ArmStatus NmRsCBUPointGun::setNeutralPose(ArmData& armData)
  {
    NmRsHumanArm* arm = armData.arm;

    if(poseUnusedArm(armData))
    {
#if ART_ENABLE_BSPY 
      m_character->m_currentSubBehaviour = "-neutral"; 
#endif
      if (m_weaponMode == kSidearm && arm == getLeftArm())
      {
        setNeutralPoseType(armData, npsBySide);
      }
      else if (m_weaponMode == kPistol && arm == getLeftArm())
      {
        // looks bad setNeutralPoseType(arm, armData, npsByFaceSupport);
        setNeutralPoseType(armData, npsBySide);
      }
      else if (m_weaponMode != kRifle)
      {
        //for pistol Hand
        switch(m_parameters.pistolNeutralType) 
        {
        case 0://byFace
          setNeutralPoseType(armData, npsByFace);
          break;
        case 1://acrossFront
          setNeutralPoseType(armData, npsAcrossFront);
          break;
        case 2://bySide
          setNeutralPoseType(armData, npsBySide);//NB: won't be able to be connected so be careful if combined with kPistol and oneHandedPointing = 0 or 2
          break;
        }
      }
      else//Rifle
      {
        //if right hand rifle with left support
        if (arm == getLeftArm())
        {
          setNeutralPoseType(armData, npsAcrossFrontSupport);
        }
        else
        {
          if (m_rifleFalling)
          {
            setNeutralPoseType(armData, m_rifleFallingPose);
          }
          else if (m_neutralSidePoseTimer > 0.0f && m_supportConstraintInhibitTimer > 0.0f)//Only do if constraint broken
          {
            setNeutralPoseType(armData, npsBySide);
            m_neutralSidePoseTimer -= m_character->getLastKnownUpdateStep();
          }
          else //if (m_neutralSidePoseTimer <= 0.0f)
            setNeutralPoseType(armData, npsAcrossFront);
        }
      }

#if ART_ENABLE_BSPY 
      m_character->m_currentSubBehaviour = ""; 
#endif
      return kNeutralPose;
    }
    return kUnused;

  }

  //mmmmTODO take a closer look at fireWeapon
  //Only fireWeapon if weapon attached to hand?  Or assume a hand direction and fire anyway.  
  //Simulate reload by force to support hand only if supporting?
  //Apply forces to supporting hand as well if supporting?  
  //Force on hand from gun barrel instead of hand center?
  //better bSpy output
  void NmRsCBUPointGun::fireWeapon(NmRsHumanBody& body, int hand, float fireStrength, bool applyFireGunForceAtClavicle, float inhibitTime, rage::Vector3& direction, float split /* = 0.5 */)
  {
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    bspyLogf(info, L"fireWeapon force %f", fireStrength);
#endif

    const NmRsHumanArm* armSetup = 0;

    // Apply a force to simulate the recoil of the gun.
    // get the desired arm and timer

    // jrp todo are we sure this is set up at this point? seems we don't...
    float &gunTimer = m_armData[NmRsCharacter::kLeftHand].fireInhibitTimer;
    if (hand) // == NmRsCharacter::kRightHand
    {
      gunTimer = m_armData[NmRsCharacter::kRightHand].fireInhibitTimer;
      armSetup = body.getRightArm();
    }
    else
    {
      armSetup = body.getLeftArm();
    }

    NmRsCharacter::AttachedObject* leftWeapon = 0, *rightWeapon = 0;
    m_character->getAttachedWeapons(&leftWeapon, &rightWeapon);

    if (armSetup&&(gunTimer<=0))
    {
      gunTimer = inhibitTime; // 0.4f;

      //We could get gunToWorld directly from the gun
      rage::Matrix34 gunToWorld;
      armSetup->getHand()->getMatrix(gunToWorld);//handToWorld
      // apply stored local weapon offset
      gunToWorld.Dot3x3FromLeft(m_character->m_gunToHandCurrent[hand]);//gunToHand x handToWorld
#if ART_ENABLE_BSPY & PointGunBSpyDraw
      m_character->bspyDrawCoordinateFrame(1.0f, gunToWorld);
#endif

      rage::Vector3 forceV;
      gunToWorld.Transform3x3(direction, forceV);

      // shouldn't force be applied in character func?
      if (applyFireGunForceAtClavicle)
      {
        rage::Vector3 impulse;
        impulse.SetScaled(forceV, (1-split)*fireStrength*m_character->getLastKnownUpdateStep());
        // we really mean to apply force at shoulder.
        armSetup->getClaviclePart()->applyImpulse(impulse, armSetup->getShoulder()->getJointPosition());
        impulse.SetScaled(forceV, split*fireStrength*m_character->getLastKnownUpdateStep());
        armSetup->getHand()->applyImpulse(impulse,armSetup->getHand()->getPosition());
      }
      else
      {
        forceV.Scale(fireStrength*m_character->getLastKnownUpdateStep());
        armSetup->getHand()->applyImpulse(forceV,armSetup->getHand()->getPosition());
      }
    }

    m_armData[hand].relaxTimer = m_parameters.fireWeaponRelaxTime;
    m_armData[hand].relaxScale = 1.0f - m_parameters.fireWeaponRelaxAmount;

    // set stiffnesses now to avoid frame-delay
    if(hand == NmRsCharacter::kRightHand)
    {
      body.setStiffness(m_armData[NmRsCharacter::kRightHand].relaxScale*m_parameters.armStiffness, m_parameters.armDamping, bvmask_ArmRight);
      // do we have a support hand to worry about?
      if((m_weaponMode == kPistol || m_weaponMode == kRifle))//can we see if it is supporting?
      {
        NmRsArmInputWrapper* leftArmInputData = body.getLeftArmInputData();
        leftArmInputData->getClavicle()->setStiffness(m_armData[NmRsCharacter::kLeftHand].relaxScale*m_parameters.armStiffness*0.7f, m_parameters.armDamping);
        leftArmInputData->getShoulder()->setStiffness(m_armData[NmRsCharacter::kLeftHand].relaxScale*m_parameters.armStiffness*0.6f, m_parameters.armDamping);
        leftArmInputData->getElbow()->setStiffness(m_armData[NmRsCharacter::kLeftHand].relaxScale*m_parameters.armStiffness*0.5f, m_parameters.armDamping);
        leftArmInputData->getWrist()->setStiffness(m_armData[NmRsCharacter::kLeftHand].relaxScale*m_parameters.armStiffness*0.3f, m_parameters.armDamping);
      }
    }
    else
    {
      body.setStiffness(m_armData[NmRsCharacter::kLeftHand].relaxScale*m_parameters.armStiffness, m_parameters.armDamping, bvmask_ArmLeft);
    }
  }

  void NmRsCBUPointGun::updateArmData(float timeStep)
  {
    int i, targetIndex;
    rage::Vector3 target;
    ArmData* armData = NULL;

    float amount = 1.0f - m_parameters.fireWeaponRelaxAmount;
    // for lead target calcs.
    rage::Matrix34 spine3TM;
    getSpine()->getSpine3Part()->getMatrix(spine3TM);
    rage::Vector3 spine3AngVel = getSpine()->getSpine3Part()->getAngularVelocity();
    rage::Vector3 spine3Vel = getSpine()->getSpine3Part()->getLinearVelocity();

    //bcr used to request the gun to push off from wall
    //NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    //Assert(balColReactTask);
    //if (balColReactTask->m_pushingOff)
    //m_character->instanceToWorldSpace(&targetInWorld, m_character->m_COM - 3.0f*balColReactTask->m_normal1stContact, -1);//should be collision index
    //  However if this is re-instated then pointGun target and it's vel calculations would be affected unless you put this elsewhere in pointGun and
    //   should also turn off any intoWorldTest.

    for(i = 0; i < 2; ++i)
    {
      // copy stored weapon offset from character and initialize armData
      m_armData[i].gunToHand = m_character->m_gunToHandCurrent[i];
      m_armData[i].neutralPoseType = npsNotRequested;
      m_armData[i].desiredStatus = kNotSet;
      m_armData[i].lastTarget = m_armData[i].currentTarget;
      m_armData[i].desiredTargetGunSupportable = false;

      armData = &m_armData[i];
      if(i == NmRsCharacter::kRightHand)
      {
        targetIndex = m_parameters.rightHandTargetIndex;
        target = m_parameters.rightHandTarget;
        armData->arm = getRightArm();
        armData->input = &getRightArmInput();
        armData->inputData = getRightArmInputData();
      }
      else
      {
        targetIndex = m_parameters.leftHandTargetIndex;
        target = m_parameters.leftHandTarget;
        armData->arm = getLeftArm();
        armData->input = &getLeftArmInput();
        armData->inputData = getLeftArmInputData();
      }

      //mmmmTODO handle lost targets
      //if (targetIndex != -1 && !m_character->getIsInLevel(targetIndex))
      //We have lost the target.  Force neutral (could keep last good target in world? or shoulder matrix1?)
      if (!m_character->getIsInLevel(targetIndex))
        targetIndex = -1;
      m_character->instanceToWorldSpace(&armData->target, target, targetIndex);

      // update target velocity.
      if (targetIndex != -1 && m_character->getLevel()->IsInLevel(targetIndex))
      {
        m_character->getVelocityOnInstance(targetIndex, armData->target, &armData->targetVel);
      }
      else
      {
        //mmmmTODO have a new target parameter if it is not just an updated position otherwise velocity will be totally wrong
        if((armData->lastTarget.Mag2() < 0.01f) || (timeStep < 0.0f))//timeStep < 0.0f means this routine has been called from activate
          armData->targetVel.Zero();
        else
        {
          armData->targetVel.SetScaled(armData->target - armData->lastTarget, 1.0f/m_character->getLastKnownUpdateStep());
        }
      }
      //armData->lastTarget.Set(armData->target);//moved to start of tick - as here it makes the currentTarget and lastTarget the same in bSpy
      armData->currentTarget = armData->target;
      armData->leadTarget.Set(armData->target);
      // lead target based on spine3 motion and targetVelocity.
      if (m_parameters.leadTarget > 0.0f && !(m_weaponMode == kPistol || m_weaponMode == kRifle))
      {
        rage::Vector3 radius = armData->target - spine3TM.d;
        rage::Vector3 targetVel;
        targetVel.Cross(-spine3AngVel, radius); // effect of spine3 rotation
        targetVel.Subtract(spine3Vel);          // effect of spine3 translation
        targetVel.Add(armData->targetVel);      // effect of actual target translation
        //mmmmTODO leadTarget causes target to be bad on the frame of the message due to wrong armData->targetVel?
        armData->leadTarget.AddScaled(targetVel, (m_character->getLastKnownUpdateStep() * m_parameters.leadTarget));
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "PGun:leadTarget", targetVel);
        bspyScratchpad(m_character->getBSpyID(), "PGun:leadTarget", radius);
#endif
      }

      // fire weapon relax.
      if(m_parameters.fireWeaponRelaxTime > 0.0f)
	  {
        armData->relaxScale = amount + (1.0f-armData->relaxTimer/m_parameters.fireWeaponRelaxTime) * (1.0f - amount);
      }
      else
      {
        armData->relaxScale = 1.0f;
      }
      if(timeStep < 0.0f)
      {
        armData->relaxTimer -= m_character->getLastKnownUpdateStep();
      }
      else
      {
        armData->relaxTimer -= timeStep;
      }
      armData->relaxTimer = rage::Clamp(armData->relaxTimer, 0.0f, 5.0f);

      armData->armStatusChanged = false;
    }


  }

  void NmRsCBUPointGun::setHandCollisionExclusion()
  {
    //mmmmTODO - not now: see if we're actually using the arms before turning collisions off?
    //Turn off collisions between the hands - the two handed pistol animation has the hands penetrating
    //if m_parameters.disableArmCollisions=true: also disable collisions between right hand/forearm and the torso/legs.
    //if m_parameters.disableRifleCollisions=true and m_weaponMode == kRifle: also disable collisions between right hand/forearm and spine3/spine2.
    m_character->m_rightHandCollisionExclusion.a = bvmask_HandRight;
    ART::BehaviourMask baseMask = bvmask_HandLeft;
#if NM_POINTGUN_COLLISIONS_OFF

    if((m_weaponMode == kRifle && m_parameters.disableRifleCollisions || m_parameters.disableArmCollisions))
    {
      m_character->m_rightHandCollisionExclusion.a |= bvmask_ForearmRight;
      baseMask |= bvmask_ForearmLeft;
      if(m_weaponMode == kRifle && m_parameters.disableRifleCollisions)
        baseMask |= (bvmask_Spine3 | bvmask_Spine2);
      if(m_parameters.disableArmCollisions)
        baseMask |= ( bvmask_Spine | bvmask_ThighLeft | bvmask_ShinLeft | bvmask_ThighRight | bvmask_ShinRight );
    }
#endif //NM_POINTGUN_COLLISIONS_OFF
    m_character->m_rightHandCollisionExclusion.setB(baseMask);
  }

  void NmRsCBUPointGun::setArmStatus(ArmData& armData, NmRsCBUPointGun::ArmStatus status)
  {
    if(armData.status != status)
    {
      armData.armStatusChanged = true;
      armData.status = status;
    }
  }

  // Picks clavicle angles based on target direction
  // TODO: Drive twist intelligently. angles are clamped within
  // shoulder limits (with some padding for soft limit action
  // TODO: Pass back shoulder matrix1 after drive to avoid recompute
#if NM_POINTGUN_RECOIL_IK
  void NmRsCBUPointGun::clavicleIK(const NmRsHumanArm* arm, NmRsLimbInput& armInput, const ArmData& armData, rage::Vector3& target, float limitAmount /*= 0.9f*/)
#else
  void NmRsCBUPointGun::clavicleIK(const NmRsHumanArm* arm, NmRsLimbInput& armInput, const ArmData& /*armData*/, rage::Vector3& target, float limitAmount /*= 0.9f*/)
#endif
  {
    NmRsArmInputWrapper *armInputData = armInput.getData<NmRsArmInputWrapper>();
    Assert(armInputData);

    rage::Matrix34 clavicleMatrix1, clavicleMatrix2;
    arm->getClavicle()->getMatrix1(clavicleMatrix1);
    arm->getClavicle()->getMatrix2(clavicleMatrix2);
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    rage::Matrix34 clavMatrix1, clavMatrix2;
    getLeftArm()->getClavicle()->getMatrix1(clavMatrix1);
    getLeftArm()->getClavicle()->getMatrix2(clavMatrix2);

    //m_character->bspyDrawCoordinateFrame(0.4f, clavMatrix1);
    //m_character->bspyDrawCoordinateFrame(0.2f, clavMatrix2);
    if (arm==getLeftArm())
    {
      m_character->bspyDrawLine(clavicleMatrix1.d, target, rage::Vector3(1.0f, 0.0f, 0.0f));
    }
#endif

    rage::Quaternion driveQuat;

    // assume clavicle limits are symmetrical.
    // this is almost true.

    // compute target direction in clavicle mat1 frame.
    rage::Vector3 targetDirection(target - clavicleMatrix1.d);
    targetDirection.Normalize();
    if (arm==getLeftArm() && (m_weaponMode == kPistol || m_weaponMode == kRifle))
    {
      //For support arm when the angle gets 30deg ish from zero to the support side 
      //  then let the clavicle go backwards to give shoulder a chance
      //  really want zerobone direction from target
      //We offset matrix1 by 45deg as it is pointing 15deg ish in front of bone i.e in the wrong direction
      //NB:  this is rig dependent
      //mmmmTODO Test to see if we can extend this to all modes/arms?  Other modes/arms look better when blended with itm pose
      //  so probably not needed? unless unblended looks bad because clavicles too far forward
      clavicleMatrix1.RotateLocalY(0.7f);
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
      m_character->bspyDrawCoordinateFrame(0.4f, clavicleMatrix1);
#endif
    }
    clavicleMatrix1.UnTransform3x3(targetDirection);

    // zeroed twist axis is z.
    rage::Vector3 zeroDirection(0.0f, 0.0f, 1.0f);
    driveQuat.FromVectors(zeroDirection, targetDirection);

#if NM_POINTGUN_RECOIL_IK    // try scaling so that it drives away from the 
    // target when recoil is active.
    //float scale = (armData->relaxTimer/m_parameters.fireWeaponRelaxTime-0.5f)*-2.0f;
	float scale = 1.0f;
	if (m_parameters.fireWeaponRelaxTime > 0.0f)
	{
      scale -= armData.relaxTimer/m_parameters.fireWeaponRelaxTime;
    }
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    bspyScratchpad(m_character->getBSpyID(), "recoil.clav", scale);
#endif
    driveQuat.ScaleAngle(scale);
#endif

    // convert to lean and twist and drive the joint.
    rage::Vector3 twistLean = rsQuatToRageDriveTwistSwing(driveQuat);
    arm->getClavicle()->clampRawLeanTwist(twistLean, limitAmount);
    arm->getClavicle()->getTwistAndSwingFromRawTwistAndSwing(twistLean, twistLean);
    armInputData->getClavicle()->setDesiredTwist(twistLean.x);
    armInputData->getClavicle()->setDesiredLean1(twistLean.y);
    armInputData->getClavicle()->setDesiredLean2(twistLean.z);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    m_character->bspyDrawLine(clavicleMatrix1.d, clavicleMatrix1.d+clavicleMatrix1.c*0.2f, rage::Vector3(1,0,0));
    m_character->bspyDrawLine(clavicleMatrix1.d, clavicleMatrix1.d+clavicleMatrix2.c*0.2f, rage::Vector3(1,1,0));
    m_character->bspyDrawLine(clavicleMatrix1.d, clavicleMatrix1.d+targetDirection*0.2f, rage::Vector3(0,1,0));
#endif
  }

#if USE_CLAVICLEIK2
  // newer clavicle drive method.
  rage::Quaternion NmRsCBUPointGun::clavicleIK2(
    const NmRsHumanArm* arm,
    NmRsLimbInput* armInput,
    rage::Vector3& targetDirection,
    float limitAmount)
  {
    rage::Matrix34 clavicle1;
    arm->getClavicle()->getMatrix1(clavicle1);

    // get rotation between clavicle part +/-z and target direction.
    // get target direction in clavicle clavicle1 space.
    rage::Vector3 targetDirectionLocal;
    clavicle1.UnTransform3x3(targetDirection, targetDirectionLocal);
    rage::Quaternion clavicleDrive;
    rage::phJoint* clavJt = arm->getClavicle()->getJoint();
    rage::Matrix34 clavOrientationChild = clavJt->GetOrientationChild();
    rage::Vector3 clavicleForward(clavOrientationChild.c);
    if(arm == getRightArm())
      clavicleForward.Scale(-1.f);
    clavicleDrive.FromVectors(clavicleForward, targetDirectionLocal);

    // drive clavicle.
    NmRsArmInputWrapper* armInputData = armInput->getData<NmRsArmInputWrapper>();
    clampAndDriveQuat(arm->getClavicle(), armInputData->getClavicle(), clavicleDrive, limitAmount);

    return clavicleDrive;
  }
#endif

  void NmRsCBUPointGun::getDesiredHandTM(rage::Matrix34& handTM)
  {
    rage::Matrix34 matrix1;
    rage::Matrix34 matrix2;
    //get desired clavicle partTM
    getSpine()->getSpine3Part()->getMatrix(handTM);
    getEffectorMatrix1FromParentPartTM(handTM, getRightArm()->getClavicle(), matrix1);
    getEffectorMatrix2FromMatrix1AndDesiredDriveAngles(matrix1, getRightArm()->getClavicle(), matrix2, getRightArmInputData()->getClavicle());
    getPartMatrixFromParentEffectorMatrix2(handTM, getRightArm()->getClavicle(), matrix2);
    //get desired upperArm partTM
    getEffectorMatrix1FromParentPartTM(handTM, getRightArm()->getShoulder(), matrix1);
    getEffectorMatrix2FromMatrix1AndDesiredDriveAngles(matrix1, getRightArm()->getShoulder(), matrix2, getRightArmInputData()->getShoulder());
    getPartMatrixFromParentEffectorMatrix2(handTM, getRightArm()->getShoulder(), matrix2);
    //get desired lowerArm partTM
    getEffectorMatrix1FromParentPartTM(handTM, getRightArm()->getElbow(), matrix1);
    getEffectorMatrix2FromMatrix1AndDesiredDriveAngles(matrix1, getRightArm()->getElbow(), matrix2, getRightArmInputData()->getElbow());
    getPartMatrixFromParentEffectorMatrix2(handTM, getRightArm()->getElbow(), matrix2);
    //get desired hand partTM
    getEffectorMatrix1FromParentPartTM(handTM, getRightArm()->getWrist(), matrix1);
    getEffectorMatrix2FromMatrix1AndDesiredDriveAngles(matrix1, getRightArm()->getWrist(), matrix2, getRightArmInputData()->getWrist());
    getPartMatrixFromParentEffectorMatrix2(handTM, getRightArm()->getWrist(), matrix2);
  }
  // pick a target gun orientation to keep the barrel pointed at the
  // target and the gun oriented roughly upright.
  // Returns desiredGunToWorld
  void NmRsCBUPointGun::getDesiredGunToWorldOrientation(rage::Matrix34 &desiredGunToWorld, const rage::Vector3 &pointingFromPosition, const rage::Vector3 &target)
  {
    // take spine3 tm and flip it over to get hand target orientation
    // when pointing forwards.  trust me, it works.
    rage::Matrix34 spine3TM, gunToWorldTarget;
    getSpine()->getSpine3Part()->getMatrix(spine3TM);
    gunToWorldTarget.a.Set(-spine3TM.c);
    gunToWorldTarget.b.Set(spine3TM.b);
    gunToWorldTarget.c.Set(spine3TM.a);
    gunToWorldTarget.d.Set(target);
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    m_character->bspyDrawCoordinateFrame(0.3f, spine3TM);
    m_character->bspyDrawCoordinateFrame(0.2f, gunToWorldTarget);
#endif 

    // find the rotation from hand -y (gun direction) to target.
    rage::Vector3 toTarget = target - pointingFromPosition;
    rage::Quaternion targetRotation;
    targetRotation.FromVectors(gunToWorldTarget.a, toTarget);
    rage::Matrix34 targetRotationMatrix;
    targetRotationMatrix.FromQuaternion(targetRotation);

    // transform the hand target.
    rage::Vector3 temp; // Transform won't allow in==out.
    targetRotation.Transform(gunToWorldTarget.a, temp);
    gunToWorldTarget.a.Set(temp);
    targetRotation.Transform(gunToWorldTarget.b, temp);
    gunToWorldTarget.b.Set(temp);
    targetRotation.Transform(gunToWorldTarget.c, temp);
    gunToWorldTarget.c.Set(temp);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    m_character->bspyDrawCoordinateFrame(0.1f, gunToWorldTarget);
    m_character->bspyDrawLine(gunToWorldTarget.d, gunToWorldTarget.d-gunToWorldTarget.b, rage::Vector3(0,1,0));
#endif 

    desiredGunToWorld.Set(gunToWorldTarget);
  }

#define POINTGUN_TWOBONEIK_DEBUG (0 && PointGunBSpyDraw)
  /*
  *  drive shoulder and elbow to put wrist at target
  *  position. shoulderMatrix1 should be the computed
  *  after any clavicle drive.
  *
  *  TODO: 
  *    - add twist support
  *    - check base pose
  *    - review advanced ik functionality and add if necessary
  */
  void NmRsCBUPointGun::twoBoneIK(
    const NmRsHumanArm* arm,
    NmRsLimbInput& armInput,
    rage::Vector3& target,
    float twist,
    rage::Matrix34& shoulderMatrix1)
  {

#if ART_ENABLE_BSPY & PointGunBSpyDraw & POINTGUN_TWOBONEIK_DEBUG & 0
    m_character->bspyDrawPoint(target, 0.1f, rage::Vector3(1.0f,1.0f,0.0f));
    m_character->bspyDrawPoint(shoulderMatrix1.d, 0.1f, rage::Vector3(1.0f,1.0f,0.0f));
#endif

    NmRsArmInputWrapper* armInputData = armInput.getData<NmRsArmInputWrapper>();
    Assert(armInputData);

    rage::Matrix34 rootTM1, rootTM2, hingeTM1, hingeTM2, wristTM1;

    // operates in root local frame with starting shoulder
    // configuration computed from incoming transforms to
    // capture base twist.

    // work out the joint matrices from ITMs
    // collect part ITMs
    rage::Matrix34 clavicleITM, upperArmITM, lowerArmITM;
    if(m_parameters.useIncomingTransforms)
    {
      m_character->getITMForPart(arm->getClaviclePart()->getPartIndex(), &clavicleITM);
      m_character->getITMForPart(arm->getUpperArm()->getPartIndex(), &upperArmITM);
      m_character->getITMForPart(arm->getLowerArm()->getPartIndex(), &lowerArmITM);
    }
    else
    {
      //This doesn't work at all well
      arm->getClaviclePart()->getMatrix(clavicleITM);
      arm->getUpperArm()->getMatrix(upperArmITM);
      arm->getLowerArm()->getMatrix(lowerArmITM);
      //so...
      if (m_weaponMode == kRifle && arm == getLeftArm())
      {
        clavicleITM.a.Set(-0.13670942f, 0.61082631f, 0.77987277f);
        clavicleITM.b.Set(0.96765798f, -0.08614505f, 0.23709939f);
        clavicleITM.c.Set(0.21200874f , 0.78706443f, -0.57929444f);
        clavicleITM.d.Set(-5.25499773f, -26.7526035f, 7.57275534f);

        upperArmITM.a.Set(0.76990020f, -0.59864831f, -0.22107352f);
        upperArmITM.b.Set(-0.40090253f, -0.72324258f, 0.56231415f);
        upperArmITM.c.Set(-0.49651798f, -0.34429699f, -0.79682421f);
        upperArmITM.d.Set(-5.25335073f, -26.6333351f, 7.48707819f);

        lowerArmITM.a.Set(0.73016142f, -0.64308607f, -0.23087574f);
        lowerArmITM.b.Set(-0.62302708f, -0.48788982f, -0.61139202f);
        lowerArmITM.c.Set(0.28053587f, 0.59025687f, -0.75689852f);
        lowerArmITM.d.Set(-5.12500715f, -26.4680119f, 7.48787117f);
      }
      if (m_weaponMode == kRifle && arm == getRightArm())
      {
        clavicleITM.a.Set(0.43644362f , 0.10755956f , 0.89327937f);
        clavicleITM.b.Set(-0.56194585f, 0.80795508f, 0.17727272f);
        clavicleITM.c.Set(-0.70266234f , -0.57934385f, 0.41306945f);
        clavicleITM.d.Set(-4.63284636f, -30.7096367f, 7.55434608f);

        upperArmITM.a.Set(0.79875093f, -0.03230418f, 0.60079401f);
        upperArmITM.b.Set(-0.51689022f, -0.54790741f, 0.65774017f);
        upperArmITM.c.Set(0.30793154f, -0.83591491f, -0.45433911f);
        upperArmITM.d.Set(-4.50646782f, -30.6794853f, 7.46436024f);

        lowerArmITM.a.Set(0.74070721f, 0.06237474f, 0.66892576f);
        lowerArmITM.b.Set(0.62422162f, -0.43202915f, -0.65092122f);
        lowerArmITM.c.Set(0.24839453f, 0.89969998f, -0.3589428f);
        lowerArmITM.d.Set(-4.50811625f, -30.557951f  , 7.45797634f);
      }
      //if (m_weaponMode != kRifle && arm = getRightArm())
      //{
      //  clavicleITM.a.Set(xxxx, yyyy, zzz);
      //  clavicleITM.b.Set(xxxx, yyyy, zzz);
      //  clavicleITM.c.Set(xxxx, yyyy, zzz);
      //  clavicleITM.d.Set(xxxx, yyyy, zzz);

      //  upperArmITM.a.Set(xxxx, yyyy, zzz);
      //  upperArmITM.b.Set(xxxx, yyyy, zzz);
      //  upperArmITM.c.Set(xxxx, yyyy, zzz);
      //  upperArmITM.d.Set(xxxx, yyyy, zzz);

      //  lowerArmITM.a.Set(xxxx, yyyy, zzz);
      //  lowerArmITM.b.Set(xxxx, yyyy, zzz);
      //  lowerArmITM.c.Set(xxxx, yyyy, zzz);
      //  lowerArmITM.d.Set(xxxx, yyyy, zzz);
      //}
      //if (m_weaponMode != kRifle && arm = getLeftArm())//NB left hand pointing not done
      //{
      //  clavicleITM.a.Set(xxxx, yyyy, zzz);
      //  clavicleITM.b.Set(xxxx, yyyy, zzz);
      //  clavicleITM.c.Set(xxxx, yyyy, zzz);
      //  clavicleITM.d.Set(xxxx, yyyy, zzz);

      //  upperArmITM.a.Set(xxxx, yyyy, zzz);
      //  upperArmITM.b.Set(xxxx, yyyy, zzz);
      //  upperArmITM.c.Set(xxxx, yyyy, zzz);
      //  upperArmITM.d.Set(xxxx, yyyy, zzz);

      //  lowerArmITM.a.Set(xxxx, yyyy, zzz);
      //  lowerArmITM.b.Set(xxxx, yyyy, zzz);
      //  lowerArmITM.c.Set(xxxx, yyyy, zzz);
      //  lowerArmITM.d.Set(xxxx, yyyy, zzz);
      //}
    }

    // compute joint matrices from part ITMs
    getEffectorMatrix1FromParentPartTM(clavicleITM, arm->getShoulder(), rootTM1);
    getEffectorMatrix2FromChildPartTM(upperArmITM, arm->getShoulder(), rootTM2);
    getEffectorMatrix1FromParentPartTM(upperArmITM, arm->getElbow(), hingeTM1);
    getEffectorMatrix2FromChildPartTM(lowerArmITM, arm->getElbow(), hingeTM2);
    getEffectorMatrix1FromParentPartTM(lowerArmITM, arm->getWrist(), wristTM1);
#if ART_ENABLE_BSPY & PointGunBSpyDraw & POINTGUN_TWOBONEIK_DEBUG
    m_character->bspyDrawCoordinateFrame(0.1f, rootTM1);
    m_character->bspyDrawCoordinateFrame(0.05f, rootTM2);
    m_character->bspyDrawCoordinateFrame(0.1f, hingeTM1);
    m_character->bspyDrawCoordinateFrame(0.05f, hingeTM2);
    m_character->bspyDrawCoordinateFrame(0.1f, wristTM1);
#endif

    // compute shoulder base pose offset for later
    rage::Quaternion rootBasePose = getDriveQuatFromMatrices(rootTM1, rootTM2);

    // get joint matrices local to shoulder matrix1
    rage::Matrix34 rootTM1Inv, hingeTM2Inv;

    rootTM1Inv.Inverse(rootTM1);
    hingeTM1.Dot(rootTM1Inv);
    hingeTM2.Dot(rootTM1Inv);
    wristTM1.Dot(rootTM1Inv);

    hingeTM2Inv.Inverse(hingeTM2);
    wristTM1.Dot(hingeTM2Inv);
    wristTM1.Dot(hingeTM1);

    //Speedup? Note up to this point all variables are actually constants if itm's have not changed (and the character joint model is the same)
    //We only need hingeTM1, wristTM1.d, rootBasePose from now on.

    // copy the positions to fit in with existing ik code
    rage::Vector3 h(hingeTM1.d);
    rage::Vector3 e(wristTM1.d);

    // mapping to shoulder by way of desired matrix 1
    // rather than actual matrix 1
    rage::Vector3 t;
    shoulderMatrix1.UnTransform(target, t);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & POINTGUN_TWOBONEIK_DEBUG
    {
      // draw target and starting limb configuration
      rage::Vector3 a, b;
      a = shoulderMatrix1.d;
      shoulderMatrix1.Transform(t, b);
      m_character->bspyDrawPoint(b, 0.05f, rage::Vector3(1.0f,1.0f,0.0f));
      shoulderMatrix1.Transform(h, b);
      m_character->bspyDrawLine(a, b, rage::Vector3(1.0f,0.0f,0.0f));
      shoulderMatrix1.Transform(e, a);
      m_character->bspyDrawLine(a, b, rage::Vector3(1.0f,0.0f,0.0f));
    }
#endif

    /*
    *  bend hinge to make root to effector and root
    *  to target distances equal.  bones are not 
    *  guaranteed to be ortho to hinge axis (z), so
    *  flat them against z before measuring.
    */
    rage::Vector3 rh = h;
    rage::Vector3 he = e-h;
    float rtMag = t.Mag();
    float reMag = e.Mag();
    float hAngle = 0;

    /*
    *  arms should never be quite straight or elbow math
    *  gets upset
    */
    if(rtMag > reMag)
      rtMag = reMag - 0.001f;

    /*
    *  math to get hinge angle from desired root effector
    *  distance and zero root and effector position in
    *  hinge space.  courtesy of AR (as in: ask him if you
    *  need an explanation of how it works).
    *
    *  assumes hinge axis is hinge tm z
    */
    rage::Vector3 pe, pr;
    hingeTM1.UnTransform3x3((e-h), pe);
    hingeTM1.UnTransform3x3(-h, pr);
    float peMag = pe.Mag();
    float prMag = pr.Mag();
    float a = -2.0f*pe.x*pr.x-2.0f*pe.y*pr.y;
    float b = -2.0f*pe.x*pr.y+2.0f*pe.y*pr.x;
    float c = rtMag*rtMag-peMag*peMag-prMag*prMag+2.0f*pe.z*pr.z;
    hAngle = 2.0f*atanf((-b-rage::Sqrtf(rage::Clamp(a*a+b*b-c*c, 0.0f, 10.0f)))/(-a-c));

    /*
    *  update internal model by rotating hinge to effector
    */
    rage::Quaternion q;
    he = e - h;
    q.FromRotation(hingeTM1.c, hAngle);
    q.Transform(he);
    e = h + he;

#if ART_ENABLE_BSPY & PointGunBSpyDraw & POINTGUN_TWOBONEIK_DEBUG
    {
      rage::Vector3 a, b;
      shoulderMatrix1.Transform(h, a);
      shoulderMatrix1.Transform(e, b);
      m_character->bspyDrawLine(a, b, rage::Vector3(0.9f,0.5f,0.0f));
    }
#endif

    /*
    *  get root rotation to align root to effector with 
    *  root to target.
    */
    rage::Vector3 re = e;
    re.Normalize();
    rage::Vector3 rt = t;
    rt.Normalize();
    rage::Vector3 rAxis;
    rAxis.Cross(rt, re);
    float rAngle = rage::AcosfSafe(re.Dot(rt));
    rAxis.Normalize();
    rage::Quaternion rLeanQuat;
    rLeanQuat.FromRotation(rAxis, rAngle);
    rLeanQuat.UnTransform(h);
    rLeanQuat.UnTransform(e);

    /*
    *  twist around root to effector
    *
    *  use pole vector to inform the twist, if
    *  provided.  otherwise use twist value.
    *
    *  pole vector is interpreted as desired hinge
    *  axis direction (as opposed to desired hinge
    *  position.
    *
    *  using the hinge axis is not strictly correct,
    *  since there is no guarantee that the arm
    *  "bones" will be ortho to the hinge axis,
    *  however, i can't think of a cleverer way
    *  to do this.
    */
    re = e; re.Normalize();
    rage::Quaternion rTwistQuat, rQuat;
    rTwistQuat.FromRotation(re, twist);

    /*
    *  update internal model by rotating hinge and 
    *  effector.
    */
    rTwistQuat.UnTransform(h);
    rTwistQuat.UnTransform(e);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & POINTGUN_TWOBONEIK_DEBUG
    {
      rage::Vector3 a, b;
      shoulderMatrix1.Transform(h, a);
      shoulderMatrix1.Transform(e, b);
      m_character->bspyDrawLine(shoulderMatrix1.d, a, rage::Vector3(1.0f,1.0f,0.0f));
      m_character->bspyDrawLine(a, b, rage::Vector3(1.0f,1.0f,0.0f));
    }
#endif

    /*
    *  combine root rotations
    */
    rQuat.Set(rootBasePose);
    rQuat.Multiply(rootBasePose, rLeanQuat);
    rQuat.Multiply(rTwistQuat);

    /*
    *  set hinge angle.
    */
    armInputData->getElbow()->setDesiredAngle(hAngle);

    /*
    *  set root angles.
    */
    rQuat.Inverse(); // to accommodate new joint function
    rage::Vector3 tss = rsQuatToRageDriveTwistSwing(rQuat);
#if 1
    // trying to cut out instability when on ground
    arm->getShoulder()->clampRawLeanTwist(tss, 1.1f);
#endif
    arm->getShoulder()->getTwistAndSwingFromRawTwistAndSwing(tss, tss);
    armInputData->getShoulder()->setDesiredTwist(tss.x);
    armInputData->getShoulder()->setDesiredLean1(tss.y);
    armInputData->getShoulder()->setDesiredLean2(tss.z);
  }

  void NmRsCBUPointGun::pistolIk(
    ArmData& armData,
    float /* twist */,
    rage::Matrix34& target)
  { 
#if ART_ENABLE_BSPY 
    m_character->m_currentSubBehaviour = "-pistolIK"; 
#endif

    NmRsHumanArm* arm = armData.arm;
    NmRsArmInputWrapper* armInputData = armData.inputData;
    Assert(arm);
    Assert(armInputData);

    // get normalized target direction.
    rage::Vector3 targetDirection(target.d - arm->getShoulder()->getJointPosition());
    rage::Vector3 targetDirectionLocal; // for later.
    targetDirection.Normalize();

    rage::Matrix34 shoulder1, shoulder2, elbow1, elbow2, wrist1;  // joint matrices. consider making a handy struct here.
    rage::Matrix34 clavicle1, clavicle2;
    rage::Matrix34 clavicle, upperArm, lowerArm;                  // part matrices.

    /*
    *  1. point clavicle forward (+/-z) towards the target and clamp.
    */

    // todo limit clavicle lean separately from twist. we want to allow
    // much more twist than lean.

    // assumes zero as starting point. may want to bias to make prettier.
    arm->getClavicle()->getMatrix1(clavicle1);

    rage::Quaternion clavicleDrive;

    // get rotation between clavicle part +/-z and target direction.
    // get target direction in clavicle clavicle1 space.
    clavicle1.UnTransform3x3(targetDirection, targetDirectionLocal);
#define POINT_CLAVICLE_FORWARD 1
#if POINT_CLAVICLE_FORWARD
    rage::phJoint* clavJt = arm->getClavicle()->getJoint();
    rage::Matrix34 clavOrientationChild = clavJt->GetOrientationChild();
    rage::Vector3 clavicleForward(clavOrientationChild.c);
    if(arm->getType() == kRightArm)
      clavicleForward.Scale(-1.f);
#else
    // point shoulder twist axis (shoulder1.z) at target.
    // get clavicle part matrix.
    getPartMatrixFromParentEffectorMatrix2(clavicle, arm->getClavicle(), clavicle2);
    // get shoulder matrix1.z in clavicle1 space.
    getEffectorMatrix1FromParentPartTM(clavicle, arm->getShoulder(), shoulder1);
    rage::Vector3 clavicleForward;
    clavicle1.UnTransform3x3(shoulder1.c, clavicleForward);
#endif
    clavicleDrive.FromVectors(clavicleForward, targetDirectionLocal);

    // drive clavicle.
    clampAndDriveQuat(arm->getClavicle(), armInputData->getClavicle(), clavicleDrive, 0.95f);

    // blend with itm clavicle
    if(m_parameters.clavicleBlend > 0)
    {
      //Get the clavicle angles from the current itms
      rage::Quaternion q;
      if (arm->getClavicle()->getJointQuaternionFromIncomingTransform_uncached(&q))
      {
        rage::Vector3 tss = rsQuatToRageDriveTwistSwing(q);
        arm->getClavicle()->getTwistAndSwingFromRawTwistAndSwing(tss, tss);
        armInputData->getClavicle()->blendToSpecifiedPose(tss, m_parameters.clavicleBlend);
        // clamp again, as zero pose could be past limits
        armInputData->getClavicle()->clamp(arm->getClavicle(), 0.9f);
      }
    }


    /*
    * 2. rotate shoulder in the direction of the target,
    *    stopping at (or just inside) the limits.
    */

    // fk down the arm.
    // todo consider truncating this. we don't need full matrices for
    // most of the joints/parts.

    // get clavicle2 from clamped drive.
    getEffectorMatrix2FromMatrix1AndDesiredDriveAngles(clavicle1, arm->getClavicle(), clavicle2, armInputData->getClavicle());

    // get clavicle part matrix.
    getPartMatrixFromParentEffectorMatrix2(clavicle, arm->getClavicle(), clavicle2);

    // get shoulder matrices.
    // assumes joint drive is zeroed. may want to bias to make prettier.
    getEffectorMatrix1FromParentPartTM(clavicle, arm->getShoulder(), shoulder1);

#define IK_USE_BASE_POSE 0
#if IK_USE_BASE_POSE
    // this will be our starting point for the reach.
    rage::Vector3 shoulderBaseTwistLean(0.0f, 0.0f, 0.5f);
    rage::Quaternion shoulderBaseQuat = rsRageLimitTwistSwingToQuat(shoulderBaseTwistLean);
    rage::Matrix34 shoulderBaseMatrix;
    shoulderBaseMatrix.FromQuaternion(shoulderBaseQuat);
    shoulderBaseMatrix.Identity();
    shoulderBaseMatrix.FromQuaternion(shoulderBaseQuat);
    shoulder2.Dot(shoulderBaseMatrix, shoulder1);
#else
    // starting from zero for now.
    shoulder2.Set(shoulder1); 
#endif

    // get upper arm part matrix.
    // only needed to generate elbow position.
    getPartMatrixFromParentEffectorMatrix2(upperArm, arm->getShoulder(), shoulder2);

    // get elbow matrix1.
    // we only need position.
    getEffectorMatrix1FromParentPartTM(upperArm, arm->getElbow(), elbow1);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & PointGunIKBSpyDraw
#if 0
    m_character->bspyDrawCoordinateFrame(0.05f, clavicle2);
    m_character->bspyDrawCoordinateFrame(0.05f, clavicle);
    m_character->bspyDrawCoordinateFrame(0.05f, shoulder1);
    m_character->bspyDrawCoordinateFrame(0.05f, upperArm);
    m_character->bspyDrawCoordinateFrame(0.05f, elbow1);
#endif
    bspyScratchpad(m_character->getBSpyID(), "pistol_ik", clavicle1.d);
    bspyScratchpad(m_character->getBSpyID(), "pistol_ik", clavicle.d);
    bspyScratchpad(m_character->getBSpyID(), "pistol_ik", shoulder1.d);
    bspyScratchpad(m_character->getBSpyID(), "pistol_ik", upperArm.d);
    bspyScratchpad(m_character->getBSpyID(), "pistol_ik", elbow1.d);
#endif

    // get upper arm direction.
    rage::Vector3 rh(elbow1.d - shoulder1.d);
    rh.Normalize();

    // get drive to point upper arm towards target.
    rage::Quaternion shoulderRot;
    rage::Vector3 rhLocal;
    shoulder1.UnTransform3x3(rh, rhLocal);                            // arm direction into shoulder1 space.
    shoulder1.UnTransform3x3(targetDirection, targetDirectionLocal);  // target direction into shoulder1 space.
    shoulderRot.FromVectors(rhLocal, targetDirectionLocal);           // quat from the difference between vectors.

    // clamp against joint limits.
#if IK_USE_BASE_POSE
    rage::Quaternion shoulderDrive(shoulderBaseQuat);
    shoulderDrive.MultiplyFromLeft(shoulderRot);
#else
    rage::Quaternion shoulderDrive(shoulderRot);
#endif
    Assert(shoulderDrive.x == shoulderDrive.x);

    float shoulderError = armData.arm->getShoulder()->clampRawLeanTwist(shoulderDrive, 0.95f);

    // update upper arm bone direction.
    shoulderDrive.Transform(rhLocal);   // apply drive rotation.

    /*
    * 3.  rotate shoulder around the upper arm axis in such
    *     a way that the elbow axis is perpendicular to
    *     the target direction. be careful of hinge direction.
    */

    // fk down the arm.
    // todo consider truncating this. we don't need full matrices for
    // most of the joints/parts.

    // get shoulder matrix2.
    rage::Matrix34 shoulderRotMat;
    shoulderRotMat.FromQuaternion(shoulderDrive);
    shoulder2.Dot3x3(shoulderRotMat, shoulder1);

    // get upper arm part matrix.
    // only needed to generate elbow position.
    getPartMatrixFromParentEffectorMatrix2(upperArm, arm->getShoulder(), shoulder2);

    // get elbow matrix1.
    // only need x axis (except for debug draw).
    getEffectorMatrix1FromParentPartTM(upperArm, arm->getElbow(), elbow1);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & PointGunIKBSpyDraw
    m_character->bspyDrawCoordinateFrame(0.025f, shoulder2);
    m_character->bspyDrawCoordinateFrame(0.025f, upperArm);
    m_character->bspyDrawCoordinateFrame(0.025f, elbow1);
#endif

    // we only need to bother with the elbow if we were forced to clamp
    // the shoulder drive in step #2. otherwise we can skip this bit.
    if(shoulderError > 0)
    {
      rage::Vector3 hingeNormalLocal(-elbow1.a);
#if ART_ENABLE_BSPY & PointGunBSpyDraw & PointGunIKBSpyDraw
      m_character->bspyDrawLine(elbow1.d, elbow1.d+hingeNormalLocal*0.1f, rage::Vector3(1, 0, 0));
#endif
      // get hinge normal (perpendicular to hinge axis) into shoulder1 space
      shoulder1.UnTransform3x3(hingeNormalLocal);

      // cross upper arm direction with target direction
      rage::Vector3 temp; // better name??
      temp.Cross(rhLocal, targetDirectionLocal);
      temp.Normalize();

      // cross again with upper arm to get perpendicular
      temp.Cross(rhLocal);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & PointGunIKBSpyDraw
      rage::Vector3 clamped(temp);
      shoulder1.Transform3x3(clamped);
      m_character->bspyDrawLine(shoulder1.d, shoulder1.d+clamped*0.1f, rage::Vector3(1, 0, 0));
#endif

      // AcosfSafe is probably unnecessary if switching logic is working.
      // Acosf occasionally producing #IND. keep an eye on this...
      float angle = rage::AcosfSafe(temp.Dot(hingeNormalLocal));

      // flip sign if necessary.
      temp.Cross(hingeNormalLocal);
      float axisDotUpperArm = rhLocal.Dot(temp);
      if(axisDotUpperArm > 0)
        angle *= -1;

      // make quat from rotation.
      rage::Quaternion shoulderTwist;
      shoulderTwist.FromRotation(rhLocal, angle);
      Assert(shoulderTwist.x == shoulderTwist.x);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & PointGunIKBSpyDraw
      {
        bspyScratchpad(m_character->getBSpyID(), "ptGunNewIKTest", axisDotUpperArm);
        bspyScratchpad(m_character->getBSpyID(), "ptGunNewIKTest", angle);
        rage::Vector3 axis(elbow1.c);
        shoulder1.UnTransform3x3(axis);
        shoulderTwist.Transform(axis);
        shoulder1.Transform3x3(axis);
        m_character->bspyDrawLine(elbow1.d, elbow1.d+axis*0.1f, rage::Vector3(0, 0, 1));
      }
#endif

      // at this point, the shoulder drive is stable, barring
      // having to do a second pass based on limit issues.
      shoulderDrive.MultiplyFromLeft(shoulderTwist);
      Assert(shoulderDrive.x == shoulderDrive.x);
    }

    // drive shoulder
    clampAndDriveQuat(armData.arm->getShoulder(), armData.inputData->getShoulder(), shoulderDrive, 0.95f);

    /*
    * 4. bend elbow to point forearm at target. this is a
    *    bit of a fudge. it assumes the forearm bone is
    *    perpendicular to the elbow hinge axis, which isn't
    *    exactly correct. we're close enough to fix up with
    *    the wrist later.
    */

    // fk down the arm.
    // todo consider truncating this. we don't need full matrices for
    // most of the joints/parts.

    // get shoulder matrix2
    shoulderRotMat.FromQuaternion(shoulderDrive);
    shoulder2.Dot3x3(shoulderRotMat, shoulder1);

    // get upper arm part matrix.
    // only needed to generate elbow1.
    getPartMatrixFromParentEffectorMatrix2(upperArm, arm->getShoulder(), shoulder2);

    // get elbow matrices.
    // only need position.
    getEffectorMatrix1FromParentPartTM(upperArm, arm->getElbow(), elbow1);
    elbow2 = elbow1;

    // get lower arm matrix.
    // only needed to generate wrist1.
    getPartMatrixFromParentEffectorMatrix2(lowerArm, arm->getElbow(), elbow2);

    // get wrist matrix1.
    // only need position.
    getEffectorMatrix1FromParentPartTM(lowerArm, arm->getElbow(), wrist1);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & PointGunIKBSpyDraw
    m_character->bspyDrawCoordinateFrame(0.025f, shoulder2);
    m_character->bspyDrawCoordinateFrame(0.025f, upperArm);
    m_character->bspyDrawCoordinateFrame(0.025f, elbow1);
    m_character->bspyDrawCoordinateFrame(0.025f, lowerArm);
    m_character->bspyDrawCoordinateFrame(0.025f, wrist1);
#endif

    // get forearm direction
    // assumes elbow is aligned optimally
    // assumes target is infinitely far away
    rage::Vector3 he(wrist1.d - elbow1.d);
    he.Normalize();
    float angle = rage::AcosfSafe(he.Dot(targetDirection));
    angle = rage::Clamp(angle, arm->getElbow()->getInfo().minAngle, arm->getElbow()->getInfo().maxAngle);
    armInputData->getElbow()->setDesiredAngle(angle);

    /* 
    * rotate wrist such that the gun barrel and forearm
    * are parallel. twist around forearm axis to keep gun
    * upright (or not, gangsta).
    */

    // fk down the arm.
    // todo consider truncating this. we don't need full matrices for
    // most of the joints/parts.

    // get elbow2 from desired angle.
    getEffectorMatrix2FromMatrix1AndDesiredDriveAngles(elbow1, arm->getElbow(), elbow2, armInputData->getElbow());

    // get lower arm matrix.
    getPartMatrixFromParentEffectorMatrix2(lowerArm, arm->getElbow(), elbow2);

    // get wrist matrices.
    getEffectorMatrix1FromParentPartTM(lowerArm, arm->getWrist(), wrist1);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & PointGunIKBSpyDraw
    m_character->bspyDrawCoordinateFrame(0.025f, lowerArm);
    m_character->bspyDrawCoordinateFrame(0.025f, wrist1);
#endif

    // todo take extra leans into account.

    // get target matrix in wrist1 space.
    rage::Matrix34 wrist1Inv, targetLocal, handLocal;
    wrist1Inv.Transpose(wrist1);
    targetLocal.Dot3x3(target, wrist1Inv);

    // get hand local from joint child orientation.
    rage::phJoint* wristJt = arm->getWrist()->getJoint();
    handLocal = wristJt->GetOrientationChild();

    // generate quat from difference.
    rage::Matrix34 wristDriveMat;
    targetLocal.Transpose();
    wristDriveMat.Dot3x3(targetLocal, handLocal);
    wristDriveMat.Transpose();

    // make drive quat.
    rage::Quaternion wristDrive;
    wristDriveMat.ToQuaternion(wristDrive);

    // drive wrist.
    clampAndDriveQuat(arm->getWrist(), armInputData->getWrist(), wristDrive, 0.95f);

#if ART_ENABLE_BSPY 
    m_character->m_currentSubBehaviour = ""; 
#endif
  }

  // classic ik used for rifle primary arm and all
  // supporting arms.
  ////armData used for recoil only clavicleIK/clavicleIK2(armData->relaxTimer), targetDirection in clavicleIK2, unused pdWrist uses armData->relaxScale
  void NmRsCBUPointGun::ik(
    ArmData& armData,
    float twist,
    rage::Matrix34& target,
    float wristBlend)
  {
#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour("-ik"); 
#endif

    NmRsHumanArm* arm = armData.arm;
    NmRsLimbInput* armInput = armData.input;
    NmRsArmInputWrapper *armInputData = armData.inputData;

#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    m_character->bspyDrawCoordinateFrame(0.1f, target);
    rage::Matrix34 handToWorld; arm->getHand()->getMatrix(handToWorld);
    m_character->bspyDrawCoordinateFrame(0.05f, handToWorld);
#endif

    // work out wrist target position from hand target matrix
    rage::Vector3 wristTargetPosition;
    rage::phJoint* jt = arm->getWrist()->getJoint();
    rage::Vector3 positionChild = jt->GetPositionChild();
    rage::Matrix34 orientationChild = jt->GetOrientationChild();
    target.Transform3x3(positionChild, wristTargetPosition);
    wristTargetPosition.Add(target.d);
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    m_character->bspyDrawPoint(wristTargetPosition, 0.2f, rage::Vector3(1,0,1));
#endif

    // pick clavicle angles. blend with incoming animation
    // if desired.
#if !USE_CLAVICLEIK2
    clavicleIK(arm, *armInput, armData, target.d);//only armData->relaxTimer used for recoil only
#else
    clavicleIK2(arm, *armInput, armData.targetDirection, 0.95f);
#endif

    float clavicleBlend = m_parameters.clavicleBlend;
#if 1
    // Keep the clavicleIK result for a supporting arm
    if((m_weaponMode == kRifle || m_weaponMode == kPistol) && arm == getLeftArm())
      clavicleBlend = 0.0f;
#endif
    if(clavicleBlend > 0.0f)
    {
      //Get the clavicle angles from the current itms
      rage::Quaternion q;
      if (arm->getClavicle()->getJointQuaternionFromIncomingTransform_uncached(&q))
      {
        rage::Vector3 tss = rsQuatToRageDriveTwistSwing(q);
        arm->getClavicle()->getTwistAndSwingFromRawTwistAndSwing(tss, tss);
        armInputData->getClavicle()->blendToSpecifiedPose(tss, clavicleBlend);
        // clamp again, as zero pose could be past limits
        armInputData->getClavicle()->clamp(arm->getClavicle(), 0.9f);
      }
    }

    // fk to new shoulder matrix 1
    rage::Matrix34 clavicleMatrix1, shoulderMatrix1;
    arm->getClavicle()->getMatrix1(clavicleMatrix1);
    fk(clavicleMatrix1, shoulderMatrix1, arm->getClavicle(), arm->getShoulder());

#if 1
    // test to see if we can point with a straight arm and
    // shorten weapon distance if necessary.
    // does not apply to rifles because weapon distance is
    // pretty much fixed.
    //Don't do this for a supporting arm or when pointing a rifle
    if (!(m_weaponMode == kRifle || (m_weaponMode == kPistol && arm == getLeftArm())))
    {
      // get target direction in shoulder limit frame (same as frame1)
      rage::Vector3 targetDirection, targetDirectionLocal, desiredTwistLean;
      rage::Quaternion desiredRotation;
      targetDirection.Normalize(wristTargetPosition-shoulderMatrix1.d);
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
      m_character->bspyDrawLine(shoulderMatrix1.d, shoulderMatrix1.d+targetDirection, rage::Vector3(1, 0, 0));
      m_character->bspyDrawLine(shoulderMatrix1.d, shoulderMatrix1.d+shoulderMatrix1.c, rage::Vector3(1, 0, 0));
#endif
      shoulderMatrix1.UnTransform3x3(targetDirection, targetDirectionLocal);
      desiredRotation.FromVectors(rage::Vector3(0,0,1), targetDirectionLocal);
      desiredTwistLean = rsQuatToRageDriveTwistSwing(desiredRotation);
      float error = arm->getShoulder()->clampRawLeanTwist(desiredTwistLean);

      // if error is greater than 0, find the difference between the two.
      // bend the elbow to get this distance (for now, assume elbow ori
      // is in a helpful direction.
      if(error > 0)
      {
        rage::Quaternion differenceRotation, clampedRotation;
        clampedRotation = rsRageDriveTwistSwingToQuat(desiredTwistLean);
        rage::Vector3 temp;
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0      
        clampedRotation.Transform(rage::Vector3(0,0,1), temp);
        shoulderMatrix1.Transform3x3(temp);
        m_character->bspyDrawLine(shoulderMatrix1.d, shoulderMatrix1.d+temp, rage::Vector3(0, 0, 1));
#endif
        differenceRotation.MultiplyInverse(desiredRotation, clampedRotation);
        rage::Vector3 axis;
        float angle;
        differenceRotation.ToRotation(axis, angle);
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
        shoulderMatrix1.Transform3x3(axis, temp);
        m_character->bspyDrawLine(shoulderMatrix1.d, shoulderMatrix1.d+temp, rage::Vector3(1, 1, 0));
#endif

        // ignore axis for now. later we will use it to inform twist.

        // use angle to get new target distance.
        // compute the length of the forearm (assume upper arm is the same length).
        // todo: optimize - move to activation or character init.
        temp = arm->getElbow()->getJointPosition() - arm->getWrist()->getJointPosition();
        float forearmLength = temp.Mag();
        float newTargetDistance = rage::Clamp(forearmLength * 2.0f * rage::Cosf(angle) , 0.15f, forearmLength * 2.0f);
        wristTargetPosition = shoulderMatrix1.d + targetDirection * newTargetDistance;
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
        bspyScratchpad(m_character->getBSpyID(), "ptgun.ik", newTargetDistance);
#endif
      }//if(error > 0)       
    }//if(m_weaponMode != kRifle)     

    // do the two bone IK. provide updated shoulder matrix 1.
    twoBoneIK(arm, *armInput, wristTargetPosition, twist, shoulderMatrix1);

    // fk to updated wrist matrix 1
    rage::Matrix34 elbowMatrix1, wristMatrix1;
    fk(shoulderMatrix1, elbowMatrix1, arm->getShoulder(), arm->getElbow());
    fk(elbowMatrix1, wristMatrix1, arm->getElbow(), arm->getWrist());

#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    m_character->bspyDrawCoordinateFrame(0.2f, wristMatrix1);
#endif

#define POINTGUN_PD_HAND_POSITION_DEBUG (0 & PointGunBSpyDraw)
#if 0
    // pd wrist to target
    if(m_parameters.posStiff > 0 && armData->relaxScale > 0)
    {
      NmRsGenericPart* lowerArm = arm->getLowerArm();
      NmRsEffectorBase* wrist = arm->getWrist();
      rage::Matrix34 lowerArmTM;
      lowerArm->getMatrix(lowerArmTM);
      rage::Vector3 positionParent = wrist->getJoint()->GetPositionParent();
      lowerArmTM.Transform3x3(positionParent);
      rage::Vector3 wristPosition(lowerArmTM.d+positionParent);

#if ART_ENABLE_BSPY & PointGunBSpyDraw & POINTGUN_PD_HAND_POSITION_DEBUG
      m_character->bspyDrawPoint(wristPosition, 0.025f, rage::Vector3(1,1,0));
      m_character->bspyDrawPoint(wristMatrix1.d, 0.025f, rage::Vector3(1,0,0));
#endif

      // compute error
      rage::Vector3 error;
      error = wristPosition - wristMatrix1.d;

      // apply the impulse
      rage::Vector3 impulse(m_parameters.posStiff * m_parameters.posStiff * m_character->getLastKnownUpdateStep() * -error);
      lowerArm->applyImpulse(impulse, wristPosition);

      // compute dError
      rage::Vector3 dError = lowerArm->getLinearVelocity(&wristPosition);

      // apply the impulse
      impulse.Set(-2.0f * m_parameters.posStiff * m_parameters.posDamp * m_character->getLastKnownUpdateStep() * dError);
      lowerArm->applyImpulse(impulse, wristPosition);

    }
#endif

    // compute desired wrist matrix2 from hand transform
    rage::Matrix34 wristMatrix2;
    getEffectorMatrix2FromChildPartTM(target, arm->getWrist(), wristMatrix2);
#if ART_ENABLE_BSPY & PointGunBSpyDraw & 0
    m_character->bspyDrawCoordinateFrame(0.025f, wristMatrix2);
#endif

    if (wristBlend > 0.0f)
    {
      // compute wrist drive angles from desired wrist matrix 1
      // and wrist matrix 2
      wristMatrix1.Normalize();
      wristMatrix2.Normalize();
      rage::Matrix34 driveMatrix;
      driveMatrix.Dot3x3Transpose(wristMatrix1, wristMatrix2);
      rage::Quaternion driveQuat;
      driveQuat.FromMatrix34(driveMatrix);
      driveQuat.Inverse();

      rage::Vector3 twistLean = rsQuatToRageDriveTwistSwing(driveQuat);
      arm->getWrist()->getTwistAndSwingFromRawTwistAndSwing(twistLean, twistLean);

#if ART_ENABLE_BSPY
      float qAngle = driveQuat.GetAngle();
      bspyScratchpad(m_character->getBSpyID(), "ik", qAngle);
#endif
      rage::Vector3 zero(0.0f,0.0f,0.0f);
      twistLean.Lerp(1.0f - wristBlend, zero);

      armInputData->getWrist()->setDesiredTwist(twistLean.x);
      armInputData->getWrist()->setDesiredLean1(twistLean.y);
      armInputData->getWrist()->setDesiredLean2(twistLean.z);
    }
    else
    {
      armInputData->getWrist()->setDesiredTwist(0.0f);
      armInputData->getWrist()->setDesiredLean1(0.0f);
      armInputData->getWrist()->setDesiredLean2(0.0f);
    }


#if ART_ENABLE_BSPY 
    m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]); 
#endif

  }
#endif

  bool NmRsCBUPointGun::armEnabled(const NmRsHumanArm* arm)
  {
    if(arm->getType() == kLeftArm)
      return m_parameters.enableLeft;
    if(arm->getType() == kRightArm)
      return m_parameters.enableRight;
    else
      Assert(false);
    return false;
  }

  void NmRsCBUPointGun::armStatusFeedback()
  {
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      feedback->m_agentID = m_character->getID();

      feedback->m_argsCount = 5;
      ART::ARTFeedbackInterface::FeedbackUserdata data;
      data.setInt(pgfbArmStatus);
      feedback->m_args[0] = data;

      data.setInt(m_armData[NmRsCharacter::kLeftHand].status);
      feedback->m_args[1] = data;

      data.setInt(m_armData[NmRsCharacter::kRightHand].status);
      feedback->m_args[2] = data;

      data.setFloat(m_armData[NmRsCharacter::kLeftHand].error);
      feedback->m_args[3] = data;

      data.setFloat(m_armData[NmRsCharacter::kRightHand].error);
      feedback->m_args[4] = data;

      strcpy(feedback->m_behaviourName, NMPointGunFeedbackName);
      feedback->onBehaviourEvent();
    }
  }

  void NmRsCBUPointGun::animationFeedback()
  {
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      feedback->m_agentID = m_character->getID();

      feedback->m_argsCount = 7;
      ART::ARTFeedbackInterface::FeedbackUserdata data;
      data.setInt(pgfbAnimation);
      feedback->m_args[0] = data;

      data.setInt(m_armData[NmRsCharacter::kLeftHand].status);
      feedback->m_args[1] = data;
      data.setInt(m_armData[NmRsCharacter::kRightHand].status);
      feedback->m_args[2] = data;

      data.setInt(m_armData[NmRsCharacter::kLeftHand].neutralPoseType);
      feedback->m_args[3] = data;
      data.setInt(m_armData[NmRsCharacter::kRightHand].neutralPoseType);
      feedback->m_args[4] = data;

      data.setFloat(m_armData[NmRsCharacter::kLeftHand].error);
      feedback->m_args[5] = data;
      data.setFloat(m_armData[NmRsCharacter::kRightHand].error);
      feedback->m_args[6] = data;
      if (m_weaponMode == kPistol || m_weaponMode == kRifle)
      {
        feedback->m_argsCount = 12;
        float brokenTimeRemaining = m_supportConstraintInhibitTimer;
        float currentGunSupportDistanceFromHand = m_armData[NmRsCharacter::kLeftHand].currentGunSupportDistanceFromHand;
        bool broken = m_supportConstraintInhibitTimer > 0.0f;// or armDataL->desiredStatus == kNeutralBroken;
        //if true and kRifle look at neutralPoseType of each arm Choose a non connected animation 
        //else if true and kPistol - the pistol can still be pointing Choose a non connected animation maybe pointing animation
        //else
        bool connected = supportConstraintEffective(m_armData[NmRsCharacter::kLeftHand]);
        bool connecting = (m_armData[NmRsCharacter::kLeftHand].status == kSupporting && !connected);//supportArmData.currentGunSupportDistanceFromHand //mmmmto havevelocity of this?
        //if connected || connecting (if currentGunSupportDistanceFromHand > threshold maybe choose unnconnected anim start but go to connected)
        // if rightArm kPointing or kPointing Error -> Choose a connected pointing animation 
        // else look at rightArm neutralPoseType and choose appropriate neutral connected animation
        //else
        // if rightArm kPointing or kPointing Error choose right arm pointing animation, left closest to left arm neutralPoseType - (kUnused maps to npsNotRequested)
        // else rightArm/leftArm closest to right neutralPoseType/left neutralPoseType
        data.setBool(broken);
        feedback->m_args[7] = data;
        data.setBool(connected);
        feedback->m_args[8] = data;
        data.setBool(connecting);
        feedback->m_args[9] = data;
        data.setFloat(brokenTimeRemaining);
        feedback->m_args[10] = data;
        data.setFloat(currentGunSupportDistanceFromHand);
        feedback->m_args[11] = data;

      }
      strcpy(feedback->m_behaviourName, NMPointGunFeedbackName);
      feedback->onBehaviourEvent();
    }
  }

  void NmRsCBUPointGun::handAnimationFeedback()
  {
    HandAnimationType handAnimType = haLoose;//loose
    if (supportConstraintEffective(m_armData[NmRsCharacter::kLeftHand]))
      handAnimType = haHoldingWeapon;//holdingWeapoon
    if (m_handAnimationType != handAnimType)//handAnimationType has changed
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();

        feedback->m_argsCount = 2;
        ART::ARTFeedbackInterface::FeedbackUserdata data;
        data.setInt(NmRsCharacter::kLeftHand);
        feedback->m_args[0] = data;
        data.setInt(handAnimType);
        feedback->m_args[1] = data;

        strcpy(feedback->m_behaviourName, NMHandAnimationFeedbackName);
        feedback->onBehaviourEvent();
      }
      m_handAnimationType = handAnimType;
    }

  }
  //Given the orientation between matrix1 and matrix2 of the joint expressed as a quaternion q
  //  1)Clamp the lean and twist angles to amount*jointLimit
  //  2)Set the desired joint lean and twist angles to these clamped values
  float NmRsCBUPointGun::clampAndDriveQuat(const NmRs3DofEffector* eff, NmRs3DofEffectorInputWrapper* effData, rage::Quaternion& q, float amount)
  {
    Assert(eff);
    Assert(effData);

    float result = eff->clampRawLeanTwist(q, amount);
    // drive the effector based on the computed quat.
    rage::Vector3 tss = rsQuatToRageDriveTwistSwing(q);
    eff->getTwistAndSwingFromRawTwistAndSwing(tss, tss);

    effData->setDesiredTwist(tss.x);
    effData->setDesiredLean1(tss.y);
    effData->setDesiredLean2(tss.z);

    return result;
  }

#if ART_ENABLE_BSPY
  void NmRsCBUPointGun::sendParameters(NmRsSpy& spy)
  {

    static const char* bspy_PointGunArmStatusStrings[] = 
    {
#define PG_NAME_ACTION(_name) #_name ,
      PG_STATES(PG_NAME_ACTION)
#undef PG_NAME_ACTION
    };

    static const char* bspy_npsStrings[] =
    {
#define PGNP_NAME_ACTION(_name) #_name ,
      PGNP_STATES(PGNP_NAME_ACTION)
#undef PGNP_NAME_ACTION
    };

    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.enableRight, true);
    bspyTaskVar(m_parameters.enableLeft, true);
    bspyTaskVar(m_enableLeft, true);
    bspyTaskVar(m_parameters.leftHandTarget, true);
    bspyTaskVar(m_parameters.leftHandTargetIndex, true);
    bspyTaskVar(m_parameters.rightHandTarget, true);
    bspyTaskVar(m_parameters.rightHandTargetIndex, true);
    bspyTaskVar(m_parameters.leadTarget, true);
    bool usingLeadTarget = (m_parameters.leadTarget > 0.0f && !(m_weaponMode == kPistol || m_weaponMode == kRifle));
    bspyTaskVar(usingLeadTarget, true);

    bspyTaskVar(m_parameters.armStiffness, true);
    bspyTaskVar(m_parameters.armStiffnessDetSupport, true);
    bspyTaskVar(m_parameters.armDamping, true);
    bspyTaskVar(m_parameters.gravityOpposition, true);
    bspyTaskVar(m_parameters.gravOppDetachedSupport, true);
    bspyTaskVar(m_parameters.massMultDetachedSupport, true);
    bspyTaskVar(m_parameters.allowShotLooseness, true);

    bspyTaskVar(m_parameters.clavicleBlend, true);
    bspyTaskVar(m_parameters.elbowAttitude, true);

    bspyTaskVar(m_parameters.supportConstraint, true);
    bspyTaskVar(m_parameters.constraintMinDistance, true);
    bspyTaskVar(m_parameters.makeConstraintDistance, true);
    bspyTaskVar(m_parameters.reduceConstraintLengthVel, true);	
    bspyTaskVar(m_parameters.breakingStrength, true);
    bspyTaskVar(m_parameters.constraintStrength, true);
    bspyTaskVar(m_parameters.constraintThresh, true);
    bspyTaskVar(m_parameters.brokenSupportTime, true);
    bspyTaskVar(m_parameters.brokenToSideProb, true);
    bspyTaskVar(m_parameters.connectAfter, true);
    bspyTaskVar(m_parameters.connectFor, true);

    bspyTaskVar(m_parameters.oneHandedPointing, true);
    bspyTaskVar(m_parameters.alwaysSupport, true);
    bspyTaskVar(m_parameters.poseUnusedGunArm, true);
    bspyTaskVar(m_parameters.poseUnusedSupportArm, true);
    bspyTaskVar(m_parameters.poseUnusedOtherArm, true);
    bspyTaskVar(poseUnusedArm(m_armData[NmRsCharacter::kLeftHand]), true);
    bspyTaskVar(poseUnusedArm(m_armData[NmRsCharacter::kRightHand]), true);
    bspyTaskVar(m_parameters.maxAngleAcross, true);
    bspyTaskVar(m_parameters.maxAngleAway, true);
    bspyTaskVar(m_parameters.fallingLimits, true);
    bspyTaskVar(m_parameters.acrossLimit, true);
    bspyTaskVar(m_parameters.awayLimit, true);
    bspyTaskVar(m_parameters.upLimit, true);
    bspyTaskVar(m_parameters.downLimit, true);

    bspyTaskVar(m_parameters.pistolNeutralType, true);
    bspyTaskVar(m_parameters.rifleFall, true);
    bspyTaskVar(m_parameters.fallingSupport, true);
    bspyTaskVar(m_parameters.fallingTypeSupport, true);
    
    bspyTaskVar(m_parameters.neutralPoint4Pistols, true);
    bspyTaskVar(m_parameters.neutralPoint4Rifle, true);
    bspyTaskVar(m_parameters.checkNeutralPoint, true);
    bspyTaskVar(m_parameters.point2Side, true);
    bspyTaskVar(m_parameters.add2WeaponDistSide, true);
    bspyTaskVar(m_parameters.point2Connect, true);
    bspyTaskVar(m_parameters.add2WeaponDistConnect, true);

    bspyTaskVar(m_parameters.usePistolIK, true);
    bspyTaskVar(m_parameters.useSpineTwist, true);
    bspyTaskVar(m_parameters.useTurnToTarget, true);
    bspyTaskVar(m_parameters.useHeadLook, true);

    bspyTaskVar(m_parameters.errorThreshold, true);
    bspyTaskVar(m_parameters.fireWeaponRelaxTime, true);
    bspyTaskVar(m_parameters.fireWeaponRelaxAmount, true);
    bspyTaskVar(m_parameters.fireWeaponRelaxDistance, true);

    //gun and gun to character info
    bspyTaskVar(m_parameters.useIncomingTransforms, true);
    bspyTaskVar(m_parameters.measureParentOffset, true);
    bspyTaskVar(m_parameters.leftHandParentOffset, true);
    bspyTaskVar(m_parameters.leftHandParentEffector, true);
    bspyTaskVar(m_parameters.rightHandParentOffset, true);
    bspyTaskVar(m_parameters.rightHandParentEffector, true);
    bspyTaskVar(m_parameters.primaryHandWeaponDistance, true);
    bspyTaskVar(m_parameters.weaponMask, true);

    bspyTaskVar(m_parameters.constrainRifle, true);
    bspyTaskVar(m_parameters.rifleConstraintMinDistance, true);

    bspyTaskVar(m_parameters.timeWarpActive, true);
    bspyTaskVar(m_parameters.timeWarpStrengthScale, true);

    bspyTaskVar(m_parameters.oriStiff, true);
    bspyTaskVar(m_parameters.oriDamp, true);
    bspyTaskVar(m_parameters.posStiff, true);
    bspyTaskVar(m_parameters.posDamp, true);

    bspyTaskVar(m_parameters.disableArmCollisions, true);
    bspyTaskVar(m_parameters.disableRifleCollisions, true);

    bspyTaskVar(m_measuredRightHandParentOffset, true);
    bspyTaskVar(m_measuredLeftHandParentOffset, true);


    bspyTaskVar(m_character->m_gunToHandAiming[NmRsCharacter::kLeftHand].a, true);
    bspyTaskVar(m_character->m_gunToHandAiming[NmRsCharacter::kLeftHand].b, true);
    bspyTaskVar(m_character->m_gunToHandAiming[NmRsCharacter::kLeftHand].c, true);
    bspyTaskVar(m_character->m_gunToHandAiming[NmRsCharacter::kLeftHand].d, true);
    bspyTaskVar(m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].a, true);
    bspyTaskVar(m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].b, true);
    bspyTaskVar(m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].c, true);
    bspyTaskVar(m_character->m_gunToHandAiming[NmRsCharacter::kRightHand].d, true);

    //to help with pulling out the aiming pose gunToHand for testing
    bspyTaskVar(m_character->m_gunToHandCurrent[NmRsCharacter::kLeftHand].a, true);
    bspyTaskVar(m_character->m_gunToHandCurrent[NmRsCharacter::kLeftHand].b, true);
    bspyTaskVar(m_character->m_gunToHandCurrent[NmRsCharacter::kLeftHand].c, true);
    bspyTaskVar(m_character->m_gunToHandCurrent[NmRsCharacter::kLeftHand].d, true);
    bspyTaskVar(m_character->m_gunToHandCurrent[NmRsCharacter::kRightHand].a, true);
    bspyTaskVar(m_character->m_gunToHandCurrent[NmRsCharacter::kRightHand].b, true);
    bspyTaskVar(m_character->m_gunToHandCurrent[NmRsCharacter::kRightHand].c, true);
    bspyTaskVar(m_character->m_gunToHandCurrent[NmRsCharacter::kRightHand].d, true);

    rage::Matrix34 worldToGunHand, supportHandToWorld;
    getRightArm()->getHand()->getMatrix(worldToGunHand);
    getLeftArm()->getHand()->getMatrix(supportHandToWorld);
    worldToGunHand.Inverse();
    rage::Matrix34 supportHandToGunHand;
    supportHandToGunHand.Dot(supportHandToWorld, worldToGunHand);//supportHandToWorld.worldToGunHand
    bspyTaskVar(supportHandToGunHand.a, true);
    bspyTaskVar(supportHandToGunHand.b, true);
    bspyTaskVar(supportHandToGunHand.c, true);
    bspyTaskVar(supportHandToGunHand.d, true);
    bspyTaskVar(m_supportHandToGun.a, true);
    bspyTaskVar(m_supportHandToGun.b, true);
    bspyTaskVar(m_supportHandToGun.c, true);
    bspyTaskVar(m_supportHandToGun.d, true);

    static const char* wm_state_names[] = 
    {
#define WM_NAME_ACTION(_name) #_name ,
      WM_STATES(WM_NAME_ACTION)
#undef WM_NAME_ACTION
    };
    bspyTaskVar_StringEnum(m_weaponMode, wm_state_names, true);       

    static const char* ha_state_names[] = 
    {
#define HA_NAME_ACTION(_name) #_name ,
      HA_STATES(HA_NAME_ACTION)
#undef HA_NAME_ACTION
    };
    bspyTaskVar_StringEnum(m_handAnimationType, ha_state_names, true);       

    ArmData* armDataR = &m_armData[NmRsCharacter::kRightHand];
    bspyTaskVar(armDataR->desiredHandToWorld.d, false);
    bspyTaskVar(armDataR->desiredSupportHandToWorld.d, false);
    //gunToHand later
    //bspyTaskVar(armDataR->currentGunSupportInWorld.d, false);
    //bspyTaskVar(armDataR->desiredGunSupportInWorld.d, false);
    bspyTaskVar(armDataR->pointingFromPosition, false);
    bspyTaskVar(armDataR->target, false);
    bspyTaskVar(armDataR->currentTarget, false);
    bspyTaskVar(armDataR->lastTarget, false);
    bspyTaskVar(armDataR->leadTarget, false);
    bspyTaskVar(armDataR->targetDirection, false);
    bspyTaskVar(armDataR->targetVel, false);
    bspyTaskVar(armDataR->lastHandTargetPos, false);
    bspyTaskVar(armDataR->handTargetVel, false);

    bspyTaskVar(armDataR->relaxTimer, false);
    bspyTaskVar(armDataR->relaxScale, false);
    bspyTaskVar(armDataR->weaponDistance, false);
    bspyTaskVar(armDataR->palmScaleDir, false);
    bspyTaskVar(armDataR->error, false);
    bspyTaskVar(armDataR->fireInhibitTimer, false);
    //bspyTaskVar(armDataR->currentGunSupportDistanceFromShoulder, false);
    //bspyTaskVar(armDataR->desiredGunSupportDistanceFromShoulder, false);
    //bspyTaskVar(armDataR->desiredTargetGunSupportDistanceFromShoulder, false);
    //bspyTaskVar(armDataR->currentGunSupportDistanceFromHand, false);

    bspyTaskVar(armDataR->currentGunSupportable, false);
    bspyTaskVar(armDataR->desiredGunSupportable, false);
    bspyTaskVar(armDataR->desiredTargetGunSupportable, false);
    bspyTaskVar(armDataR->currentGunSupportBehindBack, false);
    bspyTaskVar(armDataR->supportHandBehindBack, false);
    bspyTaskVar(armDataR->supportBehindBack, false);
    bspyTaskVar(armDataR->desiredGunSupportBehindBack, false);

    bspyTaskVar_StringEnum(armDataR->status, bspy_PointGunArmStatusStrings, false); 
    bspyTaskVar_StringEnum(armDataR->desiredStatus, bspy_PointGunArmStatusStrings, false); 
    bspyTaskVar_StringEnum(armDataR->neutralPoseType, bspy_npsStrings, false); 
    bspyTaskVar(armDataR->armStatusChanged, false);//not output correctly.  Is reset before output

    bspyTaskVar(m_forceNeutral, false);

    ArmData* armDataL = &m_armData[NmRsCharacter::kLeftHand];
    bspyTaskVar_StringEnum(armDataL->status, bspy_PointGunArmStatusStrings, false); 
    bspyTaskVar_StringEnum(armDataL->desiredStatus, bspy_PointGunArmStatusStrings, false); 
    bspyTaskVar_StringEnum(armDataL->neutralPoseType, bspy_npsStrings, false); 
    bspyTaskVar(armDataL->armStatusChanged, false);//not output correctly.  Is reset before output

    bspyTaskVar(armDataL->desiredHandToWorld.d, false);
    bspyTaskVar(armDataL->desiredSupportHandToWorld.d, false);
    //gunToHand later
    bspyTaskVar(armDataL->currentGunSupportInWorld.d, false);
    bspyTaskVar(armDataL->desiredGunSupportInWorld.d, false);
    bspyTaskVar(armDataL->pointingFromPosition, false);
    bspyTaskVar(armDataL->target, false);
    bspyTaskVar(armDataL->currentTarget, false);
    bspyTaskVar(armDataL->lastTarget, false);
    bspyTaskVar(armDataL->leadTarget, false);
    bspyTaskVar(armDataL->targetDirection, false);
    bspyTaskVar(armDataL->targetVel, false);
    bspyTaskVar(armDataL->lastHandTargetPos, false);
    bspyTaskVar(armDataL->handTargetVel, false);

    bspyTaskVar(armDataL->relaxTimer, false);
    bspyTaskVar(armDataL->relaxScale, false);
    bspyTaskVar(armDataL->weaponDistance, false);
    bspyTaskVar(armDataL->palmScaleDir, false);
    bspyTaskVar(armDataL->error, false);
    bspyTaskVar(armDataL->fireInhibitTimer, false);
    bspyTaskVar(armDataL->currentGunSupportDistanceFromShoulder, false);
    bspyTaskVar(armDataL->desiredGunSupportDistanceFromShoulder, false);
    bspyTaskVar(armDataL->desiredTargetGunSupportDistanceFromShoulder, false);
    bspyTaskVar(armDataL->currentGunSupportDistanceFromHand, false);

    bspyTaskVar(armDataL->currentGunSupportable, false);
    bspyTaskVar(armDataL->desiredGunSupportable, false);
    bspyTaskVar(armDataL->desiredTargetGunSupportable, false);
    bspyTaskVar(armDataL->currentGunSupportBehindBack, false);
    bspyTaskVar(armDataL->supportHandBehindBack, false);
    bspyTaskVar(armDataL->supportBehindBack, false);
    bspyTaskVar(armDataL->desiredGunSupportBehindBack, false);

    //mmmmtoto check attached object output - add left output here? 
    NmRsCharacter::AttachedObject* leftWeapon = NULL;
    NmRsCharacter::AttachedObject* rightWeapon = NULL;
    m_character->getAttachedWeapons(&leftWeapon, &rightWeapon);
    bool rightWeaponValid = false;
    if(rightWeapon)
    {
      bspyTaskVar(rightWeapon->levelIndex, false);
      bspyTaskVar(rightWeapon->mass, false);
      bspyTaskVar(rightWeapon->worldCOMPos, false);
      rightWeaponValid = true;
    }

    //module variables/functions
    bspyTaskVar(rightWeaponValid, false);

    //rage::Matrix34 m_supportHandToGunHand; // store offset computed from ITMs
    //rage::Matrix34 m_supportHandToGun;
    bspyTaskVar(m_maxArmReach, false);
    bspyTaskVar(m_rifleConstraintDistance, false);
    bspyTaskVar(m_timeWarpScale, false);

    bspyTaskVar(m_neutralSidePoseTimer, false);
    bspyTaskVar(m_supportConstraintInhibitTimer, false);
    bspyTaskVar(m_add2WeaponDist, false);

    bspyTaskVar(m_probeHitSomeThing, false);
    bspyTaskVar(m_forceConstraintMade, false);
    bspyTaskVar(m_decideFallingSupport, false);
    bspyTaskVar(m_allowFallingSupportArm, false);    
    bspyTaskVar(m_rifleFalling, false);
    bspyTaskVar_StringEnum(m_rifleFallingPose, bspy_npsStrings, false); 

    bspyTaskVar(supportConstraintActive(*armDataL, 0.15f), false);
    bspyTaskVar(supportConstraintEffective(m_armData[NmRsCharacter::kLeftHand]), false);
    bspyTaskVar(m_character->isSupportHandConstraintActive(), false);
    bspyTaskVar(isFiring(), false);

    if (m_weaponMode == kPistol || m_weaponMode == kRifle)
    {
      float brokenTimeRemaining = m_supportConstraintInhibitTimer;
      float currentGunSupportDistanceFromHand = m_armData[NmRsCharacter::kLeftHand].currentGunSupportDistanceFromHand;
      bool broken = m_supportConstraintInhibitTimer > 0.0f;// or armDataL->desiredStatus == kNeutralBroken;
      //if true and kRifle look at neutralPoseType of each arm Choose a non connected animation 
      //else if true and kPistol - the pistol can still be pointing Choose a non connected animation maybe pointing animation
      //else
      bool connected = supportConstraintEffective(m_armData[NmRsCharacter::kLeftHand]);
      bool connecting = (armDataL->status == kSupporting && !connected);//supportArmData->currentGunSupportDistanceFromHand //mmmmto havevelocity of this?
      //if connected || connecting (if currentGunSupportDistanceFromHand > threshold maybe choose unnconnected anim start but go to connected)
      // if rightArm kPointing or kPointing Error -> Choose a connected pointing animation 
      // else look at rightArm neutralPoseType and choose appropriate neutral connected animation
      //else
      // if rightArm kPointing or kPointing Error choose right arm pointing animation, left closest to left arm neutralPoseType - (kUnused maps to npsNotRequested)
      // else rightArm/leftArm closest to right neutralPoseType/left neutralPoseType
      bspyTaskVar(broken, false);
      bspyTaskVar(brokenTimeRemaining, false);
      bspyTaskVar(connected, false);
      bspyTaskVar(connecting, false);//output currentGunSupportDistanceFromHand
      bspyTaskVar(currentGunSupportDistanceFromHand, false);
    }
    //else
    // for each arm:
    //   if kPointing or kPointing Error choose arm pointing animation 
    //   else rightArm/leftArm closest neutralPoseType - (kUnused maps to npsNotRequested)
    //armData->error will give an estimate for NeutralPointing aswell - but not neutralPoses.

    bspyTaskVar(m_gunArmNotPointingTime, false);
    bspyTaskVar(m_reConnectTimer, false);

    bool badInput_disabledRight2Handed = m_parameters.enableLeft && !m_parameters.enableRight && (m_weaponMode == kPistol || m_weaponMode == kRifle);
    bspyTaskVar(badInput_disabledRight2Handed, false);
    bool badInput_primaryHandWeaponDistance = m_parameters.primaryHandWeaponDistance < 0.0f && m_parameters.useIncomingTransforms == false;
    bspyTaskVar(badInput_primaryHandWeaponDistance, false);

  }
#endif // ART_ENABLE_BSPY

}
