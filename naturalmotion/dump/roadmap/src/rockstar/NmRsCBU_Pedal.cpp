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
#include "NmRsCBU_Pedal.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsEngine.h"
#include "NmRsGenericPart.h"

namespace ART
{

  NmRsCBUPedal::NmRsCBUPedal(ART::MemoryManager* services) : CBUTaskBase(services, bvid_pedalLegs)
  {
    initialiseCustomVariables();
  }

  NmRsCBUPedal::~NmRsCBUPedal()
  {
  }

  void NmRsCBUPedal::initialiseCustomVariables()
  {
    m_mask = bvmask_Legs;

    m_pedalTimer = 0.0f;
    m_leftAngle = 0.0f;
    m_rightAngle = 0.0f;
    m_noiseSeed = 0;
    m_legDamping = 0.5f;
    m_leftRandAsymMult = 0.f;
    m_rightRandAsymMult = 0.f;
    m_startAngleOffset = 0.f;
  }

  void NmRsCBUPedal::onActivate()
  {
    Assert(m_character);
    m_pedalTimer = m_character->getRandom().GetRanged(0.0f, 5000.0f);
    m_noiseSeed = m_character->getRandom().GetRanged(0.0f, 5000.0f);

    m_leftRandAsymMult  = 2.0f * (m_character->getEngine()->perlin3(m_pedalTimer, (float)m_character->getID(), m_noiseSeed)-0.5f);
    m_startAngleOffset = 2.0f*PI*m_character->getRandom().GetFloat();

    // default constant angular velocities
    calculateCAVs(m_parameters.angularSpeed);

    //Randomize the start angles
    m_leftAngle = m_startAngleOffset;
    m_rightAngle = m_leftAngle;
    if (!m_parameters.hula)//start at opposite sides of circle if not hula
      m_rightAngle += PI;
    m_hula = m_parameters.hula;

    //measure legLength
    m_legLength = (getLeftLeg()->getKnee()->getJointPosition() - getLeftLeg()->getHip()->getJointPosition()).Mag();
    m_legLength += (getLeftLeg()->getKnee()->getJointPosition() - getLeftLeg()->getAnkle()->getJointPosition()).Mag();
    m_legLength += 0.07f;//foot height (ankle to bottom of foot)
  }

  void NmRsCBUPedal::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();
  }

  void NmRsCBUPedal::configureLegStiffnesses()
  {
	m_body->setStiffness(m_parameters.legStiffness, m_legDamping, (bvmask_Legs & ~(bvmask_FootLeft | bvmask_FootRight)), NULL, true);
    m_body->setStiffness(1.5f * m_parameters.legStiffness, m_legDamping, (bvmask_FootLeft | bvmask_FootRight), NULL, true);
  }

  void NmRsCBUPedal::calculateCAVs(float angularSpeed)
  {
    m_leftRandAsymMult  = 2.0f * (m_character->getEngine()->perlin3(m_pedalTimer, (float)m_character->getID(), m_noiseSeed)-0.5f);
    m_rightRandAsymMult = 2.0f * (m_character->getEngine()->perlin3(m_noiseSeed, (float)m_character->getID(), m_pedalTimer)-0.5f);

    m_leftConstantAngularVelocity = angularSpeed + (m_leftRandAsymMult * m_parameters.speedAsymmetry);
    if (m_leftConstantAngularVelocity < 0.0f)
      m_leftConstantAngularVelocity = 0.2f;

    m_rightConstantAngularVelocity = angularSpeed + (m_rightRandAsymMult * m_parameters.speedAsymmetry);
    if (m_rightConstantAngularVelocity < 0.0f)
      m_rightConstantAngularVelocity = 0.2f;
  }

  CBUTaskReturn NmRsCBUPedal::onTick(float timeStep)
  {
    //if hula state has changed then offset the left and right target angles appropriately
    //No offset for hula, 180deg offset for normal pedal
    if (m_parameters.hula != m_hula)
    {
      m_hula = m_parameters.hula;
      m_rightAngle = m_leftAngle;
      if (!m_parameters.hula)
        m_rightAngle += PI;
    }
    m_pedalTimer += timeStep;

    configureLegStiffnesses();

    float radiusVariance = m_parameters.radiusVariance * (m_character->getEngine()->perlin3(m_pedalTimer + 10.0f, m_noiseSeed, (float)m_character->getID())-0.5f);
    float moddedRadius = m_parameters.radius + radiusVariance;

    bool pedalLeftLeg = m_parameters.pedalLeftLeg;
    bool pedalRightLeg = m_parameters.pedalRightLeg;
    bool backPedal = m_parameters.backPedal;
    float angularSpeed = m_parameters.angularSpeed;

    //adaptivePedal4Dragging
    if (m_parameters.adaptivePedal4Dragging) //pedal relative to direction of movement
    {
      rage::Vector3 comVel, bodyRight, bodyUp;
      comVel = getSpine()->getSpine0Part()->getLinearVelocity();
      angularSpeed = comVel.Mag();
      comVel.Normalize();

      rage::Matrix34 tmCom;
      getSpine()->getSpine0Part()->getBoundMatrix(&tmCom);
      bodyRight = -tmCom.b;
      bodyUp = tmCom.a; //Note .a for tmCom = Spine2Part TM, .b for tmCom = m_COMTM

      comVel.Cross(m_character->m_gUp);
      float comVelDotBack = -comVel.Dot(bodyRight);

      //Set legs angular speed = COM linear speed / pedal radius
      angularSpeed /= m_parameters.radius;
      angularSpeed *= m_parameters.angSpeedMultiplier4Dragging; 
      angularSpeed = rage::Clamp(angularSpeed, 0.f, 13.f);//mmmm make the max a parameter

      if (comVelDotBack > 0.f)
        backPedal = !m_parameters.backPedal;

      //TDL Don't pedal when lying ontop of car/bonnet
      float upsidedowness = bodyUp.Dot(m_character->m_gUp);
      //so can be dragged with pedal down hill but no pedal in handstand 
      //mmmmTDL use pedal to stretch out legs? move offset + reduce radius
      if (upsidedowness < -0.4f)
      {
        pedalLeftLeg = false;
        pedalRightLeg = false;
      }
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "adaptivePedal", comVelDotBack);
      bspyScratchpad(m_character->getBSpyID(), "adaptivePedal", upsidedowness);
      bspyScratchpad(m_character->getBSpyID(), "adaptivePedal", pedalLeftLeg);
      bspyScratchpad(m_character->getBSpyID(), "adaptivePedal", pedalRightLeg);
      bspyScratchpad(m_character->getBSpyID(), "adaptivePedal", angularSpeed);
      bspyScratchpad(m_character->getBSpyID(), "adaptivePedal", backPedal);
#endif
    }

    calculateCAVs(angularSpeed);

    if (pedalLeftLeg)
    {
      if (backPedal)
        m_leftAngle -= m_leftConstantAngularVelocity * timeStep;
      else
        m_leftAngle += m_leftConstantAngularVelocity * timeStep;
      moveLeg(getLeftLeg(), m_leftAngle, m_parameters.pedalOffset, moddedRadius);
    }

    if (pedalRightLeg)
    {
      if (backPedal)
        m_rightAngle -= m_rightConstantAngularVelocity * timeStep;
      else
        m_rightAngle += m_rightConstantAngularVelocity * timeStep;
      moveLeg(getRightLeg(), m_rightAngle, -m_parameters.pedalOffset, moddedRadius);
    }

// limbs todo support hula in limbs.  this presents a bit of a problem for a number of reasons:
//  1) wants to read back results of ik, which has not been done yet (pushed to limb tick)
//  2) currently no way to blend specific messages (or blend at all, actually).
// this is a good test case for a lot of things... enjoy, future john!
#if 0
    if (m_parameters.hula)
    {
      m_character->setBackAngles(nmrsGetDesiredLean1(getLeftLeg()->getHip()),-nmrsGetDesiredLean2(getLeftLeg()->getHip()),-nmrsGetDesiredLean1(getLeftLeg()->getHip()));
      nmrsSetAngle(getLeftLeg()->getKnee(),-nmrsGetDesiredLean1(getLeftLeg()->getHip()));
      nmrsSetAngle(getRightLeg()->getKnee(),-nmrsGetDesiredLean1(getLeftLeg()->getHip()));
      m_character->getLeftArmSetup()->getShoulder()->setDesiredLean2(nmrsGetDesiredLean2(getLeftLeg()->getHip())+0.5f);
      m_character->getRightArmSetup()->getShoulder()->setDesiredLean2(nmrsGetDesiredLean2(getLeftLeg()->getHip())+0.8f);
      m_character->getLeftArmSetup()->getShoulder()->setDesiredLean1(nmrsGetDesiredLean1(getLeftLeg()->getHip()));
      m_character->getRightArmSetup()->getShoulder()->setDesiredLean1(nmrsGetDesiredLean1(getLeftLeg()->getHip()));
      nmrsSetAngle(m_character->getLeftArmSetup()->getElbow(),rage::Max(0.2f,nmrsGetDesiredLean1(getLeftLeg()->getHip())+0.5f));
      nmrsSetAngle(m_character->getRightArmSetup()->getElbow(),rage::Max(0.2f,nmrsGetDesiredLean1(getLeftLeg()->getHip())+0.2f));
    } 
#endif

    return eCBUTaskComplete;
  }

  void NmRsCBUPedal::moveLeg(NmRsHumanLeg* leg, float angle, float pedalOffset, float moddedRadius)
  {
    //Calculate the point on the circle/ellipse to aim for
    //The plane of the circle used to be down the RAGE hip joint axes (which is a function of joint limit mins/maxs)
    //  As these are offset from say the pelvis body axes it leads to an ellipse that is squashed along a 45deg
    //  The plane was also offset as though following open legs when looking from the front
    //Now they are in the plane of the pelvis
    //  The ellipse is squashed horizontally
    //  When looking from the front the plane is vertical
    rage::Matrix34 pelvisToWorld;
    getSpine()->getPelvisPart()->getBoundMatrix(&pelvisToWorld);
    rage::Vector3 vel = getSpine()->getSpine0Part()->getLinearVelocity();
    rage::Vector3 target;
    float twistSway  = m_parameters.legAngleVariance * (m_character->getEngine()->perlin3((float)m_character->getID(),m_pedalTimer * 0.5f, m_noiseSeed)-0.5f);

    float c,s;
    rage::cos_and_sin(c, s, angle);
    float verticalSquish = 1.0f;
    float horizontalSquish = m_parameters.ellipse;
    if (m_parameters.ellipse < 0.f)
    {
      verticalSquish = -m_parameters.ellipse;
      horizontalSquish = 1.0f;
    }

    target.x = s*verticalSquish;
    target.y = 0.0f;
    target.z = c*horizontalSquish;
    target *= m_parameters.radius;

    // Calculate center for pedal axes in root coordinates (up,left,back)
    //Center is set so as to put the lower part of the circle on the foot bottom with legs straight down.
    //  (1.0f-verticalSquish)*m_parameters.radius moves the vertically squished ellipse down
    //Then centre offsets are applied
    rage::Vector3 centre(moddedRadius - (1.0f-verticalSquish)*m_parameters.radius - m_legLength + pedalOffset + m_parameters.centreUp,
                         -m_parameters.centreSideways, 
                         -m_parameters.centreForwards);
    if (m_parameters.hula)
    {
      target.RotateZ(PI*0.5f);
      //Center is set at the hip joint centre
      //Then centre offsets are applied
      centre.Set(pedalOffset + m_parameters.centreUp,
        0.0f,//apply sideways offset for hula? m_parameters.centreSideways,
        -m_parameters.centreForwards);
    }

    target += centre;
    target.Dot3x3(pelvisToWorld);
    target += leg->getHip()->getJointPosition();

    // ik priority is slightly less to allow hula block to overwrite the knee settings if necessary.
    NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(-1);

    NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();

    ikInputData->setTarget(target);
    ikInputData->setTwist(twistSway);
    ikInputData->setDragReduction(m_parameters.dragReduction);
    ikInputData->setVelocity(vel);
    ikInputData->setTwistIsFixed(true);
    
    leg->postInput(ikInput);

#if ART_ENABLE_BSPY
    rage::Vector3 col(1.0f, 0.0f, 0.0f);
    if (leg == getLeftLeg())
    {
      m_leftTarget = target;
      m_leftCentre = centre;
      col.Set(0.0f,0.9f,0.0f);
    }
    else
    {
      m_rightTarget = target;
      m_rightCentre = centre;
    }
    m_character->bspyDrawPoint(centre, 0.1f, col);
    m_character->bspyDrawPoint(target, 0.1f, col);
    //Draw the desired circle/ellipse
    rage::Vector3 oldTarget;
    float timer = 0.f;
    int sides = 16;
    for(int i=0; i<=sides; i++)
    {
      // Calculate IK target.
      timer += 2.0f*PI/sides;
      rage::cos_and_sin(c, s, timer);
      target.x = s*verticalSquish;
      target.y = 0.0f;
      target.z = c*horizontalSquish;
      target *= m_parameters.radius;
      if (m_parameters.hula)
        target.RotateZ(PI*0.5f);

      target += centre;
      target.Dot3x3(pelvisToWorld);
      target += leg->getHip()->getJointPosition();

      if (i>0)
        m_character->bspyDrawLine(oldTarget, target, col);
      oldTarget = target;
    }

#endif // ART_ENABLE_BSPY
  }


#if ART_ENABLE_BSPY
  void NmRsCBUPedal::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_leftAngle, false);
    bspyTaskVar(m_rightAngle, false);
    bspyTaskVar(m_leftCentre, false);
    bspyTaskVar(m_rightCentre, false);

    bspyTaskVar(m_leftTarget, false);
    bspyTaskVar(m_rightTarget, false);

    bspyTaskVar(m_pedalTimer, false);
    bspyTaskVar(m_noiseSeed, false);
    bspyTaskVar(m_parameters.radius, true);
    bspyTaskVar(m_parameters.radiusVariance, true);
    bspyTaskVar(m_parameters.dragReduction, true);

    bspyTaskVar(m_parameters.randomSeed, true);

    bspyTaskVar(m_parameters.pedalOffset, true);
    bspyTaskVar(m_parameters.legStiffness, true);
    bspyTaskVar(m_parameters.angularSpeed, true);
    bspyTaskVar(m_parameters.speedAsymmetry, true);
    bspyTaskVar(m_parameters.legAngleVariance, true);
    bspyTaskVar(m_parameters.angSpeedMultiplier4Dragging, true);
    bspyTaskVar(m_parameters.centreForwards, true);
    bspyTaskVar(m_parameters.centreSideways, true);
    bspyTaskVar(m_parameters.centreUp, true);
    bspyTaskVar(m_parameters.ellipse, true);
    bspyTaskVar(m_parameters.backPedal, true);
    bspyTaskVar(m_parameters.pedalLeftLeg, true);
    bspyTaskVar(m_parameters.pedalRightLeg, true);
    bspyTaskVar(m_parameters.adaptivePedal4Dragging, true);
    bspyTaskVar(m_parameters.hula, true);
    

    bspyTaskVar(m_legDamping, false);
    bspyTaskVar(m_leftConstantAngularVelocity, false);
    bspyTaskVar(m_rightConstantAngularVelocity, false);
    bspyTaskVar(m_leftRandAsymMult, false);
    bspyTaskVar(m_rightRandAsymMult, false);
    bspyTaskVar(m_startAngleOffset, false);     
    bspyTaskVar(m_legLength, false);     

  }
#endif // ART_ENABLE_BSPY
}
