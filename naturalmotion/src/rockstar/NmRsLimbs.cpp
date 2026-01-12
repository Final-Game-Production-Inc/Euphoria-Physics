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
#include "NmRsLimbs.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "NmRsShadows.h"
#include "NmRsIk.h"
#include "NmRsUtils.h"

#if ART_ENABLE_BSPY_LIMBS
#include "bSpy\bSpyCommonPackets.h"
#include "NmRsCBU_Shared.h"
#include "NmRsCBU_TaskManager.h"
#endif // ART_ENABLE_BSPY_LIMBS

#undef NM_RS_PARAMETER

#undef NM_RS_RO_PARAMETER
#define NM_RS_RO_PARAMETER(classname, type, name)

#undef NM_RS_RO_PARAMETER_ACTUAL
#define NM_RS_RO_PARAMETER_ACTUAL(_prefix, _type, _name)

#undef NM_RS_PARAMETER_DIRECT
#define NM_RS_PARAMETER_DIRECT(_prefix, _type, _name, _min, _max, _default)

namespace ART
{

  // Some global functions to support setting all parameters on a given effector
  // from a blendable input.
  //---------------------------------------------------------------------------
#if ART_ENABLE_BSPY_LIMBS
#define DEBUG_EFFECTOR_SETBY(input, parameter), input->m_##parameter##SetBy
	
  // string map of human limbs
  static const char* humanLimbTypeStrings[] = 
  {
    "Left Arm",
    "Right Arm",
    "Left Leg",
    "Right Leg",
    "Spine",
  };

  //
  // String map of input types
  static const char* limbInputTypeStrings[] = 
  {
    "IK",
    "PoseArm",
    "PoseLeg",
    "PoseSpine",
    "SetStiffness",
    "StopAllBehaviours"
  };
#else
#define DEBUG_EFFECTOR_SETBY(input, parameter)
#endif //ART_ENABLE_BSPY_LIMBS

#define SET_PARAMETER(_prefix, _name){\
  if(input->m_data.m_flags & _prefix##InputData::apply##_name)\
  {\
    effector->set##_name(input->get##_name() DEBUG_EFFECTOR_SETBY(input, _name));\
  }\
  else if(input->m_##_name##Weight > 0)\
  {\
    float weight = input->m_##_name##Weight;\
    effector->set##_name(weight * input->get##_name() +  (1.0f - weight) * effector->get##_name() DEBUG_EFFECTOR_SETBY(input, _name));\
  }\
}

  void set3DofParameters(NmRs3DofEffector* effector, NmRs3DofEffectorInputBlendable* input)
  {
    Assert(effector);

#if 0
    // if we have a completed blend, just write the value to the effector
    if(input->m_data.m_flags & NmRs3DofEffectorInputData::applyDesiredLean1)
    {
      effector->setDesiredLean1(input->getDesiredLean1() DEBUG_EFFECTOR_SETBY(input, DesiredLean1));
    }
    // if we have a positive weight, blend with current values
    else if(input->m_DesiredLean1Weight > 0)
    {
      float weight = input->m_DesiredLean1Weight;
      effector->setDesiredLean1(weight * input->getDesiredLean1() +  (1.0f - weight) * effector->getDesiredLean1() DEBUG_EFFECTOR_SETBY(input, DesiredLean1));
    }
    // otherwise do nothing at all because the current values are just fine.
#else
    // Auto-generate blend from inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) SET_PARAMETER(_prefix, _name)
    #include "common\NmRs3DofEffector.inl"
    #undef NM_RS_PARAMETER

    Assert(effector->getDesiredLean1() == effector->getDesiredLean1());
    Assert(effector->getDesiredLean2() == effector->getDesiredLean2());
    Assert(effector->getDesiredTwist() == effector->getDesiredTwist());
#endif
  }

  void set1DofParameters(NmRs1DofEffector* effector, NmRs1DofEffectorInputBlendable* input)
  {
    Assert(effector);
    Assert(input);

#if 0
    // Blend each parameter with the current effector values based on weight.  E.g.:
    //
    // if we have a completed blend, just write the value to the effector
    if(input->m_data.m_flags & NmRs1DofEffectorInputData::applyDesiredAngle)
    {
      effector->setDesiredAngle(input->getDesiredAngle() DEBUG_EFFECTOR_SETBY(input, DesiredAngle));
    }
    // if we have a positive weight, blend with current values
    else if(input->m_DesiredAngleWeight > 0)
    {
      float weight = input->m_DesiredAngleWeight;
      effector->setDesiredAngle(weight * input->getDesiredAngle() +  (1.0f - weight) * effector->getDesiredAngle() DEBUG_EFFECTOR_SETBY(input, DesiredAngle));
    }
    // otherwise do nothing at all because the current values are just fine.
    //
#else
    // Auto-generate blend from inline file.
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) SET_PARAMETER(_prefix, _name)
    #include "common\NmRs1DofEffector.inl"
    #undef NM_RS_PARAMETER
#endif

    Assert(effector->getDesiredAngle() == effector->getDesiredAngle());
  }


  // NmRsLimb
  //---------------------------------------------------------------------------

#define SET_CURRENT_MASKED(_name)\
  if(mask == bvmask_Full || m_character->isEffectorInMask(mask, get##_name()->getJointIndex()))\
  data->get##_name()->setToCurrent((NmRsEffectorBase*)get##_name());

#define BODYSTIFFNESS_MASKED(_name)\
  if(mask == bvmask_Full || m_character->isEffectorInMask(mask, get##_name()->getJointIndex()))\
  data->get##_name()->setStiffness(stiffness, damping, muscleStiffness);

#define BODYSTIFFNESSSCALING_MASKED(_name)\
  if(mask == bvmask_Full || m_character->isEffectorInMask(mask, m_##_name->getJointIndex()))\
  {\
    m_##_name->setMuscleStrengthScaling(strengthScale);\
    m_##_name->setMuscleDampingScaling(dampScale);\
    m_##_name->setMuscleStiffnessScaling(muscleStiffnessScale);\
  }

#define SETRELAXATION_MASKED(_name)\
  if(mask == bvmask_Full || m_character->isEffectorInMask(mask, get##_name()->getJointIndex()))\
  data->get##_name()->setRelaxation((NmRsEffectorBase*)get##_name(), mult, pMultDamping);

#define BLENDTOZERO_MASKED(_name)\
  if(mask == bvmask_Full || m_character->isEffectorInMask(mask, get##_name()->getJointIndex()))\
  data->get##_name()->blendToZeroPose((NmRsEffectorBase*)get##_name(), t);

#define RESET_EFFECTORS_MASKED(_name)\
  if(mask == bvmask_Full || m_character->isEffectorInMask(mask, get##_name()->getJointIndex()))\
  data->get##_name()->reset((NmRsEffectorBase*)get##_name(), type, scale);

#define OPPOSE_GRAVITY_MASKED(_name)\
  if(mask == bvmask_Full || m_character->isEffectorInMask(mask, get##_name()->getJointIndex()))\
  data->get##_name()->setOpposeGravity(oppose);

#define HOLD_POSE_MASKED(_name)\
  if(mask == bvmask_Full || m_character->isEffectorInMask(mask, get##_name()->getJointIndex()))\
  data->get##_name()->holdPose((NmRsEffectorBase*)get##_name());

#define ACTIVE_POSE_MASKED(_name)\
  if(mask == bvmask_Full || m_character->isEffectorInMask(mask, get##_name()->getJointIndex()))\
  data->get##_name()->activePose((NmRsEffectorBase*)get##_name(), transformSource);

  NmRsLimb::NmRsLimb() :
    m_character(0),
    m_type((NmRsHumanLimbTypes)-1)
  {
#if ART_ENABLE_BSPY_LIMBS
    m_maxInputsUsed = 0;
#endif
  }

  NmRsLimb::~NmRsLimb()
  {

  }

  void NmRsLimb::postInput(NmRsLimbInput& input)
  {
    if(!(input.valid && input.dataValid))
    {
      // Clear input for safety.
      input.init();
      return;
    }
#if __ASSERT
    // Spot-check for bad IK targets. This will catch them while we still know
    // who the culprit is...
    if(input.type == kIk)
    {
      rage::Vector3 target = input.getData<NmRsIKInputWrapper>()->getTarget();
      Assert(target == target);
    }
#endif

    if(input.mask & m_allEffectorMask)
    {
#if __ASSERT
		// Assert on validity of common problem values
		if(input.type == kIk)
		{
			rage::Vector3 target = input.getData<NmRsIKInputWrapper>()->getTarget();
			Assert(target == target);
		}
#endif
      m_input.add(input);
    }

    // Mark as invalid to indicate that no further changes can be made to this input.
    // Clear input for safety.
    input.init();
  }

  void NmRsLimb::tick(float /*timeStep*/)
  {
#if ART_ENABLE_BSPY_LIMBS
    m_maxInputsUsed = rage::Max((int)m_input.size(), m_maxInputsUsed);
    bspyScratchpad(m_character->getBSpyID(), "limb", m_maxInputsUsed);
#endif

    clearQueue();
  }

  void NmRsLimb::clearQueue()
  {
    m_input.clear();
  }

#if ART_ENABLE_BSPY_LIMBS
  void NmRsLimb::sendMessageDebug(NmRsLimbInput* input)
  {
    if( ART::bSpyServer::inst() &&
        ART::bSpyServer::inst()->isClientConnected() &&
        ART::bSpyServer::inst()->shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_Limbs))
    {
      Assert(input);

      bSpyStringToken limbName = bSpyServer::inst()->getTokenForString(humanLimbTypeStrings[m_type]);
      bSpyStringToken messageName = bSpyServer::inst()->getTokenForString(limbInputTypeStrings[input->type]);

      char buffer[1024];
      if(input->subTask == 0)
        sprintf(buffer, "%s", s_bvIDNames[input->task]);
      else
        sprintf(buffer, "%s:%s", s_bvIDNames[input->task], input->subTask);

      bSpy::LimbsBeginPacket lp(
        limbName,
        messageName,
        bSpyServer::inst()->getTokenForString(buffer),
        input->priority,
        input->subPriority,
        input->mask,
        input->weight,
        (bs_int8)m_character->getID());

      bspySendPacket(lp);


      Assert(input->valid && input->dataValid && input->data);
      NmRsInputWrapperBase* inputBase = input->getBase();
      inputBase->sendComponents();

      bSpy::LimbsEndPacket mep;
      bspySendPacket(mep);
    }
  }

  void NmRsLimb::sendOutputDebug(NmRsInputWrapperBase* data)
  {
    if( ART::bSpyServer::inst() &&
      ART::bSpyServer::inst()->isClientConnected() &&
      ART::bSpyServer::inst()->shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_Limbs))
    {
      Assert(data);

      bSpyStringToken limbName = bSpyServer::inst()->getTokenForString(humanLimbTypeStrings[m_type]);
      bSpyStringToken messageName = bSpyServer::inst()->getTokenForString("Output");
      bSpy::LimbsBeginPacket lp(
        limbName,
        messageName,
        0,              // task
        0,              // priority
        0,              // sub priority
        (int)bvmask_Full,
        1.0f,           // weight
        (bs_int8)m_character->getID());
      bspySendPacket(lp);

      data->sendComponents("Output");

      bSpy::LimbsEndPacket mep;
      bspySendPacket(mep);
    }
  }
#endif //ART_ENABLE_BSPY_LIMBS

  //
  // NmRsHumanArm
  //---------------------------------------------------------------------------

  NmRsHumanArm::NmRsHumanArm() :
    NmRsLimb()
  {
    m_Clavicle = 0;
    m_Shoulder = 0;
    m_Elbow = 0;
    m_Wrist = 0;
    m_root = 0;
    m_claviclePart = 0;
    m_upperArm = 0;
    m_lowerArm = 0;
    m_hand = 0;
  }
  
  void NmRsHumanArm::setup(NmRsCharacter* character,
                          NmRsHumanLimbTypes type,
                          NmRsEffectorBase * clavicle,
                          NmRsEffectorBase * shoulder,
                          NmRsEffectorBase * elbow,
                          NmRsEffectorBase * wrist,
                          NmRsGenericPart * root,
                          NmRsGenericPart * claviclePart,
                          NmRsGenericPart * upperArm,
                          NmRsGenericPart * lowerArm,
                          NmRsGenericPart * hand,
                          float direction,
                          float hingeDirection,
                          bool /* twistIsFixed */)
  {
    Assert(character);
    m_character = character;

    Assert(type >= 0 && type < kNumNmRsHumanLimbs);
    m_type = type;
    Assert(clavicle->is3DofEffector());
    m_Clavicle = (NmRs3DofEffector*)clavicle;
    Assert(shoulder->is3DofEffector());
    m_Shoulder = (NmRs3DofEffector*)shoulder;
    Assert(!elbow->is3DofEffector());
    m_Elbow = (NmRs1DofEffector*)elbow;
    Assert(wrist->is3DofEffector());
    m_Wrist = (NmRs3DofEffector*)wrist;
    m_root = root;
    m_claviclePart = claviclePart;
    m_upperArm = upperArm;
    m_lowerArm = lowerArm;
    m_hand = hand;

    // Build a mask of all the effectors in this limb.
    m_allEffectorMask = bvmask_None;
    m_allEffectorMask |= m_character->partToMask(claviclePart->getPartIndex());
    m_allEffectorMask |= m_character->partToMask(upperArm->getPartIndex());
    m_allEffectorMask |= m_character->partToMask(lowerArm->getPartIndex());
    m_allEffectorMask |= m_character->partToMask(hand->getPartIndex());

    m_direction = direction;
    m_hingeDirection = hingeDirection;
  }

  void NmRsHumanArm::init()
  {
    rage::Matrix34 threeDofMatrix2;
    m_Elbow->getMatrix1(m_elbowMat);
    m_Shoulder->getMatrix2(threeDofMatrix2);
    m_elbowMat.DotTranspose(threeDofMatrix2);
  }

  NmRsHumanArm::~NmRsHumanArm()
  {

  }

  void NmRsHumanArm::tick(float timeStep)
  {
    if(m_input.size() == 0)
      return;

    // Sort inputs by weight priority
#if ART_ENABLE_BSPY
    bspyProfileStart("NmRsHumanArm::tick");
    bspyProfileStart("sort");
#endif

    m_input.sort();

#if ART_ENABLE_BSPY
    bspyProfileEnd("sort");
#endif

#if ART_ENABLE_BSPY_LIMBS & 1
    // Post messages, in order, for bSpy viewing.
    for(unsigned int i = 0; i < m_input.size(); ++i)
      sendMessageDebug(m_input.get(i));
#endif //ART_ENABLE_BSPY_LIMBS

    // Set up blendable target structure.
    m_blendTarget.init();

    // Step down through the list of inputs.
    NmRsArmInputWrapper* pCurrent = 0;
    NmRsArmInputWrapper  current; 

    for(unsigned int i = 0; i < m_input.size(); ++i)
    {
      NmRsLimbInput* it = m_input.get(i);
      if(it->type == kIk)
      {
        // If the required blend positions are still free, resolve IK request
        // into a full pose.
        NmRsIKInputWrapper* ikDataIn = it->getData<NmRsIKInputWrapper>();
        Assert(ikDataIn);

        if(m_blendTarget.canDoIK(ikDataIn->getCanDoIKGreedy()))
        {
          doIk(*(it), current);
          pCurrent = &current;
        }
      }
      else if(it->type == kArmPose)
      {
        // Otherwise just point to current pose input.
        pCurrent = it->getData<NmRsArmInputWrapper>();
      }
      else if(it->type == kSetStiffness)
      {
        NmRsSetStiffnessInputWrapper* inputData = it->getData<NmRsSetStiffnessInputWrapper>();
        Assert(inputData);
        setStiffness(&current, inputData->m_stiffness, inputData->m_damping, it->mask, inputData->m_applyMuscleStiffness ? &inputData->m_muscleStiffness : 0);
        pCurrent = &current;
      }
      else if(it->type == kStopAll)
      {
        stopAll(&current);
        pCurrent = &current;
      }
      else
      {
        // Unrecognized input type.
        Assert(false);
      }

      // Blend current request into the blend target.
#if ART_ENABLE_BSPY
      bspyProfileStart("blend");
#endif

      bool done = false;
      if(pCurrent)
        done = blend(pCurrent, it->weight, it->mask DEBUG_LIMBS_PARAMETER(it->task) DEBUG_LIMBS_PARAMETER(it->subTask));

      // Clear input for safety.
      it->init();

#if ART_ENABLE_BSPY
      bspyProfileEnd("blend");
#endif

      // Exit if we have no remaining available blend positions.
      // TODO rework into cleaner loop with do/while.
      if(done)
      {
        break;
      }
    }

    // send output debug
#if ART_ENABLE_BSPY_LIMBS
    sendOutputDebug(static_cast<NmRsInputWrapperBase*>(&m_blendTarget));
#endif

#if ART_ENABLE_BSPY
    bspyProfileStart("write");
#endif

    // write to effectors from temporary pose data
    set3DofParameters(m_Clavicle, m_blendTarget.getClavicleBlendable());
    set3DofParameters(m_Shoulder, m_blendTarget.getShoulderBlendable());
    set1DofParameters(m_Elbow, m_blendTarget.getElbowBlendable());
    set3DofParameters(m_Wrist, m_blendTarget.getWristBlendable());
#if ART_ENABLE_BSPY
    bspyProfileEnd("write");
#endif

    // tick base to clear input vector, etc.
    NmRsLimb::tick(timeStep);

#if ART_ENABLE_BSPY
    bspyProfileEnd("NmRsHumanArm::tick");
#endif
  }

#undef BLEND_EFFECTOR
#if ART_ENABLE_BSPY_LIMBS
#define BLEND_EFFECTOR(_prefix, _name)\
  if(mask == bvmask_Full || m_character->isEffectorInMask(mask, get##_name()->getJointIndex()))\
    result = m_blendTarget.get##_name##Blendable()->blend(input->m_data.m_##_name, task, weight, subTask) && result;
#else
#define BLEND_EFFECTOR(_prefix, _name)\
  if(mask == bvmask_Full || m_character->isEffectorInMask(mask, get##_name()->getJointIndex()))\
    result = m_blendTarget.get##_name##Blendable()->blend(input->m_data.m_##_name, bvid_Invalid, weight) && result;
#endif

#if ART_ENABLE_BSPY_LIMBS
  bool NmRsHumanArm::blend(NmRsArmInputWrapper* input, float weight, BehaviourMask mask, BehaviourID task, const char* subTask)
#else
  bool NmRsHumanArm::blend(NmRsArmInputWrapper* input, float weight, BehaviourMask mask)
#endif
  {
    // Input must be an arm pose. Possibly expand this function to handle other
    // kinds of arm inputs (setStiffness, etc) if it will speed up the tick.
    Assert(input);

    bool result = true;
    
    // Call each blendable effector blend function. e.g:
    //   if(mask == bvmask_Full || m_character->isEffectorInMask(mask, getShoulder()->getJointIndex()))\
    //     result = m_blendTarget.getShoulderBlendable()->blend(input->m_data.m_Shoulder, task, weight) && result;
    #define NM_RS_PARAMETER(_prefix, _name) BLEND_EFFECTOR(_prefix, _name)
    #include "common\NmRsHumanArm.inl"
    #undef NM_RS_PARAMETER

    return result;
  }

  // phase2 todo generalize this function for use by all limbs that support
  // 2bone IK. rename. add more IK types and mechanism for enabling support in
  // different limb types.
  void NmRsHumanArm::doIk(NmRsLimbInput& ikMsg, NmRsArmInputWrapper& poseDataOut)
  {
#if ART_ENABLE_BSPY
    bspyProfileStart("doIk");
    bspyProfileStart("setup");
#endif
    Assert(ikMsg.type == kIk);
    NmRsIKInputWrapper* ikDataIn = ikMsg.getData<NmRsIKInputWrapper>();
    Assert(ikDataIn);

    // phase2 todo implement pre-ik clavicle matching here

    NmRsLimbIKInput ikInput;
    NmRsLimbIKOutput ikOutput;

    ShadowGPart rootPart, endPart;
    Shadow1Dof oneDof;
    Shadow3Dof threeDof;

    // phase2 todo consider merging NmRsLimbIKInput/NmRsLimbIKOutput with the
    // new structures to avoid all this setup. currently works well to maintain
    // backward compatibility.

    rage::Vector3 target(ikDataIn->getTarget());
    rage::Vector3 targetVelocity(ikDataIn->getVelocity());
    rage::Vector3 poleVector(ikDataIn->getPoleVector());
    rage::Vector3 effectorOffset(ikDataIn->getEffectorOffset());

    ikInput.target                    = &target;
    ikInput.twist                     = ikDataIn->getTwist();
    ikInput.dragReduction             = ikDataIn->getDragReduction();
    ikInput.maxReachDistance          = ikDataIn->getMaxReachDistance();
    ikInput.blend                     = ikDataIn->getBlend();
    if(ikDataIn->m_data.m_flags & NmRsIKInputData::applyVelocity)
    {
      ikInput.option.useTargetVelocity  = true;
      ikInput.targetVelocity = &targetVelocity;
      Assert(ikInput.targetVelocity->x == ikInput.targetVelocity->x);
    }
    ikInput.option.twistIsFixed       = ikDataIn->getTwistIsFixed();
    ikInput.option.advancedIK         = ikDataIn->getUseAdvancedIk();
    ikInput.straightness              = ikDataIn->getAdvancedStaightness();
    ikInput.maxSpeed                  = ikDataIn->getAdvancedMaxSpeed();

    if(ikDataIn->m_data.m_flags & NmRsIKInputData::applyPoleVector)
    {
      ikInput.poleDirection = &poleVector;
      Assert(ikInput.poleDirection->x == ikInput.poleDirection->x);
    }

    if(ikDataIn->m_data.m_flags & NmRsIKInputData::applyEffectorOffset)
    {
      ikInput.effectorOffset = &effectorOffset;
      Assert(ikInput.effectorOffset->x == ikInput.effectorOffset->x);
    }

    ikInput.direction = m_direction;
    ikInput.hingeDirection = m_hingeDirection;
    ikInput.elbowMat = &m_elbowMat;

    m_Shoulder->saveToShadow(threeDof);
    m_Elbow->saveToShadow(oneDof);
    m_claviclePart->saveToShadow(rootPart);
    m_hand->saveToShadow(endPart);

    ikInput.threeDof = &threeDof;
    ikInput.oneDof = &oneDof;
    ikInput.rootPart = &rootPart;
    ikInput.endPart = &endPart;

    ikOutput.threeDof = threeDof;
    ikOutput.oneDof = oneDof;

#if ART_ENABLE_BSPY
    bspyProfileEnd("setup");
#endif

    // solve
    limbIK(ikInput, ikOutput);

    // copy to output pose structure
    poseDataOut.getShoulder()->setDesiredTwist(ikOutput.threeDof.m_desiredTwist);
    poseDataOut.getShoulder()->setDesiredLean1(ikOutput.threeDof.m_desiredLean1);
    poseDataOut.getShoulder()->setDesiredLean2(ikOutput.threeDof.m_desiredLean2);

    poseDataOut.getElbow()->setDesiredAngle(ikOutput.oneDof.m_desiredAngle);

#if ART_ENABLE_BSPY
    bspyProfileStart("clavicle");
#endif
    switch(ikDataIn->getMatchClavicle())
    {
    case kMatchClavicle:
      matchClavicleToShoulder(poseDataOut);
      break;
    case kMatchClavicleBetter:
      matchClavicleToShoulderBetter(poseDataOut);
      break;
    case kMatchClavicleUsingTwist:
      matchClavicleToShoulderUsingTwist(poseDataOut);
      break;
    case kDontMatchClavicle:
    default:
      break;
    }
#if ART_ENABLE_BSPY
    bspyProfileEnd("clavicle");
#endif

    if(ikDataIn->m_data.m_flags & NmRsIKInputData::applyMinimumElbowAngle)
      poseDataOut.getElbow()->setDesiredAngle(rage::Max(ikDataIn->getMinimumElbowAngle(), poseDataOut.getElbow()->getDesiredAngle()));
    if(ikDataIn->m_data.m_flags & NmRsIKInputData::applyMaximumElbowAngle)
      poseDataOut.getElbow()->setDesiredAngle(rage::Min(ikDataIn->getMaximumElbowAngle(), poseDataOut.getElbow()->getDesiredAngle()));

    // do wrist IK if we have a target.
    if(ikDataIn->m_data.m_flags & NmRsIKInputData::applyWristTarget)
    {
#if ART_ENABLE_BSPY
      bspyProfileStart("wrist");
#endif
      wristIk(poseDataOut, ikDataIn);
#if ART_ENABLE_BSPY
      bspyProfileEnd("wrist");
#endif
    }

#if ART_ENABLE_BSPY
    bspyProfileEnd("doIk");
#endif
  }

  void NmRsHumanArm::matchClavicleToShoulder(NmRsArmInputWrapper& poseData)
  {
    float shL1    = poseData.getShoulder()->getDesiredLean1();
    float shL2    = poseData.getShoulder()->getDesiredLean2();
    float L1_gte  = shL1 * (m_Clavicle->getMaxLean1() / m_Shoulder->getMaxLean1());
    float L1_lt   = shL1 * (m_Clavicle->getMinLean1() / m_Shoulder->getMinLean1());
    float L2_gte  = shL2 * (m_Clavicle->getMaxLean2() / m_Shoulder->getMaxLean2());
    float L2_lt   = shL2 * (m_Clavicle->getMinLean2() / m_Shoulder->getMinLean2());
    poseData.getClavicle()->setDesiredLean1( rage::Selectf(shL1, L1_gte, L1_lt) );
    poseData.getClavicle()->setDesiredLean2( rage::Selectf(shL2, L2_gte, L2_lt) );
  }

  void NmRsHumanArm::matchClavicleToShoulderBetter(NmRsArmInputWrapper& poseData)
  {

    float totalLean1 = m_Clavicle->getActualLean1() + poseData.getShoulder()->getDesiredLean1();
    float totalLean2 = m_Clavicle->getActualLean2() + poseData.getShoulder()->getDesiredLean2();

    float totalMaxL1 = 1.0f / (m_Clavicle->getMaxLean1() + m_Shoulder->getMaxLean1());
    float totalMinL1 = 1.0f / (m_Clavicle->getMinLean1() + m_Shoulder->getMinLean1());
    float totalMaxL2 = 1.0f / (m_Clavicle->getMaxLean2() + m_Shoulder->getMaxLean2());
    float totalMinL2 = 1.0f / (m_Clavicle->getMinLean2() + m_Shoulder->getMinLean2());

    float gte_clavL1 = totalLean1 * (m_Clavicle->getMaxLean1()) * totalMaxL1;
    float gte_shldL1 = totalLean1 * (m_Clavicle->getMaxLean1()) * totalMaxL1;
    float gte_clavL2 = totalLean2 * (m_Clavicle->getMaxLean2()) * totalMaxL2;
    float gte_shldL2 = totalLean2 * (m_Clavicle->getMaxLean2()) * totalMaxL2;

    float lt_clavL1 = totalLean1 * (m_Clavicle->getMinLean1()) * totalMinL1;
    float lt_shldL1 = totalLean1 * (m_Shoulder->getMinLean1()) * totalMinL1;
    float lt_clavL2 = totalLean2 * (m_Clavicle->getMinLean2()) * totalMinL2;
    float lt_shldL2 = totalLean2 * (m_Shoulder->getMinLean2()) * totalMinL2;

    poseData.getClavicle()->setDesiredLean1( rage::Selectf(totalLean1, gte_clavL1, lt_clavL1) );
    poseData.getShoulder()->setDesiredLean1( rage::Selectf(totalLean1, gte_shldL1, lt_shldL1) );

    poseData.getClavicle()->setDesiredLean2( rage::Selectf(totalLean2, gte_clavL2, lt_clavL2) );
    poseData.getShoulder()->setDesiredLean2( rage::Selectf(totalLean2, gte_shldL2, lt_shldL2) );

  }

  void NmRsHumanArm::matchClavicleToShoulderUsingTwist(NmRsArmInputWrapper& poseData)
  {
    float lean1XS =  poseData.getShoulder()->getDesiredLean1() - getShoulder()->getMaxLean1();
    if (lean1XS > 0.0f )
    {
      poseData.getClavicle()->setDesiredTwist(-lean1XS);
    }
    else
    {
      float lean1XS =  poseData.getShoulder()->getDesiredLean1() - getShoulder()->getMinLean1();
      if (lean1XS < 0.0f )
      {
        poseData.getClavicle()->setDesiredTwist(-lean1XS);
      }
    }
  }

  void NmRsHumanArm::wristIk(NmRsArmInputWrapper& poseDataOut, NmRsIKInputWrapper* ikDataIn)
  {
    bool useActualAngles1 = false;
    float twistLimit1 = 2.4f;
    if (ikDataIn->m_data.m_flags & NmRsIKInputData::applyWristUseActualAngles)
      useActualAngles1 = ikDataIn->getWristUseActualAngles();
    if (ikDataIn->m_data.m_flags & NmRsIKInputData::applyWristTwistLimit)
      twistLimit1 = ikDataIn->getWristTwistLimit();

    rage::Vector3 elbowPos;
    rage::Vector3 target(ikDataIn->getWristTarget());
    rage::Vector3 normal(ikDataIn->getWristNormal());

    Shadow3Dof shoulder, wrist;
    Shadow1Dof elbow;

    getShoulder()->saveToShadow(shoulder);
    getElbow()->saveToShadow(elbow);
    getWrist()->saveToShadow(wrist);
    getLimbHingePos(elbowPos, shoulder, elbow, m_direction);

    /*float dirResult = */wristIK(target, normal, elbowPos, shoulder, wrist, m_direction, useActualAngles1, twistLimit1);

    poseDataOut.getWrist()->setDesiredAngles(wrist.m_desiredLean1, wrist.m_desiredLean2, wrist.m_desiredTwist);
  }

  void NmRsHumanArm::setToCurrent(NmRsArmInputWrapper* data, BehaviourMask mask /*= bvmask_Full*/) const
  {
    #define NM_RS_PARAMETER(_type, _name) SET_CURRENT_MASKED(_name)
    #include "common\NmRsHumanArm.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRsHumanArm::setStiffness(NmRsArmInputWrapper* data, float stiffness, float damping, BehaviourMask mask /*= bvmask_Full*/, float *muscleStiffness /*= 0*/) const
  {
    #define NM_RS_PARAMETER(_type, _name) BODYSTIFFNESS_MASKED(_name)
    #include "common\NmRsHumanArm.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRsHumanArm::setBodyStiffness(NmRsLimbInput& input, float stiffness, float damping, BehaviourMask mask /*= bvmask_Full*/, float *muscleStiffness /*= 0*/) const
  {
    NmRsArmInputWrapper* inputData = input.getData<NmRsArmInputWrapper>();
    Assert(inputData);
    setStiffness(inputData, stiffness, damping, mask, muscleStiffness);
  }

  void NmRsHumanArm::setBodyStiffnessScaling(float stiffnessScale, float dampingScale, float muscleStiffnessScale, BehaviourMask mask /*= bvmask_Full*/) const
  {
    float strengthScale = stiffnessScale * stiffnessScale;
    float dampScale = stiffnessScale * dampingScale;

    // Apply scaling to each effector in the mask, eg:
    //
    // if(m_character->isEffectorInMask(mask, m_Clavicle->getJointIndex()))
    // {
    //   m_Clavicle->setMuscleStrengthScaling(strengthScale);
    //   m_Clavicle->setMuscleDampingScaling(dampScale);
    //   m_Clavicle->setMuscleStiffnessScaling(muscleStiffnessScale);
    // }
    //
    // Auto-generate parameter initialization from inline file.
    #define NM_RS_PARAMETER(_type, _name) BODYSTIFFNESSSCALING_MASKED(_name)
    #include "common\NmRsHumanArm.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRsHumanArm::setRelaxation(NmRsLimbInput& input, float mult, BehaviourMask mask, float *pMultDamping /*= 0*/) const
  {
    NmRsArmInputWrapper* data = input.getData<NmRsArmInputWrapper>();
#define NM_RS_PARAMETER(_type, _name) SETRELAXATION_MASKED(_name)
#include "common\NmRsHumanArm.inl"
#undef NM_RS_PARAMETER
  }

  void NmRsHumanArm::activePose(NmRsLimbInput& input, int transformSource, BehaviourMask mask /*= bvmask_Full*/) const
  {
    NmRsArmInputWrapper* data = input.getData<NmRsArmInputWrapper>();
#define NM_RS_PARAMETER(_type, _name) ACTIVE_POSE_MASKED(_name)
#include "common\NmRsHumanArm.inl"
#undef NM_RS_PARAMETER
  }

  void NmRsHumanArm::blendToZeroPose(NmRsLimbInput& input, float t, BehaviourMask mask /*= bvmask_Full*/) const
  {
    NmRsArmInputWrapper* data = input.getData<NmRsArmInputWrapper>();
#define NM_RS_PARAMETER(_type, _name) BLENDTOZERO_MASKED(_name)
#include "common\NmRsHumanArm.inl"
#undef NM_RS_PARAMETER
  }

  void NmRsHumanArm::holdPose(NmRsLimbInput& input, BehaviourMask mask /*= bvmask_Full*/) const
  {
    NmRsArmInputWrapper* data = input.getData<NmRsArmInputWrapper>();
#define NM_RS_PARAMETER(_type, _name) HOLD_POSE_MASKED(_name)
#include "common\NmRsHumanArm.inl"
#undef NM_RS_PARAMETER
  }

  void NmRsHumanArm::resetEffectors(NmRsLimbInput& input, ResetEffectorsType type, BehaviourMask mask /*= bvmask_Full*/, float scale /*= 1.0f*/) const
  {
    NmRsArmInputWrapper* data = input.getData<NmRsArmInputWrapper>();
#define NM_RS_PARAMETER(_type, _name) RESET_EFFECTORS_MASKED(_name)
#include "common\NmRsHumanArm.inl"
#undef NM_RS_PARAMETER
  }

  void NmRsHumanArm::setOpposeGravity(NmRsLimbInput& input, float oppose, BehaviourMask mask /*= bvmask_Full*/) const
  {
    NmRsArmInputWrapper* data = input.getData<NmRsArmInputWrapper>();
#define NM_RS_PARAMETER(_type, _name) OPPOSE_GRAVITY_MASKED(_name)
#include "common\NmRsHumanArm.inl"
#undef NM_RS_PARAMETER
  }

  void NmRsHumanArm::stopAll(NmRsArmInputWrapper* data, BehaviourMask mask /*= bvmask_Full*/) const
  {
    const ResetEffectorsType type = kResetCalibrations | kResetAngles;
    const float scale = 1.0f;
#define NM_RS_PARAMETER(_type, _name) RESET_EFFECTORS_MASKED(_name)
#include "common\NmRsHumanArm.inl"
#undef NM_RS_PARAMETER

#define NM_RS_PARAMETER(_type, _name) HOLD_POSE_MASKED(_name)
#include "common\NmRsHumanArm.inl"
#undef NM_RS_PARAMETER

    setStiffness(data, 5.0f, 0.5f, mask);
  }

  void NmRsHumanArm::callMaskedEffectorFunctionFloatArg(
    NmRsLimbInput& input,
    BehaviourMask mask,
    float floatValue,
    Effector1DofDataFuncFloatArg oneDofFn,
    Effector3DofDataFuncFloatArg threeDofFn) const
  {
    NmRsArmInputWrapper* data = input.getData<NmRsArmInputWrapper>();
    if(m_character->isEffectorInMask(mask, m_Clavicle->getJointIndex()))
      (data->getClavicle()->*threeDofFn)(floatValue);
    if(m_character->isEffectorInMask(mask, m_Shoulder->getJointIndex()))
      (data->getShoulder()->*threeDofFn)(floatValue);
    if(m_character->isEffectorInMask(mask, m_Elbow->getJointIndex()))
      (data->getElbow()->*oneDofFn)(floatValue);
    if(m_character->isEffectorInMask(mask, m_Wrist->getJointIndex()))
      (data->getWrist()->*threeDofFn)(floatValue);
  }

  //
  // NmRsHumanLeg
  //---------------------------------------------------------------------------

  NmRsHumanLeg::NmRsHumanLeg() :
    NmRsLimb()
  {
    m_Hip = 0;
    m_Knee = 0;
    m_Ankle = 0;
    m_root = 0;
    m_thigh = 0;
    m_shin = 0;
    m_foot = 0;
  }
  
  void NmRsHumanLeg::setup(NmRsCharacter* character,
                          NmRsHumanLimbTypes type,
                          NmRsEffectorBase * hip,
                          NmRsEffectorBase * knee,
                          NmRsEffectorBase * ankle,
                          NmRsGenericPart * root,
                          NmRsGenericPart * thigh,
                          NmRsGenericPart * shin,
                          NmRsGenericPart * foot,
                          float direction,
                          float hingeDirection,
                          bool /* twistIsFixed */)
  {
    Assert(character);
    m_character = character;

    Assert(type >= 0 && type < kNumNmRsHumanLimbs);
    m_type = type;
    Assert(hip->is3DofEffector());
    m_Hip = (NmRs3DofEffector*)hip;
    Assert(!knee->is3DofEffector());
    m_Knee = (NmRs1DofEffector*)knee;
    Assert(ankle->is3DofEffector());
    m_Ankle = (NmRs3DofEffector*)ankle;
    m_root = root;
    m_thigh = thigh;
    m_shin = shin;
    m_foot = foot;

    // Build a mask of all the effectors in this limb.
    m_allEffectorMask = bvmask_None;
    m_allEffectorMask |= m_character->partToMask(thigh->getPartIndex());
    m_allEffectorMask |= m_character->partToMask(shin->getPartIndex());
    m_allEffectorMask |= m_character->partToMask(foot->getPartIndex());

    m_direction = direction;
    m_hingeDirection = hingeDirection;
  }

  void NmRsHumanLeg::init()
  {
    rage::Matrix34 threeDofMatrix2;
    m_Knee->getMatrix1(m_elbowMat);
    m_Hip->getMatrix2(threeDofMatrix2);
    m_elbowMat.DotTranspose(threeDofMatrix2);
  }

  NmRsHumanLeg::~NmRsHumanLeg()
  {

  }

  void NmRsHumanLeg::tick(float timeStep)
  {
    if(m_input.size() == 0)
      return;

#if ART_ENABLE_BSPY
    bspyProfileStart("NmRsHumanLeg::tick");
#endif

    // sort inputs by weight priority
    m_input.sort();

#if ART_ENABLE_BSPY_LIMBS
    // Send messages, in order, for bSpy viewing.
    for(unsigned int i = 0; i < m_input.size(); ++i)
      sendMessageDebug(m_input.get(i));
#endif //ART_ENABLE_BSPY_LIMBS

    // Set up blendable target structure.
    m_blendTarget.init();

    // step down through the list of inputs.
    NmRsLegInputWrapper* pCurrent = 0;
    NmRsLegInputWrapper  current;

    for(unsigned int i = 0; i < m_input.size(); ++i)
    {
      NmRsLimbInput* it = m_input.get(i);

      if(it->type == kIk)
      {
        NmRsIKInputWrapper* ikDataIn = it->getData<NmRsIKInputWrapper>();
        Assert(ikDataIn);

        if(m_blendTarget.canDoIK(ikDataIn->getCanDoIKGreedy()))
        {
          doIk(*(it), current);
          pCurrent = &current;
        }
      }
      else if(it->type == kLegPose)
      {
        // otherwise just point to current pose input.
        pCurrent = it->getData<NmRsLegInputWrapper>();
      }
      else if(it->type == kSetStiffness)
      {
        NmRsSetStiffnessInputWrapper* inputData = it->getData<NmRsSetStiffnessInputWrapper>();
        Assert(inputData);
        setStiffness(&current, inputData->m_stiffness, inputData->m_damping, it->mask, inputData->m_applyMuscleStiffness ? &inputData->m_muscleStiffness : 0);
        pCurrent = &current;
      }
      else if(it->type == kStopAll)
      {
        stopAll(&current);
        pCurrent = &current;
      }
      else
      {
        // todo support other kinds of inputs.
        // zero pose...
      }

      // blend into the temporary limb input.
      bool done = false;
      if(pCurrent)
        done = blend(pCurrent, it->weight, it->mask DEBUG_LIMBS_PARAMETER(it->task) DEBUG_LIMBS_PARAMETER(it->subTask));

      // if not blending (or blend depth reached) and nothing is left to set, we're done!
      if(done)
        break;
    }

    // send output debug
#if ART_ENABLE_BSPY_LIMBS
    sendOutputDebug(static_cast<NmRsInputWrapperBase*>(&m_blendTarget));
#endif

    // write to effectors from temporary pose data
    set3DofParameters(m_Hip, m_blendTarget.getHipBlendable());
    set1DofParameters(m_Knee, m_blendTarget.getKneeBlendable());
    set3DofParameters(m_Ankle, m_blendTarget.getAnkleBlendable());

    // tick base to clear input vector, etc.
    NmRsLimb::tick(timeStep);

#if ART_ENABLE_BSPY
    bspyProfileEnd("NmRsHumanLeg::tick");
#endif
  }

#if ART_ENABLE_BSPY_LIMBS
  bool NmRsHumanLeg::blend(NmRsLegInputWrapper* input, float weight, BehaviourMask mask, BehaviourID task, const char* subTask)
#else
  bool NmRsHumanLeg::blend(NmRsLegInputWrapper* input, float weight, BehaviourMask mask)
#endif
  {
    // Input must be a leg pose. Possibly expand this function to handle other
    // kinds of arm inputs (setStiffness, etc) if it will speed up the tick.
    Assert(input);

    bool result = true;

#if 0
    // Call each blendable effector blend function. e.g:
    if(mask == bvmask_Full || m_character->isEffectorInMask(mask, getHip()->getJointIndex()))\
      result = m_blendTarget.getHipBlendable()->blend(input->m_data.m_Hip, task, weight, subTask) && result;
#else
    #define NM_RS_PARAMETER(_prefix, _name) BLEND_EFFECTOR(_prefix, _name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
#endif

    return result;
  }

  void NmRsHumanLeg::doIk(NmRsLimbInput& ikMsg, NmRsLegInputWrapper& poseDataOut)
  {
    Assert(ikMsg.type == kIk);
    NmRsIKInputWrapper* ikDataIn = ikMsg.getData<NmRsIKInputWrapper>();
    Assert(ikDataIn);

    // todo compute desired clavicle transform

    NmRsLimbIKInput ikInput;
    NmRsLimbIKOutput ikOutput;

    ShadowGPart rootPart, endPart;
    Shadow1Dof oneDof;
    Shadow3Dof threeDof;

    // phase2 todo consider merging NmRsLimbIKInput/NmRsLimbIKOutput with the
    // new structures to avoid all this setup. currently works well to maintain
    // backward compatibility.

    rage::Vector3 target(ikDataIn->getTarget());
    rage::Vector3 targetVelocity(ikDataIn->getVelocity());
    rage::Vector3 poleVector(ikDataIn->getPoleVector());
    rage::Vector3 effectorOffset(ikDataIn->getEffectorOffset());

    ikInput.poleDirection             = 0;
    ikInput.effectorOffset            = 0;
    ikInput.targetVelocity            = 0;

    ikInput.target                    = &target;
    ikInput.twist                     = ikDataIn->getTwist();
    ikInput.dragReduction             = ikDataIn->getDragReduction();
    ikInput.maxReachDistance          = ikDataIn->getMaxReachDistance();
    ikInput.blend                     = ikDataIn->getBlend();
    if(ikDataIn->m_data.m_flags & NmRsIKInputData::applyVelocity)
    {
      ikInput.option.useTargetVelocity  = true;
      ikInput.targetVelocity = &targetVelocity;
    }
    else
    {
      ikInput.option.useTargetVelocity  = false;
      ikInput.targetVelocity = 0;
    }
    ikInput.option.twistIsFixed       = ikDataIn->getTwistIsFixed();
    ikInput.option.advancedIK         = ikDataIn->getUseAdvancedIk();
    ikInput.straightness              = ikDataIn->getAdvancedStaightness();
    ikInput.maxSpeed                  = ikDataIn->getAdvancedMaxSpeed();

    if(ikDataIn->m_data.m_flags & NmRsIKInputData::applyPoleVector)
    {
      ikInput.poleDirection = &poleVector;
    }

    if(ikDataIn->m_data.m_flags & NmRsIKInputData::applyEffectorOffset)
    {
      ikInput.effectorOffset = &effectorOffset;
    }

    ikInput.direction = m_direction;
    ikInput.hingeDirection = m_hingeDirection;
    ikInput.elbowMat = &m_elbowMat;

    m_Hip->saveToShadow(threeDof);
    m_Knee->saveToShadow(oneDof);
    m_root->saveToShadow(rootPart);
    m_foot->saveToShadow(endPart);

    ikInput.threeDof = &threeDof;
    ikInput.oneDof = &oneDof;
    ikInput.rootPart = &rootPart;
    ikInput.endPart = &endPart;

    ikOutput.threeDof = threeDof;
    ikOutput.oneDof = oneDof;

    // solve
    limbIK(ikInput, ikOutput);

    // copy to output pose structure
    poseDataOut.getHip()->setDesiredTwist(ikOutput.threeDof.m_desiredTwist);
    poseDataOut.getHip()->setDesiredLean1(ikOutput.threeDof.m_desiredLean1);
    poseDataOut.getHip()->setDesiredLean2(ikOutput.threeDof.m_desiredLean2);
      
    poseDataOut.getKnee()->setDesiredAngle(ikOutput.oneDof.m_desiredAngle);
  }

  void NmRsHumanLeg::setToCurrent(NmRsLegInputWrapper* data, BehaviourMask mask /*= bvmask_Full*/)
  {
#define NM_RS_PARAMETER(_type, _name) SET_CURRENT_MASKED(_name)
#include "common\NmRsHumanLeg.inl"
#undef NM_RS_PARAMETER
  }

  void NmRsHumanLeg::setBodyStiffness(NmRsLimbInput& input, float stiffness, float damping, BehaviourMask mask /*= bvmask_Full*/, float *muscleStiffness /*= 0*/) const
  {
    NmRsLegInputWrapper* data = input.getData<NmRsLegInputWrapper>();
    setStiffness(data, stiffness, damping, mask, muscleStiffness);
  }

  void NmRsHumanLeg::setStiffness(NmRsLegInputWrapper* data, float stiffness, float damping, BehaviourMask mask /*= bvmask_Full*/, float *muscleStiffness /*= 0*/) const
  {
    #define NM_RS_PARAMETER(_type, _name) BODYSTIFFNESS_MASKED(_name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRsHumanLeg::setBodyStiffnessScaling(float stiffnessScale, float dampingScale, float muscleStiffnessScale, BehaviourMask mask /*= bvmask_Full*/) const
  {
    float strengthScale = stiffnessScale * stiffnessScale;
    float dampScale = stiffnessScale * dampingScale;

    // Auto-generate parameter initialization from inline file.
    #define NM_RS_PARAMETER(_type, _name) BODYSTIFFNESSSCALING_MASKED(_name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRsHumanLeg::setRelaxation(NmRsLimbInput& input, float mult, BehaviourMask mask, float *pMultDamping /*= 0*/) const
  {
    NmRsLegInputWrapper* data = input.getData<NmRsLegInputWrapper>();
    #define NM_RS_PARAMETER(_type, _name) SETRELAXATION_MASKED(_name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRsHumanLeg::activePose(NmRsLimbInput& input, int transformSource, BehaviourMask mask /*= bvmask_Full*/) const
  {
    NmRsLegInputWrapper* data = input.getData<NmRsLegInputWrapper>();
    #define NM_RS_PARAMETER(_type, _name) ACTIVE_POSE_MASKED(_name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRsHumanLeg::blendToZeroPose(NmRsLimbInput& input, float t, BehaviourMask mask /*= bvmask_Full*/) const
  {
    NmRsLegInputWrapper* data = input.getData<NmRsLegInputWrapper>();
    #define NM_RS_PARAMETER(_type, _name) BLENDTOZERO_MASKED(_name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRsHumanLeg::holdPose(NmRsLimbInput& input, BehaviourMask mask /*= bvmask_Full*/) const
  {
    NmRsLegInputWrapper* data = input.getData<NmRsLegInputWrapper>();
    #define NM_RS_PARAMETER(_type, _name) HOLD_POSE_MASKED(_name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRsHumanLeg::resetEffectors(NmRsLimbInput& input, ResetEffectorsType type, BehaviourMask mask /*= bvmask_Full*/, float scale /*= 1.0f*/) const
  {
    NmRsLegInputWrapper* data = input.getData<NmRsLegInputWrapper>();
    #define NM_RS_PARAMETER(_type, _name) RESET_EFFECTORS_MASKED(_name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRsHumanLeg::setOpposeGravity(NmRsLimbInput& input, float oppose, BehaviourMask mask /*= bvmask_Full*/) const
  {
    NmRsLegInputWrapper* data = input.getData<NmRsLegInputWrapper>();
    #define NM_RS_PARAMETER(_type, _name) OPPOSE_GRAVITY_MASKED(_name)
    #include "common\NmRsHumanLeg.inl"
    #undef NM_RS_PARAMETER
  }

  void NmRsHumanLeg::stopAll(NmRsLegInputWrapper* data, BehaviourMask mask /*= bvmask_Full*/) const
  {
    const ResetEffectorsType type = kResetCalibrations | kResetAngles;
    const float scale = 1.0f;
#define NM_RS_PARAMETER(_type, _name) RESET_EFFECTORS_MASKED(_name)
#include "common\NmRsHumanLeg.inl"
#undef NM_RS_PARAMETER

#define NM_RS_PARAMETER(_type, _name) HOLD_POSE_MASKED(_name)
#include "common\NmRsHumanLeg.inl"
#undef NM_RS_PARAMETER

    setStiffness(data, 5.0f, 0.5f, mask);
  }

  void NmRsHumanLeg::callMaskedEffectorFunctionFloatArg(
    NmRsLimbInput& input,
    BehaviourMask mask,
    float floatValue,
    Effector1DofDataFuncFloatArg oneDofFn,
    Effector3DofDataFuncFloatArg threeDofFn) const
  {
    NmRsLegInputWrapper* data = input.getData<NmRsLegInputWrapper>();
    if(m_character->isEffectorInMask(mask, m_Hip->getJointIndex()))
      (data->getHip()->*threeDofFn)(floatValue);
    if(m_character->isEffectorInMask(mask, m_Knee->getJointIndex()))
      (data->getKnee()->*oneDofFn)(floatValue);
    if(m_character->isEffectorInMask(mask, m_Ankle->getJointIndex()))
      (data->getAnkle()->*threeDofFn)(floatValue);
  }

  //
  // NmRsHumanSpine
  //---------------------------------------------------------------------------

  NmRsHumanSpine::NmRsHumanSpine() :
    NmRsLimb()
  {

  }

  NmRsHumanSpine::~NmRsHumanSpine()
  {

  }

  void NmRsHumanSpine::setup(NmRsCharacter* character,
                            NmRsHumanLimbTypes type,
                            NmRsEffectorBase * spine0,
                            NmRsEffectorBase * spine1,
                            NmRsEffectorBase * spine2,
                            NmRsEffectorBase * spine3,
                            NmRsEffectorBase * lowerNeck,
                            NmRsEffectorBase * upperNeck,
                            NmRsGenericPart * pelvis,
                            NmRsGenericPart * spine0Part,
                            NmRsGenericPart * spine1Part,
                            NmRsGenericPart * spine2Part,
                            NmRsGenericPart * spine3Part,
                            NmRsGenericPart * neck,
                            NmRsGenericPart * head )
  {
    Assert(character);
    m_character = character;

    Assert(type >= 0 && type < kNumNmRsHumanLimbs);
    m_type = type;
    Assert(spine0->is3DofEffector());
    m_Spine0 = (NmRs3DofEffector*)spine0;
    Assert(spine1->is3DofEffector());
    m_Spine1 = (NmRs3DofEffector*)spine1;
    Assert(spine2->is3DofEffector());
    m_Spine2 = (NmRs3DofEffector*)spine2;
    Assert(spine3->is3DofEffector());
    m_Spine3 = (NmRs3DofEffector*)spine3;
    Assert(lowerNeck->is3DofEffector());
    m_LowerNeck = (NmRs3DofEffector*)lowerNeck;
    Assert(upperNeck->is3DofEffector());
    m_UpperNeck = (NmRs3DofEffector*)upperNeck;

    // Build a mask of all the effectors in this limb.
    m_allEffectorMask = bvmask_None;
    m_allEffectorMask |= m_character->partToMask(spine0Part->getPartIndex());
    m_allEffectorMask |= m_character->partToMask(spine1Part->getPartIndex());
    m_allEffectorMask |= m_character->partToMask(spine2Part->getPartIndex());
    m_allEffectorMask |= m_character->partToMask(spine3Part->getPartIndex());
    m_allEffectorMask |= m_character->partToMask(neck->getPartIndex());
    m_allEffectorMask |= m_character->partToMask(head->getPartIndex());

    m_pelvis = pelvis;
    m_Spine0Part = spine0Part;
    m_Spine1Part = spine1Part;
    m_Spine2Part = spine2Part;
    m_Spine3Part = spine3Part;
    m_neck = neck;
    m_head = head;
  }

  void NmRsHumanSpine::tick(float timeStep)
  {

    if(m_input.size() == 0)
      return;

#if ART_ENABLE_BSPY
    bspyProfileStart("NmRsHumanSpine::tick");
#endif

    // sort inputs by weight priority
    m_input.sort();

#if ART_ENABLE_BSPY_LIMBS
    // Send messages, in order, for bSpy viewing.
    for(unsigned int i = 0; i < m_input.size(); ++i)
      sendMessageDebug(m_input.get(i));
#endif //ART_ENABLE_BSPY_LIMBS

    // Set up blendable target structure.
    m_blendTarget.init();

    // step down through the list of inputs.
    NmRsSpineInputWrapper* pCurrent = 0;
    NmRsSpineInputWrapper  current;

    for(unsigned int i = 0; i < m_input.size(); ++i)
    {
      NmRsLimbInput* it = m_input.get(i);

      if(it->type == kSpinePose)
      {
        pCurrent = it->getData<NmRsSpineInputWrapper>();
      }
      else if(it->type == kSetStiffness)
      {
        NmRsSetStiffnessInputWrapper* inputData = it->getData<NmRsSetStiffnessInputWrapper>();
        Assert(inputData);
        setStiffness(&current, inputData->m_stiffness, inputData->m_damping, it->mask, inputData->m_applyMuscleStiffness ? &inputData->m_muscleStiffness : 0);
        pCurrent = &current;
      }
      else if(it->type == kStopAll)
      {
        stopAll(&current);
        pCurrent = &current;
      }
      else
      {
        Assert(false);
        // todo support pose from ITMs
        // todo support some kind of IK?
        // todo support setBackAngles as a separate message?
      }

      // blend into the temporary limb input.
      Assert(pCurrent);
      bool done = blend(pCurrent, it->weight, it->mask DEBUG_LIMBS_PARAMETER(it->task) DEBUG_LIMBS_PARAMETER(it->subTask));

      // if not blending (or blend depth reached) and nothing is left to set, we're done!
      if(done)
      {
        break;
      }
    }

    // send output debug
#if ART_ENABLE_BSPY_LIMBS
    sendOutputDebug(static_cast<NmRsInputWrapperBase*>(&m_blendTarget));
#endif

    // write to effectors from temporary pose data
    set3DofParameters(m_Spine0, m_blendTarget.getSpine0Blendable());
    set3DofParameters(m_Spine1, m_blendTarget.getSpine1Blendable());
    set3DofParameters(m_Spine2, m_blendTarget.getSpine2Blendable());
    set3DofParameters(m_Spine3, m_blendTarget.getSpine3Blendable());
    set3DofParameters(m_LowerNeck, m_blendTarget.getLowerNeckBlendable());
    set3DofParameters(m_UpperNeck, m_blendTarget.getUpperNeckBlendable());

    // tick base to clear input vector, etc.
    NmRsLimb::tick(timeStep);

#if ART_ENABLE_BSPY
    bspyProfileEnd("NmRsHumanSpine::tick");
#endif
  }

#if ART_ENABLE_BSPY_LIMBS
  bool NmRsHumanSpine::blend(NmRsSpineInputWrapper* input, float weight, BehaviourMask mask, BehaviourID task, const char* subTask)
#else
  bool NmRsHumanSpine::blend(NmRsSpineInputWrapper* input, float weight, BehaviourMask mask)
#endif
  {
    // Input must be a spine pose. Possibly expand this function to handle other
    // kinds of arm inputs (setStiffness, etc) if it will speed up the tick.
    Assert(input);

    bool result = true;

    // Call each blendable effector blend function. e.g:
    //   if(mask == bvmask_Full || m_character->isEffectorInMask(mask, getSpine0()->getJointIndex()))\
    //     result = m_blendTarget.getSpine0Blendable()->blend(input.m_data.m_Spine0, task, weight) && result;
    #define NM_RS_PARAMETER(_prefix, _name) BLEND_EFFECTOR(_prefix, _name)
    #include "common\NmRsHumanSpine.inl"
    #undef NM_RS_PARAMETER

    return result;
  }

  void NmRsHumanSpine::setToCurrent(NmRsSpineInputWrapper* data, BehaviourMask mask /*= bvmask_Full*/)
  {
#define NM_RS_PARAMETER(_type, _name) SET_CURRENT_MASKED(_name)
#include "common\NmRsHumanSpine.inl"
#undef NM_RS_PARAMETER
  }

  void NmRsHumanSpine::setBodyStiffness(NmRsLimbInput& input, float stiffness, float damping, BehaviourMask mask /*= bvmask_Full*/, float *muscleStiffness /*= 0*/) const
  {
    NmRsSpineInputWrapper* data = input.getData<NmRsSpineInputWrapper>();
    setStiffness(data, stiffness, damping, mask, muscleStiffness);
  }

  void NmRsHumanSpine::setStiffness(NmRsSpineInputWrapper* data, float stiffness, float damping, BehaviourMask mask /*= bvmask_Full*/, float *muscleStiffness /*= 0*/) const
  {
    #define NM_RS_PARAMETER(_type, _name) BODYSTIFFNESS_MASKED(_name)
    #include "common\NmRsHumanSpine.inl"
    #undef NM_RS_PARAMETER
  }

void NmRsHumanSpine::setBodyStiffnessScaling(float stiffnessScale, float dampingScale, float muscleStiffnessScale, BehaviourMask mask /*= bvmask_Full*/) const
{
  float strengthScale = stiffnessScale * stiffnessScale;
  float dampScale = stiffnessScale * dampingScale;

  // Auto-generate parameter initialization from inline file.
  #define NM_RS_PARAMETER(_type, _name) BODYSTIFFNESSSCALING_MASKED(_name)
  #include "common\NmRsHumanSpine.inl"
  #undef NM_RS_PARAMETER
}

void NmRsHumanSpine::setRelaxation(NmRsLimbInput& input, float mult, BehaviourMask mask, float *pMultDamping /*= 0*/) const
{
  NmRsSpineInputWrapper* data = input.getData<NmRsSpineInputWrapper>();
  #define NM_RS_PARAMETER(_type, _name) SETRELAXATION_MASKED(_name)
  #include "common\NmRsHumanSpine.inl"
  #undef NM_RS_PARAMETER
}

void NmRsHumanSpine::activePose(NmRsLimbInput& input, int transformSource, BehaviourMask mask /*= bvmask_Full*/) const
{
  NmRsSpineInputWrapper* data = input.getData<NmRsSpineInputWrapper>();
  #define NM_RS_PARAMETER(_type, _name) ACTIVE_POSE_MASKED(_name)
  #include "common\NmRsHumanSpine.inl"
  #undef NM_RS_PARAMETER
}

void NmRsHumanSpine::blendToZeroPose(NmRsLimbInput& input, float t, BehaviourMask mask /*= bvmask_Full*/) const
{
  NmRsSpineInputWrapper* data = input.getData<NmRsSpineInputWrapper>();
  #define NM_RS_PARAMETER(_type, _name) BLENDTOZERO_MASKED(_name)
  #include "common\NmRsHumanSpine.inl"
  #undef NM_RS_PARAMETER
}

void NmRsHumanSpine::resetEffectors(NmRsLimbInput& input, ResetEffectorsType type, BehaviourMask mask /*= bvmask_Full*/, float scale /*= 1.0*/) const
{
  NmRsSpineInputWrapper* data = input.getData<NmRsSpineInputWrapper>();
  #define NM_RS_PARAMETER(_type, _name) RESET_EFFECTORS_MASKED(_name)
  #include "common\NmRsHumanSpine.inl"
  #undef NM_RS_PARAMETER
}

void NmRsHumanSpine::setOpposeGravity(NmRsLimbInput& input, float oppose, BehaviourMask mask /*= bvmask_Full*/) const
{
  NmRsSpineInputWrapper* data = input.getData<NmRsSpineInputWrapper>();
  #define NM_RS_PARAMETER(_type, _name) OPPOSE_GRAVITY_MASKED(_name)
  #include "common\NmRsHumanSpine.inl"
  #undef NM_RS_PARAMETER
}

void NmRsHumanSpine::holdPose(NmRsLimbInput& input, BehaviourMask mask /*= bvmask_Full*/) const
{
  NmRsSpineInputWrapper* data = input.getData<NmRsSpineInputWrapper>();
  #define NM_RS_PARAMETER(_type, _name) HOLD_POSE_MASKED(_name)
  #include "common\NmRsHumanSpine.inl"
  #undef NM_RS_PARAMETER
}

  void NmRsHumanSpine::stopAll(NmRsSpineInputWrapper* data, BehaviourMask mask /*= bvmask_Full*/) const
  {
    const ResetEffectorsType type = kResetCalibrations | kResetAngles;
    const float scale = 1.0f;
#define NM_RS_PARAMETER(_type, _name) RESET_EFFECTORS_MASKED(_name)
#include "common\NmRsHumanSpine.inl"
#undef NM_RS_PARAMETER

#define NM_RS_PARAMETER(_type, _name) HOLD_POSE_MASKED(_name)
#include "common\NmRsHumanSpine.inl"
#undef NM_RS_PARAMETER

    setStiffness(data, 5.0f, 0.5f, mask);
  }

void NmRsHumanSpine::callMaskedEffectorFunctionFloatArg(
  NmRsLimbInput& input,
  BehaviourMask mask,
  float floatValue,
  Effector1DofDataFuncFloatArg /*oneDofFn*/,
  Effector3DofDataFuncFloatArg threeDofFn) const
{
  NmRsSpineInputWrapper* data = input.getData<NmRsSpineInputWrapper>();
  if(m_character->isEffectorInMask(mask, m_Spine0->getJointIndex()))
    (data->getSpine0()->*threeDofFn)(floatValue);
  if(m_character->isEffectorInMask(mask, m_Spine1->getJointIndex()))
    (data->getSpine1()->*threeDofFn)(floatValue);
  if(m_character->isEffectorInMask(mask, m_Spine2->getJointIndex()))
    (data->getSpine2()->*threeDofFn)(floatValue);
  if(m_character->isEffectorInMask(mask, m_Spine3->getJointIndex()))
    (data->getSpine3()->*threeDofFn)(floatValue);
  if(m_character->isEffectorInMask(mask, m_LowerNeck->getJointIndex()))
    (data->getLowerNeck()->*threeDofFn)(floatValue);
  if(m_character->isEffectorInMask(mask, m_UpperNeck->getJointIndex()))
    (data->getUpperNeck()->*threeDofFn)(floatValue);
}

  void NmRsHumanSpine::keepHeadAwayFromGround(NmRsLimbInput& input, float leanAmount, rage::Vector3 *direction)
  {
    NmRsSpineInputWrapper* data = input.getData<NmRsSpineInputWrapper>();

    rage::Vector3 dir;

    dir = m_character->m_gUp;
    if (direction)
      dir = *direction;

    rage::Vector3 axis;
    rage::Matrix34 TM;

    getNeckPart()->getBoundMatrix(&TM);

    axis = TM.c;
    float neckLean1 = axis.Dot(dir);

    axis = TM.a;
    float neckLean2 = axis.Dot(dir);

#if ART_ENABLE_BSPY & 0
    bspyScratchpad(m_character->getBSpyID(), "HeadAvoid", leanAmount);
    bspyScratchpad(m_character->getBSpyID(), "HeadAvoid", dir);
    bspyScratchpad(m_character->getBSpyID(), "HeadAvoid", TM.a);
    bspyScratchpad(m_character->getBSpyID(), "HeadAvoid", TM.b);
    bspyScratchpad(m_character->getBSpyID(), "HeadAvoid", TM.c);
    bspyScratchpad(m_character->getBSpyID(), "HeadAvoid", TM.d);
    bspyScratchpad(m_character->getBSpyID(), "HeadAvoid", neckLean1);
    bspyScratchpad(m_character->getBSpyID(), "HeadAvoid", neckLean2);
#endif

    data->getLowerNeck()->setDesiredLean1(-neckLean1*leanAmount);
    data->getLowerNeck()->setDesiredLean2(-neckLean2*leanAmount);
    data->getUpperNeck()->setDesiredLean1(-neckLean1*leanAmount);
    data->getUpperNeck()->setDesiredLean2(-neckLean2*leanAmount);
  }

  //
  // NmRsBody
  //
  //-----------------------------------------------------------------------------

#if ART_ENABLE_BSPY_LIMBS
  NmRsBody::NmRsBody(MemoryManager* services) :
    m_artMemoryManager(services)
#else
  NmRsBody::NmRsBody(MemoryManager* services) :
    m_artMemoryManager(services)
#endif
  {
    Assert(m_artMemoryManager);

    for(int i = 0; i < MAX_LIMBS; ++i)
    {
      m_allLimbs[i] = NULL;
    }

    m_bodyInput = NULL;
  }
  
  NmRsBody::~NmRsBody()
  {
    // TODO template function to make this cleaner?
    for(int i = 0; i < MAX_LIMBS; ++i)
    {
      if(m_allLimbs[i] != NULL)
      {
        if(m_allLimbs[i]->getType() == kLeftArm || m_allLimbs[i]->getType() == kRightArm)
          ARTCustomPlacementDelete(static_cast<NmRsHumanArm*>(m_allLimbs[i]), NmRsHumanArm);
        else if (m_allLimbs[i]->getType() == kLeftLeg || m_allLimbs[i]->getType() == kRightLeg)
          ARTCustomPlacementDelete(static_cast<NmRsHumanLeg*>(m_allLimbs[i]), NmRsHumanLeg);
        else if (m_allLimbs[i]->getType() == kSpine)
          ARTCustomPlacementDelete(static_cast<NmRsHumanSpine*>(m_allLimbs[i]), NmRsHumanSpine);
        else
          Assert(false);
          m_allLimbs[i] = 0;
      }
    }
  }

  void NmRsBody::postLimbInputs()
  {
    LimbIterator it(*this);
    while (!it.finished())
    {
      if(it.currentLimbInput(false).valid)
      {
        it.currentLimb()->postInput(it.currentLimbInput());
        it.currentLimbInput().valid = false;
      }
      it.next();
    }

    // Pop previous body input set.
#if ART_ENABLE_BSPY & 0
    bspyLogf(info, L"postLimbInputs");
#endif
    m_bodyInput = m_character->getLimbManager()->popBodyState();
  }

  void NmRsBody::init(NmRsCharacter* character)
  {
    Assert(character);
    m_character = character;
    m_bodyInput = NULL;
  }

  void NmRsBody::initAllLimbs()
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.next())
    {
      it.currentLimb()->init();
    }
  }

  void NmRsBody::clearQueueAllLimbs()
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.next())
    {
      it.currentLimb()->clearQueue();
    }
  }

#if ART_ENABLE_BSPY_LIMBS
  void NmRsBody::setup(BehaviourID bvid, int priority, int subPriority, float blend, BehaviourMask mask /*= bvmask_Full*/, const char* subTask /*= 0*/)
  {
#if ART_ENABLE_BSPY & 0
    bspyLogf(info, L"setup(%d)", bvid);
#endif
    m_bodyInput = m_character->getLimbManager()->pushBodyState();

    Assert(m_bodyInput);
    m_bodyInput->m_bvid = bvid;
    m_bodyInput->m_priority = priority;
    m_bodyInput->m_subPriority = subPriority;
    m_bodyInput->m_mask = mask;
    m_bodyInput->m_blend = blend;
    m_bodyInput->m_subTask = subTask;
  }
#else
  void NmRsBody::setup(BehaviourID bvid, int priority, int subPriority, float blend, BehaviourMask mask /*= bvmask_Full*/)
  {
    m_bodyInput = m_character->getLimbManager()->pushBodyState();
    Assert(m_bodyInput);
    m_bodyInput->m_bvid = bvid;
    m_bodyInput->m_priority = priority;
    m_bodyInput->m_subPriority = subPriority;
    m_bodyInput->m_mask = mask;
    m_bodyInput->m_blend = blend;
  }
#endif

  void NmRsBody::tick(float timeStep)
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.next())
    {
      it.currentLimb()->tick(timeStep);
    }
  }

  // limbs todo, these three might make things faster if they're in the header.
  NmRsLimb* NmRsBody::getLimb(NmRsHumanLimbTypes limbType)
  {
    Assert((limbType >= 0) && (limbType < kNumNmRsHumanLimbs));
    Assert(m_allLimbs[limbType] != NULL);
    return m_allLimbs[limbType];
  }

  const NmRsLimb* NmRsBody::getLimb(NmRsHumanLimbTypes limbType) const
  {
    Assert((limbType >= 0) && (limbType < kNumNmRsHumanLimbs));
    Assert(m_allLimbs[limbType] != NULL);
    return m_allLimbs[limbType];
  }

  NmRsLimbInput& NmRsBody::getInputNoSetup(NmRsHumanLimbTypes limbType)
  {
    Assert(m_bodyInput);
    Assert((limbType >= 0) && (limbType < kNumNmRsHumanLimbs));
    return m_bodyInput->m_allLimbInputs[limbType];
  }

  //
  // whole body functions
  //---------------------------------------------------------------------------

  void NmRsBody::setStiffness(float stiffness, float damping, BehaviourMask mask /*= 0*/, float *muscleStiffness /*= NULL*/, bool queued /*= false*/, int subPriority /* = 0 */)
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.nextWithMask(mask))
    {
      if(queued)
      {
        NmRsLimbInput input = createNmRsLimbInput<NmRsSetStiffnessInputWrapper>(subPriority, 1.0f, mask);
        NmRsSetStiffnessInputWrapper* inputData = input.getData<NmRsSetStiffnessInputWrapper>();
        inputData->setValues(stiffness, damping, muscleStiffness);
        it.currentLimb()->postInput(input);
      }
      else
      {
        it.currentLimb()->setBodyStiffness(it.currentLimbInput(), stiffness, damping, mask, muscleStiffness);
      }
    }
  }

  void NmRsBody::stopAllBehaviours()
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.next())
    {
      NmRsLimbInput input = createNmRsLimbInput<NmRsStopAllInputWrapper>();
      it.currentLimb()->postInput(input);
    }
  }

  void NmRsBody::setStiffnessScaling(float stiffnessScale, float dampingScale, float muscleStiffnessScale, BehaviourMask mask /*= bvmask_Full*/)
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.nextWithMask(mask))
    {
      it.currentLimb()->setBodyStiffnessScaling(stiffnessScale, dampingScale, muscleStiffnessScale, mask);
    }
  }

  void NmRsBody::setRelaxation(float mult, BehaviourMask mask /*= bvmask_Full*/, float *pMultDamping /*= 0*/)
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.nextWithMask(mask))
    {
      it.currentLimb()->setRelaxation(it.currentLimbInput(), mult, mask, pMultDamping);
    }
  }

  void NmRsBody::activePose(int transformSource, BehaviourMask mask /*= bvmask_Full*/)
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.nextWithMask(mask))
    {
      it.currentLimb()->activePose(it.currentLimbInput(), transformSource, mask);
    }
  }

  void NmRsBody:: holdPose(BehaviourMask mask /*= bvmask_Full*/)
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.nextWithMask(mask))
    {
      it.currentLimb()->holdPose(it.currentLimbInput(), mask);
    }
  }

  void NmRsBody::blendToZeroPose(float blend, BehaviourMask mask /*= bvmask_Full*/)
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.nextWithMask(mask))
    {
      it.currentLimb()->blendToZeroPose(it.currentLimbInput(), blend, mask);
    }
  }

  void NmRsBody::setOpposeGravity(float oppose, BehaviourMask mask /*= bvmask_Full*/)
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.nextWithMask(mask))
    {
      it.currentLimb()->setOpposeGravity(it.currentLimbInput(), oppose, mask);
    }
  }

  void NmRsBody::resetEffectors(ResetEffectorsType resetType, BehaviourMask mask /*= bvmask_Full*/, float scale /*= 1.0*/)
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.nextWithMask(mask))
    {
      it.currentLimb()->resetEffectors(it.currentLimbInput(), resetType, mask, scale);
    }
  }

  void NmRsBody::callMaskedEffectorDataFunctionFloatArg(
    BehaviourMask mask,
    float floatValue,
    Effector1DofDataFuncFloatArg oneDofFn,
    Effector3DofDataFuncFloatArg threeDofFn)
  {
    for (NmRsBody::LimbIterator it(*this); !it.finished(); it.nextWithMask(mask))
    {
      it.currentLimb()->callMaskedEffectorFunctionFloatArg(it.currentLimbInput(), mask, floatValue, oneDofFn, threeDofFn);
    }
  }

  //
  // NmRsHumanBody
  //
  //-----------------------------------------------------------------------------

  NmRsHumanBody::NmRsHumanBody(MemoryManager* services) :
    NmRsBody(services)
  {
  }

#if ART_ENABLE_BSPY_LIMBS
  void NmRsHumanBody::setup(BehaviourID bvid, int priority, int subPriority, float blend, BehaviourMask mask /*= bvmask_Full*/, const char* subTask /*= 0*/)
  {
    NmRsBody::setup(bvid, priority, subPriority, blend, mask, subTask);
  }
#if NM_UNUSED_CODE
  void NmRsHumanBody::setSubTask(const char* subTask)
  {
    Assert(m_bodyInput);
    m_bodyInput->m_subTask = subTask;
  }
#endif
#else
  void NmRsHumanBody::setup(BehaviourID bvid, int priority, int subPriority, float blend, BehaviourMask mask /*= bvmask_Full*/)
  {
    NmRsBody::setup(bvid, priority, subPriority, blend, mask);
  }
#endif

  void NmRsHumanBody::addHumanArm(
    NmRsHumanLimbTypes type,
    NmRsEffectorBase * clavicle,
    NmRsEffectorBase * shoulder,
    NmRsEffectorBase * elbow,
    NmRsEffectorBase * wrist,
    NmRsGenericPart * spine3,
    NmRsGenericPart * claviclePart,
    NmRsGenericPart * upperArm,
    NmRsGenericPart * lowerArm,
    NmRsGenericPart * hand,
    float direction,
    float hingeDirection,
    bool twistIsFixed)
  {
    ARTCustomPlacementNewNoService(m_allLimbs[type], NmRsHumanArm)
    NmRsHumanArm* arm = (NmRsHumanArm*) m_allLimbs[type];
    arm->setup(
      m_character, type,
      clavicle, shoulder, elbow, wrist,
      spine3, claviclePart, upperArm, lowerArm, hand,
      direction, hingeDirection, twistIsFixed);
  }

  void NmRsHumanBody::addHumanLeg(
    NmRsHumanLimbTypes type,
    NmRsEffectorBase * hip,
    NmRsEffectorBase * knee,
    NmRsEffectorBase * ankle,
    NmRsGenericPart * pelvis,
    NmRsGenericPart * thigh,
    NmRsGenericPart * shin,
    NmRsGenericPart * foot,
    float direction,
    float hingeDirection,
    bool twistIsFixed)
  {
    ARTCustomPlacementNewNoService(m_allLimbs[type], NmRsHumanLeg)
    NmRsHumanLeg* leftLeg = (NmRsHumanLeg*) m_allLimbs[type];
    leftLeg->setup(
      m_character, type,
      hip, knee, ankle,
      pelvis, thigh, shin, foot,
      direction, hingeDirection, twistIsFixed);
  }

  void NmRsHumanBody::addHumanSpine(
    NmRsHumanLimbTypes type,
    NmRsEffectorBase * spine0,
    NmRsEffectorBase * spine1,
    NmRsEffectorBase * spine2,
    NmRsEffectorBase * spine3,
    NmRsEffectorBase * lowerNeck,
    NmRsEffectorBase * upperNeck,
    NmRsGenericPart * pelvis,
    NmRsGenericPart * spine0Part,
    NmRsGenericPart * spine1Part,
    NmRsGenericPart * spine2Part,
    NmRsGenericPart * spine3Part,
    NmRsGenericPart * neck,
    NmRsGenericPart * head )
  {
    ARTCustomPlacementNewNoService(m_allLimbs[type], NmRsHumanSpine)
    NmRsHumanSpine* spine = (NmRsHumanSpine*) m_allLimbs[type];
    spine->setup(
      m_character, type,
      spine0, spine1, spine2, spine3, lowerNeck, upperNeck,
      pelvis, spine0Part, spine1Part, spine2Part, spine3Part, neck, head );
  }


  // The conditionals in the function below allow get*Input() to be used in
  // the same way that the limb pointers were in the past. This minimized the
  // refactor burden for 90% of the behaviours. this could be done up front
  // by preparing the inputs we know the behaviour will need to use, but the
  // logic of many behaviours is such that we don't know right away and risk
  // wasting limbs system memory. we may be able to avoid this in the future
  // if phase 2 refactor allows us to be confident we know which inputs are
  // really in use.
  NmRsLimbInput& NmRsHumanBody::getInput(NmRsHumanLimbTypes limbType)
  {
    Assert(m_bodyInput);
    Assert((limbType >= kLeftArm) && (limbType <= kSpine));
    if (!m_bodyInput->m_allLimbInputs[limbType].valid)
    {
      switch (limbType)
      {
      case kLeftArm:
      case kRightArm:
        m_bodyInput->m_allLimbInputs[limbType] = createNmRsLimbInput<NmRsArmInputWrapper>(m_bodyInput->m_subPriority);
        break;

      case kLeftLeg:
      case kRightLeg:
        m_bodyInput->m_allLimbInputs[limbType] = createNmRsLimbInput<NmRsLegInputWrapper>(m_bodyInput->m_subPriority);
        break;

      case kSpine:
        m_bodyInput->m_allLimbInputs[limbType] = createNmRsLimbInput<NmRsSpineInputWrapper>(m_bodyInput->m_subPriority);
        break;

      default:
        Assert(false); // TODO better error logging
      }
    }
    return m_bodyInput->m_allLimbInputs[limbType];
  }

  NmRsArmInputWrapper* NmRsHumanBody::getLeftArmInputData()
  {
    return getInput(kLeftArm).getData<NmRsArmInputWrapper>();
  }

  NmRsArmInputWrapper* NmRsHumanBody::getRightArmInputData()
  {
    return getInput(kRightArm).getData<NmRsArmInputWrapper>();
  }

  NmRsLegInputWrapper* NmRsHumanBody::getLeftLegInputData()
  {
    return getInput(kLeftLeg).getData<NmRsLegInputWrapper>();
  }

  NmRsLegInputWrapper* NmRsHumanBody::getRightLegInputData()
  {
    return getInput(kRightLeg).getData<NmRsLegInputWrapper>();
  }

  NmRsSpineInputWrapper* NmRsHumanBody::getSpineInputData()
  {
    return getInput(kSpine).getData<NmRsSpineInputWrapper>();
  }



  NmRsBodyStateHelper::NmRsBodyStateHelper(NmRsBody* body) :
    m_body(body),
    m_level(0)
  {
    Assert(m_body);
  }

  NmRsBodyStateHelper::NmRsBodyStateHelper(NmRsBody* body, BehaviourID bvid, int priority, int subPriority, float blend, BehaviourMask mask /* = bvmask_Full */ SUBTASK_PARAM_NODEFAULT) :
    m_body(body),
    m_level(0)
  {
    Assert(m_body);
    pushState(bvid, priority, subPriority, blend, mask DEBUG_LIMBS_PARAMETER(subTask));
  }

  // Suports scoped body contexts.  When this object destructs, it ensures the
  // body state stack is returned to its original level.
  NmRsBodyStateHelper::~NmRsBodyStateHelper()
  {
    while(m_level > 0)
    {
      popState();
    }
  }

  void NmRsBodyStateHelper::pushState(BehaviourID bvid, int priority, int subPriority, float blend, BehaviourMask mask /* = bvmask_Full */ SUBTASK_PARAM_NODEFAULT)
  {
    m_body->setup(bvid, priority, subPriority, blend, mask DEBUG_LIMBS_PARAMETER(subTask));
    m_level++;
  }

  void NmRsBodyStateHelper::popState()
  {
    m_body->postLimbInputs();
    m_level--;
  }
} // namespace ART
