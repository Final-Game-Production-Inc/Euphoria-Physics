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
 * 
 * Spine Twist. System for twisting the spine of the character towards a point
 * which is possibly moving.
 * 
 */


#include "NmRsInclude.h"
#include "NmRsCBU_SpineTwist.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "ART/ARTFeedback.h"

namespace ART
{
  NmRsCBUSpineTwist::NmRsCBUSpineTwist(ART::MemoryManager* services) : CBUTaskBase(services, bvid_spineTwist)
  {
    initialiseCustomVariables();

  }

  NmRsCBUSpineTwist::~NmRsCBUSpineTwist()
  {
  }

  void NmRsCBUSpineTwist::initialiseCustomVariables()
  {
    m_mask = bvmask_Spine;

    m_pos.Set(0,0,0);
    m_offset = 0.f;
    m_velX = 0.0f; 
    m_velY = 0.0f;
    m_velZ = 0.0f;
    m_twistClavicles = false;
    m_allwaysTwist = false;

    m_doingTwist = false;
    m_twist = 0;
  }

  void NmRsCBUSpineTwist::onActivate()
  {
    Assert(m_character);

    m_twist = 0.f; // referenced by catch fall
    m_doingTwist = true;

    // Facilitate motion beyond the joint's limits. The following are fractions.
    m_spineTwistExcess  = 0.35f;

    // Spine 0.
    m_sp0TwistMin = (1.0f + m_spineTwistExcess)*getSpine()->getSpine0()->getMinTwist();
    m_sp0TwistMax = (1.0f + m_spineTwistExcess)*getSpine()->getSpine0()->getMaxTwist();
    m_sp0Strength = getSpine()->getSpine0()->getMuscleStrength();
    m_sp0Damping  = getSpine()->getSpine0()->getMuscleDamping();

    // Spine 1.
    m_sp1TwistMin = (1.0f + m_spineTwistExcess)*getSpine()->getSpine1()->getMinTwist();
    m_sp1TwistMax = (1.0f + m_spineTwistExcess)*getSpine()->getSpine1()->getMaxTwist();
    m_sp1Strength = getSpine()->getSpine1()->getMuscleStrength();
    m_sp1Damping  = getSpine()->getSpine1()->getMuscleDamping();

    // Spine 2.
    m_sp2TwistMin = (1.0f + m_spineTwistExcess)*getSpine()->getSpine2()->getMinTwist();
    m_sp2TwistMax = (1.0f + m_spineTwistExcess)*getSpine()->getSpine2()->getMaxTwist();
    m_sp2Strength = getSpine()->getSpine2()->getMuscleStrength();
    m_sp2Damping  = getSpine()->getSpine2()->getMuscleDamping();

    // Spine 3.
    m_sp3TwistMin = (1.0f + m_spineTwistExcess)*getSpine()->getSpine3()->getMinTwist();
    m_sp3TwistMax = (1.0f + m_spineTwistExcess)*getSpine()->getSpine3()->getMaxTwist();
    m_sp3Strength = getSpine()->getSpine3()->getMuscleStrength();
    m_sp3Damping  = getSpine()->getSpine3()->getMuscleDamping();

    // Whole spine.
    m_spTwistMin  = m_sp0TwistMin + m_sp1TwistMin + m_sp2TwistMin + m_sp3TwistMin;
    m_spTwistMax  = m_sp0TwistMax + m_sp1TwistMax + m_sp2TwistMax + m_sp3TwistMax;
    m_spStrength  = 0.25f*(m_sp0Strength + m_sp1Strength + m_sp2Strength + m_sp3Strength);
    m_spDamping   = 0.25f*(m_sp0Damping + m_sp1Damping + m_sp2Damping + m_sp3Damping);

    m_spInvTwistMin = 1.0f / m_spTwistMin;
    m_spInvTwistMax = 1.0f / m_spTwistMax;
  }

  void NmRsCBUSpineTwist::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUSpineTwist::onTick(float )
  {
    NM_RS_DBG_LOGF(L"SPINE TWIST");
    NM_RS_DBG_LOGF(L"The posX =  %.4f", m_pos.x);
    NM_RS_DBG_LOGF(L"The posY =  %.4f", m_pos.y);
    NM_RS_DBG_LOGF(L"The posZ =  %.4f", m_pos.z);

    rage::Vector3 posTarget;
    rage::Vector3 velTarget;
    posTarget = m_pos;
    velTarget.Set(m_velX,m_velY,m_velZ);

    // Determine if the point can be twisted toward (relative to the base TM).
    // 'Fixed' frame relative to which one determines if the target point 
    // can be twisted toward.
    rage::Matrix34 tmBaseFrame;
    getSpine()->getPelvisPart()->getBoundMatrix(&tmBaseFrame);
    rage::Vector3 posBaseFrame;
    posBaseFrame = tmBaseFrame.d;

#if ART_ENABLE_BSPY
    m_character->bspyDrawCoordinateFrame(0.1f, tmBaseFrame);
#endif

    // Rotate the base frame around x (up) by m_offset.
    if(m_offset != 0.f)
    {
      rage::Matrix34 offsetMat;
      offsetMat.FromEulersXYZ(rage::Vector3(m_offset, 0, 0));
      //tmBaseFrame.Dot3x3(offsetMat);
      tmBaseFrame.Dot3x3FromLeft(offsetMat);
#if ART_ENABLE_BSPY
      m_character->bspyDrawCoordinateFrame(0.2f, tmBaseFrame);
#endif
    }

    // Calculate the actual target point to be twisted towards.
    rage::Vector3 velBaseFrame(getSpine()->getPelvisPart()->getLinearVelocity());
    rage::Vector3 angVelBaseFrame;

    angVelBaseFrame = getSpine()->getPelvisPart()->getAngularVelocity(); // Note undone next
    angVelBaseFrame.Zero();

    posTarget = m_character->targetPosition(posTarget, velTarget, posBaseFrame, velBaseFrame, angVelBaseFrame, 0.1f);

    // Target direction relative to the origin of our 'base' frame.
    rage::Vector3 dirTarget;         
    dirTarget = posTarget - posBaseFrame;

    dirTarget.Normalize();
    dirTarget.Dot3x3Transpose(tmBaseFrame);

    m_twist = rage::Atan2f(dirTarget.y, -dirTarget.z);
    float twist = m_twist;
    NM_RS_DBG_LOGF(L"_g.twist: %.4f", m_twist);

    // Twist if physically possible.
    if (((twist < PI*0.8f) && (twist > -PI*0.8f)) || m_allwaysTwist) 
    {
      m_doingTwist = true;
      NM_RS_DBG_LOGF(L"ActualTwist1: %.4f , max: %.4f", twist, m_spTwistMax);

      twist *= rage::Abs(m_character->vectorHeight(m_character->m_COMTM.b));//reduce twist as character becomes horizontal
      // Twist calculation. Direct twist toward target. 
      twist = rage::Clamp(twist, m_spTwistMin, m_spTwistMax);

      NM_RS_DBG_LOGF(L"ActualTwist2: %.4f", twist);

      // Must have: weight0 + weight1 + weight2 + weight3 = 1.0
#ifdef NM_HAS_FSEL_INTRINSIC
      float weight0 = (float)__fsel(twist, m_sp0TwistMax * m_spInvTwistMax, m_sp0TwistMin * m_spInvTwistMin);
      float weight1 = (float)__fsel(twist, m_sp1TwistMax * m_spInvTwistMax, m_sp1TwistMin * m_spInvTwistMin);
      float weight2 = (float)__fsel(twist, m_sp2TwistMax * m_spInvTwistMax, m_sp2TwistMin * m_spInvTwistMin);
      float weight3 = (float)__fsel(twist, m_sp3TwistMax * m_spInvTwistMax, m_sp3TwistMin * m_spInvTwistMin);
#else
      float weight0 = 0.0f;              
      float weight1 = 0.0f;
      float weight2 = 0.0f;
      float weight3 = 0.0f;
      if (twist > 0.0f)
      {
        weight0 = m_sp0TwistMax * m_spInvTwistMax;
        weight1 = m_sp1TwistMax * m_spInvTwistMax;
        weight2 = m_sp2TwistMax * m_spInvTwistMax;
        weight3 = m_sp3TwistMax * m_spInvTwistMax;
      }
      else if (twist < 0.0f)
      {
        weight0 = m_sp0TwistMin * m_spInvTwistMin;
        weight1 = m_sp1TwistMin * m_spInvTwistMin;
        weight2 = m_sp2TwistMin * m_spInvTwistMin;
        weight3 = m_sp3TwistMin * m_spInvTwistMin;
      }
#endif // NM_PLATFORM_X360

      getSpineInputData()->getSpine0()->setDesiredTwist(weight0*twist);
      getSpineInputData()->getSpine1()->setDesiredTwist(weight1*twist);
      getSpineInputData()->getSpine2()->setDesiredTwist(weight2*twist);
      getSpineInputData()->getSpine3()->setDesiredTwist(weight3*twist);

      if (m_twistClavicles)
      {
        getRightArmInputData()->getClavicle()->setDesiredTwist(twist);
        getLeftArmInputData()->getClavicle()->setDesiredTwist(-twist);
      }
    }
    else
    {
      m_doingTwist = false;
    }

    return eCBUTaskComplete;
  }


#if ART_ENABLE_BSPY
  void NmRsCBUSpineTwist::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_pos, false);
    bspyTaskVar(m_offset, false);

    bspyTaskVar(m_velX, false);
    bspyTaskVar(m_velY, false);
    bspyTaskVar(m_velZ, false);
    bspyTaskVar(m_twist, false);
    bspyTaskVar(m_spineTwistExcess, false);
    bspyTaskVar(m_twistClavicles, true);
    bspyTaskVar(m_allwaysTwist, true);
    bspyTaskVar(m_doingTwist, false);
  }
#endif // ART_ENABLE_BSPY
}

