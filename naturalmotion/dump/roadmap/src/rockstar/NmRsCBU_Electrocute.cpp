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
 */


#include "NmRsInclude.h"
#include "NmRsCBU_Electrocute.h"
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"

namespace ART
{
  NmRsCBUElectrocute::NmRsCBUElectrocute(ART::MemoryManager* services) : CBUTaskBase(services, bvid_electrocute)
  {
    initialiseCustomVariables();
  }

  NmRsCBUElectrocute::~NmRsCBUElectrocute()
  {
  }

  void NmRsCBUElectrocute::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
  }

  void NmRsCBUElectrocute::onActivate()
  {
    Assert(m_character);
    m_direction = 1.f;     
    m_intialSnapCount = 0;
    m_tazeTimer = -1.0f;
    m_largeTazeTimer = m_character->getRandom().GetRanged(m_parameters.largeMinTime, m_parameters.largeMaxTime);
    m_subTimer      = m_character->getRandom().GetRanged(0.0f, 4000.0f);
    m_noiseSeed     = m_character->getRandom().GetRanged(0.0f, 4000.0f);
  }

  void NmRsCBUElectrocute::onDeactivate()
  {
    Assert(m_character);

    m_subTimer = 0.0f;
    m_tazeTimer = -1.0f;
  }

  CBUTaskReturn NmRsCBUElectrocute::onTick(float timeStep)
  {
    // make the main timer value non-linear
    m_subTimer += timeStep * 0.7f;
    float variableTS = (0.75f + (m_character->getEngine()->perlin3(m_subTimer, m_noiseSeed, 16.0f))) * timeStep;
    m_tazeTimer += variableTS;
    m_largeTazeTimer -= variableTS;
    
    //Reduce drifting on the floor caused by the snaps
    //We may need to do something more complicated to identify if the character is on the floor
    //  e.g. once on floor don't allow to be not on floor for a few frames
    //  we want to ignore small jumps in the air but pick up say falling off a cliff
    if (m_character->hasCollidedWithWorld(bvmask_UpperBody))//Includes colliding with other characters
    {
      rage::Vector3 dampF = m_character->m_COMvel;
      m_character->levelVector(dampF);
      getSpine()->getPelvisPart()->applyForce(-295.1f*dampF);
    }

    //Do the snap
    if (m_tazeTimer > m_parameters.tazeInterval || m_tazeTimer < 0.0f)
    {
      float snapMag = m_parameters.tazeMag;
      if (m_intialSnapCount < 2)
      {
        m_intialSnapCount++;
        snapMag *= m_parameters.initialMult;
      }
      //Apply a larger snap?  //mmmmtodo should this be paired with it's unsnap?
      if (m_largeTazeTimer < 0.0f)
      {
        snapMag *= m_parameters.largeMult;
        m_largeTazeTimer = m_character->getRandom().GetRanged(m_parameters.largeMinTime, m_parameters.largeMaxTime);
        Assert(m_parameters.largeMinTime < m_parameters.largeMaxTime);
        //Stop large snaps happening all the time if m_parameters.largeMinTime > m_parameters.largeMaxTime
        if (m_largeTazeTimer < 0.0f)
          m_largeTazeTimer *= -1.0f;
      }

      float mult = 1.f;
      //mult used to be set to 2.0f if catchFall running - this only mults the snap on tha legs for useTorques=false only
      m_character->snap(
        snapMag*m_direction,
        m_parameters.directionRandomness, 
        m_parameters.hipType,
        m_parameters.leftArm,
        m_parameters.rightArm,
        m_parameters.leftLeg,  
        m_parameters.rightLeg,  
        m_parameters.spine,  
        m_parameters.neck, 
        m_parameters.phasedLegs, 
        m_parameters.useTorques,
        mult,
        -1,
        NULL,
        m_parameters.movingMult,
        m_parameters.balancingMult,
        m_parameters.airborneMult,
        m_parameters.movingThresh);

      m_direction *= -1.f;//Reverse the snap direction
      m_tazeTimer = 0.f;
    }

    //Set muscles
    float muscleStiffness;
    if (m_parameters.applyStiffness)
    {
      muscleStiffness = 1.f;
      m_body->setStiffness(12.f, 1.f, bvmask_Full, &muscleStiffness);
    }
    //Stabilize Wrists and ankles
    muscleStiffness = 10.f;
    getLeftArmInputData()->getWrist()->setStiffness(12.f, 1.f, &muscleStiffness);
    getRightArmInputData()->getWrist()->setStiffness(12.f, 1.f, &muscleStiffness);
    getLeftLegInputData()->getAnkle()->setStiffness(12.f, 1.f, &muscleStiffness);
    getRightLegInputData()->getAnkle()->setStiffness(12.f, 1.f, &muscleStiffness);

    return eCBUTaskComplete;
  }


#if ART_ENABLE_BSPY
  void NmRsCBUElectrocute::sendParameters(NmRsSpy& spy)
  {

    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.tazeMag, true);
    bspyTaskVar(m_parameters.initialMult, true);
    bspyTaskVar(m_parameters.largeMult, true);
    bspyTaskVar(m_parameters.largeMinTime, true);
    bspyTaskVar(m_parameters.largeMaxTime, true);
    bspyTaskVar(m_parameters.movingMult, true);
    bspyTaskVar(m_parameters.balancingMult, true);
    bspyTaskVar(m_parameters.airborneMult, true);
    bspyTaskVar(m_parameters.movingThresh, true);
    bspyTaskVar(m_parameters.tazeInterval, true);
    bspyTaskVar(m_parameters.directionRandomness, true);
    bspyTaskVar(m_parameters.hipType, true);
    bspyTaskVar(m_parameters.leftLeg, true);
    bspyTaskVar(m_parameters.rightLeg, true);
    bspyTaskVar(m_parameters.leftArm, true);
    bspyTaskVar(m_parameters.rightArm, true);
    bspyTaskVar(m_parameters.spine, true);
    bspyTaskVar(m_parameters.neck, true);
    bspyTaskVar(m_parameters.phasedLegs, true);
    bspyTaskVar(m_parameters.applyStiffness, true);
    bspyTaskVar(m_parameters.useTorques, true);

    bspyTaskVar(m_tazeTimer, false);
    bspyTaskVar(m_largeTazeTimer, false);   
    bspyTaskVar(m_direction, false);     
    bspyTaskVar(m_subTimer, false);
    bspyTaskVar(m_noiseSeed, false);

  }
#endif // ART_ENABLE_BSPY
}

