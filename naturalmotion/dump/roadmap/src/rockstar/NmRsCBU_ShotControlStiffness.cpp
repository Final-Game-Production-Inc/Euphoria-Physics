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
#include "NmRsCBU_Shot.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_StaggerFall.h"


namespace ART
{
     //----------------CONTROL STIFFNESS------------------------------------------------
    bool NmRsCBUShot::controlStiffness_entryCondition()
    {
      return m_newHit /*&& !m_falling*/;
    }
    void NmRsCBUShot::controlStiffness_entry()
    {
    }

    // Should only be called from within normal Shot tick sequence (activate,
    // tick or deactivate) to ensure m_body is correctly set up.
    void NmRsCBUShot::controlStiffness_tick(float timeStep)
    {
      controlStiffness_tick(timeStep, *m_body);
    }

    void NmRsCBUShot::controlStiffness_tick(float timeStep, NmRsHumanBody& body)
    {
      m_spineStiffness = m_parameters.bodyStiffness;
      m_armsStiffness = m_parameters.armStiffness;
      m_spineDamping = m_parameters.spineDamping;//1.f;


      // ramp up MUSCLE STIFFNESS: an initial flat response is followed by ramp:
      if(m_controlStiffnessTime >= m_parameters.initialWeaknessZeroDuration)
        m_controlStiffnessStrengthScale = rage::Min(1.f, m_controlStiffnessStrengthScale + timeStep/m_parameters.initialWeaknessRampDuration);

      // ramp up neck stiffness/damping an initial flat response is followed by ramp:
      if(m_controlNeckStiffnessTime >= m_parameters.initialNeckDuration)
        m_controlNeckStiffnessScale = rage::Min(1.f, m_controlNeckStiffnessScale + timeStep/m_parameters.initialNeckRampDuration);
      m_neckStiffness = m_parameters.initialNeckStiffness + m_controlNeckStiffnessScale*(m_parameters.neckStiffness - m_parameters.initialNeckStiffness);
      m_neckDamping = m_parameters.initialNeckDamping + m_controlNeckStiffnessScale*(m_parameters.neckDamping - m_parameters.initialNeckDamping);

      NmRsCBUStaggerFall* staggerFallTask = (NmRsCBUStaggerFall*)m_cbuParent->m_tasks[bvid_staggerFall];
      if (m_falling && (m_parameters.looseness4Fall > 0.01f))
      {
          // calculate ramped muscle stiffness
          float scale = (1.f-m_parameters.looseness4Fall) + m_controlStiffnessStrengthScale*m_parameters.looseness4Fall;
          float strengthScale = 1.f+(1.f-scale)*m_parameters.kMultOnLoose;
          scale = rage::Clamp(scale,0.05f,1.f);
          body.setStiffnessScaling(strengthScale, scale, scale, bvmask_BodyExceptHandsAndFeet);
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "CS:falling: strengthScale", strengthScale);
          bspyScratchpad(m_character->getBSpyID(), "CS:falling: dampingScale", scale);
          bspyScratchpad(m_character->getBSpyID(), "CS:falling: stiffnessScale", scale);
#endif

      }
      //mmmmnote we could also expose e.g. teeter here
      //mmmmnote staggerFall always sets the upperBodyStiffness even if upperBodyReation = false
      else if (staggerFallTask->isActive() && (m_parameters.looseness4Stagger > 0.01f))
      {
        // calculate ramped muscle stiffness
        float scale = (1.f-m_parameters.looseness4Stagger) + m_controlStiffnessStrengthScale*m_parameters.looseness4Stagger;
        float strengthScale = 1.f+(1.f-scale)*m_parameters.kMultOnLoose;
        scale = rage::Clamp(scale,0.05f,1.f);
        body.setStiffnessScaling(strengthScale, scale, scale, bvmask_UpperBody);

#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "CS:stagger: strengthScale", strengthScale);
        bspyScratchpad(m_character->getBSpyID(), "CS:stagger: dampingScale", scale);
        bspyScratchpad(m_character->getBSpyID(), "CS:stagger: stiffnessScale", scale);
#endif
      }
      else if (!m_falling)
      {
        m_character->controlStiffness(
          body,
          timeStep,
          m_controlStiffnessStrengthScale,
          m_spineStiffness,
          m_armsStiffness,
          m_armsDamping,
          m_spineDamping,
          m_neckStiffness,
          m_neckDamping,
          m_upperBodyStiffness,
          m_lowerBodyStiffness,
          m_injuredLArm,
          m_injuredRArm,
          m_parameters.loosenessAmount,
          m_parameters.kMultOnLoose,
          m_parameters.kMult4Legs,
          m_parameters.minLegsLooseness,
          m_parameters.minArmsLooseness,
          m_parameters.bulletProofVest,
          m_parameters.allowInjuredArm,
          m_parameters.stableHandsAndNeck);
      }
    }

    bool NmRsCBUShot::controlStiffness_exitCondition()
    {
      return m_newHit /*|| m_falling*/;
    }

    void NmRsCBUShot::controlStiffness_exit()
    {
      //these 2 ifs are to reset if the character is falling - probably redundant as shot control stiffness is over at this point
      //  and isn't restarted unless m_falling becomes false (which it generally doesn't.  The 2 stiffness scales are reset on activate anyway.
      if (!m_newHit || m_controlStiffnessStrengthScale >= 1.f || m_parameters.alwaysResetLooseness)
        m_controlStiffnessStrengthScale = 0.01f;
      if (!m_newHit || m_controlNeckStiffnessScale >= 1.f || m_parameters.alwaysResetNeckLooseness)
        m_controlNeckStiffnessScale = 0.01f;

      m_body->resetEffectors(kResetMuscleStiffness);//also done when m_falling becomes true to undo muscleStiffness changes now that 
    }
}
