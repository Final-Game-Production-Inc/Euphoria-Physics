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
* Curls into foetal position, at a speed defined by the strength and damping values;
* This behaviour is full-body and resets the character when it starts.
*
*/


#include "NmRsInclude.h"
#include "NmRsCBU_BodyFoetal.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "NmRsEngine.h"
#include "NmRsCBU_TaskManager.h"

namespace ART
{
  NmRsCBUBodyFoetal::NmRsCBUBodyFoetal(ART::MemoryManager* services) : CBUTaskBase(services, bvid_bodyFoetal)
  {
    initialiseCustomVariables();
  }

  NmRsCBUBodyFoetal::~NmRsCBUBodyFoetal()
  {
  }

  void NmRsCBUBodyFoetal::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
  }

  void NmRsCBUBodyFoetal::onActivate()
  {
    Assert(m_character);

    doAllTheThings();
  }

  void NmRsCBUBodyFoetal::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUBodyFoetal::onTick(float /*timeStep*/)
  {
    doAllTheThings();
    return eCBUTaskComplete;
  }

  void NmRsCBUBodyFoetal::doAllTheThings()
  {
    m_mask = m_parameters.m_effectorMask;
    
    m_randGen.Reset(m_parameters.m_randomSeed);

    if (m_parameters.m_asymmetrical == 0)
    {
      m_body->setStiffness(m_parameters.m_stiffness, m_parameters.m_damping, m_parameters.m_effectorMask, NULL, true);
    }
    else
    {
      float stiffLow = m_parameters.m_stiffness*(1.f-0.5f*m_parameters.m_asymmetrical);
      float stiffHigh = m_parameters.m_stiffness*(1.f+0.3f*m_parameters.m_asymmetrical);
      float dampLow = m_parameters.m_damping*(1.f-0.3f*m_parameters.m_asymmetrical);
      float dampHigh = m_parameters.m_damping*(1.f+0.3f*m_parameters.m_asymmetrical);

      // way to get indexed access to effectors in new limb system? this is super-fugly.
      // perhaps a special message that sets random values for these parameters across the entire limb?
      getLeftArmInputData()->getClavicle()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getLeftArmInputData()->getShoulder()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getLeftArmInputData()->getElbow()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getLeftArmInputData()->getWrist()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getRightArmInputData()->getClavicle()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getRightArmInputData()->getShoulder()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getRightArmInputData()->getElbow()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getRightArmInputData()->getWrist()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getLeftLegInputData()->getHip()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getLeftLegInputData()->getKnee()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getLeftLegInputData()->getAnkle()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getRightLegInputData()->getHip()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getRightLegInputData()->getKnee()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));
      getRightLegInputData()->getAnkle()->setStiffness(m_randGen.GetRanged(stiffLow, stiffHigh) ,m_randGen.GetRanged(dampLow, dampHigh));

      getSpineInputData()->getSpine0()->setMuscleStrength(m_randGen.GetRanged(stiffLow, stiffHigh));
      getSpineInputData()->getSpine0()->setMuscleDamping(m_randGen.GetRanged(dampLow, dampHigh));
      getSpineInputData()->getSpine1()->setMuscleStrength(m_randGen.GetRanged(stiffLow, stiffHigh));
      getSpineInputData()->getSpine1()->setMuscleDamping(m_randGen.GetRanged(dampLow, dampHigh));
      getSpineInputData()->getSpine2()->setMuscleStrength(m_randGen.GetRanged(stiffLow, stiffHigh));
      getSpineInputData()->getSpine2()->setMuscleDamping(m_randGen.GetRanged(dampLow, dampHigh));
      getSpineInputData()->getSpine3()->setMuscleStrength(m_randGen.GetRanged(stiffLow, stiffHigh));
      getSpineInputData()->getSpine3()->setMuscleDamping(m_randGen.GetRanged(dampLow, dampHigh));
    }

    float backTwist = m_parameters.m_backTwist * m_randGen.GetRanged(0.0f, 1.0f);

    // todo this is a good candidate for anim-driven pose.
    getLeftLegInputData()->getHip()->setDesiredLean1(2.0f);
    getRightLegInputData()->getHip()->setDesiredLean1(2.0f);
    getLeftLegInputData()->getKnee()->setDesiredAngle(-2.0f);
    getRightLegInputData()->getKnee()->setDesiredAngle(-2.0f);
    getLeftLegInputData()->getAnkle()->setMuscleStrength(20.0f);
    getLeftLegInputData()->getAnkle()->setMuscleDamping(5.0f);
    getRightLegInputData()->getAnkle()->setMuscleStrength(20.0f);
    getRightLegInputData()->getAnkle()->setMuscleDamping(5.0f);
    getLeftLegInputData()->getAnkle()->setDesiredLean1(-1.0f);
    getRightLegInputData()->getAnkle()->setDesiredLean1(-1.0f);

    getLeftArmInputData()->getShoulder()->setDesiredTwist(-0.3f);
    getRightArmInputData()->getShoulder()->setDesiredTwist(-0.3f);
    getLeftArmInputData()->getShoulder()->setDesiredLean1(1.57f);
    getRightArmInputData()->getShoulder()->setDesiredLean1(1.57f);
    getLeftArmInputData()->getElbow()->setDesiredAngle(2.0f);
    getRightArmInputData()->getElbow()->setDesiredAngle(2.0f);

    getSpineInputData()->setBackAngles(2.f, 0.f, backTwist);
    getSpineInputData()->getLowerNeck()->setDesiredLean1(0.5f);
  }

#if ART_ENABLE_BSPY
  void NmRsCBUBodyFoetal::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_backTwist, true);
    bspyTaskVar(m_parameters.m_asymmetrical, true);
    bspyTaskVar(m_parameters.m_stiffness, true);
    bspyTaskVar(m_parameters.m_damping, true);
    bspyTaskVar(m_parameters.m_randomSeed, true);
    bspyTaskVar_Bitfield32(m_parameters.m_effectorMask, true);
  }
#endif // ART_ENABLE_BSPY
}

