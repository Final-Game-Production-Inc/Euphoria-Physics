/*
 * Copyright (c) 2005-2010 NaturalMotion Ltd. All rights reserved. 
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
#include "NmRsCBU_HeadLook.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_SpineTwist.h"

namespace ART
{
     //----------------MELEE------------------------------------------------
    bool NmRsCBUShot::melee_entryCondition()
    {
      return m_parameters.melee && m_newHit;
    }
    void NmRsCBUShot::melee_entry()
    {
      m_melee.motionMultiplier = 0.f;
      m_melee.neckTilt = 0.f;

      // left elbow
      m_melee.LESwingMin  = getLeftArm()->getElbow()->getMinAngle();
      m_melee.LESwingMax  = getLeftArm()->getElbow()->getMaxAngle() * 0.2f;

      // right elbow
      m_melee.RESwingMin  = getRightArm()->getElbow()->getMinAngle();
      m_melee.RESwingMax  = getRightArm()->getElbow()->getMaxAngle() * 0.35f;

      m_melee.hipYaw = m_character->getCharacterConfiguration().m_hipYaw;
      m_melee.headYaw = m_character->getCharacterConfiguration().m_headYaw;

      m_melee.hipangle = 2.f*m_melee.hipYaw;
      m_melee.bodyangle = 2.f*m_melee.hipYaw;
      m_melee.headangle = m_melee.headYaw;

      m_melee.headLookyTimer = 0.4f;

      getSpineInputData()->setBackAngles(0.f,0.f,0.f);
    }
    void NmRsCBUShot::melee_tick(float timeStep)
    {
      NmRsCBUSpineTwist *spineTwist = (NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist];      
      Assert(spineTwist);
      if (!spineTwist->isActive())
      {
        spineTwist->initialiseCustomVariables();
        spineTwist->setSpineTwistPos(m_parameters.headLookPos);
        //spineTwist->setSpineTwistStiffness(m_parameters.bodyStiffness);
        spineTwist->setSpineTwistAllwaysTwist(true);
        spineTwist->activate();
      }

      // update the looking at the attacker position 
      NmRsCBUHeadLook *headLook = (NmRsCBUHeadLook *)m_cbuParent->m_tasks[bvid_headLook];      
      Assert(headLook);
      if (!headLook->isActive())
      {
        headLook->updateBehaviourMessage(NULL);
        headLook->activate();
      }

      headLook->m_parameters.m_instanceIndex = (-1);

      if (m_melee.headLookyTimer >0.f){
          m_melee.headLookyTimer -= m_character->getLastKnownUpdateStep();
    
          rage::Vector3 tempHeadLookPos = getSpine()->getHeadPart()->getPosition();
          rage::Matrix34 headTM;
          getSpine()->getHeadPart()->getMatrix(headTM);
          rage::Vector3 backV = headTM.c;
          backV.Scale(-5.f);
          tempHeadLookPos = tempHeadLookPos + backV;
          headLook->m_parameters.m_pos = (tempHeadLookPos);
      }
      else
      {
           headLook->m_parameters.m_pos = (m_parameters.headLookPos);
      }

      // based on the attacker position and current foot/hip/shoulder orientation choose the new orientations to drive to 
      rage::Vector3 attackDir = m_parameters.headLookPos;
      rage::Vector3 currentPos = m_character->m_COM;
      attackDir -= currentPos;
      attackDir.Normalize();

      rage::Vector3 currentPelDir;
      rage::Matrix34 pelTm;
      getSpine()->getPelvisPart()->getMatrix(pelTm);
      currentPelDir = -pelTm.c;

      // work out the angle in the horizontal plane between the currentPelDir and the attackDir
      // drop down into the ground plane
      m_character->levelVector(attackDir);
      attackDir.Normalize();
      m_character->levelVector(currentPelDir);
      currentPelDir.Normalize();

      rage::Vector3 origonPos = currentPos;
      //float offsetAngle = rage::AcosfSafe(attackDir.Dot(currentPelDir));
      rage::Vector3 tempV;
      tempV.Cross(attackDir,currentPelDir);
//      float upordown = tempV.Dot(m_character->m_gUp);
      //offsetAngle *= upordown/rage::Abs(upordown);
      //NM_RS_DBG_LOGF(L" The angle between the target and front = %.3f", offsetAngle);
      
      // based on the desired angles choose the desired foot/hip/shoulder targets
      // the foot angle should be:
      //float desiredFootAngle = -m_melee.headangle; // -(upordown/math.rage::Abs(upordown))*headangle
      // rotate attackDir about origon by desiredFootAngle
      //rage::Vector3 desiredFootTarget;
      rage::Matrix34 rotMat;
      rage::Vector3 up = m_character->m_gUp;
     // rotMat.Identity();
     // rotMat.FromEulersXYZ(rage::Vector3(desiredFootAngle*up.x,desiredFootAngle*up.y,desiredFootAngle*up.z));
      //rotMat.Transform3x3(attackDir, desiredFootTarget);
      //desiredFootTarget += origonPos;


      // the Hip angle should be:
      float desiredHipAngle = -m_melee.headangle+m_melee.hipangle; //-(upordown/math.rage::Abs(upordown))*(headangle-hipangle)
      // rotate attackDir about origon by desiredFootAngle
      rage::Vector3 desiredHipTarget;
      rotMat.Identity();
      rotMat.FromEulersXYZ(rage::Vector3(desiredHipAngle*up.x,desiredHipAngle*up.y,desiredHipAngle*up.z));
      rotMat.Transform3x3(attackDir, desiredHipTarget);

      NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];      
      dynamicBalancerTask->useCustomTurnDir(true, desiredHipTarget);
	  if (!m_fTK.m_bendLegs)
      dynamicBalancerTask->setLegStraightnessModifier(0.f);
      dynamicBalancerTask->setHipPitch(0.f);


      desiredHipTarget += origonPos;
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "shot.melee", desiredHipTarget);
#endif

      // the Shoulder angle should be:
      float desiredShoulderAngle = -m_melee.headangle+m_melee.bodyangle; //-(upordown/math.rage::Abs(upordown))*
      // rotate attackDir about origon by desiredFootAngle
      rage::Vector3 desiredShoulderTarget;
      rotMat.Identity();
      rotMat.FromEulersXYZ(rage::Vector3(desiredShoulderAngle*up.x,desiredShoulderAngle*up.y,desiredShoulderAngle*up.z));
      rotMat.Transform3x3(attackDir, desiredShoulderTarget);
      desiredShoulderTarget += origonPos;

      spineTwist->setSpineTwistPos(desiredShoulderTarget);
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "shot.melee", desiredShoulderTarget);
#endif

      // then blend to zero pose as we stop moving
      m_melee.motionMultiplier = (m_character->m_COMvelRelativeMag - .1f*m_melee.motionMultiplier) * (timeStep * 120.0f);
      
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "shot.melee", m_melee.motionMultiplier);
#endif

      if (m_melee.motionMultiplier > 1.0f)
        m_melee.motionMultiplier = 1.0f;
      if (m_melee.motionMultiplier < 0.1f)
        m_melee.motionMultiplier = 0.001f;

#if 0
      getSpine()->getSpine0()->setDesiredLean1(0.f);
      getSpine()->getSpine0()->setDesiredLean2(0.f);

      getSpine()->getSpine1()->setDesiredLean1(0.f);
      getSpine()->getSpine1()->setDesiredLean2(0.f);

      getSpine()->getSpine2()->setDesiredLean1(0.f);
      getSpine()->getSpine2()->setDesiredLean2(0.f);

      getSpine()->getSpine3()->setDesiredLean1(0.f);
      getSpine()->getSpine3()->setDesiredLean2(0.f);
#endif

    getSpineInputData()->applySpineLean(0.f, 0.f);
    getSpine()->blendToZeroPose(getSpineInput(), 1.f-.5f*m_melee.motionMultiplier, bvmask_LowSpine);

    float rampDuration = 4.9f;
    float hitTimerClamped = rage::Clamp(m_hitTime,0.f,rampDuration);
    float blendAmount = rage::Clamp(rage::Sinf(hitTimerClamped*(3.15f/(2.f*rampDuration))),0.f,1.f);

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "shot.melee", blendAmount);
    bspyScratchpad(m_character->getBSpyID(), "shot.melee", m_melee.motionMultiplier);
#endif

    float tempBlendAmount = blendAmount;
    if (m_hitTime>0.7f)
      tempBlendAmount = 1.0f-0.65f*m_melee.motionMultiplier;
    getLeftArm()->blendToZeroPose(getLeftArmInput(), tempBlendAmount);
    getRightArm()->blendToZeroPose(getRightArmInput(), tempBlendAmount);
    }

    bool NmRsCBUShot::melee_exitCondition()
    {
      return m_falling || m_newHit;
    }
    void NmRsCBUShot::melee_exit()
    {
      NmRsCBUSpineTwist *spineTwist = (NmRsCBUSpineTwist *)m_cbuParent->m_tasks[bvid_spineTwist];      
      Assert(spineTwist);
      if (spineTwist->isActive())
        spineTwist->deactivate();
    }

}

