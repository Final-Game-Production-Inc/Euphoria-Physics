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
 * Legs pedalling or bicycle pedalling type motion
 * 
 */


#include "NmRsInclude.h"
#include "NmRsCBU_ArmHang.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsEngine.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_SpineTwist.h"
#include "NmRsCBU_Pedal.h"
#include "NmRsCBU_PointArm.h"


namespace ART
{
  NmRsCBUArmHang::NmRsCBUArmHang(ART::MemoryManager* services) : CBUTaskBase(services, bvid_armHang),
    m_constraint(0),
    m_constraintR(0)
  {
    initialiseCustomVariables();
  }

  NmRsCBUArmHang::~NmRsCBUArmHang()
  {
  }

  void NmRsCBUArmHang::initialiseCustomVariables()
  {
    m_mask = bvmask_ArmLeft | bvmask_ArmRight;

    m_constraint = 0;
    m_constraintR = 0;
  }

  void NmRsCBUArmHang::onActivate()
  {
    Assert(m_character);

    if (m_parameters.m_constraintPosition.IsZero()&& m_parameters.m_instanceIndex == -1)
      m_parameters.m_constraintPosition = m_leftArm->getHand()->getPosition();
    
    NmRsCBUPointArm* pointArmTask = (NmRsCBUPointArm*)m_cbuParent->m_tasks[bvid_pointArm];
    Assert(pointArmTask);
    if (!pointArmTask->isActive())
    {
      pointArmTask->updateBehaviourMessage(NULL);
      pointArmTask->activate();
    }
    pointArmTask->m_parameters_Right.useRightArm = true;
    pointArmTask->m_parameters_Right.target = m_parameters.m_targetPosition;


    NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)m_cbuParent->m_tasks[bvid_headLook];    
    Assert(headLookTask);
    if (!headLookTask->isActive())
    {
      headLookTask->updateBehaviourMessage(NULL);
      headLookTask->activate();
    }
    headLookTask->m_parameters.m_pos = m_parameters.m_targetPosition;

    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist];    
    Assert(spineTwistTask);
    if (!spineTwistTask->isActive())
      spineTwistTask->activate();
    spineTwistTask->setSpineTwistPos(m_parameters.m_targetPosition);

    NmRsCBUPedal *pedalTask = (NmRsCBUPedal *)m_cbuParent->m_tasks[bvid_pedalLegs];    
    Assert(pedalTask);
    if (!pedalTask->isActive())
    {
      pedalTask->updateBehaviourMessage(NULL);
      pedalTask->activate();
    }
    pedalTask->m_parameters.m_radius = 0.1f;
    pedalTask->m_parameters.m_angularSpeed = 1.f;

    // create the constraint with the left Hand 
    rage::Vector3 handP = m_leftArm->getHand()->getPosition();
   if (m_parameters.m_instanceIndex == -1)
    {
      m_character->fixPart(m_leftArm->getHand()->getPartIndex(), m_parameters.m_constraintPosition, handP, 0.f, &m_constraint);
      m_character->fixPart(m_leftArm->getHand()->getPartIndex(), m_parameters.m_constraintPosition, handP, 0.f, &m_constraintR);
   }
    else
    {
      m_character->fixPartsTogether2(m_leftArm->getHand()->getPartIndex(), m_parameters.m_boundIndex, 0.f, handP, m_parameters.m_constraintPosition, m_parameters.m_instanceIndex, &m_constraint);
      m_character->fixPart(m_leftArm->getHand()->getPartIndex(), m_parameters.m_constraintPosition, handP, 0.f, &m_constraintR);
   }

   if (m_constraintR && m_constraintR->IsActive())
   {
     m_constraintR->SetFixedRotation();
   }
  }

  void NmRsCBUArmHang::onDeactivate()
  {
    Assert(m_character);

    if (m_constraint && m_constraint->IsActive() &&
      (m_constraint->GetInstanceA()==m_character->getFirstInstance() ||
      m_constraint->GetInstanceB()==m_character->getFirstInstance()))
    {
        rage::phConstraintMgr *mgr = m_character->getSimulator()->GetConstraintMgr();
      if (mgr)
        mgr->ReleaseConstraint(m_constraint);
      m_constraint = 0;
    }

    if (m_constraintR && m_constraintR->IsActive() &&
      (m_constraintR->GetInstanceA()==m_character->getFirstInstance() ||
      m_constraintR->GetInstanceB()==m_character->getFirstInstance()))
    {
        rage::phConstraintMgr *mgr = m_character->getSimulator()->GetConstraintMgr();
      if (mgr)
        mgr->ReleaseConstraint(m_constraintR);
      m_constraintR = 0;
    }

    NmRsCBUPointArm* pointArmTask = (NmRsCBUPointArm*)m_cbuParent->m_tasks[bvid_pointArm];
    Assert(pointArmTask);
    if (pointArmTask->isActive())
      pointArmTask->deactivate();

    NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)m_cbuParent->m_tasks[bvid_headLook];    
    Assert(headLookTask);
    if (headLookTask->isActive())
      headLookTask->deactivate();

    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist];    
    Assert(spineTwistTask);
    if (spineTwistTask->isActive())
      spineTwistTask->deactivate();

    NmRsCBUPedal *pedalTask = (NmRsCBUPedal *)m_cbuParent->m_tasks[bvid_pedalLegs];    
    Assert(pedalTask);
    if (pedalTask->isActive())
      pedalTask->deactivate();

    initialiseCustomVariables();
  }

    CBUTaskReturn NmRsCBUArmHang::onTick(float /*timeStep*/)
  {

      if (m_parameters.m_constraintPosition.IsZero()&& m_parameters.m_instanceIndex == -1)
        m_parameters.m_constraintPosition = m_leftArm->getHand()->getPosition();
      
      NmRsCBUPointArm* pointArmTask = (NmRsCBUPointArm*)m_cbuParent->m_tasks[bvid_pointArm];
      Assert(pointArmTask);
      if (!pointArmTask->isActive())
      {
        pointArmTask->updateBehaviourMessage(NULL);
        pointArmTask->activate();
      }
      pointArmTask->m_parameters_Right.useRightArm = true;
      pointArmTask->m_parameters_Right.target = m_parameters.m_targetPosition;


      NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)m_cbuParent->m_tasks[bvid_headLook];    
      Assert(headLookTask);
      if (!headLookTask->isActive())
      {
        headLookTask->updateBehaviourMessage(NULL);
        headLookTask->activate();
      }
      headLookTask->m_parameters.m_pos = m_parameters.m_targetPosition;

      NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist];    
      Assert(spineTwistTask);
      if (!spineTwistTask->isActive())
        spineTwistTask->activate();
      spineTwistTask->setSpineTwistPos(m_parameters.m_targetPosition);


      // Update the constraint position.
      if (m_constraint && m_constraint->IsActive())
      {
       if (m_parameters.m_instanceIndex == -1)
       {
          m_constraint->SetWorldPosition(m_parameters.m_constraintPosition); 
       }
      }
      if (m_constraintR && m_constraintR->IsActive())
      {
        m_constraintR->SetFixedRotation();
      }

    return eCBUTaskComplete;
  }

#if ART_ENABLE_BSPY
  void NmRsCBUArmHang::sendParameters(NmRsSpy& spy)
  {
      CBUTaskBase::sendParameters(spy);
  }
#endif // ART_ENABLE_BSPY

}
