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
#include "NmRsCBU_FallOverWall.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"
#if useNewFallOverWall
#include "NmRsCBU_Catchfall.h" 
#endif//useNewFallOverWall

#include "ART/ARTFeedback.h"

namespace ART
{
  NmRsCBUFallOverWall::NmRsCBUFallOverWall(ART::MemoryManager* services) : CBUTaskBase(services, bvid_fallOverWall),
    m_hitWall(false),
    m_negateTorques(false)
  {
    initialiseCustomVariables();
  }

  NmRsCBUFallOverWall::~NmRsCBUFallOverWall()
  {
  }

  void NmRsCBUFallOverWall::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    // reset the params
    m_hitWall   = false;
    m_negateTorques = false;
    m_blean1    = 0.0f;
    m_blean2    = 0.0f;
    m_forceMultiplier = 60.0f;
    m_wallEdge.Zero();
    m_wallNormal.Zero();
    m_FOWState = fow_ApproachingWall;

    m_forceTimer = 0.0f;
    m_amountOfRoll = 0.0f;
    m_projPelvisPos.Zero();
    m_pelvisToWall.Zero();
    m_comAngVelFromMomentum.Zero();
    m_comAngVelEdgeComponent = 0.0f;
    m_overWallYet = false;
    m_upNess = 0.0f;
    m_rollingBack = false;
    m_rollingPotentialOnImpact = 0.0f;
    m_rollingPotentialOnImpactObtained = false;

#if useNewFallOverWall
    m_isOnTheGround = false;
    m_isTotallyBack = false;
    m_neverHitWall = true;
    m_parameters.minVelForLiedOnTheGround = 1.5f;
#endif// useNewFallOverWall

  }

  void NmRsCBUFallOverWall::onActivate()
  {
    Assert(m_character);

    // pre-calculate wall info. better info only available from impact later
    m_wallEdge = m_parameters.fallOverWallEndA - m_parameters.fallOverWallEndB;
    m_wallEdge.Normalize();
    m_wallNormal.CrossSafe(m_character->m_gUp, m_wallEdge);
    m_wallNormal.Normalize();
    m_pelvisPos = getSpine()->getPelvisPart()->getPosition();
    m_projPelvisPos = m_pelvisPos;
    getHorizontalDistancePointFromLine(m_pelvisPos,m_parameters.fallOverWallEndA,m_parameters.fallOverWallEndB,&m_projPelvisPos);
    float upWall = setHeightTarget(m_projPelvisPos);
    m_character->levelVector(m_projPelvisPos,upWall);
    m_pelvisToWall = m_projPelvisPos - m_pelvisPos;
    // make sure wall normal is facing right direction, i.e. from wall to character      
    float sideOfWallDot = m_pelvisToWall.Dot(m_wallNormal);
    if(sideOfWallDot > 0.0f)
    {
      m_wallNormal.Scale(-1.0f);
      m_wallEdge.Scale(-1.0f);
    }
#if ART_ENABLE_BSPY && 0
    bspyScratchpad(m_character->getBSpyID(), "FoW activate", m_wallEdge);
    bspyScratchpad(m_character->getBSpyID(), "FoW activate", m_wallNormal);
#endif

    // TEST: setup balancer exclusion zone to avoid stepping with knee into wall   
    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);
    if (!balColReactTask->isActive())
    {
      balColReactTask->m_pos1stContact = m_projPelvisPos;
      balColReactTask->m_normal1stContact = m_wallNormal;
      balColReactTask->m_impactOccurred = true;
      balColReactTask->m_sideOfPlane = sideOfWallDot;
      balColReactTask->m_parameters.exclusionZone = m_parameters.stepExclusionZone;
      balColReactTask->m_balancerState = bal_Trip;//MMMMtest fow in standalone mode works with this commented out
    }
    sendStateFeedback(m_FOWState);

    m_twist = 0.f;
  }

  void NmRsCBUFallOverWall::onDeactivate()
  {
    Assert(m_character);

    initialiseCustomVariables();
    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);
    if (!balColReactTask->isActive())
    {
      balColReactTask->m_impactOccurred = false;
    }
    //fallOverWall changes the leg muscles
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    if (dynamicBalancerTask->isActive())
      dynamicBalancerTask->calibrateLowerBodyEffectors(m_body);
  }

  CBUTaskReturn NmRsCBUFallOverWall::onTick(float timeStep)
  {
    calculateCommonVariables();

#if useNewFallOverWall
    if(m_parameters.useArmIK){
      applyArmIK(timeStep);
    }
#endif//end of useNewfallOverWall

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // test for wall collision to decided whether to start roll
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (!m_hitWall)
    {
      rage::Vector3 hitPos;
      rage::Vector3 hitNormal;

      if (getSpine()->getPelvisPart()->collidedWithEnvironment())
      {
        getSpine()->getPelvisPart()->getCollisionZMPWithEnvironment(hitPos,hitNormal);
        m_hitWall = true;
      }
      else if (getSpine()->getSpine0Part()->collidedWithEnvironment())
      {
        getSpine()->getSpine0Part()->getCollisionZMPWithEnvironment(hitPos,hitNormal);
        m_hitWall = true;
      }
      else if (getSpine()->getSpine1Part()->collidedWithEnvironment())
      {
        getSpine()->getSpine1Part()->getCollisionZMPWithEnvironment(hitPos,hitNormal);
        m_hitWall = true;
      }
      else
      {
        // for hands and elbows, check first to see how far they're away from the pelvis
        // only report hitWall when close enough (not when arms are stretched out).
        // ordered these so that the most useful part will be taken if there are several collisions
        if (getLeftLeg()->getThigh()->collidedWithEnvironment())
        {
          getLeftLeg()->getThigh()->getCollisionZMPWithEnvironment(hitPos,hitNormal);
          m_hitWall = true;
        }
        else if (getRightLeg()->getThigh()->collidedWithEnvironment())
        {
          getRightLeg()->getThigh()->getCollisionZMPWithEnvironment(hitPos,hitNormal);
          m_hitWall = true;
        }
        else if(getLeftArm()->getHand()->collidedWithEnvironment())
        {
          getLeftArm()->getHand()->getCollisionZMPWithEnvironment(hitPos,hitNormal);
          m_hitWall = true;
        }
        else if(getRightArm()->getHand()->collidedWithEnvironment())
        {
          getRightArm()->getHand()->getCollisionZMPWithEnvironment(hitPos,hitNormal);
          m_hitWall = true;
        }
        else if(getRightArm()->getLowerArm()->collidedWithEnvironment())
        {
          getRightArm()->getLowerArm()->getCollisionZMPWithEnvironment(hitPos,hitNormal);
          m_hitWall = true;
        }
        else if(getLeftArm()->getLowerArm()->collidedWithEnvironment())
        {
          getLeftArm()->getLowerArm()->getCollisionZMPWithEnvironment(hitPos,hitNormal);
          m_hitWall = true;
        }
        else if(getSpine()->getSpine2Part()->collidedWithEnvironment())
        {
          getSpine()->getSpine2Part()->getCollisionZMPWithEnvironment(hitPos,hitNormal);
          m_hitWall = true;
        }
        // if collision with above parts occurred,
        // check distance first to see whether we really want m_hitWall to be true
        if(m_hitWall == true)
        {
          rage::Vector3 levelPelvisToWall(m_pelvisToWall);
          m_character->levelVector(levelPelvisToWall);
          float dist = levelPelvisToWall.Mag();
          m_hitWall = dist < m_parameters.maxDistanceFromPelToHitPoint;
          if(m_hitWall) 
            hitPos = m_projPelvisPos;
        }
      }

      m_wallHitPos = hitPos;

      // check that we are standing up tall enough: calculate height of pelvis above feet.
      rage::Vector3 lFootP = getLeftLeg()->getFoot()->getPosition();
      lFootP = lFootP - m_pelvisPos;
      rage::Vector3 vTem = lFootP;
      m_character->levelVector(vTem);
      lFootP = lFootP - vTem;

      rage::Vector3 rFootP = getRightLeg()->getFoot()->getPosition();
      rFootP = rFootP - m_pelvisPos;
      vTem = rFootP;
      m_character->levelVector(vTem);
      rFootP = rFootP - vTem;

      float rLegH = rFootP.Mag();
      float lLegH = lFootP.Mag();
      float maxLegHeight = rage::Max(rLegH, lLegH);
      bool legHGood = maxLegHeight > m_parameters.minLegHeight;

      // check that the hitNormal is in the horizontal direction (well enough to be good) - if it isn't use the wallNormal
      //  hitNormal is only used to determine the spine lean1, lean2
      if (rage::Abs(m_character->m_gUp.Dot(hitNormal)) > 0.7f)//not hrizontal enough
      {
        hitNormal = m_wallNormal;
      }
      rage::Vector3 characterSideways = getSpine()->getLowerNeck()->getJointPosition() - getSpine()->getPelvisPart()->getPosition();
      characterSideways.Cross(m_wallNormal);
      characterSideways.Normalize();
      float upDotCharacaterSide = rage::Abs(m_character->m_gUp.Dot(characterSideways));
      float limitLeaning = rage::Abs(rage::Cosf(PI*(m_parameters.leaningAngleThreshold+90.f)/180));
      if (m_parameters.leaningAngleThreshold>90.f)
      {
        limitLeaning = 1.f;
      }
      bool leaningGood = upDotCharacaterSide<limitLeaning;

      rage::Vector3 dir = m_character->m_COMvel;
      m_character->levelVector(dir);
      dir.Normalize();
      float normDot = m_wallNormal.Dot(dir);
      float limitDir = rage::Abs(rage::Cosf(PI*m_parameters.angleDirWithWallNormal/180.f));
      if (m_parameters.angleDirWithWallNormal>90.f)
      {
        limitDir = 1.f;
      }
      bool directionGood = m_hitWall || (normDot < limitDir);

      rage::Vector3 upVector  = m_character->getUpVector();
      lFootP = getLeftLeg()->getFoot()->getPosition();
      rFootP = getRightLeg()->getFoot()->getPosition();
      float heightWall = rage::Min(m_parameters.fallOverWallEndA.Dot(upVector),m_parameters.fallOverWallEndB.Dot(upVector));
      float LowestFoot = rage::Min(rFootP.Dot(upVector),lFootP.Dot(upVector));
      heightWall -= LowestFoot;
      bool heightWallGood = m_parameters.maxWallHeight< 0.f || heightWall<m_parameters.maxWallHeight;


#if useNewFallOverWall
      m_isOnTheGround = !(m_hitWall && legHGood && leaningGood && directionGood);

#endif
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
      float angleLimitLeaning = PI*rage::AcosfSafe(upDotCharacaterSide)/180.f;
      float angleLimitDir = PI*rage::AcosfSafe(normDot)/180.f;
      bspyScratchpad(m_character->getBSpyID(), "FoW", legHGood);
      bspyScratchpad(m_character->getBSpyID(), "FoW", limitLeaning);
      bspyScratchpad(m_character->getBSpyID(), "FoW", angleLimitLeaning);
      bspyScratchpad(m_character->getBSpyID(), "FoW", leaningGood);
      bspyScratchpad(m_character->getBSpyID(), "FoW", angleLimitDir);
      bspyScratchpad(m_character->getBSpyID(), "FoW", directionGood);
#if useNewFallOverWall
      bspyScratchpad(m_character->getBSpyID(), "FoW",m_isOnTheGround);
#endif
      bspyScratchpad(m_character->getBSpyID(), "FoW", heightWall);
      bspyScratchpad(m_character->getBSpyID(), "FoW", heightWallGood);
#endif
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw && useNewFallOverWall
      //bspy
      bspyScratchpad(m_character->getBSpyID(), "FoW",m_isOnTheGround);
#endif

      //Put this here or just before end of if (!m_hitWall) block?
      if (m_hitWall && !m_rollingPotentialOnImpactObtained)
      {
        m_rollingPotentialOnImpact = rollingPotentialOnImpact();
        m_rollingPotentialOnImpactObtained = true;
      }

      if (m_hitWall && legHGood && leaningGood && directionGood && heightWallGood)
      {
        // reevaluate the wall edge from collision data, (shouldn't be necessary anymore, with proper check for direction at activation (see above)
        //m_wallEdge = hitNormal;
        //m_wallEdge.CrossSafe(m_wallEdge, m_character->m_gUp);
        //m_wallEdge.Normalize();

        // work out the back lean1, lean2
        rage::Matrix34 pelMat;
        getSpine()->getPelvisPart()->getMatrix(pelMat);
        rage::Vector3 sideV = pelMat.b;
        rage::Vector3 frontV = pelMat.c;
        m_blean1 = -hitNormal.Dot(frontV);
        m_blean2 = -hitNormal.Dot(sideV);

        // TEST : now in the tick too
        m_body->setStiffness(m_parameters.bodyStiffness, m_parameters.bodyDamping);

#if useNewFallOverWall
        if(m_parameters.useArmIK)
        {
          //modify strength of the arms to be sure armIK has an effect
          float clampedStiffness =  rage::Clamp(m_parameters.bodyStiffness,10.f,16.f);
          float clampedDamping =  rage::Clamp(m_parameters.bodyDamping,0.5f,2.f);
          m_character->setBodyStiffness(clampedStiffness, clampedDamping, bvmask_Arms);
        }
#endif
        m_negateTorques = m_character->getRandom().GetBool();

        // TEST: avoid legs pushing away from wall
        if (m_parameters.moveLegs)
        {
          NmRsCBUDynamicBalancer *dynBal = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];
          dynBal->setForceBalance(true);
        }
      }
      else
      {
        m_hitWall = false;
      }
    } // if (!m_hitWall)

#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
    rage::Vector3 c;
    c = m_hitWall ? rage::Vector3(1,0,0) : rage::Vector3(0,1,0);
    m_character->bspyDrawPoint(m_wallHitPos, 0.2f, c);
    m_character->bspyDrawLine(m_projPelvisPos, m_projPelvisPos + m_wallEdge, rage::Vector3(1,0,0));
    m_character->bspyDrawLine(m_projPelvisPos, m_projPelvisPos + m_wallNormal, rage::Vector3(1,0,0));
#endif

    stateLogicTick();
    feedbackLogicTick();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Go over the wall
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (m_hitWall)
    {
      m_body->setStiffness(m_parameters.bodyStiffness, m_parameters.bodyDamping);//mmmmtodo consider doing this only if rolling

      m_character->m_uprightConstraint.forceActive = false;
      m_character->m_uprightConstraint.torqueActive = false;
      //m_leftLeg->getThigh()->setFrictionMultiplier(fric);
      //m_rightLeg->getThigh()->setFrictionMultiplier(fric);
#if useNewFallOverWall
      m_neverHitWall = !m_hitWall;
      if(m_parameters.useArmIK)
      {
        //modify strength of the arms to be sure armIK has an effect
        float clampedStiffness =  rage::Clamp(m_parameters.bodyStiffness,10.f,16.f);
        float clampedDamping =  rage::Clamp(m_parameters.bodyDamping,0.5f,2.f);
        m_character->setBodyStiffness(clampedStiffness, clampedDamping, bvmask_Arms);
      }
#endif// useNewFallOverWall

      // TEST: move hit pos along
      m_wallHitPos = m_projPelvisPos;

      // finally do the falling bit
      if (m_FOWState == fow_RollingOverWall)
      {
        // !hdd! perhaps make the arms-up-swing based on comrotvel, so if fallOverWall isnt
        // successful in getting the character to fall over the wall, he wont just fling his
        // arms into the air and then fall to the floor

        //-- make the arms go up
        if (m_parameters.moveArms)
        {
          float wt = PI-0.3f;
          float swing = rage::Min(4.0f-wt,0.32f);

          getLeftArmInputData()->getElbow()->setDesiredAngle(3.5f*swing);
          getLeftArmInputData()->getClavicle()->setDesiredLean1(0.6f*sin(wt)+(-0.2f));
          getLeftArmInputData()->getClavicle()->setDesiredLean2(cos(wt)-0.5f);
          getLeftArmInputData()->getShoulder()->setDesiredLean1(0.5f*sin(wt));
          getLeftArmInputData()->getShoulder()->setDesiredLean2((1.5f)*cos(wt));
          getLeftArmInputData()->getShoulder()->setDesiredTwist(-2.5f*sin(wt)-1.0f);

          getRightArmInputData()->getElbow()->setDesiredAngle(3.2f*swing);
          getRightArmInputData()->getClavicle()->setDesiredLean1(0.654f*sin(wt)+(-0.24f));
          getRightArmInputData()->getClavicle()->setDesiredLean2(cos(wt)-0.7f);
          getRightArmInputData()->getShoulder()->setDesiredLean1(0.4f*sin(wt));
          getRightArmInputData()->getShoulder()->setDesiredLean2((1.6f)*cos(wt));
          getRightArmInputData()->getShoulder()->setDesiredTwist(-2.35f*sin(wt)-1.1f);
        }

        static bool updateSpineBend = true;
        if (m_parameters.bendSpine)
        {
          if (updateSpineBend)
          {
            rage::Matrix34 pelMat;
            getSpine()->getPelvisPart()->getMatrix(pelMat);
            rage::Vector3 sideV = pelMat.b;
            rage::Vector3 frontV = pelMat.c;
            m_blean1 = -m_wallNormal.Dot(frontV);
            m_blean2 = -m_wallNormal.Dot(sideV);
          }
          getSpineInputData()->applySpineLean(-2.f*m_blean1, 1.0f*m_blean2);//mmmmtodo these leans only calulated on 1st impact - consider updating continuously
        }

        if (m_parameters.moveLegs)
        {
          //float legLength = rage::Clamp(.8f - (1.0f-rage::Abs(upDot))*1.3f,0.0f,1.0f);
          rage::Matrix34 pelMat;
          getSpine()->getPelvisPart()->getMatrix(pelMat);
          rage::Vector3 forward = pelMat.c;
          forward *= -0.2f;
          rage::Vector3 left = pelMat.b;
          left *= 0.05f;
          float leftDotUp = left.Dot(m_character->m_gUp);
          rage::Vector3 downV = pelMat.a;//a=up,b=left,c=back
          float legLength = rage::Abs(m_upNess)*0.5f + leftDotUp*0.5f;
          legLength = rage::Clamp(legLength,0.6f,0.9f);//0.9 rig dependent to get not straight legs
          //float legLength = rage::Clamp(0.5f*downV.Dot(m_wallNormal)+rage::Abs(upDot)*0.8f + left.Dot(m_character->m_gUp)*0.2f,0.2f,1.0f);
          //float legLength = rage::Clamp(1.f-0.8f*downV.Dot(m_wallNormal)+ left.Dot(m_character->m_gUp)*0.2f,0.2f,1.0f);

          downV.Scale(-legLength);
          // left leg IK
          rage::Vector3 legIkP = getLeftLeg()->getHip()->getJointPosition();
          legIkP = legIkP + downV + left + forward*1.25f;
          if (!(getLeftLeg()->getFoot()->collidedWithNotOwnCharacter()))
          {
            NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
            NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
            ikInputData->setTarget(legIkP);
            ikInputData->setTwist(1.5f);
            getLeftLeg()->postInput(ikInput);
          }

          legLength = rage::Abs(m_upNess)*0.7f - leftDotUp*0.3f;
          legLength = rage::Clamp(0.6f+legLength*0.3f,0.6f,0.9f);//0.9 rig dependent to get not straight legs

          downV = pelMat.a;//a=up,b=left,c=back
          downV.Scale(-legLength);
          // right leg IK
          legIkP = getRightLeg()->getHip()->getJointPosition();
          legIkP = legIkP + downV - left + forward;
          if (!(getRightLeg()->getFoot()->collidedWithNotOwnCharacter()))
          {
            NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
            NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
            ikInputData->setTarget(legIkP);
            ikInputData->setTwist(1.5f);
            getLeftLeg()->postInput(ikInput);
          }

          static bool setLean = true;
          getLeftLegInputData()->getHip()->setDesiredTwist(0.f);
          getRightLegInputData()->getHip()->setDesiredTwist(0.f);
          if (setLean)
          {
            getLeftLegInputData()->getHip()->setDesiredLean2(0.f);
            getRightLegInputData()->getHip()->setDesiredLean2(0.f);
          }
        }

        if (m_forceTimer < m_parameters.forceTimeOut)
        {
          float totalMass = m_character->getTotalMass();
          applyGetOverWallForceToPart(getSpine()->getSpine2Part(), m_wallHitPos, totalMass, m_comAngVelFromMomentum, 0.8f);
          applyGetOverWallForceToPart(getSpine()->getSpine3Part(), m_wallHitPos, totalMass, m_comAngVelFromMomentum, 0.75f);
          applyGetOverWallForceToPart(getSpine()->getNeckPart(), m_wallHitPos, totalMass, m_comAngVelFromMomentum, 0.5f);
          applyGetOverWallForceToPart(getSpine()->getHeadPart(), m_wallHitPos, totalMass, m_comAngVelFromMomentum, 0.25f);
          //removed clavicle forces as produces a twist that aligns front or back to the wall
          //applyGetOverWallForceToPart(m_leftArm->getClaviclePart(), m_wallHitPos, totalMass, m_comAngVelFromMomentum, 0.8f);
          //applyGetOverWallForceToPart(m_rightArm->getClaviclePart(), m_wallHitPos, totalMass, m_comAngVelFromMomentum, 0.6f);
          //lower part forces help the character over the wall using less overall helper force and help with the pacing of the fallOverWall
          applyGetOverWallForceToPart(getSpine()->getPelvisPart(), m_wallHitPos, totalMass, m_comAngVelFromMomentum);
          applyGetOverWallForceToPart(getSpine()->getSpine0Part(), m_wallHitPos, totalMass, m_comAngVelFromMomentum);
          applyGetOverWallForceToPart(getSpine()->getSpine1Part(), m_wallHitPos, totalMass, m_comAngVelFromMomentum, 0.9f);
          //Removed as character is now a lot less stiff and applying forces to legs is not as needed
          //and looks bad with a loose character.
          //applyGetOverWallForceToPart(m_leftArm->getUpperArm(), m_wallHitPos, totalMass, 0.6f);
          //applyGetOverWallForceToPart(m_rightArm->getUpperArm(), m_wallHitPos, totalMass, 0.4f);
          //applyGetOverWallForceToPart(m_leftLeg->getThigh(), m_wallHitPos, totalMass, 1.0f + m_legAsym);
          //applyGetOverWallForceToPart(m_rightLeg->getThigh(), m_wallHitPos, totalMass, 1.0f - m_legAsym);

          static bool magnet = false;
          // TEST: apply magnet
          if (magnet)
            getSpine()->getPelvisPart()->applyForce(1000.0f*m_pelvisToWall);
        }//if (m_forceTimer< m_parameters::forceTimeOut) 

          static bool useOldSpin = false;
          if (useOldSpin)
          {
            // apply some cheat forces to make the dude roll over the wall instead of perfectly back-flip
            rage::Vector3 torqueVector = getSpine()->getSpine3()->getJointPosition() - getSpine()->getSpine0()->getJointPosition();
            torqueVector.Normalize();
          //test to give barrel roll - almost but really only want when character near horizontal? torqueVector = m_character->m_gUp;
            if (m_negateTorques)
              torqueVector.Negate();

            torqueVector.Scale(m_character->getRandom().GetRanged(m_parameters.minTwistTorqueScale, m_parameters.maxTwistTorqueScale));
            getSpine()->getSpine0Part()->applyTorque(torqueVector);
            getSpine()->getSpine1Part()->applyTorque(torqueVector);

            torqueVector.Scale(0.6f);
            getSpine()->getSpine2Part()->applyTorque(torqueVector);
            getSpine()->getSpine3Part()->applyTorque(torqueVector);
          }
          else
        {
            RollOver(timeStep);
        }

        //Removed as character is now a lot less stiff and applying forces to legs is not as needed
        //and looks bad with a loose character.
        //// TEST: extra leg forces when fwd fall, otherwise legs get trapped as upper body curls around wall
        //rage::Vector3 pelvisToWallDir = pelvisToWall;
        //pelvisToWallDir.Normalize();
        //rage::Vector3 fwd = -m_character->m_COMTM.c;
        //float facingWall = fwd.Dot(pelvisToWallDir);
        //facingWall = rage::Clamp(facingWall, 0.0f, 1.0f);
#if ART_ENABLE_BSPY && 0
        //bspyScratchpad(m_character->getBSpyID(), "FoW", fwd);
        //bspyScratchpad(m_character->getBSpyID(), "FoW", facingWall);
#endif
        //applyGetOverWallForceToPart(getLeftLeg()->getThigh(), m_wallHitPos, totalMass, facingWall*2);
        //applyGetOverWallForceToPart(getRightLeg()->getThigh(), m_wallHitPos, totalMass, facingWall*2);
      }//if (m_FOWState == fow_RollingOverWall)
    }//if (m_hitWall)

    return eCBUTaskComplete;
  }

  void NmRsCBUFallOverWall::RollOver(float timeStep)    
  {
    if (rage::Abs(m_twist) < m_parameters.maxTwist)
    {
      rage::Vector3 torqueVector;
      rage::Vector3 spine0Pos;
      rage::Vector3 spine3Pos;

      spine0Pos = getSpine()->getSpine0()->getJointPosition();
      spine3Pos = getSpine()->getSpine3()->getJointPosition();
      torqueVector = spine3Pos - spine0Pos;
      torqueVector.Normalize(torqueVector);
      float verticalness = rage::Abs(torqueVector.Dot(m_character->m_gUp));
      float twistVel = m_character->m_COMrotvel.Dot(torqueVector);
      float twistDelta = twistVel * timeStep;
      m_twist += twistDelta;
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "FoW:RO", twistDelta);
      bspyScratchpad(m_character->getBSpyID(), "FoW:RO", verticalness);
#endif
      if (m_character->hasCollidedWithWorld(bvmask_JustSpine) && (rage::Abs(m_twist) < m_parameters.maxTwist) && (verticalness < 0.5f))
      {
        float rotVelMagCom = m_character->m_COMrotvel.Dot(torqueVector);
        float rotVelMagHip = getSpine()->getPelvisPart()->getAngularVelocity().Dot(torqueVector);
        float rotVelMag = getSpine()->getSpine3Part()->getAngularVelocity().Dot(torqueVector);
        float rollOverDirection = rage::Sign(m_twist);
        rotVelMag = rage::Abs(rotVelMag);
        rotVelMagHip = rage::Abs(rotVelMagHip);
        rotVelMagCom = rage::Abs(rotVelMagCom);
        if (rotVelMag < 6.f && rotVelMagHip < 6.f && rotVelMagCom < 6.f)
        {
          torqueVector.Scale(m_character->getRandom().GetRanged(m_parameters.minTwistTorqueScale, m_parameters.maxTwistTorqueScale));
          torqueVector *= rollOverDirection;
          getSpine()->getSpine2Part()->applyTorque(torqueVector);
          getSpine()->getSpine3Part()->applyTorque(torqueVector);
          torqueVector.Scale(0.6f);
          getSpine()->getSpine0Part()->applyTorque(torqueVector);
          getSpine()->getSpine1Part()->applyTorque(torqueVector);
          getSpine()->getPelvisPart()->applyTorque(-2.f*torqueVector);
        }//not spinning enough
      }//collided with spine
    }//twist < limit
  }

  void NmRsCBUFallOverWall::applyGetOverWallForceToPart(NmRsGenericPart *part, const rage::Vector3 &topOfWall, float totMass, const rage::Vector3 &angVelCOM, float forceScale) const
  {
    float partMass = getPartMass(m_character->getArticulatedBody()->GetLink(part->getPartIndex()));
    rage::Vector3 force = part->getPosition();
    force.Subtract(topOfWall);

    float distanceFromWallEdge = 0.f;
    if (m_parameters.adaptForcesToLowWall)
    {
      rage::Vector3 projectedPoint = part->getPosition();
      rage::Vector3 end1 = m_parameters.fallOverWallEndA;
      rage::Vector3 end2 = m_parameters.fallOverWallEndB;
      projectOntoLine(end1,end2,projectedPoint);
      rage::Vector3 dist = part->getPosition() - projectedPoint;
      distanceFromWallEdge = dist.Mag();
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "FoW", part->getPartIndex());
      bspyScratchpad(m_character->getBSpyID(), "FoW", distanceFromWallEdge);
#endif
    }

    float distAwayFromRotPoint = force.Mag();
    if ( distAwayFromRotPoint < m_parameters.maxForceDist)
    {
      // force is perpendicular to both edge and part-to-hitPos vector
      force.Normalize();
      force.CrossSafe(m_wallEdge, force);

      float fMag = rage::Clamp(m_parameters.magOfForce*m_forceMultiplier*(partMass/totMass),-5.5f,5.5f);

      float factor = fMag * forceScale;
      if (m_parameters.adaptForcesToLowWall)
      {
        float max = 1.5f;
        float min = 0.2f;
        float clampedval = rage::Clamp((max - distanceFromWallEdge),0.f,max);
        float factorDistanceFromWallEdge = clampedval/max;
        factorDistanceFromWallEdge = min + (1 - min)*factorDistanceFromWallEdge;
        //factor always between min and max
        factor *= factorDistanceFromWallEdge;
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "FoW", clampedval);
        bspyScratchpad(m_character->getBSpyID(), "FoW", factorDistanceFromWallEdge);
#endif
      }
      force.Scale(60.0f * factor);

      // do not apply force considering current angular velocity to avoid too high angular velocity
      if (m_parameters.maxAngVel > 0)
      {
        float currentAngVel = m_wallEdge.Dot(angVelCOM);
        if(rage::Abs(currentAngVel) < m_parameters.maxAngVel) // BBDD: measure change in angular momentum - respond to Agent deceleration PD controller?
        {
          part->applyForce(force);
      }
      }
      // or apply force always
      else
      {
        part->applyForce(force);
      }
    }
  }

  void NmRsCBUFallOverWall::getAngVelCOMFromAngMomentum(rage::Vector3 &angVelCOM) const
  {
#if NM_RIGID_BODY_BULLET //m_characterInertiaAboutComInN is only calculated #if NM_RIGID_BODY_BULLET
      // Calculate angular velocity from angular momentum.
      // NOTE: m_COMrotvel could be used, however
      // calculating from angular momentum proved to be more accurate
      // especially when Agent is spinning about its spine axis due to the approximations
      // that were used to calculate m_COMrotvel.
      angVelCOM = m_character->m_angMom;
      rage::Matrix34 invCharacterInertiaAboutComInN(m_character->m_characterInertiaAboutComInN);
      invCharacterInertiaAboutComInN.Inverse3x3();
      invCharacterInertiaAboutComInN.Transform3x3(angVelCOM);
#else
      angVelCOM = m_character->m_COMrotvel;
#endif
  }

  bool NmRsCBUFallOverWall::getPelvisBelowEdge(float belowEdgeDepth) const
  {
    // Measure if COM is below the edge.
    rage::Vector3 wallHitPos2PelvisPos = m_pelvisPos;
    wallHitPos2PelvisPos -= m_wallHitPos;
    const bool pelvisBelowEdge = (wallHitPos2PelvisPos.Dot(m_character->m_gUp) < (-belowEdgeDepth)) ? true : false;
    return pelvisBelowEdge;
  }

  void NmRsCBUFallOverWall::calculateCommonVariables()
  {
    // extract info about edge<->character spatial relationship
    m_pelvisPos = m_character->m_COM;
    m_projPelvisPos = m_pelvisPos;
    getHorizontalDistancePointFromLine(m_pelvisPos,m_parameters.fallOverWallEndA,m_parameters.fallOverWallEndB,&m_projPelvisPos);
    const float upWall = setHeightTarget(m_projPelvisPos);
    m_character->levelVector(m_projPelvisPos, upWall);
    m_pelvisToWall = m_projPelvisPos - m_pelvisPos;
    m_pelvisPastEdge = m_pelvisToWall.Dot(m_wallNormal) > 0.0f;//mmmmto instead of just being past edge, do we want to be a bit past the edge by say 10cm
#if ART_ENABLE_BSPY && 0
    bspyScratchpad(m_character->getBSpyID(), "FoW", sideOfWallDot);
#endif

    m_amountOfRoll = (m_character->m_COMvelMag + m_character->m_COMrotvelMag) / 2.0f;

    m_forceTimer += (m_FOWState == fow_RollingOverWall) ? m_character->getLastKnownUpdateStep() : 0.0f;

    // The character is overTheWall if the pelvis has passed the edge and is below the edge.
    const float belowEdgeDepth = 0.1f; //BBDD TODO: Expose as a param.
    m_overWallYet = m_pelvisPastEdge && getPelvisBelowEdge(belowEdgeDepth);

    // Check how upright the character is.
    m_upNess = (m_character->m_COMTM.b).Dot(m_character->m_gUp);

    // COM angular velocity and its edge component, determine whether character is rolling back.
    getAngVelCOMFromAngMomentum(m_comAngVelFromMomentum);
    m_comAngVelEdgeComponent = (m_wallEdge.Dot(m_comAngVelFromMomentum));
    m_rollingBack = (m_comAngVelEdgeComponent < 0.0f && rage::Abs(m_comAngVelEdgeComponent) > m_parameters.rollingBackThr) ? true : false;
  }

  //Predict the angVel the character will have about the wall edge caused by it's collision with the wall.
  // We may get more accurate with this i.e. measure the angular momentum about the wall edge
  //  i.e. for a solid railing assume all parts below the edge will have zero velocity after impact 
  //  and those parts above the edge will convert their linear velocity to angular velocity about the edge.
  // For now we just take a measure of the spine and neck velocity along the wall normal.
  float NmRsCBUFallOverWall::rollingPotentialOnImpact()
  {
    // Upper body (no arms) linear velocities.
    rage::Vector3 linearVel = getSpine()->getSpine1Part()->getLinearVelocity();
    linearVel += getSpine()->getSpine2Part()->getLinearVelocity();
    linearVel += getSpine()->getSpine3Part()->getLinearVelocity();
    linearVel += getSpine()->getNeckPart()->getLinearVelocity();
    linearVel += getSpine()->getHeadPart()->getLinearVelocity();
    return (-m_wallNormal.Dot(linearVel)*0.2f);//0.2 = 1/numParts
  }

  void NmRsCBUFallOverWall::feedbackLogicTick()
  {
    // Feedback logic.
    switch (m_FOWState)
    {
      case fow_OverTheWall:
        {
          if (m_parameters.distanceToSendSuccessMessage < 0.0f || m_pelvisToWall.Mag() > m_parameters.distanceToSendSuccessMessage)
          {
            sendSuccessFeedback();
          }
        }
        break;

      case fow_StuckOnWall:
      case fow_RollingBack:
      case fow_Aborted:
        {
          sendFailureFeedback();
        }
        break;

      case fow_ApproachingWall:
      case fow_RollingOverWall:
      default:
        break;
    } //< End of feedback logic switch.
  }

  void NmRsCBUFallOverWall::stateLogicTick()
  {
    fallOverWallState FOWStatePrevious = m_FOWState;

    switch (m_FOWState)
    {
      case fow_ApproachingWall:
        {
          if (m_hitWall)
          {
            if (m_rollingPotentialOnImpact > m_parameters.rollingPotential) // enough upper body speed to roll
            {
              m_FOWState = fow_RollingOverWall;
            }
            else
            {
              m_FOWState = fow_Aborted;
            }
          }
        }
        break; //< End of fow_ApproachingWall.

      case fow_RollingOverWall:
        {
          if (m_overWallYet)//m_pelvisPastEdge && getPelvisBelowEdge(belowEdgeDepth)
          {
            m_FOWState = fow_OverTheWall;
          }
          else if (m_amountOfRoll < 0.2f)
          {
            m_FOWState = fow_StuckOnWall;
          }
          else if ((m_rollingBack || m_pelvisToWall.Mag() > 0.5f) && !m_pelvisPastEdge)
          {
            m_FOWState = fow_RollingBack;
          }
        }
        break; //< End of fow_RollingOverWall.

      case fow_StuckOnWall:
        {
          if (m_overWallYet)
          {
            m_FOWState = fow_OverTheWall;
          }
          else if ((m_rollingBack || m_pelvisToWall.Mag() > 0.5f) && !m_pelvisPastEdge)
          {
            m_FOWState = fow_RollingBack;
          }
        }
        break; //< End of fow_StuckOnWall.
      case fow_Aborted:
      case fow_RollingBack:
      case fow_OverTheWall:
      default:
        break;
    } //< End of switch.

    // Send state feedback only when state changed.
    if (m_FOWState != FOWStatePrevious)
    {
      sendStateFeedback(m_FOWState);
    }
  }

  void NmRsCBUFallOverWall::projectOntoLine(rage::Vector3 &endOne, rage::Vector3 &endTwo, rage::Vector3 &point) const
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

#if useNewFallOverWall
  void NmRsCBUFallOverWall::applyArmIK(float timeStep){
    //reinitialize
#if useNewFallOverWall
    m_isTotallyBack = false;
#endif//useNewFallOverWall

    /******************************/
    /*  set target for the IK     */
    /******************************/
    rage::Vector3 lefthandTarget ;
    rage::Vector3 righthandTarget;
    //phase 1 : hands are on the initial side of the wall
    //max distance from the wall to start am arm IK
    float distanceTostart = 2.f;
    rage::Vector3 leftHandPos = getLeftArm()->getHand()->getPosition();
    rage::Vector3 rightHandPos = getRightArm()->getHand()->getPosition();
    rage::Vector3 leftIntersection;
    rage::Vector3 rightIntersection;
    float distanceLeftHand = getHorizontalDistancePointFromLine(leftHandPos,m_parameters.fallOverWallEndA,m_parameters.fallOverWallEndB,&leftIntersection);
    float distanceRightHand = getHorizontalDistancePointFromLine(rightHandPos,m_parameters.fallOverWallEndA,m_parameters.fallOverWallEndB,&rightIntersection);

    rage::Vector3 leftHandToWall = leftIntersection - leftHandPos;
    m_character->levelVector(leftHandToWall);
    leftHandToWall.Normalize();
    rage::Vector3 rightHandToWall = rightIntersection -rightHandPos;
    m_character->levelVector(rightHandToWall);
    rightHandToWall.Normalize();

    //test to know if hand already passed the wall (phase 2)
    //in that case setHandsTarget will fail to find the target

    // extract info about edge<->character spatial relationship
    bool leftHandPassed = leftHandToWall.Dot(m_wallNormal) > 0.0f;
    bool rightHandPassed = rightHandToWall.Dot(m_wallNormal) > 0.0f;
    //phase 1 : try to reach the wall in the direction of the velocity 
    bool leftHandHasTarget= false;
    bool rightHandHasTarget= false;
    float armtwist = 0.f;
    if (!leftHandPassed||!rightHandPassed)
    {
      setHandsTarget(&lefthandTarget, &leftHandHasTarget, &righthandTarget, &rightHandHasTarget, distanceTostart,&armtwist);
    }

    //phase 2 : try to reach at the nearest point
    float heightWall = 1.2f;//average value to add to distance to start
    float distanceToFinish = heightWall +distanceTostart;
    rage::Vector3 upVector  = m_character->getUpVector();
    if(rightHandPassed&&(distanceRightHand<distanceToFinish)){

      righthandTarget = rightIntersection;
      rightHandHasTarget = isIntheWallEdge(m_parameters.fallOverWallEndA,m_parameters.fallOverWallEndB,&righthandTarget);
      if (rightHandHasTarget)
      {
        float Min = rage::Min(m_parameters.fallOverWallEndA.Dot(upVector),m_parameters.fallOverWallEndB.Dot(upVector));
        m_character->levelVector(righthandTarget,Min);
      }
    }
    if(leftHandPassed&&(distanceLeftHand<distanceToFinish)){
      lefthandTarget = leftIntersection;
      leftHandHasTarget = isIntheWallEdge(m_parameters.fallOverWallEndA,m_parameters.fallOverWallEndB,&lefthandTarget);
      if (leftHandHasTarget)
      {
        float Min = rage::Min(m_parameters.fallOverWallEndA.Dot(upVector),m_parameters.fallOverWallEndB.Dot(upVector));
        m_character->levelVector(lefthandTarget,Min);
      }
    }

    /******/
    /* IK */
    /******/

    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    bool testVelForLiedOnTheGround = m_character->m_COMvelMag<m_parameters.minVelForLiedOnTheGround;
    bool liedOntheGround = (!dynamicBalancerTask->isActive())&& m_isOnTheGround&&testVelForLiedOnTheGround;
#if ART_ENABLE_BSPY &&FallOverWallBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "FoW.", testVelForLiedOnTheGround);
    bspyScratchpad(m_character->getBSpyID(), "FoW.", liedOntheGround);
#endif
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    if (rightHandHasTarget&&!liedOntheGround)
    {
      if (catchFallTask->isActive())
      {
        catchFallTask->deactivate();//mmmmtodo why is catchfall turned off when it is before fow in tick order?
      }
      callMaskedEffectorFunctionFloatArg(m_character, bvmask_ArmRight, 1.f, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
      //IK for right arm
      //Compensation of angular velocity and linear velocity of the root of the IK
      rage::Vector3 TargetVel (0,0,0);
      rage::Vector3 rootPos = getRightArm()->getClaviclePart()->getPosition();
      rage::Vector3 rootLinVel = getRightArm()->getClaviclePart()->getLinearVelocity();
      rage::Vector3 rootAngVel = getRightArm()->getClaviclePart()->getAngularVelocity();
      rage::Vector3 target = righthandTarget;
      if(!m_isTotallyBack)
      {
        //set of height of the target
        float upOfrightTarget = setHeightTarget(righthandTarget);
        m_character->levelVector(righthandTarget,upOfrightTarget);
        target = m_character->targetPosition(righthandTarget,TargetVel, rootPos, rootLinVel, rootAngVel,timeStep);
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw &&0
        m_character->bspyDrawPoint(target, 0.1f, rage::Vector3(1,1,0));
#endif
      }
      if(! m_character->hasCollidedWithWorld(bvmask_ArmRight))
      {      
        getRightArm()->getClavicle()->setStiffness(15.f,1.f);
        getRightArm()->getShoulder()->setStiffness(15.f,1.f);
        getRightArm()->getElbow()->setStiffness(15.f,1.f);
        getRightArm()->getWrist()->setStiffness(15.f,1.f);
      }
      m_character->rightArmIK(target, armtwist);

      rage::Vector3 normalTarget = m_wallNormal;
      if (target.Dot(upVector)<getRightArm()->getHand()->getPosition().Dot(upVector))
      {
        normalTarget.Cross(m_wallEdge,m_wallNormal);
        if(normalTarget.Dot(upVector)<0.f){
          normalTarget.Negate();
        }
      }
      if (rightHandPassed)
      {
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "FoW.rightArmIK",rightHandPassed);
#endif
        normalTarget.Negate();
      }
      m_character->rightWristIK(target,normalTarget);
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "FoW.rightArmIK", target);
      m_character->bspyDrawPoint(righthandTarget, 0.1f, rage::Vector3(1,0,0));
      m_character->bspyDrawLine(righthandTarget,righthandTarget + normalTarget*0.2f, rage::Vector3(1,0,0));
      bspyScratchpad(m_character->getBSpyID(), "FoW.rightArmIK", righthandTarget);
      bspyScratchpad(m_character->getBSpyID(), "FoW.rightArmIK", normalTarget);
#endif
    }
    if (leftHandHasTarget && !liedOntheGround)
    {
      if (catchFallTask->isActive())
      {
        catchFallTask->deactivate();//mmmmtodo why is catchfall turned off when it is before fow in tick order?
      }
      callMaskedEffectorFunctionFloatArg(m_character, bvmask_ArmLeft, 1.f, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);
      //Compensation of angular velocity and linear velocity of the root of the IK
      rage::Vector3 TargetVel (0,0,0);
      rage::Vector3 rootPos = getLeftArm()->getClaviclePart()->getPosition();
      rage::Vector3 rootLinVel = getLeftArm()->getClaviclePart()->getLinearVelocity();
      rage::Vector3 rootAngVel = getLeftArm()->getClaviclePart()->getAngularVelocity();
      rage::Vector3 target = lefthandTarget;
      if(!m_isTotallyBack)
      {
        //set height of target : try to reach wall edge
        float upOfleftTarget = setHeightTarget(lefthandTarget);
        m_character->levelVector(lefthandTarget,upOfleftTarget);
        target = m_character->targetPosition(lefthandTarget,TargetVel, rootPos, rootLinVel, rootAngVel,timeStep);
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw && 0
        m_character->bspyDrawPoint(target, 0.1f, rage::Vector3(1,1,0));
#endif
      }
      //IK for left arm
      if(! m_character->hasCollidedWithWorld(bvmask_ArmLeft))
      {      
        getLeftArm()->getClavicle()->setStiffness(15.f,1.f);
        getLeftArm()->getShoulder()->setStiffness(15.f,1.f);
        getLeftArm()->getElbow()->setStiffness(15.f,1.f);
        getLeftArm()->getWrist()->setStiffness(15.f,1.f);
      }

      m_character->leftArmIK(target,armtwist);
      rage::Vector3 normalTarget = m_wallNormal;
      if (target.Dot(upVector)<getLeftArm()->getHand()->getPosition().Dot(upVector))
      {
        normalTarget.Cross(m_wallEdge,m_wallNormal);
        if(normalTarget.Dot(upVector)<0.f){
          normalTarget.Negate();
        }
      }
      if (leftHandPassed)
      {
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "FoW.leftArmIK",leftHandPassed);
#endif
        normalTarget.Negate();
      }
      m_character->leftWristIK(target,normalTarget);
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "FoW.leftArmIK", target);
      m_character->bspyDrawPoint(lefthandTarget, 0.1f, rage::Vector3(1,0,0));
      m_character->bspyDrawLine(lefthandTarget, lefthandTarget + normalTarget*0.2f, rage::Vector3(1,0,0));
      bspyScratchpad(m_character->getBSpyID(), "FoW.leftArmIK", lefthandTarget);
      bspyScratchpad(m_character->getBSpyID(), "FoW.leftArmIK", normalTarget);
#endif
    }
  }
#endif//useNewFallOverWall

#if useNewFallOverWall
  //WALLL MUST BE VERTICAL
  //will try to grab the wall edge in the direction of velocity of COM but if try too reach behind him, clamp the direction
  //if try too reach on the other side (if left hand try to reach too much on his right side, clamp the direction
  void NmRsCBUFallOverWall::setHandsTarget(rage::Vector3 *lefthandTarget, bool *leftHandHasTarget, rage::Vector3 *righthandTarget, bool *rightHandHasTarget, float distanceTostart, float *armTwist){
    *armTwist = 0.f;

    rage::Vector3 UpVector= m_character->getUpVector();

    //distance between hand and wallEdge
    rage::Vector3 leftHandPos = getLeftArm()->getHand()->getPosition();
    rage::Vector3 rightHandPos = getRightArm()->getHand()->getPosition();
    rage::Vector3 horVelCOM = m_character->m_COMvel;
    m_character->levelVector(horVelCOM);

    rage::Vector3 side = getLeftArm()->getShoulder()->getJointPosition() - getRightArm()->getShoulder()->getJointPosition();
    m_character->levelVector(side);
    side.Normalize();
    rage::Vector3 front;
    front.Cross(side, UpVector);
    rage::Vector3 endOfProbe;

    if(horVelCOM.Mag()>0.5f){
      horVelCOM.Normalize();
      endOfProbe = horVelCOM;
      endOfProbe.Scale(distanceTostart);
      if (m_neverHitWall)
      {
        defaultEndOfProbe = endOfProbe;
      }
    }
    else
    {
      if (m_hitWall||(!m_neverHitWall))
      {
        endOfProbe = defaultEndOfProbe;
      }
      else
      {
        endOfProbe = front;
        endOfProbe.Scale(distanceTostart);
        if (m_wallNormal.Dot(front)>0.f)
        {
          endOfProbe.Negate();
        }  
      }
    }


    //probe send horizontally from the height of the lowest feet
    float ZOfProbe = rage::Min(getLeftLeg()->getFoot()->getPosition().Dot(UpVector),getRightLeg()->getFoot()->getPosition().Dot(UpVector));

    rage::Vector3 probeVector = getSpine()->getPelvisPart()->getPosition();
    probeVector.SetZ(ZOfProbe);
    rage::Vector3 hitPos;
    bool didItHit = didHitWall(probeVector,probeVector+endOfProbe,m_parameters.fallOverWallEndA, m_parameters.fallOverWallEndB,&hitPos);
    //didItHit = false;
    if (didItHit)
    {
      rage::Vector3 dirOfProbe = endOfProbe;
      dirOfProbe.Normalize();

      rage::Vector3 frontN = front;
      float angleWithWall = rage::AcosfSafe(frontN.Dot(m_wallNormal));

      float limitBetweenBackOrFront = PI*90/180;
      bool isFrontOfThewall = true;
      if (angleWithWall < limitBetweenBackOrFront)
      {
        isFrontOfThewall = false;
      }

      float limitToTest = PI*10/180;
      rage::Vector3 angVel = getSpine()->getSpine3Part()->getAngularVelocity();
      float horAngVel = angVel.Dot(UpVector);
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "FoW.", horAngVel);
#endif
      if (angleWithWall < (limitBetweenBackOrFront + limitToTest)&&angleWithWall > (limitBetweenBackOrFront - limitToTest))
      {
        float difAngle = angleWithWall - limitBetweenBackOrFront;
        float factorVelAngle = horAngVel*5.f;
        if (difAngle > 0.f && factorVelAngle>difAngle)
        {
          //change from front to back
          isFrontOfThewall = false;
        }
        else if (difAngle < 0.f && factorVelAngle<difAngle){
          //change from back to front
          isFrontOfThewall = true;
        }
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "FoW.nearLimit", difAngle);
        bspyScratchpad(m_character->getBSpyID(), "FoW.nearLimit", horAngVel);
#endif
      }

      rage::Vector3 rightTarget = hitPos;
      rage::Vector3 leftTarget = hitPos;
      rage::Vector3 toLeft = m_wallEdge;
      toLeft.Scale(m_parameters.reachDistanceFromHitPoint);

      if (isFrontOfThewall)
      {
        leftTarget.Add(toLeft);
        rightTarget.Subtract(toLeft);

      }
      else
      {
        leftTarget.Subtract(toLeft);
        rightTarget.Add(toLeft);
      }
      //test to be sure targets are in the wall edge
      if (didItHit)
      {
        *rightHandHasTarget = isIntheWallEdge(m_parameters.fallOverWallEndA,m_parameters.fallOverWallEndB,&rightTarget);
        *leftHandHasTarget = isIntheWallEdge(m_parameters.fallOverWallEndA,m_parameters.fallOverWallEndB,&leftTarget);
      }
      //////
      *righthandTarget=rightTarget;
      *lefthandTarget=leftTarget;
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "FoW.", isFrontOfThewall);
#endif

      float limit = PI*m_parameters.angleTotallyBack/180;
      rage::Vector3 sideN = side;
      float angleShoulderWall = rage::AcosfSafe(sideN.Dot(m_wallNormal));
      if ((!isFrontOfThewall)&&(angleShoulderWall>(PI/2.f-limit))&&(angleShoulderWall<(PI/2.f+limit))&&m_neverHitWall)
      {
        m_isTotallyBack = true;
        //particular case : totally back, just try to put his hands behind his buttocks , overwrite all

        float sideValue = 0.3f;
        float backValue = 0.25f;
        float verticalValue = 0.f;
        rage::Matrix34 pelvisMat;
        getSpine()->getPelvisPart()->getMatrix(pelvisMat);
        rage::Vector3 rightTarget = getSpine()->getPelvisPart()->getPosition();//pelvis
        rage::Vector3 frontV = pelvisMat.c;
        m_character->levelVector(frontV);
        frontV.Normalize();
        rightTarget.AddScaled(frontV,backValue);
        rage::Vector3 vertV = pelvisMat.a;
        vertV.Normalize();
        rightTarget.AddScaled(vertV,verticalValue);
        rage::Vector3 leftTarget = rightTarget;
        rage::Vector3 sideV = pelvisMat.b;//direction : right to left
        m_character->levelVector(sideV);
        sideV.Normalize();
        rightTarget.AddScaled(sideV,-sideValue);
        leftTarget.AddScaled(sideV,sideValue);
        *righthandTarget=rightTarget;
        *lefthandTarget=leftTarget;
        *armTwist=PI/2.f;
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "FoW.test", *armTwist);
#endif
      }
    }

  }

#endif//useNewFallOverWall

#if useNewFallOverWall
  //do not considerate vertical value 
  bool NmRsCBUFallOverWall::didHitWall(const rage::Vector3 &startProbe,const rage::Vector3 &endProbe,const rage::Vector3 &startWall,const rage::Vector3 &endWall, rage::Vector3 *hitPos){
    /**************************************/
    /*    angle between probe and wall    */
    /**************************************/
    rage::Vector3 probe = endProbe - startProbe;
    m_character->levelVector(probe);
    rage::Vector3 wall = endWall - startWall;
    m_character->levelVector(wall);
    rage::Vector3 probeN = probe;
    probeN.Normalize();
    rage::Vector3 wallN = wall;
    wallN.Normalize();

    rage::Vector3 intersectionPoint;
    getHorizontalDistancePointFromLine(startProbe,m_parameters.fallOverWallEndA,m_parameters.fallOverWallEndB,&intersectionPoint);
    rage::Vector3 intersectionVector = intersectionPoint - startProbe;
    m_character->levelVector(intersectionVector);
    rage::Vector3 intersectionVectorN =intersectionVector;
    intersectionVectorN.Normalize();
    float cosAngle = probeN.Dot(intersectionVectorN);
    /***************/
    /*    hitpos   */
    /***************/
    float distToWall = rage::Abs(intersectionVector.Mag())/cosAngle;//in meter
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
    bspyScratchpad(m_character->getBSpyID(), "FoW.diditWall", distToWall);     
#endif

    rage::Vector3 hitPostemp = startProbe;
    hitPostemp += probeN*distToWall;
    *hitPos = hitPostemp;

    /**************/
    /* didItWall  */
    /**************/
    bool inProbe = false;
    bool inWall = false;

    rage::Vector3 posToStartProbe = startProbe - hitPostemp;
    m_character->levelVector(posToStartProbe);
    rage::Vector3 posToStartProbeN = posToStartProbe;
    posToStartProbeN.Normalize();
    rage::Vector3 posToEndProbe = endProbe - hitPostemp;
    m_character->levelVector(posToEndProbe);
    rage::Vector3 posToEndProbeN = posToEndProbe;
    posToEndProbeN.Normalize();  
    float scalarProbe = posToStartProbeN.Dot(posToEndProbeN);
    //parallel inverse sens
    if (scalarProbe<(-0.999f))
    {
      inProbe = true;
    }

    rage::Vector3 posToStart = startWall - hitPostemp;
    rage::Vector3 posToEnd = endWall - hitPostemp;
    m_character->levelVector(posToStart);
    m_character->levelVector(posToEnd);
    rage::Vector3 posToStartN = posToStart;
    posToStartN.Normalize();
    rage::Vector3 posToEndN = posToEnd;
    posToEndN.Normalize();
    float scalar = posToStartN.Dot(posToEndN);
    //parallel
    if (scalar<-0.999f)
    {
      inWall = true;
    }
    bool result = inProbe && inWall;


    /******************/
    /* result && BSpy */
    /******************/

    rage::Vector3 color (0,0,1);
    if (result)
    {
      color.SetX(1);
      color.SetZ(0);
    }

#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
    m_character->bspyDrawLine(startProbe, endProbe,color);
#endif
    return result;

  }
#endif//useNewFallOverWall

  float NmRsCBUFallOverWall::setHeightTarget(const rage::Vector3 &target)
  {
    rage::Vector3 upVector = m_character->getUpVector();
    rage::Vector3 intersectPos = target;    
    m_character->levelVector(intersectPos);
    rage::Vector3 End1 = m_parameters.fallOverWallEndA;
    m_character->levelVector(End1);
    float distToEnd1 = (intersectPos - End1).Mag();
    rage::Vector3 wall = m_parameters.fallOverWallEndB - m_parameters.fallOverWallEndA;
    m_character->levelVector(wall);

    float factor = 0.0f;
    if(wall.Mag() != 0.0f)
      factor = (m_parameters.fallOverWallEndB.Dot(upVector) - m_parameters.fallOverWallEndA.Dot(upVector))/wall.Mag();
    return m_parameters.fallOverWallEndA.Dot(upVector)+distToEnd1*factor;
  }

#if useNewFallOverWall
  bool NmRsCBUFallOverWall::isIntheWallEdge(const rage::Vector3 &startWall,const rage::Vector3 &endWall,rage::Vector3 *hitPos){
    rage::Vector3 PointToEnd = endWall - *hitPos;
    rage::Vector3 PointToStart = startWall - *hitPos;
    m_character->levelVector(PointToStart);
    m_character->levelVector(PointToEnd);
    PointToEnd.Normalize();
    PointToStart.Normalize();
    if (PointToEnd.Dot(PointToStart)<-0.999f)
    {//hitpos is inside
      return true;
    }
    else if (PointToEnd.Dot(PointToStart)>0.999f){//hitpos is outside the wall edge
      PointToEnd = endWall - *hitPos;
      PointToStart = startWall - *hitPos;
      m_character->levelVector(PointToStart);
      m_character->levelVector(PointToEnd);
      float minFromWall1 = PointToEnd.Mag();
      float minFromWall2 = PointToStart.Mag();
      float minFromWall = rage::Min(minFromWall1,minFromWall2);
      float minreachDist = m_parameters.minReachDistanceFromHitPoint;

      if (minFromWall< (m_parameters.reachDistanceFromHitPoint - minreachDist))
      {
        if(minFromWall2<minFromWall1){
          *hitPos = startWall;

        }
        else
        {
          *hitPos = endWall;
        }
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "FoW.isIntheWallEdge.targetmodified", *hitPos);
#endif
        return true;
      }
      else
      {//too far from wall edge
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
        bspyScratchpad(m_character->getBSpyID(), "FoW.isIntheWallEdge.targettoofar", *hitPos);
#endif
        hitPos = NULL;
        return false;
      }

    }
    else
    {//normally never happened, hitpos is not in the line between by the two points of the wall
#if ART_ENABLE_BSPY && FallOverWallBSpyDraw
      m_character->bspyDrawPoint(*hitPos,0.3f,rage::Vector3(1,0,1));
      bspyScratchpad(m_character->getBSpyID(), "FoW.isIntheWallEdge.bug", *hitPos);
#endif
      hitPos = NULL;
      return false;
    }
  }
#endif//useNewFallOverWall
  void NmRsCBUFallOverWall::sendSuccessFeedback()
  {
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      feedback->m_agentID = m_character->getID();
      feedback->m_argsCount = 0;
      strcpy(feedback->m_behaviourName, NMFallOverWallFeedbackName);
      feedback->onBehaviourSuccess();
    }
  }

  void NmRsCBUFallOverWall::sendFailureFeedback()
  {
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      feedback->m_agentID = m_character->getID();
      feedback->m_argsCount = 0;
      strcpy(feedback->m_behaviourName, NMFallOverWallFeedbackName);
      feedback->onBehaviourFailure();
    }
  }

  void NmRsCBUFallOverWall::sendStateFeedback(int currentState)
  {
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      feedback->m_agentID = m_character->getID();
      feedback->m_argsCount = 1;
      ART::ARTFeedbackInterface::FeedbackUserdata data;
      data.setInt(currentState);
      feedback->m_args[0] = data;
      strcpy(feedback->m_behaviourName, NMFallOverWallStateFeedbackName);
      feedback->onBehaviourEvent();
    }
  }

  //returns intersectionPoint  
  float NmRsCBUFallOverWall::getHorizontalDistancePointFromLine(const rage::Vector3 &point, const rage::Vector3 &pointline1,const rage::Vector3 &pointline2,rage::Vector3 *intersectionPoint)
  {
    rage::Vector3 pointH;
    rage::Vector3 pl1 =pointline1;
    rage::Vector3 pl2 =pointline2;
    float toLevel = point.Dot(m_character->getUpVector());
    m_character->levelVector(pl1,toLevel);
    m_character->levelVector(pl2,toLevel);
    rage::Vector3 line = pl1-pl2;
    float dist = line.Mag();
    if (dist!=0.f)
    {

      //one of the factor is equal to zero
      float factor1 = (point.GetX()-pl1.GetX())*(pl2.GetX()-pl1.GetX());
      float factor2 = (point.GetY()-pl1.GetY())*(pl2.GetY()-pl1.GetY());
      float factor3 = (point.GetZ()-pl1.GetZ())*(pl2.GetZ()-pl1.GetZ());
      float factor = (factor1 +factor2+factor3)/(dist*dist);
      pointH.SetX(pl1.GetX() + factor*(pl2.GetX()-pl1.GetX()));
      pointH.SetY (pl1.GetY() + factor*(pl2.GetY()-pl1.GetY()));
      pointH.SetZ(pl1.GetZ() + factor*(pl2.GetZ()-pl1.GetZ()));
      m_character->levelVector(pointH,point.Dot(m_character->getUpVector()));
      intersectionPoint->SetX(pointH.GetX());
      intersectionPoint->SetY(pointH.GetY());
      intersectionPoint->SetZ(pointH.GetZ());
      return (*intersectionPoint - point).Mag();
    }
    else
    {//pointline1 is the same as pointline2, not a line
      intersectionPoint = NULL;
      return 0.f;
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUFallOverWall::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.fallOverWallEndA, true);
    bspyTaskVar(m_parameters.fallOverWallEndB, true);

    bspyTaskVar(m_parameters.bodyDamping, true);
    bspyTaskVar(m_parameters.bodyStiffness, true);
    bspyTaskVar(m_parameters.magOfForce, true);
    bspyTaskVar(m_parameters.forceTimeOut, true);
    bspyTaskVar(m_parameters.forceAngleAbort, true);
    bspyTaskVar(m_parameters.minLegHeight, true);
    bspyTaskVar(m_parameters.maxDistanceFromPelToHitPoint, true);
    bspyTaskVar(m_parameters.minTwistTorqueScale, true);
    bspyTaskVar(m_parameters.maxTwistTorqueScale, true);
    bspyTaskVar(m_parameters.maxTwist, true);
    bspyTaskVar(m_parameters.moveArms, true);
    bspyTaskVar(m_parameters.moveLegs, true);
    bspyTaskVar(m_parameters.bendSpine, true);
    bspyTaskVar(m_parameters.leaningAngleThreshold, true);
    bspyTaskVar(m_parameters.angleDirWithWallNormal, true);
    bspyTaskVar(m_parameters.maxAngVel, true);
    bspyTaskVar(m_parameters.adaptForcesToLowWall, true);
    bspyTaskVar(m_parameters.maxWallHeight, true);
    bspyTaskVar(m_parameters.rollingBackThr, true);
    bspyTaskVar(m_parameters.rollingPotential, true);

    bspyTaskVar(m_wallNormal, false);
    bspyTaskVar(m_wallEdge, false);
    bspyTaskVar(m_wallHitPos, false);
    bspyTaskVar(m_projPelvisPos, false);
    bspyTaskVar(m_pelvisToWall, false);
    bspyTaskVar(m_pelvisToWall.Mag(), false);

    bspyTaskVar(m_blean1, false);
    bspyTaskVar(m_blean2, false);
    bspyTaskVar(m_forceMultiplier, false);

    bspyTaskVar(m_twist, false);

    bspyTaskVar(m_hitWall, false);
    bspyTaskVar(m_pelvisPastEdge, false);
    bspyTaskVar(m_overWallYet, false);
    bspyTaskVar(m_rollingBack, false);
    bspyTaskVar(m_negateTorques, false);
    bspyTaskVar(m_upNess, false);
    bspyTaskVar(m_forceTimer, false);

    bspyTaskVar(m_amountOfRoll, false);

    bspyTaskVar(m_comAngVelFromMomentum, false);
    bspyTaskVar(m_comAngVelEdgeComponent, false);
    bspyTaskVar(m_rollingPotentialOnImpact, false);
//end bart
    static const char* fow_state_names[] =
    {
#define FOW_ENUM_STATE_NAMES(_name) #_name,
      FOW_STATE(FOW_ENUM_STATE_NAMES)
#undef FOW_ENUM_STATE_NAMES
    };
    bspyTaskVar_StringEnum(m_FOWState, fow_state_names, false);

#if useNewFallOverWall
    bspyTaskVar(m_parameters.distanceToSendSuccessMessage, true);
    bspyTaskVar(m_parameters.useArmIK, true);
    bspyTaskVar(m_parameters.minVelForLiedOnTheGround, true);
    bspyTaskVar(m_parameters.reachDistanceFromHitPoint, true);
    bspyTaskVar(m_parameters.minReachDistanceFromHitPoint, true);
    bspyTaskVar(m_parameters.angleTotallyBack, true);
    bspyTaskVar(m_neverHitWall, false);
    bspyTaskVar(defaultEndOfProbe, false);
#endif//useNewFallOverWall
  }
#endif // ART_ENABLE_BSPY
}

