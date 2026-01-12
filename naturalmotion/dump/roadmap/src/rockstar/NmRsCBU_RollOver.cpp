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
 * Rolling motion. Apply when the character is to be tumbling, either down a hill,
 * after being blown by an explosion, or hit by a car.
 * The character is in a rough foetal position, he puts his arms out to brace against
 * collisions with the ground, and he will relax after he stops tumbling.
 *
 * Roll Up behaviour. This is a pre-condition type behaviour which has to occur
 * before the character starts rolling or tumbling. Rolling and tumbling are
 * largely dependent on the shape of the character before it hits the ground/terrain
 * and the slope etc. of the ground/terrain. Part of the post-ground impact suite of behaviours.
 *
 * TDL current implementation simple curls the character up in proportion to how fast he's rotating
 * This behaviour is designed to transition in from a fall behaviour as the character approaches the ground
 */


#include "NmRsInclude.h"
#include "NmRsCBU_RollOver.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_BodyFoetal.h" 

namespace ART
{
     NmRsCBURollOver::NmRsCBURollOver(ART::MemoryManager* services) :
      CBUTaskBase(services, bvid_rollOver),
      m_leftArm(0),
      m_rightArm(0),
      m_leftLeg(0),
      m_rightLeg(0),
      m_spine(0),
      m_painState(kRelaxed)
    {
      initialiseCustomVariables();
    }

    NmRsCBURollOver::~NmRsCBURollOver()
    {
    }

    void NmRsCBURollOver::initialiseCustomVariables()
    {
      m_mask = bvmask_Full;

      m_rollOverTimer       = 0.0f;
    }

    void NmRsCBURollOver::onActivate()
    {
      Assert(m_character);

      m_rollOverTimer = 0;
      m_relaxTimer = m_character->getRandom().GetRanged(0.f, 5.f);

      // locally cache the limb definitions
      m_leftArm = m_character->getLeftArmSetup();
      m_rightArm = m_character->getRightArmSetup();
      m_leftLeg = m_character->getLeftLegSetup();
      m_rightLeg = m_character->getRightLegSetup();
      m_spine = m_character->getSpineSetup();

     //m_failed = false;

    }

    void NmRsCBURollOver::onDeactivate()
    {
      Assert(m_character);

      initialiseCustomVariables();
      m_leftArm       = NULL;
      m_rightArm      = NULL;
      m_leftLeg       = NULL;
      m_rightLeg      = NULL;
      m_spine         = NULL;
    }

    CBUTaskReturn NmRsCBURollOver::onTick(float timeStep)
    {
      // Increment behavior-level timer
      m_rollOverTimer += timeStep;

      NM_RS_DBG_LOGF(L"during");

      // Gather directional data
      GatherStateData();

      // Return early if not on the ground
      if (!IsOnGround())
        return eCBUTaskComplete;

      // Rolling in pain?
      if (m_parameters.m_rollingInPain)
      {
        if (m_painState == kRelaxed)
        {
          m_character->setBodyStiffness(3.0f,1.0f,"fb");

          // Update relaxed timer
          static float m_finishingTime = 2.0f;
          m_finishingTimer = m_finishingTime;
          m_relaxTimer -= timeStep;
          if (m_relaxTimer < 0.0f)
          {
            if (m_onLeftSide)
              m_painState = kRollRight;
            else if (m_onRightSide)
              m_painState = kRollLeft;
            m_relaxTimer = m_character->getRandom().GetRanged(0.f, 5.f);
          }
        }
        else  
        {
          // Stiffen the body but keep the arms loose
          m_character->setBodyStiffness(3.0f,1.0f,"ua");
          m_character->setBodyStiffness(10.0f,1.0f,"fb");
          m_character->setBodyStiffness(20.0f,1.0f,"us");

          // Grab the side vector
          rage::Vector3 toSide;
          toSide.Cross(m_bodyUp, m_character->m_gUp);
          toSide.Normalize();

          // Rolling strategy - push that side's ankle into the ground
          if (m_painState == kRollLeft)
          {
            // Twist the back to the left
            static float twist = 5.0f;
            nmrsSetTwist(m_spine->getSpine0(), twist);
            nmrsSetTwist(m_spine->getSpine1(), twist);
            nmrsSetTwist(m_spine->getSpine2(), twist);
            nmrsSetTwist(m_spine->getSpine3(), twist);

            float fac = 0.1f * (cos(8.0f * m_rollOverTimer)-1.0f) - 0.2f;
            rage::Vector3 goalPos = m_spine->getPelvisPart()->getPosition() + 
              toSide * -0.2f + m_bodyUp * -0.4f + m_character->m_gUp * fac;

            m_rightLeg->getHip()->setStiffness(20.0f, 1.0f);
            m_rightLeg->getKnee()->setStiffness(20.0f, 1.0f);
            m_character->rightLegIK(goalPos);

            nmrsSetLean2(m_leftLeg->getHip(), -2.0f);
            nmrsSetAngle(m_leftLeg->getKnee(), -2.0f);

            // Check exit conditions
            float dot = m_bodySide.Dot(m_character->m_gUp);
            if (dot > 0.7f)
            {
              m_painState = kRelaxed;
            }
          }
          else // m_painState == kRollRight
          {
            // Twist the back to the right
            static float twist = -5.0f;
            nmrsSetTwist(m_spine->getSpine0(), twist);
            nmrsSetTwist(m_spine->getSpine1(), twist);
            nmrsSetTwist(m_spine->getSpine2(), twist);
            nmrsSetTwist(m_spine->getSpine3(), twist);

            float fac = 0.1f * (cos(8.0f * m_rollOverTimer)-1.0f) - 0.2f;
            rage::Vector3 goalPos = m_spine->getPelvisPart()->getPosition() + 
              toSide * 0.2f + m_bodyUp * -0.4f + m_character->m_gUp * fac;

            m_leftLeg->getHip()->setStiffness(20.0f, 1.0f);
            m_leftLeg->getKnee()->setStiffness(20.0f, 1.0f);
            m_character->leftLegIK(goalPos);

            nmrsSetLean2(m_rightLeg->getHip(), -2.0f);
            nmrsSetAngle(m_rightLeg->getKnee(), -2.0f);

            // Check exit conditions
            float dot = m_bodySide.Dot(m_character->m_gUp);
            if (dot < -0.7f)
            {
              m_painState = kRelaxed;
            }
          }
        }
      }

      return eCBUTaskComplete;
    } 

    void NmRsCBURollOver::GatherStateData()
    {
      // determine character orientation
      rage::Matrix34 tmCom;
      static bool useCOMorient = true;
      if (useCOMorient) 
      {
        tmCom.Set(m_character->m_COMTM);
        m_bodySide = tmCom.a;  // faces right
        m_bodyUp = tmCom.b;
        m_bodyBack = tmCom.c;
      }
      else 
      {
        m_spine->getSpine1Part()->getBoundMatrix(&tmCom); 
        m_bodyUp = tmCom.a;
        m_bodySide = -tmCom.b;
        m_bodyBack = tmCom.c;
      }

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(),"RollOver",m_bodyUp);
      bspyScratchpad(m_character->getBSpyID(),"RollOver",m_bodyBack);
      bspyScratchpad(m_character->getBSpyID(),"RollOver",m_bodySide);
#endif

      // determine roll state (face up, face down, right side, left side)
      m_downFacing = (m_bodyBack.Dot(m_character->m_gUp) > 0.5f);
      m_upFacing = (m_bodyBack.Dot(m_character->m_gUp) < -0.6f);
      m_onLeftSide = (m_bodySide.Dot(m_character->m_gUp) > 0.4f);
      m_onRightSide = (m_bodySide.Dot(m_character->m_gUp) < -0.4f);

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(),"RollOver",m_downFacing);
      bspyScratchpad(m_character->getBSpyID(),"RollOver",m_upFacing);
      bspyScratchpad(m_character->getBSpyID(),"RollOver",m_onLeftSide);
      bspyScratchpad(m_character->getBSpyID(),"RollOver",m_onRightSide);
#endif

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(),"RollOver",IsOnGround());
#endif
    }

    bool NmRsCBURollOver::IsOnGround()
    {
      int numCollParts = 0;
      for (int i = 0; i<m_character->getNumberOfParts(); i++)
      {
        NmRsGenericPart* part = m_character->getGenericPartByIndex(i);
        if (part->collidedWithEnvironment())
          numCollParts++;
      }
      static int maxCollParts = 5;
      return numCollParts > maxCollParts;
    }

#if ART_ENABLE_BSPY
    void NmRsCBURollOver::sendParameters(NmRsSpy& spy)
    {
      CBUTaskBase::sendParameters(spy);

      bspyTaskVar(m_rollOverTimer, false);

      bspyTaskVar(m_parameters.m_rollingInPain, true);
      bspyTaskVar(m_parameters.m_rollingInPainIntensity, true);
    }
#endif // ART_ENABLE_BSPY
}
