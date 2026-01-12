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
#include "ART/ARTFeedback.h"

namespace ART
{
     //----------------FLING------------------------------------------------
    enum 
    {
      Region_Upper,
      Region_LegsLeft,
      Region_LegsRight,
      Region_Arms,
      Region_Body
    };
    bool NmRsCBUShot::fling_entryCondition()
    {
      return m_parameters.fling && m_newHit;
    }
    void NmRsCBUShot::fling_entry()
    {
      NM_RS_DBG_LOGF(L"Fling Entry");
      m_fling.flingTimer = 0;
      m_fling.useRight = true;
      m_fling.useLeft = true;
      m_fling.bodyRegion = Region_Upper;

      // Make decisions about what to do:
      m_fling.backLeanDir = 0.f;
      m_fling.leftDir = 0.f;
      m_fling.rightDir = 0.f;

      if (m_falling)
      {
        getSpine()->setBodyStiffness(getSpineInput(), 15.0f, 0.5f);

        m_fling.useLeft = false;
        m_fling.useRight = false;
        m_fling.backLeanDir = 3.f;
      }
      else
      {
        NmRsGenericPart *part = m_character->getGenericPartByIndex(m_parameters.bodyPart);
        if (part == getLeftLeg()->getFoot() || part==getLeftLeg()->getShin())
        {
          m_fling.bodyRegion = Region_LegsLeft;
          m_disableBalance = true;
        }
        else if (part==getRightLeg()->getShin() || part==getRightLeg()->getFoot())
        {
          m_fling.bodyRegion = Region_LegsRight;
          m_disableBalance = true;
        }
        else if (part==getLeftArm()->getHand() || part==getRightArm()->getHand() || 
                 part==getLeftArm()->getLowerArm() || part==getRightArm()->getLowerArm())
        {
          m_fling.bodyRegion  = Region_Arms;
        }
        else
        {
          m_fling.bodyRegion = Region_Body;
          m_fling.backLeanDir = 1.f;
          m_fling.leftDir = 1.f;
          m_fling.rightDir = 1.f;
        }
      }

      // based on the direction of shot, front/back and left/right
      rage::Vector3 bulletDir = m_parameters.bulletVel;
      bulletDir.Normalize();
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "shot.fling", bulletDir);
#endif
      rage::Matrix34 spine2Mat;
      getSpine()->getSpine2Part()->getMatrix(spine2Mat);
      rage::Vector3 sideV = spine2Mat.b;
      m_fling.sAngle = bulletDir.Dot(sideV); // the dot from side to the shot direction in the spine2 y*z plane
      sideV = spine2Mat.c;
      m_fling.fAngle = bulletDir.Dot(sideV);  // the dot from forward to the shot direction in the spine2 y*z plane
      
      // bigger from the front/back
      m_fling.period = m_reactionTime/3.f;
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "shot.fling", m_fling.fAngle);
    bspyScratchpad(m_character->getBSpyID(), "shot.fling", m_fling.sAngle);
#endif

      m_spineStiffness = m_parameters.bodyStiffness * 12.f/m_defaultBodyStiffness;
      m_armsStiffness  = m_parameters.bodyStiffness * 14.f/m_defaultBodyStiffness;
      m_neckStiffness  = m_parameters.neckStiffness * 10.f/m_defaultBodyStiffness;
      m_wristStiffness = m_parameters.bodyStiffness * 10.f/m_defaultBodyStiffness;

      getLeftArmInputData()->getShoulder()->setOpposeGravity(1.f);
      getLeftArmInputData()->getClavicle()->setOpposeGravity(1.f);
      getLeftArmInputData()->getElbow()->setOpposeGravity(1.f);
      getRightArmInputData()->getShoulder()->setOpposeGravity(1.f);
      getRightArmInputData()->getClavicle()->setOpposeGravity(1.f);
      getRightArmInputData()->getElbow()->setOpposeGravity(1.f);
    }
    void NmRsCBUShot::fling_tick(float timeStep)
    {
      m_fling.flingTimer += timeStep;
      if (m_falling)
      {
        float stiff = 0.5f;
        m_body->setStiffness(16.0f, 0.5f, bvmask_Spine, &stiff);
        m_body->setStiffness(12.0f, 0.5f, bvmask_ArmLeft | bvmask_ArmRight, &stiff);
        getSpineInputData()->applySpineLean(7.f*m_fling.fAngle,-m_fling.sAngle);
      }
      else
      {
        // calculate an rotation offset based on the initial position of the arms
        rage::Vector3 Faxis = m_character->m_COMTM.a;
        rage::Vector3 toHitP = m_hitPointWorld - m_character->m_COM;
        m_character->levelVector(toHitP);
        m_character->levelVector(Faxis);
        Faxis.Normalize();
        toHitP.Normalize();
        float rightAngleOffset = -toHitP.Dot(Faxis)/4.f;
        float leftAngleOffset = -rightAngleOffset;
        
        float wt = 0.f;
        float swing = 0.f;
        float stiff = 0.25f;
        m_body->setStiffness(14.0f, 0.5f, bvmask_ArmLeft | bvmask_ArmRight, &stiff);
        getLeftArmInputData()->getWrist()->setStiffness(12.0f, 0.5f);
        getRightArmInputData()->getWrist()->setStiffness(12.0f, 0.5f);

        if (m_fling.useLeft)
        {
          wt = rage::Min(rage::Abs(m_fling.leftDir)*(PI*m_fling.flingTimer/m_fling.period + leftAngleOffset+.7f),5.f); 
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "shot.fling.left", wt);
        bspyScratchpad(m_character->getBSpyID(), "shot.fling.left", leftAngleOffset);
#endif
          if (wt==5.f)
            m_fling.useLeft = false;
          wt = rage::Clamp(wt,0.f,PI-.3f);
          swing = rage::Abs(m_fling.leftDir)*rage::Min(4.f-wt,.32f); 
          getLeftArmInputData()->getElbow()->setDesiredAngle(2.f*swing*(1.f-m_parameters.flingWidth)+0.5f);
          getLeftArmInputData()->getClavicle()->setDesiredLean1(1.2f*rage::Sinf(wt)+(3.6f/4.f*.5f*m_parameters.flingWidth)-1.0f);
          getLeftArmInputData()->getClavicle()->setDesiredLean2((1.f-m_parameters.flingWidth)*rage::Cosf(wt)-.5f);
          getLeftArmInputData()->getShoulder()->setDesiredLean1(1.2f*rage::Sinf(wt)-0.6f);
          getLeftArmInputData()->getShoulder()->setDesiredLean2(1.65f*(1.f-m_parameters.flingWidth)*rage::Cosf(wt));
          getLeftArmInputData()->getShoulder()->setDesiredTwist(-2.5f*rage::Sinf(wt)-1.f);
        }
        else
          getLeftArm()->setBodyStiffness(getLeftArmInput(), 9.0f, 0.5f);

        if (m_fling.useRight)
        {
          wt = rage::Min(rage::Abs(m_fling.rightDir)*(PI*m_fling.flingTimer/m_fling.period + rightAngleOffset+.7f),4.5f); 
          if (wt==4.5f)
            m_fling.useRight = false;
          wt = rage::Clamp(wt,0.f,PI-.1f);
          swing = rage::Abs(m_fling.rightDir)*rage::Min(4.f-wt,.32f);
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "shot.fling.right", wt);
#endif

          getRightArmInputData()->getElbow()->setDesiredAngle(2.f*swing*(1.f-m_parameters.flingWidth)+0.5f);
          getRightArmInputData()->getClavicle()->setDesiredLean1(.7f*rage::Sinf(wt)+(3.6f/4.f*.5f*m_parameters.flingWidth)-1.0f);
          getRightArmInputData()->getClavicle()->setDesiredLean2((1.f-m_parameters.flingWidth)*rage::Cosf(wt)-.5f);
          getRightArmInputData()->getShoulder()->setDesiredLean1(.8f*rage::Sinf(wt)-0.6f);
          getRightArmInputData()->getShoulder()->setDesiredLean2(1.65f*(1.f-m_parameters.flingWidth)*rage::Cosf(wt));
          getRightArmInputData()->getShoulder()->setDesiredTwist(-2.5f*rage::Sinf(wt)-1.f);
        }
        else
          getRightArm()->setBodyStiffness(getRightArmInput(), 9.0f, 0.5f);
        getSpineInputData()->getLowerNeck()->setDesiredTwist(-0.3f*rage::Sinf(wt));

        // Back
        // if hit in the body and from behind, do the 'shot from behind' spine snap
        if(m_fling.bodyRegion == Region_Body)
        {
          // get hit normal in spine3 local (it is stored local to the hit part)
          rage::Matrix34 mat;
          m_character->getGenericPartByIndex(m_parameters.bodyPart)->getMatrix(mat);
          rage::Vector3 normal;
          mat.Transform3x3(m_hitNormalLocal, normal);
          getSpine()->getSpine3Part()->getMatrix(mat);
          mat.UnTransform3x3(normal);
          if(normal.z < 0.f)
          {
            getSpine()->setBodyStiffness(getSpineInput(), 13.0f, 0.5f, bvmask_LowSpine);
            wt = (1.f-m_fling.flingTimer/m_fling.period);
            getSpineInputData()->applySpineLean(-2.f * wt, 0.f); // * wt);
            if(!m_parameters.useHeadLook)
            {
              getSpine()->setBodyStiffness(getSpineInput(), 13.f ,0.5f, bvmask_HighSpine);
              getSpineInputData()->getLowerNeck()->setDesiredLean1(-2.f * wt);
              getSpineInputData()->getUpperNeck()->setDesiredLean1(-2.f * wt);
            }
          }
        }
        // otherwise do whatever was here before
        else
        {
          getSpineInputData()->applySpineLean(-0.3f*m_fling.backLeanDir*m_fling.fAngle, 0.15f*m_fling.sAngle);
          getSpineInputData()->getLowerNeck()->setDesiredLean1(-m_fling.backLeanDir*rage::Abs(m_fling.fAngle));
          getSpineInputData()->getUpperNeck()->setDesiredLean1(-m_fling.backLeanDir*rage::Abs(m_fling.fAngle));
          getSpineInputData()->getLowerNeck()->setDesiredLean2(m_fling.sAngle);
          getSpineInputData()->getUpperNeck()->setDesiredLean2(m_fling.sAngle);
        }

       // reaction for getting hit in the legs, turns back on the dynamic balance after .2 of a sec, 
       if (m_fling.bodyRegion == Region_LegsLeft && m_fling.useLeft)
       {
         rage::Vector3 fHeight = getLeftLeg()->getFoot()->getPosition();
         fHeight += m_character->m_gUp * 0.6f;
         NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);
         NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
         ikInputData->setTarget(fHeight);
         ikInputData->setTwist(0.0f);
         ikInputData->setDragReduction(1.0f);
         getLeftLeg()->postInput(ikInput);
       }
       else if (m_fling.bodyRegion == Region_LegsRight && m_fling.useRight)
       {
         rage::Vector3 fHeight = getRightLeg()->getFoot()->getPosition();
         fHeight += m_character->m_gUp * 0.6f;
         NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);
         NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
         ikInputData->setTarget(fHeight);
         ikInputData->setTwist(0.0f);
         ikInputData->setDragReduction(1.0f);
         getRightLeg()->postInput(ikInput);
       }
       if (m_fling.flingTimer > .2f && (m_fling.bodyRegion == Region_LegsRight || m_fling.bodyRegion == Region_LegsLeft))
         m_disableBalance = false;
      }
    }
    bool NmRsCBUShot::fling_exitCondition()
    {
      return (m_hitTime > m_reactionTime) || m_newHit;
    }
    void NmRsCBUShot::fling_exit()
    {
      getSpineInputData()->getUpperNeck()->setDesiredLean1(0.f);
      getSpineInputData()->getUpperNeck()->setDesiredLean2(0.f);
      getSpineInputData()->getUpperNeck()->setDesiredTwist(0.f);
      getSpineInputData()->getLowerNeck()->setDesiredLean1(0.f);
      getSpineInputData()->getLowerNeck()->setDesiredLean2(0.f);
      getSpineInputData()->getLowerNeck()->setDesiredTwist(0.f);
      getSpineInputData()->applySpineLean(0.f,0.f);

      m_spineStiffness = 12.f;
      m_armsStiffness  = 12.f;
      m_neckStiffness  = m_parameters.neckStiffness;
      m_wristStiffness = 17.f;
    }
}

