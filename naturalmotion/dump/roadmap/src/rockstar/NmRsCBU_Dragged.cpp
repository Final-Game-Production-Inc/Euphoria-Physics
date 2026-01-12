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
#include "NmRsCBU_Dragged.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_Pedal.h"

namespace ART
{
  NmRsCBUDragged::NmRsCBUDragged(ART::MemoryManager* services) : CBUTaskBase(services, bvid_dragged),
    m_reachArm(0),
    m_ropeArm(0)
  {
    initialiseCustomVariables();
  }

  NmRsCBUDragged::~NmRsCBUDragged()
  {
  }

  void NmRsCBUDragged::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
  }

  void NmRsCBUDragged::onActivate()
  {
    Assert(m_character);

    ////mmmmmEUPHORIA ONLY
    //m_character->m_grabLeft = true;
    //m_character->m_grabRight = true;
    //m_parameters.m_grabLeft = m_character->m_grabLeft;
    //m_parameters.m_grabRight = m_character->m_grabRight;

    m_ropeArm = 0;
    m_reachArm = 0;
    m_bodyPartConstraint.Reset();
    m_upperConstraint.Reset();
    m_lowerConstraint.Reset();

    //Dragged Entry
    NM_RS_DBG_LOGF(L"- Dragged Entry");

    m_leftHandDominant = true;
    m_lowerConstraint.Reset();
    m_reach = false;
    m_pullUpTimer = 1.f;
    m_reachTimer = -0.1f;
    m_timeTillRelax = 1.0f;
    rage::Vector3 partPos;
    int fixPartIndex;

    m_character->instanceToWorldSpace(&m_ropeAttachmentPos, m_parameters.m_ropePos, m_parameters.m_ropeAttachedToInstance);

    //Make m_bodyPartConstraint
    if ((m_parameters.m_ropedBodyPart != -1) 
      && (m_parameters.m_ropedBodyPart == getLeftArm()->getHand()->getPartIndex())   
      && (m_parameters.m_ropedBodyPart == getRightArm()->getHand()->getPartIndex()) )   
    {
      fixPartIndex = m_parameters.m_ropedBodyPart;
      partPos = m_character->getGenericPartByIndex(fixPartIndex)->getPosition();
      if (m_parameters.m_ropeAttachedToInstance == -1)
        m_character->fixPart(fixPartIndex,m_ropeAttachmentPos,partPos,(partPos - m_ropeAttachmentPos).Mag(),m_bodyPartConstraint);
      else
        m_character->fixPartsTogether2(fixPartIndex,0, (partPos - m_ropeAttachmentPos).Mag(), partPos, m_ropeAttachmentPos, m_parameters.m_ropeAttachedToInstance, m_bodyPartConstraint, false);      
      rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_bodyPartConstraint) );
      if (distConstraint)
        m_taughtRopeLength = distConstraint->GetMaxDistance();
    }

    m_grabbedRight = false;
    m_grabbedLeft = false;
    m_grabbedRight = ((m_parameters.m_playerControl && m_parameters.m_grabRight) 
      || ((m_parameters.m_ropedBodyPart == -1) && !(m_parameters.m_playerControl && m_parameters.m_grabLeft))
      || (m_parameters.m_ropedBodyPart == getRightArm()->getHand()->getPartIndex()) );

    m_grabbedLeft = ((m_parameters.m_playerControl && m_parameters.m_grabLeft) 
      || (m_parameters.m_ropedBodyPart == getLeftArm()->getHand()->getPartIndex()) );
    if (m_grabbedLeft && m_grabbedRight)
    {
      if (m_parameters.m_ropedBodyPart == getLeftArm()->getHand()->getPartIndex())
      {
        m_leftHandIsRopeHand = true;
        m_ropeArm = getLeftArm();
        m_reachArm = getRightArm();
      }
      else
      {
        //right is rope hand
        m_leftHandIsRopeHand = false;
        m_ropeArm = getRightArm();
        m_reachArm = getLeftArm();
      }
      partPos = m_ropeArm->getHand()->getPosition();
      fixPartIndex = m_ropeArm->getHand()->getPartIndex();

      if (m_parameters.m_ropeAttachedToInstance == -1)
        m_character->fixPart(fixPartIndex,m_ropeAttachmentPos,partPos,(partPos - m_ropeAttachmentPos).Mag(),m_upperConstraint);
      else
        m_character->fixPartsTogether2(fixPartIndex,0, (partPos - m_ropeAttachmentPos).Mag(), partPos, m_ropeAttachmentPos, m_parameters.m_ropeAttachedToInstance, m_upperConstraint, false);      

      rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_upperConstraint) );
      if (distConstraint)
        m_taughtRopeLength = distConstraint->GetMaxDistance();

      //Constrain other hand to the other hand
      rage::Vector3 ropeArmPosition = m_ropeArm->getHand()->getPosition();
      rage::Vector3 reachArmPos = m_reachArm->getHand()->getPosition();

      m_character->fixPartsTogether2(m_reachArm->getHand()->getPartIndex(),m_ropeArm->getHand()->getPartIndex(), (ropeArmPosition - reachArmPos).Mag(), reachArmPos, ropeArmPosition, m_character->getFirstInstance()->GetLevelIndex(), m_lowerConstraint, false);      
    }
    else
    {
      if (m_grabbedRight)
      {
        m_leftHandIsRopeHand = false;
        m_ropeArm = getRightArm();
        m_reachArm = getLeftArm();
      }
      else
      {
        //this makes the right and the reach hand if no upperconstraint
        m_leftHandIsRopeHand = true;
        m_ropeArm = getLeftArm();
        m_reachArm = getRightArm();
      }
      if (m_grabbedRight || m_grabbedLeft)
      {
        partPos = m_ropeArm->getHand()->getPosition();
        fixPartIndex = m_ropeArm->getHand()->getPartIndex();

        if (m_parameters.m_ropeAttachedToInstance == -1)
          m_character->fixPart(fixPartIndex,m_ropeAttachmentPos,partPos,(partPos - m_ropeAttachmentPos).Mag(),m_upperConstraint);
        else
          m_character->fixPartsTogether2(fixPartIndex,0, (partPos - m_ropeAttachmentPos).Mag(), partPos, m_ropeAttachmentPos, m_parameters.m_ropeAttachedToInstance, m_upperConstraint, false);      

        rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_upperConstraint) );
        if (distConstraint)
          m_taughtRopeLength = distConstraint->GetMaxDistance();
      }

    }
    SendGrabbedFeedback(m_grabbedLeft,m_grabbedRight, m_leftHandIsRopeHand);

  }

  void NmRsCBUDragged::onDeactivate()
  {
    Assert(m_character);

    // turn off any constraints that may be on
    m_character->ReleaseConstraintSafetly(m_bodyPartConstraint);
    m_character->ReleaseConstraintSafetly(m_upperConstraint);
    m_character->ReleaseConstraintSafetly(m_lowerConstraint);

    SendGrabbedFeedback(false,false,false);

    NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
    Assert(pedalTask);
    pedalTask->deactivate();
  }

  CBUTaskReturn NmRsCBUDragged::onTick(float timeStep)
  {
    if (!m_character->getArticulatedBody())
      return eCBUTaskComplete;

    // check to see if the articulated body is asleep, in which case we are 'stable' and can't / needn't balance
    if (rage::phSleep *sleep = m_character->getArticulatedWrapper()->getArticulatedCollider()->GetSleep())
    {
      if (sleep->IsAsleep())
        return eCBUTaskComplete;
    }

    ////mmmmmEUPHORIA ONLY
    //m_parameters.m_grabLeft = m_character->m_grabLeft;
    //m_parameters.m_grabRight = m_character->m_grabRight;

    NM_RS_DBG_LOGF(L"- Dragged During")
      //Dragged
      bool grabbedLeft = m_grabbedLeft;
    bool grabbedRight = m_grabbedRight;

    //m_parameters.m_ropePos.y += timeStep;
    //This is equivalent to SetWorldPosition but works on instances aswell.  Well in Euhoria it works but it causes a bug in game.
    //m_upperConstraint->SetPositionB(m_parameters.m_ropePos);

    //Get world position of ropeAttachment from m_ropePos and m_ropeAttachedToInstance
    m_character->instanceToWorldSpace(&m_ropeAttachmentPos, m_parameters.m_ropePos, m_parameters.m_ropeAttachedToInstance);

    //replaced with parameter
    //rage::Vector3 handPos = m_ropeArm->getHand()->getPosition();
    //m_ropeTaut = false;
    //if ((handPos - m_ropeAttachmentPos).Mag() >= m_taughtRopeLength - 0.1f)

    //Player Control - release constraints
    if (m_parameters.m_playerControl)
    {
      if (!(((!m_parameters.m_grabRight) && (!m_parameters.m_grabLeft)) || ((!m_lowerConstraint.IsValid()) 
        && (   ((m_ropeArm == getLeftArm()) && (!m_parameters.m_grabLeft))
        ||((m_ropeArm == getRightArm()) && (!m_parameters.m_grabRight)) ) )))
      {
        //Release Left
        if (!m_parameters.m_grabLeft)
        {
          if ((m_reachArm == getLeftArm()) && (m_lowerConstraint.IsValid()))
          {
            m_grabbedLeft = false;
            m_character->ReleaseConstraintSafetly(m_lowerConstraint);
            m_reachTimer = -0.1f;
            m_pullUpTimer = 1.f;
            m_timeTillRelax = 1.0f;
          }
          if ((m_ropeArm == getLeftArm()) && (m_upperConstraint.IsValid()))
          {
            m_grabbedLeft = false;
            m_character->ReleaseConstraintSafetly(m_upperConstraint);
            m_character->ReleaseConstraintSafetly(m_lowerConstraint);

            //Make the lower constraint the upper
            rage::Vector3 handPos = m_reachArm->getHand()->getPosition();            
            if (m_parameters.m_ropeAttachedToInstance == -1)
              m_character->fixPart(m_reachArm->getHand()->getPartIndex(),m_ropeAttachmentPos,handPos,(handPos - m_ropeAttachmentPos).Mag(),m_upperConstraint);
            else
              m_character->fixPartsTogether2(m_reachArm->getHand()->getPartIndex(),0, (handPos - m_ropeAttachmentPos).Mag(), handPos, m_ropeAttachmentPos, m_parameters.m_ropeAttachedToInstance, m_upperConstraint, false);      

            rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_upperConstraint) );
            if (distConstraint)
              m_taughtRopeLength = distConstraint->GetMaxDistance();

            //Swap Constrained hand
            m_leftHandIsRopeHand = false;
            m_ropeArm = getRightArm();
            m_reachArm = getLeftArm();
            m_grabAlongDistance = -0.1f;
            m_reachTimer = -0.1f;
            m_pullUpTimer = 1.f;
            m_timeTillRelax = 1.0f;
          }

        }//release left

        //Release Right
        if (!m_parameters.m_grabRight)
        {
          if ((m_reachArm == getRightArm()) && (m_lowerConstraint.IsValid()))
          {
            m_grabbedRight = false;
            m_character->ReleaseConstraintSafetly(m_lowerConstraint);
            m_reachTimer = -0.1f;
            m_pullUpTimer = 1.f;
            m_timeTillRelax = 1.0f;
          }
          if ((m_ropeArm == getRightArm()) && (m_upperConstraint.IsValid()))
          {
            m_grabbedRight = false;
            m_character->ReleaseConstraintSafetly(m_upperConstraint);
            m_character->ReleaseConstraintSafetly(m_lowerConstraint);

            //Make the lower constraint the upper
            rage::Vector3 handPos = m_reachArm->getHand()->getPosition();            
            if (m_parameters.m_ropeAttachedToInstance == -1)
              m_character->fixPart(m_reachArm->getHand()->getPartIndex(),m_ropeAttachmentPos,handPos,(handPos - m_ropeAttachmentPos).Mag(),m_upperConstraint);
            else
              m_character->fixPartsTogether2(m_reachArm->getHand()->getPartIndex(),0, (handPos - m_ropeAttachmentPos).Mag(), handPos, m_ropeAttachmentPos, m_parameters.m_ropeAttachedToInstance, m_upperConstraint, false);      

            rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_upperConstraint) );
            if (distConstraint)
              m_taughtRopeLength = distConstraint->GetMaxDistance();

            //Swap Constrained hand
            m_leftHandIsRopeHand = true;
            m_ropeArm = getLeftArm();
            m_reachArm = getRightArm();
            m_grabAlongDistance = -0.1f;
            m_reachTimer = -0.1f;
            m_pullUpTimer = 1.f;
            m_timeTillRelax = 1.0f;
          }

        }//release Right
      }//fail
    }//player control

    if (!m_parameters.m_ropeTaut)
    {
      m_reach = false;
      m_pullUpTimer = 1.f;
      m_reachTimer = -0.1f;
      m_timeTillRelax = 1.0f;
    }
    else
    {
      //1)Grab rope if reaching hand near to target //Save last reachTarget distance to ropeAttachmentPos and grab if <= to this distance within a tolerance
      //2)Todo: Grab rope sometimes if near rope line segment        
      if (m_reach)
        GrabRope();

      //Reach For Target?
      m_reach = false;
      //Under player control only drops arms when relaxed.  Keeps reach arm in last ik position
      if ((m_pullUpTimer <= 0.f) || m_parameters.m_playerControl)
        ReachForTarget(timeStep);

      //Under player control drops arms when release rope.
      //if (m_parameters.m_playerControl)
      //{
      //  bool dontReach = ((m_leftHandIsRopeHand && !m_parameters.m_grabRight) || (!m_leftHandIsRopeHand && !m_parameters.m_grabLeft));
      //  if ((m_upperConstraint != 0) && (m_lowerConstraint != 0) && (m_parameters.m_grabRight || m_parameters.m_grabLeft))
      //    ReachForTarget(timeStep);
      //  else if (!dontReach)
      //    ReachForTarget(timeStep);
      //}
      //else if (m_pullUpTimer <= 0.f)
      //  ReachForTarget(timeStep);

      //1)Grab rope if reaching hand near to target //Save last reachTarget distance to ropeAttachmentPos and grab if <= to this distance within a tolerance
      if (m_reach)
        GrabRope();
    }

    //todo: if rope constrained on other body part but on front do pull up with both hands or push up etc
    //Pull up with hand/s?
    if (m_upperConstraint.IsValid())
      PullUp();

    //Set arm strengths (also set reach arm angles if not reaching)
    SetArmMuscles();

    //Update timers
    if (m_pullUpTimer > 0.f) 
      m_pullUpTimer -= timeStep;

    if (m_pullUpTimer <= 0.f)
    {
      if (m_timeTillRelax > -1.f)
        m_timeTillRelax -= timeStep;
      else
        m_timeTillRelax = 1.f;
    }

    //Do lower body pose if on front
    rage::Matrix34 tmCom;
    getSpine()->getSpine0Part()->getBoundMatrix(&tmCom);
    rage::Vector3 bodyBack = tmCom.c;

    NM_RS_DBG_LOGF(L"dot up %.4f", bodyBack.Dot(m_character->m_gUp));
    //If on front spread legs and arch back
    if (bodyBack.Dot(m_character->m_gUp) > 0.4f)
    {
      NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
      Assert(pedalTask);
      pedalTask->deactivate();
      //Get rope segment coordinates (p1-hand,p2-attach) and work out sideways angle
      rage::Vector3 p2 = m_ropeAttachmentPos;
      rage::Vector3 p1Top2;
      if (m_upperConstraint.IsValid())
        p1Top2 = -m_ropeArm->getHand()->getPosition();
      else
        p1Top2 = -m_character->getGenericPartByIndex(m_parameters.m_ropedBodyPart)->getPosition();

      p1Top2 += m_ropeAttachmentPos;
      rage::Vector3 bodyUp = tmCom.a;
      rage::Vector3 bodyRight = -tmCom.b;
      float angle2rope = rage::Clamp(rage::Atan2f(bodyRight.Dot(p1Top2),bodyUp.Dot(p1Top2)),-0.7f,0.7f)/1.2f;
      NM_RS_DBG_LOGF(L"angle2rope =  %f", rage::Atan2f(bodyRight.Dot(p1Top2),bodyUp.Dot(p1Top2)))
        NM_RS_DBG_LOGF(L"angle2ropeC =  %f", angle2rope)

      getLeftLegInputData()->getAnkle()->setDesiredAngles(-0.57f,0.0f,-0.3f);
      getRightLegInputData()->getAnkle()->setDesiredAngles(-0.6f,0.0f,-0.25f);
      getLeftLegInputData()->getKnee()->setDesiredAngle(-0.2f-angle2rope);
      getRightLegInputData()->getKnee()->setDesiredAngle(-0.2f+angle2rope);
      getLeftLegInputData()->getHip()->setDesiredAngles(0.f,-0.45f+angle2rope,-0.3f); //lean2 -0.8 to -0.1
      getRightLegInputData()->getHip()->setDesiredAngles(0.f,-0.45f-angle2rope,-0.3f);

      float twist = -0.1f;
      if (m_leftHandIsRopeHand)
        twist *=-1.f;
      if (m_reach)
        getSpineInputData()->setBackAngles(-0.8f,0.f,twist);
      else
        getSpineInputData()->setBackAngles(-0.2f,0.f,3.f*twist);
    }
    else
    {
      getLeftLegInputData()->getAnkle()->setDesiredAngles(0.0f,0.0f,0.0f);
      getRightLegInputData()->getAnkle()->setDesiredAngles(0.0f,0.0f,0.0f);
      getLeftLegInputData()->getHip()->setDesiredAngles(0.f,0.f,0.f);
      getRightLegInputData()->getHip()->setDesiredAngles(0.f,0.f,0.f);
      getSpineInputData()->setBackAngles(0.0f,0.f,0.f);

      NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
      Assert(pedalTask);
      pedalTask->updateBehaviourMessage(NULL); // initialise params

      pedalTask->m_parameters.pedalLeftLeg=(true);
      pedalTask->m_parameters.pedalRightLeg=(true);
      pedalTask->m_parameters.backPedal=(false);
      pedalTask->m_parameters.radius=(0.15f);
      pedalTask->m_parameters.angularSpeed=(10.f);
      pedalTask->m_parameters.legStiffness=(6.f);
      pedalTask->m_parameters.pedalOffset=(0.05f);
      pedalTask->m_parameters.randomSeed=(100);
      pedalTask->m_parameters.speedAsymmetry=(0.4f);
      pedalTask->m_parameters.adaptivePedal4Dragging=(true);
      pedalTask->m_parameters.angSpeedMultiplier4Dragging=(1.0f);
      pedalTask->m_parameters.radiusVariance=(0.05f);
      pedalTask->m_parameters.legAngleVariance=(0.05f);
      pedalTask->m_parameters.centreForwards=(0.2f);
      pedalTask->m_parameters.centreUp=(0.0f);
      pedalTask->activate();
    }

    if ((m_grabbedLeft != grabbedLeft) || (m_grabbedRight != grabbedRight))
    {
      SendGrabbedFeedback(m_grabbedLeft,m_grabbedRight,m_leftHandIsRopeHand);
#if NM_RS_ENABLE_LOGGING
      if (m_grabbedLeft)
        NM_RS_DBG_LOGF(L"m_grabbedLeft = true")
      else
      NM_RS_DBG_LOGF(L"m_grabbedLeft = false")
      if (m_grabbedRight)
        NM_RS_DBG_LOGF(L"m_grabbedRight = true")
      else
      NM_RS_DBG_LOGF(L"m_grabbedRight = false")
#endif
    }


#if ART_ENABLE_BSPY
    rage::Vector3 pointColour(1.f, 1.f, 1.f); //white
    if (!m_parameters.m_ropeTaut)
      pointColour.Set(1.f, 0.f, 0.f); //red

    //Draw actual constraints
    rage::Vector3 ptB;
    rage::Vector3 ptA;
    if (m_bodyPartConstraint.IsValid())
    {
      rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_bodyPartConstraint) );
      if (distConstraint)
      {
        ptA = VEC3V_TO_VECTOR3(distConstraint->GetWorldPosA());
        ptB = VEC3V_TO_VECTOR3(distConstraint->GetWorldPosB());
        m_character->bspyDrawLine(ptA,ptB, pointColour);
      }
    }

    if (m_upperConstraint.IsValid())
    {
      rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_upperConstraint) );
      if (distConstraint)
      {
        ptA = VEC3V_TO_VECTOR3(distConstraint->GetWorldPosA());
        ptB = VEC3V_TO_VECTOR3(distConstraint->GetWorldPosB());
        m_character->bspyDrawLine(ptA,ptB, pointColour);
      }
    }

    if (m_lowerConstraint.IsValid())
    {
      rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_lowerConstraint) );
      if (distConstraint)
      {
        ptA = VEC3V_TO_VECTOR3(distConstraint->GetWorldPosA());
        ptB = VEC3V_TO_VECTOR3(distConstraint->GetWorldPosB());
        m_character->bspyDrawLine(ptA,ptB, pointColour);
      }
    }

#endif // ART_ENABLE_BSPY

#if NM_RS_ENABLE_LOGGING
    if (m_reach)
      NM_RS_DBG_LOGF(L"m_reach = true")
    else
    NM_RS_DBG_LOGF(L"m_reach = false")
#endif
    NM_RS_DBG_LOGF(L"m_grabAlongDistance = %f", m_grabAlongDistance)
    NM_RS_DBG_LOGF(L"m_pullUpTimer = %f", m_pullUpTimer)
    NM_RS_DBG_LOGF(L"m_timeTillRelax =     %f", m_timeTillRelax)
    NM_RS_DBG_LOGF(L"m_reachTimer =  %f", m_reachTimer)
    NM_RS_DBG_LOGF(L"rope taut =  %s", m_parameters.m_ropeTaut?"true":"false")

    return eCBUTaskComplete;
  }

  void NmRsCBUDragged::SetArmMuscles()
  {
    //Character setup: Pull Up (will have to do this with pId controller on stiffness as function of error of posture
    float constraintStiffness = 20.0f;
    float armStiff = m_parameters.m_armMuscleStiffness;

    //ToDo: these are the same! If 2 handed grab Change arm strengths
    //Todo: Less stiff elbows
    if ((m_upperConstraint.IsValid()) && (m_lowerConstraint.IsValid()))
    {
      m_body->setStiffness(m_parameters.m_armStiffness, m_parameters.m_armDamping, bvmask_ArmLeft | bvmask_ArmRight, &armStiff);
    }
    else
    {
       m_body->setStiffness(m_parameters.m_armStiffness, m_parameters.m_armDamping, bvmask_ArmLeft | bvmask_ArmRight, &armStiff);
    }

    //Make sure constraint doesn't shake at hands
    getLeftArmInputData()->getWrist()->setMuscleStiffness(constraintStiffness);
    getRightArmInputData()->getWrist()->setMuscleStiffness(constraintStiffness);

    NmRsArmInputWrapper* reachArmInput = 0;
    // this is fugly, but i can' think of a better way at the mo.
    if(m_reachArm->getType() == kLeftArm)
      reachArmInput = getLeftArmInputData();
    else
      reachArmInput = getRightArmInputData();

    armStiff = 1.0f;
    if (m_reach)
    {
      //Character setup: Reach
      if (m_leftHandIsRopeHand)
        getRightArm()->setBodyStiffness(getRightArmInput(), 12.f, 1.f, bvmask_Full,&armStiff);
      else
        getLeftArm()->setBodyStiffness(getLeftArmInput(), 12.f, 1.f, bvmask_Full,&armStiff);

      reachArmInput->getElbow()->setStiffness(12.f, 0.75f*1.f,&armStiff);
      reachArmInput->getWrist()->setStiffness(12.f - 1.0f, 1.75f,&armStiff);
    }
    else if ((!m_upperConstraint.IsValid()) || (!m_lowerConstraint.IsValid()))
    {
      //Character setup: Not Reaching
      if (m_leftHandIsRopeHand)
        getRightArm()->setBodyStiffness(getRightArmInput(), 7.f, 1.f, bvmask_Full,&armStiff);
      else
        getLeftArm()->setBodyStiffness(getLeftArmInput(), 7.f, 1.f, bvmask_Full,&armStiff);

      reachArmInput->getElbow()->setStiffness(7.f, 0.75f,&armStiff);
      reachArmInput->getWrist()->setStiffness(6.f , 1.75f,&armStiff);
      //Todo: Don't reach do something else
      reachArmInput->getElbow()->setDesiredAngle(0.0f);
      reachArmInput->getShoulder()->setDesiredAngles(0.0f, 0.f, 0.0f);
      reachArmInput->getClavicle()->setDesiredAngles(0.0f, 0.0f, 0.0f);
    }
    if (m_timeTillRelax < -0.f)//Relax both Arms
    {
      m_body->setStiffness(8.f, 1.f, bvmask_ArmLeft | bvmask_ArmRight ,&armStiff);
      getLeftArmInputData()->getElbow()->setStiffness(9.f, 1.f);
      getRightArmInputData()->getElbow()->setStiffness(9.f, 1.f);
      armStiff = 5.f;
      getLeftArmInputData()->getWrist()->setStiffness(20.0f, 1.f,&armStiff);//wrist so no shaking when hanging by constraint
      getRightArmInputData()->getWrist()->setStiffness(20.0f, 1.f,&armStiff);//wrist so no shaking when hanging by constraint
    }
  }

  //Sets the pose for a constrained hand
  void NmRsCBUDragged::PullUp()    
  {
    //Use arm IK with spine3Part as target for higher hand, spine2part for lower hand
    rage::Vector3 reachTarget = getSpine()->getSpine3Part()->getPosition();
    //Do lower body pose if on front
    rage::Matrix34 tmCom;
    getSpine()->getSpine0Part()->getBoundMatrix(&tmCom);
    rage::Vector3 bodyBack = tmCom.c;

    if ((!m_parameters.m_ropeTaut) || (bodyBack.Dot(m_character->m_gUp) > 0.2f))
    {
      rage::Matrix34 tmCom;
      getSpine()->getSpine3Part()->getBoundMatrix(&tmCom); 
      rage::Vector3 spineForward = -tmCom.c;
      reachTarget +=  0.23f*spineForward;     
      rage::Vector3 spineUp = tmCom.a;
      reachTarget +=  0.1f*spineUp; 
    }

    NmRsArmInputWrapper* ropeArmInput;
    if(m_ropeArm->getType() == kLeftArm)
      ropeArmInput = getLeftArmInputData();
    else
      ropeArmInput = getRightArmInputData();

    float dragReduction = 1.f;
    float twist = 0.25f;//seems the best for this target
    rage::Vector3 targetVel = m_ropeArm->getClaviclePart()->getLinearVelocity();
    //float direction = m_leftHandIsRopeHand? 1.0f:-1.f;
    NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(0, 1.0f DEBUG_LIMBS_PARAMETER("PullUp"));
    NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
    ikInputData->setTarget(reachTarget);
    ikInputData->setTwist(twist);
    ikInputData->setDragReduction(dragReduction);
    ikInputData->setVelocity(targetVel);
    ikInputData->setMatchClavicle(kMatchClavicleBetter);
    m_ropeArm->postInput(ikInput);

    //TODO: Near target align hands with line of rope
    ropeArmInput->getWrist()->setDesiredAngles(0.0f, 0.0f, -1.5f);

    if (!m_reach)
    {
      reachTarget = getSpine()->getSpine2Part()->getPosition();
      float hand2HandLength = (m_ropeArm->getHand()->getPosition() - m_reachArm->getHand()->getPosition()).Mag();
      if (m_lowerConstraint.IsValid())
      {
        // If large distance between hands spine1part for the lower hand target to avoid getting stuck...
        // Todo: maybe offset to hand side a bit although this seems to happen naturally
        NM_RS_DBG_LOGF(L"hand2HandLength = %f", hand2HandLength)
          if (hand2HandLength > 0.3f)
          {
            hand2HandLength -= 0.3f;
            if (hand2HandLength < 0.f)
              hand2HandLength =0.f;
            if (bodyBack.Dot(m_character->m_gUp) > 0.2f)
              reachTarget = getSpine()->getSpine1Part()->getPosition();
          }
      }

      if ((!m_parameters.m_ropeTaut) || (bodyBack.Dot(m_character->m_gUp) > 0.2f))
      {
        rage::Matrix34 tmCom;
        getSpine()->getSpine2Part()->getBoundMatrix(&tmCom); 
        rage::Vector3 spineForward = -tmCom.c;
        reachTarget +=  0.23f*spineForward; 
        rage::Vector3 spineUp = tmCom.a;
        reachTarget +=  (0.1f - hand2HandLength/3.f)*spineUp; 
        //todo: could reach for rope anywhere below ropeHand
      }

      dragReduction = 1.f;
      targetVel = m_reachArm->getClaviclePart()->getLinearVelocity();
      //direction = m_leftHandIsRopeHand? -1.0f:1.f;
      NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(0, 1.0f DEBUG_LIMBS_PARAMETER("PullUp"));
      NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
      ikInputData->setTarget(reachTarget);
      ikInputData->setTwist(twist);
      ikInputData->setDragReduction(dragReduction);
      ikInputData->setVelocity(targetVel);
      ikInputData->setMatchClavicle(kMatchClavicleBetter);
      m_reachArm->postInput(ikInput);

      if(m_reachArm->getType() == kLeftArm)
        getLeftArmInputData()->getWrist()->setDesiredAngles(0.0f, 0.0f, 0.0f);
      else
        getRightArmInputData()->getWrist()->setDesiredAngles(0.0f, 0.0f, 0.0f);
    }
  }

  //1)Grab rope if reaching hand near to target //Save last reachTarget distance to ropeAttachmentPos and grab if <= to this distance within a tolerance
  //2)ToDo: Grab rope sometimes if near rope line segment        
  void NmRsCBUDragged::GrabRope()    
  {
    //Get rope segment coordinates (p1,p2)
    rage::Vector3 p2 = m_ropeAttachmentPos;

    rage::Vector3 p1;//constrained position + (offset - hand length?)
    if (m_upperConstraint.IsValid())
      p1 = m_ropeArm->getHand()->getPosition();
    else
      p1 = m_character->getGenericPartByIndex(m_parameters.m_ropedBodyPart)->getPosition();

    //if near rope line from ropeAttachmentPos to m_grabAlongDistance along rope, then Grab (a cylinder)
    rage::Vector3 handPos = m_reachArm->getHand()->getPosition();
    rage::Vector3 p2Top1 = p1 - p2;
    rage::Vector3 p2ToHandPos = handPos - p2;
    float s = p2ToHandPos.Dot(p2Top1)/p2Top1.Dot(p2Top1);
    rage::Vector3 perpPointOnLine = p2 + s * p2Top1;

    float perpDistanceFromLine = (perpPointOnLine - handPos).Mag();
    float distanceAlongRope = (p2 - handPos).Mag();
    float hand2handDist = (handPos - p1).Mag();
    float distanceDifference = (p2 - p1).Mag() - distanceAlongRope;//don't have to check for -ve as s done later
    float tolerance = rage::Min(m_parameters.m_lengthTolerance,distanceDifference);

    if ((s > 0.f) && (s < 1.f) && (distanceAlongRope - tolerance <= m_grabAlongDistance) && (perpDistanceFromLine < m_parameters.m_radiusTolerance)) //on rope  
    {
      bool firstHandConstraint = true;
      //Release rope-hand constraint
      if (m_upperConstraint.IsValid())
        firstHandConstraint = false;
      m_character->ReleaseConstraintSafetly(m_upperConstraint);

      //Constrain other hand to the rope
      if (m_parameters.m_ropeAttachedToInstance == -1)
        m_character->fixPart(m_reachArm->getHand()->getPartIndex(),m_ropeAttachmentPos,handPos,(handPos - m_ropeAttachmentPos).Mag(),m_upperConstraint);
      else
        m_character->fixPartsTogether2(m_reachArm->getHand()->getPartIndex(),0, (handPos - m_ropeAttachmentPos).Mag(), handPos, m_ropeAttachmentPos, m_parameters.m_ropeAttachedToInstance, m_upperConstraint, false);      

      rage::phConstraintDistance* distConstraint = static_cast<rage::phConstraintDistance*>( PHCONSTRAINT->GetTemporaryPointer(m_upperConstraint) );
      if (distConstraint)
        m_taughtRopeLength = distConstraint->GetMaxDistance();

      if (!firstHandConstraint) //bodyPart roped, 1st time hand was constrained therefore don't make hand2hand
      {
        //Constrain other hand to the other hand
        p1 = m_ropeArm->getHand()->getPosition();
        m_character->fixPartsTogether2(m_ropeArm->getHand()->getPartIndex(),m_reachArm->getHand()->getPartIndex(), (p1 - handPos).Mag(), p1, handPos, m_character->getFirstInstance()->GetLevelIndex(), m_lowerConstraint, false);      
      }

      //Swap Constrained hand
      m_leftHandIsRopeHand = !m_leftHandIsRopeHand;
      m_grabAlongDistance = -0.1f;
      m_reachTimer = -0.1f;
      if (m_leftHandIsRopeHand)
      {
        m_ropeArm = getLeftArm();
        m_reachArm = getRightArm();
        m_grabbedLeft = true;
      }
      else
      {
        m_ropeArm = getRightArm();
        m_reachArm = getLeftArm();
        m_grabbedRight = true;
      }
      m_pullUpTimer = 1.f;
      m_timeTillRelax = 1.0f;

    }
    else if ((m_timeTillRelax < 0.5f) && (s > 1.f) && (hand2handDist >= 0.1f) && (hand2handDist <= 0.2f) && (perpDistanceFromLine < m_parameters.m_radiusTolerance)) //on rope  
    {
      if ((m_upperConstraint.IsValid()))
      {
        //UnderHand grab
        //Constrain other hand to the other hand
        p1 = m_ropeArm->getHand()->getPosition();
        m_character->fixPartsTogether2(m_reachArm->getHand()->getPartIndex(),m_ropeArm->getHand()->getPartIndex(), (p1 - handPos).Mag(), handPos, p1, m_character->getFirstInstance()->GetLevelIndex(), m_lowerConstraint, false);      
        m_grabAlongDistance = -0.1f;
        m_reachTimer = -0.1f;
        m_pullUpTimer = 1.f;
        m_timeTillRelax = 1.0f;
        if (m_leftHandIsRopeHand)
          m_grabbedRight = true;
        else
          m_grabbedLeft = true;
      }
    }

  }

  //(Decides to free a hand and) reach for a new target 
  void NmRsCBUDragged::ReachForTarget(float timeStep)    
  {
    //Get rope segment coordinates (p1,p2)
    rage::Vector3 p2 = m_ropeAttachmentPos;    
    rage::Vector3 p1;//constrained position + (offset - hand length?)
    if (m_upperConstraint.IsValid())
      p1 = m_ropeArm->getHand()->getPosition();
    else
      p1 = m_character->getGenericPartByIndex(m_parameters.m_ropedBodyPart)->getPosition();

    rage::Vector3 p1p2 = p2 - p1;
    if (p1p2.Mag() > 0.12f)
    {
      p1p2.Normalize();
      p1 += 0.12f*p1p2;
    }
    //Get Reach volume for the reaching arm
    //Hemisphere radius arm length attached to spine3 + definition(spine3_2_clavicle)
    //float m_parameters.m_reach = 0.65f;//calculate in constructor
    rage::Vector3 centre = m_reachArm->getShoulder()->getJointPosition();
    rage::Matrix34 tmCom;
    getSpine()->getSpine3Part()->getBoundMatrix(&tmCom); 
    centre = m_reachArm->getClavicle()->getJointPosition();
#if ART_ENABLE_BSPY
    rage::Vector3 pointColour(0.7f, 0.f, 0.f); //red
    m_character->bspyDrawPoint(centre, 0.5f, pointColour);
#endif // ART_ENABLE_BSPY
    rage::Vector3 spine3Left = tmCom.b;
    if (m_leftHandIsRopeHand)
      centre -= 0.23f*spine3Left;
    else
      centre += 0.23f*spine3Left;

    //Line segment 2 line segment infront of hemisphere bottom
    //Hemisphere bottom = spine3 side/up plane at centre
    rage::Vector3 spine3Forward = -tmCom.c;
    float d1 = spine3Forward.Dot(p1-centre); 
    float d2 = spine3Forward.Dot(p2-centre); 
#if ART_ENABLE_BSPY
    pointColour.Set(1.f, 1.f, 1.f); //NORMAL of plane white
    m_character->bspyDrawLine(centre, centre+m_parameters.m_reach*spine3Forward, pointColour);
    pointColour.Set(0.8f, 0.8f, 0.8f);//The plane grey
    m_character->bspyDrawLine(centre, centre-m_parameters.m_reach*tmCom.a, pointColour);
    m_character->bspyDrawLine(centre, centre+m_parameters.m_reach*tmCom.a, pointColour);
    m_character->bspyDrawLine(centre, centre-m_parameters.m_reach*tmCom.b, pointColour);
    m_character->bspyDrawLine(centre, centre+m_parameters.m_reach*tmCom.b, pointColour);
#endif // ART_ENABLE_BSPY

    m_reach = false;
    bool twoPoints = true;
    if ((d1 > 0.f) || (d2 > 0.f))//At least 1 point in front of plane 
    {
      if ((d1 < 0.f) || (d2 < 0.f))//find intersection
      {
        //NB won't now be parallel to plane but assert anyway...
        //Assert(spine3Forward.Dot(p2-p1) == 0.f);
        float u = spine3Forward.Dot(centre-p1)/spine3Forward.Dot(p2-p1);
        rage::Vector3 intersectionPoint = p1 + u*(p2 - p1);
        if (d1 < 0.f)
          p1 = intersectionPoint;
        else
          p2 = intersectionPoint;
        if ((p2-m_ropeAttachmentPos).Mag() >= (p1-m_ropeAttachmentPos).Mag())//1pt only
        {
          p2 = p1;
          twoPoints = false;
        }
      }
#if ART_ENABLE_BSPY
      pointColour.Set(0.7f, 0.f, 0.f); //red
      m_character->bspyDrawPoint(p2, 0.5f, pointColour);//top of line to grab infront of plane
      pointColour.Set(0.f, 0.7f, 0.f); //green
      m_character->bspyDrawPoint(p1, 0.5f, pointColour);//bottom of line to grab infront of plane
#endif // ART_ENABLE_BSPY
      //Line Segment hemisphere intersection
      //Line Segment sphere intersection
      //1)line segment not inside sphere (p1,p2) = (p1,p2)
      rage::Vector3 top1 = p1-centre;
      rage::Vector3 top2 = p2-centre;
      if ((top1.Mag() > m_parameters.m_reach) || (top2.Mag() > m_parameters.m_reach))//(part of) line segment outside sphere
      {

        if (twoPoints)
        {
          //find intersection(s) with sphere
          p1p2 = p2 - p1;
          rage::Vector3 centre2p1 = p1 - centre;
          float a = p1p2.Dot(p1p2);
          float b = 2.f * p1p2.Dot(centre2p1);
          float c = centre.Dot(centre);
          c += p1.Dot(p1);
          c -= 2.f * centre.Dot(p1);
          c -= m_parameters.m_reach * m_parameters.m_reach;
          float bb4ac = b * b - 4.f * a * c;
          if ((rage::Abs(a) > 1e-10f) && (bb4ac >= 0.f))//there are intersections
          {
            m_reach = true;
            float mu1 = 0.f;
            float mu2 = 0.f;
            if (bb4ac == 0.f) //1 intersection point
            {
              mu1 = -b/(2.f*a);
              p1 = p1 + mu1*p1p2;
              p2 = p1;
              twoPoints = false;
              if ((mu1 > 1.f) || (mu1 < 0.f))
                m_reach = false;
            }
            else//2 intersection points
            {
              mu1 = (-b + rage::Sqrtf(bb4ac)) / (2.f * a);
              mu2 = (-b - rage::Sqrtf(bb4ac)) / (2.f * a);
              //keep new p2 closest to p2
              if (mu1 > mu2)
              {
                float dum = mu1;
                mu1 = mu2;
                mu2 = dum;
              }
              if (mu2 <= 1.f)//else keep original point inside sphere (don't extrapolate to intersection)
                p2 = p1 + mu2*p1p2;
              if (mu1 >= 0.f)//else keep original point inside sphere (don't extrapolate to intersection)
                p1 += mu1*p1p2;
              if ((mu1 > 1.f) && (top1.Mag() > m_parameters.m_reach))
                m_reach = false;
              if ((mu2 < 0.f) && (top2.Mag() > m_parameters.m_reach))
                m_reach = false;

            }          

          }
          //else//no intersection//m_reach = false;
        }
        //else 1pt only and its not in the sphere
      }
      else //line segment inside hemisphere: go with original targets
      {
        m_reach = true;
      }
    }
    if ((p2-m_ropeAttachmentPos).Mag() >= (p1-m_ropeAttachmentPos).Mag())//1pt only
    {
      p2 = p1;
      twoPoints = false;
    }


#if ART_ENABLE_BSPY
    pointColour.Set(1.f, 1.f, 1.f); //white
#endif
    if (m_reach) //Choose reachTarget from line p1-p2.  For now choose nearest original ropeAttachmentPos which should be p2
    {
      if (!m_parameters.m_playerControl)
      {
        //Release lower hand constraint so that it can reach
        m_character->ReleaseConstraintSafetly(m_lowerConstraint);

      }

      //
      m_reachTimer = 0.7f;

      //Save the grab length along rope
      m_grabAlongDistance = (p2 - m_ropeAttachmentPos).Mag();
    } 
    else if ((!m_lowerConstraint.IsValid()) && (m_reachTimer >= 0.f) && (d1 > 0.f))//grab at m_grabAlongDistance from rope start - grab hand infront of shoulders
    {
      //p2 = m_grabAlongDistance along rope
      p1 = m_ropeAttachmentPos;
      p2 = m_ropeArm->getHand()->getPosition();
      p1p2 = p2 - p1;
#if ART_ENABLE_BSPY
      float ropeLength = p1p2.Mag();
#endif
      p1p2.Normalize();
      p2 = p1 + m_grabAlongDistance*p1p2;
      m_reach = true;
#if ART_ENABLE_BSPY
      pointColour.Set(0.f, 0.f, 1.f); //blue
      if (m_grabAlongDistance > ropeLength)
        pointColour.Set(0.f, 1.f, 0.f); //green
#endif
    }
    else if (((!m_lowerConstraint.IsValid()) && (m_timeTillRelax < 0.5f) && (m_timeTillRelax > -0.0f) && (d1 > 0.f))
      || (m_parameters.m_playerControl) && (m_timeTillRelax > -0.0f))//mmmmHEREPLAYERControl option remove second condition for always reaching
    {
      //p2 = m_grabAlongDistance along rope
      p1 = m_ropeAttachmentPos;
      p2 = m_ropeArm->getHand()->getPosition();
      p1p2 = p2 - p1;
      m_grabAlongDistance = p1p2.Mag();
      p1p2.Normalize();
      p2 += 0.1f*p1p2;
      m_grabAlongDistance += 0.1f;
      m_reachTimer = 0.5f;
      m_reach = true;
#if ART_ENABLE_BSPY
      pointColour.Set(0.f, 1.f, 0.f); //green
#endif
    }

    if ((m_reach) && !m_lowerConstraint.IsValid())
    {
      if ((!m_parameters.m_playerControl) || (m_leftHandIsRopeHand && m_parameters.m_grabRight)  || (!m_leftHandIsRopeHand && m_parameters.m_grabLeft))
      {
        rage::Vector3 reachTarget = p2;
        //ArmIK
#if ART_ENABLE_BSPY
        m_character->bspyDrawLine(m_ropeAttachmentPos,reachTarget, pointColour);
        m_character->bspyDrawLine(p1,m_ropeArm->getHand()->getPosition(), pointColour);

        pointColour.Set(1.f, 0.f, 0.f); //red
        m_character->bspyDrawPoint(reachTarget, 0.6f, pointColour);
        m_character->bspyDrawLine(reachTarget,p1, pointColour);

        pointColour.Set(0.f, 1.f, 0.f); //green
        m_character->bspyDrawPoint(p1, 0.6f, pointColour);
#endif // ART_ENABLE_BSPY

        NM_RS_DBG_LOGF(L"pos x: %.4f  y: %.4f z: %.4f", reachTarget.x, reachTarget.y, reachTarget.z)
          //float m_parameters.m_armTwist = 1.5f;//arms in front or to side, 0 = arms a bit wider(1.0f for behind)
          float dragReduction = 1.f;
        rage::Vector3 targetVel = m_reachArm->getClaviclePart()->getLinearVelocity();

        //Caused popping up on front
        //m_character->C_LimbIK(m_reachArm, direction, 1.f, false, &reachTarget, &m_parameters.m_armTwist, &dragReduction, NULL, &targetVel, NULL, NULL, NULL);       
        //m_character->matchClavicleToShoulder(m_reachArm->getClavicle(),m_reachArm->getShoulder());
        float straightness = 0.4f;
        float maxSpeed = 5.f;//from balance 200.f; //from catch fall - ie out of range
        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(0, 1.0f DEBUG_LIMBS_PARAMETER("ReachForTarget"));
        NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
        ikInputData->setTarget(reachTarget);
        ikInputData->setTwist(m_parameters.m_armTwist);
        ikInputData->setDragReduction(dragReduction);
        ikInputData->setVelocity(targetVel);
        ikInputData->setAdvancedStaightness(straightness);
        ikInputData->setAdvancedMaxSpeed(maxSpeed);
        ikInputData->setUseAdvancedIk(true);
        ikInputData->setMatchClavicle(kMatchClavicle);
        m_reachArm->postInput(ikInput);

        if(m_reachArm->getType() == kLeftArm)
          getLeftArmInputData()->getWrist()->setDesiredAngles(0.0f, 0.0f, -1.5f);
        else
          getRightArmInputData()->getWrist()->setDesiredAngles(0.0f, 0.0f, -1.5f);
      }
    }
    else
    {
      m_reach = false;
    }

    if (m_reachTimer >= 0.f)
      m_reachTimer -= timeStep;


  }

  void NmRsCBUDragged::SendGrabbedFeedback(bool grabbedLeft, bool grabbedRight, bool leftHandIsUpperHand) 
  {

    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      feedback->m_agentID = m_character->getID();
      feedback->m_argsCount = 3;

      ART::ARTFeedbackInterface::FeedbackUserdata data;
      data.setBool(grabbedLeft);
      feedback->m_args[0] = data;
      data.setBool(grabbedRight);
      feedback->m_args[1] = data;

      int upperConstraintBodyPartNo;
      if (!m_upperConstraint.IsValid())
        upperConstraintBodyPartNo = m_parameters.m_ropedBodyPart;
      else if (leftHandIsUpperHand)
      {
        upperConstraintBodyPartNo = getLeftArm()->getHand()->getPartIndex();
      }
      else
        upperConstraintBodyPartNo = getRightArm()->getHand()->getPartIndex();

      data.setInt(upperConstraintBodyPartNo);
      feedback->m_args[2] = data;
#if ART_ENABLE_BSPY
      strcpy(feedback->m_behaviourName, NMDraggedFeedbackName);
#endif
      feedback->onBehaviourEvent();
    }
  }



#if ART_ENABLE_BSPY
  void NmRsCBUDragged::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_armStiffness, true);
    bspyTaskVar(m_parameters.m_armDamping, true);
    bspyTaskVar(m_parameters.m_armMuscleStiffness, true);
    bspyTaskVar(m_parameters.m_radiusTolerance, true);
    bspyTaskVar(m_parameters.m_ropePos, true);
    bspyTaskVar(m_parameters.m_ropeAttachedToInstance, true);
    bspyTaskVar(m_parameters.m_ropedBodyPart, true);
    bspyTaskVar(m_parameters.m_ropeTaut, true);

    bspyTaskVar(m_parameters.m_reach, true);
    bspyTaskVar(m_parameters.m_armTwist, true);
    bspyTaskVar(m_parameters.m_lengthTolerance, true);

    bspyTaskVar(m_grabAlongDistance, false);
    bspyTaskVar(m_pullUpTimer, false);
    bspyTaskVar(m_timeTillRelax, false);
    bspyTaskVar(m_reachTimer, false);

    bspyTaskVar(m_ropeAttachmentPos, true);

  }
#endif // ART_ENABLE_BSPY
}

