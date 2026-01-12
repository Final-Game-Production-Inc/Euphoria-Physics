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
 * Wave the arms around in a windmilling motion; 
 * this behaviour rotates the arms based on which way the character is rotating (around the somersault axis).
 *
 * m_bodyStiffness:  Controls how stiff the rest of the body is
 * armStiffness:  How stiff the arms are controls how pronounced the windmilling motion appears; smaller values means weaker movement
 * angSpeed:  Controls the speed of the windmilling
 * amplitude:  Controls how large the motion is, higher values means the character waves his arms in a massive arc
 * phase:  Set to a non-zero value to desynchronise the left and right arms motion.
 * disableOnImpact:  If true, each arm will stop windmilling if it hits the ground
 * 
 */


#include "NmRsInclude.h"
#include "NmRsCBU_ArmsWindmillAdaptive.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

namespace ART
{
  NmRsCBUArmsWindmillAdaptive::NmRsCBUArmsWindmillAdaptive(ART::MemoryManager* services) : CBUTaskBase(services, bvid_armsWindmillAdaptive)
  {
    initialiseCustomVariables();
  }

  NmRsCBUArmsWindmillAdaptive::~NmRsCBUArmsWindmillAdaptive()
  {
  }

  void NmRsCBUArmsWindmillAdaptive::initialiseCustomVariables()
  {
    m_mask = bvmask_UpperBody;

    m_armsWindmillAdaptiveAngle     = 0.0f;
  }

  void NmRsCBUArmsWindmillAdaptive::onActivate()
  {
    Assert(m_character);
    m_mask = m_parameters.effectorMask;

    m_armsWindmillAdaptiveAngle = m_character->getRandom().GetRanged(0.0f, 5000.0f);//mmmmtodo pick closest point to circle?-0.f;

    // Elbow.
    m_ElbowBend      = PI/12.0f;
    m_Elbow_w         = 15.0f;

    // Parameters.
    m_bodyDamping = 0.5f;
    m_armDamping = 1.0f;
    
    //Initializations
    m_leftHandCollided = false;
    m_doLeftArm = true;
    m_rightHandCollided = false;
    m_doRightArm = true;

    m_body->setStiffness(m_parameters.bodyStiffness, m_bodyDamping, m_parameters.effectorMask, NULL, true);
  }

  void NmRsCBUArmsWindmillAdaptive::onDeactivate()
  {
    Assert(m_character);
    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUArmsWindmillAdaptive::onTick(float timeStep)
  {
    m_mask = m_parameters.effectorMask;
    // Determine if the arms are to perform the windmilling.
    if (m_parameters.disableOnImpact)
    {
      if ((getLeftArm()->getHand()->collidedWithNotOwnCharacter()) && !m_leftHandCollided)
        m_leftHandCollided = true;
    }

    if (m_parameters.disableOnImpact && m_leftHandCollided && m_doLeftArm) 
      m_doLeftArm = false;

    if (m_parameters.disableOnImpact)
    {
      if ((getRightArm()->getHand()->collidedWithNotOwnCharacter()) && !m_rightHandCollided)
        m_rightHandCollided = true;
    }

    if (m_parameters.disableOnImpact && m_rightHandCollided && m_doRightArm)
      m_doRightArm = false;

    if (m_doLeftArm || m_doRightArm)
    {
      getSpine()->setBodyStiffness(getSpineInput(), m_parameters.bodyStiffness, m_bodyDamping);
      getLeftArm()->setBodyStiffness(getLeftArmInput(), m_parameters.armStiffness, m_armDamping);
      getRightArm()->setBodyStiffness(getRightArmInput(), m_parameters.armStiffness, m_armDamping);

      //Set arm circling direction
      // Backwards = -1
      // Forwards = 1
      //Adaptive = 0
      float sign = (float) m_parameters.armDirection;
      //Automatically change circling direction based on angMom or angVel
      if (m_parameters.armDirection == 0)
      {
        float cavD = m_character->m_COMrotvel.Dot(m_character->m_COMTM.a);//bodyRight
      if (m_parameters.useAngMom)
      {
        rage::Matrix34 tmCom(m_character->m_COMTM);
        rage::Vector3 bodyBack = tmCom.c;
        rage::Vector3 bodyRight = tmCom.a;
        float somersaultAngMom = bodyRight.Dot(m_character->m_angMom);//+ve is forward
        NM_RS_DBG_LOGF(L"somersaultAngMom: %f", somersaultAngMom);

        float qSom;
        float arcsin = bodyBack.z;
        qSom = rage::AsinfSafe(arcsin);
        NM_RS_DBG_LOGF(L"qSom: %f", qSom);
        qSom *= 6.8f;
        if (somersaultAngMom>12.f || somersaultAngMom<-12.f || 
          (qSom < 0.f && somersaultAngMom > 0.f) || 
          (qSom > 0.f && somersaultAngMom < 0.f) )
          somersaultAngMom -= qSom;
        else
          somersaultAngMom = -qSom;

        //rage::Vector3 forward;
        //forward.Cross(bodyRight,m_character->m_gUp);
        //float forwardness = rage::Clamp(forward.Dot(m_character->m_COMvel),-10.f,10.f);
        //somersaultAngMom += 3.f*forwardness;

        cavD = somersaultAngMom;
        NM_RS_DBG_LOGF(L"somersaultAngMom2: %f", somersaultAngMom);
        NM_RS_DBG_LOGF(L"qSom2: %f", qSom);

      }    
        sign = rage::Selectf(cavD, -1.0f, 1.0f);// Falling forwards(sign+ve). Falling backwards(sign -ve).
      }

      // Update the time.
      //float mult = 1.f;//could mult time at different segments of circle to speed/slow that segment
      m_armsWindmillAdaptiveAngle += m_parameters.angSpeed * sign * timeStep;// * mult;
      bool forwards = sign == 1 ? true:false;
      if (m_doLeftArm)
        moveArm(timeStep, getLeftArm(), getLeftArmInputData(), m_armsWindmillAdaptiveAngle, m_parameters.leftElbowAngle, m_parameters.bendLeftElbow, forwards);
      if (m_doRightArm)
        moveArm(timeStep, getRightArm(), getRightArmInputData(), m_armsWindmillAdaptiveAngle + m_parameters.phase, m_parameters.rightElbowAngle, m_parameters.bendRightElbow, forwards);

      // Back
      if (m_parameters.setBackAngles)
      {
        float sinAngle = rage::Sinf(m_armsWindmillAdaptiveAngle);
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "WindmillAdaptive", sinAngle);
#endif
        getSpineInputData()->setBackAngles(0.0f, m_parameters.swayAmount*-sinAngle, 0.4f*sinAngle);
        getSpineInputData()->getLowerNeck()->setDesiredTwist(-0.3f*sinAngle);
      }
    }
    else
    {
      // appears to be relaxing when his arms have been disabled. higher level
      // logic should be taking care of this.
      //
    }

    return eCBUTaskComplete;
  }

  void NmRsCBUArmsWindmillAdaptive::moveArm(float timeStep, NmRsHumanArm *arm, NmRsArmInputWrapper* inputData, float angle, float elbowBlendAngle, bool bendElbow, bool forwards)
  {
    float c, s;
    rage::cos_and_sin(c, s, angle);
    // Clavicle motion.
    inputData->getClavicle()->setDesiredAngles(
      -m_parameters.amplitude * (0.25f + 0.75f * s),//-ve shoulders back
      m_parameters.amplitude * c - 0.15f,//-ve shoulders up
      0.f);//+ve rotate down - Twist is set otherwize other behaviours e.g. writhe leave this in a twisted state

    // Shoulder motion.
    inputData->getShoulder()->setDesiredAngles(
      -m_parameters.lean1mult * m_parameters.amplitude * s - 0.3f + m_parameters.lean1offset,//frankenstein +ve
      m_parameters.amplitude * c,//flapping +ve towards torso
      0.f);//Twist is set otherwize other behaviours e.g. writhe leave this in a twisted state

    //When elbowBlendAngle(m_parameters.leftElbowAngle/m_parameters.rightElbowAngle) < 0.0
    //mmmmTodo elbow bend is not set by armswindmillAdaptive unless < m_ElbowBend = 15deg.
    //  meaning bent elbows will be preserved - is that really what we want?
    // Elbow bend for "visual" effect during behaviour.
    float elbowAngle = nmrsGetActualAngle(arm->getElbow());
    float elbowSwingMin = arm->getElbow()->getMinAngle();
    float elbowSwingMax = arm->getElbow()->getMaxAngle();
    if (elbowAngle < m_ElbowBend)
    { 
      elbowAngle = rage::Clamp(elbowAngle + timeStep * m_Elbow_w, elbowSwingMin, elbowSwingMax);//assumes elbowSwingMax>elbowSwingMin
      inputData->getElbow()->setDesiredAngle(elbowAngle);
    }
    if (elbowBlendAngle > -0.01f)
    {
      elbowAngle = rage::Clamp(elbowAngle + 2.0f*m_parameters.elbowRate*timeStep * (elbowBlendAngle - elbowAngle), elbowSwingMin, elbowSwingMax);
      inputData->getElbow()->setDesiredAngle(elbowAngle);
    }
    if (bendElbow && forwards)
    {
      float eA = 2.f*s;
      if (s<0.f)//straighten the arms
        //0.2 min elbow bend unless elbowBlendAngle(m_parameters.leftElbowAngle/m_parameters.rightElbowAngle) higher.
        //elbowBlendAngle can be negative so we need this max
        eA = rage::Max(elbowBlendAngle, 0.2f);
      inputData->getElbow()->setDesiredAngle(eA);
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUArmsWindmillAdaptive::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.bodyStiffness, true);
    bspyTaskVar(m_parameters.armStiffness, true);
    bspyTaskVar(m_parameters.angSpeed, true);
    bspyTaskVar(m_parameters.amplitude, true);
    bspyTaskVar(m_parameters.phase, true);
    bspyTaskVar(m_parameters.leftElbowAngle, true);
    bspyTaskVar(m_parameters.rightElbowAngle, true);
    bspyTaskVar(m_parameters.lean1mult, true);
    bspyTaskVar(m_parameters.lean1offset, true);
    bspyTaskVar(m_parameters.elbowRate, true);
    bspyTaskVar(m_parameters.swayAmount, true);
    bspyTaskVar(m_parameters.armDirection, true);
    bspyTaskVar_Bitfield32(m_parameters.effectorMask, true);
    bspyTaskVar(m_parameters.disableOnImpact, true);
    bspyTaskVar(m_parameters.setBackAngles, true);
    bspyTaskVar(m_parameters.useAngMom, true);
    bspyTaskVar(m_parameters.bendLeftElbow, true);
    bspyTaskVar(m_parameters.bendRightElbow, true);

    bspyTaskVar(m_armsWindmillAdaptiveAngle, false);

    bspyTaskVar(m_ElbowBend, false);
    bspyTaskVar(m_Elbow_w, false);
    bspyTaskVar(m_bodyDamping, false);
    bspyTaskVar(m_armDamping, false);

    bspyTaskVar(m_leftHandCollided, false);
    bspyTaskVar(m_doLeftArm, false);
    bspyTaskVar(m_rightHandCollided, false);
    bspyTaskVar(m_doRightArm, false);
  }
#endif // ART_ENABLE_BSPY
}
