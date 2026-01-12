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
* This will take any animation currently set as the incoming transforms of the agent and drive the 
* character to the pose defined therein.
* You can choose to have the animPose override the Headlook/PointArm/PointGun behaviours if the animPose
* mask includes (all) neck and head/ua,ul,ur/ua,ul,ur respectively.
*
* This is also the (temporary) place for setting up gravity compensation on the character.
*/


#include "NmRsInclude.h"
#include "NmRsCBU_AnimPose.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

namespace ART
{
  NmRsCBUAnimPose::NmRsCBUAnimPose(ART::MemoryManager* services) : CBUTaskBase(services, bvid_animPose)
  {
    initialiseCustomVariables();
  }

  NmRsCBUAnimPose::~NmRsCBUAnimPose()
  {
  }

  void NmRsCBUAnimPose::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
  }

  void NmRsCBUAnimPose::onActivate()
  {
    Assert(m_character);
  }

  void NmRsCBUAnimPose::onDeactivate()
  {
    Assert(m_character);
    m_character->m_posture.useZMP = true;

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUAnimPose::onTick(float /*timeStep*/)
  {
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    if (m_character->overideAnimPose)
    {
      updateBehaviourMessage(NULL);
    }
#endif // NM_SCRIPTING && NM_SCRIPTING_VARIABLES

    NM_RS_DBG_LOGF(L"ANIM POSE");
    Assert(m_parameters.stiffness >= 2.f || m_parameters.stiffness < 0.f );

    // set stiffness if parameter is positive.
    if(m_parameters.stiffness > 0.f)
    {
      // set muscleStiffness if parameter is positive.
      float* pMuscleStiffness = 0;
      if(m_parameters.muscleStiffness > 0.f)
        pMuscleStiffness = &m_parameters.muscleStiffness;
      m_body->setStiffness(m_parameters.stiffness, m_parameters.damping, m_parameters.effectorMask, pMuscleStiffness);
    }

    m_body->activePose(m_parameters.animSource, m_parameters.effectorMask);

    if (m_parameters.gravCompLeftArm >= -0.0001f ||
      m_parameters.gravCompRightArm >= -0.0001f ||
      m_parameters.gravCompLeftLeg >= -0.0001f ||
      m_parameters.gravCompRightLeg >= -0.0001f ||
      m_parameters.gravCompSpine >= -0.0001f)
      m_character->m_posture.useZMP = m_parameters.useZMPGravityCompensation;

    if (m_parameters.connectedLeftHand == -1)
      m_character->m_posture.leftArmAutoSet = true;
    else if (m_parameters.connectedLeftHand > 0)
      m_character->setLeftHandConnected(true);
    else if (m_parameters.connectedLeftHand == 0)
      m_character->setLeftHandConnected(false);

    if (m_parameters.connectedRightHand == -1)
      m_character->m_posture.rightArmAutoSet = true;
    else if (m_parameters.connectedRightHand > 0)
      m_character->setRightHandConnected(true);
    else if (m_parameters.connectedRightHand == 0)
      m_character->setRightHandConnected(false);

    if (m_parameters.connectedLeftFoot == -1)
      m_character->m_posture.leftLegAutoSet = true;
    else if (m_parameters.connectedLeftFoot > 0)
      m_character->setLeftFootConnected(true);
    else if (m_parameters.connectedLeftFoot == 0)
      m_character->setLeftFootConnected(false);

    if (m_parameters.connectedRightFoot == -1)
      m_character->m_posture.rightLegAutoSet = true;
    else if (m_parameters.connectedRightFoot > 0)
      m_character->setRightFootConnected(true);
    else if (m_parameters.connectedRightFoot == 0)
      m_character->setRightFootConnected(false);

    // difference from original code: we are honoring the effector mask when
    // it was previously ignored.
    //
    if (m_parameters.gravCompLeftArm >= -0.00001f)
      getLeftArm()->setOpposeGravity(m_body->getInput(kLeftArm), m_parameters.gravCompLeftArm, m_parameters.effectorMask);
    if (m_parameters.gravCompRightArm >= -0.00001f)
      getRightArm()->setOpposeGravity(m_body->getInput(kRightArm), m_parameters.gravCompRightArm, m_parameters.effectorMask);
    if (m_parameters.gravCompLeftLeg >= -0.00001f)
      getLeftLeg()->setOpposeGravity(m_body->getInput(kLeftLeg), m_parameters.gravCompLeftLeg, m_parameters.effectorMask);
    if (m_parameters.gravCompRightLeg >= -0.00001f)
      getRightLeg()->setOpposeGravity(m_body->getInput(kRightLeg), m_parameters.gravCompRightLeg, m_parameters.effectorMask);
    if (m_parameters.gravCompSpine >= -0.00001f)
      getSpine()->setOpposeGravity(m_body->getInput(kSpine), m_parameters.gravCompSpine, m_parameters.effectorMask);

    return eCBUTaskComplete;
  }

#if ART_ENABLE_BSPY
  void NmRsCBUAnimPose::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.muscleStiffness, true);
    bspyTaskVar(m_parameters.stiffness, true);
    bspyTaskVar(m_parameters.damping, true);
    bspyTaskVar_Bitfield32(m_parameters.effectorMask, true);
    bspyTaskVar(m_parameters.overideHeadlook, true);
    bspyTaskVar(m_parameters.overidePointArm, true);
    bspyTaskVar(m_parameters.overidePointGun, true);

    bspyTaskVar(m_parameters.useZMPGravityCompensation, true);
    bspyTaskVar(m_parameters.gravityCompensation, true);

    bspyTaskVar(m_parameters.muscleStiffnessLeftArm, true);
    bspyTaskVar(m_parameters.muscleStiffnessRightArm, true);
    bspyTaskVar(m_parameters.muscleStiffnessSpine, true);
    bspyTaskVar(m_parameters.muscleStiffnessLeftLeg, true);
    bspyTaskVar(m_parameters.muscleStiffnessRightLeg, true);

    bspyTaskVar(m_parameters.stiffnessLeftArm, true);
    bspyTaskVar(m_parameters.stiffnessRightArm, true);
    bspyTaskVar(m_parameters.stiffnessSpine, true);
    bspyTaskVar(m_parameters.stiffnessLeftLeg, true);
    bspyTaskVar(m_parameters.stiffnessRightLeg, true);

    bspyTaskVar(m_parameters.dampingLeftArm, true);
    bspyTaskVar(m_parameters.dampingRightArm, true);
    bspyTaskVar(m_parameters.dampingSpine, true);
    bspyTaskVar(m_parameters.dampingLeftLeg, true);
    bspyTaskVar(m_parameters.dampingRightLeg, true);

    bspyTaskVar(m_parameters.gravCompLeftArm, true);
    bspyTaskVar(m_parameters.gravCompRightArm, true);
    bspyTaskVar(m_parameters.gravCompLeftLeg, true);
    bspyTaskVar(m_parameters.gravCompRightLeg, true);
    bspyTaskVar(m_parameters.gravCompSpine, true);
    static const char* connectedTypeStrings[] = 
    {
      "balanceDecides",
      "AutoByImpacts",
      "Unconnected",
      "Fully",
      "Point",
      "Line",
    };  
    bspyTaskVar_StringEnum(m_parameters.connectedLeftHand+2, connectedTypeStrings, true);  
    bspyTaskVar_StringEnum(m_parameters.connectedRightHand+2, connectedTypeStrings, true);  
    bspyTaskVar_StringEnum(m_parameters.connectedLeftFoot+2, connectedTypeStrings, true);  
    bspyTaskVar_StringEnum(m_parameters.connectedRightFoot+2, connectedTypeStrings, true);  
    static const char* animSourceStrings[] = 
    {
      "kITSourceCurrent", /**< Transforms are for the current frame */
      "kITSourcePrevious",    /**< Transforms are for the previous frame */
#if NM_ANIM_MATRICES
      "kITSourceAnimation",
#endif
      "kITSourceUnknown",    /**< Transforms are for the previous frame */
      /* how many transform sources will be prepared */
    };
    int animSource = m_parameters.animSource;
    if (animSource<0 || animSource>=KITSourceCount)
      animSource = KITSourceCount;
    bspyTaskVar_StringEnum(animSource, animSourceStrings, true);  

  }
#endif // ART_ENABLE_BSPY
}