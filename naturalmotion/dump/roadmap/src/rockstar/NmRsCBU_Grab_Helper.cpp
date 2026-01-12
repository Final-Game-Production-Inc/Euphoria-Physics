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
 * A behaviour only on the arms and clavicles of the character.
 * Will grab/brace to a point, the nearest point on a line or the nearest point on a surface. Grab will create a constrant between the hand and a object. Brace will not create any constraint.
 * This behaviour can be played over other behaviours. There is NO logic about when to grab/brace. Will always try to grab/brace to the desired points, no matter what.
 *
 * There are 3 ways to specify the grab points.
 * Points: right grab/brace point (pos1) and/or left grab/brace point(pos2).
 * Line: grabs/braces with the left and/or right hand to the nearest point on the line between (pos1) and (pos2).
 * Quad Surface: grabs/braces to the surface specified by (pos1), (pos2), (pos3) and (pos4). These points must be specified in a anitclockwise order.
 *
 * The normals ( right/left are normal(normalR)/normal(normalL) respectively) can be specified for all of the grab point input methods. If no normal is specified the behaviour will attempt to find the appropriate normal.
 *
 * The grab points are specified in the coord frame of the instance specified by instanceIndex. ( -1 = world space).
 *
 * PullUp: setting a pull up strength , pullUpStrength, result in the arms trying to pull relative to this strength over a time, pullUpTime. 0 = no attempt to pull up. 1 = attemp to pull up the maximum amount.  Pull up is inhibited when both hands are constrained.
 */




#include "NmRsInclude.h"
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_Grab.h"
#include "NmRsCBU_SpineTwist.h"
#include "NmRsCBU_HeadLook.h"



#include "ART/ARTFeedback.h"

namespace ART
{

    void NmRsCBUGrab::projectPointOntoPlane(float edgeDist,rage::Vector3 &point,rage::Vector3 &normal)
    {
      rage::Vector3 ontoLineZeroOne = point;
      rage::Vector3 ontoLineOneTwo = point;
      rage::Vector3 ontoLineTwoThree = point;
      rage::Vector3 ontoLineThreeZero = point;

      rage::Vector3 plotTemp;
      rage::Vector3 plotTempA;
      rage::Vector3 plotTempB;

      plotTemp = m_parameters.pos1-m_parameters.pos;
      m_character->rotateBoundToWorldSpace(&plotTempB,plotTemp,m_parameters.instanceIndex,m_parameters.boundIndex);
      m_character->boundToWorldSpace(&plotTempA,m_parameters.pos,m_parameters.instanceIndex,m_parameters.boundIndex);
      //NM_RS_CBU_DRAWVECTOR(plotTempA,plotTempB);

      m_character->boundToWorldSpace(&plotTempA,m_parameters.pos1,m_parameters.instanceIndex,m_parameters.boundIndex);
      plotTemp = m_parameters.pos2-m_parameters.pos1;
      m_character->rotateBoundToWorldSpace(&plotTempB,plotTemp,m_parameters.instanceIndex,m_parameters.boundIndex);
      //NM_RS_CBU_DRAWVECTOR(plotTempA,plotTempB);

      m_character->boundToWorldSpace(&plotTempA,m_parameters.pos2,m_parameters.instanceIndex,m_parameters.boundIndex);
      plotTemp = m_parameters.pos3-m_parameters.pos2;
      m_character->rotateBoundToWorldSpace(&plotTempB,plotTemp,m_parameters.instanceIndex,m_parameters.boundIndex);
      //NM_RS_CBU_DRAWVECTOR(plotTempA,plotTempB);

      m_character->boundToWorldSpace(&plotTempA,m_parameters.pos3,m_parameters.instanceIndex,m_parameters.boundIndex);
      plotTemp = m_parameters.pos-m_parameters.pos3;
      m_character->rotateBoundToWorldSpace(&plotTempB,plotTemp,m_parameters.instanceIndex,m_parameters.boundIndex);
      //NM_RS_CBU_DRAWVECTOR(plotTempA,plotTempB);


      // project the point onto the edges
      projectOntoLine(m_parameters.pos,m_parameters.pos1,ontoLineZeroOne);
      m_character->boundToWorldSpace(&plotTemp,ontoLineZeroOne,m_parameters.instanceIndex,m_parameters.boundIndex);
      //NM_RS_CBU_DRAWPOINT(plotTemp,0.2f,rage::Vector3(0,0,1));

      projectOntoLine(m_parameters.pos1,m_parameters.pos2,ontoLineOneTwo);
      m_character->boundToWorldSpace(&plotTemp,ontoLineOneTwo,m_parameters.instanceIndex,m_parameters.boundIndex);
      //NM_RS_CBU_DRAWPOINT(plotTemp,0.2f,rage::Vector3(0,0,1));

      projectOntoLine(m_parameters.pos2,m_parameters.pos3,ontoLineTwoThree);
      m_character->boundToWorldSpace(&plotTemp,ontoLineTwoThree,m_parameters.instanceIndex,m_parameters.boundIndex);
      //NM_RS_CBU_DRAWPOINT(plotTemp,0.2f,rage::Vector3(0,0,1));

      projectOntoLine(m_parameters.pos3,m_parameters.pos,ontoLineThreeZero);
      m_character->boundToWorldSpace(&plotTemp,ontoLineThreeZero,m_parameters.instanceIndex,m_parameters.boundIndex);
      //NM_RS_CBU_DRAWPOINT(plotTemp,0.2f,rage::Vector3(0,0,1));

      // work out the Plane normal
      rage::Vector3 planeNT1;
      rage::Vector3 planeNT2;
      planeNT1 = m_parameters.pos-m_parameters.pos1;
      planeNT1.Normalize();
      planeNT2 = m_parameters.pos2 - m_parameters.pos1;
      planeNT2.Normalize();
      normal.Cross(planeNT1,planeNT2);
      normal.Scale(-1.0f);


      //-- project point onto the plane
      point = point - m_parameters.pos;
      float projOntoNormal = point.Dot(normal);
      rage::Vector3 normTemp = normal;
      normTemp.Scale(projOntoNormal);
      point = point - normTemp+m_parameters.pos;


      // work out the closest edge
      float distZero  = point.Dist(ontoLineZeroOne);
      float distOne   = point.Dist(ontoLineOneTwo);
      float distTwo   = point.Dist(ontoLineTwoThree);
      float distThree = point.Dist(ontoLineThreeZero);
      float minDist = distZero;
      rage::Vector3 nearestPoint = ontoLineZeroOne;
      rage::Vector3 nearestSidePointOne = m_parameters.pos;
      rage::Vector3 nearestSidePointTwo = m_parameters.pos1;

      if (distOne < minDist)
      {
        nearestPoint = ontoLineOneTwo;
        minDist = distOne;
        nearestSidePointOne = m_parameters.pos1;
        nearestSidePointTwo = m_parameters.pos2;
      }
      if (distTwo < minDist)
      {
        nearestPoint = ontoLineTwoThree;
        minDist = distTwo;
        nearestSidePointOne = m_parameters.pos2;
        nearestSidePointTwo = m_parameters.pos3;
      }
      if (distThree < minDist)
      {
        nearestPoint = ontoLineThreeZero;
        minDist = distThree;
        nearestSidePointOne = m_parameters.pos3;
        nearestSidePointTwo = m_parameters.pos;
      }

      // work out if we inside or outside the polygon
      rage::Vector3 sideToPlanePoint = nearestPoint - point;
      sideToPlanePoint.Normalize();
      rage::Vector3 sideVec = nearestSidePointTwo - nearestSidePointOne;
      sideVec.Normalize();
      rage::Vector3 inOutNormal;
      inOutNormal.Cross(sideToPlanePoint,sideVec);
      float normalsDot = inOutNormal.Dot(normal);
      NM_RS_DBG_LOGF(L"normalsDot = : %.5f",  normalsDot);

      // if we are outside the Poly or close enough to the edge, move to the edge
      if ((m_parameters.justBrace && (normalsDot < 0)) || (!m_parameters.justBrace && ((normalsDot < 0)||(minDist<edgeDist))))
      {
        point = nearestPoint;
      }

      // to ensure the normals are in the right dirction when we are behind the plane.
      if (projOntoNormal < 0)
        normal.Scale(-1.0f);
    }


    void NmRsCBUGrab::moveGrabPointsApart(bool mLeft, bool mRight)
    {
      if (mLeft) 
      {
        rage::Vector3 targetDiff = m_rightTarget-m_leftTarget;
        targetDiff.Normalize();

        rage::Vector3 endAlongMoveDirection = m_parameters.pos-m_parameters.pos1;
        endAlongMoveDirection.Normalize();
        if (endAlongMoveDirection.Dot(targetDiff)>0)
          endAlongMoveDirection = m_parameters.pos1;
        else
          endAlongMoveDirection = m_parameters.pos;

        float endDist = m_rightTarget.Dist(endAlongMoveDirection);
        float moveDist = rage::Min(endDist,0.15f);

        targetDiff.Scale(moveDist);

        // visualisation:
        //m_character->boundToWorldSpace(&endAlongMoveDirection,m_leftTarget,m_parameters.instanceIndex,m_parameters.boundIndex);
        // NM_RS_CBU_DRAWPOINT(endAlongMoveDirection, 2.2f, rage::Vector3(1,0,0));

        m_leftTarget = m_leftTarget - targetDiff;
      }
      else if(mRight)
      {
        rage::Vector3 targetDiff = m_leftTarget-m_rightTarget;
        targetDiff.Normalize();

        rage::Vector3 endAlongMoveDirection = m_parameters.pos1-m_parameters.pos;
        endAlongMoveDirection.Normalize();
        if (endAlongMoveDirection.Dot(targetDiff)>0)
          endAlongMoveDirection = m_parameters.pos;
        else
          endAlongMoveDirection = m_parameters.pos1;

        float endDist = m_leftTarget.Dist(endAlongMoveDirection);
        float moveDist = rage::Min(endDist,0.15f);

        targetDiff.Scale(moveDist);

        // visualisation:
        //m_character->boundToWorldSpace(&endAlongMoveDirection,m_rightTarget,m_parameters.instanceIndex,m_parameters.boundIndex);
        //  NM_RS_CBU_DRAWPOINT(endAlongMoveDirection, 2.2f, rage::Vector3(1,0,0));


        m_rightTarget = m_rightTarget - targetDiff;
      }
    }

    void NmRsCBUGrab::projectOntoLine(rage::Vector3 &endOne, rage::Vector3 &endTwo, rage::Vector3 &point)
    {
      rage::Vector3 dir;
      rage::Vector3 dirPoint;
      float magOfLine;
      float projOntoLine;

      dir = endOne-endTwo;
      magOfLine = dir.Mag();
      dir.Normalize();
      dirPoint = point-endTwo;
      projOntoLine = dir.Dot(dirPoint);
      float projOntoLineT = rage::Clamp(projOntoLine, 0.0f, magOfLine);
      point.AddScaled(endTwo,dir,projOntoLineT);
    }

    void NmRsCBUGrab::localShoulderToTarget(rage::Vector3 &grabPosLocal,rage::Vector3 &grabNormalLocal,rage::Vector3 &shoulderPos){
      rage::Vector3 normalTemp;
      m_character->boundToLocalSpace(false, &normalTemp, shoulderPos, m_parameters.instanceIndex,m_parameters.boundIndex);
      grabNormalLocal = normalTemp-grabPosLocal;
      grabNormalLocal.Normalize();
    }

    void NmRsCBUGrab::updateGrabPos(rage::Vector3 &target, rage::Vector3 &normal, rage::Vector3 &grabPointOne, rage::Vector3 &grabPointTwo,rage::Vector3 &inputNormal,rage::Vector3 &shoulderPos)
    {
      rage::Vector3 grabPosLocal;
      rage::Vector3 grabNormalLocal;

      grabPosLocal =  grabPointOne;
      if (!inputNormal.IsZero())
      {
        grabNormalLocal =  inputNormal;
        grabNormalLocal.Normalize();
      }
      else
      {
        localShoulderToTarget(grabPosLocal, grabNormalLocal,shoulderPos);
      }

      if (m_parameters.useLineGrab)
			{
        rage::Vector3 handPos = shoulderPos;
        rage::Vector3 handPosTemp;
        m_character->boundToLocalSpace(false, &handPosTemp,handPos,m_parameters.instanceIndex,m_parameters.boundIndex);
        projectOntoLine(grabPointOne,grabPointTwo,handPosTemp);
        grabPosLocal = handPosTemp;

        if (inputNormal.IsZero()) 
				{
          rage::Vector3 line;
          localShoulderToTarget(grabPosLocal, grabNormalLocal,shoulderPos);
          line = grabPointOne - grabPointTwo;
          line.Normalize();
          grabNormalLocal.Cross(line);
          grabNormalLocal.Cross(line);
          grabNormalLocal.Scale(-1.0f);
        }
      }

      if (m_parameters.surfaceGrab){
        rage::Vector3 handPos = shoulderPos;
        m_character->boundToLocalSpace(false, &grabPosLocal,handPos,m_parameters.instanceIndex,m_parameters.boundIndex);
        projectPointOntoPlane(0.8f,grabPosLocal,grabNormalLocal);
      }

      target = grabPosLocal;
      normal = grabNormalLocal;
    }

    void NmRsCBUGrab::moveArm(rage::Vector3 &handTarget, rage::Vector3 &handTargetNormal, BehaviourMask effectorMask, rage::phConstraintHandle &constraint, NmRsHumanArm *limb, NmRsLimbInput& input, int direction, bool &isConstrained, float &distance, float &holdTimer, float &localPullUpStrength, bool useHardConstraint /* = true */)
    {
      float armStr = m_parameters.armStiffness;
      float armDamp = 1.0f;
      float armStiff = 1.0f;
      float velComp = 0.75f;
      float pullAmount = 3.0f*localPullUpStrength;
      float twist = 0.71f;
      rage::Vector3 ikPos = handTarget;
      float threshold = 0.25f;
#if ART_ENABLE_BSPY
			bspyScratchpad(m_character->getBSpyID(), "HandTarget", ikPos.z);
#endif

      // the velocity of the target point in relation to the body
      rage::Vector3 relativeVelocityofPoint;
      float mag = m_character->m_COMvelMag;
      relativeVelocityofPoint.Normalize(m_character->m_COMvel);  //??
      rage::Vector3 shoulderP = limb->getShoulder()->getJointPosition();

      // if no constraints and relative velocity is large then project onto plane normal to apparent velocity: for grabbing moving objects
      rage::Vector3 comToIKPos;
      rage::Vector3 comPos(m_character->m_COM);
      float velocityGrabbingImportance =  (rage::Clamp(mag,5.0f,20.0f)-5.0f)/15.0f;
      comToIKPos = ikPos - comPos;

      float ontoDot = -0.5f*velocityGrabbingImportance*relativeVelocityofPoint.Dot(comToIKPos);
      comToIKPos.AddScaled(comToIKPos,relativeVelocityofPoint,-ontoDot);
      ikPos = comPos + comToIKPos;
#if ART_ENABLE_BSPY
			bspyScratchpad(m_character->getBSpyID(), "HandTarget1", ikPos.z);
#endif

      //For when airborne mmmmnote for underwater don't want this as may not be falling
      if (!(m_character->hasCollidedWithWorld(bvmask_Full)))
      {
        rage::Vector3 gUp = m_character->m_gUp;
        gUp.Scale(velocityGrabbingImportance);
        ikPos = ikPos + gUp;
      }
#if ART_ENABLE_BSPY
			bspyScratchpad(m_character->getBSpyID(), "HandTarget2", ikPos.z);
#endif

      //if hand below the grab point (w.r.t. grabNormal) lift the grab point up (the normal) a bit 
			rage::Vector3 handPos = limb->getHand()->getPosition();	
			rage::Vector3 hand2Target = ikPos - handPos;
			rage::Matrix34 handTM;
			limb->getHand()->getMatrix(handTM);
      float hand2TargetDist = hand2Target.Mag();//mmmmtodo velocity this up
			hand2Target.Normalize();
			float liftBadHand = 0.f;
			//Palmup
			if (limb == getLeftArm())
				liftBadHand = 0.2f*(rage::Clamp(handTargetNormal.Dot(handTM.a),0.2f,1.f)-0.2f);//0.2f should be handlength
			else
				liftBadHand = -0.2f*(rage::Clamp(handTargetNormal.Dot(handTM.a),-1.f,-0.2f)+0.2f);//0.2f should be handlength
#if ART_ENABLE_BSPY
			bspyScratchpad(m_character->getBSpyID(), "liftBadHand1", liftBadHand);
#endif
			//HandBack
			liftBadHand += 0.2f*(rage::Clamp(handTargetNormal.Dot(handTM.b),0.2f,1.f)-0.2f);//0.2f should be handlength
#if ART_ENABLE_BSPY
			bspyScratchpad(m_character->getBSpyID(), "liftBadHand2", liftBadHand);
#endif
			//HandSide
			liftBadHand += 0.2f*(rage::Clamp(rage::Abs(handTargetNormal.Dot(handTM.c)),0.2f,1.f)-0.2f);//0.2f should be handlength
#if ART_ENABLE_BSPY
			bspyScratchpad(m_character->getBSpyID(), "liftBadHand3", liftBadHand);
#endif
			liftBadHand = rage::Min(liftBadHand,0.2f);
#if ART_ENABLE_BSPY
			m_character->bspyDrawLine(handTM.d, handTM.d+handTM.a, rage::Vector3(1,0,0));
			m_character->bspyDrawLine(handTM.d, handTM.d+handTM.b, rage::Vector3(0,1,0));
			m_character->bspyDrawLine(handTM.d, handTM.d+handTM.c, rage::Vector3(0,0,1));
#endif
			if (hand2Target.Dot(handTargetNormal) < 0)
			  liftBadHand += rage::Min(hand2TargetDist*0.5f, 0.1f);//below target
			ikPos += liftBadHand*handTargetNormal; 

      //Pushoff
      static float pushoff = 0.0f;
      if (liftBadHand < 0.03f && limb->getHand()->collidedWithNotOwnCharacter())
        ikPos -= pushoff*handTargetNormal; 

      // limit the distance away the ikPos is away from the shoulder.
      float ikPosDist = shoulderP.Dist(ikPos);
      rage::Vector3 vShoulderToIkpos = ikPos - shoulderP;
      float reachDist = rage::Clamp(ikPosDist, 0.f, m_reachDistance);

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "HandTarget3", m_reachDistance);
      //bspyScratchpad(m_character->getBSpyID(), "HandTarget3", rDist);
      //bspyScratchpad(m_character->getBSpyID(), "HandTarget3", reachDistImportance);
      bspyScratchpad(m_character->getBSpyID(), "HandTarget3", ikPosDist);
      bspyScratchpad(m_character->getBSpyID(), "HandTarget3", reachDist);
#endif
      vShoulderToIkpos.Normalize();
      vShoulderToIkpos.Scale(reachDist);
      ikPos = shoulderP + vShoulderToIkpos;
      twist = 0.05f-rage::Clamp(reachDist,0.0f,0.71f);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "HandTarget3", ikPos.z);
#endif

      NmRsArmInputWrapper* inputData = input.getData<NmRsArmInputWrapper>();
      if (!constraint.IsValid())
      {
        limb->setBodyStiffness(input, armStr, armDamp, effectorMask, &armStiff);

#if ART_ENABLE_BSPY && GrabBSpyDraw
        if (limb == getLeftArm())
          m_character->bspyDrawPoint(ikPos, 0.1f, rage::Vector3(1,1,0));
        else
        m_character->bspyDrawPoint(ikPos, 0.1f, rage::Vector3(1,0,0));
#endif

        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
        NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
        ikInputData->setTarget(ikPos);
        ikInputData->setTwist(twist);
        ikInputData->setDragReduction(velComp);
        ikInputData->setMaxReachDistance(m_parameters.maxReachDistance);
        ikInputData->setMatchClavicle(kMatchClavicleBetter);

        rage::Vector3 shoulder2Hand = handPos;
        shoulder2Hand -= shoulderP;
        shoulder2Hand.Normalize();
        float dirDot = shoulder2Hand.Dot(handTargetNormal);

        if (dirDot>0.85f)//mmmmwhy? If hanging below grab point have zero hand twist and lean1?
        {
          inputData->getWrist()->setDesiredTwist(0.0f);
          inputData->getWrist()->setDesiredLean1(0.0f);
        }
        else 
        {
          bool useActualAngles = false;
          float twistLimit = 2.4f*(1.5f-rage::Clamp(ikPosDist,0.f,1.5f)); // if the object is far away dont have much wrist twist.
          rage::Vector3 tempNormal = handTargetNormal;

          ikInputData->setWristTarget(ikPos);
          ikInputData->setWristNormal(tempNormal);
          ikInputData->setWristUseActualAngles(useActualAngles);
          ikInputData->setWristTwistLimit(twistLimit);
        }
        float softStr = m_parameters.stickyHands;
        //if (m_parameters.justBrace) 
        //  softStr = 0;

        limb->postInput(ikInput);
        // limbs todo this was set relative. we don't have the implementation as it's not often used
        // and may not be appropriate. change if we see wrist strangeness.
        inputData->getWrist()->setDesiredLean2(-1.0f);

#if NM_EA
        if (m_parameters.fromEA)
          m_character->cleverHandIK(handTarget,limb->getHand(),(float)direction,!m_parameters.justBrace && useHardConstraint, softStr, &constraint, distance, NULL, m_character->m_patches[m_grabPatchR].instLevelIndex, threshold,m_character->m_patches[m_grabPatchR].boundIndex);
        else
#endif//#if NM_EA
        m_character->cleverHandIK(handTarget,limb->getHand(),(float)direction,!m_parameters.justBrace && useHardConstraint, softStr, &constraint, distance, NULL, m_parameters.instanceIndex, threshold,m_parameters.boundIndex);
      }
      else //the constraint is valid
      {
        float constraintStiffness = 20.0f;
        float constraintStrength = 12.0f;
        float constraintDamping = 0.05f;
        float armStiff = 7.0f;
        float armDamp = 0.5f;
        float armStren = 5.0;
        holdTimer = holdTimer + m_character->getLastKnownUpdateStep();
        float grabSt = m_parameters.stickyHands;
        if (m_character->hasCollidedWithWorld(bvmask_Full))//mmmmNote exclude feet and shins?
          grabSt = grabSt*.025f;
        float pullUpArmStrength = rage::Clamp(armStren + (localPullUpStrength*9.0f/(m_parameters.pullUpTime+0.01f))*holdTimer,armStren,localPullUpStrength*9.0f);
        pullUpArmStrength = rage::Max(pullUpArmStrength,armStren);

        limb->setBodyStiffness(input, pullUpArmStrength, armDamp, effectorMask, &armStiff);

        inputData->getWrist()->setMuscleStiffness(constraintStiffness);
        inputData->getWrist()->setMuscleStrength(constraintStrength*constraintStrength);
        inputData->getWrist()->setMuscleDamping(2.0f*constraintDamping*constraintStrength);

#if ART_ENABLE_BSPY && GrabBSpyDraw
        m_character->bspyDrawPoint(ikPos, 0.1f, rage::Vector3(0,1,0));
#endif        
#if NM_EA
				if (m_parameters.fromEA)
					m_character->cleverHandIK(handTarget,limb,(float)direction,!m_parameters.justBrace && useHardConstraint,grabSt,&constraint,distance,NULL,m_character->m_patches[m_grabPatchR].instLevelIndex, threshold,m_character->m_patches[m_grabPatchR].boundIndex);
				else
#endif//#if NM_EA
					m_character->cleverHandIK(handTarget,limb->getHand(),(float)direction,!m_parameters.justBrace && useHardConstraint,grabSt,&constraint,distance,NULL,m_parameters.instanceIndex,threshold,m_parameters.boundIndex);

			if (constraint.IsValid())
			{
				rage::phConstraintBase* baseConstraint = static_cast<rage::phConstraintBase*>( PHCONSTRAINT->GetTemporaryPointer(constraint) );
				if (baseConstraint)
				{
					if (m_parameters.grabStrength == -1)
						baseConstraint->SetBreakable(false);
					else
						baseConstraint->SetBreakable(true, m_parameters.grabStrength);
				}
			}

      inputData->getWrist()->setDesiredLean1(limb->getWrist()->getActualLean1());
      inputData->getWrist()->setDesiredLean2(limb->getWrist()->getActualLean2());
      inputData->getWrist()->setDesiredTwist(limb->getWrist()->getActualTwist());

      inputData->getElbow()->setDesiredAngle(limb->getElbow()->getActualAngle());

      inputData->getShoulder()->setDesiredLean1(limb->getShoulder()->getActualLean1());
      inputData->getShoulder()->setDesiredLean2(limb->getShoulder()->getActualLean2());
      inputData->getShoulder()->setDesiredTwist(limb->getShoulder()->getActualTwist());

      // Also set the clavicles to be there current positions        
      inputData->getClavicle()->setDesiredLean1(limb->getClavicle()->getActualLean1()); 
      inputData->getClavicle()->setDesiredLean2(limb->getClavicle()->getActualLean2()); 
      inputData->getClavicle()->setDesiredTwist(limb->getClavicle()->getActualTwist()); 

        // release constraint if wrist orientation is out by more than 90 degrees
        float error = softWristDirectionConstraint(limb->getHand(),handTargetNormal,(float)direction);       
        if(error > m_parameters.maxWristAngle)
        {
          m_character->ReleaseConstraintSafetly(constraint);
        }

        if (pullAmount > 0 && !(m_constraintL.IsValid() && m_constraintR.IsValid())) 
        {
          inputData->getElbow()->setDesiredAngle(pullAmount);
          inputData->getShoulder()->setDesiredLean1(rage::Clamp(0.7f*pullAmount,0.0f,2.0f));
          inputData->getClavicle()->setDesiredLean1(0.2f*pullAmount);
        }

        float unitStren = 1.0f;
        bool HeadLookToTargetIsRunning = (m_parameters.useHeadLookToTarget && (m_rightIsConstrained || m_leftIsConstrained));
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "Grab", HeadLookToTargetIsRunning);
#endif
        if(!HeadLookToTargetIsRunning)
        {//will be set by headlookToTarget
          float spineStiffness = 10.0f;
          float clavStiffness = 10.0f;
          if (!(m_character->hasCollidedWithWorld(bvmask_LegRight)) && !(m_character->hasCollidedWithWorld(bvmask_LegLeft))) {
            spineStiffness = 4.0f;
            clavStiffness = 7.0f; }
          getSpine()->setBodyStiffness(getSpineInput(), spineStiffness, 1.0f, bvmask_LowSpine, &unitStren);
          getLeftArmInputData()->getClavicle()->setStiffness(clavStiffness, 1.0f, &unitStren);
          getRightArmInputData()->getClavicle()->setStiffness(clavStiffness, 1.0f, &unitStren);
        }//if(!HeadLookToTargetIsRunning)
      }

      //send feedback if the state of the constraint has changed from the value of isConstrained.
      if (constraint.IsValid() && (!isConstrained))
      {
        isConstrained = true;
        sendSuccessFeedback(direction);
      }
      if ((!constraint.IsValid()) && isConstrained)
      {
        //due to error>m_parameters.maxWristAngle
        // or cleverIK may have broken it due to m_parameters.justBrace being set to true after it was set to false and a contraint was made
        isConstrained = false;
        sendFailureFeedback(direction);
    }

    }

    float NmRsCBUGrab::softWristDirectionConstraint(NmRsGenericPart* endPart,rage::Vector3 &normal,float direction)
    {
      Assert(normal==normal);

      rage::Matrix34 handMat;
      rage::Vector3 handDir;
      endPart->getMatrix(handMat);
      Assert(handMat.a==handMat.a);
      Assert(handMat.b==handMat.b);
      Assert(handMat.c==handMat.c);
      Assert(handMat.d==handMat.d);

      handDir = handMat.GetVector(0);//normal from palm
      handDir.Scale(-direction);//direction = 1 for left, -1 for right mmmmcheck mmmtodo - the direction has been changed so make sure this is not torquing in the wrong direction 
      float angle =  rage::AcosfSafe(handDir.Dot(normal));

      handDir.Cross(normal);
      if (handDir.NormalizeSafeRet())
      {
        Assert(m_character->getLastKnownUpdateStep() != 0.0f);
        float scale = rage::Clamp(60.f * m_character->getLastKnownUpdateStep(), 1.f, 60.f);//clamp to keep torque same above 60fps and reduce torque from 60fps to 1fps
        handDir.Scale(m_parameters.orientationConstraintScale * angle / scale);
        Assert(handDir.x == handDir.x);
        endPart->applyTorque(handDir);
      }
      return angle;
    }

		void NmRsCBUGrab::decideToGrab(bool &moveRight, bool &moveLeft)
		{
#if NM_EA
			m_rightClosestTarget.Zero();
			m_leftClosestTarget.Zero();
			if (m_parameters.fromEA)
			{
				bool moveR = false;
				bool moveL = false;
				m_grabPatchR = -1;
				m_grabPatchL = -1;
				rage::Vector3 shoulderPosL = getLeftArm()->getShoulder()->getJointPosition();
				rage::Vector3 shoulderPosR = getRightArm()->getShoulder()->getJointPosition();
				float minDistL = m_parameters.grabDistance + m_parameters.move2Radius;
				float minDistR = minDistL;
				rage::Vector3 normalGlob;
				for (int i = 0; i<NUM_OF_PATCHES; i++) 
				{
					m_character->Patch_Cull(i);
					if (
#if NM_EA_TEST_FROM_IMPACTS
            m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Capsule || 
#endif//#if NM_EA_TEST_FROM_IMPACTS					
            m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Plane || 
						m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Disc || 
						m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Line || 
						m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Point)
					{
						//find points closer than  m_parameters.grabDistance      
						//add hysteresis
						//This also globalizes the patch info Really just for the from contact capsules
						if (m_character->m_patches[i].instLevelIndex > -1)
						{
							//convert the input to global co-ordinates
							m_character->boundToWorldSpace(&m_character->m_patches[i].knownContactPointFromCollision[0].global, m_character->m_patches[i].knownContactPointFromCollision[0].local, m_character->m_patches[i].instLevelIndex, m_character->m_patches[i].boundIndex);
							m_character->rotateBoundToWorldSpace(&m_character->m_patches[i].knownContactNormalFromCollision[0].global, m_character->m_patches[i].knownContactNormalFromCollision[0].local, m_character->m_patches[i].instLevelIndex, m_character->m_patches[i].boundIndex);
						}
						else
						{
               m_character->m_patches[i].knownContactPointFromCollision[0].global = m_character->m_patches[i].knownContactPointFromCollision[0].local;
               m_character->m_patches[i].knownContactNormalFromCollision[0].global =  m_character->m_patches[i].knownContactNormalFromCollision[0].local;
						}
						float velcompscale = 0.45f;
						//if (m_parameters.justBrace)
						//	velcompscale = 0.94f;
						float grabDistance = m_parameters.grabDistance;
						if (m_doGrab)
							grabDistance += 0.2f;


						//mmmmtodo add vel onto shouldL
						if (m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Plane
							|| m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Disc
							|| m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Point
							|| m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Line)
						{
							rage::Vector3 shoulderPosL_Local = shoulderPosL;
							if (m_character->m_patches[i].instLevelIndex > -1)
							{
								//convert the input to global co-ordinates
								m_character->boundToLocalSpace(false, &shoulderPosL_Local, shoulderPosL, m_character->m_patches[i].instLevelIndex, m_character->m_patches[i].boundIndex);
							}
              m_character->m_patches[i].nearestPoint(shoulderPosL_Local, m_character->m_patches[i].knownContactPointFromCollision[0].local);

							if (m_character->m_patches[i].instLevelIndex > -1)
							{
								//convert the input to global co-ordinates
								m_character->boundToWorldSpace(&m_character->m_patches[i].knownContactPointFromCollision[0].global, m_character->m_patches[i].knownContactPointFromCollision[0].local, m_character->m_patches[i].instLevelIndex, m_character->m_patches[i].boundIndex);
								m_character->rotateBoundToWorldSpace(&m_character->m_patches[i].knownContactNormalFromCollision[0].global, m_character->m_patches[i].faceNormals[0].local, m_character->m_patches[i].instLevelIndex, m_character->m_patches[i].boundIndex);
							}
							else
							{
								m_character->m_patches[i].knownContactPointFromCollision[0].global = m_character->m_patches[i].knownContactPointFromCollision[0].local;
								m_character->m_patches[i].knownContactNormalFromCollision[0].global =  m_character->m_patches[i].faceNormals[0].local;
							}
#if ART_ENABLE_BSPY
							m_character->bspyDrawLine(m_character->m_patches[i].knownContactPointFromCollision[0].global, m_character->m_patches[i].knownContactPointFromCollision[0].global+m_character->m_patches[i].knownContactNormalFromCollision[0].global, rage::Vector3(0,1,0) );
							bspyScratchpad(m_character->getBSpyID(), "Grab decide L", m_character->m_patches[i].knownContactPointFromCollision[0].global);
							bspyScratchpad(m_character->getBSpyID(), "Grab decide L", m_character->m_patches[i].knownContactNormalFromCollision[0].global);
#endif

						}
						rage::Matrix34 peltm;
						getSpine()->getSpine3Part()->getMatrix(peltm);
						peltm.Inverse();
						rage::Vector3 Tpel;
						Tpel.Dot(m_character->m_patches[i].knownContactPointFromCollision[0].global,peltm);
						float py = Tpel.y;
						float pz = Tpel.z;
						float frontangle =  atan2f(py,-pz);
#if ART_ENABLE_BSPY
						bspyScratchpad(m_character->getBSpyID(), "Grab decide", frontangle);
#endif

						//Left
						if ((frontangle < m_parameters.reachAngle) && (frontangle > (-m_parameters.reachAngle + m_parameters.oneSideReachAngle)))
      {

							rage::Vector3 tdirection = m_character->m_patches[i].knownContactPointFromCollision[0].global - shoulderPosL;
							float cdist = tdirection.Mag();
							tdirection.Normalize();
							rage::Vector3 comvel = m_character->m_COMvel;
							float speedTowards = tdirection.Dot(comvel);
							cdist = cdist - rage::Clamp(velcompscale*speedTowards,0.0f,cdist);

							if (cdist < minDistR)
							{
								minDistL = cdist;
								m_leftClosestTarget = m_character->m_patches[i].knownContactPointFromCollision[0].global;
								if (cdist < grabDistance)
								{
									m_grabPatchL = i;
									moveL = true;
									m_leftTarget = m_character->m_patches[i].knownContactPointFromCollision[0].global;
									m_leftNormal = m_character->m_patches[i].knownContactNormalFromCollision[0].global;
						    }
							}		
						}//Left

						
							//mmmmtodo add vel onto shouldR
						if (m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Plane
							|| m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Disc
							|| m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Point
							|| m_character->m_patches[i].type == NmRsCharacter::Patch::EO_Line)
							{
								rage::Vector3 shoulderPosR_Local = shoulderPosR;
								if (m_character->m_patches[i].instLevelIndex > -1)
								{
									//convert the input to global co-ordinates
									m_character->boundToLocalSpace(false, &shoulderPosR_Local, shoulderPosR, m_character->m_patches[i].instLevelIndex, m_character->m_patches[i].boundIndex);
								}
								m_character->m_patches[i].nearestPoint(shoulderPosR_Local, m_character->m_patches[i].knownContactPointFromCollision[0].local);

								if (m_character->m_patches[i].instLevelIndex > -1)
								{
									//convert the input to global co-ordinates
									m_character->boundToWorldSpace(&m_character->m_patches[i].knownContactPointFromCollision[0].global, m_character->m_patches[i].knownContactPointFromCollision[0].local, m_character->m_patches[i].instLevelIndex, m_character->m_patches[i].boundIndex);
									m_character->rotateBoundToWorldSpace(&m_character->m_patches[i].knownContactNormalFromCollision[0].global, m_character->m_patches[i].faceNormals[0].local, m_character->m_patches[i].instLevelIndex, m_character->m_patches[i].boundIndex);
      }
      else
      {
									m_character->m_patches[i].knownContactPointFromCollision[0].global = m_character->m_patches[i].knownContactPointFromCollision[0].local;
									m_character->m_patches[i].knownContactNormalFromCollision[0].global =  m_character->m_patches[i].faceNormals[0].local;
      }

#if ART_ENABLE_BSPY
								m_character->bspyDrawLine(m_character->m_patches[i].knownContactPointFromCollision[0].global, m_character->m_patches[i].knownContactPointFromCollision[0].global+m_character->m_patches[i].knownContactNormalFromCollision[0].global, rage::Vector3(1,0,0) );
								bspyScratchpad(m_character->getBSpyID(), "Grab decide R", m_character->m_patches[i].knownContactPointFromCollision[0].global);
								bspyScratchpad(m_character->getBSpyID(), "Grab decide R", m_character->m_patches[i].knownContactNormalFromCollision[0].global);
#endif
							}							
						//Right
						if ((frontangle < m_parameters.reachAngle - m_parameters.oneSideReachAngle) && (frontangle > (-m_parameters.reachAngle)))
						{
							rage::Vector3 tdirection = m_character->m_patches[i].knownContactPointFromCollision[0].global - shoulderPosR;
							float cdist = tdirection.Mag();
							tdirection.Normalize();
							rage::Vector3 comvel = m_character->m_COMvel;
							float speedTowards = tdirection.Dot(comvel);
							cdist = cdist - rage::Clamp(velcompscale*speedTowards,0.0f,cdist);

							if (cdist < minDistR)
							{
								minDistR = cdist;
								m_rightClosestTarget = m_character->m_patches[i].knownContactPointFromCollision[0].global;
								if (cdist < grabDistance)
      {
									m_grabPatchR = i;
									moveR = true;
									m_rightTarget = m_character->m_patches[i].knownContactPointFromCollision[0].global;
									m_rightNormal = m_character->m_patches[i].knownContactNormalFromCollision[0].global;
      }
    }
						}//Right

					}

				}
				moveLeft = (moveLeft && moveL);
				moveRight = (moveRight && moveR);

			}
			else
#endif//#if NM_EA
			{
			  decideToGrabFromParameters(moveLeft, moveRight);
			}

		}

    void NmRsCBUGrab::decideToGrabFromParameters(bool &moveRight, bool &moveLeft)
    {
      bool doGrab = false;
      rage::Vector3 bodyTarget;
      rage::Vector3 bodyTargetTemp, bodyTargetTempL;
      rage::Vector3 chestPos = getSpine()->getSpine3Part()->getPosition();
      float tempGrabDist = 
				m_parameters.grabDistance;
      //if (!m_parameters.justBrace)
      //  tempGrabDist = m_parameters.grabDistance*1.75f;

      m_character->boundToLocalSpace(false, &bodyTargetTemp,chestPos,m_parameters.instanceIndex,m_parameters.boundIndex);

      if (m_parameters.useLineGrab)
      {
        projectOntoLine(m_parameters.pos,m_parameters.pos1,bodyTargetTemp);
      }
      else if(m_parameters.surfaceGrab)
      {
        rage::Vector3 bodyNormal;
        projectPointOntoPlane(0.0f,bodyTargetTemp,bodyNormal);
      }
      else
      {
        if (m_parameters.useLeft && m_parameters.useRight)
        {
					if (m_parameters.pointsX4grab)
					{
						if (bodyTargetTemp.Dist2(m_parameters.pos) < bodyTargetTemp.Dist2(m_parameters.pos2))
							bodyTargetTemp = m_parameters.pos;
						else
							bodyTargetTemp = m_parameters.pos2;
						if (bodyTargetTemp.Dist2(m_parameters.pos1) < bodyTargetTemp.Dist2(m_parameters.pos3))
							bodyTargetTempL = m_parameters.pos1;
						else
							bodyTargetTempL = m_parameters.pos3;
					}
					else
					{
						bodyTargetTemp = m_parameters.pos;
						bodyTargetTempL = m_parameters.pos1;
					}
        }
        else if (m_parameters.useLeft)
        {
					if (m_parameters.pointsX4grab)
					{
						if (bodyTargetTemp.Dist2(m_parameters.pos1) > bodyTargetTemp.Dist2(m_parameters.pos3))
							bodyTargetTemp = m_parameters.pos1;
						else
							bodyTargetTemp = m_parameters.pos3;
					}
					else
          bodyTargetTemp = m_parameters.pos1;
        }
        else if (m_parameters.useRight)
        {
					if (m_parameters.pointsX4grab)
					{
						if (bodyTargetTemp.Dist2(m_parameters.pos) > bodyTargetTemp.Dist2(m_parameters.pos2))
							bodyTargetTemp = m_parameters.pos;
						else
							bodyTargetTemp = m_parameters.pos2;
					}
					else
          bodyTargetTemp = m_parameters.pos;
        }
      }

	  if (m_parameters.useLineGrab || m_parameters.surfaceGrab || (!(m_parameters.useLeft && m_parameters.useRight)))//not both hands only
	  {
      m_character->boundToWorldSpace(&bodyTarget,bodyTargetTemp,m_parameters.instanceIndex,m_parameters.boundIndex);

      float velcompscale = 0.45f;
      //if (m_parameters.justBrace)
      //  velcompscale = 0.94f;

      rage::Vector3 tdirection = bodyTarget - chestPos;
      float cdist = tdirection.Mag();
      tdirection.Normalize();
      NM_RS_DBG_LOGF(L"Distance to target = %.4f", cdist);

      rage::Vector3 comvel = m_character->m_COMvel;
      float speedTowards = tdirection.Dot(comvel);
      NM_RS_DBG_LOGF(L"SpeedTowards =  %.4f", speedTowards);

      cdist = cdist - rage::Clamp(velcompscale*speedTowards,0.0f,cdist);
      NM_RS_DBG_LOGF(L"Percieved Distance to target =  %.4f", cdist);
      NM_RS_DBG_LOGF(L" cdist = %.4f  and grabdist = %.4f",cdist,tempGrabDist);

      //add hysteresis
      if (!m_doGrab) 
      {
        doGrab = (cdist < tempGrabDist);
      }
      else
      {
        doGrab = (cdist < (tempGrabDist+0.2f));
      }

      rage::Matrix34 peltm;
      getSpine()->getSpine3Part()->getMatrix(peltm);
      peltm.Inverse();
      rage::Vector3 Tpel;
      Tpel.Dot(bodyTarget,peltm);

      float py = Tpel.y;
      float pz = Tpel.z;
      float frontangle =  atan2f(py,-pz);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "Grab decide", frontangle);
#endif

      if (doGrab && ((frontangle < m_parameters.reachAngle) && (frontangle > (-m_parameters.reachAngle))))
      {
        if (frontangle > (m_parameters.reachAngle - m_parameters.oneSideReachAngle))
          moveRight = false;
        else 
          moveRight = m_parameters.useRight;

        if (frontangle < (-m_parameters.reachAngle + m_parameters.oneSideReachAngle))
          moveLeft = false;
        else
          moveLeft = m_parameters.useLeft;
      }
      else
      {
        moveLeft = false;
        moveRight = false;
      }
	  }
	  else //both hands only
	  {
		  moveRight = false;
		  moveLeft = false;
			bool doGrabR = false;
			bool doGrabL = false;

		  //RIGHT
		  chestPos = getRightArm()->getShoulder()->getJointPosition();
		  m_character->boundToWorldSpace(&bodyTarget,bodyTargetTemp,m_parameters.instanceIndex,m_parameters.boundIndex);

		  float velcompscale = 0.45f;
		  //if (m_parameters.justBrace)
			 // velcompscale = 0.94f;

		  rage::Vector3 tdirection = bodyTarget - chestPos;
		  float cdist = tdirection.Mag();
		  tdirection.Normalize();
		  NM_RS_DBG_LOGF(L"Distance to target = %.4f", cdist);

		  rage::Vector3 comvel = m_character->m_COMvel;
		  float speedTowards = tdirection.Dot(comvel);
		  NM_RS_DBG_LOGF(L"SpeedTowards =  %.4f", speedTowards);

		  cdist = cdist - rage::Clamp(velcompscale*speedTowards,0.0f,cdist);
		  NM_RS_DBG_LOGF(L"Percieved Distance to target =  %.4f", cdist);
		  NM_RS_DBG_LOGF(L" cdist = %.4f  and grabdist = %.4f",cdist,tempGrabDist);

		  //add hysteresis
		  if (!m_doGrab) 
		  {
			  doGrabR = (cdist < tempGrabDist);
		  }
		  else
		  {
			  doGrabR = (cdist < (tempGrabDist+0.2f));
		  }

		  rage::Matrix34 peltm;
		  getSpine()->getSpine3Part()->getMatrix(peltm);
		  peltm.Inverse();
		  rage::Vector3 Tpel;
		  Tpel.Dot(bodyTarget,peltm);

		  float py = Tpel.y;
		  float pz = Tpel.z;
		  float frontangle =  atan2f(py,-pz);
#if ART_ENABLE_BSPY
		  bspyScratchpad(m_character->getBSpyID(), "Grab decide", frontangle);
#endif

		  if (doGrabR && ((frontangle < m_parameters.reachAngle) && (frontangle > (-m_parameters.reachAngle))))
		  {
			  if (frontangle > (m_parameters.reachAngle - m_parameters.oneSideReachAngle))
				  moveRight = false;
			  else 
				  moveRight = m_parameters.useRight;

		  }
		  else
		  {
			  doGrabR = false;
			  moveRight = false;
		  }

		  //LEFT
		  doGrabL = false;
		  chestPos = getLeftArm()->getShoulder()->getJointPosition();
		  m_character->boundToWorldSpace(&bodyTarget,bodyTargetTempL,m_parameters.instanceIndex,m_parameters.boundIndex);

		  velcompscale = 0.45f;
		  //if (m_parameters.justBrace)
			 // velcompscale = 0.94f;

		  tdirection = bodyTarget - chestPos;
		  cdist = tdirection.Mag();
		  tdirection.Normalize();
		  NM_RS_DBG_LOGF(L"Distance to target = %.4f", cdist);

		  comvel = m_character->m_COMvel;
		  speedTowards = tdirection.Dot(comvel);
		  NM_RS_DBG_LOGF(L"SpeedTowards =  %.4f", speedTowards);

		  cdist = cdist - rage::Clamp(velcompscale*speedTowards,0.0f,cdist);
		  NM_RS_DBG_LOGF(L"Percieved Distance to target =  %.4f", cdist);
		  NM_RS_DBG_LOGF(L" cdist = %.4f  and grabdist = %.4f",cdist,tempGrabDist);

		  //add hysteresis
		  if (!m_doGrab) 
		  {
			  doGrabL = (cdist < tempGrabDist);

		  }
		  else
		  {
			  doGrabL = (cdist < (tempGrabDist+0.2f));
		  }

		  getSpine()->getSpine3Part()->getMatrix(peltm);
		  peltm.Inverse();
		  Tpel.Dot(bodyTarget,peltm);

		  py = Tpel.y;
		  pz = Tpel.z;
		  frontangle =  atan2f(py,-pz);
#if ART_ENABLE_BSPY
		  bspyScratchpad(m_character->getBSpyID(), "Grab decide", frontangle);
#endif

		  if (doGrabL && ((frontangle < m_parameters.reachAngle) && (frontangle > (-m_parameters.reachAngle))))
		  {
			  if (frontangle < (-m_parameters.reachAngle + m_parameters.oneSideReachAngle))
				  moveLeft = false;
			  else
				  moveLeft = m_parameters.useLeft;
		  }
		  else
		  {
			  doGrabL = false;
			  moveLeft = false;
		  }

	  }

      // if we have things in our hands, don't grab (unless hand is very close to target and we're allowed to drop gun)//mmmmTODO allow pistol reaching
      if (!m_parameters.dropWeaponIfNecessary && m_character->getCharacterConfiguration().m_leftHandState != CharacterConfiguration::eHS_Free)
      {
        moveLeft = false;
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "Grab:WeaponInHand", moveLeft);
#endif
      }

      if (!m_parameters.dropWeaponIfNecessary && m_character->getCharacterConfiguration().m_rightHandState != CharacterConfiguration::eHS_Free)
      {
        moveRight = false;
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "Grab:WeaponInHand", moveRight);
#endif
      }
    }

    void NmRsCBUGrab::sendSuccessFeedback(int direction)
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 0;
        if (direction==-1)
          strcpy(feedback->m_behaviourName, NMArmGrabLeftFeedbackName);
        else
          strcpy(feedback->m_behaviourName, NMArmGrabRightFeedbackName);

        feedback->onBehaviourSuccess();
      }
    }

    void NmRsCBUGrab::sendFailureFeedback(int direction)
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 0;
        if (direction==-1)
          strcpy(feedback->m_behaviourName, NMArmGrabLeftFeedbackName);
        else
          strcpy(feedback->m_behaviourName, NMArmGrabRightFeedbackName);

        feedback->onBehaviourFailure();
      }
    }

    void NmRsCBUGrab::sendDropWeaponFeedback(int hand)
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 0;

        if (hand == 0)
          strcpy(feedback->m_behaviourName, NMGrabDropLeftWeaponFeedbackName);
        else
          strcpy(feedback->m_behaviourName, NMGrabDropRightWeaponFeedbackName);

        feedback->onBehaviourEvent();
      }
    }

    // todo: deal with all the other parameterisations
    // add more robust version that takes character com into account
		// uses local targets but converts them to world
    bool NmRsCBUGrab::handsCrossed()
    {
      //will inhibit left constraint : useful only if one of the hands is already constrained
      if(m_constraintL.IsValid()||m_constraintR.IsValid())
			{
      //targets in world space
      rage::Vector3 leftTargetWorld;
      rage::Vector3 rightTargetWorld;
#if NM_EA
				if (m_parameters.fromEA)
				{
					if (m_grabPatchL >= 0)
					{
						leftTargetWorld = m_character->m_patches[m_grabPatchL].knownContactPointFromCollision[0].global;
					}
					if (m_grabPatchR >= 0)
					{
						rightTargetWorld = m_character->m_patches[m_grabPatchR].knownContactPointFromCollision[0].global;
					}
				}
				else
#endif//#if NM_EA
				{
      m_character->boundToWorldSpace(&leftTargetWorld,m_leftTarget,m_parameters.instanceIndex,m_parameters.boundIndex);
      m_character->boundToWorldSpace(&rightTargetWorld,m_rightTarget,m_parameters.instanceIndex,m_parameters.boundIndex);
				}     
      //compare orientation of both targets and both hands
      rage::Vector3 edge = rightTargetWorld - leftTargetWorld;
      rage::Vector3 elbows = getRightArm()->getElbow()->getJointPosition() - getLeftArm()->getElbow()->getJointPosition();
      m_character->levelVector(elbows);
      float UpHand = getRightArm()->getHand()->getPosition().Dot(m_character->getUpVector()) - getLeftArm()->getHand()->getPosition().Dot(m_character->getUpVector());
      float UpEdge = edge.Dot(m_character->getUpVector());
      m_character->levelVector(edge);
      bool crossedHorizontal = edge.Dot(elbows)< 0.f && edge.Mag()>0.05f;//test horizontally : use elbows because if hands already crossed, this test would be true
      bool crossedVertical = (UpEdge > 0.f) != (UpHand > 0.f) && rage::Abs(UpEdge)>0.05f;//test vertically

#if ART_ENABLE_BSPY && GrabBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "handsCrossed", crossedVertical);
      bspyScratchpad(m_character->getBSpyID(), "handsCrossed", crossedHorizontal);
#endif 
      //true if both crossed or if crossed vertically and close horizontally (or the inverse)
      return (crossedVertical && crossedHorizontal)||(crossedHorizontal && UpEdge<0.3f)||(crossedVertical && edge.Mag()<0.1f);
      }
			else
			{
				return false;
			}
    }
}