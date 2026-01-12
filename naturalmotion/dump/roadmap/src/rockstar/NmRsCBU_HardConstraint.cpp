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
#include "NmRsCBU_HardConstraint.h"
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

namespace ART
{
  NmRsCBUHardConstraint::NmRsCBUHardConstraint(ART::MemoryManager* services) : CBUTaskBase(services, bvid_hardConstraint)
  {
    initialiseCustomVariables();
  }

  NmRsCBUHardConstraint::~NmRsCBUHardConstraint()
  {
  }

  void NmRsCBUHardConstraint::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_parameters.m_FramesSinceTMUpdate = 0;
    for (int i=0;i<21;i++)
    {
      m_hardConstraintOnCharacter[i] = 0;
      m_hardConstraintRotationOnCharacter[i] = 0;
      m_parameters.m_partIndex[i] = -1;
      m_parameters.m_constrainOrientation[i] = false;
      m_parameters.m_constrainTranslation[i] = false;
      m_parameters.m_blendWithPreviousFrame[i] = false;
    }
  }

  void NmRsCBUHardConstraint::onActivate()
  {
    Assert(m_character);

    m_spine = m_character->getSpineSetup();
  }

  void NmRsCBUHardConstraint::onDeactivate()
  {
    Assert(m_character);

    for (int i=0;i<21;i++)
    {
      removeConstraint(i);
    }
    initialiseCustomVariables();
    m_spine = NULL;
  }

  CBUTaskReturn NmRsCBUHardConstraint::onTick(float /*timeStep*/)
  {
    for (int i=0;i<21;i++)
    {
      if (m_parameters.m_partIndex[i]!=-1)
      {
        updateConstraint(i,m_parameters.m_constrainOrientation[i],m_parameters.m_constrainTranslation[i],m_parameters.m_blendWithPreviousFrame[i]);
      }
      else
      {
        removeConstraint(i);
      }
    }
    m_parameters.m_FramesSinceTMUpdate++;

    return eCBUTaskComplete;
  }

  void NmRsCBUHardConstraint::removeConstraint(int partIndex)
  {
    rage::phConstraintMgr *mgr = m_character->getSimulator()->GetConstraintMgr();
    if (mgr)
    {
      // remove the constraint
      if (m_hardConstraintOnCharacter[partIndex])
      {
        mgr->ReleaseConstraint(m_hardConstraintOnCharacter[partIndex]);
        m_hardConstraintOnCharacter[partIndex] = 0;
        if (m_parameters.m_telportedVelOnDeactivation&&m_parameters.m_applyTeleporation&&!partIndex)
        {
          applyVelocityOnDeactivation();
        }
      }
      if (m_hardConstraintRotationOnCharacter[partIndex])
      {
        mgr->ReleaseConstraint(m_hardConstraintRotationOnCharacter[partIndex]);
        m_hardConstraintRotationOnCharacter[partIndex] = 0;
      }
    }
    m_parameters.m_partIndex[partIndex] = -1;
  }

  void  NmRsCBUHardConstraint::updateConstraint(int partIndex, bool constraintO, bool constraintT, bool blendWithPreviousFrame)
  {
    rage::Matrix34 targetM;
    targetM.Identity();
    m_character->getGenericPartByIndex(partIndex)->getBoundMatrix(&targetM);
    getIncomingTranformByBodyPart(targetM,partIndex,blendWithPreviousFrame);

    if (!partIndex&&m_parameters.m_applyTeleporation)
    {
      rage::Matrix34 currentPTM;
      m_character->getGenericPartByIndex(0)->getMatrix(currentPTM);
      rage::Vector3 pOffset = targetM.d - currentPTM.d;

      for (int i = 0;i<21;i++)
      {
        rage::Matrix34 currentTM;
        m_character->getGenericPartByIndex(i)->getMatrix(currentTM);
        currentTM.d = currentTM.d + pOffset;
        m_character->getGenericPartByIndex(i)->setMatrix(currentTM);
      }
      m_character->getLevel()->UpdateObjectLocation(m_character->getFirstInstance()->GetLevelIndex());

      if (m_parameters.m_stabiliseHead)
      {
        m_spine->getSpine0()->setMuscleStiffness(2.f);
        m_spine->getSpine1()->setMuscleStiffness(2.f);
        m_spine->getSpine2()->setMuscleStiffness(2.f);
        m_spine->getSpine3()->setMuscleStiffness(2.f);

        m_spine->getLowerNeck()->setMuscleStiffness(4.f);
        m_spine->getUpperNeck()->setMuscleStiffness(4.f);
      }
      else
      {
        m_spine->getLowerNeck()->resetEffectorCalibrations();
        m_spine->getLowerNeck()->resetEffectorCalibrations();
      }
    }

    rage::Quaternion partQ;
    rage::Vector3 partPos = m_character->getGenericPartByIndex(partIndex)->getPosition();
    rage::Vector3 conPoint = targetM.d;
    targetM.ToQuaternion(partQ);

    if (constraintT)
    {
      if (m_hardConstraintOnCharacter[partIndex])
      {
        m_hardConstraintOnCharacter[partIndex]->SetWorldPosition(conPoint);
      }
      else
      {
        m_character->fixPart(partIndex, conPoint,partPos, 0.f, &m_hardConstraintOnCharacter[partIndex]);
      }
    }

    if (constraintO)
    {
      if (m_hardConstraintRotationOnCharacter[partIndex])
      {
        m_hardConstraintRotationOnCharacter[partIndex]->SetRelativeOrientation(partQ);
      }
      else
      {
        m_character->fixPart(partIndex, conPoint,partPos, 0.f, &m_hardConstraintRotationOnCharacter[partIndex]);
        m_hardConstraintRotationOnCharacter[partIndex]->AddHardRotation();
        m_hardConstraintRotationOnCharacter[partIndex]->SetRelativeOrientation(partQ);
      }
    }
  }

  void NmRsCBUHardConstraint::getIncomingTranformByBodyPart(rage::Matrix34 &matrix, int partIndex, bool blendWithPreviousFrame)
  {
    // find the incoming transform for the pelvious
    int incomingComponentCount = 0;
    NMutils::NMMatrix4 *itPtr = 0;
    IncomingTransformStatus itmStatusFlags = kITSNone;
    m_character->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, kITSourceCurrent);
    if (incomingComponentCount == 0 || itPtr == 0)
      return;
    NMutils::NMMatrix4 &tmPel = itPtr[partIndex];
    matrix.Set(tmPel[0][0], tmPel[0][1], tmPel[0][2], 
      tmPel[1][0], tmPel[1][1], tmPel[1][2],
      tmPel[2][0], tmPel[2][1], tmPel[2][2], 
      tmPel[3][0], tmPel[3][1], tmPel[3][2]);

    float animVelScale = m_character->getEngine()->getIncomingAnimationVelocityScale();
    float invAnimVelScale = 1.f/animVelScale;

    // we want to blend between the previous and the next incoming transform.
    // ONLY WORKS ON POSITION AT THE MOMENT. PLEASE IMPROVE/CHANGE IF NEEDED.
    if (blendWithPreviousFrame)
    {
      float numOfStepsToUpdate = (float)m_parameters.m_FramesSinceTMUpdate;//getEngine()->getNumOfStepsToUpdate();
      // so % we need to interpolate
      float perc = (numOfStepsToUpdate+1.f)/invAnimVelScale;

      // Get the previous incoming transform.
      int incomingComponentCountPre = 0;
      NMutils::NMMatrix4 *itPtrPre = 0;
      IncomingTransformStatus itmStatusFlagsPre = kITSNone;
      m_character->getIncomingTransforms(&itPtrPre, itmStatusFlagsPre, incomingComponentCountPre, kITSourcePrevious);

      NMutils::NMMatrix4 &tmPelPre = itPtrPre[partIndex];
      rage::Matrix34 targetMPre(tmPelPre[0][0], tmPelPre[0][1], tmPelPre[0][2], 
        tmPelPre[1][0], tmPelPre[1][1], tmPelPre[1][2],
        tmPelPre[2][0], tmPelPre[2][1], tmPelPre[2][2], 
        tmPelPre[3][0], tmPelPre[3][1], tmPelPre[3][2]);

      rage::Vector3 offSetDiff = matrix.d - targetMPre.d;

      // Now scale the rotation and the offset by the appropriate amount. 
      offSetDiff.Scale(perc);

      // apply the scalled rotation and offset to yeld the new desired matrix.
      matrix.d = targetMPre.d + offSetDiff;
    }
  }

  void NmRsCBUHardConstraint::applyVelocityOnDeactivation()
  {
    // find the incoming transform for the pelvious
    int incomingComponentCount = 0;
    NMutils::NMMatrix4 *itPtr = 0;
    IncomingTransformStatus itmStatusFlags = kITSNone;
    m_character->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, kITSourceCurrent);
    int incomingComponentCountPrevious = 0;
    NMutils::NMMatrix4 *itPtrPrevious = 0;
    IncomingTransformStatus itmStatusFlagsPrevious = kITSNone;
    m_character->getIncomingTransforms(&itPtrPrevious, itmStatusFlagsPrevious, incomingComponentCountPrevious, kITSourcePrevious);
    if (incomingComponentCount != 0 && itPtr != 0 && incomingComponentCountPrevious != 0 && itPtrPrevious != 0 )
    {
      float timeStepClamped = rage::Max(m_character->getLastKnownUpdateStep(), SMALLEST_TIMESTEP);//protect against division by zero
      for (int i=0;i<m_character->getNumberOfParts();i++)
      {
        NMutils::NMMatrix4 &tmPel = itPtr[i];
        NMutils::NMMatrix4 &tmPelPre = itPtrPrevious[i];
        m_character->getGenericPartByIndex(i)->applyVelocitiesToPart(tmPelPre,tmPel,1.0f/timeStepClamped,false);
      }
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUHardConstraint::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_FramesSinceTMUpdate, true);  
    bspyTaskVar(m_parameters.m_applyTeleporation, true);
    bspyTaskVar(m_parameters.m_stabiliseHead, true);
    bspyTaskVar(m_parameters.m_telportedVelOnDeactivation, true);
    bspyTaskVar(m_parameters.m_blendWithPreviousFrame[0], true);
  }
#endif // ART_ENABLE_BSPY
}
