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
* Provides assistance in getting the character to roll down stairs, along
* terrain, ... standard stunt-man stuff
* 
*/


#include "NmRsInclude.h"
#include "NmRsCBU_RollDownStairs.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_BraceForImpact.h"
#include "NmRsCBU_RollUp.h"

namespace ART
{
  NmRsCBURollDownStairs::NmRsCBURollDownStairs(ART::MemoryManager* services) : CBUTaskBase(services, bvid_rollDownStairs)
  {
    initialiseCustomVariables();
  }

  NmRsCBURollDownStairs::~NmRsCBURollDownStairs()
  {
  }

  void NmRsCBURollDownStairs::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_forwardVelVec.Set(0, 0, 0);
    m_accel.Set(0, 0, 0);
    m_decayTime = 0.0f;
    m_currentSlope = 0.0f;
    m_alreadyCollided = false;
  }

  void NmRsCBURollDownStairs::onActivate()
  {
    Assert(m_character);
    //Fall2Knees
    m_fall2Knees = false;
    m_ftk_SpineBend = 0.0f;
    m_ftk_armsIn = false;
    m_ftk_armsOut = false;
    m_ftk_StiffSpine = false;

    if (!m_parameters.m_onlyApplyHelperForces)
    {
      // configure roll-up task
      NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
      Assert(rollUpTask);
      rollUpTask->updateBehaviourMessage(NULL); // initialise parameters

      rollUpTask->m_parameters.m_useArmToSlowDown = m_parameters.m_UseArmsToSlowDown;
      rollUpTask->m_parameters.m_armReachAmount = m_parameters.m_ArmReachAmount;
      rollUpTask->m_parameters.m_stiffness = m_parameters.m_Stiffness;
      rollUpTask->m_parameters.m_legPush = m_parameters.m_LegPush;
      rollUpTask->m_armL = m_parameters.m_ArmL;
      rollUpTask->m_parameters.m_asymmetricalLegs = m_parameters.m_AsymmetricalLegs;
      // .. and activate it
      rollUpTask->activate();
    }
    if (m_parameters.m_applyFoetalToLegs)
    {
      rage::Vector3 upVec = m_character->getUpVector();
      //upper body touches the stairs and not on his feet
      float maxHeight = getSpine()->getHeadPart()->getPosition().Dot(upVec) - 0.8f;
      bool onHisFeet = rage::Max(getLeftLeg()->getFoot()->getPosition().Dot(upVec),getRightLeg()->getFoot()->getPosition().Dot(upVec))<maxHeight;//both feet are really under the head
      //if isColliding is true, applyfoetalToLeg can start
      bool isColliding = m_character->hasCollidedWithWorld(bvmask_UpperBody) && !onHisFeet;
#if ART_ENABLE_BSPY && RDSBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "RDS.", onHisFeet);
      bspyScratchpad(m_character->getBSpyID(), "RDS.", isColliding);
#endif

      m_body->setStiffness(m_parameters.m_Stiffness, m_parameters.m_Damping);

      //before he is really rolling down the stairs
      if (!isColliding && !m_alreadyCollided)
      {
        // configure roll-up task
        NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
        Assert(rollUpTask);
        rollUpTask->updateBehaviourMessage(NULL); // initialise parameters

        rollUpTask->m_parameters.m_useArmToSlowDown = m_parameters.m_UseArmsToSlowDown;
        rollUpTask->m_parameters.m_armReachAmount = m_parameters.m_ArmReachAmount;
        rollUpTask->m_parameters.m_stiffness = m_parameters.m_Stiffness;
        rollUpTask->m_parameters.m_legPush = m_parameters.m_LegPush;
        rollUpTask->m_armL = m_parameters.m_ArmL;
        rollUpTask->m_parameters.m_asymmetricalLegs = m_parameters.m_AsymmetricalLegs;
        // .. and activate it
        rollUpTask->activate();
      }
      else
      {
        m_alreadyCollided = true;
        //deactivate the former rollupTask
        NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
        Assert(rollUpTask);
        rollUpTask->deactivate();
        //start foetal to legs
        m_body->setStiffness(10.0f, m_parameters.m_Damping, bvmask_LegLeft | bvmask_LegRight);
        applyFoetalToLegs();
      }
    }

    if (m_parameters.m_UseCustomRollDir)
      m_forwardVelVec.Set(m_parameters.m_CustomRollDir);
    else
    {
      if (m_parameters.m_useRelativeVelocity)
        m_forwardVelVec.Set(m_character->m_COMvelRelative);
      else
        m_forwardVelVec.Set(m_character->m_COMvel);
    }
    //! we are OK with zeros but NANs would be bad!
    rage::Vector3 zeros; zeros.Zero();
    m_forwardVelVec.NormalizeSafeV(zeros);

    float sinAngle = m_forwardVelVec.Dot(m_character->getUpVector());
    m_currentSlope = rage::AsinfSafe(sinAngle);

    //Initialise the smoothing of m_COMvel direction for passing to m_forwardVelVec
    m_accel.SetScaled(m_character->getUpVector(), -1.0f);

    m_decayTime = 0.f;

    //#ifdef NM_RS_CBU_ASYNCH_PROBES
    //    m_character->InitializeProbe(NmRsCharacter::pi_rollDownStairs);//mmmmtodo test true as it's not overly vital
    //#endif //NM_RS_CBU_ASYNCH_PROBES
  }

  void NmRsCBURollDownStairs::onDeactivate()
  {
    Assert(m_character);

    Assert(m_cbuParent->m_tasks[bvid_bodyRollUp]);
    m_cbuParent->m_tasks[bvid_bodyRollUp]->deactivate();

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBURollDownStairs::onTick(float timeStep)
  {

    bool isColliding = false;
    if ((m_parameters.m_applyNewRollingCheatingTorques||m_parameters.m_applyFoetalToLegs)&&!m_alreadyCollided)
    {
      /*******************************************************************/
      /* test useful before character is really rolling down the stairs, */
      /* will define when start applyFoetalToLegs (and applyTorqueToRoll */
      /* if isFallingInRDSDirection is true too)                         */
      /*******************************************************************/
      rage::Vector3 upVec = m_character->getUpVector();
      //upperbody touches the stairs and not on his feet
      float maxHeight = getSpine()->getHeadPart()->getPosition().Dot(upVec) - 0.8f;
      bool onHisFeet = rage::Max(getLeftLeg()->getFoot()->getPosition().Dot(upVec),getRightLeg()->getFoot()->getPosition().Dot(upVec))<maxHeight;//both feet are really under the head
      //if isColliding or m_alreadyCollided is true, will start applyFoetalToLegs and applyTorqueToRoll
      isColliding = m_character->hasCollidedWithWorld(bvmask_UpperBody) && !onHisFeet;

#if ART_ENABLE_BSPY && RDSBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "RDS.", onHisFeet);
      bspyScratchpad(m_character->getBSpyID(), "RDS.", isColliding);
#endif
    }


    bool NotApplytorques = false;
    if ((m_parameters.m_applyNewRollingCheatingTorques||m_parameters.m_applyHelPerTorqueToAlign)&&m_parameters.m_maxAngVelAroundFrontwardAxis>0.f)
    {
      /*******************************************************************/
      /* test useful during character is really rolling down the stairs, */
      /* will avoid to spin around frontal axis                          */
      /*******************************************************************/
      NotApplytorques = avoidSpinAroundFrontalAxis();
    }

    if (m_parameters.m_applyHelPerTorqueToAlign && m_parameters.m_delayToAlignBody<m_decayTime)
    {
      rage::Vector3 torqueVec;
      rage::Vector3 upVec = m_character->getUpVector();
      torqueVec.Cross(m_forwardVelVec, upVec);
      torqueVec.Normalize();
      torqueVec.Negate();
      float currentAngVelAbs = rage::Abs(torqueVec.Dot(m_character->m_COMrotvel));
      bool NotEnoughAngVelToApplyTorques = currentAngVelAbs<m_parameters.m_minAngVel;
#if ART_ENABLE_BSPY && RDSBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "RDS", currentAngVelAbs);
      bspyScratchpad(m_character->getBSpyID(), "RDS", NotEnoughAngVelToApplyTorques);
#endif
      if (!NotEnoughAngVelToApplyTorques && ! NotApplytorques)
      {
        applyAlignBodyOrthogonally(m_parameters.m_magOfTorqueToAlign);
      }

    }

    // handle body stiffness decay-over-time, if set
    m_decayTime += timeStep;
    if (!m_parameters.m_onlyApplyHelperForces)
    {
      if (m_parameters.m_StiffnessDecayTime > 0.0f)
      {
        float decayT = m_decayTime / m_parameters.m_StiffnessDecayTime;
        decayT = rage::Min(decayT, 1.0f);

        float newBodyStiffnessValue = rage::Clamp((m_parameters.m_Stiffness * (1.0f - decayT)) + (decayT * m_parameters.m_StiffnessDecayTarget),0.f,20.f);
        m_body->setStiffness(newBodyStiffnessValue, m_parameters.m_Damping);
      }
    }

    if (m_parameters.m_useRelativeVelocity)
      m_accel.Add(m_character->m_COMvelRelative);
    else
      m_accel.Add(m_character->m_COMvel);

    m_accel.Normalize();

    if (m_parameters.m_UseCustomRollDir)
      m_forwardVelVec.Set(m_parameters.m_CustomRollDir);
    else
      m_forwardVelVec.Set(m_accel);

    //! we are OK with zeros but NANs would be bad!
    rage::Vector3 zeros; zeros.Zero();
    m_forwardVelVec.NormalizeSafeV(zeros);

    m_accel.Scale(5.0f);//smooths m_COMvel direction for passing to m_forwardVelVec

    float sinAngle = m_forwardVelVec.Dot(m_character->getUpVector());
    m_currentSlope = rage::AsinfSafe(sinAngle);

    NM_RS_DBG_LOGF(L"slope is : %.2f", m_currentSlope);

    // based on the angular velocity blend between zero pose and the Roll Up
    if ((m_parameters.m_UseZeroPose)&&(!m_parameters.m_onlyApplyHelperForces))
    {
      float rotVel = m_character->m_COMrotvelMag;
      NM_RS_DBG_LOGF(L"rotVel is : %.2f", rotVel);

      rotVel = rage::Clamp(rotVel, 0.0f, 10.0f);
      rotVel = 1.0f - (rotVel / 10.0f);
      NM_RS_DBG_LOGF(L"zero-pose rotVel is : %.2f", rotVel);

      m_body->blendToZeroPose(rotVel);
    }
    if (m_parameters.m_applyFoetalToLegs)
    {

      //before he is really rolling down the stairs
      if (!isColliding && !m_alreadyCollided)
      {
        // configure roll-up task
        NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
        Assert(rollUpTask);
        rollUpTask->updateBehaviourMessage(NULL); // initialise parameters

        rollUpTask->m_parameters.m_useArmToSlowDown = m_parameters.m_UseArmsToSlowDown;
        rollUpTask->m_parameters.m_armReachAmount = m_parameters.m_ArmReachAmount;
        rollUpTask->m_parameters.m_stiffness = m_parameters.m_Stiffness;
        rollUpTask->m_parameters.m_legPush = m_parameters.m_LegPush;
        rollUpTask->m_armL = m_parameters.m_ArmL;
        rollUpTask->m_parameters.m_asymmetricalLegs = m_parameters.m_AsymmetricalLegs;
        // .. and activate it
        if (!(rollUpTask->isActive())){
          rollUpTask->activate();
        }
      }
      else
      {
        m_alreadyCollided = true;
        //deactivate the former rollupTask
        NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
        Assert(rollUpTask);
        rollUpTask->deactivate();
        //start foetal to legs
        m_body->setStiffness(10.0f, m_parameters.m_Damping, bvmask_LegLeft | bvmask_LegRight);
        applyFoetalToLegs();
      }
    }
    bool applyNewRollDownStairsForces = false;
    if (m_parameters.m_applyNewRollingCheatingTorques)
    {
      bool isFallingInRDSDirection = (m_forwardVelVec.Dot(m_character->m_COMvel))>0.f;
      //will mostly define when start to apply cheating torque
      //because when he is rolling down the stairs, isFallingInverseDirection will be false
      applyNewRollDownStairsForces = (isColliding || m_alreadyCollided) && isFallingInRDSDirection;
#if ART_ENABLE_BSPY && RDSBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "RDS", isFallingInRDSDirection);
      bspyScratchpad(m_character->getBSpyID(), "RDS", applyNewRollDownStairsForces);
#endif
    }
    // calculate and apply forces
    if (!m_parameters.m_applyNewRollingCheatingTorques)
    {
      float linearTargetVelocityTarget = m_parameters.m_targetLinearVelocity;
      if (m_parameters.m_targetLinearVelocityDecayTime > NM_RS_FLOATEPS)
      {
        linearTargetVelocityTarget *= (1.0f - rage::Clamp(m_decayTime,0.f,m_parameters.m_targetLinearVelocityDecayTime)/m_parameters.m_targetLinearVelocityDecayTime);
      }
      float comVelMag = m_character->m_COMvelMag;
      rage::Vector3 velocity = m_character->m_COMvel;
      NmRsCBUBraceForImpact* bfiTask = (NmRsCBUBraceForImpact*)m_cbuParent->m_tasks[bvid_braceForImpact];
      if (bfiTask->isActive())
      {
        if (m_parameters.m_useRelativeVelocity)
        {
          comVelMag = m_parameters.m_CustomRollDir.Mag();//m_CustomRollDir is the relative velocity of the character to the car
          velocity = m_parameters.m_CustomRollDir;
        }
        if (m_parameters.m_UseCustomRollDir)
        {
          velocity = m_parameters.m_CustomRollDir;
        }
      }
      if (bfiTask->isActive() && bfiTask->m_parameters.roll2Velocity)
        m_character->roll2Velocity(velocity, 0.35f);
      else
        m_character->applyRollDownStairForces(
        comVelMag,
        m_parameters.m_UseCustomRollDir,
        m_parameters.m_useVelocityOfObjectBelow,
        m_parameters.m_useRelativeVelocity,
        m_forwardVelVec, m_currentSlope,
        m_parameters.m_ForceMag,
        m_parameters.m_SpinWhenInAir,
        m_character->getTotalMass(),
        m_parameters.m_zAxisSpinReduction,
        linearTargetVelocityTarget,
        m_parameters.m_airborneReduction);
    }
    else if (!NotApplytorques && applyNewRollDownStairsForces)
    {
      m_character->applyTorqueToRoll(m_parameters.m_minAngVel,m_parameters.m_maxAngVel, m_parameters.m_magOfTorqueToRoll,m_forwardVelVec);
    }
    //Set by shot fallToKnees
    if (m_fall2Knees)
    {
      //As the fallToKnees shot goes from a Knee hit to landing on front:
      //  Give the hips more strength 
      //  Align the thighs with the torso 
      //  Straighten up the spine. 
      //This is to try and have a flat landing on the chest to get a satisfying impact.  
      //  (Otherwise the angle between thighs and spine dampens the landing)         
      getLeftLegInputData()->getHip()->setDesiredLean1(-0.1f);
      getRightLegInputData()->getHip()->setDesiredLean1(-0.1f);
      getLeftLegInputData()->getHip()->setMuscleStrength(m_parameters.m_Stiffness*m_parameters.m_Stiffness);
      getRightLegInputData()->getHip()->setMuscleStrength(m_parameters.m_Stiffness*m_parameters.m_Stiffness);
      getSpineInputData()->getSpine0()->setDesiredLean1(m_ftk_SpineBend);
      getSpineInputData()->getSpine1()->setDesiredLean1(m_ftk_SpineBend);
      getSpineInputData()->getSpine2()->setDesiredLean1(m_ftk_SpineBend);
      getSpineInputData()->getSpine3()->setDesiredLean1(m_ftk_SpineBend);

      if (m_ftk_StiffSpine)
      {
        getSpineInputData()->getSpine0()->setMuscleStrength((m_parameters.m_Stiffness+1.f)*(m_parameters.m_Stiffness+1.f));
        getSpineInputData()->getSpine1()->setMuscleStrength((m_parameters.m_Stiffness+1.f)*(m_parameters.m_Stiffness+1.f));
        getSpineInputData()->getSpine2()->setMuscleStrength((m_parameters.m_Stiffness+1.f)*(m_parameters.m_Stiffness+1.f));
        getSpineInputData()->getSpine3()->setMuscleStrength((m_parameters.m_Stiffness+1.f)*(m_parameters.m_Stiffness+1.f));
      }
      //Optionally get the arms to the side of the torso so they don't dampen the impact with the chest
      if (m_ftk_armsIn || m_ftk_armsOut)
      {
        if (m_ftk_armsIn)
        {
          getLeftArmInputData()->getShoulder()->setDesiredAngles(0.f,0.9f,0.7f);
          getRightArmInputData()->getShoulder()->setDesiredAngles(0.f,0.7f,0.7f);
        }
        else//fling out arms
        {
          getLeftArmInputData()->getShoulder()->setDesiredAngles(0.f,-0.5f,-0.7f);
          getRightArmInputData()->getShoulder()->setDesiredAngles(0.f,-0.9f,-0.7f);
        }
        getLeftArmInputData()->getElbow()->setDesiredAngle(0.9f);
        getRightArmInputData()->getElbow()->setDesiredAngle(0.6f);
      }
    }

    return eCBUTaskComplete;
  }

  bool NmRsCBURollDownStairs::avoidSpinAroundFrontalAxis(){
    /***************************************************/
    /* test to avoid a fast rotation around up         */
    /* vector when he is aligned with roll direction.  */
    /***************************************************/
    // determination of his angular velocity  
    rage::Vector3 currentAngVel = m_character->m_COMrotvel;
    rage::Matrix34 pelvisMat ;
    getSpine()->getPelvisPart()->getMatrix(pelvisMat);
    //by default angular velocity around frontward axis
    rage::Vector3 torqueToTest = pelvisMat.c;
    torqueToTest.Negate();
    torqueToTest.Normalize();
    //if he is not his feet, angular velocity around upFromforwardVel
    rage::Vector3 upVec = m_character->getUpVector();
    float maxHeight = getSpine()->getHeadPart()->getPosition().Dot(upVec) - 0.8f;
    bool onHisFeet = rage::Max(getLeftLeg()->getFoot()->getPosition().Dot(upVec),getRightLeg()->getFoot()->getPosition().Dot(m_character->getUpVector()))<maxHeight;//both feet are really under the head
    if (!onHisFeet)
    {
      rage::Vector3 sideLeveled ;
      sideLeveled.Cross(m_forwardVelVec,upVec);
      m_character->levelVector(sideLeveled);
      sideLeveled.Normalize();
      rage::Vector3 upFromforwardVel ;
      upFromforwardVel.Cross(m_forwardVelVec,sideLeveled);
      upFromforwardVel.Negate();
      torqueToTest = upFromforwardVel;
    }
    float aroundFrontward = rage::Abs(currentAngVel.Dot(torqueToTest));

    //determination of orientation of the character
    rage::Vector3 pelvisUp = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
    rage::Vector3 pelvisUpN = pelvisUp;
    m_character->levelVector(pelvisUpN);
    pelvisUpN.Normalize();
    rage::Vector3 RollDir = m_forwardVelVec;
    m_character->levelVector(RollDir);
    RollDir.Normalize();
    float angleAlignedRollDir = rage::AcosfSafe(rage::Abs(RollDir.Dot(pelvisUpN)));
    bool TooalignedInRollDir = angleAlignedRollDir<1.15f;//65 degrees
    bool tooMuchAngVelAroundFrontward = aroundFrontward>m_parameters.m_maxAngVelAroundFrontwardAxis;
    bool NotApplytorques =  tooMuchAngVelAroundFrontward && TooalignedInRollDir;
#if ART_ENABLE_BSPY && RDSBSpyDraw
    rage::Vector3 pelvisP =getSpine()->getPelvisPart()->getPosition();
    m_character->bspyDrawLine(pelvisP, pelvisP+torqueToTest*0.15f, rage::Vector3(0,0,1));
    bspyScratchpad(m_character->getBSpyID(), "RDS.avoidSpinAroundFrontalAxis", aroundFrontward);
    bspyScratchpad(m_character->getBSpyID(), "RDS.avoidSpinAroundFrontalAxis", tooMuchAngVelAroundFrontward);
    bspyScratchpad(m_character->getBSpyID(), "RDS.avoidSpinAroundFrontalAxis", angleAlignedRollDir);
    bspyScratchpad(m_character->getBSpyID(), "RDS.avoidSpinAroundFrontalAxis", TooalignedInRollDir);
    bspyScratchpad(m_character->getBSpyID(), "RDS.avoidSpinAroundFrontalAxis", NotApplytorques);
#endif
    return NotApplytorques;
  }
  void NmRsCBURollDownStairs::applyAlignBodyOrthogonally(float magOfTorque){
    rage::Vector3 upVec = m_character->getUpVector();
    rage::Vector3 pelvisUp = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
    rage::Vector3 pelvisUpN = pelvisUp;
    pelvisUpN.Normalize();
    rage::Vector3 sideVelVec;
    rage::Vector3 frontVelVec = m_forwardVelVec;
    m_character->levelVector(frontVelVec);
    frontVelVec.Normalize();
    sideVelVec.Cross(m_forwardVelVec,upVec);
    sideVelVec.Normalize();
    rage::Vector3 torqueVec;
    torqueVec.Cross(pelvisUpN,sideVelVec);
    torqueVec.Normalize();

    float rotation = rage::AcosfSafe(sideVelVec.Dot(pelvisUpN));
    rage::Vector3 angleVelCOM = m_character->m_COMrotvel;
    float velocityOntoTorqueVec = angleVelCOM.Dot(torqueVec);
    bool leftSide = rotation-PI/(2.f)<0.f;
    if (rage::Abs(rotation-PI/(2.f))<0.2f)
    {
      leftSide = (rotation-PI/(2.f)+ velocityOntoTorqueVec/10.f)<0;
    }

    if (!leftSide)
    {
      torqueVec.Negate();
    }
    rage::Vector3 Pos = getSpine()->getPelvisPart()->getPosition();
#if ART_ENABLE_BSPY && RDSBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "RDS.applyAlignBodyOrthogonally", rotation);
    bspyScratchpad(m_character->getBSpyID(), "RDS.applyAlignBodyOrthogonally", velocityOntoTorqueVec);
    bspyScratchpad(m_character->getBSpyID(), "RDS.applyAlignBodyOrthogonally", leftSide);
#endif

    float total = 1.57f + 3.0f;//max diff angle and max angular velocity
    float P = 2.5f/total;  
    float D = 0.6f/total;
    if (rotation>(PI/2.f))
    {
      rotation = PI - rotation;
    }
    float torqMag = (P*rotation - D*rage::Abs(velocityOntoTorqueVec));
    torqMag = rage::Clamp(torqMag, 0.0f, 1.f);
    torqueVec.Normalize();
    torqueVec.Scale(torqMag*magOfTorque);
    getSpine()->getPelvisPart()->applyTorque(torqueVec*0.2f);
    getSpine()->getSpine0Part()->applyTorque(torqueVec*0.2f);
    getSpine()->getSpine1Part()->applyTorque(torqueVec*0.2f);
    getSpine()->getSpine2Part()->applyTorque(torqueVec*0.2f);
    getSpine()->getSpine3Part()->applyTorque(torqueVec*0.2f);
  }

  void NmRsCBURollDownStairs::applyFoetalToLegs(){
    // configure roll-up task for upper body
    NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
    Assert(rollUpTask);
    if (! rollUpTask->isActive() )
    {
      rollUpTask->updateBehaviourMessage(NULL); // initialise parameters
      rollUpTask->m_parameters.m_useArmToSlowDown = m_parameters.m_UseArmsToSlowDown;
      rollUpTask->m_parameters.m_armReachAmount = m_parameters.m_ArmReachAmount;
      rollUpTask->m_parameters.m_stiffness = m_parameters.m_Stiffness;
      rollUpTask->m_armL = m_parameters.m_ArmL;
      rollUpTask->m_parameters.m_effectorMask = bvmask_UpperBody;
      rollUpTask->activate();
    }

    // configure legs
    // current position of the legs
    rage::Vector3 pelvisPos = getSpine()->getPelvisPart()->getPosition();
    rage::Vector3 feetPosR = getRightLeg()->getFoot()->getPosition();
    rage::Vector3 feetPosL = getLeftLeg()->getFoot()->getPosition();
    rage::Vector3 pelvisToFootR = feetPosR - pelvisPos;
    rage::Vector3 pelvisToFootL = feetPosL - pelvisPos;
    pelvisToFootR.Scale(1.f); 
    pelvisToFootL.Scale(1.f);
    rage::Vector3 upVec = m_character->getUpVector();
    rage::Vector3 sideVelVec;
    sideVelVec.Cross(m_parameters.m_CustomRollDir,upVec);

    float factormovement = m_parameters.m_movementLegsInFoetalPosition;

    if (m_parameters.m_UseCustomRollDir)
    {
      if (m_parameters.m_CustomRollDir.Mag()>0.7f)
      {
        rage::Vector3 frontDir = m_parameters.m_CustomRollDir;
        sideVelVec.Cross(frontDir,upVec);
        upVec.Cross(sideVelVec,frontDir);
      }
    }
    float amountR = pelvisToFootR.Dot(upVec);
    float amountL = pelvisToFootL.Dot(upVec);

    //set default angles
    float angleHipLean1 = 1.6f;
    float angleHipLean2 = -0.15f;
    float angleHipTwist = -0.15f;
    float angleKnee = -2.0f;

    //add movement regarding position of the legs compare to upVector
    float factorHipLean1 = 0.15f;//variation of knee angles
    float factorHipLean2 = 0.15f;//variation of knee angles
    float factorHipTwist = 0.15f;//variation of knee angles
    float factorKnee = 1.f;//variation of knee angles
    getLeftLegInputData()->getHip()->setDesiredAngles(
      angleHipLean1 - amountL*factorHipLean1*factormovement,
      angleHipLean2 - amountL*factorHipLean2*factormovement,
      angleHipTwist - amountL*factorHipTwist*factormovement);
    getRightLegInputData()->getHip()->setDesiredAngles(
      angleHipLean1 - amountR*factorHipLean1*factormovement,
      angleHipLean2 - amountR*factorHipLean2*factormovement,
      angleHipTwist - amountR*factorHipTwist*factormovement);
    getLeftLegInputData()->getKnee()->setDesiredAngle(angleKnee + amountL*factorKnee*factormovement);
    getRightLegInputData()->getKnee()->setDesiredAngle(angleKnee + amountR*factorKnee*factormovement);
  }
#if ART_ENABLE_BSPY 
  void NmRsCBURollDownStairs::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_Stiffness, true);
    bspyTaskVar(m_parameters.m_Damping, true);
    bspyTaskVar(m_parameters.m_ForceMag, true);
    bspyTaskVar(m_parameters.m_AsymmetricalForces, true);//unused
    bspyTaskVar(m_parameters.m_AsymmetricalLegs, true);
    bspyTaskVar(m_parameters.m_UseArmsToSlowDown, true);
    bspyTaskVar(m_parameters.m_ArmReachAmount, true);

    bspyTaskVar(m_parameters.m_LegPush, true);
    bspyTaskVar(m_parameters.m_ArmL, true);
    bspyTaskVar(m_parameters.m_StiffnessDecayTime, true);
    bspyTaskVar(m_parameters.m_StiffnessDecayTarget, true);
    bspyTaskVar(m_parameters.m_zAxisSpinReduction, true);
    bspyTaskVar(m_parameters.m_targetLinearVelocity, true);
    bspyTaskVar(m_parameters.m_targetLinearVelocityDecayTime, true);

    bspyTaskVar(m_parameters.m_UseZeroPose, true);
    bspyTaskVar(m_parameters.m_SpinWhenInAir, true);
    bspyTaskVar(m_parameters.m_TryToAvoidHeadButtingGround, true);
    bspyTaskVar(m_parameters.m_UseCustomRollDir, true);

    bspyTaskVar(m_parameters.m_airborneReduction, true);

    bspyTaskVar(m_parameters.m_CustomRollDir, true);
    bspyTaskVar(m_parameters.m_onlyApplyHelperForces, true);

    bspyTaskVar(m_parameters.m_applyFoetalToLegs, true);
    bspyTaskVar(m_parameters.m_movementLegsInFoetalPosition, true);

    bspyTaskVar(m_parameters.m_useVelocityOfObjectBelow, true);
    bspyTaskVar(m_parameters.m_useRelativeVelocity, true);

    bspyTaskVar(m_parameters.m_maxAngVelAroundFrontwardAxis, true);
    bspyTaskVar(m_parameters.m_applyNewRollingCheatingTorques, true);
    bspyTaskVar(m_alreadyCollided, false);
    bspyTaskVar(m_parameters.m_magOfTorqueToRoll, true);
    bspyTaskVar(m_parameters.m_minAngVel, true);
    bspyTaskVar(m_parameters.m_maxAngVel, true);

    bspyTaskVar(m_parameters.m_applyHelPerTorqueToAlign, true);
    bspyTaskVar(m_parameters.m_delayToAlignBody, true);
    bspyTaskVar(m_parameters.m_magOfTorqueToAlign, true);

    bspyTaskVar(m_decayTime, false);
    bspyTaskVar(m_currentSlope, false);
    float linearTargetVelocityTarget = m_parameters.m_targetLinearVelocity;
    if (m_parameters.m_targetLinearVelocityDecayTime > NM_RS_FLOATEPS)
    {
      linearTargetVelocityTarget *= (1.0f - rage::Clamp(m_decayTime,0.f,m_parameters.m_targetLinearVelocityDecayTime)/m_parameters.m_targetLinearVelocityDecayTime);
    }
    bspyTaskVar(linearTargetVelocityTarget, false);

    bspyTaskVar(m_forwardVelVec, false);
    bspyTaskVar(m_accel, false);
    //Fall2Knees
    bspyTaskVar(m_fall2Knees, true);
    bspyTaskVar(m_ftk_armsOut, true);
    bspyTaskVar(m_ftk_armsIn, true);

  }

#endif // ART_ENABLE_BSPY
}

