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
* Head Look. System for looking at a point. The head will look at the desired point
* if it is physically possible to do so. The motion of both the point being looked 
* at and the head are taken into account.
*
* Acts on the neck to make the head look at the position defined by pos[X,Y,Z]. 
* Optionally the velocity of the point can be compensated for by setting vel[X,Y,Z].
* If alwayslook is true the character will try to look at the point even if it is out side the field of view. 
*
*/


#include "NmRsInclude.h"
#include "NmRsCBU_HeadLook.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_AnimPose.h"

namespace ART
{
  NmRsCBUHeadLook::NmRsCBUHeadLook(ART::MemoryManager* services) : CBUTaskBase(services, bvid_headLook)
  {
    initialiseCustomVariables();
  }

  NmRsCBUHeadLook::~NmRsCBUHeadLook()
  {
  }

  void NmRsCBUHeadLook::initialiseCustomVariables()
  {
    m_mask = bvmask_Spine;
  }

  void NmRsCBUHeadLook::onActivate()
  {
    Assert(m_character);

    NM_RS_DBG_LOGF(L"HEAD LOOK ENTRY");

    //Parameters.
    // Field of view in the xz-plane, at x = 0.0
    m_viewAngle = PI/3.0f;    

    // Neck lower.
    m_NLSwing1Min = getSpine()->getLowerNeck()->getMinLean1();
    m_NLSwing1Max = getSpine()->getLowerNeck()->getMaxLean1();
    m_NLSwing2Min = getSpine()->getLowerNeck()->getMinLean2();
    m_NLSwing2Max = getSpine()->getLowerNeck()->getMaxLean2();
    m_NLTwistMin  = getSpine()->getLowerNeck()->getMinTwist();
    m_NLTwistMax  = getSpine()->getLowerNeck()->getMaxTwist();

    // Neck upper.
    m_NUSwing1Min = getSpine()->getUpperNeck()->getMinLean1();
    m_NUSwing1Max = getSpine()->getUpperNeck()->getMaxLean1();
    m_NUSwing2Min = getSpine()->getUpperNeck()->getMinLean2();
    m_NUSwing2Max = getSpine()->getUpperNeck()->getMaxLean2();
    m_NUTwistMin  = getSpine()->getUpperNeck()->getMinTwist();
    m_NUTwistMax  = getSpine()->getUpperNeck()->getMaxTwist();

    // Whole neck.
    m_NTwistMin   = (m_NLTwistMin + m_NUTwistMin);
    m_NTwistMax   = (m_NLTwistMax + m_NUTwistMax);
    m_NStrength   = 0.5f*(getSpine()->getLowerNeck()->getMuscleStrength() + getSpine()->getUpperNeck()->getMuscleStrength());
    m_NDamping    = 0.5f*0.5f*(getSpine()->getLowerNeck()->getMuscleDamping() + getSpine()->getUpperNeck()->getMuscleDamping());

    m_canLook = false;

    // Offset of 'fixed' look coordinate frame origin.
    rage::Vector3 chestToHead = getSpine()->getHeadPart()->getPosition() - getSpine()->getSpine3Part()->getPosition();
    m_headOffset = chestToHead.Mag();

    // List of neck effectors.

    // Set the body stiffness of the parts in the neck list to something higher
    // otherwise, the neck doesn't have the strength to move.
    m_body->setStiffness(m_parameters.m_stiffness, m_parameters.m_damping, bvmask_HighSpine, NULL);
  }

  void NmRsCBUHeadLook::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUHeadLook::onTick(float /*timeStep*/)
  {

    NmRsCBUAnimPose* animPoseTask = (NmRsCBUAnimPose*)m_cbuParent->m_tasks[bvid_animPose];
    Assert(animPoseTask);
    //Don't do a headlook if animPose is active on the neck and head
    if (! (animPoseTask->isActive() && animPoseTask->m_parameters.overideHeadlook && (animPoseTask->m_parameters.effectorMask & bvmask_CervicalSpine)))
    {
      m_body->setStiffness(m_parameters.m_stiffness, m_parameters.m_damping, bvmask_HighSpine, NULL);
      //getSpine()->setBodyStiffness(getSpineInput(), m_parameters.m_stiffness, m_parameters.m_damping, bvmask_HighSpine);

      m_NStrength   = 0.5f*(getSpine()->getLowerNeck()->getMuscleStrength() + getSpine()->getUpperNeck()->getMuscleStrength());
      m_NDamping    = 0.5f*0.5f*(getSpine()->getLowerNeck()->getMuscleDamping() + getSpine()->getUpperNeck()->getMuscleDamping());

      //Don't look if we lose the inst - alternatively we could cache the last good global poisition?
      if (m_parameters.m_instanceIndex != -1 && (!m_character->IsInstValid_NoGenIDCheck(m_parameters.m_instanceIndex)))
      {
        //look ahead and don't twist spine
        getSpineInputData()->getLowerNeck()->setDesiredAngles(0.0f,0.0f,0.0f);
        getSpineInputData()->getUpperNeck()->setDesiredAngles(0.0f,0.0f,0.0f);
        if(m_parameters.twistSpine)
        {
          getSpineInputData()->getSpine3()->setDesiredTwist(0.0f);
          getSpineInputData()->getSpine2()->setDesiredTwist(0.0f);
        }
        return eCBUTaskComplete;
      }

      rage::Vector3 posTargetLocal = m_parameters.m_pos;
      rage::Vector3 posTarget;
      m_character->instanceToWorldSpace(&posTarget, posTargetLocal, m_parameters.m_instanceIndex);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "headlook", posTargetLocal);
      bspyScratchpad(m_character->getBSpyID(), "headlook", posTarget);
#endif

#if ART_ENABLE_BSPY & HeadLookBSpyDraw 
      //Draw Laser eyes + target
      rage::Matrix34 tmHead;
      getSpine()->getHeadPart()->getBoundMatrix(&tmHead); 
      rage::Vector3 headSide = tmHead.a;
      rage::Vector3 headUp = tmHead.b;
      rage::Vector3 headBack = tmHead.c;
      rage::Vector3 headPosG = getSpine()->getHeadPart()->getPosition();
      headPosG.AddScaled(headUp, 0.03f);

      //Right eye
      rage::Vector3 col(1, 0, 0);//Red
      rage::Vector3 focusPoint = headPosG - headBack;
      focusPoint.AddScaled(headSide, 0.05f);
      rage::Vector3 hPos(headPosG);//Red
      hPos.AddScaled(headSide, 0.05f);
      m_character->bspyDrawLine(hPos, focusPoint, col);

      //Left eye
      col.Set(0.8f, 0, 0);//darker red
      focusPoint = headPosG - headBack;
      focusPoint.AddScaled(headSide, -0.05f);
      hPos.Set(headPosG);//Red
      hPos.AddScaled(headSide, -0.05f);
      m_character->bspyDrawLine(hPos, focusPoint, col);
      m_character->bspyDrawLine(headPosG, posTarget, rage::Vector3(1,1,1));
#endif // ART_ENABLE_BSPY//& HeadLookBSpyDraw

      rage::Vector3 delta;
      rage::Vector3 velTarget;
      rage::Vector3 velFrame;
      rage::Vector3 angVelFrame;
      rage::Vector3 dirTarget;

      rage::Matrix34  headPartMatrix;

      rage::Matrix34 spine3PartMatrix; 
      getSpine()->getSpine3Part()->getBoundMatrix(&spine3PartMatrix); 

      rage::Vector3 spine3Up = spine3PartMatrix.a;
      rage::Vector3 spine3Left = spine3PartMatrix.b;
      rage::Vector3 spine3Back = spine3PartMatrix.c;
      rage::Vector3 posFrame = spine3PartMatrix.d;

      //Remove lean1 offset of head/neck to spine3 (offset is difference of lean1 of spine3 from zero twist,lean1,lean2 of neck/head)
      // i.e. rotate by offset around the spine3 left axis
      static float offset = -0.23819628357887f;//-0.1 may be better for fred
      rage::Vector3 rot = spine3Left*offset;

      rage::Quaternion q;
      q.FromRotation(rot);
      q.Transform(spine3Up);
      q.Transform(spine3Back);

      rage::Vector3  headUpDesired;
      rage::Vector3  headRightDesired;
      rage::Vector3  headBackDesired;

      rage::Vector3 headPos = getSpine()->getSpine3Part()->getPosition();
      // Hoist the frame up a bit along the x-axis to the head level.
      delta.SetScaled(spine3Up, m_headOffset);
      headPos +=  delta;

      if (m_parameters.m_instanceIndex == -1) 
        velTarget = m_parameters.m_vel;
      else
        m_character->getVelocityOnInstance(m_parameters.m_instanceIndex, posTarget, &velTarget);

      // Decide if the target point can be physically looked at. 'Fixed' frame 
      // relative to which one determines if the target point can be looked at.

      // Calculate the actual target point to be looked at.
      velFrame = getSpine()->getSpine3Part()->getLinearVelocity();

      angVelFrame = getSpine()->getSpine3Part()->getAngularVelocity();

      NM_RS_DBG_LOGF(L"velFrame: %.6f", velFrame.Mag());
      NM_RS_DBG_LOGF(L"angVelFrame: %.6f", angVelFrame.Mag());

      posTarget = m_character->targetPosition(posTarget, velTarget, posFrame, velFrame, angVelFrame,  m_NDamping/m_NStrength);

      // Target direction relative to the origin of the 'fixed' coordinate frame. 
      dirTarget.Subtract(posTarget,posFrame);
      dirTarget.Normalize();


      float dotX= dirTarget.Dot(spine3Up);
      float angle = 0.f;
      if (dotX > 0.0f)
        angle = (0.5f*PI - m_viewAngle)*dotX + m_viewAngle;
      else 
        angle = (m_viewAngle - 0.5f*PI)*dotX + m_viewAngle;

      float dotZ = dirTarget.Dot(spine3Back);
      if (dotZ < rage::Cosf(angle))
        m_canLook = true;
      else
        m_canLook = false;

      // Look if physically possible.
      if (m_canLook || m_parameters.m_alwaysLook) 
      {
        //Get desired Head orientation
        headBackDesired = posTarget - headPos;
        headBackDesired *= -1.0f;
        headBackDesired.Normalize();
        //NM_RS_CBU_DRAWARROW( headPos, headPos+headBackDesired, rage::Vector3(0, 0, 0.7f));

        //Make new target vector(headBackDesired)if target outside twist limit
        //if twist over twist limit rotate target dir onto spine3up/spine3twistlimit plane
        //remember excesive twist and feed into spine
        float spineTwist = 0.0f;  
        rage::Vector3 headBackProjected;
        //Work out twist
        headBackProjected.Cross(spine3Up,headBackDesired);
        headBackProjected.Cross(spine3Up);
        headBackProjected.Normalize();
        float twistAngle = rage::AcosfSafe(spine3Back.Dot(headBackProjected));
        float maxTwist = m_NTwistMax*1.2f; //increase the limit a little (decrease this if you get weird head orientations)
        NM_RS_DBG_LOGF(L"twistAngle: %.6f", twistAngle);
        if (twistAngle > maxTwist)//assumes twistmin=-twistmax which it is for the neck
        {
          //Add some twist onto the spine, as there is extra left over from the headtwist
          spineTwist = maxTwist  - twistAngle;
          if (spine3Left.Dot(headBackProjected) < 0.0f)//turn left?
            spineTwist *= -1.0f;

          //rotate headBackDesired onto headTwist limit (i.e. spineTwist radians about spine3Up axis). 
          rage::Quaternion quat;
          quat.FromRotation(spine3Up,-spineTwist);
          rage::Vector3 temp;
          quat.Transform(headBackDesired,temp);
          headBackDesired = temp;
        }
        //NM_RS_CBU_DRAWARROW( headPos, headPos+headBackDesired, rage::Vector3(0, 0, 1.0f));

        getSpine()->getHeadPart()->getBoundMatrix(&headPartMatrix); 

        float dotVertical = spine3Up.Dot(m_character->m_gUp);
        if (m_parameters.m_eyesHorizontal && (dotVertical*dotVertical > 0.25f || m_parameters.m_alwaysEyesHorizontal)) //60deg
        {
          //Horizontal eyes

          //no singularity but 2 stage movement on large change of target
          //project headRightNow onto World horizontal plane
          headRightDesired = headPartMatrix.a;
          m_character->levelVector(headRightDesired);

          float vertical =  rage::Abs(headBackDesired.Dot(m_character->m_gUp));
          if (vertical<0.9f)
          {
            float upright = headPartMatrix.b.Dot(m_character->m_gUp)>0.f ? 1.f:-1.f;
            // singularity but nice movement on large change of target
            headRightDesired.Cross(upright*m_character->m_gUp, headBackDesired);
          }
        }
        else
        {
          headRightDesired.Cross(spine3Up, headBackDesired);
        }
        headRightDesired.Normalize();

#if ART_ENABLE_BSPY & HeadLookBSpyDraw
        m_character->bspyDrawLine(headPos, headPos+headUpDesired, rage::Vector3(0,1,0));
#endif

        headUpDesired.Cross(headBackDesired,headRightDesired);
        headUpDesired.Normalize();

        //Get twist and leans
        //twist - rotate Desired head axes so that up aligns with spine3 up (i.e. remove lean1 and lean2)
        rage::Vector3  projBack;
        rage::Vector3  projUp;
        float theta = rage::AcosfSafe(headUpDesired.Dot(spine3Up));
        rot.Cross(headUpDesired,spine3Up);
        rot.Normalize();
        rot *= theta;

        q.FromRotation(rot);
        q.Transform(headBackDesired,projBack);
        projBack.Normalize();

        float sign = 1.0f;
        float dot = -projBack.Dot(spine3Left);
        if (dot < 0.0f) 
          sign = -1.0f; 
        float twistCalc = rage::AcosfSafe(projBack.Dot(spine3Back))*sign;

        //lean1
        //projection of desired headUp onto spine back,up plane
        projUp.Cross(headUpDesired, spine3Left);
        projUp.Cross(spine3Left);
        projUp *= -1.0f;;
        projUp.Normalize();
        sign = 1.0f;
        dot = -spine3Back.Dot(projUp);
        if (dot < 0.0f) 
          sign = -1.0f; 
        float lean1Calc = rage::AcosfSafe(projUp.Dot(spine3Up))*sign;

        //lean2
        //projection of desired headUp onto spine left,up plane
        projUp.Cross(headUpDesired, spine3Back);
        projUp.Cross(spine3Back);
        projUp *= -1.0f;;
        projUp.Normalize();

        sign = 1.0f;
        dot = spine3Left.Dot(projUp);
        if (dot < 0.0f) 
          sign = -1.0f; 
        float lean2Calc = rage::AcosfSafe(projUp.Dot(spine3Up))*sign;

        if(m_parameters.twistSpine)
        {
          NM_RS_DBG_LOGF(L"spineTwist: %.6f", spineTwist);
          //Mult by 1.35 for compatibility to SpineTwist (m_spineTwistExcess  = 0.35f extra % added to limits).
          float spine3Min = 1.35f*getSpine()->getSpine3()->getMinTwist();
          float spine3Max = 1.35f*getSpine()->getSpine3()->getMaxTwist();
          float spine2Min = 1.35f*getSpine()->getSpine2()->getMinTwist();
          float spine2Max = 1.35f*getSpine()->getSpine2()->getMaxTwist();

          float spine3Twist = 0.65f*spineTwist+getSpine()->getSpine3()->getDesiredTwist();
          spine3Twist = rage::Clamp(spine3Twist,spine3Min,spine3Max);
          float spine2Twist = 0.35f*spineTwist+getSpine()->getSpine2()->getDesiredTwist();
          spine2Twist = rage::Clamp(spine2Twist,spine2Min,spine2Max);

          getSpineInputData()->getSpine3()->setDesiredTwist(spine3Twist);
          getSpineInputData()->getSpine2()->setDesiredTwist(spine2Twist);
        }

        //Apply head Twist
        float NLTwist = 0.f, NUTwist = 0.f;
        m_character->getNeckAngles(&NLTwist, &NUTwist, twistCalc, m_NLTwistMin, m_NLTwistMax, m_NUTwistMin, m_NUTwistMax);
        NLTwist = rage::Clamp(NLTwist, -9.0f, 9.0f);
        NUTwist = rage::Clamp(NUTwist, -9.0f, 9.0f);

        getSpineInputData()->getLowerNeck()->setDesiredTwist(NLTwist);
        getSpineInputData()->getUpperNeck()->setDesiredTwist(NUTwist);

        //Apply head Lean1
        float NLSwing1 = 0.f, NUSwing1 = 0.f;
        m_character->getNeckAngles(&NLSwing1, &NUSwing1, lean1Calc, m_NLSwing1Min ,m_NLSwing1Max, m_NUSwing1Min, m_NUSwing1Max);
        NLSwing1 = rage::Clamp(NLSwing1, -9.0f, 9.0f);
        NUSwing1 = rage::Clamp(NUSwing1, -9.0f, 9.0f);

        getSpineInputData()->getLowerNeck()->setDesiredLean1(NLSwing1);
        getSpineInputData()->getUpperNeck()->setDesiredLean1(NUSwing1);

        //Apply head Lean2
        float NLSwing2 = 0.f, NUSwing2 = 0.f; 
        m_character->getNeckAngles(&NLSwing2, &NUSwing2, lean2Calc, m_NLSwing2Min, m_NLSwing2Max, m_NUSwing2Min, m_NUSwing2Max);
        NLSwing2 = rage::Clamp(NLSwing2, -9.0f, 9.0f);
        NUSwing2 = rage::Clamp(NUSwing2, -9.0f, 9.0f);

        getSpineInputData()->getLowerNeck()->setDesiredLean2(NLSwing2);
        getSpineInputData()->getUpperNeck()->setDesiredLean2(NUSwing2);
			}//if (m_canLook || m_parameters.m_alwaysLook) 

      if (m_parameters.m_keepHeadAwayFromGround)
        getSpine()->keepHeadAwayFromGround(getSpineInput(), 1.f);

    }//if not doing animPose on neck and head

    return eCBUTaskComplete;
  }

#if ART_ENABLE_BSPY
  void NmRsCBUHeadLook::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_pos, true);
    bspyTaskVar(m_parameters.m_vel, true);
    bspyTaskVar(m_parameters.m_instanceIndex, true);

    bspyTaskVar(m_parameters.m_stiffness, true);
    bspyTaskVar(m_parameters.m_damping, true);
    bspyTaskVar(m_parameters.twistSpine, true);
    bspyTaskVar(m_parameters.m_alwaysLook, true);
    bspyTaskVar(m_parameters.m_eyesHorizontal, true);
    bspyTaskVar(m_parameters.m_alwaysEyesHorizontal, true);
    bspyTaskVar(m_parameters.m_keepHeadAwayFromGround, true);

    bool instanceIndexValid = m_parameters.m_instanceIndex == -1 || m_character->IsInstValid_NoGenIDCheck(m_parameters.m_instanceIndex);

    bspyTaskVar(instanceIndexValid,false);
    bspyTaskVar(m_viewAngle, false);
    bspyTaskVar(m_headOffset, false);

    bspyTaskVar(m_canLook, false);
  }
#endif // ART_ENABLE_BSPY
}
