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

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_ArmsWindmill.h"
#include "NmRsCBU_ArmsWindmillAdaptive.h"
#include "NmRsCBU_BodyBalance.h"

namespace ART
{
    //----------------DEFAULT ARM MOTION------------------------------------------------
    bool NmRsCBUShot::defaultArmMotion_entryCondition()
    {
    return !m_falling;
    }
    void NmRsCBUShot::defaultArmMotion_entry()
    { 
      m_defaultArmMotion.rightBrace = false;
      m_defaultArmMotion.leftBrace = false;
    m_defaultArmMotion.timer1 = m_character->getRandom().GetRanged(-2.f*PI, 2.f*PI);
    m_defaultArmMotion.timer2 = m_character->getRandom().GetRanged(-2.f*PI, 2.f*PI);
    }
    void NmRsCBUShot::defaultArmMotion_tick(float timeStep)
    {
			if (m_character->m_underwater)
			{

				bool useLeft = (!m_reachLeftEnabled || m_defaultArmMotion.releaseLeftWound);
				bool useRight = (!m_reachRightEnabled || m_defaultArmMotion.releaseRightWound);
				NmRsCBUArmsWindmillAdaptive* armsWindmillAdaptiveTask = (NmRsCBUArmsWindmillAdaptive*)m_cbuParent->m_tasks[bvid_armsWindmillAdaptive];
				Assert(armsWindmillAdaptiveTask);
        if (useLeft || useRight)
				{
          armsWindmillAdaptiveTask->activate();
					armsWindmillAdaptiveTask->setUseArms(useLeft, useRight);
					armsWindmillAdaptiveTask->m_parameters.armStiffness = m_upperBodyStiffness*11.f;
				}
				else
				{
					armsWindmillAdaptiveTask->deactivate();
				}

			}
			else
			{
      m_defaultArmMotion.timer1 += timeStep;
      m_defaultArmMotion.timer2 += timeStep;

      rage::Vector3 braceTarget;
      rage::Matrix34 tmCom;
      tmCom = m_character->m_COMTM;
      braceTarget = 0.6f*(getSpine()->getSpine3Part()->getLinearVelocity() - m_character->getFloorVelocity());
      //braceTarget += 0.3f*(getSpine()->getPelvisPart()->getLinearVelocity() - m_character->getFloorVelocity());
      getSpine()->getSpine1Part()->getBoundMatrix(&tmCom); 
      float magForwards = rage::Clamp(m_character->m_COMrotvel.Dot(tmCom.b),0.f,4.f);
      magForwards *=0.25;
      braceTarget -= magForwards * braceTarget.Mag()*tmCom.c; 
      braceTarget += 0.5f*magForwards * braceTarget.Mag()*tmCom.a; 
      if (m_character->m_gUp.Dot(tmCom.c) > 0.5f) //facing down falling forwards
        braceTarget -= braceTarget.Mag()*tmCom.c;//tmCom.b;m_bodyUp

      float reachSpeed = braceTarget.Mag(); 
      braceTarget.Normalize();
      getSpine()->getSpine3Part()->getBoundMatrix(&tmCom); 
      float reachDot = braceTarget.Dot(tmCom.c);//m_bodyBack; 

      rage::Vector3 bodyRight = tmCom.a;
      rage::Vector3 bodyBack = tmCom.c;
      float sideDotVel;
      rage::Vector3 comVel = m_character->m_COMvelRelative;
      comVel.Normalize();
      sideDotVel = bodyRight.Dot(comVel);

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "Default arms", reachDot);
    bspyScratchpad(m_character->getBSpyID(), "Default arms", reachSpeed);
#endif


      //ArmsWindmill?
      m_defaultArmMotion.leftArmsWindmill = false;
      m_defaultArmMotion.rightArmsWindmill = false;
      NmRsCBUArmsWindmill* armsWindmillTask = (NmRsCBUArmsWindmill*)m_cbuParent->m_tasks[bvid_armsWindmill];
      Assert(armsWindmillTask);
      //m_parameters.useArmsWindmill = false;//reachspeed 0.7 only on hill reachdot 0.2-0.4
      float armsWindmillTimeLeft = m_character->getRandom().GetRanged(0.5f,1.f);
      float armsWindmillTimeRight = m_character->getRandom().GetRanged(0.5f,1.f);
      //not everything wrapped in m_parameters.useArmsWindmill as if this is changed by the game to be false we want to turn off armswindmill 
      float reachSpeedVal = 0.6f;
      if (m_character->m_underwater)
        reachSpeedVal = 0.1f;
      if (m_parameters.useArmsWindmill && (reachDot>0.3f) && (reachSpeed>reachSpeedVal))//Going Backwards fast enough)
        {
        m_defaultArmMotion.armsWindmillTimeLeft += timeStep;
        m_defaultArmMotion.armsWindmillTimeRight += timeStep;
        //Check to see if arms windmill has stopped using that arm due to it colliding 
        //  (this doesn't get reset by armswindmill on deactivate so once it has collided that's it we won't armsWindmill again - I'm ok with that) 
        if (!m_reachLeftEnabled && (m_defaultArmMotion.armsWindmillTimeLeft < armsWindmillTimeLeft) && (sideDotVel < 0.7f) && (!armsWindmillTask->m_parameters.m_disableOnImpact || armsWindmillTask->couldUseLeft()))
        {
          m_defaultArmMotion.leftArmsWindmill = true;
        }
        if (!m_reachRightEnabled && (m_defaultArmMotion.armsWindmillTimeRight < armsWindmillTimeRight) && (sideDotVel > -0.7f) && (!armsWindmillTask->m_parameters.m_disableOnImpact || armsWindmillTask->couldUseRight()))
        {
          m_defaultArmMotion.rightArmsWindmill = true;
        }
        }

      if (m_defaultArmMotion.leftArmsWindmill || m_defaultArmMotion.rightArmsWindmill)
      {
        armsWindmillTask->m_parameters.m_useLeft = m_defaultArmMotion.leftArmsWindmill;
        armsWindmillTask->m_parameters.m_useRight = m_defaultArmMotion.rightArmsWindmill;
          if (!armsWindmillTask->isActive())
          {
            armsWindmillTask->activate();
          }
        //set shot armswindmill parameters
          float size = 0.3f*rage::Clamp(m_parameters.AWRadiusMult*(reachSpeed-0.5f), 0.0f,1.5f);//0:0.45 
          float sign = armsWindmillTask->m_parameters.m_leftCircleDesc.speed > 0.f ? 1.f : -1.f;
          armsWindmillTask->setLeftSpeedDelta(sign*rage::Clamp(m_parameters.AWSpeedMult*(reachSpeed-0.5f), 0.0f,1.5f));//0:1.5
          armsWindmillTask->setLeftRadius1Delta(size);
          armsWindmillTask->setLeftRadius2Delta(size);
          sign = armsWindmillTask->m_parameters.m_rightCircleDesc.speed > 0.f ? 1.f : -1.f;
          armsWindmillTask->setRightSpeedDelta(sign*rage::Clamp(m_parameters.AWSpeedMult*(reachSpeed-0.5f), 0.0f,1.7f));//0:1.7
          armsWindmillTask->setRightRadius1Delta(size);
          armsWindmillTask->setRightRadius2Delta(size);
          size = size/(1.5f*0.3f);//0:1
        armsWindmillTask->setShoulderStiffnessDelta(size*m_parameters.AWStiffnessAdd*m_upperBodyStiffness - (1.f-m_upperBodyStiffness)*armsWindmillTask->m_parameters.m_shoulderStiffness);
        armsWindmillTask->setElbowStiffnessDelta(-(1.f-m_upperBodyStiffness)*armsWindmillTask->m_parameters.m_elbowStiffness);
        float bodyRightTilt = m_character->m_COMTM.a.Dot(m_character->m_gUp);
        armsWindmillTask->setPhaseOffsetDelta(30.f*bodyRightTilt);
        }
        else
        {
        if (armsWindmillTask->isActive() && armsWindmillTask->getOK2Deactivate())
          armsWindmillTask->deactivate();
        if (m_defaultArmMotion.armsWindmillTimeLeft>armsWindmillTimeLeft)
          m_defaultArmMotion.armsWindmillTimeLeft += timeStep;
        if (m_defaultArmMotion.armsWindmillTimeLeft>armsWindmillTimeLeft + 0.3f + m_character->getRandom().GetRanged(0.f,0.9f))
          m_defaultArmMotion.armsWindmillTimeLeft = 0.f;
        if (m_defaultArmMotion.armsWindmillTimeRight>armsWindmillTimeRight)
          m_defaultArmMotion.armsWindmillTimeRight += timeStep;
        if (m_defaultArmMotion.armsWindmillTimeRight>armsWindmillTimeRight + 0.3f + m_character->getRandom().GetRanged(0.f,0.9f))
          m_defaultArmMotion.armsWindmillTimeRight = 0.f;
        }


      // Release a reach for wound?
      m_defaultArmMotion.releaseLeftWound = false;
      m_defaultArmMotion.releaseRightWound = false;
      if (((m_parameters.releaseWound == 1 && m_parameters.brace) || m_parameters.releaseWound == 2)
        && (reachDot<0.2f) && (reachSpeed>0.5f))
      {
        rage::Matrix34 pelvis;
        getSpine()->getPelvisPart()->getBoundMatrix(&pelvis);
        rage::Vector3 comVelNorm = m_character->m_COMvelRelative;
        comVelNorm.Normalize();
        if(comVelNorm.Dot(pelvis.b) > 0.3f)//pelvis left
          m_defaultArmMotion.releaseLeftWound = true;
        if(comVelNorm.Dot(pelvis.b) < -0.3f)//pelvis left
          m_defaultArmMotion.releaseRightWound = true;

        //reduce the probability that reachForWound will release for injured limbs
        if (m_injuredRLeg || m_injuredLLeg || m_injuredLArm || m_injuredRArm)
      {
          if (m_defaultArmMotion.releaseLeftWound)
            m_defaultArmMotion.releaseLeftWound = m_character->getRandom().GetFloat() > 0.9f;
          if (m_defaultArmMotion.releaseRightWound)
            m_defaultArmMotion.releaseRightWound = m_character->getRandom().GetFloat() > 0.9f;
        }//injured limb?
      }// Release a reach for wound?           

      if (m_parameters.brace)
        {
        m_defaultArmMotion.leftBrace = !m_defaultArmMotion.leftArmsWindmill && (!m_reachLeftEnabled || m_defaultArmMotion.releaseLeftWound);
        m_defaultArmMotion.rightBrace = !m_defaultArmMotion.rightArmsWindmill && (!m_reachRightEnabled || m_defaultArmMotion.releaseRightWound);
        if (m_defaultArmMotion.leftBrace || m_defaultArmMotion.rightBrace)
        {
#if ART_ENABLE_BSPY 
          m_character->setCurrentSubBehaviour("-DefArmsBr"); 
#endif
          defaultArmMotion_armsBrace();
#if ART_ENABLE_BSPY 
          m_character->setCurrentSubBehaviour("-DefArms"); 
#endif
        }
      }//brace?

      m_defaultArmMotion.leftDefault = !m_defaultArmMotion.leftArmsWindmill && !m_defaultArmMotion.leftBrace && (!m_reachLeftEnabled || m_defaultArmMotion.releaseLeftWound);
      m_defaultArmMotion.rightDefault = !m_defaultArmMotion.rightArmsWindmill && !m_defaultArmMotion.rightBrace && (!m_reachRightEnabled || m_defaultArmMotion.releaseRightWound);
      if (m_defaultArmMotion.leftDefault || m_defaultArmMotion.rightDefault)
      {
      //if set both to false then could use bodyBalance arms
#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour("-DefArmsDef"); 
#endif
        defaultArmMotion_deDefaultMotion();
#if ART_ENABLE_BSPY 
      m_character->setCurrentSubBehaviour("-DefArms"); 
#endif

    }
    }

    }

    bool NmRsCBUShot::defaultArmMotion_exitCondition()
    {
      return m_newHit || m_falling;
    }

    void NmRsCBUShot::defaultArmMotion_exit()
    {
      //Deactivate sub-behaviours used

        NmRsCBUArmsWindmill* armsWindmillTask = (NmRsCBUArmsWindmill*)m_cbuParent->m_tasks[bvid_armsWindmill];
        Assert(armsWindmillTask);
        if (armsWindmillTask->isActive())
          armsWindmillTask->deactivate();   

				NmRsCBUArmsWindmillAdaptive* armsWindmillAdaptiveTask = (NmRsCBUArmsWindmillAdaptive*)m_cbuParent->m_tasks[bvid_armsWindmillAdaptive];
				Assert(armsWindmillAdaptiveTask);
				if (armsWindmillAdaptiveTask->isActive())
					armsWindmillAdaptiveTask->deactivate();   

      }

    void NmRsCBUShot::defaultArmMotion_deDefaultMotion()
    {
      NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
      Assert(bodyBalanceTask);
      bool setAngles = true;
      if (bodyBalanceTask->isActive())
        setAngles = false;//let bodyBalance be the default arm motion if running - however use the shot stiffnesses

      rage::Vector3 lean = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
      rage::Vector3 leanVel = getSpine()->getSpine3Part()->getLinearVelocity() - getSpine()->getPelvisPart()->getLinearVelocity();
      rage::Vector3 fall = (lean*0.5f + leanVel*0.3f)*2.f;
      float fallMag = leanVel.Mag();
      fallMag = rage::Clamp(fallMag - 0.15f, 0.f, 1.f);
      rage::Matrix34 mat;

      // TDL default arm motion here.
      if (m_defaultArmMotion.leftDefault)
      {
        getLeftArm()->setBodyStiffness(getLeftArmInput(), (7.0f + 3.0f*fallMag)*m_upperBodyStiffness, 1.0f, bvmask_ArmLeft & ~bvmask_ForearmLeft);
        getLeftArmInputData()->getElbow()->setStiffness((3.0f + 3.0f*fallMag)*m_upperBodyStiffness, 1.0f);
        getLeftArmInputData()->getWrist()->setStiffness(10.f,1.f);//no floppy wrists

        getLeftArm()->getShoulder()->getMatrix1(mat);
        float fallX = 4.f * fall.Dot(mat.a);
        float fallZ = rage::Clamp(4.f * fall.Dot((mat.c - mat.b)*0.707f), -8.f, 8.f);

        float l1 = -sinf(fallX);
        if (l1<0.f)//mmmmNewArms
          l1 *=-0.7f;
        float l2 = cosf(fallX) + fallZ - 0.5f*fallMag;

        getLeftArmInputData()->getShoulder()->setOpposeGravity(fallMag);
        getLeftArmInputData()->getClavicle()->setOpposeGravity(fallMag);

        if (setAngles)
        {
          // TDL we can replace the static pose with a zero pose if we need to. Make 1-fallMag the blend factor
          getLeftArmInputData()->getShoulder()->setDesiredAngles(
            (l1 + getLeftArm()->getShoulder()->getMidLean1())*fallMag,
            (l2 + getLeftArm()->getShoulder()->getMidLean2())*fallMag + (0.2f)*(1.f-fallMag), 0.f);
          getLeftArmInputData()->getClavicle()->setDesiredAngles(0.f, (l2 + getLeftArm()->getClavicle()->getMidLean2())*fallMag, 0.f);
          getLeftArmInputData()->getWrist()->setDesiredAngles(0.0f, 0.0f, 0.0f);
          getLeftArmInputData()->getElbow()->setDesiredAngle(1.f*fallMag + 0.7f*(1.f-fallMag));
      }
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "LeftDefault", l1);
        bspyScratchpad(m_character->getBSpyID(), "LeftDefault", l2);
        bspyScratchpad(m_character->getBSpyID(), "LeftDefault", fallMag);
#endif // ART_ENABLE_BSPY

      }
      if (m_defaultArmMotion.rightDefault)
      {
        getRightArm()->setBodyStiffness(getRightArmInput(), (7.f + 3.f*fallMag)*m_upperBodyStiffness, 1.f, bvmask_ArmRight & ~bvmask_ForearmRight);
        getRightArmInputData()->getElbow()->setStiffness((3.f + 3.f*fallMag)*m_upperBodyStiffness, 1.f);
        getRightArmInputData()->getWrist()->setStiffness(10.f, 1.f);//no floppy wrists

        getRightArm()->getShoulder()->getMatrix1(mat);
        float fallX = 4.f * fall.Dot(mat.a);
        float fallZ = rage::Clamp(4.f * fall.Dot((mat.c + mat.b)*0.707f), -8.f, 8.f);

        float l1 = -sinf(fallX);
        if (l1<0.f)//mmmmNewArms
          l1 *=0.7f;
        float l2 = cosf(fallX) + fallZ - 0.5f*fallMag;
        getRightArmInputData()->getShoulder()->setOpposeGravity(fallMag);
        getRightArmInputData()->getClavicle()->setOpposeGravity(fallMag);

        if (setAngles)
        {
          getRightArmInputData()->getShoulder()->setDesiredAngles(
            (l1 + getRightArm()->getShoulder()->getMidLean1())*fallMag,
            (l2 + getRightArm()->getShoulder()->getMidLean2())*fallMag + (0.2f)*(1.f-fallMag), -0.1f);
          getRightArmInputData()->getClavicle()->setDesiredAngles(0.0f, 0.0f, 0.0f);
          getRightArmInputData()->getWrist()->setDesiredTwist(0.0f);
          getRightArmInputData()->getElbow()->setDesiredAngle(0.8f*fallMag + 0.8f*(1.f-fallMag));
       }
      }

    }

    /*
    */
    void NmRsCBUShot::defaultArmMotion_armsBrace()    
    {
      const float scaleRightness = 1.f;
      const float scaleBackwardsRightness = 0.5f;
      const float scaleContraRightness = 0.8f;
      const float backwardsScale = 0.5f;//1 has no problems 
      
      //Set muscle stiffnesses here for clarity
      float armStiffness = 11.f*m_upperBodyStiffness;
#if HACK_GTA4 // If armStiffness is allowed to be less than 1, it causes an assert on the 3Dof effector.
	  armStiffness = rage::Max(armStiffness, 1.0f);
#endif // HACK_GTA4
      const float armDamping = 0.7f;
      if(m_defaultArmMotion.leftBrace)
      {
        getLeftArm()->setBodyStiffness(getLeftArmInput(), armStiffness, armDamping);
        getLeftArmInputData()->getElbow()->setStiffness(0.75f*armStiffness, 0.75f*armDamping);
        getLeftArmInputData()->getWrist()->setStiffness(rage::Max(armStiffness - 1.0f,7.f), 1.75f);
      }
      if(m_defaultArmMotion.rightBrace)
      {
        getRightArm()->setBodyStiffness(getRightArmInput(), armStiffness, armDamping);
        getRightArmInputData()->getElbow()->setStiffness(armStiffness*0.75f, 0.75f*armDamping);
        getRightArmInputData()->getWrist()->setStiffness(rage::Max(armStiffness - 1.0f,7.f), 1.75f);
      }

      rage::Vector3 braceTarget;
      //static float velMult = 0.6f;//was 0.3f
      static float maxArmLength = 0.61f;//0.55 quite bent, 0.6 Just bent, 0.7 straight
      rage::Matrix34 tmCom;
      getSpine()->getSpine1Part()->getBoundMatrix(&tmCom); 
      rage::Vector3 vec;
      float mag, armTwist;//, straightness;
      float dragReduction = 1.f;
      //float maxSpeed = 5.f;//from balance 200.f; //from catch fall - ie out of range
      rage::Vector3 targetVel;
      //float magForwards = rage::Clamp(m_character->m_COMrotvel.Dot(tmCom.b),0.f,4.f);
      //magForwards *=0.25;

      rage::Vector3 rightDir1 = -tmCom.b;
      rage::Vector3 forwardDir1 = -tmCom.c;
      m_character->levelVector(rightDir1);
      m_character->levelVector(forwardDir1);


      rage::Vector3 reachVel = getSpine()->getSpine3Part()->getLinearVelocity() - m_character->getFloorVelocity();
      m_character->levelVector(reachVel);
      getLeftLeg()->getThigh()->getBoundMatrix(&tmCom);
      rage::Vector3 rightDir = tmCom.a;
      rage::Vector3 forwardDir = -tmCom.c;
      m_character->levelVector(rightDir);
      m_character->levelVector(forwardDir);

      float rightness = scaleRightness*reachVel.Dot(rightDir);
      float forwardness = reachVel.Dot(forwardDir);
      bool goingBackwards = false;
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", rightness);
      bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", forwardness);
#endif // ART_ENABLE_BSPY
      if (forwardness < 0.f)
      {
        //forwardness = 0.f;//both these work 0 gives more arms out but more arms back. Leaving it as a negative number makes a backward moving bending forward look odd
        forwardness = -backwardsScale*forwardness;//0..1  actually 2 is ok.
        goingBackwards = true;
      }
      float feetHeight = 0.5f*(getLeftLeg()->getFoot()->getPosition().Dot(m_character->m_gUp) + getRightLeg()->getFoot()->getPosition().Dot(m_character->m_gUp));
      float leftHandRelativeHeight = rage::Max(0.f,getRightArm()->getHand()->getPosition().Dot(m_character->m_gUp) - feetHeight);
      float rightHandRelativeHeight = rage::Max(0.f,getRightArm()->getHand()->getPosition().Dot(m_character->m_gUp) - feetHeight);


      //LEFT ARM
      if(m_defaultArmMotion.leftBrace)
      {
        armTwist = -0.5f;//arms in front or to side, 0 = arms a bit wider(1.0f for behind)
        braceTarget = -m_character->m_gUp;
        if (rightness > 0.f)
        {
          rightness *= scaleContraRightness*rage::Sinf(3.f*m_defaultArmMotion.timer1);
          maxArmLength = m_character->getRandom().GetRanged(0.51f,0.62f);
        }
        if (goingBackwards)
        {
        armTwist = -0.35f;////mmmmNewArms m_character->getRandom().GetRanged(-0.5f,0.5f);
          rightness *= scaleBackwardsRightness;
          braceTarget += rightness*rightDir;//didn't make alot of difference
          braceTarget += forwardness*forwardDir;
        }
        else
        {
          braceTarget += rightness*rightDir1;
          braceTarget += forwardness*forwardDir1;
        }


          // clamp left arm not to reach too far
          mag = braceTarget.Mag();
        braceTarget.Normalize();
        braceTarget *= rage::Min(mag , maxArmLength);
        braceTarget += getLeftArm()->getShoulder()->getJointPosition();;
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", rightness);
        bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", forwardness);
        bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", braceTarget);
        bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", goingBackwards);
        bspyScratchpad(m_character->getBSpyID(), "LeftArmsBrace", armTwist);
#endif // ART_ENABLE_BSPY

        //straightness = 0.8f;//m_character->getRandom().GetRanged(0.0f, 0.4f);
        targetVel = getLeftArm()->getClaviclePart()->getLinearVelocity();
        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
        NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
        ikInputData->setTarget(braceTarget);
        ikInputData->setTwist(armTwist);
        ikInputData->setDragReduction(dragReduction);
        ikInputData->setVelocity(targetVel);
        getLeftArm()->postInput(ikInput);
        getLeftArmInputData()->getClavicle()->setDesiredAngles(0.0f, 0.0f, 0.0f);

        // by default, characters tip hands up in preparation for connecting with object
        float desiredLean2 = 0.f;
        if (leftHandRelativeHeight < 0.7f)
        {
          desiredLean2 -= 2.5f*(0.7f-leftHandRelativeHeight);
        }
        getLeftArmInputData()->getWrist()->setDesiredAngles(0.f,desiredLean2,0.f);
        if (goingBackwards)
        {
          getLeftArmInputData()->getElbow()->setDesiredAngle(nmrsGetDesiredAngle(getLeftArm()->getElbow()) + 0.5f*rage::Sinf(5.f*m_defaultArmMotion.timer2));
        }
        getLeftArmInputData()->getElbow()->setDesiredAngle(rage::Max(nmrsGetDesiredAngle(getLeftArm()->getElbow()), 0.3f));
      }

      //RIGHT ARM
      getRightLeg()->getThigh()->getBoundMatrix(&tmCom);
      rightDir = tmCom.a;
      forwardDir = -tmCom.c;
      m_character->levelVector(rightDir);
      m_character->levelVector(forwardDir);

      rightness = scaleRightness*reachVel.Dot(rightDir);
      forwardness = reachVel.Dot(forwardDir);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", rightness);
      bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", forwardness);
#endif // ART_ENABLE_BSPY
      if (forwardness < 0.f)
      {
        //forwardness = 0.f;//both these work 0 gives more arms out but more arms back. Leaving it as a negative number makes a backward moving bending forward look odd
        forwardness = -backwardsScale*forwardness;//0..1  actually 2 is ok.
        goingBackwards = true;
      }
      if(m_defaultArmMotion.rightBrace)
      {
        targetVel = getRightArm()->getClaviclePart()->getLinearVelocity();
        armTwist = -0.5f;//arms in front or to side, 0 = arms a bit wider(1.0f for behind)
        braceTarget = -m_character->m_gUp;
        if (rightness < 0.f)
        {
          rightness *= scaleContraRightness*rage::Sinf(3.f*m_defaultArmMotion.timer2);
          maxArmLength = m_character->getRandom().GetRanged(0.51f,0.62f);
        }
        if (goingBackwards)
        {
          armTwist = -0.4f;
          rightness *= scaleBackwardsRightness;
          braceTarget += rightness*rightDir;
          braceTarget += forwardness*forwardDir;
        }
        else
        {
          braceTarget += rightness*rightDir1;
          braceTarget += forwardness*forwardDir1;
        }

        // clamp left arm not to reach too far
        mag = braceTarget.Mag();
        braceTarget.Normalize();
        braceTarget *= rage::Min(mag , maxArmLength);
        braceTarget += getRightArm()->getShoulder()->getJointPosition();;
#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", rightness);
        bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", forwardness);
        bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", braceTarget);
        bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", goingBackwards);
        bspyScratchpad(m_character->getBSpyID(), "RightArmsBrace", armTwist);
#endif // ART_ENABLE_BSPY

        //straightness = 0.8f;//m_character->getRandom().GetRanged(0.0f, 0.4f);
        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
        NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
        ikInputData->setTarget(braceTarget);
        ikInputData->setTwist(armTwist);
        ikInputData->setDragReduction(dragReduction);
        ikInputData->setVelocity(targetVel);
        ikInputData->setMatchClavicle(kMatchClavicle);
        getRightArm()->postInput(ikInput);

        float desiredLean2 = 0.f;
        if (rightHandRelativeHeight < 0.7f)
        {
          desiredLean2 -= 2.5f*(0.7f-rightHandRelativeHeight);
        }
        // limbs todo is this an internal blend?
        // looks like the intention is to blend with the results of IK.
        getRightArmInputData()->getWrist()->setDesiredAngles(0.f,desiredLean2,0.f);
        if (goingBackwards)
        {
          getRightArmInputData()->getElbow()->setDesiredAngle(nmrsGetDesiredAngle(getRightArm()->getElbow()) + 0.5f*rage::Sinf(5.f*m_defaultArmMotion.timer1));
        }
        getRightArmInputData()->getElbow()->setDesiredAngle(rage::Max(nmrsGetDesiredAngle(getRightArm()->getElbow()), 0.3f));
    }
  }
}

