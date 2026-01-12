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
#include "NmRsCBU_RelaxUnwind.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

namespace ART
{
  NmRsCBURelaxUnwind::NmRsCBURelaxUnwind(ART::MemoryManager* services) : CBUTaskBase(services, bvid_relaxUnwind),
    m_whichWayUp(kFaceUp),
    m_leftArm(0),
    m_rightArm(0),
    m_leftLeg(0),
    m_rightLeg(0),
    m_spine(0),
    m_behaviourTime(0.0f)
  {
    initialiseCustomVariables();
  }

  NmRsCBURelaxUnwind::~NmRsCBURelaxUnwind()
  {
  }

  void NmRsCBURelaxUnwind::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_whichWayUp           = kFaceUp;

    for(int i = 0; i < TotalKnownHumanEffectors; ++i)
    {
      m_startingStiffness[i] = 0.0f;
      m_startingPose[i].Zero();
    }
  }

  void NmRsCBURelaxUnwind::onActivate()
  {
    Assert(m_character);

    m_behaviourTime     = 0.0f;

    // get local references to limbs and spine
    m_leftArm = m_character->getLeftArmSetup();
    m_rightArm = m_character->getRightArmSetup();
    m_leftLeg = m_character->getLeftLegSetup();
    m_rightLeg = m_character->getRightLegSetup();
    m_spine = m_character->getSpineSetup();

    // cache starting state of the body
    for(int i = 0; i < TotalKnownHumanEffectors; ++i)
    {
      NmRsEffectorBase* effector = m_character->getEffector(i);
      Assert(effector);

      // cache initial stiffness
      float strength = effector->getMuscleStrength();
      m_startingStiffness[i] = rage::Sqrtf(strength);

      // cache initial pose
      if(effector->is3DofEffector()) 
      {
        NmRs3DofEffector* eff3Dof = static_cast<NmRs3DofEffector*>(effector);
        m_startingPose[i].SetX(eff3Dof->getDesiredLean1());
        m_startingPose[i].SetY(eff3Dof->getDesiredLean2());
        m_startingPose[i].SetZ(eff3Dof->getDesiredTwist());
      } 
      else 
      {
        NmRs1DofEffector* eff1Dof = static_cast<NmRs1DofEffector*>(effector);
        m_startingPose[i].SetX(eff1Dof->getDesiredAngle());
      }
    }
  }

  void NmRsCBURelaxUnwind::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();
    m_spine = NULL;
    m_leftArm = NULL;
    m_rightArm = NULL;
    m_leftLeg = NULL;
    m_rightLeg = NULL;
  }

  CBUTaskReturn NmRsCBURelaxUnwind::onTick(float timeStep)
  {
    // figure out which way is up
    // project up on spine3 yz plane and normalize
    // TODO: decide if this should be done just once in activate
    rage::Vector3 upProjection;
    rage::Matrix34 spine3TM;
    m_spine->getSpine3Part()->getBoundMatrix(&spine3TM);
    upProjection.Cross(spine3TM.a, m_character->m_gUp);
    upProjection.Cross(spine3TM.a);
    upProjection.Normalize();
    NM_RS_CBU_DRAWCOORDINATEFRAME(0.5f, spine3TM);
    NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, m_character->m_gUp, rage::Vector3(1,0,1));
    NM_RS_CBU_DRAWVECTORCOL(spine3TM.d, upProjection, rage::Vector3(1,1,0));
    float dotForward = upProjection.Dot(spine3TM.c);
    float dotLeft = upProjection.Dot(spine3TM.b);

    // label the directions
    if(dotForward > 0.5f) {
      m_whichWayUp = kFaceDown;
    } else if(dotForward < -0.5f) {
      m_whichWayUp = kFaceUp;
    } else if(dotLeft > 0.0f) {
      m_whichWayUp = kRightSide;
    } else {
      m_whichWayUp = kLeftSide;
    }

    float tRelax = 0.0f;
    if(m_behaviourTime < m_parameters.relaxTime)
      tRelax = 1.0f-m_behaviourTime/m_parameters.relaxTime;

    float tUnwind = 0.0f;
    if(m_behaviourTime < m_parameters.relaxTime)
      tUnwind = 1.0f-m_behaviourTime/m_parameters.unwindTime;
    tUnwind *= m_parameters.unwindAmount;
    float invTunwind = 1.0f - tUnwind;

    for(int i = 0; i < TotalKnownHumanEffectors; ++i)
    {
      // interpolate stiffness
      float stiffness = m_parameters.spineStiffness + (tRelax * (m_startingStiffness[i] - m_parameters.spineStiffness));
      m_character->getEffector(i)->setStiffness(stiffness, 1.0f);

      // interpolate pose
      NmRsEffectorBase* effector = m_character->getEffector(i);
      Assert(effector);
      rage::Vector3 targetPose;
      targetPose.Zero(); // put target pose here!!!

      if(effector->is3DofEffector()) 
      {
        NmRs3DofEffector* eff3Dof = static_cast<NmRs3DofEffector*>(effector);
        eff3Dof->setDesiredLean1((m_startingPose[i].x * invTunwind) + (targetPose.x * tUnwind));
        eff3Dof->setDesiredLean2((m_startingPose[i].y * invTunwind) + (targetPose.y * tUnwind));
        eff3Dof->setDesiredTwist((m_startingPose[i].z * invTunwind) + (targetPose.z * tUnwind));
      } 
      else 
      {
        NmRs1DofEffector* eff1Dof = static_cast<NmRs1DofEffector*>(effector);
        eff1Dof->setDesiredAngle((m_startingPose[i].x * invTunwind) + (targetPose.x * tUnwind));
      }

    }

    // make sure this is a human character
    // TODO: this could work for the quads, but would need a different spine setup
    if(m_character->isBiped())
    {
      // turn head under certain conditions
      if(m_whichWayUp == kFaceDown) 
      {
        // turn head to keep face off of ground
        float twistAngle;
        if(dotLeft > 0)
          twistAngle = -1.0f;
        else
          twistAngle = 1.0f;
        m_spine->getLowerNeck()->setDesiredTwist(twistAngle);
        m_spine->getUpperNeck()->setDesiredTwist(twistAngle);

        // lift head away from ground
        m_spine->getLowerNeck()->setDesiredLean1(-1.0f);
        m_spine->getUpperNeck()->setDesiredLean1(-1.0f);
      } 
      else if(m_whichWayUp == kFaceUp)
      {
        // lift head away from ground
        m_spine->getLowerNeck()->setDesiredLean1(1.0f);
        m_spine->getUpperNeck()->setDesiredLean1(1.0f);
      }
    }

    m_behaviourTime += timeStep;

    return eCBUTaskComplete;
  }

#if ART_ENABLE_BSPY
  void NmRsCBURelaxUnwind::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);
  }
#endif // ART_ENABLE_BSPY

}

