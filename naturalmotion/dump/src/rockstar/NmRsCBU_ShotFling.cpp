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

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_StaggerFall.h" 

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
      m_fling.period = m_parameters.flingTime;
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
        rage::Vector3 Faxis = m_character->m_COMTM.a;//bodyRight
        rage::Vector3 toHitP = m_hitPointWorld - m_character->m_COM;
        m_character->levelVector(toHitP);
        m_character->levelVector(Faxis);
        Faxis.Normalize();
        toHitP.Normalize();
        float rightAngleOffset = -toHitP.Dot(Faxis)/4.f;//cos(angle between com2HitPoint and bodyRight projected onto horizontal plane)/4
        float leftAngleOffset = -rightAngleOffset;
        
        float wt = 0.f;
        float swing = 0.f;
        float stiff = 0.25f;
        m_body->setStiffness(14.0f, 0.5f, bvmask_ArmLeft | bvmask_ArmRight, &stiff);
        getLeftArmInputData()->getWrist()->setStiffness(12.0f, 0.5f);
        getRightArmInputData()->getWrist()->setStiffness(12.0f, 0.5f);

        if (m_fling.useLeft)
        {
          float angleFromTimer = 0.0f;
          if (m_fling.period > NM_RS_FLOATEPS)
            angleFromTimer = PI*m_fling.flingTimer/m_fling.period;
          //angleFromTimer - 0..PI
          //leftAngleOffset - -0.25..0.25
          //wt - 0.7..3.8 (+- 0.25)
          wt = rage::Min(rage::Abs(m_fling.leftDir)*(angleFromTimer + leftAngleOffset + 0.7f), 5.0f); 
          if (wt==5.f)
            m_fling.useLeft = false;
          wt = rage::Clamp(wt,0.f,PI-.3f);
          //wt - 0.7..2.84 or 0.0
          swing = rage::Abs(m_fling.leftDir)*rage::Min(4.f-wt,.32f); 
          //swing=0.32 or 0.0    (min of 1.16..3.3 and 0.32
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "shot.fling.left", wt);
          bspyScratchpad(m_character->getBSpyID(), "shot.fling.left", swing);
          bspyScratchpad(m_character->getBSpyID(), "shot.fling.left", leftAngleOffset);
          bspyScratchpad(m_character->getBSpyID(), "shot.fling.left", angleFromTimer);
#endif
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
          float angleFromTimer = 0.0f;
          if (m_fling.period > NM_RS_FLOATEPS)
            angleFromTimer = PI*m_fling.flingTimer/m_fling.period;
          wt = rage::Min(rage::Abs(m_fling.rightDir)*(angleFromTimer + rightAngleOffset + 0.7f), 4.5f); 
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
            wt = 1.0f;
            if (m_fling.period > NM_RS_FLOATEPS)
            wt = (1.f-m_fling.flingTimer/m_fling.period);
            getSpineInputData()->applySpineLean(-2.f * wt, 0.f); // * wt);
            if(!m_parameters.useHeadLook)
            {
              getSpine()->setBodyStiffness(getSpineInput(), 13.f ,0.5f, bvmask_CervicalSpine);
              getSpineInputData()->getLowerNeck()->setDesiredLean1(-2.f * wt);
              getSpineInputData()->getUpperNeck()->setDesiredLean1(-2.f * wt);
            }
          }
        }
        // otherwise do whatever was here before
        else//if(m_fling.bodyRegion == Region_Body)
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
         NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);//should be -2.  1 means reachForWound will be overwritten
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
         NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);//should be -2.  1 means reachForWound will be overwritten
         NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
         ikInputData->setTarget(fHeight);
         ikInputData->setTwist(0.0f);
         ikInputData->setDragReduction(1.0f);
         getRightLeg()->postInput(ikInput);
       }
       if (m_fling.flingTimer > 0.2f && (m_fling.bodyRegion == Region_LegsRight || m_fling.bodyRegion == Region_LegsLeft))
         m_disableBalance = false;//mmmmThis will not turn the balance back on.  If m_disableBalance = true m_falling will be true and this code will not be reachable anyway
      }//!m_falling
    }
    bool NmRsCBUShot::fling_exitCondition()
    {
      //mmmmtodo we should exit mid fling if !m_parameters.fling.
      return (m_hitTime > m_fling.period) || m_newHit;
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

    /*******************************************************************/
    /* Fling2                                                          */
    /*******************************************************************/
    bool NmRsCBUShot::fling2_entryCondition()
    {
      return m_parameters.fling2 && (m_parameters.fling2Left || m_parameters.fling2Right)&& m_newHit;
    }
    void NmRsCBUShot::fling2_entry()
    {
      m_fling2.flingTimer = 0.0f;
      m_fling2.period = m_parameters.fling2Time;//locks the period until another fling starts regardless if m_parameters.fling2Time changes
      m_fling2.relaxTimeL = m_parameters.fling2RelaxTimeL + m_parameters.fling2TimeBefore;
      m_fling2.relaxTimeR = m_parameters.fling2RelaxTimeR + m_parameters.fling2TimeBefore;
      float armLengthMult = 1.0f;
      if (m_character->getBodyIdentifier() == gtaWilma)//Wilma has shorter arms
        armLengthMult = 0.92f;//=0.580/0.628
      m_fling2.angleL = m_character->getRandom().GetRanged(m_parameters.fling2AngleMinL, m_parameters.fling2AngleMaxL);
      m_fling2.angleR = m_character->getRandom().GetRanged(m_parameters.fling2AngleMinR, m_parameters.fling2AngleMaxR);
      m_fling2.lengthL = m_character->getRandom().GetRanged(m_parameters.fling2LengthMinL*armLengthMult, m_parameters.fling2LengthMaxL*armLengthMult);
      m_fling2.lengthR = m_character->getRandom().GetRanged(m_parameters.fling2LengthMinR*armLengthMult, m_parameters.fling2LengthMaxR*armLengthMult);
      m_fling2.hasFlungL = false;
      m_fling2.hasFlungR = false;

      getLeftArmInputData()->getShoulder()->setOpposeGravity(1.f);
      getLeftArmInputData()->getClavicle()->setOpposeGravity(1.f);
      getLeftArmInputData()->getElbow()->setOpposeGravity(1.f);
      getRightArmInputData()->getShoulder()->setOpposeGravity(1.f);
      getRightArmInputData()->getClavicle()->setOpposeGravity(1.f);
      getRightArmInputData()->getElbow()->setOpposeGravity(1.f);

      // Make decisions about what to do:
      rage::Vector3 charUp = getSpine()->getHeadPart()->getPosition() - getSpine()->getPelvisPart()->getPosition();
      charUp.Normalize();
      rage::Vector3 charRight = getRightLeg()->getHip()->getJointPosition() - getLeftLeg()->getHip()->getJointPosition();
      charRight.Normalize();
      rage::Vector3 charForward;
      charForward.Cross(charUp, charRight);
      charForward.Normalize();
      static float impOut = 0.0f;//5.f;
      static float impFL = 0.0f;//10.f;
      static float impFR = 0.0f;//10.f;
      rage::Vector3 wristPos = getLeftArm()->getWrist()->getJointPosition();
      getLeftArm()->getLowerArm()->applyImpulse(-impOut*charRight + impFL*charForward, wristPos);
      wristPos = getRightArm()->getWrist()->getJointPosition();
      getRightArm()->getLowerArm()->applyImpulse(impOut*charRight + impFR*charForward, wristPos);
    }
    void NmRsCBUShot::fling2_tick(float timeStep)
    {

      m_fling2.flingTimer += timeStep;
      if (m_fling2.flingTimer > m_parameters.fling2TimeBefore)
      {
        //mmmtodo differently applied to left and right to give asymmetry
        static float armStiffness = 12.0f;
        float armMuscleStiffness = 0.95f;
        static float armDamping = 0.7f;
        static float armStiffnessEnd = 7.0f;
        static float armMuscleStiffnessEnd = 0.25f;
        static float armDampingEnd = 0.7f;
        
        static float predTime = 0.2f;
        static float opposeGravity = 0.5f;
        static float opposeGravityEnd = 0.0f;
        static float staticShoulderVal = 1.0f;
        static float staticShoulderValR = 1.0f;

        // Make decisions about what to do:
        rage::Vector3 charUp = getSpine()->getHeadPart()->getPosition() - getSpine()->getPelvisPart()->getPosition();
        charUp.Normalize();
        rage::Vector3 charRight = getRightLeg()->getHip()->getJointPosition() - getLeftLeg()->getHip()->getJointPosition();
        charRight.Normalize();
        rage::Vector3 charForward;
        charForward.Cross(charUp, charRight);
        charForward.Normalize();

        rage::Vector3 braceTarget;
        float armLength;
        float theta;
        float shoulderVel;
        rage::Vector3 predShoulderToWrist;
        float predShoulderToWrisDot;      
        float armTwist;
        const float dragReduction = 1.0f;
        rage::Vector3 targetVel;
        //This is a totally IK solution
        //  subPriority = 2. So overwites brace(sp=-3), fling (sp=1 [should be-2]), default_arms (sp=0)
        //  but it would overwrite reachForWound(sp=0) therefore we do not fling if reaching enabled.
        NmRsLimbInput ikInput;
        NmRsIKInputWrapper* ikInputData;
        NmRsCBUStaggerFall* staggerFallTask = (NmRsCBUStaggerFall*)m_cbuParent->m_tasks[bvid_staggerFall];
        //Override staggerArms even if staggerFallTask->m_parameters.m_upperBodyReaction = true
        //  This is done in stagger.  All we do here is allow fling2 to be set even if reaching enabled
        //  otherwise reaching would also override staggerFall arms.
        bool overrideStagger = (m_parameters.fling2OverrideStagger && staggerFallTask->isActive() && staggerFallTask->m_parameters.m_upperBodyReaction);

        if (m_parameters.fling2Left && ((!m_reachLeftEnabled) || overrideStagger))
        {
          m_fling2Left = true;
          //Set arm muscles
          //Phase1
          armMuscleStiffness = m_parameters.fling2MStiffL;
          if (armMuscleStiffness > 0.0f)
          {
            getLeftArm()->setBodyStiffness(getLeftArmInput(), armStiffness, armDamping, bvmask_ArmLeft, &armMuscleStiffness);
            getLeftArmInputData()->getElbow()->setStiffness(0.75f*armStiffness, 0.75f*armDamping, &armMuscleStiffness);
          }
          else
          {
            getLeftArm()->setBodyStiffness(getLeftArmInput(), armStiffness, armDamping, bvmask_ArmLeft);
            getLeftArmInputData()->getElbow()->setStiffness(0.75f*armStiffness, 0.75f*armDamping);
          }
          getLeftArmInputData()->getWrist()->setStiffness(armStiffness, 1.0f);//mmmmtodo sort out wrist muscles

          getLeftArmInputData()->getShoulder()->setOpposeGravity(opposeGravity);
          getLeftArmInputData()->getClavicle()->setOpposeGravity(opposeGravity);
          getLeftArmInputData()->getElbow()->setOpposeGravity(opposeGravity);       
         //Phase2
          if (m_fling2.flingTimer > m_fling2.relaxTimeL)
          {
            getLeftArm()->setBodyStiffness(getLeftArmInput(), armStiffnessEnd, armDampingEnd, bvmask_ArmLeft, &armMuscleStiffnessEnd);
            getLeftArmInputData()->getElbow()->setStiffness(0.75f*armStiffnessEnd, 0.75f*armDampingEnd, &armMuscleStiffnessEnd);   
            getLeftArmInputData()->getWrist()->setStiffness(armStiffness, 1.0f, &armMuscleStiffnessEnd);//mmmmtodo sort out wrist muscles
            getLeftArmInputData()->getShoulder()->setOpposeGravity(opposeGravityEnd);
            getLeftArmInputData()->getClavicle()->setOpposeGravity(opposeGravityEnd);
            getLeftArmInputData()->getElbow()->setOpposeGravity(opposeGravityEnd);
          }

          //Decide whether the fling has flung to the side.  If so go into Phase2 and relax and straighten arm
          //  Is the shoulder effector anglularVelocity below a threshold?
          rage::Vector3 leftShoulderAngVel(m_leftShoulderAngles);
          m_leftShoulderAngles.Set(
            nmrsGetActualTwist(getLeftArm()->getShoulder()),
            nmrsGetActualLean1(getLeftArm()->getShoulder()),
            nmrsGetActualLean2(getLeftArm()->getShoulder()));
          leftShoulderAngVel -= m_leftShoulderAngles; 
          shoulderVel = leftShoulderAngVel.Mag()/timeStep;
          if (shoulderVel < staticShoulderVal && m_fling2.flingTimer > m_parameters.fling2TimeBefore + 0.1f)
            m_fling2.hasFlungL = true;

          //In Phase2 we could straighten the arm by increasing the armLength here
          //  - it would straighten the arm along the angle from shoulder to wrist.
          //  - This looks like the arm goes up to much though.
          //Instead we straighten the arm by setting a maxElbow angle in IK which preserves the angle of the upperArm
          // and makes the elbow look as though it has straightened more naturally/under gravity
          armLength = m_fling2.lengthL;
          theta = m_fling2.angleL;
          braceTarget = armLength*(rage::Sinf(theta)*charUp - rage::Cosf(theta)*charRight); 
          braceTarget += getLeftArm()->getShoulder()->getJointPosition();
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "fling2", shoulderVel);
          bspyScratchpad(m_character->getBSpyID(), "fling2", braceTarget);
#endif // ART_ENABLE_BSPY
          //Change Ik twist dependent on where the wrist is going to be.
          //  Go more negative the more it is out to the side or above the shoulder
          predShoulderToWrist = getLeftArm()->getWrist()->getJointPosition()+predTime*getLeftArm()->getHand()->getLinearVelocity();
          predShoulderToWrist -= getLeftArm()->getShoulder()->getJointPosition();
          predShoulderToWrisDot = predShoulderToWrist.Dot(charRight);
          if (predShoulderToWrist.Dot(charUp) > 0.0f)
            predShoulderToWrisDot -= predShoulderToWrist.Dot(charUp);
          armTwist = rage::Clamp((predShoulderToWrisDot+0.2f)*2.5f, -1.7f, 0.2f);

          targetVel = getLeftArm()->getClaviclePart()->getLinearVelocity();
          ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(2, 1.0f DEBUG_LIMBS_PARAMETER("FLIK"));//MMMMtod0 Make so overwites brace(sp=-3), fling (sp=-2) but not reachForWound(sp=0)
          ikInputData = ikInput.getData<NmRsIKInputWrapper>();
          ikInputData->setTarget(braceTarget);
          ikInputData->setTwist(armTwist);
          ikInputData->setDragReduction(dragReduction);
          ikInputData->setVelocity(targetVel);
          //In Phase2 straighten the arm by setting a maxElbow angle in IK which preserves the angle of the upperArm
          // and makes the elbow look as though it has straightened more naturally/under gravity
          if (m_fling2.hasFlungL)
          {
            ikInputData->setMaximumElbowAngle(0.1f);
            m_fling2.relaxTimeL = m_fling2.flingTimer;//Tell the muscles to go into phase2 now
          }
          ikInputData->setMatchClavicle(kMatchClavicleBetter);
          getLeftArm()->postInput(ikInput);
          getLeftArmInputData()->getWrist()->setDesiredAngles(0.0f, 0.0f, 0.24f);
        }//if (m_parameters.fling2Left)
      

        //RIGHT
         if (m_parameters.fling2Right && ((!m_reachRightEnabled) || overrideStagger))
        {
          m_fling2Right = true;

          armMuscleStiffness = m_parameters.fling2MStiffR;
          if (armMuscleStiffness > 0.0f)
          {
            getRightArm()->setBodyStiffness(getRightArmInput(), armStiffness, armDamping, bvmask_ArmRight, &armMuscleStiffness);
            getRightArmInputData()->getElbow()->setStiffness(0.75f*armStiffness, 0.75f*armDamping, &armMuscleStiffness);
          }
          else
          {
            getRightArm()->setBodyStiffness(getRightArmInput(), armStiffness, armDamping);
            getRightArmInputData()->getElbow()->setStiffness(0.75f*armStiffness, 0.75f*armDamping);
          }

          getRightArmInputData()->getWrist()->setStiffness(armStiffness, 1.0f);//mmmmtodo sort out wrist muscles
          getRightArmInputData()->getShoulder()->setOpposeGravity(opposeGravity);
          getRightArmInputData()->getClavicle()->setOpposeGravity(opposeGravity);
          getRightArmInputData()->getElbow()->setOpposeGravity(opposeGravity);

          if (m_fling2.flingTimer > m_fling2.relaxTimeR)
          {
            //elbow not set for separately for right for asymmetry
            getRightArm()->setBodyStiffness(getLeftArmInput(), armStiffnessEnd, armDampingEnd, bvmask_ArmRight, &armMuscleStiffnessEnd);
            getRightArmInputData()->getShoulder()->setOpposeGravity(opposeGravityEnd);
            getRightArmInputData()->getClavicle()->setOpposeGravity(opposeGravityEnd);
            getRightArmInputData()->getElbow()->setOpposeGravity(opposeGravityEnd);
            //mmmmtodo sort out wrist muscles
            ////pose held for right for asymmetry
            //getRightArm()->holdPose(getRightArmInput());//This is overwritten by ik
          }

          //Decide whether the fling has flung to the side.  If so go into phase2 and relax and straighten arm
          //  Is the shoulder effector anglularVelocity below a threshold?
          rage::Vector3 rightShoulderAngVel(m_rightShoulderAngles);
          m_rightShoulderAngles.Set(
            nmrsGetActualTwist(getRightArm()->getShoulder()),
            nmrsGetActualLean1(getRightArm()->getShoulder()),
            nmrsGetActualLean2(getRightArm()->getShoulder()));
          rightShoulderAngVel -= m_rightShoulderAngles; 
          shoulderVel = rightShoulderAngVel.Mag()/timeStep;
          if (shoulderVel < staticShoulderValR && m_fling2.flingTimer > m_parameters.fling2TimeBefore + 0.1f)
            m_fling2.hasFlungR = true;

          //In Phase2 we could straighten the arm by increasing the armLength here
          //  - it would straighten the arm along the angle from shoulder to wrist.
          //  - This looks like the arm goes up to much though.
          //Instead we straighten the arm by setting a maxElbow angle in IK which preserves the angle of the upperArm
          // and makes the elbow look as though it has straightened more naturally/under gravity
          armLength = m_fling2.lengthR;
          theta = m_fling2.angleR;
          braceTarget = armLength*(rage::Sinf(theta)*charUp + rage::Cosf(theta)*charRight);
          braceTarget += getRightArm()->getShoulder()->getJointPosition();
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "fling2", shoulderVel);
          bspyScratchpad(m_character->getBSpyID(), "fling2", braceTarget);
#endif // ART_ENABLE_BSPY
          //Change Ik twist dependent on where the wrist is going to be.
          //  Go more negative the more it is out to the side or above the shoulder
          predShoulderToWrist = getRightArm()->getWrist()->getJointPosition()+predTime*getRightArm()->getHand()->getLinearVelocity();
          predShoulderToWrist -= getRightArm()->getShoulder()->getJointPosition();
          predShoulderToWrisDot = -predShoulderToWrist.Dot(charRight);
          if (predShoulderToWrist.Dot(charUp) > 0.0f)
            predShoulderToWrisDot -= predShoulderToWrist.Dot(charUp);
          armTwist = rage::Clamp((predShoulderToWrisDot+0.2f)*2.5f, -1.7f, 0.2f);

          targetVel = getRightArm()->getClaviclePart()->getLinearVelocity();
          ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(2, 1.0f DEBUG_LIMBS_PARAMETER("FLIK"));
          ikInputData = ikInput.getData<NmRsIKInputWrapper>();
          ikInputData->setTarget(braceTarget);
          ikInputData->setTwist(armTwist);
          ikInputData->setDragReduction(dragReduction);
          ikInputData->setVelocity(targetVel);
          //In Phase2 straighten the arm by setting a maxElbow angle in IK which preserves the angle of the upperArm
          // and makes the elbow look as though it has straightened more naturally/under gravity
          if (m_fling2.hasFlungR)
          {
            ikInputData->setMaximumElbowAngle(0.1f);
            m_fling2.relaxTimeR = m_fling2.flingTimer;
          }
          ikInputData->setMatchClavicle(kMatchClavicle);
          getRightArm()->postInput(ikInput);
          //getRightArmInputData()->getClavicle()->setDesiredAngles(0.0f, 0.0f, 0.0f);

          getRightArmInputData()->getWrist()->setDesiredAngles(0.0f, 0.0f, 0.24f);
        }//if (m_parameters.fling2Right)

      }//if (m_fling2.flingTimer > m_parameters.fling2TimeBefore)
    }
    bool NmRsCBUShot::fling2_exitCondition()
    {
      return (m_hitTime > (m_fling2.period + m_parameters.fling2TimeBefore)) || m_newHit || (!(m_parameters.fling2 && (m_parameters.fling2Left || m_parameters.fling2Right)));
    }
    void NmRsCBUShot::fling2_exit()
    {
      m_fling2Left = false;
      m_fling2Right = false;
    }
}

