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
* <DESCRIPTION OF BEHAVIOUR>
* Designed for when a character jumps off an object. Controls the 
* character during a jump/fall. During the jump/fall tries to 
* maintain an upright stance, windmills arms, pedal legs and 
* looks down. As approaches the ground tries to keep balance or 
* doing a forward roll following by standing up. 
* (Try to improve highfall)
*/


#include "NmRsInclude.h"

#if ALLOW_TRAINING_BEHAVIOURS

#include "NmRsCBU_Landing.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_Pedal.h"
#include "NmRsCBU_ArmsWindmill.h"
#include "NmRsCBU_RollUp.h"
#include "NmRsCBU_Catchfall.h"
#include "NmRsCBU_BodyBalance.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_RollDownStairs.h"

namespace ART
{
  NmRsCBULanding::NmRsCBULanding(ART::MemoryManager* services) : CBUTaskBase(services, bvid_landing),
    m_leftArm(0), 
    m_rightArm(0), 
    m_leftLeg(0), 
    m_rightLeg(0), 
    m_spine(0)
  {
    initialiseCustomVariables();
  }

  NmRsCBULanding::~NmRsCBULanding(){}

  void NmRsCBULanding::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    // reset the variables
    m_timer = 0.0f;
    m_hasCollidedWithWorld = false;
    m_controlOrientation = true;
    m_willLandOk = true;
    m_timeToImpact = 50.0f;
    m_forwardVel = 0.0f;
    m_messageSent = false; 
    m_goToCatchFallFlag = false;
    m_rollToBBtimer = 0.0f;
    m_averageComVel = 0.0f;
    m_alreadyCollidedL = false;
    m_alreadyCollidedR = false;
    m_phase1FW = true;
    m_phase2FW = false;
    m_phase3FW = false;
    m_phase1DoneFW = false;
    m_phase2DoneFW = false;
    m_nextFW = 0;
    m_firstFrame = true;
    m_lastFrame = true;
    m_timerTryToStandUp = 0;
    m_healthCatchFall = 1.0f;
    m_waitFW = 0;
    m_testSideRoll = true;
    m_lastKneeStrength = 1.0f;
    m_isStayingOnHisHead = false;
    m_initCheatingTorque = true;
    m_lastCOMUp = 0;
    m__spineAlreadyCollided = false;
    m_ForwardRollFinished = false;
    m_lastApplied = 0.1f;
    m_timeStep = 0.f;
    m_sideRollTest = false;
    m_slopeGround = 0.f;
    alreadyForcedToInitialize = false;
    // reset state machine to initial state
    Reset();
  }

  void NmRsCBULanding::onActivate()
  {
    Assert(m_character);

    // locally cache the limb definitions
    m_leftArm = m_character->getLeftArmSetup();
    m_rightArm = m_character->getRightArmSetup();
    m_leftLeg = m_character->getLeftLegSetup();
    m_rightLeg = m_character->getRightLegSetup();
    m_spine = m_character->getSpineSetup();

#if LANDING_AUTOMATIC_WEAPON_DETECTION
    m_character->setDontRegisterCollsion(1.0f, 0.05f);//mmmmtodo Note this will be turned off if balancerCollisionsReaction deactivates
#endif
    // state machine init: will execute first real state's enter function
    Initialize();

    //Scale incoming angVel
    for (int i = 0; i<m_character->getNumberOfParts(); i++)
    {
      if(m_character->isPartInMask("fb", i))
      {
        NmRsGenericPart *part = m_character->getGenericPartByIndex(i);
        rage::Vector3 rotVel = part->getAngularVelocity();
        rotVel *= m_parameters.m_factorToReduceInitialAngularVelocity;
        part->setAngularVelocity(rotVel, false);
      }
    }

    //A: decide things
    decideThings();
  }

  void NmRsCBULanding::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();

    //Turn off not Registering certain collisions
    m_character->setDontRegisterCollsion(-1.f, -1.f); //mmmmtodo note this may reset balancerCollisionsReaction's settings

    // turn off all possible sub behaviours (new wrapper)
    m_character->deactivateTask(bvid_bodyRollUp);
    m_character->deactivateTask(bvid_rollDownStairs);
    m_character->deactivateTask(bvid_catchFall);
    m_character->deactivateTask(bvid_bodyBalance);
    m_character->deactivateTask(bvid_headLook);
    m_character->deactivateTask(bvid_pedalLegs);
    m_character->deactivateTask(bvid_armsWindmill);
    //default values
    m_character->enableFootSlipCompensationActive(true);
    //m_character->m_footSlipMult = 1.0f;
  }

  // tick: not much happening here, most stuff is in the state machine update and individual state's ticks
  // ---------------------------------------------------------------------------------------------------------
  CBUTaskReturn NmRsCBULanding::onTick(float timeStep)
  {
    m_timeStep = timeStep;

#if LANDING_AUTOMATIC_WEAPON_DETECTION
    // contact filtering won't affect what we're getting in 1st frame, hence wait:
    if(m_timer > 0.0f)
      m_hasCollidedWithWorld = m_character->hasCollidedWithWorld("fb");

#else
    m_hasCollidedWithWorld = m_character->hasCollidedWithWorld("fb");
#endif

    m_timer += m_character->getLastKnownUpdateStep();

    // update state machine
    Update();

    //A: decide things and apply torques --Gumdrops
    decideThings();

    return eCBUTaskComplete;
  }

  // state machine states and transitions
  // ---------------------------------------------------------------------------------------------------------
  bool NmRsCBULanding::States( StateMachineEvent event, int state )
  {
    BeginStateMachine

      // Initial State: Falling  ////////////////////////////////////////////////////
      State( HF_Falling )
      OnUpdate
    {
      NM_RS_DBG_LOGF(L"Falling behavior");
      duringFall();
      if(m_testSideRoll){
        rage::Vector3 COMVel = m_character->m_COMvel;
        float verticalVel = COMVel.Dot(m_character->getUpVector());
        if(verticalVel<-1.f){//do not want to test at the first frame, wait a little bit and do it once
          m_testSideRoll = false;
          m_sideRollTest = isVerticalHighFall();
        }
      }
      float toCloseToGround = (m_timeToImpact - m_parameters.m_catchfalltime - 0.15f);
      bool toLand = (m_timer > toCloseToGround);
      bool toBailOut =  (toLand && !m_willLandOk) || (toLand && m_hasCollidedWithWorld);//(m_hasCollidedWithWorld && (m_timer>0.4f)));            
      if (toBailOut && !m_parameters.m_ignorWorldCollisions)
      {
        //if can't land OK (not on his feet or too much rotation)
        SetState( HF_BailOut );
      }
      else if (m_parameters.m_sideRoll && m_sideRollTest) {
        //if it's a high vertical fall
        //overwrite parameters : will try to arrive straight legs under COM
        m_parameters.m_aimAngleBase = 0.05f;
        m_parameters.m_legL = 1.0f;
        if (toLand)
        {
          //will put feet randomly on a side to fall on the other
          m_rightSideRoll = m_character->getRandom().GetBool();
          SetState( HF_SideRoll);
        }

      }
      else if(toLand)
      {
        //if can land OK
        SetState( HF_PrepareForLanding );
      }
    }

    OnExit
    {
      m_character->deactivateTask(bvid_headLook);
      m_character->deactivateTask(bvid_pedalLegs);
      m_character->deactivateTask(bvid_armsWindmill);
    }
    /**************/
    /*   Bail Out */
    /**************/
    State( HF_BailOut )      
      OnEnter
      m_controlOrientation = false;

    OnUpdate
      duringBailOut();
    /***************************/
    /*   Prepare For Landing   */
    /***************************/
    State( HF_PrepareForLanding )      
      OnUpdate
    {
      NM_RS_DBG_LOGF(L"Prepare For Landing behavior");
      duringPrepareForLanding();
      bool decideToBalance = m_hasCollidedWithWorld; 
      bool decideToCatchFall = (m_hasCollidedWithWorld && m_goToCatchFallFlag);
      if (decideToCatchFall)
      {
        SetState( HF_CatchFall);
      }
      else if(decideToBalance)
      {
        SetState(HF_Balance);             
      } 
    }
    OnExit
      m_controlOrientation = false;
    /******************/
    /*   Catch Fall   */
    /******************/
    State( HF_CatchFall )      
      OnUpdate
      NM_RS_DBG_LOGF(L"Catch Fall behavior");
    duringCatchFall();
    /******************/
    /*    Balance     */
    /******************/
    State( HF_Balance )      
      OnUpdate
    {
      NM_RS_DBG_LOGF(L"body balance behavior");
      duringBalance();
      m_nextBalance = decidePhaseBalance();
      if (m_nextBalance==1)
      {m_healthCatchFall = 0.8f;
      SetState( HF_CatchFall );
      }
      if (m_nextBalance==2)
      {
        m_nextFW = decidePhaseFW();
        SetState( HF_ForwardRoll );
      }
      if (m_nextBalance==3)
      {
        m_healthCatchFall = 0.5f;
        SetState( HF_CatchFall );
      }
    }
    OnExit
      m_character->deactivateTask(bvid_bodyBalance);
    State( HF_ForwardRoll )
      OnEnter
      m_rollDownTimer = 0;
    OnUpdate
      duringForwardRoll();
    m_nextFW = decidePhaseFW();
    /*********************************/
    /*    Test head on the ground    */
    /*********************************/
    bool headCollided = isStayingOnHisHead(6);
    if (headCollided)
    {//catchfall really weak
      m_healthCatchFall = 0.3f;
      SetState( HF_CatchFall );
    }
    if (m_nextFW==2)
    {//catchfall healthy
      m_healthCatchFall = 1.f;
      m_nextFW = 0;
      SetState( HF_CatchFall );
    }
    if (m_nextFW==3)
    {///catchfall really weak
      m_healthCatchFall = 0.5f;
      m_nextFW = 0;
      SetState( HF_CatchFall );
    }
    if (m_nextFW==1){
      //forward roll done, stand up
      m_nextFW = 0;
      SetState( HF_StandUp );
    }
    OnExit
      m_character->deactivateTask(bvid_bodyRollUp);
    m_character->deactivateTask(bvid_rollDownStairs);

    /****************************************/
    /*        Stand up  behavior            */
    /****************************************/
    State( HF_StandUp )      
      OnUpdate
    {
      duringStandUp();
      float legLengthP = lengthLegsPercentage();
      float legLength = lengthLegs();
      rage::Vector3 UpVector =  m_character->getUpVector();
      float HeightCOM = m_character->m_COM.Dot(UpVector);
      rage::Vector3 footPL = m_leftLeg->getFoot()->getPosition();
      rage::Vector3 footPR = m_rightLeg->getFoot()->getPosition();
      float LowestFoot = rage::Min(footPR.Dot(UpVector),footPL.Dot(UpVector));
      HeightCOM -= LowestFoot;
      bool StandUp = legLengthP> 0.7f && (HeightCOM> 0.6f*legLength);//one or both legs are straight

      if (m_timerTryToStandUp>6){
        //too long too stand up --> catch fall
        m_healthCatchFall = 1.f;
        SetState( HF_CatchFall);
      }
      //already stand up
      if(StandUp){
        SetState( HF_BalanceStandUp);
      }
    }
    OnExit
      m_character->deactivateTask(bvid_bodyBalance);
    /****************************************/
    /*     Balance Stand up  behavior       */
    /****************************************/
    State( HF_BalanceStandUp )      
      OnUpdate
    {
      NM_RS_DBG_LOGF(L"body balance stand up behavior");
      duringBalanceStandUp();
      rage::Vector3 UpVector =  m_character->getUpVector();
      float HeightCOM = m_character->m_COM.Dot(UpVector);
      rage::Vector3 footPL = m_leftLeg->getFoot()->getPosition();
      rage::Vector3 footPR = m_rightLeg->getFoot()->getPosition();
      float LowestFoot = rage::Min(footPR.Dot(UpVector),footPL.Dot(UpVector));
      HeightCOM -= LowestFoot;
      float legLength = lengthLegsPercentage();
      bool NotStandUp = HeightCOM< 0.7f*legLength; 
      if (NotStandUp){
        if(m_timerTryToStandUp>3){
          m_healthCatchFall = 1.f;
          SetState( HF_CatchFall);
        }
      }
    }
    OnExit
      m_character->deactivateTask(bvid_bodyBalance);
    /****************************************/
    /*        Side Roll behavior            */
    /****************************************/
    State( HF_SideRoll )      
      OnUpdate
    {
      NM_RS_DBG_LOGF(L"Side Roll behavior");

      if (!m_hasCollidedWithWorld){
        duringPrepareForSideRoll();
      }
      else if (m_hasCollidedWithWorld)
      {
        //will do a catch fall
        m_healthCatchFall = 1.0f;
        duringSideRoll();
      }
    }

    EndStateMachine
  }


  // bail out tick
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBULanding::duringBailOut()
  {
    NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
    Assert(rollUpTask);
    if (!(rollUpTask->isActive()))
    {
      rollUpTask->updateBehaviourMessage(NULL);
      rollUpTask->m_parameters.m_useArmToSlowDown=(1.9f);
      rollUpTask->m_parameters.m_armReachAmount=(1.40f);
      rollUpTask->m_parameters.m_stiffness=(m_parameters.m_bodyStiffness*0.7f);
      rollUpTask->m_parameters.m_legPush=(0.0f);
      rollUpTask->m_parameters.m_asymmetricalLegs=(0.75f);
      rollUpTask->activate();
    }

    // if we bailed out and came to rest we're finished here
    if ((m_character->m_COMvelRelativeMag < 0.05f) && !m_messageSent) 
    {
      m_messageSent = true;
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 0;
        strcpy(feedback->m_behaviourName, NMLandingFeedbackName);
        feedback->onBehaviourFailure();
      }
    }
  }

  // get feet in position etc..
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBULanding::duringPrepareForLanding()
  {
    callMaskedEffectorFunctionFloatArg(m_character, "ub", 0.5f, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
    //to know the direction during falling
    if(m_lastFrame){m_lastPostFall = m_character->m_COM;m_lastFrame = false;m_PFLtimer = 0.0f;}
    m_PFLtimer += m_character->getLastKnownUpdateStep();
    // the state we want to go into for the landing; Arms up, lean forward, head forward, knees slightly bent, legs forward
    m_leftLeg->getKnee()->setMuscleStiffness(1.0f);
    m_rightLeg->getKnee()->setMuscleStiffness(1.0f);
    m_leftLeg->getHip()->setMuscleStiffness(0.275f);
    m_rightLeg->getHip()->setMuscleStiffness(0.275f);
    rage::Vector3 gUp = m_character->m_gUp;
    gUp.Normalize();
    //set the head position
    m_spine->getLowerNeck()->setDesiredLean1(0.2f);
    m_spine->getUpperNeck()->setDesiredLean1(0.5f);
    m_spine->getLowerNeck()->setDesiredLean2(0.f);
    m_spine->getUpperNeck()->setDesiredLean2(0.f);
    m_spine->getLowerNeck()->setDesiredTwist(0.f);
    m_spine->getUpperNeck()->setDesiredTwist(0.f);

    //set the spine position
    rage::Vector3 horComVel =m_character->m_COMvel;
    m_character->levelVector(horComVel);
    float horVel = horComVel.Mag();
    if(horVel>5.f){
      m_character->applySpineLean(0.6f,0.f);
    }
    rage::Vector3 hipVel = m_spine->getPelvisPart()->getLinearVelocity();
    // The Arms:
    rage::Vector3 ikPos;
    rage::Vector3 hSideVec;
    ikPos.AddScaled(m_spine->getHeadPart()->getPosition(),gUp,m_parameters.m_armsUp);
    //add frontward 
    rage::Vector3 frontward = m_character->m_COMvel;
    m_character->levelVector(frontward);

    frontward.Normalize();
    hSideVec.Cross(frontward,gUp);
    //hSideVec.Negate();
    hSideVec.Normalize();
    //to avoid head collision during landing
    rage::Vector3 forVel = m_character->m_COMvel;
    m_character->levelVector(forVel);
    float forwardVel = forVel.Mag();

    ikPos.AddScaled(frontward,m_parameters.m_armsFrontward+(forwardVel*0.02f));
    float sideSet = 0.25f;
    ikPos.AddScaled(hSideVec,sideSet);
    rage::Vector3 rightPos = ikPos;
    ikPos.AddScaled(hSideVec,-2.0f*sideSet);
    rage::Vector3 leftPos = ikPos;
    if (m_goToCatchFallFlag)
    {
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "PrepareForLanding.", m_goToCatchFallFlag);
#endif
      //too disoriented will do a catchfall
      ikPos = m_spine->getHeadPart()->getPosition();
      ikPos.AddScaled(frontward,0.25f);//hands in direction of the velocity
      rage::Matrix34 pelvisMat ;
      m_spine->getPelvisPart()->getMatrix(pelvisMat);
      rage::Vector3 frontward = pelvisMat.c;
      m_character->levelVector(frontward);
      frontward.Negate();
      m_character->levelVector(frontward);
      frontward.Normalize();
      hSideVec.Cross(frontward,gUp);
      hSideVec.Normalize();
      ikPos = m_spine->getHeadPart()->getPosition();
      ikPos.AddScaled(frontward,0.35f);
      ikPos.AddScaled(hSideVec,0.25f);
      rightPos = ikPos;
      ikPos.AddScaled(hSideVec,-0.7f);
      leftPos = ikPos;
    }
    if (m_parameters.m_forwardRoll)
    {
      m_character->rightArmIK(rightPos,0.0f,0.0f, NULL, &hipVel);
      rage::Vector3 Normal = m_character->getUpVector();
#if ART_ENABLE_BSPY && LandingBSpyDraw
      m_character->bspyDrawPoint(rightPos, 0.05f, rage::Vector3(1,1,0));
      m_character->bspyDrawLine(rightPos, rightPos+Normal*0.1f, rage::Vector3(1,1,0));
      m_character->bspyDrawPoint(leftPos, 0.05f, rage::Vector3(0,1,0));
      m_character->bspyDrawLine(leftPos,leftPos+Normal*0.1f, rage::Vector3(0,1,0));
#endif
      m_character->leftArmIK(leftPos,0.0f,0.0f, NULL, &hipVel);
    }
    //to avoid hands oriented sideways
    setHands(0.3f);

    /*********/
    /* Legs: */
    /*********/

    //modify strength and damping for IK
    float strength = 14.f;
    float damping = 0.5f;
    m_character->setBodyStiffness(strength, damping,"lb");
    //axis : from the horizontal velocity
    horComVel =m_character->m_COMvel;
    m_character->levelVector(horComVel);
    horVel = horComVel.Mag();
    rage::Vector3 frontOffset = horComVel;
    m_character->levelVector(frontOffset);
    frontOffset.Normalize();
    rage::Vector3 sideOffset ;
    sideOffset.Cross(frontOffset,m_character->getUpVector());
    m_character->levelVector(sideOffset);
    sideOffset.Normalize();

    //Modification of ikPos : will try to put the feet behind the COM
    float behind = m_parameters.m_feetBehindCOM;
    float behindVel = horVel*m_parameters.m_feetBehindCOMVel;
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", behind+behindVel);
#endif

    // work out the average of the two hip positions
    rage::Vector3 hipPL = m_leftLeg->getHip()->getJointPosition();
    rage::Vector3 hipPR = m_rightLeg->getHip()->getJointPosition();
    rage::Vector3 hipP = hipPL + hipPR;
    hipP.Scale(0.5f);

    //init of IKPos : under the hip
    rage::Vector3  hipPos = hipPL;
    rage::Vector3  kneePos = m_leftLeg->getKnee()->getJointPosition();
    rage::Vector3  anklePos = m_leftLeg->getAnkle()->getJointPosition();
    rage::Vector3 leg = hipPos - kneePos;
    float maxLegLength = leg.Mag();
    leg = kneePos - anklePos;
    maxLegLength += leg.Mag();

    //add the frontOffset : left leg
    ikPos =gUp*-m_parameters.m_legL*maxLegLength;
    rage::Vector3 rotVel = sideOffset;
    rage::Vector3 origin = ikPos;
    rage::Quaternion rot;
    float sinAngle = rage::Clamp((behind+behindVel)/ikPos.Mag(),-1.f,1.f);
    float angleToApply = rage::AsinfSafe(sinAngle);
    float numberOfFramePrepareForLanding = m_parameters.m_catchfalltime/m_timeStep;
    m_lastApplied +=(1.f/(numberOfFramePrepareForLanding-1.f));//normally will be one just before landing
    //ikPos is moving to try to keep the leg length desired
    float part = rage::Clamp(m_lastApplied,0.f,1.f);
    rotVel.Scale(-(angleToApply*part));
    rot.FromRotation(rotVel);
    rot.Transform(origin,ikPos);
    leftPos = ikPos;

    //add the frontOffset ; right leg
    ikPos =gUp*-m_parameters.m_legL*maxLegLength;
    rotVel = sideOffset;
    origin = ikPos;
    sinAngle = rage::Clamp((behind+behindVel)/ikPos.Mag(),-1.f,1.f);
    angleToApply = rage::AsinfSafe(sinAngle);
    rotVel.Scale(-(angleToApply*part));
    rot.FromRotation(rotVel);
    rot.Transform(origin,ikPos);
    rightPos = ikPos;

    //left leg : add the sideOffset and do the IK
    ikPos = leftPos;
    sinAngle = rage::Clamp((m_parameters.m_sideD)/ikPos.Mag(),-1.f,1.f);
    float angle = rage::AsinfSafe(sinAngle);
    rotVel = frontOffset;
    rotVel.Scale(angle);
    rot.FromRotation(rotVel);
    origin = ikPos;
    rot.Transform(origin,leftPos);
    leftPos += hipP;
    rage::Vector3 UpVector = m_character->getUpVector(); 
    if (m_goToCatchFallFlag)
    {
      //too disoriented will do a catchfall
      ikPos =gUp*-0.65f*maxLegLength;//bend his legs
      rage::Matrix34 pelvisMat ;
      m_spine->getPelvisPart()->getMatrix(pelvisMat);
      rage::Vector3 charDir = pelvisMat.c;
      charDir.Negate();
      m_character->levelVector(charDir);
      charDir.Normalize();
      ikPos.AddScaled(charDir,0.4f);
      rage::Vector3 sideDir = pelvisMat.b;
      m_character->levelVector(sideDir);
      sideDir.Normalize();
      ikPos.AddScaled(sideDir,0.2f);
      leftPos = ikPos;
      leftPos += hipP;
    }
    m_character->leftLegIK(leftPos, 0.0f, 0.0f, NULL,&hipVel);
    float legLength = lengthLegsPercentage(false);
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", leftPos);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", hipP);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.left.", legLength);
    m_character->bspyDrawPoint(leftPos, 0.05f, rage::Vector3(0,1,0));
#endif

    //right leg : add the sideOffset and do the IK
    angle = rage::AsinfSafe(m_parameters.m_sideD/ikPos.Mag());
    rotVel = frontOffset;
    rotVel.Scale(-angle);
    rot.FromRotation(rotVel);
    origin = rightPos;
    rot.Transform(origin,rightPos);
    rightPos += hipP;
    if (m_goToCatchFallFlag)
    {
      //too disoriented will do a catchfall
      rage::Matrix34 pelvisMat ;
      m_spine->getPelvisPart()->getMatrix(pelvisMat);
      rage::Vector3 sideDir = pelvisMat.b;
      m_character->levelVector(sideDir);
      sideDir.Normalize();
      ikPos.AddScaled(sideDir,-0.4f);
      rightPos = ikPos;
      rightPos += hipP;
    }
    m_character->rightLegIK(rightPos, 0.0f, 0.0f, NULL,&hipVel);
    legLength = lengthLegsPercentage(true);
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", rightPos);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right.", legLength);
    m_character->bspyDrawPoint(rightPos, 0.05f, rage::Vector3(1,1,0));
#endif


    //try to put his feet near the horizontal
    rage::Matrix34 rightFoot;
    m_rightLeg->getFoot()->getMatrix(rightFoot);
    rage::Matrix34 leftFoot ;
    m_leftLeg->getFoot()->getMatrix(leftFoot);
    rage::Vector3 rFoot = rightFoot.b;
    rFoot.Normalize();
    rage::Vector3 lFoot = leftFoot.b;
    lFoot.Normalize();
    float footRightAng = rFoot.Dot(UpVector);
    float footLeftAng = lFoot.Dot(UpVector);
    float frAng = rage::AcosfSafe(footRightAng);
    float flAng = rage::AcosfSafe(footLeftAng);

    float footAng = -0.03f*rage::Abs(m_forwardVel) + m_slopeGround;

    if ((frAng>footAng))
    {
      m_rightLeg->getAnkle()->setDesiredLean1(frAng-0.15f);
    }
    if ((flAng>footAng))
    {
      m_leftLeg->getAnkle()->setDesiredLean1(flAng-0.15f);
    }
#if ART_ENABLE_BSPY && LandingBSpyDraw
    m_character->bspyDrawLine(leftPos, leftPos+lFoot*0.3f, rage::Vector3(0,1,0));
    m_character->bspyDrawLine(rightPos, rightPos+rFoot*0.3f, rage::Vector3(1,1,0));
    bspyScratchpad(m_character->getBSpyID(), "duringPrepareForLanding.", frAng);
    bspyScratchpad(m_character->getBSpyID(), "duringPrepareForLanding.", flAng);
#endif
  }

  void NmRsCBULanding::duringPrepareForSideRoll(){
    rage::Vector3 upVector = m_character->getUpVector();

    /*********/
    /* Legs  */
    /*********/
    //try to put his feet under the COM with straight legs
    rage::Matrix34 comTM = m_character->m_COMTM;
    rage::Vector3 sideOffset =  comTM.GetVector(0);
    m_character->levelVector(sideOffset);
    sideOffset.Normalize();

    rage::Vector3 frontOffset =comTM.GetVector(2);
    frontOffset.Negate();
    m_character->levelVector(frontOffset);
    frontOffset.Normalize();

    // work out the average of the two hip positions
    rage::Vector3 hipPL = m_leftLeg->getHip()->getJointPosition();
    rage::Vector3 hipPR = m_rightLeg->getHip()->getJointPosition();
    rage::Vector3 hipP = hipPL + hipPR;
    hipP.Scale(0.5f);

    // move the hip pos to be under the COM
    rage::Vector3 hipPTemp = hipP;
    m_character->levelVector(hipPTemp);
    hipP = hipP - hipPTemp;
    hipPTemp = m_character->m_COM;
    m_character->levelVector(hipPTemp);
    hipP = hipP + hipPTemp;

    rage::Vector3 ikPos = hipP - upVector;
    rage::Vector3 vIkPHip = ikPos -hipP;
    vIkPHip.Normalize();
    vIkPHip.Scale(1.f);
    ikPos = hipP + vIkPHip;
    float sideValue = 0.35f;
    float sideD = 0.05f;
    //-- add the sideOffset and do the IK
    if (m_rightSideRoll)
    {
      ikPos.AddScaled(ikPos,sideOffset,-sideValue);
    }
    else
    {
      ikPos.AddScaled(ikPos,sideOffset,sideValue);
    }
    rage::Vector3 hipVel = m_spine->getPelvisPart()->getLinearVelocity();
    m_character->leftLegIK(ikPos, 0.0f, 0.0f, NULL,&hipVel);

#if ART_ENABLE_BSPY && LandingBSpyDraw
    m_character->bspyDrawPoint(ikPos, 0.05f, rage::Vector3(0,1,0));
#endif

    ikPos.AddScaled(ikPos,sideOffset,2.0f*sideD);
    m_character->rightLegIK(ikPos, 0.0f, 0.0f, NULL,&hipVel);

#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "SideRoll", m_rightSideRoll);
    bspyScratchpad(m_character->getBSpyID(), "SideRoll", sideD);
    m_character->bspyDrawPoint(ikPos, 0.05f, rage::Vector3(1,1,0));
#endif

    /********/
    /* Arms */
    /********/

    rage::Vector3 ShoulderL = m_leftArm->getShoulder()->getJointPosition();
    rage::Vector3 ShoulderR = m_rightArm->getShoulder()->getJointPosition();
    rage::Vector3 ShoulderRtoL = ShoulderL - ShoulderR;
    ShoulderRtoL.Normalize();
    rage::Vector3 frontward;
    frontward.Cross(ShoulderRtoL,upVector);
    m_character->levelVector(frontward);
    frontward.Normalize();
    rage::Vector3 rightIKPos;
    rage::Vector3 leftIKPos;
    float rightTwist = 0.2f;
    float leftTwist = 0.2f;
    if (!m_rightSideRoll)
    {
      rightIKPos = ShoulderR;
      rightIKPos.AddScaled(frontward,0.4f);
      rightIKPos.AddScaled(ShoulderRtoL,-0.2f);
      rightTwist = 0.5f;
      leftIKPos = ShoulderL;
      leftIKPos.AddScaled(ShoulderRtoL,0.5f);
    }
    else
    {
      leftIKPos = ShoulderL;
      leftIKPos.AddScaled(frontward,0.4f);
      leftIKPos.AddScaled(ShoulderRtoL,0.2f);
      leftTwist = 0.5f;
      rightIKPos = ShoulderR;
      rightIKPos.AddScaled(ShoulderRtoL,-0.5f);
    }
    m_character->rightArmIK(rightIKPos,rightTwist,0.0f, NULL, &hipVel);
    m_character->rightWristIK(rightIKPos,upVector);
    m_character->leftArmIK(leftIKPos,leftTwist,0.0f, NULL,&hipVel);
    m_character->leftWristIK(leftIKPos,upVector);
#if ART_ENABLE_BSPY && LandingBSpyDraw
    m_character->bspyDrawPoint(rightIKPos, 0.05f, rage::Vector3(1,1,0));
    m_character->bspyDrawPoint(leftIKPos, 0.05f, rage::Vector3(0,1,0)); 
#endif

    //to avoid hands oriented sideways
    setHands(0.3f);

  }



  // another option if we can't balance
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBULanding::duringCatchFall()
  {
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "landing - Catchfall", m_healthCatchFall);

#endif
    if (!catchFallTask->isActive())
    {
      catchFallTask->updateBehaviourMessage(NULL); // set parameters to defaults
      catchFallTask->m_parameters.m_legsStiffness = m_healthCatchFall*0.6f*m_parameters.m_bodyStiffness;
      catchFallTask->m_parameters.m_torsoStiffness = m_healthCatchFall*0.85f*m_parameters.m_bodyStiffness;
      catchFallTask->m_parameters.m_armsStiffness = m_healthCatchFall*1.45f*m_parameters.m_bodyStiffness;
      //cFParameters.m_forwardMaxArmOffset = .52f;
      catchFallTask->activate();
    }
    m_spine->keepHeadAwayFromGround(getSpineInput(), 2.0f);
    // if we've come to rest, fin
    if (!m_messageSent)
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback && m_character->m_COMvelRelativeMag < 0.075f)
      {
        m_messageSent = true;
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 0;
        strcpy(feedback->m_behaviourName, NMLandingFeedbackName);
        feedback->onBehaviourFailure();
      }
    }
  }
  // during balance
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBULanding::duringBalance()
  {
    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    Assert(bodyBalanceTask);
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    rage::Vector3 comVel = m_character->m_COMvel;
    m_character->levelVector(comVel);
    float ComVel =  comVel.Mag();
    float ComRotVel = m_character->m_COMrotvelMag;
    float TooMuchRotVel = (ComRotVel-4.5f);
    float TooMuchVel = (ComVel-2.5f);
    bool needTorqueFromRot = TooMuchRotVel>0.0f;
    bool needTorqueFromVel = TooMuchVel>0.0f;
    rage::Vector3 dirFall = m_lastPostFall - m_initPosFall;
    m_character->levelVector(dirFall);
    dirFall.Normalize();
    if (!bodyBalanceTask->isActive())
    {

      dynamicBalancerTask->taperKneeStrength(false);
      //m_character->m_footSlipMult = 0.5f;
      //tune params to body balance
      bodyBalanceTask->updateBehaviourMessage(NULL);// sets values to defaults
      m_character->enableFootSlipCompensationActive(true,true);
      rage::Vector3 toLookAndToGo = m_character->m_COM;
      rage::Vector3 upVector  = m_character->getUpVector();
      rage::Vector3 lFootP = m_leftLeg->getFoot()->getPosition();
      rage::Vector3 rFootP = m_rightLeg->getFoot()->getPosition();
      float LowestFoot = rage::Min(rFootP.Dot(upVector),lFootP.Dot(upVector));
      m_character->levelVector(toLookAndToGo,LowestFoot);
      toLookAndToGo.AddScaled(dirFall, 5.f);
#if ART_ENABLE_BSPY && LandingBSpyDraw
      m_character->bspyDrawPoint(toLookAndToGo,0.5f,rage::Vector3(1,1,1));
#endif
      bodyBalanceTask->m_parameters.m_useHeadLook =  false;
      bodyBalanceTask->m_parameters.m_headLookInstanceIndex = -1;
      bodyBalanceTask->m_parameters.m_headLookPos= toLookAndToGo;
      bodyBalanceTask->m_parameters.m_turnOffProb = 0.f; 
      bodyBalanceTask->m_parameters.m_turn2VelProb = 0.f;
      bodyBalanceTask->m_parameters.m_turnAwayProb = 0.f; 
      bodyBalanceTask->m_parameters.m_turnLeftProb = 0.f;
      bodyBalanceTask->m_parameters.m_turnRightProb = 0.f;
      bodyBalanceTask->m_parameters.m_turn2TargetProb = 1.0f;
      bodyBalanceTask->m_parameters.m_headLookAtVelProb = 0.0f;

      if (m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl)//Up the position, as 0 is at feet
#if ART_ENABLE_BSPY && LandingBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "//////////DuringBalance.RDRCharacters",0);
#endif
      bodyBalanceTask->m_parameters.m_headLookPos.y = 1.0f;
      bodyBalanceTask->m_parameters.m_headLookPos.z = -4.0f;
      bodyBalanceTask->m_parameters.m_headLookInstanceIndex = m_character->getFirstInstance()->GetLevelIndex();

      bodyBalanceTask->m_parameters.m_spineStiffness = 10.0f;
      bodyBalanceTask->m_parameters.m_spineDamping = 1.0f;
      bodyBalanceTask->m_parameters.m_useBodyTurn = m_character->getRandom().GetBool();
      bodyBalanceTask->m_parameters.m_somersaultAngleThreshold = 0.25f;//Amount of somersault 'angle' before bodyBalanceTask->m_parameters.m_somersaultAngle is used for ArmsOut
      bodyBalanceTask->m_parameters.m_sideSomersaultAngleThreshold = 0.25f;
      bodyBalanceTask->m_parameters.m_somersaultAngVelThreshold = 1.2f;
      bodyBalanceTask->m_parameters.m_twistAngVelThreshold = 3.0f;
      bodyBalanceTask->m_parameters.m_sideSomersaultAngVelThreshold = 1.2f;

      bodyBalanceTask->m_parameters.m_armStiffness = m_parameters.m_bodyStiffness*(9.0f/11.0f);
      bodyBalanceTask->m_parameters.m_armDamping = 0.7f;

      bodyBalanceTask->m_parameters.m_elbow = 1.2f; //How much the elbow swings based on the leg movement was 2 good but hit head if long step forward
      bodyBalanceTask->m_parameters.m_shoulder = 0.90f; //How much the shoulder swings based on the leg movement (lean1)

      bodyBalanceTask->m_parameters.m_somersaultAngle = 1.0f; //multiplier of the somersault 'angle' (lean forward/back) for arms out (lean2) 
      bodyBalanceTask->m_parameters.m_sideSomersaultAngle = 1.0f; //as above but for lean (side/side)

      bodyBalanceTask->m_parameters.m_somersaultAngVel = 2.5f;//multiplier of the somersault(lean forward/back) angular velocity  for arms out (lean2) 
      bodyBalanceTask->m_parameters.m_twistAngVel = 0.9f;//multiplier of the twist angular velocity  for arms out (lean2) //make this lower than 1 also thresh higher?
      bodyBalanceTask->m_parameters.m_sideSomersaultAngVel = 2.5f;//multiplier of the side somersault(lean side/side) angular velocity  for arms out (lean2) 

      bodyBalanceTask->m_parameters.m_armsOutOnPush = true; //Put arms out based on lean2 of legs, or angular velocity (lean or twist), or lean (front/back or side/side)
      bodyBalanceTask->m_parameters.m_armsOutOnPushMultiplier = 1.0f; //Arms out based on lean2 of the legs to simulate being pushed
      bodyBalanceTask->m_parameters.m_armsOutOnPushTimeout = 0.8f;//number of seconds before turning off the armsOutOnPush response only for Arms out based on lean2 of the legs (NOT for the angle or angular velocity) 

      bodyBalanceTask->m_parameters.m_returningToBalanceArmsOut = 0.1f;//range 0:1 0 = don't raise arms if returning to upright position, 0.x = 0.x*raise arms based on angvel and 'angle' settings, 1 = raise arms based on angvel and 'angle' settings 
      bodyBalanceTask->m_parameters.m_armsOutStraightenElbows = 0.0f;//multiplier for straightening the elbows based on the amount of arms out(lean2) 0 = dont straighten elbows. Otherwise straighten elbows proprtionately to armsOut
      bodyBalanceTask->m_parameters.m_armsOutMinLean2  = -10.0f;// stop the arms going too high (lean2)

      bodyBalanceTask->m_parameters.m_elbowAngleOnContact = 0.7f; //Minimum desired angle of elbow during contact(with upper body) arm swing
      bodyBalanceTask->m_parameters.m_bendElbowsTime = 0.0f; //Time after contact that the min bodyBalanceTask->m_parameters.m_elbowAngleOnContact is applied
      bodyBalanceTask->m_parameters.m_bendElbowsGait = 0.7f; //Minimum desired angle of elbow during non contact arm swing

      bodyBalanceTask->m_parameters.m_headLookAtVelProb = 0.9f; //Will look at vel 90% of time if stepping
      bodyBalanceTask->m_parameters.m_turnOffProb = 0.05f; //5% Probability that turn will be off. This is one of six turn type weights.
      bodyBalanceTask->m_parameters.m_turn2TargetProb = 0.05f; //5% Probability of turning towards headLook target. This is one of six turn type weights.
      bodyBalanceTask->m_parameters.m_turn2VelProb = 0.70f; //50% Probability of turning towards velocity. This is one of six turn type weights.
      bodyBalanceTask->m_parameters.m_turnAwayProb = 0.0f; //0% Probability of turning away from headLook target. This is one of six turn type weights.
      bodyBalanceTask->m_parameters.m_turnLeftProb = 0.10f; //10% Probability of turning left. This is one of six turn type weights.
      bodyBalanceTask->m_parameters.m_turnRightProb = 0.10f; //10% Probability of turning right. This is one of six turn type weights.

      bodyBalanceTask->activate();
      m_rollToBBtimer = 0.0f;
    }
    //if(m_rollToBBtimer<0.20f){//come back to a default value
    //  m_character->m_footSlipMult = 1.0f;
    //}
    if (m_rollToBBtimer>0.35f){//come back to default values
      m_character->setBodyStiffness(m_parameters.m_bodyStiffness, m_parameters.m_bodydamping,"fb");
      bodyBalanceTask->m_parameters.m_useHeadLook =  true;
    }
    //update headlook
    rage::Vector3 toLookAndToGo = m_character->m_COM;
    rage::Vector3 upVector  = m_character->getUpVector();
    rage::Vector3 lFootP = m_leftLeg->getFoot()->getPosition();
    rage::Vector3 rFootP = m_rightLeg->getFoot()->getPosition();
    float LowestFoot = rage::Min(rFootP.Dot(upVector),lFootP.Dot(upVector));
    m_character->levelVector(toLookAndToGo,LowestFoot);
    toLookAndToGo.AddScaled(dirFall, 5.f);
    bodyBalanceTask->m_parameters.m_headLookAtVelProb = 0.0f;
    bodyBalanceTask->m_parameters.m_headLookPos = toLookAndToGo;
    bodyBalanceTask->m_parameters.m_headLookInstanceIndex = -1;

    //Neck
    if (needTorqueFromVel&&needTorqueFromRot)
    {
      //TEST : to avoid head collision
      float strength = 18.f;
      float damping = 0.5f;
      m_character->setBodyStiffness(strength, damping,"un");
      //over joint limits
      m_spine->getLowerNeck()->setDesiredTwist(0.f);
      m_spine->getLowerNeck()->setDesiredLean1(0.47f);
      m_spine->getLowerNeck()->setDesiredLean2(0.0f);

      m_spine->getLowerNeck()->setDesiredTwist(0.f);
      m_spine->getUpperNeck()->setDesiredLean1(1.10f);
      m_spine->getUpperNeck()->setDesiredLean2(0.0f);
    }
    else if (m_rollToBBtimer<0.35f && m_parameters.m_forwardRoll){
      float strength = 18.f;
      float damping = 0.5f;
      m_character->setBodyStiffness(strength, damping,"un");
      //joint limits
      m_spine->getLowerNeck()->setDesiredTwist(0.f);
      m_spine->getLowerNeck()->setDesiredLean1(0.26f);
      m_spine->getLowerNeck()->setDesiredLean2(0.0f);

      m_spine->getLowerNeck()->setDesiredTwist(0.f);
      m_spine->getUpperNeck()->setDesiredLean1(0.6f);
      m_spine->getUpperNeck()->setDesiredLean2(0.0f);
    }

    //hands
    if (m_rollToBBtimer<0.35f){
      setHands(0.3f);
    }
    m_rollToBBtimer += m_character->getLastKnownUpdateStep();       

    if (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
    {
      dynamicBalancerTask->taperKneeStrength(false);
      float  hipPitch = rage::Clamp(-0.7f + m_rollToBBtimer*3, -0.5f, 0.0f);//float  hipPitch = rage::Clamp(-0.5f + m_rollToBBtimer, -0.5f, 0.0f);

      dynamicBalancerTask->setHipPitch(hipPitch);

      float legStr = rage::Clamp(m_parameters.m_legStrength,1.0f,16.0f);
      float kneeStr = rage::Clamp((legStr/12.0f)*(-1.0f+0.5f+(1.0f + m_rollToBBtimer)*(1.0f + m_rollToBBtimer)),0.2f,1.0f);
      NM_RS_DBG_LOGF(L"kneeStr = : %.5f", kneeStr);
      dynamicBalancerTask->setKneeStrength(kneeStr);
    }
    if (needTorqueFromRot&&needTorqueFromVel)
    {//try to reduce the angular velocity during landing by a cheating torque
      rage::Vector3 upVec = m_character->getUpVector();
      m_character->levelVector(comVel);
      rage::Vector3 torqueVec;
      torqueVec.Cross(comVel,upVec);
      //torqueVec.Negate();
      float torqMag = 0;
      if (needTorqueFromVel)
        torqMag = TooMuchVel*10;
      if (needTorqueFromRot)
        torqMag += TooMuchRotVel*10;
      torqueVec.Normalize();
      torqueVec.Scale(rage::Abs(torqMag));
      m_spine->getPelvisPart()->applyTorque(torqueVec);
      //if too much torque or vel, means will try to do a forward roll
      // need too help him to take a good shape by bending earlier the spine and the neck
      m_character->applySpineLean(1.4f,0.0f);
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "BalanceReduceAngularVel", torqMag);
#endif
    }

    //keep arms near the head at the beginning
    if(m_rollToBBtimer<0.20f && m_parameters.m_forwardRoll){//12 frames
      rage::Vector3 ikPos;
      ikPos=m_spine->getHeadPart()->getPosition();
      rage::Vector3 upVec = m_character->getUpVector();
      //add frontward side compare to shoulders
      rage::Vector3 frontward = m_character->m_COMvel;
      m_character->levelVector(frontward);
      if (frontward.Mag()<1.0f)
      {
        rage::Vector3 ShoulderL = m_leftArm->getShoulder()->getJointPosition();
        rage::Vector3 ShoulderRtoL = m_leftArm->getShoulder()->getJointPosition() - m_rightArm->getShoulder()->getJointPosition();
        frontward.Cross(ShoulderRtoL,upVec);
        m_character->levelVector(frontward);
      }
      frontward.Normalize();
      rage::Vector3 hSideVec;
      hSideVec.Cross(frontward,upVec);
      hSideVec.Normalize();
      ikPos.AddScaled(frontward,0.35f);
      ikPos.AddScaled(hSideVec,0.25f);
      rage::Vector3 Normal = m_character->getUpVector();
      if (!m_character->hasCollidedWithWorld("ur"))
      {
        callMaskedEffectorFunctionFloatArg(m_character, "ul", 0.5f, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
        m_character->rightArmIK(ikPos,0.0f,0.0f);
        m_character->rightWristIK(ikPos,Normal);
#if ART_ENABLE_BSPY
        m_character->bspyDrawPoint(ikPos, 0.05f, rage::Vector3(1,1,0));
        m_character->bspyDrawLine(ikPos, ikPos+Normal*0.1f, rage::Vector3(1,1,0));
#endif
        NmRs3DofEffector *effectorRwrist = (NmRs3DofEffector *) m_character->getEffector(m_rightArm->getWrist()->getJointIndex());
        effectorRwrist->setDesiredLean2Relative(-1.5f);
      }

      ikPos.AddScaled(hSideVec,-0.5f);
      if (!m_character->hasCollidedWithWorld("ul"))
      {
        callMaskedEffectorFunctionFloatArg(m_character, "ul", 0.5f, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
#if ART_ENABLE_BSPY && LandingBSpyDraw
        m_character->bspyDrawPoint(ikPos, 0.05f, rage::Vector3(0,1,0));
        m_character->bspyDrawLine(ikPos, ikPos+Normal*0.1f, rage::Vector3(0,1,0));
#endif
        m_character->leftArmIK(ikPos,0.0f,0.0f);
        m_character->leftWristIK(ikPos,Normal);
        NmRs3DofEffector *effectorLwrist = (NmRs3DofEffector *) m_character->getEffector(m_leftArm->getWrist()->getJointIndex());
        effectorLwrist->setDesiredLean2Relative(-1.5f);
      }
    }
    // send feedback
    if (!m_messageSent)
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback && m_character->m_COMvelRelativeMag < 0.15f)
      {
        m_messageSent = true;
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 0;
        strcpy(feedback->m_behaviourName, NMLandingFeedbackName);
        if (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK)
          feedback->onBehaviourFailure();
        else
          feedback->onBehaviourSuccess();
      }
    }
  }

  void NmRsCBULanding::duringBalanceStandUp(){
    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    Assert(bodyBalanceTask);
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    if (!bodyBalanceTask->isActive())
    {
      dynamicBalancerTask->updateBehaviourMessage(NULL);
      dynamicBalancerTask->taperKneeStrength(false);
      dynamicBalancerTask->setIgnoreFailure(false);
      dynamicBalancerTask->setStepWithBoth(false);
      m_character->enableFootSlipCompensationActive(true,true);
      //m_character->m_footSlipMult = 0.5f;
      //tune params to body balance
      bodyBalanceTask->updateBehaviourMessage(NULL);// sets values to defaults
      rage::Matrix34 pelvisMat ;
      m_spine->getPelvisPart()->getMatrix(pelvisMat);
      rage::Vector3 charDir = pelvisMat.c;
      charDir.Negate();
      m_character->levelVector(charDir);
      charDir.Normalize();
      rage::Vector3 toLookAndToGo = m_character->m_COM;
      rage::Vector3 upVector  = m_character->getUpVector();
      rage::Vector3 lFootP = m_leftLeg->getFoot()->getPosition();
      rage::Vector3 rFootP = m_rightLeg->getFoot()->getPosition();
      float LowestFoot = rage::Min(rFootP.Dot(upVector),lFootP.Dot(upVector));
      m_character->levelVector(toLookAndToGo,LowestFoot);
      toLookAndToGo.AddScaled(charDir, 5.f);
#if ART_ENABLE_BSPY && LandingBSpyDraw
      m_character->bspyDrawPoint(toLookAndToGo,0.5f,rage::Vector3(1,1,1));
#endif
      bodyBalanceTask->m_parameters.m_useHeadLook =  true;
      bodyBalanceTask->m_parameters.m_headLookInstanceIndex = -1;
      bodyBalanceTask->m_parameters.m_headLookPos= toLookAndToGo;
      bodyBalanceTask->m_parameters.m_headLookAtVelProb = 0.0f;
      bodyBalanceTask->m_parameters.m_turnOffProb = 0.f; 
      bodyBalanceTask->m_parameters.m_turn2VelProb = 0.f;
      bodyBalanceTask->m_parameters.m_turnAwayProb = 0.f; 
      bodyBalanceTask->m_parameters.m_turnLeftProb = 0.f;
      bodyBalanceTask->m_parameters.m_turnRightProb = 0.f;
      bodyBalanceTask->m_parameters.m_turn2TargetProb = 1.0f;

      bodyBalanceTask->activate();
      m_timerTryToStandUp=0;
    }

#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.BalanceStandUp", m_lastKneeStrength);
#endif
    float strengthKnee = rage::Clamp(rage::SqrtfSafe(m_lastKneeStrength),11.0f,16.0f);
    float damping = m_parameters.m_bodydamping;
    m_leftLeg->getKnee()->setStiffness(strengthKnee,damping);
    m_rightLeg->getKnee()->setStiffness(strengthKnee,damping);
    dynamicBalancerTask->setHipPitch(0.0f);
    if (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
    {
      dynamicBalancerTask->taperKneeStrength(false);
    }
    //the character is trying to stand up, if the hip doesn't goes up, stop to try and do a catch fall
    rage::Vector3 velCOM = m_character->m_COMvel;
    float vertVelCOM  = velCOM.Dot(m_character->getUpVector());
    if(vertVelCOM<0.03f){
      m_timerTryToStandUp++;
      m_lastCOMUp=0;
    }
    else
    {
      if (m_timerTryToStandUp>0)
      {
        if (m_lastCOMUp>1)//need to go up during more than 2 frame
        {m_timerTryToStandUp--;
        }
        m_lastCOMUp++;
      }
    }
    // send feedback
    if (!m_messageSent)
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback && m_character->m_COMvelRelativeMag < 0.15f)
      {
        m_messageSent = true;
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 0;
        strcpy(feedback->m_behaviourName, NMLandingFeedbackName);
        if (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK)
          feedback->onBehaviourFailure();
        else
          feedback->onBehaviourSuccess();
      }
    }


  }

  //other option if can t balance
  void NmRsCBULanding::duringForwardRoll()
  {
    //init : m_phase1FW = true, others one are false
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", m_phase1FW);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", m_phase2FW);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", m_phase3FW);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", m_phase1DoneFW);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", m_phase2DoneFW);
#endif

    //update timer
    m_rollToBBtimer += m_character->getLastKnownUpdateStep();

    /*****************************************************/
    /* 1-Apply torque to avoid rotation around ZAxis     */
    /*****************************************************/
    float zAxisSpinReduction = 0.5f;
    m_character->antiSpinAroundVerticalAxisJuice(zAxisSpinReduction);

    /***********/
    /* 2-SPINE */
    /***********/
    //return to default values if in balance has been modified
    float strength = 11.f;
    float damping = m_parameters.m_bodydamping;
    m_character->setBodyStiffness(strength, damping,"un");

    //to keep more the foetal position even if rotation is high
    float comrotVel = m_character->m_COMrotvelMag;
    float strengthSpine = strength+(comrotVel-3.0f);
    strengthSpine = rage::Clamp(strengthSpine,strength, 20.f);
    m_character->setBodyStiffness(strengthSpine, damping,"us");

    //bend spine
    bool spineCollided = m_spine->getSpine2Part()->collidedWithEnvironment()||m_spine->getSpine1Part()->collidedWithEnvironment()||m_spine->getSpine0Part()->collidedWithEnvironment();
    if(m_phase1FW||m_phase2FW){

      m_spine->getLowerNeck()->setDesiredLean2(0.0f);
      m_spine->getUpperNeck()->setDesiredLean2(0.0f);
      if (!m__spineAlreadyCollided)
      {
        //values to be at joint limits ASAP during phase 1
        float strength = 18.f;
        float damping = 0.5f;
        //will be overwritten if the head will collide the environment (line 1492)
        m_character->setBodyStiffness(strength, damping,"un");

        //value over joint limits to be at joint limits ASAP during phase 1
        m_spine->getLowerNeck()->setDesiredLean1(0.6f);
        m_spine->getUpperNeck()->setDesiredLean1(1.3f);
        m_spine->getSpine3()->setDesiredLean1(1.0f); 
        m_spine->getSpine2()->setDesiredLean1(1.5f);
        m_spine->getSpine1()->setDesiredLean1(1.5f);
        m_spine->getSpine0()->setDesiredLean1(1.5f);
      }
      else
      {
        //return to default values 
        m_character->setBodyStiffness(strengthSpine, damping,"un");

        m_spine->getLowerNeck()->setDesiredLean1(0.26f);
        m_spine->getUpperNeck()->setDesiredLean1(0.47f);
        m_spine->getSpine3()->setDesiredLean1(0.47427f); 
        m_spine->getSpine2()->setDesiredLean1(0.57552f);
        m_spine->getSpine1()->setDesiredLean1(0.523198f);
        m_spine->getSpine0()->setDesiredLean1(0.523198f);
      }
    }

    /************************************************/
    /* modify the position regarding the inclination*/
    /* of the spine on side of the most behind foot */
    /************************************************/
    if(m_phase1FW&&m_sideToRollisRight){
      //Spine : desired angle -> right side
      m_spine->getSpine3()->setDesiredTwistRelative(-0.1f); 
      m_spine->getSpine2()->setDesiredTwistRelative(-0.1f);
      m_spine->getSpine1()->setDesiredTwistRelative(-0.1f);
      m_spine->getSpine0()->setDesiredTwistRelative(-0.1f);
    }
    else if(m_phase1FW){
      //Spine : desired angle -> left twist
      m_spine->getSpine3()->setDesiredTwistRelative(0.1f);
      m_spine->getSpine2()->setDesiredTwistRelative(0.1f);
      m_spine->getSpine1()->setDesiredTwistRelative(0.1f);
      m_spine->getSpine0()->setDesiredTwistRelative(0.1f);
    }
    if(m_phase2DoneFW){//reset to the default twist value 
      m_spine->getSpine3()->setDesiredTwistRelative(0.f);
      m_spine->getSpine2()->setDesiredTwistRelative(0.f);
      m_spine->getSpine1()->setDesiredTwistRelative(0.f);
      m_spine->getSpine0()->setDesiredTwistRelative(0.f);
    }
    if ((m_phase3FW||spineCollided)){
      m_character->enableFootSlipCompensationActive(true, true);
    }


    /******************/
    /* ARMS :position */
    /******************/
    //determination Ikpos = just front of the head
    if(m_phase1FW){
      rage::Vector3 ikPos;
      ikPos=m_spine->getHeadPart()->getPosition();
      rage::Vector3 upVec = m_character->getUpVector();
      //add frontward side compare to shoulders
      rage::Vector3 ShoulderL = m_leftArm->getShoulder()->getJointPosition();
      rage::Vector3 ShoulderR = m_rightArm->getShoulder()->getJointPosition();
      rage::Vector3 ShoulderRtoL = ShoulderL - ShoulderR;
      rage::Vector3 hSideVec = ShoulderRtoL;
      rage::Vector3 frontward ;
      frontward.Cross(ShoulderRtoL,upVec);
      frontward.Normalize();
      hSideVec.Normalize();
      ikPos.AddScaled(frontward,0.15f);
      ikPos.AddScaled(hSideVec,-0.15f);

      //send a probe to identify height of the ground
      rage::Vector3 startOfProbe = m_rightArm->getHand()->getPosition() + m_leftArm->getHand()->getPosition();
      startOfProbe.Scale(0.5f);
      rage::Vector3 endOfProbe;
      endOfProbe.AddScaled(startOfProbe, upVec, 3);
      bool didItHit = m_character->probeRay(NmRsCharacter::pi_UseNonAsync, startOfProbe, endOfProbe, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);
      rage::Vector3 hitPos = m_character->m_probeHitPos[NmRsCharacter::pi_UseNonAsync];
      rage::Vector3 hitNormal = m_character->m_probeHitNormal[NmRsCharacter::pi_UseNonAsync];;

      if (didItHit)
      {
        ikPos.SetZ(hitPos.Dot(upVec)+0.05f);//try to reach the ground,0.05 to avoid to touch the ground during trajectory
      }
      else
      {
        ikPos.SetZ(m_spine->getHeadPart()->getPosition().Dot(upVec)-0.25f);
      }
      rage::Vector3 Normal = m_character->m_gUp;


      if ((!m_character->hasCollidedWithWorld("ur"))&&!m_alreadyCollidedR)
      {
        callMaskedEffectorFunctionFloatArg(m_character, "ul", 0.5f, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
        m_character->rightArmIK(ikPos,0.0f,0.0f);
        m_character->rightWristIK(ikPos,Normal);
#if ART_ENABLE_BSPY && LandingBSpyDraw
        m_character->bspyDrawPoint(ikPos, 0.05f, rage::Vector3(1,1,0));
        m_character->bspyDrawLine(ikPos, ikPos+Normal*0.1f, rage::Vector3(1,1,0));
#endif
        NmRs3DofEffector *effectorRwrist = (NmRs3DofEffector *) m_character->getEffector(m_rightArm->getWrist()->getJointIndex());
        effectorRwrist->setDesiredLean2Relative(-1.5f);
      }
      else
      {
        m_alreadyCollidedR = true;
        m_phase1DoneFW = true;
      }
      ikPos.AddScaled(hSideVec,0.3f);
      if ((!m_character->hasCollidedWithWorld("ul"))&&!m_alreadyCollidedL)
      {
        callMaskedEffectorFunctionFloatArg(m_character, "ul", 0.5f, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
#if ART_ENABLE_BSPY && LandingBSpyDraw
        m_character->bspyDrawPoint(ikPos, 0.05f, rage::Vector3(0,1,0));
        m_character->bspyDrawLine(ikPos, ikPos+Normal*0.1f, rage::Vector3(0,1,0));
#endif
        m_character->leftArmIK(ikPos,0.0f,0.0f);
        m_character->leftWristIK(ikPos,Normal);
        NmRs3DofEffector *effectorLwrist = (NmRs3DofEffector *) m_character->getEffector(m_leftArm->getWrist()->getJointIndex());
        effectorLwrist->setDesiredLean2Relative(-1.5f);
      }
      else
      {
        m_alreadyCollidedL = true;
        m_phase1DoneFW = true;
      }
      setHands(0.3f);
    }
    if(m_phase2FW){
      //try to put his hands near his knee 
      //set the wrist angle at the default value
      rage::Vector3 ikPos;
      if (!m_character->hasCollidedWithWorld("ul")){
        ikPos=m_leftLeg->getKnee()->getJointPosition();
        rage::Vector3 leftHand =m_leftArm->getHand()->getPosition();
        rage::Vector3 leftKneeToleftHand = leftHand - ikPos;
        ikPos.AddScaled(leftKneeToleftHand,0.2f);
        m_character->leftArmIK(ikPos,0.0f,0.0f);
#if ART_ENABLE_BSPY && LandingBSpyDraw
        m_character->bspyDrawPoint(ikPos, 0.05f, rage::Vector3(1,1,0));
        m_character->bspyDrawLine(ikPos, ikPos+m_character->getUpVector()*0.1f, rage::Vector3(1,1,0));
#endif
        NmRs3DofEffector *effectorLwrist = (NmRs3DofEffector *) m_character->getEffector(m_leftArm->getWrist()->getJointIndex());
        effectorLwrist->setDesiredLean2Relative(0.f);
      }
      if (!m_character->hasCollidedWithWorld("ur"))
      {
        ikPos=m_rightLeg->getKnee()->getJointPosition();
        rage::Vector3 rightHand =m_rightArm->getHand()->getPosition();
        rage::Vector3 rightKneeTorightHand = rightHand - ikPos;
        ikPos.AddScaled(rightKneeTorightHand,0.2f);
        m_character->rightArmIK(ikPos,0.0f,0.0f);
#if ART_ENABLE_BSPY && LandingBSpyDraw
        m_character->bspyDrawPoint(ikPos, 0.05f, rage::Vector3(0,1,0));
        m_character->bspyDrawLine(ikPos, ikPos+m_character->getUpVector()*0.1f, rage::Vector3(0,1,0));
#endif
        NmRs3DofEffector *effectorRwrist = (NmRs3DofEffector *) m_character->getEffector(m_leftArm->getWrist()->getJointIndex());
        effectorRwrist->setDesiredLean2Relative(0.f);
      }
    }
    /*********************************/
    /* ARMS : strength and stiffness */
    /*********************************/
    rage::Vector3 footPL = m_leftLeg->getFoot()->getPosition();
    rage::Vector3 footPR = m_rightLeg->getFoot()->getPosition();
    rage::Vector3 handPL = m_leftArm->getHand()->getPosition();
    rage::Vector3 handPR = m_rightArm->getHand()->getPosition();
    rage::Vector3 upVector = m_character->getUpVector();
    float LowestFoot = rage::Min(footPR.Dot(upVector),footPL.Dot(upVector));
    float LowestHand = rage::Min(handPR.Dot(upVector),handPL.Dot(upVector));
    float LowestPart = rage::Min(LowestHand,LowestFoot);
    float vertHeadPosition = m_spine->getHeadPart()->getPosition().Dot(upVector);
    vertHeadPosition -= LowestPart;
    rage::Vector3 headVel = m_spine->getHeadPart()->getLinearVelocity(); 
    float vertHeadVelocity = rage::Abs(headVel.Dot(upVector));

    if(m_phase1FW||m_phase2FW){
      float limit = 0.21f + vertHeadVelocity/30.0f;
      limit = limit- vertHeadPosition;
      float strengthua = 11.f;
      float dampingua = 2.f;
      if(limit>0){
        strengthua += limit*40;//if limit --> strength max to avoid collision with head (+0.4 by cm)

        if (strengthua>20.0f){
          strengthua=20.0f;
        }

        if((!m_character->hasCollidedWithWorld("ul"))||(!m_character->hasCollidedWithWorld("ur"))){
          strengthua = strengthua/2.f;
          strengthua = rage::Clamp(strengthua,11.f,20.f);
        }
      }
      if(m_character->hasCollidedWithWorld("ul"))
      {
        m_character->setBodyStiffness(strengthua, dampingua,"ul");
        if((vertHeadPosition+vertHeadVelocity/50.0f)>0.30){
          int index = m_leftArm->getElbow()->getJointIndex();
          NmRs1DofEffector *effectorElbow = (NmRs1DofEffector *) m_character->getEffector(index);
          float desired = effectorElbow->getDesiredAngle();
          desired=desired*0.7f;
          float clampDesired = rage::Clamp(desired,0.4f,1.5f);
          effectorElbow->setDesiredAngleRelative(clampDesired);
        }
      }
      if (m_character->hasCollidedWithWorld("ur"))
      {
        m_character->setBodyStiffness(strengthua, dampingua,"ur");
        if((vertHeadPosition+vertHeadVelocity/50.0f)>0.30){
          int index = m_rightArm->getElbow()->getJointIndex();
          NmRs1DofEffector *effector = (NmRs1DofEffector *) m_character->getEffector(index);
          float desired = effector->getDesiredAngle();
          desired=desired*0.7f;
          float clampDesired = rage::Clamp(desired,0.4f,1.5f);
          effector->setDesiredAngleRelative(clampDesired);
        }
      }
      float strength = m_parameters.m_bodyStiffness;//default muscles strength
      float damping = m_parameters.m_bodydamping;
      if(!m_character->hasCollidedWithWorld("ur")){
        //return to normal values
        m_character->setBodyStiffness(strength, damping,"ur");
      }
      if(!m_character->hasCollidedWithWorld("ul")){
        //return to normal values
        m_character->setBodyStiffness(strength, damping,"ul");
      }
      //if head will collide : modify strength of the neck
      float limitStrengthNeck = 0.15f + vertHeadVelocity/60.0f;
      limitStrengthNeck = limitStrengthNeck- vertHeadPosition;
      if (limitStrengthNeck>0.f ||m_spine->getHeadPart()->collidedWithEnvironment())
      {
        m_character->setBodyStiffness(m_parameters.m_bodyStiffness, m_parameters.m_bodydamping,"un");
      }
    }

    /******************/
    /* LEGS :position */
    /******************/

    //strength and damping
    float legStr = rage::Clamp(m_parameters.m_legStrength,1.0f,12.0f);
    strength = legStr;
    damping = m_parameters.m_bodydamping;//default muscles damping
    m_character->setBodyStiffness(strength,damping,"lb");

    if(m_phase1FW||m_phase2FW){
      //legs : keep values of High fall
      m_leftLeg->getHip()->setDesiredLean1(2.25f);
      m_rightLeg->getHip()->setDesiredLean1(2.25f);
      //reset to default
      m_leftLeg->getAnkle()->setDesiredLean1(0.0f);
      m_rightLeg->getAnkle()->setDesiredLean1(0.0f);

      if(m_phase2FW&&(!spineCollided)){
        m_leftLeg->getKnee()->setDesiredAngle(-3.0f);//over joint limits to be sure to be at joint limits
        m_rightLeg->getKnee()->setDesiredAngle(-3.0f);//over joint limits to be sure to be at joint limits
      }


      if(m_phase2FW||m_phase3FW||spineCollided){//try to move apart his feet
        if(!(m_leftLeg->getFoot()->collidedWithEnvironment())||m_phase2DoneFW){
          m_leftLeg->getHip()->setDesiredLean2(-0.3f);
          m_leftLeg->getHip()->setDesiredTwist(0.f);
        }
        if(!(m_rightLeg->getFoot()->collidedWithEnvironment())||m_phase2DoneFW){
          m_rightLeg->getHip()->setDesiredLean2(-0.3f);
          m_rightLeg->getHip()->setDesiredTwist(0.f);
        }
      }
    }

    /**********************/
    /* Push with legs   */
    /**********************/
    rage::Vector3 horComVel =m_character->m_COMvel;
    m_character->levelVector(horComVel);
    float horVel = horComVel.Mag();
    bool pushlegs  =m_phase1FW && horVel<2.5f;
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.BalanceStandUp", pushlegs);
#endif
    if(pushlegs){
      //-- need to push legs based on orientation
      rage::Vector3 theUpVec = m_character->m_gUp;
      rage::Matrix34 chCOMMAT = m_character->m_COMTM;
      rage::Vector3 characterUp = chCOMMAT.b;
      //-- should really project into the up/forward plane !!!!!!
      float upDotG = theUpVec.Dot(characterUp);
      m_leftLeg->getKnee()->setDesiredAngle(upDotG-1.65f); 
      m_rightLeg->getKnee()->setDesiredAngle(upDotG-1.65f); //was 1.75
    }
    else if (m_phase1FW){
      m_leftLeg->getKnee()->setDesiredAngleRelative(0.2f);//not push
      m_rightLeg->getKnee()->setDesiredAngleRelative(0.2f);
    }
    /***********************************/
    /* Apply cheating torque to pelvis */
    /***********************************/
    //modifying the direction of the roll : used to apply cheating torque to roll
    rage::Vector3 hipPL = m_leftLeg->getHip()->getJointPosition();
    rage::Vector3 hipPR = m_rightLeg->getHip()->getJointPosition();
    rage::Vector3 hipLToR = hipPR - hipPL;
    m_character->levelVector(hipLToR);
    hipLToR.Normalize();

    float velocity = m_character->m_COMvel.Mag();
    //angle between frontward and roll direction
    float angle = PI*0/180;
    //roll direction is based on the COM direction during falling
    rage::Vector3 dirCOM = m_character->m_COM - m_initPosFall;
    m_character->levelVector(dirCOM);
    rage::Vector3 dirRoll = dirCOM;
    if(m_sideToRollisRight){
      //add the right direction, proportionally to the velocity
      dirRoll += hipLToR*velocity*sin(angle);
      m_character->levelVector(dirRoll);
    }
    else
    {
      dirRoll -= hipLToR*velocity*sin(angle);
      m_character->levelVector(dirRoll);
    }

    rage::Vector3 comVel = m_character->m_COMvel;
    rage::Vector3 upVec = m_character->getUpVector();
    m_character->levelVector(comVel);

    float torqMag = 0.f;
    if(m_parameters.m_cheatingTorqueToForwardRoll!=0.0f){
      torqMag = 40.f*m_parameters.m_cheatingTorqueToForwardRoll*1.5f;//added 1.5f in order to have a more sensitive cheatingTorqueToForwardRoll.
      const rage::Vector3 ComRot = m_character->m_COMrotvel;
      rage::Matrix34 comTM = m_character->m_COMTM;
      rage::Vector3 sideOffset =  comTM.GetVector(0);
      sideOffset.Normalize();
      float COMRot = rage::Abs(ComRot.Dot(sideOffset));
      if (m_phase2FW||m_phase3FW)
      {
        if ((spineCollided||m__spineAlreadyCollided))
        {
          m__spineAlreadyCollided = true;
          if(COMRot<6.0f)
          {//if enough rotation at the end of forward, do not apply cheating torque, otherwise will fall often during stand up
            m_character->applyTorqueToRoll(m_parameters.m_stopFWCOMRoT,m_parameters.m_maxAngVelForForwardRoll,torqMag,dirRoll);
          }
        }
        else
        {
          m_character->applyTorqueToRoll(m_parameters.m_stopFWCOMRoT,m_parameters.m_maxAngVelForForwardRoll,torqMag,dirRoll);
        }
      }
      else if(m_phase1FW)
      {
        //clamp value to 10 to avoid head collision
        torqMag = rage::Clamp(torqMag,0.f,10.f);     
        m_character->applyTorqueToRoll(m_parameters.m_stopFWCOMRoT,m_parameters.m_maxAngVelForForwardRoll,torqMag,dirRoll);
      }
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "ForwardRollcheatingTorque.",m_rollDownTimer);
#if HACK_GTA4 // Integrating NM code drop (22/7/09)

#else //HACK_GTA4
      bspyScratchpad(m_character->getBSpyID(), "ForwardRollcheatingTorque.",torqMag);
#endif //HACK_GTA4
#endif
    }
  }

  void NmRsCBULanding::duringStandUp()
  {
    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    Assert(bodyBalanceTask);
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    if (!bodyBalanceTask->isActive())
    {
      //tune params to body balance
      bodyBalanceTask->updateBehaviourMessage(NULL);// sets values to defaults
      rage::Matrix34 pelvisMat ;
      m_spine->getPelvisPart()->getMatrix(pelvisMat);
      rage::Vector3 charDir = pelvisMat.c;
      charDir.Negate();
      m_character->levelVector(charDir);
      charDir.Normalize();
      rage::Vector3 toLookAndToGo = m_character->m_COM;
      rage::Vector3 upVector  = m_character->getUpVector();
      rage::Vector3 lFootP = m_leftLeg->getFoot()->getPosition();
      rage::Vector3 rFootP = m_rightLeg->getFoot()->getPosition();
      float LowestFoot = rage::Min(rFootP.Dot(upVector),lFootP.Dot(upVector));
      m_character->levelVector(toLookAndToGo,LowestFoot);
      toLookAndToGo.AddScaled(charDir, 5.f);
#if ART_ENABLE_BSPY && LandingBSpyDraw
      m_character->bspyDrawPoint(toLookAndToGo,0.5f,rage::Vector3(1,1,1));
#endif
      bodyBalanceTask->m_parameters.m_useHeadLook =  true;
      bodyBalanceTask->m_parameters.m_headLookInstanceIndex = -1;
      bodyBalanceTask->m_parameters.m_headLookPos= toLookAndToGo;
      bodyBalanceTask->m_parameters.m_turnOffProb = 0.f; 
      bodyBalanceTask->m_parameters.m_turn2VelProb = 0.f;
      bodyBalanceTask->m_parameters.m_turnAwayProb = 0.f; 
      bodyBalanceTask->m_parameters.m_turnLeftProb = 0.f;
      bodyBalanceTask->m_parameters.m_turnRightProb = 0.f;
      bodyBalanceTask->m_parameters.m_turn2TargetProb = 1.0f;
      bodyBalanceTask->m_parameters.m_headLookAtVelProb = 0.0f;
      bodyBalanceTask->activate();
    } 

    if(!dynamicBalancerTask->isActive()){
      dynamicBalancerTask->taperKneeStrength(false);
      dynamicBalancerTask->setIgnoreFailure(true);
      dynamicBalancerTask->setStepWithBoth(true);
      m_character->enableFootSlipCompensationActive(false);
      dynamicBalancerTask->activate();
      m_StandUpTimer = 0.f;
    }

    m_StandUpTimer += m_character->getLastKnownUpdateStep();
    rage::Vector3 UpVector =  m_character->getUpVector();
    float HeightCOM = m_character->m_COM.Dot(UpVector);

    if(m_StandUpTimer>0.30f||(HeightCOM>0.7f&&m_StandUpTimer>0.2f) && !alreadyForcedToInitialize){
      dynamicBalancerTask->setStepWithBoth(false);
      m_character->enableFootSlipCompensationActive(true, true);
      //m_character->m_footSlipMult = 0.5f;
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "landing - footsliptrue", m_StandUpTimer);
#endif
      alreadyForcedToInitialize = true;
    }
    float strengthKnee = m_parameters.m_strengthKneeToStandUp;
    float RotVel = m_character->m_COMrotvelMag;
    //value by default
    float F = 1.5f;
    // if rotvel is high, the strength increases fast to avoid falling during stand up 
    if(RotVel>3.5f){
      F -=(RotVel-3.5f)/2.0f;
      F = rage::Clamp(F,1.f,2.5f);
    }
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", strengthKnee);
#endif
    strengthKnee = rage::Clamp(strengthKnee+(m_StandUpTimer/F),0.f,2.f);
    //to reproduce the taperkneestrength but with less variations of strength
    float angleLeft = m_leftLeg->getKnee()->getActualAngle();
    float angleRight = m_rightLeg->getKnee()->getActualAngle();
    float valueLeft = rage::Clamp((1.f + 0.65f*(1.036f+angleLeft)/(PI - 1.036f)), 0.3f, 1.f);//if knee with minlean1 = (1+0.75*(-1.6))/2.5
    float valueRight = rage::Clamp((1.f + 0.65f*(1.036f+angleRight)/(PI - 1.036f)), 0.3f, 1.f);
    float strengthLeft = 100.f*strengthKnee * valueLeft;//100 default strength for knees
    float strengthRight = 100.f*strengthKnee * valueRight;

    //if stand up too fast reduce the strength too avoid a jump
    float vertVelCOM  = m_character->m_COMvel.Dot(m_character->getUpVector());
    if (vertVelCOM>1.5f)
    {
      strengthLeft /= 1.5f; 
      strengthRight /= 1.5f;
    }
    m_lastKneeStrength = rage::Max(strengthLeft,strengthRight);
    float damping = m_parameters.m_bodydamping;
    m_leftLeg->getKnee()->setStiffness(rage::Clamp(rage::SqrtfSafe(strengthLeft),6.0f,20.0f),damping);
    m_rightLeg->getKnee()->setStiffness(rage::Clamp(rage::SqrtfSafe(strengthRight),6.0f,20.0f),damping);
    //init of hippitch : angle hips   
    float actualAngleLeft = m_leftLeg->getHip()->getActualLean1();
    float actualAngleRight = m_rightLeg->getHip()->getActualLean1();
    float actualAngle = rage::Min(actualAngleLeft, actualAngleRight);
    float hipPitch  = rage::Clamp(-actualAngle+ 0.2f + m_StandUpTimer*2.0f,-1.57f,0.0f);
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", strengthKnee);
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", F);
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", RotVel);   
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", angleLeft);
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", angleRight);
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", valueRight);
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", valueLeft);
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", strengthRight);
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", strengthLeft);
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", hipPitch);
#endif
    dynamicBalancerTask->setHipPitch(hipPitch);
    //the character is trying to stand up, if the hip doesn't goes up, stop to try and do a catch fall
    rage::Vector3 velCOM = m_character->m_COMvel;
    vertVelCOM = velCOM.Dot(m_character->getUpVector());
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "landing - standUp", vertVelCOM);
#endif
    if(vertVelCOM<0.05f)
    {
      m_timerTryToStandUp++;
      m_lastCOMUp = 0;
    }
    else
    {
      if (m_timerTryToStandUp>0)
      {
        if (m_lastCOMUp>1)//need to go up during more than 2 frame
        {
          m_timerTryToStandUp--;
        }
        m_lastCOMUp++;
      }
    }
  }
  //parachutist behavior --> work in progress
  void NmRsCBULanding::duringSideRoll(){
    rage::Vector3 footPL = m_leftLeg->getFoot()->getPosition();
    rage::Vector3 footPR = m_rightLeg->getFoot()->getPosition();
    rage::Vector3 feet= footPR + footPL;
    feet.Scale(0.5f);
    rage::Vector3 feetToCOM = feet - m_character->m_COM;
    m_character->levelVector(feetToCOM);
    if (feetToCOM.Mag()<0.5f)
    {
      rage::Vector3 force = m_character->m_COMTM.GetVector(0);
      m_character->levelVector(force);
      force.Normalize();
      float amount = 1.5f/6.f;
      const SpineSetup* spine = m_character->getSpineSetup();
      if (m_rightSideRoll&&spine)
      {  
        nmrsSetTwist(spine->getSpine0(), amount * 2.0f);
        nmrsSetTwist(spine->getSpine1(), amount * 2.0f);
        nmrsSetTwist(spine->getSpine2(), amount);
        nmrsSetTwist(spine->getSpine3(), amount);
      }
      else
      {
        nmrsSetTwist(spine->getSpine0(), -amount * 2.0f);
        nmrsSetTwist(spine->getSpine1(), -amount * 2.0f);
        nmrsSetTwist(spine->getSpine2(), -amount);
        nmrsSetTwist(spine->getSpine3(), -amount);
      }
    }
    else
    {
      duringCatchFall();
    }
  }
  // windmill, pedalling etc...
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBULanding::duringFall()
  {
    callMaskedEffectorFunctionFloatArg(m_character, "fb", 0.5f, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);   
    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
    Assert(pedalTask);
    NmRsCBUArmsWindmill* armsWindmillTask = (NmRsCBUArmsWindmill*)m_cbuParent->m_tasks[bvid_armsWindmill];
    Assert(armsWindmillTask);
    //initialization of the initial position of the fall ; used to know horizontal direction in forward roll
    if(m_firstFrame){
      m_initPosFall = m_character->m_COM;m_firstFrame = false;
    }

    // turn on the pedal, headlook and armwindmill if not already on
    if (! headLookTask->isActive() )
    {
      headLookTask->updateBehaviourMessage(NULL);
      headLookTask->activate();
      headLookTask->m_parameters.m_alwaysLook = false;
      headLookTask->m_parameters.m_instanceIndex = (-1);
      headLookTask->m_parameters.m_alwaysEyesHorizontal = true;

    }

    if (! pedalTask->isActive() )
    {
      pedalTask->updateBehaviourMessage(NULL); // reset params
      pedalTask->m_parameters.randomSeed=(100);
      pedalTask->m_parameters.pedalOffset=(0.1f);
      pedalTask->m_parameters.pedalLeftLeg=(true);
      pedalTask->m_parameters.pedalRightLeg=(true);
      pedalTask->m_parameters.speedAsymmetry=(4.0f);
      pedalTask->m_parameters.radius=(m_parameters.m_legRadius);
      pedalTask->m_parameters.angularSpeed=(m_parameters.m_legAngSpeed);
      pedalTask->m_parameters.backPedal=(false);
      pedalTask->m_parameters.legStiffness=(m_parameters.m_bodyStiffness);
      pedalTask->m_parameters.adaptivePedal4Dragging=(false);
      pedalTask->activate();
    }

    if (! armsWindmillTask->isActive() )
    {
      armsWindmillTask->updateBehaviourMessage(NULL); // initialise parameters.
      armsWindmillTask->activate();
    }

    // get headLook position
    rage::Matrix34 comTM =  m_character->m_COMTM;
    rage::Vector3 currentForwardVec = comTM.c;
    currentForwardVec.Negate();
    m_character->levelVector(currentForwardVec);
    rage::Vector3 comPos = m_character->m_COM;
    rage::Vector3 gUp = m_character->m_gUp;
    currentForwardVec = comPos + currentForwardVec - gUp;
    // update headLook
    m_character->applySpineLean(0.0f,0.0f);
    headLookTask->m_parameters.m_pos = (currentForwardVec);
    headLookTask->m_parameters.m_instanceIndex = (-1);
    headLookTask->m_parameters.m_stiffness = (m_parameters.m_bodyStiffness);
    headLookTask->m_parameters.m_damping = (m_parameters.m_bodydamping);
    headLookTask->m_parameters.m_vel = m_character->m_COMvel;

    // update the armsWindMillTask
    armsWindmillTask->m_parameters.m_leftCircleDesc.radius1 = 0.55f;
    armsWindmillTask->m_parameters.m_leftCircleDesc.radius2 = 0.35f;
    armsWindmillTask->m_parameters.m_leftCircleDesc.speed = 1.0f;
    armsWindmillTask->m_parameters.m_rightCircleDesc.radius1 = 0.55f;
    armsWindmillTask->m_parameters.m_rightCircleDesc.radius2 = 0.35f;
    armsWindmillTask->m_parameters.m_rightCircleDesc.speed = 1.0f;
    armsWindmillTask->m_parameters.m_phaseOffset = 180.0f;
    armsWindmillTask->m_parameters.m_useLeft = true;
    armsWindmillTask->m_parameters.m_useRight = true;
    armsWindmillTask->m_parameters.m_forceSync = true ;
    rage::Vector3 leftNormal (0.2f,0.0f,0.0f);
    armsWindmillTask->m_parameters.m_leftCircleDesc.normal = leftNormal;
    rage::Vector3 leftCentre (0.0f,0.2f,-0.4f);
    armsWindmillTask->m_parameters.m_leftCircleDesc.centre = leftCentre;
    rage::Vector3 rightNormal (0.0f,0.2f,0.2f);
    armsWindmillTask->m_parameters.m_rightCircleDesc.normal = leftNormal;
    rage::Vector3 rightCentre (0.0f,0.5f,-0.1f);
    armsWindmillTask->m_parameters.m_rightCircleDesc.centre = leftCentre;
    armsWindmillTask->m_parameters.m_leftCircleDesc.partID = m_spine->getSpine3()->getJointIndex();
    armsWindmillTask->m_parameters.m_rightCircleDesc.partID = m_spine->getSpine3()->getJointIndex();
    armsWindmillTask->m_parameters.m_dragReduction = 0.2f;
    armsWindmillTask->m_parameters.m_IKtwist = -0.4f;
    armsWindmillTask->m_parameters.m_angVelThreshold = 0.1f;
    armsWindmillTask->m_parameters.m_angVelGain = 1.0f;
    armsWindmillTask->m_parameters.m_mirrorMode = 1;
    armsWindmillTask->m_parameters.m_adaptiveMode = 0;
    armsWindmillTask->m_parameters.m_shoulderStiffness = m_parameters.m_bodyStiffness*14.0f/11.0f;
    armsWindmillTask->m_parameters.m_elbowStiffness = m_parameters.m_bodyStiffness*14.0f/11.0f;

  }
  int NmRsCBULanding::decidePhaseBalance(){
    /*********************************************************/
    /*  TESTS FOR KEEP BALANCE OR FORWARD ROLL OR CATCH FALL  */
    /*********************************************************/
    /*********************************************/
    /*difference between feet and COM in position*/
    /*********************************************/
    rage::Vector3 hipPL = m_leftLeg->getHip()->getJointPosition();
    rage::Vector3 hipPR = m_rightLeg->getHip()->getJointPosition();
    rage::Vector3 HipLToR = hipPR - hipPL;
    //HipLToR = Backward
    rage::Vector3 UpVector = m_character->getUpVector();
    HipLToR.Cross(UpVector);
    HipLToR.Normalize();
    //HipLToR =  frontward
    HipLToR.Negate();
    /*********************************/
    /* Side of the forward roll */
    /*********************************/
    // depends of : 1.angle between vel/feet, 2.inclination of the spine 3. most behind foot
    //difference between orientation of the feet and COMVel
    rage::Vector3 footPL = m_leftLeg->getFoot()->getPosition();
    rage::Vector3 footPR = m_rightLeg->getFoot()->getPosition();
    rage::Vector3 feetLtoR = footPR-footPL;
    rage::Vector3 COMP = m_character->m_COM;
    rage::Vector3 COMVel = m_character->m_COMvel;
    float velToFeet = COMVel.Dot(feetLtoR);
    float angleOfVel = PI*45/180;
    float cosAngleVel = rage::Cosf(angleOfVel);
    /*************************/
    /* mostBehindFootisRight */
    /*************************/
    bool mostBehindFootisRight = true;
    if(feetLtoR.Dot(HipLToR)>0.f){
      mostBehindFootisRight= false;
    }
    //which side is curved the spine
    rage::Vector3 middleHips = (hipPR + hipPL);
    middleHips.Scale(0.5f);
    rage::Vector3 NeckPosition = m_spine->getNeckPart()->getPosition();
    rage::Vector3 spineDir = NeckPosition - middleHips;
    spineDir.Normalize();
    rage::Vector3 LToR = hipPR - hipPL;
    LToR.Normalize();
    //angle between ground and spine to consider it determinant for the choice of side
    float spineDirf = spineDir.Dot(LToR);
    float angleOfSpine = PI*75/180;
    float cosAngle = rage::Cosf(angleOfSpine);
    if(velToFeet>cosAngleVel){
      m_sideToRollisRight = true;
    }
    else {
      if(velToFeet<(-cosAngleVel)){
        m_sideToRollisRight = false;
      }
      else
      {
        if(spineDirf>cosAngle)
        {
          m_sideToRollisRight = true;
        }
        else 
        {
          if(spineDirf<(-cosAngle))
          {
            m_sideToRollisRight = false;
          }
          else
          {
            m_sideToRollisRight = mostBehindFootisRight;
          }
        }
      }
    }
    /******************************************************/
    /*distance between feet and COM in the parasagital plane */
    /******************************************************/
    rage::Vector3 feetP = footPL + footPR;
    feetP.Scale(0.5f);
    rage::Vector3 feetToCOM = COMP-feetP;
    float feetBehind = feetToCOM.Dot(HipLToR);
    float frontVel = COMVel.Dot(HipLToR);
    //angle between femur and trunk (max between left and right leg)
    rage::Vector3 spine0 = m_spine->getSpine0()->getJointPosition();
    rage::Vector3 spine3 = m_spine->getSpine3()->getJointPosition();
    rage::Vector3 spineOrientation = spine3 - spine0;
    spineOrientation.Normalize();
    rage::Vector3 KneeL = m_leftLeg->getKnee()->getJointPosition();
    rage::Vector3 KneeR = m_rightLeg->getKnee()->getJointPosition();
    rage::Vector3 HipL = m_leftLeg->getHip()->getJointPosition();
    rage::Vector3 HipR = m_rightLeg->getHip()->getJointPosition();
    rage::Vector3 legR = KneeR - HipR;
    legR.Normalize();
    rage::Vector3 legL = KneeL - HipL;
    legL.Normalize();
    float AngleR = spineOrientation.Dot(legR);
    float AngleL = spineOrientation.Dot(legL);
    float AngleHips = rage::Min(AngleR,AngleL);
    float LowestFoot = rage::Min(footPR.Dot(UpVector),footPL.Dot(UpVector));
    float heightHips = middleHips.Dot(UpVector)-LowestFoot;
    rage::Vector3 head = m_spine->getHeadPart()->getPosition();
    float verticalInclination = rage::Abs(spineOrientation.Dot(m_character->getUpVector()));
    float heightHead = head.Dot(m_character->getUpVector());
    heightHead = heightHead - LowestFoot;
    float COMHeight = COMP.Dot(UpVector);
    COMHeight= COMHeight - LowestFoot;
    bool testUnbalanceForward = feetBehind>(0.20*COMHeight);//proportionally of the height of the COM
    bool testUnbalanceForwardToFall = feetBehind>(0.5*COMHeight);//proportionally of the height of the COM;
    float legLengthBW = lengthLegsPercentage();
    bool StandUp = legLengthBW> 0.7f;

    const rage::Vector3 ComRot = m_character->m_COMrotvel;
    rage::Matrix34 comTM = m_character->m_COMTM;
    rage::Vector3 sideOffset =  comTM.GetVector(0);
    sideOffset.Normalize();
    float COMRot = rage::Abs(ComRot.Dot(sideOffset));
    bool testUnBalanceBackward = ((feetBehind+COMRot*0.03)<(-0.20*COMHeight))&&(!StandUp);//proportionally of the height of the COM
    //angle between spine and upvector 
    float angleVertical = PI*45/180;
    float cosAngleVert = rage::Cosf(angleVertical);
    bool testVI = verticalInclination<cosAngleVert;
    //angle between spine and hips 
    float angleHip = PI*100/180;//-0.2
    float cosAngleHip = rage::Cosf(angleHip);
    bool testAH = AngleHips>cosAngleHip;
    //if m_cheatingTorqueToForwardRoll == 1 (max value)--> don't need velocity, the cheating torque will do the work alone
    float cheatVel = m_parameters.m_cheatingTorqueToForwardRoll*0.65f;
    bool testVel = frontVel+cheatVel>1.3;
    bool testHHips = heightHips>0.45;
    bool testHHead = heightHead>0.35;
    bool testOneFootCollidedWithGround = m_leftLeg->getFoot()->collidedWithEnvironment()||m_rightLeg->getFoot()->collidedWithEnvironment();
    float leftAngle = m_leftLeg->getKnee()->getActualAngle();
    float rightAngle = m_rightLeg->getKnee()->getActualAngle();
    //120 degrees
    bool isCrashing = (rage::Min(leftAngle,rightAngle)<-2.4f)&&heightHips<0.4f;
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance",testUnBalanceBackward);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance",testUnbalanceForward);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance",testOneFootCollidedWithGround);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance",testUnbalanceForwardToFall);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance",testVel);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance.testvel", frontVel);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance.testvel", cheatVel);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance",testVI);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance.testVI", cosAngleVert);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance.testVI", verticalInclination);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance",testHHips);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance.testHHips", heightHips);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance",testHHead);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance.testHHead", heightHead);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance",testAH);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance.testAH", AngleHips);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance.testAH", cosAngleHip);
    bspyScratchpad(m_character->getBSpyID(), "landing - Balance",isCrashing);
#endif
    if(isCrashing){
      return 1;
    }
    if(testUnbalanceForward&&m_parameters.m_forwardRoll)
    {
      if(testOneFootCollidedWithGround&&testVI&&testAH&&testVel&&(testHHips||testHHead))
      {
        //Forward Roll
        return 2;
      }
      else
      {
        if(testUnbalanceForwardToFall)
        {
          //Catch Fall weak
          if (frontVel>3.5f)
          {
            return 3;
          }
          else
          {
            //Catch Fall healthy
            return 1;
          }
        }
      }
    }
    else if (testUnBalanceBackward)
    {
      //Catch Fall healthy
      return 1;
    }
    return 0;
  }
  int NmRsCBULanding::decidePhaseFW()
  { 
    //init : m_phase1FW = true, others one are false

    //at any time, if there is not enough rotation --> stop the forward roll and do a catchfall
    const rage::Vector3 ComRot = m_character->m_COMrotvel;
    rage::Matrix34 comTM = m_character->m_COMTM;
    rage::Vector3 sideOffset =  comTM.GetVector(0);
    sideOffset.Normalize();
    float COMRot = rage::Abs(ComRot.Dot(sideOffset));
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRoll", sideOffset);
    bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRoll", ComRot);
    bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRoll.comparedToStopFWCOMRoT", COMRot);
#endif
    bool stopFWCOMRoT = COMRot<m_parameters.m_stopFWCOMRoT/*||(m_phase2DoneFW&&COMRot<0.4f)*/;
    if(stopFWCOMRoT){
      m_phase1FW = false;
      m_phase2FW = false;
      m_phase1DoneFW = false;
      m_phase2DoneFW = false;
      m_phase3FW = true;
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "landing - ForwardRoll////CatchFall",stopFWCOMRoT);
#endif
      return 3;//catch fall with weakness
    }
    // test phase 2 
    rage::Vector3 shoulderPL = m_leftArm->getShoulder()->getJointPosition();
    rage::Vector3 shoulderPR = m_rightArm->getShoulder()->getJointPosition();
    rage::Vector3 handPL = m_leftArm->getWrist()->getJointPosition();
    rage::Vector3 handPR = m_rightArm->getWrist()->getJointPosition();
    rage::Vector3 armROrientation = shoulderPR - handPR;
    rage::Vector3 armLOrientation = shoulderPL - handPL;
    armLOrientation.Normalize();
    armROrientation.Normalize();
    rage::Vector3 frontvel = m_character->m_COMvel;
    m_character->levelVector(frontvel);
    frontvel.Normalize();
    float armLfrontward = armLOrientation.Dot(frontvel);
    float armRfrontward = armROrientation.Dot(frontvel);
    if(m_phase3FW||m_phase2FW||(m_phase1DoneFW&&armLfrontward>0&&armRfrontward>0)){
      if(!m_phase3FW){
        m_phase1FW = false;
        m_phase2FW = true;
        m_phase3FW = false;
        //during phase 2 : if no feet is collided with the world --> we can test phase 3
        bool LeftFootCollided = m_leftLeg->getFoot()->collidedWithEnvironment();
        bool RightFootCollided = m_rightLeg->getFoot()->collidedWithEnvironment();
        if((!LeftFootCollided)&&(!RightFootCollided)){
          m_phase2DoneFW = true;
        }
      }
      //test phase 3 : 
      //the end of the roll --> if not enough if rotation velocity&&spine is near vertical
      float COMRotVel = m_character->m_COMrotvelMag;
      //direction of the spine
      rage::Vector3 hipPL = m_leftLeg->getHip()->getJointPosition();
      rage::Vector3 hipPR = m_rightLeg->getHip()->getJointPosition();
      rage::Vector3 middleHips = (hipPR + hipPL);
      middleHips.Scale(0.5f);
      rage::Vector3 NeckPosition = m_spine->getNeckPart()->getPosition();
      rage::Vector3 spineDir = NeckPosition - middleHips;
      spineDir.Normalize();
      float vertSpine = spineDir.Dot(m_character->getUpVector());
      float vertAngle = PI*30/180;//angle between spine and horizontal
      float limitAngle = sin(vertAngle);
      bool nearVertical = vertSpine>limitAngle;
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollNearVertical", COMRotVel);
      bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollNearVertical", vertSpine);
      bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollNearVertical", limitAngle);
      bspyScratchpad(m_character->getBSpyID(), "landing - ForwardRoll",nearVertical);
#endif
      bool stopEndFWCOMRoT = COMRotVel<m_parameters.m_stopEndFWCOMRoT&&nearVertical;
      if(stopEndFWCOMRoT){
        m_phase1FW = false;
        m_phase2FW = false;
        m_phase3FW = true;
#if ART_ENABLE_BSPY && LandingBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "landing - ForwardRoll////CatchFall",stopEndFWCOMRoT);
#endif
        return 3;//catchFall with weakness
      }
      //if  it s a side roll meaning push  in a direction really different from dirFall
      rage::Vector3 dirFall =  m_lastPostFall - m_initPosFall;
      m_character->levelVector(dirFall);
      dirFall.Normalize();
      m_character->levelVector(spineDir);
      rage::Matrix34 pelvisMat ;
      m_spine->getPelvisPart()->getMatrix(pelvisMat);
      m_character->levelVector(spineDir);
      if(spineDir.Mag()<0.4f){//horizontal part sufficient otherwise take the pelvis
        spineDir = pelvisMat.c;
        m_character->levelVector(spineDir);
#if ART_ENABLE_BSPY && LandingBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollSideRollUsepelvis", spineDir);
#endif
      }
      spineDir.Normalize();
      float horAngle = PI*70/180;//angle between spine and dirFall
      float limitHorAngle = cos(horAngle);
      float horSpine = spineDir.Dot(dirFall);
      bool sideRoll = rage::Abs(horSpine)<limitHorAngle;
      float latOrientation = pelvisMat.b.Dot(m_character->getUpVector());
      float latAngle = PI*60/180;//angle with horizontal
      float sinLatAngle = sin(latAngle);
      bool latRoll = rage::Abs(latOrientation)>sinLatAngle;
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollSideRoll", dirFall);
      bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollSideRoll", spineDir);
      bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollSideRoll", limitHorAngle);
      bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollSideRoll", horSpine);
      bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollSideRoll",sideRoll);
      bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollLatRollPelvis", latOrientation);
      bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollLatRoll", sinLatAngle);
      bspyScratchpad(m_character->getBSpyID(), "landing - StopForwardRollLatRoll",latRoll);
#endif
      bool latRollOrSideRollSlow = latRoll||(COMRotVel<3.0f&&sideRoll);
      if(latRollOrSideRollSlow){
        m_phase1FW = false;
        m_phase2FW = false;
        m_phase3FW = true;
#if ART_ENABLE_BSPY && LandingBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "landing - ForwardRoll////CatchFall",latRollOrSideRollSlow);
#endif
        return 3;//catchFall with weakness
      }
      // the end of the roll --> one foot on the ground after the roll -> test for balance
      //feet position
      rage::Vector3 feetPos;
      bool oneFootOnTheGround = false;
      if(m_leftLeg->getFoot()->collidedWithEnvironment()){
        oneFootOnTheGround = true;
        feetPos = m_leftLeg->getFoot()->getPosition();
        if(m_rightLeg->getFoot()->collidedWithEnvironment())
        {
          feetPos = m_leftLeg->getFoot()->getPosition()+m_leftLeg->getFoot()->getPosition();
          feetPos.Scale(0.5f);
        }
      }
      else if(m_rightLeg->getFoot()->collidedWithEnvironment()){
        oneFootOnTheGround = true;
        feetPos = m_rightLeg->getFoot()->getPosition();
      }
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "landing - ForwardRoll",oneFootOnTheGround);
#endif
      if(oneFootOnTheGround||m_waitFW>0){

        //COM position
        rage::Vector3 COMPos = m_character->m_COM;
        //difference in velocity axis regarding the height of the comRot and vel
        rage::Vector3 COMVel = m_character->m_COMvel;
        m_character->levelVector(COMVel);
        COMVel.Normalize();
        rage::Vector3 feetToCOM = COMPos - feetPos;
        float HeightCOM = feetToCOM.Dot(m_character->getUpVector());
        m_character->levelVector(feetToCOM);
        float HorDiff = feetToCOM.Dot(COMVel);
        float threeshold = (-HeightCOM*m_parameters.m_standUpCOMBehindFeet)+(-COMRotVel*m_parameters.m_standUpRotVel);
        bool negative = HorDiff<0.f;
        bool diff = HorDiff>threeshold;
#if ART_ENABLE_BSPY && LandingBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "landing - ForwardRoll", HeightCOM);
        bspyScratchpad(m_character->getBSpyID(), "landing - ForwardRoll", HorDiff);
        bspyScratchpad(m_character->getBSpyID(), "landing - ForwardRoll", threeshold);
        bspyScratchpad(m_character->getBSpyID(), "landing - ForwardRoll", m_waitFW);
        bspyScratchpad(m_character->getBSpyID(), "landing - ForwardRoll",diff);
        bspyScratchpad(m_character->getBSpyID(), "landing - ForwardRoll",negative);
#endif

        if (negative&&diff)
        {
          m_ForwardRollFinished = true;
        }
        if (m_ForwardRollFinished&&!negative)
        {//when feet touched the ground but not even touch the ground until limit is reached
          return 1;//balance
        }
        if((negative&&diff&&!m_phase3FW)||m_waitFW>0){
          m_phase1FW = false;
          m_phase2FW = false;
          m_phase3FW = true;
          //delay the stand up if the rotation velocity is not enough (0.4f if m_waitFW = 6)
          float StartStandUp = COMRotVel + 0.6f*m_waitFW;
          if (StartStandUp>4.0f)
          {
            m_waitFW = 0;
            return 1;//balance
          } 
          else
          {
            m_waitFW++;
            return 0;//0 does nothing
          }
        }
      }

    }
    return 0;//not in phase 3 of FW
  }

  // get some information about our state and decide the best action to take
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBULanding::decideThings()
  {      
    // so that BodyBalance (executed before Landing) can set its own upper body stiffness/damping values
    if (! (m_currentState == HF_Balance || m_currentState == HF_PrepareForLanding || m_currentState == HF_CatchFall || m_currentState == HF_BailOut|| m_currentState == HF_StandUp||m_currentState == HF_ForwardRoll) ) 
    {
      m_character->setBodyStiffness(m_parameters.m_bodyStiffness, m_parameters.m_bodydamping, "fb");
    }

    if (m_currentState == HF_Falling||m_currentState ==HF_PrepareForLanding||(m_currentState ==HF_SideRoll&& !m_hasCollidedWithWorld))
    {

      // the vertical velocity
      rage::Vector3 upVec = m_character->getUpVector();
      rage::Vector3 comVel = m_character->m_COMvel;
      float velocity = comVel.Dot(upVec);

      // work out the angle that we are hitting the ground at
      float forwardVelRotation = m_parameters.m_forwardVelRotation;
      float aimAngleBase = m_parameters.m_aimAngleBase;
      // send a probe out in the direction of the fall, from the middle of the feet, for the height
      rage::Vector3 startOfProbe = m_rightLeg->getFoot()->getPosition() + m_leftLeg->getFoot()->getPosition();
      startOfProbe.Scale(0.5f);
      rage::Vector3 endOfProbe;
      endOfProbe.AddScaled(startOfProbe, comVel, 5);

      bool didItHit = m_character->probeRay(NmRsCharacter::pi_UseNonAsync, startOfProbe, endOfProbe, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);
      rage::Vector3 hitPos = m_character->m_probeHitPos[NmRsCharacter::pi_UseNonAsync];
      rage::Vector3 hitNormal = m_character->m_probeHitNormal[NmRsCharacter::pi_UseNonAsync];;

      // work out the vertical height vector to the ground
      rage::Vector3 hitDirVert = startOfProbe - hitPos;
      rage::Vector3 hitDirHor = hitDirVert;
      m_character->levelVector(hitDirHor);
      hitDirVert = hitDirVert - hitDirHor;

      // check to see if the normal of the thing we have hit is suitable for landing on.
      float normalDotUp = hitNormal.Dot(upVec);
      if (rage::Abs(normalDotUp) < .6)
        didItHit = false;

      // get height above ground: default to something large
      float height = 100.0f;
      if (didItHit)
        height = hitDirVert.Mag();

      //-- the solution to the quadratic equation are: (time to impact)
      rage::Vector3 gravity = rage::phSimulator::GetGravity();
      float a = gravity.Mag();
      float b = -rage::Abs(velocity);
      float c = height;
      if (didItHit)
      {
        Assert(b*b+2*a*c >= 0.f && rage::Abs(a)>1e-10f);
        m_timeToImpact = m_timer + (b+sqrtf(b*b+2*a*c))/(a);
      }
      else 
      {
        m_timeToImpact = m_timer + 30.f; // default to never reach
      }

      // need to decide on success or failure for the fall. gets more stringent with time:
      // accuracy 0 -> 1, as time approaches impact
      float accuracy = 1.0f-((m_timeToImpact-m_timer)/m_timeToImpact);

      //-- work out the aim matrix to orientate body to falling direction
      rage::Matrix34 aimMat;
      aimMat = m_character->m_COMTM;
      aimMat.Translate(0,0,0);

      rage::Vector3 zAx = aimMat.c;//.GetVector(2);
      rage::Vector3 yAx = aimMat.b;//.GetVector(1);
      rage::Vector3 xAx = aimMat.a;//GetVector(0);

      // here work out the forward Velocity
      m_character->levelVector(comVel);
      comVel.Negate();
      m_forwardVel = comVel.Dot(zAx);

      //-- old method  -- for when we are basically falling straight down
      if ((rage::Abs(m_forwardVel)<1) || (!m_parameters.m_orientateBodyToFallDirection))
      { 
        m_character->levelVector(xAx);
        zAx.Cross(xAx,upVec);
      }
      else
      {
        //-- new method -- for when we are falling with an large forward velocity
        zAx = m_character->m_COMvel;
        zAx.Scale(-1.0f);
        m_character->levelVector(zAx);
        zAx.Normalize();
        xAx.Cross(upVec,zAx);       
      }

      // setup desired pelvis matrix
      xAx.Scale(-1.0f);
      aimMat.Set(upVec,xAx,zAx,startOfProbe);

      // rotate desired pelvis matrix by aim angle
      float aimAngle = rage::Clamp(aimAngleBase + forwardVelRotation*m_forwardVel,-0.5f,0.7f);       

      /************************************/
      /* adapt to the slope of the ground */
      /************************************/
      //only during prepare for landing
      if(didItHit && m_currentState ==HF_PrepareForLanding){
        rage::Vector3 comVelN = m_character->m_COMvel;
        m_character->levelVector(comVelN);
        comVelN.Normalize();

        //slope from hitPos to another pos one meter away in direction of the comVel
        rage::Vector3 endOfProbeSlope = hitPos  + comVelN;//add one meter horizontally

        rage::Vector3 probeN = (endOfProbeSlope - startOfProbe);
        probeN.Normalize();
        float verticality = probeN.Dot(upVec);
        verticality = rage::Abs(verticality);
        verticality = rage::Clamp(rage::Abs(verticality),0.05f,1.f);//to limit the length of the probe
        endOfProbeSlope += probeN*(1.f/rage::Abs(verticality));//subtract one meter vertically
        bool didItHitSlope = m_character->probeRay(NmRsCharacter::pi_UseNonAsync, startOfProbe, endOfProbeSlope, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);
        rage::Vector3 hitPosSlope = m_character->m_probeHitPos[NmRsCharacter::pi_UseNonAsync];
        rage::Vector3 hitNormal = m_character->m_probeHitNormal[NmRsCharacter::pi_UseNonAsync];;
        float cosSlope = 0;
        float signSlope = 1;
        if (didItHitSlope)
        {//if the two probe hit something
          rage::Vector3 hitposLevelled = hitPos;
          m_character->levelVector(hitposLevelled);
          rage::Vector3 hitPosSlopeLevelled = hitPosSlope;
          m_character->levelVector(hitPosSlopeLevelled);
          float length = (hitposLevelled - hitPosSlopeLevelled).Mag();
          float heightStart = hitPos.Dot(upVec);
          float heightEnd = hitPosSlope.Dot(upVec);
          float slopePercentage = (heightEnd - heightStart)/length;
          m_slopeGround = slopePercentage*PI/200.f;//100 percents equal to half of Pi
#if ART_ENABLE_BSPY && LandingBSpyDraw
          bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", hitPosSlope);
          bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", length);
          bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", heightEnd - heightStart);
#endif
        }
        else
        {
          cosSlope = hitNormal.Dot(upVec);
          signSlope = hitNormal.Dot(comVelN);

          cosSlope = rage::Clamp(cosSlope,-1.f,1.f);
          m_slopeGround = rage::AcosfSafe(cosSlope);

          if (signSlope < 0.f)
          {
            m_slopeGround = -m_slopeGround;
          }
        }
        if (rage::Abs(m_slopeGround)>0.785f)//if slope too important (45 degrees) : healthy catchfall
        {
          m_healthCatchFall = 1.0f;
          SetState( HF_CatchFall);
        }

        m_slopeGround= rage::Clamp(m_slopeGround,-0.523f,0.f);//max slope to consider : 30/0 degrees
      }
      else
      {
        m_slopeGround = 0.f;
      }
      aimMat.RotateLocalY(aimAngle + m_slopeGround/2.f);

      // get necessary rotation from current to desired pelvis matrix
      rage::Matrix34 currentMat;
      m_spine->getPelvisPart()->getMatrix(currentMat); 
      rage::Vector3 compos1 = m_character->m_COM;
#if ART_ENABLE_BSPY && LandingBSpyDraw
      if (didItHit && m_currentState ==HF_PrepareForLanding)
      {
        m_character->bspyDrawPoint(hitPos, 0.05f, rage::Vector3(1,1,1));
        m_character->bspyDrawLine(hitPos, hitPos+hitNormal*0.4f, rage::Vector3(1,1,1));
      }
      m_character->bspyDrawLine(compos1, compos1+currentMat.a*0.1f, rage::Vector3(1,0,0));
      m_character->bspyDrawLine(compos1, compos1+currentMat.b*0.1f, rage::Vector3(0,1,0));
      m_character->bspyDrawLine(compos1, compos1+currentMat.c*0.1f, rage::Vector3(0,0,1));
#endif
      compos1.AddScaled(upVec,0.1f);
      rage::Vector3 currentRotation = m_character->m_COMrotvel;
      float time = m_parameters.m_predictedTimeToOrientateBodytoFallDirection;
      if (time>(m_timeToImpact-m_timer))
      {
        time = m_timeToImpact;
      }
      float currentRot = currentRotation.Mag()*time;
      currentRotation.Normalize();
      rage::Matrix34 currentMatFromPredicted = currentMat;
      currentMatFromPredicted.Rotate(currentRotation,currentRot);
#if ART_ENABLE_BSPY && LandingBSpyDraw
      m_character->bspyDrawLine(compos1, compos1+currentMatFromPredicted.a*0.1f, rage::Vector3(1,0,0));
      m_character->bspyDrawLine(compos1, compos1+currentMatFromPredicted.b*0.1f, rage::Vector3(0,1,0));
      m_character->bspyDrawLine(compos1, compos1+currentMatFromPredicted.c*0.1f, rage::Vector3(0,0,1));
#endif

      currentMatFromPredicted.Inverse();
      rage::Matrix34 newMat;
      newMat.Dot(currentMatFromPredicted,aimMat);
      currentMatFromPredicted = newMat;

      //bspy

      rage::Vector3 compos2 = compos1;
      compos2.AddScaled(upVec,0.1f);
#if ART_ENABLE_BSPY && LandingBSpyDraw
      m_character->bspyDrawLine(compos2, compos2+aimMat.a*0.1f, rage::Vector3(1,0,0));
      m_character->bspyDrawLine(compos2, compos2+aimMat.b*0.1f, rage::Vector3(0,1,0));
      m_character->bspyDrawLine(compos2, compos2+aimMat.c*0.1f, rage::Vector3(0,0,1));
#endif
      // transform necessary rotation to torque vector from predicted
      rage::Quaternion qFromPredicted;
      currentMatFromPredicted.ToQuaternion(qFromPredicted);
      float rotationFromPredicted;
      rage::Vector3 torqueVecFromPredicted;
      qFromPredicted.ToRotation(torqueVecFromPredicted,rotationFromPredicted);

      // transform necessary rotation to torque vector from current
      currentMat.Inverse();
      newMat.Dot(currentMat,aimMat);
      currentMat = newMat;
      rage::Quaternion qFromCurrent;
      currentMat.ToQuaternion(qFromCurrent);
      float rotation;
      rage::Vector3 torqueVec;
      qFromCurrent.ToRotation(torqueVec,rotation);

      //will take the worst orientation between predicted and current
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - torqueFrompredicted", rotation);
      bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - torqueFrompredicted", rotationFromPredicted);
#endif

      bool sameSign = (rotationFromPredicted>0.f) == (rotation>0.f);
      rotationFromPredicted = rage::Abs(rotationFromPredicted);
      rotation = rage::Abs(rotation);
      float rotationWillLandOk = rage::Abs(rotation);//to test if he will land OK, keep the current
      if (sameSign && (rotationFromPredicted > rotation))
      {
        rotation = rotationFromPredicted;
        torqueVec = torqueVecFromPredicted;
      }

      // if forward velocity too small, only torque sideways
      if (rage::Abs(m_forwardVel)<0.7f)
      {
        xAx = aimMat.b;
        float torqueOntoSide = torqueVec.Dot(xAx);
        torqueVec = xAx;
        torqueVec.Scale(torqueOntoSide);
      }

      // are we gonna achieve desired orientation, i.e. necessary rotation not too large?
      m_willLandOk = true;
      if (rage::Abs(accuracy*rotationWillLandOk) > m_parameters.m_crashOrLandCutOff)
        m_willLandOk = false;

      //test if character is oriented to fall direction
      rage::Matrix34 pelvisMat ;
      m_spine->getPelvisPart()->getMatrix(pelvisMat);
      rage::Vector3 charDir = pelvisMat.c;
      charDir.Negate();
      m_character->levelVector(charDir);
      charDir.Normalize();
      rage::Vector3 COMDir = m_character->m_COMvel;
      bool TooDisoriented = true;
      if (COMDir.Mag()>1.f)
      {
        m_character->levelVector(COMDir);
        COMDir.Normalize();
        float cosAngle = COMDir.Dot(charDir);
        float angleDir = rage::AcosfSafe(cosAngle);
        angleDir = angleDir*180/PI;
        TooDisoriented = rage::Abs(angleDir)>m_parameters.m_angleToCatchFallCutOff;
      }
      // do we need to do a catch fall?
      if (TooDisoriented)
        m_goToCatchFallFlag = true;
      else
        m_goToCatchFallFlag = false;

#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - probe", didItHit);
      bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - probe", (endOfProbe-startOfProbe));
      bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - probe", hitPos);
      bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - probe", hitDirVert);
      bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - probe", height);
      bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - orient", accuracy);
      bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - orient", aimAngle);
      bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - orient", rotation);
      bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - orient", torqueVec);
#endif

      if (m_controlOrientation)
      {
        rage::Vector3 comRV = m_character->m_COMrotvel;
        float velocityOntoTorqueVec = torqueVec.Dot(comRV);
        float P = 260;  // 160 -- maybe need to ramp these up
        float D = 80;   // 60
        float torqMag = m_parameters.m_pdStrength*(P*rotation - D*velocityOntoTorqueVec);
        float scale = rage::Clamp(60.f * m_character->getLastKnownUpdateStep(), 1.f, 60.f);//clamp to keep torque same above 60fps and reduce torque from 60fps to 1fps
        torqMag = torqMag/scale;
#if ART_ENABLE_BSPY && LandingBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "falling.decidethings - orient", torqMag);
#endif
        torqueVec.Scale(torqMag);
        m_spine->getPelvisPart()->applyTorque(torqueVec);
      }
    } 
  }
  float NmRsCBULanding::lengthLegsPercentage(){//return the length of the longer legs in percentage of the length of the length in T pose
    rage::Vector3  hipPos = m_leftLeg->getHip()->getJointPosition();
    rage::Vector3  kneePos = m_leftLeg->getKnee()->getJointPosition();
    rage::Vector3  anklePos = m_leftLeg->getAnkle()->getJointPosition();
    rage::Vector3  footPos = m_leftLeg->getFoot()->getPosition();
    rage::Vector3 leg = hipPos - kneePos;
    float maxLegLength = leg.Mag();
    leg = kneePos - anklePos;
    maxLegLength += leg.Mag();
    leg = anklePos - footPos;
    maxLegLength += leg.Mag();
    leg = hipPos - footPos;
    float legLength = leg.Mag();
    hipPos = m_rightLeg->getHip()->getJointPosition();
    footPos = m_rightLeg->getFoot()->getPosition();
    leg = hipPos - footPos;
    legLength = rage::Max(legLength,leg.Mag());
    return legLength/maxLegLength;
  }
  float NmRsCBULanding::lengthLegsPercentage(bool forRightLeg){//return the length of the right leg (or left if false) in percentage of the length of the length in T pose
    rage::Vector3  hipPos = m_rightLeg->getHip()->getJointPosition();
    rage::Vector3  kneePos = m_rightLeg->getKnee()->getJointPosition();
    rage::Vector3  anklePos = m_rightLeg->getAnkle()->getJointPosition();
    rage::Vector3  footPos = m_leftLeg->getFoot()->getPosition();
    rage::Vector3 leg = hipPos - kneePos;
    float maxLegLength = leg.Mag();
    leg = kneePos - anklePos;
    maxLegLength += leg.Mag();
    leg = anklePos - footPos;
    maxLegLength += leg.Mag();
    if (forRightLeg){//right leg
      leg = hipPos - footPos;
      float legLength = leg.Mag();
      return legLength/maxLegLength;
    }
    else
    {//left leg
      hipPos = m_leftLeg->getHip()->getJointPosition();
      footPos = m_leftLeg->getFoot()->getPosition();
      leg = hipPos - footPos;
      float legLength = leg.Mag();
      return legLength/maxLegLength;
    }
  }
  float NmRsCBULanding::lengthLegs(){//return the length of legs 
    rage::Vector3  hipPos = m_leftLeg->getHip()->getJointPosition();
    rage::Vector3  kneePos = m_leftLeg->getKnee()->getJointPosition();
    rage::Vector3  anklePos = m_leftLeg->getAnkle()->getJointPosition();
    rage::Vector3  footPos = m_leftLeg->getFoot()->getPosition();
    rage::Vector3 leg = hipPos - kneePos;
    float LegLength = leg.Mag();
    leg = kneePos - anklePos;
    LegLength += leg.Mag();
    leg = anklePos - footPos;
    LegLength += leg.Mag();
    return LegLength;
  }
  bool NmRsCBULanding::isVerticalHighFall(){
    rage::Vector3 COMVel = m_character->m_COMvel;
    m_character->levelVector(COMVel);
    if (COMVel.Mag()<m_parameters.m_maxVelForSideRoll)
    {
      // send a probe out in the direction of the fall, from the middle of the feet, for the height
      COMVel = m_character->m_COMvel;        
      rage::Vector3 startOfProbe = m_rightLeg->getFoot()->getPosition() + m_leftLeg->getFoot()->getPosition();
      startOfProbe.Scale(0.5f);
      rage::Vector3 endOfProbe;
      endOfProbe.AddScaled(startOfProbe, COMVel, 100);//max height of fall considered is 100 m 
      bool didItHit = m_character->probeRay(NmRsCharacter::pi_UseNonAsync, startOfProbe, endOfProbe, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);
      rage::Vector3 hitPos = m_character->m_probeHitPos[NmRsCharacter::pi_UseNonAsync];
      rage::Vector3 hitNormal = m_character->m_probeHitNormal[NmRsCharacter::pi_UseNonAsync];;
      if(didItHit){
        rage::Vector3 fall = hitPos - startOfProbe;
        float heightFall = rage::Abs(fall.Dot(m_character->getUpVector()));
        if (heightFall>m_parameters.m_limitNormalFall)
        {
          return true;
        }
      }
    }

    return false;
  }
  bool NmRsCBULanding::isStayingOnHisHead(int time){
    if(m_spine->getHeadPart()->collidedWithEnvironment()&&m_character->m_COMrotvelMag<1.5f){
      m_isStayingOnHisHead++;
    }
    else{m_isStayingOnHisHead=0;}
    if(m_isStayingOnHisHead>time){return true;}
    return false;
  }
  //copy a leg IK but knees only focused on having the right length for the legs.
  void NmRsCBULanding::setFeetposition(float sideOffset,float feetBehindCOM,float lengthLegspercentage,float factorVelocityMovements){

    // work out the average of the two hip positions
    rage::Vector3 hipPL = m_leftLeg->getHip()->getJointPosition();
    rage::Vector3 hipPR = m_rightLeg->getHip()->getJointPosition();
    rage::Vector3 hipP = hipPL + hipPR;
    hipP.Scale(0.5f);

    rage::Matrix34 pelvisOrientation;
    m_spine->getPelvisPart()->getMatrix(pelvisOrientation);
    rage::Vector3 upPelvis = pelvisOrientation.a;
    upPelvis.Normalize();
    //right foot
    float rightLeglength = lengthLegsPercentage(true);
    float sinAngle = sideOffset/rightLeglength;
    float aimLean2Hip = rage::AsinfSafe(sinAngle);
    NmRs3DofEffector *effectorRhip = (NmRs3DofEffector *) m_character->getEffector(m_rightLeg->getHip()->getJointIndex());
    float actualLean2Hip = nmrsGetActualLean2(effectorRhip);
    effectorRhip->setDesiredTwist(0.f);
    effectorRhip->setDesiredLean2(-aimLean2Hip);
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", rightLeglength);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", sinAngle);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", aimLean2Hip);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", actualLean2Hip);
#endif
    //left foot
    float leftLeglength = lengthLegsPercentage(false);
    sinAngle = sideOffset/leftLeglength;
    aimLean2Hip = rage::AsinfSafe(sinAngle);
    NmRs3DofEffector *effectorLhip = (NmRs3DofEffector *) m_character->getEffector(m_leftLeg->getHip()->getJointIndex());
    actualLean2Hip = nmrsGetActualLean2(effectorLhip);
    effectorLhip->setDesiredTwist(0.f);
    effectorLhip->setDesiredLean2(-aimLean2Hip);
#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", leftLeglength);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", sinAngle);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", aimLean2Hip);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", actualLean2Hip);
#endif

    //frontoffset,lean1 of hip
    rage::Vector3 frontOffsetV =m_character->m_COMvel;
    m_character->levelVector(frontOffsetV);
    frontOffsetV.Normalize();
    //right foot
    rage::Vector3 footPR = m_rightLeg->getFoot()->getPosition();
    rage::Vector3 actualFrontOffsetRFoot = footPR - hipP;
    m_character->levelVector(actualFrontOffsetRFoot);
    float RFootFrontOffset = actualFrontOffsetRFoot.Dot(frontOffsetV);
    float feetCOM = - feetBehindCOM;//feetCOM frontward
    float DiffRFoot = feetCOM - RFootFrontOffset;
    sinAngle = DiffRFoot/rightLeglength;
    sinAngle = rage::Clamp(sinAngle,-1.f,1.f);
    float adjustAngle = rage::AsinfSafe(sinAngle);
    rage::Vector3 upPelvisHorizontal = upPelvis;
    m_character->levelVector(upPelvisHorizontal);
    upPelvisHorizontal.Normalize();
    float anglePelvis = rage::AcosfSafe(upPelvisHorizontal.Dot(upPelvis));
    anglePelvis = (PI/2.0f) - anglePelvis;
    if (frontOffsetV.Dot(upPelvis)<0.0f)
    {anglePelvis = - anglePelvis;
    }

    float actualLean1Hip = nmrsGetActualLean1(effectorRhip);
    effectorRhip->setDesiredLean1(adjustAngle*factorVelocityMovements + actualLean1Hip + anglePelvis);

#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", frontOffsetV);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", feetBehindCOM);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", actualFrontOffsetRFoot);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", RFootFrontOffset);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", factorVelocityMovements);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", DiffRFoot);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", actualLean1Hip);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", adjustAngle);
#endif

    //left foot
    rage::Vector3 footPL = m_leftLeg->getFoot()->getPosition();
    rage::Vector3 actualFrontOffsetLFoot = footPL - hipP;
    m_character->levelVector(actualFrontOffsetLFoot);
    float LFootFrontOffset = actualFrontOffsetLFoot.Dot(frontOffsetV);
    float DiffLFoot = feetCOM - LFootFrontOffset;
    sinAngle = DiffLFoot/leftLeglength;
    sinAngle = rage::Clamp(sinAngle,-1.f,1.f);
    adjustAngle = rage::AsinfSafe(sinAngle);
    actualLean1Hip = nmrsGetActualLean1(effectorLhip);
    effectorLhip->setDesiredLean1(adjustAngle*factorVelocityMovements + actualLean1Hip + anglePelvis);

#if ART_ENABLE_BSPY && LandingBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", frontOffsetV);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", actualFrontOffsetLFoot);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", anglePelvis);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", LFootFrontOffset);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.right", DiffLFoot);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", actualLean1Hip);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.", adjustAngle);
#endif

    //length of legs, angle of knees
    rage::Vector3 kneePR = m_rightLeg->getKnee()->getJointPosition();
    rage::Vector3 anklePR = m_rightLeg->getAnkle()->getJointPosition();
    rage::Vector3 LowerLeg = kneePR - anklePR;
    rage::Vector3 UpperLeg = kneePR - hipPR;
    float lengthLowerLeg = LowerLeg.Mag();
    float lengthUpperLeg = UpperLeg.Mag();
    float legLengthMax = lengthLegs();


    float aimLengthLowerLength = legLengthMax* lengthLegspercentage- lengthUpperLeg;
    float angle = rage::AcosfSafe(aimLengthLowerLength/lengthLowerLeg);
    float angleKnees = -angle;
    NmRs1DofEffector *effectorRKnee = (NmRs1DofEffector *) m_character->getEffector(m_rightLeg->getKnee()->getJointIndex());
    NmRs1DofEffector *effectorLKnee = (NmRs1DofEffector *) m_character->getEffector(m_leftLeg->getKnee()->getJointIndex());
    effectorRKnee->setDesiredAngle(angleKnees);
    effectorLKnee->setDesiredAngle(angleKnees);
#if ART_ENABLE_BSPY && LandingBSpyDraw
    float actualAngleRKnee = nmrsGetActualAngle(effectorRKnee);
    float actualAngleLKnee = nmrsGetActualAngle(effectorLKnee);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.Left.setFeetPosition", legLengthMax);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.Left.setFeetPosition", aimLengthLowerLength);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.Right.setFeetPosition", lengthLowerLeg);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.Right.setFeetPosition", lengthUpperLeg);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.Left.setFeetPosition", actualAngleLKnee);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.Right.setFeetPosition", actualAngleRKnee);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.setFeetPosition", angle);
    bspyScratchpad(m_character->getBSpyID(), "ForwardRoll.setFeetPosition", angleKnees);
#endif

  }

  //adjust hand positions with wrists to have angle (hand - COMhorizontal velocity) asked
  //if angle negative, hands are oriented sideways
  void NmRsCBULanding::setHands(float angle){
    rage::Matrix34 leftHand;
    rage::Matrix34 rightHand;
    m_leftArm->getHand()->getMatrix(leftHand);
    m_rightArm->getHand()->getMatrix(rightHand);
    rage::Vector3 leftHandOrientation = leftHand.b;
    rage::Vector3 rightHandOrientation = rightHand.b;
    rage::Vector3 frontVel = m_character->m_COMvel;
    m_character->levelVector(leftHandOrientation);
    m_character->levelVector(rightHandOrientation);
    leftHandOrientation.Negate();
    rightHandOrientation.Negate();
    m_character->levelVector(frontVel);
    leftHandOrientation.Normalize();
    rightHandOrientation.Normalize();
    if (frontVel.Mag()<1.0f)
    {
      rage::Matrix34 pelvisMat;
      m_spine->getPelvisPart()->getMatrix(pelvisMat);
      frontVel = pelvisMat.c;
      frontVel.Negate();
      m_character->levelVector(frontVel);
    }
    frontVel.Normalize();

    float actualLeftAngle = rage::AcosfSafe(leftHandOrientation.Dot(frontVel));
    float actualRightAngle = rage::AcosfSafe(rightHandOrientation.Dot(frontVel));


    rage::Vector3 upVec = m_character->getUpVector();
    rage::Vector3 sideVec;
    sideVec.Cross(upVec,frontVel);//leftside
    if (leftHandOrientation.Dot(sideVec)>0.0f)
    {//sideways
      NmRs3DofEffector *effectorLwrist = (NmRs3DofEffector *) m_character->getEffector(m_leftArm->getWrist()->getJointIndex());
      effectorLwrist->setDesiredLean1(actualLeftAngle+angle);
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "setHands.", actualLeftAngle+angle);
#endif
    }
    else
    {
      NmRs3DofEffector *effectorLwrist = (NmRs3DofEffector *) m_character->getEffector(m_leftArm->getWrist()->getJointIndex());
      effectorLwrist->setDesiredLean1(-actualLeftAngle+angle);
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "setHands.", -actualLeftAngle+angle);
#endif
    }
    if (leftHandOrientation.Dot(sideVec)<0.0f)
    {//sideways
      NmRs3DofEffector *effectorRwrist = (NmRs3DofEffector *) m_character->getEffector(m_rightArm->getWrist()->getJointIndex());
      effectorRwrist->setDesiredLean1(actualRightAngle+angle);
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "setHands.", actualRightAngle+angle);
#endif
    }
    else
    {
      NmRs3DofEffector *effectorRwrist = (NmRs3DofEffector *) m_character->getEffector(m_rightArm->getWrist()->getJointIndex());
      effectorRwrist->setDesiredLean1(-actualRightAngle+angle);
#if ART_ENABLE_BSPY && LandingBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "setHands.", -actualRightAngle+angle);
#endif
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBULanding::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_bodyStiffness, true);
    bspyTaskVar(m_parameters.m_bodydamping, true);
    bspyTaskVar(m_parameters.m_strengthKneeToStandUp, true);
    bspyTaskVar(m_parameters.m_catchfalltime, true);
    bspyTaskVar(m_parameters.m_crashOrLandCutOff, true);
    bspyTaskVar(m_parameters.m_pdStrength, true);
    bspyTaskVar(m_parameters.m_angleToCatchFallCutOff, true);
    bspyTaskVar(m_parameters.m_armsUp, true);
    bspyTaskVar(m_parameters.m_armsFrontward, true);
    bspyTaskVar(m_parameters.m_legRadius, true);
    bspyTaskVar(m_parameters.m_legAngSpeed, true);
    bspyTaskVar(m_parameters.m_aimAngleBase, true);
    bspyTaskVar(m_parameters.m_forwardVelRotation, true);
    bspyTaskVar(m_parameters.m_legL, true);
    bspyTaskVar(m_parameters.m_sideD, true);
    bspyTaskVar(m_parameters.m_legStrength, true);
    bspyTaskVar(m_parameters.m_orientateBodyToFallDirection, true);
    bspyTaskVar(m_parameters.m_predictedTimeToOrientateBodytoFallDirection, true);
    bspyTaskVar(m_parameters.m_ignorWorldCollisions, true);
    bspyTaskVar(m_parameters.m_limitNormalFall, true);
    bspyTaskVar(m_parameters.m_forwardRoll, true);
    bspyTaskVar(m_parameters.m_cheatingTorqueToForwardRoll, true);
    bspyTaskVar(m_parameters.m_feetBehindCOM, true);
    bspyTaskVar(m_parameters.m_feetBehindCOMVel, true);
    bspyTaskVar(m_parameters.m_factorToReduceInitialAngularVelocity, true);  
    bspyTaskVar(m_parameters.m_maxAngVelForForwardRoll, true);  
    bspyTaskVar(m_parameters.m_stopFWCOMRoT, true);  
    bspyTaskVar(m_parameters.m_stopEndFWCOMRoT, true);
    bspyTaskVar(m_parameters.m_standUpCOMBehindFeet, true);
    bspyTaskVar(m_parameters.m_stopFWCOMRoT, true);
    bspyTaskVar(m_parameters.m_maxVelForSideRoll, true);
    bspyTaskVar(m_parameters.m_sideRoll, true);
    bspyTaskVar(m_timer, false);
    bspyTaskVar(m_timeToImpact, false);
    bspyTaskVar(m_forwardVel, false);
    bspyTaskVar(m_slopeGround, false);
    bspyTaskVar(m_rollToBBtimer, false);
    bspyTaskVar(m_averageComVel, false);
    bspyTaskVar(m_alreadyCollidedL, false);
    bspyTaskVar(m_alreadyCollidedR, false);
    bspyTaskVar(m_hasCollidedWithWorld, false);
    bspyTaskVar(m_controlOrientation, false);
    bspyTaskVar(m_willLandOk, false);
    bspyTaskVar(m_messageSent, false);
    bspyTaskVar(m_goToCatchFallFlag, false);
    bspyTaskVar(m_firstFrame, false);
    bspyTaskVar(m_lastFrame, false);
    bspyTaskVar(m_nextFW, false);
    bspyTaskVar(m_waitFW, false);
    bspyTaskVar(m_StandUpTimer, false);
    bspyTaskVar(m_timerTryToStandUp, false);
    bspyTaskVar(m_isStayingOnHisHead, false);
    bspyTaskVar(m_initPosFall, false);
    bspyTaskVar(m_lastPostFall, false);
    bspyTaskVar(m_healthCatchFall, false);
    bspyTaskVar(m_lastKneeStrength, false);
    bspyTaskVar(m_rollDownTimer, false);
    static const char* state_names[] = 
    {
      "Falling",
      "BailOut",
      "PrepareForLanding",
      "ForwardRoll",
      "CatchFall",
      "Balance",
      "StandUp",
      "BalanceStandUp",
      "SideRoll"
    };
    bspyTaskVar_StringEnum(GetState(),state_names, false);

    static const char* failTypeStrings[] =
    {
#define BAL_NAME_ACTION(_name) #_name ,
      BAL_STATES(BAL_NAME_ACTION)
#undef BAL_NAME_ACTION
    };

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    bspyTaskVar(dynamicBalancerTask->m_distKnee, false);
    bspyTaskVar(dynamicBalancerTask->m_heightKnee, false);
    bspyTaskVar(dynamicBalancerTask->m_distHeightKneeRatio, false);
    bspyTaskVar(dynamicBalancerTask->m_dist, false);
    bspyTaskVar(dynamicBalancerTask->m_height, false);
    bspyTaskVar(dynamicBalancerTask->m_distHeightRatio, false);
    bspyTaskVar_StringEnum(dynamicBalancerTask->m_failType, failTypeStrings, false);  
  }
#endif // ART_ENABLE_BSPY

} // nms Art

#endif
