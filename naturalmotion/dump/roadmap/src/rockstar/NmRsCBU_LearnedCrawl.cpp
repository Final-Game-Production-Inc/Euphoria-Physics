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
#include "NmRsBodyLayout.h"
#include "NmRsCBU_LearnedCrawl.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"
#include "nmutils/TypeUtils.h"
#include "NmRsEngine.h"

namespace ART
{
  // crawl4, 1 legged crawl, new crawl, MP3 15 fps, MP3 30fps
  int leftHandStance[5] = {32, 20, 44, 36, 57};
  int leftHandSwing[5]  = {17, 45, 17, 21, 30};
  int leftLegStance[5]  = {20, 43, 25, 17, 30};
  int leftLegSwing[5]   = {48, 20, 50, 8, 1};

  int rightHandStance[5] = {12, 45, 17, 11, 10};
  int rightHandSwing[5]  = {46, 20, 44, 55, 80};
  int rightLegStance[5]  = {44, 20, 50, 48, 80};
  int rightLegSwing[5]   = {21, 43, 25, 31, 44};    

  NmRsCBULearnedCrawl::NmRsCBULearnedCrawl(ART::MemoryManager* services) : CBUTaskBase(services, bvid_learnedCrawl)
  {
    initialiseCustomVariables();
  }

  void NmRsCBULearnedCrawl::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
    
    m_ctmTransforms = NULL;
    m_ctmVels = NULL;
    m_bucket = NULL;
    m_bestSequence = NULL;
  }

  NmRsCBULearnedCrawl::~NmRsCBULearnedCrawl()
  {
    if (m_ctmTransforms)
      delete[] m_ctmTransforms;
    if (m_ctmVels)
      delete[] m_ctmVels;
    if (m_bucket)
      delete[] m_bucket;
    if (m_bestSequence)
      delete[] m_bestSequence;
  }

  const rage::Vector3 NmRsCBULearnedCrawl::getPosition(NMutils::NMMatrix4 matrix) 
  {
    return rage::Vector3(matrix[3][0], matrix[3][1], matrix[3][2]);
  }

  rage::Matrix34 NmRsCBULearnedCrawl::getRageMatrix(NMutils::NMMatrix4 matrix) 
  {
    rage::Matrix34 mat;
    mat.a.x = matrix[0][0]; mat.a.y = matrix[0][1]; mat.a.z = matrix[0][2]; 
    mat.b.x = matrix[1][0]; mat.b.y = matrix[1][1]; mat.b.z = matrix[1][2]; 
    mat.c.x = matrix[2][0]; mat.c.y = matrix[2][1]; mat.c.z = matrix[2][2]; 
    mat.d.x = matrix[3][0]; mat.d.y = matrix[3][1]; mat.d.z = matrix[3][2]; 
    return mat;
  } 

  void NmRsCBULearnedCrawl::init()
  {
    m_sequence = NULL;
    m_animIndex = 0; // default, but should be overwritten at activation anyway
    memset(bSupporting, 0, 4*sizeof(bool));
    memset(bNextSupporting, 0, 4*sizeof(bool));
  }

  void NmRsCBULearnedCrawl::loadDriveSequence()
  {
    // load up the CTM - the big list of transforms
#if defined(_XBOX)
    FILE *file = fopen("GAME:\\learnedCrawl.seq", "rb");        
#else
    FILE *file = fopen("C:\\learnedCrawlFromPC.seq", "rb");     
#endif
    Assert(file);//LearnedCrawlFile could not be opened
    if (file)        
    {    
      fseek(file, 0, SEEK_END);
      int fileSize = ftell(file);
      fseek(file, 0, SEEK_SET);
      m_numberOfFrames = fileSize / sizeof(Sequence);
      if (!m_sequence) 
        m_sequence = rage_new Sequence[m_numberOfFrames];

      // save out the desired joint angles:
      fread(m_sequence, sizeof(Sequence), m_numberOfFrames, file);
      fclose(file);
    }
  }

  void NmRsCBULearnedCrawl::enablePartFriction(NmRsGenericPart *part, bool enable)
  {
    //set enabled friction multiplier of part to be a linear function of its' parent joints' injury amount
    //MMMM this may need to be scaled differently e.g. try (1-injuryAmount)^2 below
    const NmRsEffectorBase* eff = m_character->getConstEffector(part->getPartIndex() - 1);
    float injuryAmount = eff->getInjuryAmount();
    float friction = 0.1f + (1 - injuryAmount) * 2.3f;  
    part->setFrictionMultiplier((enable ? friction : 0.1f)/(part->getBound()->GetMaterial(0).GetFriction()));
  }

  void NmRsCBULearnedCrawl::changeFriction(bool *supporting)
  {
    enablePartFriction(getLeftArm()->getUpperArm(), supporting[lLeftArm]);
    enablePartFriction(getLeftArm()->getLowerArm(), supporting[lLeftArm]);
    enablePartFriction(getLeftArm()->getHand(),     supporting[lLeftArm]);
    enablePartFriction(getRightArm()->getUpperArm(),supporting[lRightArm]);
    enablePartFriction(getRightArm()->getLowerArm(),supporting[lRightArm]);
    enablePartFriction(getRightArm()->getHand(),    supporting[lRightArm]);
    enablePartFriction(getLeftLeg()->getThigh(),    supporting[lLeftLeg]);
    enablePartFriction(getLeftLeg()->getShin(),     supporting[lLeftLeg]);
    enablePartFriction(getLeftLeg()->getFoot(),     supporting[lLeftLeg]);
    enablePartFriction(getRightLeg()->getThigh(),   supporting[lRightLeg]);
    enablePartFriction(getRightLeg()->getShin(),    supporting[lRightLeg]);
    enablePartFriction(getRightLeg()->getFoot(),    supporting[lRightLeg]);
  }

  void NmRsCBULearnedCrawl::setPartFrictionMultiplier(NmRsGenericPart *part, float frictionM)
  {
    part->setFrictionMultiplier(frictionM);
  }

  void NmRsCBULearnedCrawl::resetFriction()
  {
    setPartFrictionMultiplier(getLeftArm()->getUpperArm(), 1.f);
    setPartFrictionMultiplier(getLeftArm()->getLowerArm(), 1.f);
    setPartFrictionMultiplier(getLeftArm()->getHand(),     1.f);
    setPartFrictionMultiplier(getRightArm()->getUpperArm(),1.f);
    setPartFrictionMultiplier(getRightArm()->getLowerArm(),1.f);
    setPartFrictionMultiplier(getRightArm()->getHand(),    1.f);
    setPartFrictionMultiplier(getLeftLeg()->getThigh(),    1.f);
    setPartFrictionMultiplier(getLeftLeg()->getShin(),     1.f);
    setPartFrictionMultiplier(getLeftLeg()->getFoot(),     1.f);
    setPartFrictionMultiplier(getRightLeg()->getThigh(),   1.f);
    setPartFrictionMultiplier(getRightLeg()->getShin(),    1.f);
    setPartFrictionMultiplier(getRightLeg()->getFoot(),    1.f);
  }

  bool NmRsCBULearnedCrawl::setSupportingLimbs(bool *supporting, int frame)
  {
    bool change = false;
    bool leftLeg = frame >= leftLegStance[m_animIndex] && frame < leftLegSwing[m_animIndex];
    if (leftLegSwing[m_animIndex] < leftLegStance[m_animIndex])
      leftLeg = frame >= leftLegStance[m_animIndex] || frame < leftLegSwing[m_animIndex];
    if (supporting[lLeftLeg] != leftLeg)
    {
      supporting[lLeftLeg] = leftLeg; 
      change = true;
    }
    bool rightLeg = frame >= rightLegStance[m_animIndex] && frame < rightLegSwing[m_animIndex];
    if (rightLegSwing[m_animIndex] < rightLegStance[m_animIndex])
      rightLeg = frame >= rightLegStance[m_animIndex] || frame < rightLegSwing[m_animIndex];
    if (supporting[lRightLeg] != rightLeg)
    {
      supporting[lRightLeg] = rightLeg; 
      change = true;
    }
    bool leftArm = frame >= leftHandStance[m_animIndex] && frame < leftHandSwing[m_animIndex];
    if (leftHandSwing[m_animIndex] < leftHandStance[m_animIndex])
      leftArm = frame >= leftHandStance[m_animIndex] || frame < leftHandSwing[m_animIndex];
    if (supporting[lLeftArm] != leftArm)
    {
      supporting[lLeftArm] = leftArm; 
      change = true;
    }
    bool rightArm = frame >= rightHandStance[m_animIndex] && frame < rightHandSwing[m_animIndex];
    if (rightHandSwing[m_animIndex] < rightHandStance[m_animIndex])
      rightArm = frame >= rightHandStance[m_animIndex] || frame < rightHandSwing[m_animIndex];
    if (supporting[lRightArm] != rightArm)
    {
      supporting[lRightArm] = rightArm;
      change = true;
    }
    return change; 
  }

  // this is in-game running of a pre-learned sequence
  void NmRsCBULearnedCrawl::setupCharacter()
  {
    m_frameIndex = 0;
    m_frameValue = 0.f;
    m_totalFrame = 0;

    // get heading direction
    rage::Vector3 direction = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
    if(m_character->m_gUp.y > 0.9f)
      m_yawOffset = atan2f(-direction.x, -direction.z); // y is up
    else
      m_yawOffset = atan2f(-direction.x, -direction.y); // z is up

    setSupportingLimbs(bSupporting, 0);
    setSupportingLimbs(bNextSupporting, 1);
    m_character->m_posture.init(); // make sure this is set up correctly
    m_character->m_posture.useZMP = false;
  }

  void NmRsCBULearnedCrawl::onActivate()
  {
    Assert(m_character);
#ifdef NM_RS_CBU_ASYNCH_PROBES
    m_character->InitializeProbe(NmRsCharacter::pi_learnedCrawl);
#endif //NM_RS_CBU_ASYNCH_PROBES

    comPos = m_character->m_COM;
    m_distTravelled = -1.f;

    // moved here from init, which is called before parameters are available, so wouldn't work in runtime
    m_animIndex = m_parameters.animIndex;
    setSupportingLimbs(bSupporting, 0);
    setSupportingLimbs(bNextSupporting, 1);

#if CRAWL_LEARNING
    m_character->setLearningCrawl(m_parameters.bLearn);

    if (m_parameters.bLearn)
    {
      // do we want to go through animPose once and save the transforms into a .ctm file ...?
      if(m_parameters.learnFromAnimPlayback)
      {
        m_out = NULL;
#if defined(_XBOX)
        m_out = fopen("GAME:\\Crawl.ctm", "wb");     
#else
        m_out = fopen("c:\\CrawlFromPC.ctm", "wb");     
#endif
        m_frameNumber = 0;
        m_numberOfFrames = m_parameters.numFrames2Learn;
        m_ctmTransforms = rage_new NMutils::NMMatrix4[m_numberOfFrames*21];
        m_ctmVels = rage_new rage::Vector3[m_numberOfFrames*21];
        m_bucket = rage_new Bucket[m_numberOfFrames];
        m_sequence = rage_new Sequence[m_numberOfFrames];
        m_bestSequence = rage_new Sequence[m_numberOfFrames];
        memset(m_bucket, 0, sizeof(Bucket)*m_numberOfFrames);
        memset(m_sequence, 0, sizeof(Sequence)*m_numberOfFrames);
        m_count = 30.f;
        m_bestTotalError = 0.f;
      }
      // ... or do we use an already existing .ctm file?
      else
      {
        loadCTM();
        buildBuckets();
      }
    }
    // if we won't be learning, but just playing back learned sequence
    else
#endif
    {
      // use provided sequence file...
      if(m_parameters.inputSequence != NULL && m_parameters.inputSequenceSize != 0)
      {
        m_sequence = (Sequence *)m_parameters.inputSequence;
        m_numberOfFrames = m_parameters.inputSequenceSize / sizeof(Sequence);
      }
      // ...or load from "GAME:\\learnedCrawl.seq"
      else
      {
        loadDriveSequence();
      }
    }
    m_character->disableSelfCollision();
    setupCharacter();    
  }

  void NmRsCBULearnedCrawl::onDeactivate()
  {
    Assert(m_character);

    m_character->m_posture.useZMP = true;

    resetFriction();
#if CRAWL_LEARNING
    m_character->setLearningCrawl(false);
#endif
#ifdef NM_RS_CBU_ASYNCH_PROBES
    m_character->ClearAsynchProbe_IfNotInUse(NmRsCharacter::pi_learnedCrawl);
#endif


  }
  rage::Vector3 NmRsCBULearnedCrawl::getIKTarget(NmRsGenericPart* claviclePart, Limbs limb)
  {
    float blend2 = fmodf(m_frameValue, 1.f); // we linearly interpolate the sequence for varying timestep
    float blend1 = 1.f - blend2;

    rage::Vector3 targetPos;

    rage::Vector3 pos1 = m_sequence[m_frameIndex].m_ik[limb].target;
    rage::Vector3 pos2 = m_sequence[(m_frameIndex+1)%m_numberOfFrames].m_ik[limb].target;
    targetPos = pos1*blend1 + pos2*blend2; // first interpolate the target

    rage::Matrix34 clavMatrix;
    claviclePart->getMatrix(clavMatrix);
    if(m_character->m_gUp.y > 0.9f)
      targetPos.RotateY(m_yawOffset); // this is y up
    else
      targetPos.RotateZ(m_yawOffset); // this is z up

    if(m_parameters.useSpine3Thing)
    {
      clavMatrix.d = getSpine()->getSpine3Part()->getPosition();
      // clavMatrix.d.y = claviclePart->getPosition().y; // Mod: not gUP safe
      m_character->levelVector(clavMatrix.d, m_character->vectorHeight(claviclePart->getPosition()));
    }

    // Interpolating ik positions is harder because it changes space.
    // so I must interpolate the space as well (the offset vector)
    rage::Vector3 offset1 = clavMatrix.d;
    rage::Vector3 offset2 = clavMatrix.d;
    if (!bSupporting[limb] || !bNextSupporting[limb])
    {
      rage::Vector3 probeUp = 2.0f*m_character->m_gUp;
      bool bHit = m_character->probeRay(NmRsCharacter::pi_learnedCrawl, offset1, offset1 - probeUp, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);
      if (bHit)
      {          
        if (!bSupporting[limb])
        {
          m_character->levelVector(offset1, m_character->vectorHeight(m_character->m_probeHitPos[NmRsCharacter::pi_learnedCrawl]));
        }
        if (!bNextSupporting[limb])
        {
          m_character->levelVector(offset2, m_character->vectorHeight(m_character->m_probeHitPos[NmRsCharacter::pi_learnedCrawl]));
        }
      }
    }
    targetPos += offset1*blend1 + offset2*blend2;
    return targetPos;
  }

  void NmRsCBULearnedCrawl::addSpineLean(float rotateVel)
  {
    float lean = rotateVel * 0.5f;

    // todo check whether the getDesireds are intended to be internal or external
    getSpineInputData()->getSpine0()->setDesiredLean2(getSpineInputData()->getSpine0()->getDesiredLean2() + lean);
    getSpineInputData()->getSpine1()->setDesiredLean2(getSpineInputData()->getSpine1()->getDesiredLean2() + lean);
    getSpineInputData()->getSpine2()->setDesiredLean2(getSpineInputData()->getSpine2()->getDesiredLean2() + lean);
    getSpineInputData()->getSpine3()->setDesiredLean2(getSpineInputData()->getSpine3()->getDesiredLean2() + lean);
    getSpineInputData()->getLowerNeck()->setDesiredLean2(getSpineInputData()->getLowerNeck()->getDesiredLean2() + lean*2.f);
    getSpineInputData()->getUpperNeck()->setDesiredLean2(getSpineInputData()->getUpperNeck()->getDesiredLean2() + lean*2.f);
  }

  CBUTaskReturn NmRsCBULearnedCrawl::onTick(float timeStep)
  {
    float scale = sqrtf(0.5f*(1.f + m_parameters.speed)); // TDL this scales the muscle strength with the speed a bit, to give a bigger range of sensible looking speeds
    for (int j = 0; j<20; j++)
    {
      float stiffness = m_parameters.stiffness*scale; // TDL 10 seems nice and natural.
      if (j>=getLeftArm()->getClavicle()->getJointIndex() && j<=getLeftArm()->getWrist()->getJointIndex())
        stiffness *= 17.f/12.f;
      if (j>=getRightArm()->getClavicle()->getJointIndex() && j<=getRightArm()->getWrist()->getJointIndex())
        stiffness *= 17.f/12.f;
      m_character->getEffectorDirect(j)->setMuscleStrength(stiffness*stiffness);
      m_character->getEffectorDirect(j)->setMuscleDamping(2.f*stiffness*m_parameters.damping);
    }

    setSupportingLimbs(bSupporting, m_frameIndex);
    if (setSupportingLimbs(bNextSupporting, (m_frameIndex+1)%m_numberOfFrames))
      changeFriction(bNextSupporting);

#if CRAWL_LEARNING
    if(m_parameters.bLearn) // do learning
    {
      // This is in conjunction with an animation playback. We run through exactly one anim cycle,
      // make a list of animation transforms from IncomingTransforms, and save it to *.ctm file
      if(m_parameters.learnFromAnimPlayback)
      {  
        m_frameNumber += 1;
        if (m_frameNumber <= m_parameters.numFrames2Learn)
        {  
          NMutils::NMMatrix4 *matrices;
          IncomingTransformStatus status = kITSNone;
          int numParts = NUM_PARTS;
          m_character->getIncomingTransforms(&matrices, status, numParts, kITSourceCurrent);
          Assert(numParts == NUM_PARTS); // assert also if status wrong
          //m_ctmTransforms[(m_frameNumber-1)*NUM_PARTS + i] = matrices;
          if (m_out)        
          {    
            // save out the desired joint angles:
            fwrite(matrices, sizeof(NMutils::NMMatrix4), NUM_PARTS, m_out);
          }
        }
        else if (m_frameNumber == m_parameters.numFrames2Learn + 1)
        {
          fclose(m_out);
          loadCTM();
          buildBuckets();
        }
        else
        {
          //After the first anim cycle, start learning
          learnDriveSequence();
        }
      }
      // if we're not saving out ctm file initially (but have loaded it at activation), start learning right away
      else
      {
        learnDriveSequence();
      }
    }

    // if we're not learning, but only playing back the learned sequence
    else
#endif //CRAWL_LEARNING
    { 
      float rotateScale = 4.f;
      float maxRotateSpeed = 0.15f;
      float rotateVel = 0.f;
      if (m_parameters.targetPosition != rage::Vector3(0,0,0))
      {
        rage::Vector3 toTarget = m_parameters.targetPosition - getSpine()->getSpine3Part()->getPosition();
        // toTarget.y = 0.f; // Mod: not gUP safe
        m_character->levelVector(toTarget);
        // arrived at target ?
        if (toTarget.Mag() < 0.5f)
          m_character->sendFeedbackSuccess(NMLearnedCrawlFeedbackName);

        // current heading in world space
        float yawOffset;
        if(m_character->m_gUp.y > 0.9f)
          yawOffset = atan2f(-toTarget.x, -toTarget.z); // y up
        else
          yawOffset = atan2f(-toTarget.x, -toTarget.y); // z up

        float yawDelta = yawOffset - m_yawOffset;
        if (yawDelta > PI)
          yawDelta -= 2.f*PI;
        if (yawDelta < -PI)
          yawDelta += 2.f*PI;
        rotateVel = rage::Clamp(rotateScale * yawDelta, -maxRotateSpeed, maxRotateSpeed);
        m_yawOffset += rotateVel * timeStep;
      }
      else
        m_yawOffset = m_parameters.yawOffset;

      // lying on back ?
      //if (m_character->m_COMTM.c.y < -0.5f) // Mod: not gUP safe
      // COM TM's z vector points out the character's back, hence:
      if(m_character->vectorHeight(m_character->m_COMTM.c) < 0)
        m_character->sendFeedbackFailure(NMLearnedCrawlFeedbackName);

      float blend2 = fmodf(m_frameValue, 1.f); // we linearly interpolate the sequence for varying timestep
      float blend1 = 1.f - blend2;
      NmRsCBULearnedCrawl::Sequence::EffectorTarget *target1 = m_sequence[(int)m_frameIndex].m_effector;
      NmRsCBULearnedCrawl::Sequence::EffectorTarget *target2 = m_sequence[((int)m_frameIndex+1)%m_numberOfFrames].m_effector;
      for (int i = 0; i<NUM_JOINTS; i++) // learn non-ik joint angles
      {
        if (i >= getLeftArm()->getShoulder()->getJointIndex() && i<= getLeftArm()->getWrist()->getJointIndex() &&
          i >= getRightArm()->getShoulder()->getJointIndex() && i<= getRightArm()->getWrist()->getJointIndex())
          continue; // doing IK for these joints
        if (m_character->getConstEffector(i)->is3DofEffector())
        {
          NmRs3DofEffector *threeDof = (NmRs3DofEffector *)m_character->getEffectorDirect(i);
          threeDof->setDesiredLean1(target1[i].lean1*blend1 + target2[i].lean1*blend2);
          threeDof->setDesiredLean2(target1[i].lean2*blend1 + target2[i].lean2*blend2);
          threeDof->setDesiredTwist(target1[i].twist*blend1 + target2[i].twist*blend2); 
        }
        else
        {
          NmRs1DofEffector *oneDof = (NmRs1DofEffector *)m_character->getEffectorDirect(i);
          oneDof->setDesiredAngle(target1[i].lean1*blend1 + target2[i].lean1*blend2); 
        }
      }

      // do IK
      rage::Vector3 leftTarget = getIKTarget(getLeftArm()->getClaviclePart(), lLeftArm);
      rage::Vector3 rightTarget = getIKTarget(getRightArm()->getClaviclePart(), lRightArm);

      NmRsLimbInput leftArmIkInput = createNmRsLimbInput<NmRsIKInputWrapper>();
      NmRsIKInputWrapper* leftArmIkInputData = leftArmIkInput.getData<NmRsIKInputWrapper>();
      leftArmIkInputData->setTarget(leftTarget);
      leftArmIkInputData->setTwist(m_sequence[(int)m_frameIndex].m_ik[lLeftArm].twist);
      leftArmIkInputData->setDragReduction(0.0f);
      getLeftArm()->postInput(leftArmIkInput);

      NmRsLimbInput rightArmIkInput = createNmRsLimbInput<NmRsIKInputWrapper>();
      NmRsIKInputWrapper* rightArmIkInputData = rightArmIkInput.getData<NmRsIKInputWrapper>();
      rightArmIkInputData->setTarget(rightTarget);
      rightArmIkInputData->setTwist(m_sequence[(int)m_frameIndex].m_ik[lLeftArm].twist);
      rightArmIkInputData->setDragReduction(0.0f);
      getRightArm()->postInput(rightArmIkInput);

      addSpineLean(rotateVel);

      float frameValueOld = m_frameValue;
      m_frameValue = fmodf(m_frameValue + 30.f*timeStep*m_parameters.speed, (float)m_numberOfFrames);
      m_frameIndex = (int)m_frameValue;

      // test to see if we are at the end of a cycle
      if ((frameValueOld-m_frameValue)>0.)
      {
        m_distTravelled = (comPos-m_character->m_COM).Mag();
        comPos = m_character->m_COM;
      }

      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 2;

        ART::ARTFeedbackInterface::FeedbackUserdata data;
        data.setFloat(frameValueOld / (float)m_numberOfFrames);
        feedback->m_args[0] = data;
        data.setFloat(m_distTravelled);
        feedback->m_args[1] = data;

        strcpy(feedback->m_behaviourName, NMLearnedCrawlFeedbackName);
        feedback->onBehaviourEvent();
      }

      // The Neck isnt being leaned back enough, so do some more....probably should just come in from the animation.
      getSpineInputData()->getLowerNeck()->setDesiredLean1(-2.f);
      getSpineInputData()->getUpperNeck()->setDesiredLean1(-2.f);
    }

    return eCBUTaskComplete;
  }

#if ART_ENABLE_BSPY
  void NmRsCBULearnedCrawl::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.stiffness, true);
    bspyTaskVar(m_parameters.damping, true);
    bspyTaskVar(m_parameters.bLearn, true);
    bspyTaskVar(m_parameters.numFrames2Learn, true);
    bspyTaskVar(m_parameters.yawOffset, true);
    bspyTaskVar(m_parameters.targetPosition, true);
    bspyTaskVar(m_parameters.inputSequenceSize, true);
    bspyTaskVar(m_parameters.speed, true);
    bspyTaskVar(m_parameters.learnFromAnimPlayback, true);
    bspyTaskVar(m_parameters.useSpine3Thing, true);
    bspyTaskVar(m_parameters.useRollBoneCompensation, true);
    bspyTaskVar(m_parameters.useTwister, true);

    bspyTaskVar(m_frameNumber, false);
    bspyTaskVar(m_numberOfFrames, false);      
    bspyTaskVar(m_totalFrame, false);
    bspyTaskVar(m_frameIndex, false);
    bspyTaskVar(m_frameValue, false);
    bspyTaskVar(m_yawOffset, false);
    //CrawlLearner
    bspyTaskVar(m_count, false);
    bspyTaskVar(m_totalError, false);
    bspyTaskVar(m_bestTotalError, false);

    bspyTaskVar(bSupporting[0], false);
    bspyTaskVar(bSupporting[1], false);
    bspyTaskVar(bSupporting[2], false);
    bspyTaskVar(bSupporting[3], false);
  }
#endif // ART_ENABLE_BSPY
}
