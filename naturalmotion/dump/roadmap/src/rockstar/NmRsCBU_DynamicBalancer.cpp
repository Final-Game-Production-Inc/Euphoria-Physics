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
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_DBMFootPlacement.h"
#include "NmRsCBU_DBMPelvisControl.h"
#include "NmRsCBU_DBMBalanceSolve.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_BraceForImpact.h"
#include "NmRsCBU_CatchFall.h"
#include "NmRsCBU_StaggerFall.h"
#include "NmRsCBU_FallOverWall.h"
#include "NmRsCBU_Teeter.h"
#include "NmRsCBU_Grab.h"

#include "ART/ARTFeedback.h"

namespace ART
{
#define LOOSER_ANKLES
#define LOOSER_TAPERED_KNEE_STRENGTH

  static const float cFootProbeRayLower       = 1.0f;   // how far down we test to find terrain under the foot projected onto the height of the knee, in metres

#define NM_RS_WAIT_AND_ZERO_TASK(taskHandle) \
  if (taskHandle) \
  { \
    rage::sysTaskManager::Wait(taskHandle); \
    taskHandle = 0; \
  }

  NmRsCBUDynamicBalancer::NmRsCBUDynamicBalancer(ART::MemoryManager* services) : CBUTaskBase(services, bvid_dynamicBalancer),
    m_failType(balFail_General),
    m_footTaskHandle(0),
    m_pelvisTaskHandle(0),
    m_solveTaskHandle(0),
    m_deactivateMe(false)
  {
    initialiseCustomVariables();
  }

  NmRsCBUDynamicBalancer::~NmRsCBUDynamicBalancer()
  {
    // wait for any tasks outstanding before we throw away 
    // their data!
    NM_RS_WAIT_AND_ZERO_TASK(m_footTaskHandle);
    NM_RS_WAIT_AND_ZERO_TASK(m_pelvisTaskHandle);
    NM_RS_WAIT_AND_ZERO_TASK(m_solveTaskHandle);
  }

  // called when the behaviour is activated
  void NmRsCBUDynamicBalancer::initialiseData()
  {
    m_leftLegStiffness = 12.f;
    m_rightLegStiffness = 12.f;
    m_leftLegSwingDamping = 1.f;
    m_opposeGravityLegs = 1.f;
    m_opposeGravityAnkles = 1.f;
    m_leanAcc = 0.0f;
    m_hipLeanAcc = 0.5f;
    m_leanAccMax = 5.0f;
    m_resistAcc = 0.5;
    m_resistAccMax = 3.0f;
    m_footSlipCompOnMovingFloor = true;

    m_rightLegSwingDamping = 1.f;
    m_failed = false;
    m_failType = balFail_General;
    m_balanceInstability = 0.f;
    m_upOffset.Set(0,0,0);
    m_upOffsetForce.Set(0,0,0);
    m_upOffsetHips.Set(0,0,0);

    m_tickPhase = eDynBalTPInvalid;
  }

  // called when the behaviour is initialised (on addToScene)
  void NmRsCBUDynamicBalancer::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_stepHeight = 0.1f;
#if NM_STEP_UP
    m_stepHeightInc4Step = 0.1f;
#endif//#if NM_STEP_UP

    m_hipPitch = 0.0f;
    m_hipRoll = 0.0f;
    m_hipYaw = 0.0f;
    m_useCustomTurnDir = false;
    m_customTurnDir.Zero();
    m_legStraightnessModifier = 0.0f;
    m_fallMult = 1.f;
    m_fallType = 0;
    m_fallReduceGravityComp = false;
    m_leanAgainstVelocity = 0.f;
    m_deactivateMe = false;
    m_footState.m_maxSteps = 100;
    m_kneeStrength = 1.0f;
    m_taperKneeStrength = true;
    m_minKneeAngle = -0.5f;
    m_stableSuccessMinimumRotSpeed = 0.4f;
    m_stableSuccessMinimumLinSpeed = 0.4f;
    m_balanceInstability = 0.f;
    m_roPacket.m_giveUpThreshold = 0.6f;
    m_roPacket.m_flatterSwingFeet = false;
    m_roPacket.m_movingFloor = false;
	  m_roPacket.m_avoidCar = false;
    m_roPacket.m_flatterStaticFeet = false;
    m_roPacket.m_teeter = false;
    m_roPacket.m_fallToKnees = false;
    static float timeTakenForStep = 0.3f;
    m_roPacket.m_timeTakenForStep = timeTakenForStep;
    m_roPacket.m_changeStepTime = -1.5f;//i.e don't change step after a certain time if airborne
    m_giveUpHeight = 0.5f;
#if DYNBAL_GIVEUP_RAMP
    m_giveUpHeightEnd = 0.5f;
    m_giveUpThresholdEnd = 0.6f;
    m_giveUpRampDuration = -1.0f;
    m_leanToAbort = 0.6f;
#endif
    m_stepClampScale = 1.f;
    m_stepClampScaleIn = 1.f;
    m_stepClampScaleVariance = 0.f;
    m_balanceTimeIn = 0.2f;
    m_balanceTimeVariance = 0.f;
    m_timer = 0.f;
    m_maximumBalanceTime = 50.f;
    m_balanceTimeAtRampDownStart = 50.f;
    m_balanceIndefinitely = false;
    m_rampDownBegun = false;
    m_stagger = false;
    m_plantLeg = false;
    m_airborneStep = true;
    m_standUp = false;
    m_rampHipPitchOnFail = false;
    m_bodyPacket.m_floorVelocity.Zero();
    m_bodyPacket.m_floorAcceleration.Zero();
    m_ignoreFailure = false;//don't fail the balancer.  Used for standing up from a crouch lower than the balancer's failure conditions
    m_ignoringFailure = false;//The balancer would have failed but we're ignoring that
    m_failMustCollide = false;
    m_crouching = false;
    m_doneAVelocityProbe = false;
    initialiseRAGETaskParams();
    initialiseData();

    autoLeanCancel();
    autoLeanHipsCancel();
    autoLeanForceCancel();
  }

  /**
  * Enable dynamic balancing for this character
  */

  void NmRsCBUDynamicBalancer::onActivate()
  {

    Assert(m_character);
    m_deactivateMe = false;

    initialiseData();

    // initialise the RO and state data packet
    initializeDefaultDataPackets();

    //initialize autoLean //mmmmtodo should cancel all leans?
    m_autoLeanForceParams.m_bodyPart = getSpine()->getPelvisPart()->getPartIndex();

    //Below is so that m_roPacket.m_leftFootHitPosVel and m_roPacket.m_rightFootHitPosVel is initialized properly
    //mmmmtodo If so as probeForUnevenTerrain is called on the same step then the current and last positions are the same leading to 0 velocity
    //mmmmtodo get rid of the probeForUnevenTerrain here as it messes with the logic of the asynchronous probes

    populateBodyPacket();
#ifdef NM_RS_CBU_ASYNCH_PROBES
    m_character->InitializeProbe(NmRsCharacter::pi_balLeft);
    m_character->InitializeProbe(NmRsCharacter::pi_balRight);
#endif //NM_RS_CBU_ASYNCH_PROBES
    probeForUnevenTerrain();

#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    if (!m_character->balLegCollisions)
#endif
      // configure the lower body for our use
      setLegCollision(false);//moved to before updateBalanceStatus() as this can call deactivate and send a warning that we are turning on collisions that are already on 

    m_failType = balOK;
    m_failedIfDefaultFailure = false;
    // HDD: test to see if our initial body posture
    //    could be considered a balance failure,
    //    in which case we just bail out immediately
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    if (!m_character->balIgnoreFailure)
#endif
      if (!updateBalanceStatus())
      {
        // early out
        return;
      }

      // configure the lower body for our use
      setOpposeGravityAnkles(1.f);
      setOpposeGravityLegs(1.f);
      setLowerBodyGravityOpposition(m_body);
      calibrateLowerBodyEffectors(m_body);

      m_tickPhase = eDynBalTP_1;
  }
  /**
  * Deactivate on the next frame unless dynamicBalancer is activated again
  *   This is necessary because the balancer loses some state information when 
  *   deactivated and activated (e.g. can start stepping with a different leg)
  */
  void NmRsCBUDynamicBalancer::requestDeactivate()
  {
    Assert(m_character);
    m_deactivateMe = true;
  }
  /**
  * Stop the character from balancing
  */
  void NmRsCBUDynamicBalancer::onDeactivate()
  {
    Assert(m_character);

    m_hipPitch = 0.0f;
    m_hipRoll = 0.0f;
    m_hipYaw = 0.0f;
    m_useCustomTurnDir = false;
    m_legStraightnessModifier = 0.0f;
    //remember the fail state unless deactivate has been called without failing
    if (m_failType == balOK)
      m_failType = balFail_General;
    m_character->m_posture.init(); // this isn't set by most behaviours so make sure we reset it back here when we exit the behaviour.

    m_crouching = false;
    m_ignoreFailure = false;
    m_ignoringFailure = false;
    m_failMustCollide = false;

    // undo our changes to the lower body
    setOpposeGravityAnkles(0.f);
    setOpposeGravityLegs(0.f);
    //setLowerBodyGravityOpposition(m_body);
    // limbs note changed this to avoid tripping assert in setOpposeGravity because
    // m_active has been cleared already.
    m_body->setOpposeGravity(0.0f, bvmask_LegLeft | bvmask_LegRight);

    setLegCollision(true);
    rage::Vector3 zero(0,0,0);

	m_character->m_floorVelocity.Zero();
	m_character->m_COMvelRelative = m_character->m_COMvel;
	m_character->m_COMvelRelativeMag = m_character->m_COMvelRelative.Mag();

    m_character->m_floorAcceleration.Zero();
    //Restore the velocity damping on the character to it's default
    //mmmmtodo cache the DecayRates when the balancer starts
    //Can't do this at the moments as there is no GetLinearDecayRate from phArticulatedBody
    m_character->getArticulatedBody()->SetLinearDecayRate(0.1f);
    m_character->getArticulatedBody()->SetAngularDecayRate(0.1f);

    m_tickPhase = eDynBalTPInvalid;

#ifdef NM_RS_CBU_ASYNCH_PROBES
    m_character->ClearAsynchProbe_IfNotInUse(NmRsCharacter::pi_balLeft);
    m_character->ClearAsynchProbe_IfNotInUse(NmRsCharacter::pi_balRight);
#endif
  }

  /**
  * DYNBAL TICK NOTES
  * the balancer tick has been spliced into 3 parts; this is to encourage the 
  * distribution of processing across multiple characters so that we don't leave the CPU
  * sitting idle waiting for tasks to complete very often. In a simple test with 4 characters 
  * running the balancer, the time taken to complete the balance calculations is almost cut in half.
  *
  * This approach will be especially important to get right on the PS3 as we want to avoid waiting
  * on the PPU as much as possible, flooding the SPUs with tasks will make sure we make the best
  * use of the resources available.
  *
  * If you have four characters all using the balancer, the CBU manager will step across
  * their ticks in the following fashion:
  *
  * Character_1 - Tick 1 <== no blocking
  * Character_2 - Tick 1 <== no blocking
  * Character_3 - Tick 1 <== no blocking
  * Character_4 - Tick 1 <== no blocking
  * Character_1 - Tick 2 <== blocks to wait on results of [Character_1 - Tick 1]
  * Character_2 - Tick 2 <== blocks to wait on results of [Character_2 - Tick 1]
  * Character_3 - Tick 2
  * ... etc ...
  *
  * In this way we put plenty of processing between the blocks that wait for tasks to complete, instead
  * of doing:
  *
  * Character_1 - Tick 1 <== no blocking
  * Character_1 - Tick 2 <== blocks to wait on results of [Character_1 - Tick 1]
  * Character_1 - Tick 3 <== blocks to wait on results of [Character_1 - Tick 2]
  * Character_2 - Tick 1 <== no blocking
  * Character_2 - Tick 2 <== blocks to wait on results of [Character_2 - Tick 1]
  * ... etc ...
  *
  * if the NM_RS_CBU_DISTRIBUTED_PROCESSING define is not set, the system just does
  * all the processing in tick 2 without using RAGE tasks.
  */

  /**
  * update the balancer, if it is active, for this frame
  */
  CBUTaskReturn NmRsCBUDynamicBalancer::onTick(float timeStep)
  {
    if (m_deactivateMe)
    {     
      deactivate();
      return eCBUTaskComplete;
    }

    // limbs todo these are being called multiple times per tick...
    calibrateLowerBodyEffectors(m_body);
    setLowerBodyGravityOpposition(m_body);

    switch (m_tickPhase)
    {
    case eDynBalTP_1:
      {
        CBUTaskReturn phase1Return = tickPhase1(timeStep);
        m_tickPhase = (phase1Return == eCBUTaskComplete)?eDynBalTP_1:eDynBalTP_2;
        return phase1Return;
      }
    case eDynBalTP_2:
      m_tickPhase = eDynBalTP_3;
      return tickPhase2(timeStep);
    case eDynBalTP_3:
      m_tickPhase = eDynBalTP_1;
      return tickPhase3(timeStep);
    case eDynBalTPInvalid:
    default:
      {
        Assert(0);
      }
      break;
    }

    return eCBUTaskComplete;
  }

  CBUTaskReturn NmRsCBUDynamicBalancer::tickPhase1(float timeStep)
  {
    if (!m_character->getArticulatedBody())
      return eCBUTaskComplete;



    // check to see if the articulated body is asleep, in which case we are 'stable' and can't / needn't balance
    if (rage::phSleep *sleep = m_character->getArticulatedWrapper()->getArticulatedCollider()->GetSleep())
    {
      if (sleep->IsAsleep())
        return eCBUTaskComplete;
    }

#if ART_ENABLE_BSPY
    bspyScopedProfilePoint("DynamicBalancer::tickPhase1");
#endif // ART_ENABLE_BSPY

    rage::Vector3 floorAcceleration(0.0f,0.0f,0.0f);
    if (m_roPacket.m_movingFloor)
    {
      floorAcceleration = m_bodyPacket.m_floorAcceleration;
      if (floorAcceleration.Mag() > 20.0f)//CRASH
        floorAcceleration.Zero();
      if (floorAcceleration.Mag() > m_leanAccMax)
      {
        floorAcceleration.Normalize();
        floorAcceleration *= m_leanAccMax;
      }
    }//if (m_roPacket.m_movingFloor)


    if(isActive())
    {

#if ART_ENABLE_BSPY
      m_character->setCurrentSubBehaviour("tickPhase1");
#endif

      m_timer += timeStep;

      if (m_leanAgainstVelocity > 0.000001f)//mmmmtodo what if this turns off half way through performance?
      {
        //mmmmRemove - leanAgainst velocity
        rage::Vector3 levelCom = m_character->m_COMvelRelative;
        m_character->levelVector(levelCom);
        float lean;// = levelCom.Mag();
        lean = rage::Clamp(levelCom.Mag()-1.f, 0.f,3.f);
        lean *= 0.3333f;
        levelCom.Normalize();
        levelCom *= -1.f;
        autoLeanInDirection(levelCom,m_leanAgainstVelocity*lean*0.5f);
        autoLeanHipsInDirection(levelCom,m_leanAgainstVelocity*lean*0.5f);
        setBalanceTime(0.2f+m_leanAgainstVelocity*lean*0.3f);
      }

      //mmmtodo only apply airborne feet if airborne?  I'd also like to set feet wider for e.g. fallToKnees
      // it also can help you think the character is really trying to balance)
      m_roPacket.m_lateralStepOffset = 0.08f + (m_roPacket.m_legSeparation + m_roPacket.m_extraFeetApart) * 0.3f;
      m_roPacket.m_gUp = m_character->getUpVector();
      m_roPacket.m_leanHipgUp = m_character->getUpVector();
      // update lean direction
      updateAutoLean(timeStep);
      updateAutoLeanHips(timeStep);
      updateAutoLeanForce(timeStep);

      if (m_roPacket.m_movingFloor)
      {
        rage::Vector3 floorAccelerationForce(m_bodyPacket.m_floorAcceleration);
        if (floorAccelerationForce.Mag() > 20.0f)//CRASH
          floorAccelerationForce.Zero();
        if (floorAccelerationForce.Mag() > m_resistAccMax)
        {
          floorAccelerationForce.Normalize();
          floorAccelerationForce *= m_resistAccMax;
        }
        rage::phArticulatedBody *body = m_character->getArticulatedBody();
        for (int partIndex = 0; partIndex<body->GetNumBodyParts(); partIndex++)
        {
          if (partIndex <1 || partIndex >6)
          {
            NmRsGenericPart* part = m_character->getGenericPartByIndex(partIndex);//Returns part 0 if not in correct range
            part->applyForce(floorAccelerationForce*m_resistAcc*getPartMass(m_character->getArticulatedBody()->GetLink(partIndex)));
          }
        }
      }//if (m_roPacket.m_movingFloor)

      // update the time step for this frame
      BEGIN_PROFILING("packet assemble");
      m_roPacket.m_timeStep = timeStep;
      m_roPacket.m_gUpReal = m_character->getUpVector();
      m_roPacket.m_gUp += m_leanAcc*floorAcceleration / 9.81f;
      m_roPacket.m_gUp.Normalize();//mmmmTodoCheck floor acceleration stuff works now that gUp def changed 
      m_roPacket.m_leanHipgUp += m_hipLeanAcc*floorAcceleration / 9.81f;
      m_roPacket.m_leanHipgUp.Normalize();
      //mmmmVariance        
      if (m_footState.m_newStep)
      {
        if (m_stepClampScaleVariance < 0.f) 
          m_stepClampScale = m_stepClampScaleIn + m_character->getRandom().GetRanged(m_stepClampScaleVariance, 0.f);
        else
          m_stepClampScale = m_stepClampScaleIn + m_character->getRandom().GetRanged(-m_stepClampScaleVariance, m_stepClampScaleVariance);
        if (m_balanceTimeVariance < 0.f) 
          m_roPacket.m_balanceTime = m_balanceTimeIn + m_character->getRandom().GetRanged(m_balanceTimeVariance, 0.f);
        else
          m_roPacket.m_balanceTime = m_balanceTimeIn + m_character->getRandom().GetRanged(-m_balanceTimeVariance, m_balanceTimeVariance);
        if (m_footState.m_numOfSteps > 0)
          m_roPacket.m_timeTakenForStep = 0.f;
#if NM_NEW_BALANCER// stops the over twisting back and forth - mmmmmcheck I've got the raw calc correct 
        m_pelvisState.m_twistLeft = getLeftLeg()->getHip()->getActualTwist() - getLeftLeg()->getHip()->getMidTwist();
        m_pelvisState.m_twistRight = getRightLeg()->getHip()->getActualTwist() - getRightLeg()->getHip()->getMidTwist();;
#endif
      }
      // TDL note this code below is repeated in the initialise.
      m_roPacket.m_stepClampScale = m_stepClampScale;
      m_roPacket.m_stagger = m_stagger;
      m_roPacket.m_plantLeg = m_plantLeg;
      m_roPacket.m_airborneStep = m_airborneStep;
      m_roPacket.m_stepHeight = m_stepHeight;
      m_roPacket.m_random = m_character->getRandom().GetRanged(0.f,1.f);//set the dominant foot for when the stepper cannot decide.
      rage::Vector3 left2right = getRightLeg()->getHip()->getJointPosition() - getLeftLeg()->getHip()->getJointPosition();
      //m_roPacket.levelVector(left2right,0.f);
      m_roPacket.m_left2right = left2right;
      //MMMM decideBalancerState
      NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
      Assert(balColReactTask);
      NmRsCBUFallOverWall* fow = (NmRsCBUFallOverWall*)m_cbuParent->m_tasks[bvid_fallOverWall];
      Assert(fow);
      NmRsCBUTeeter* teeterTask = (NmRsCBUTeeter*)m_cbuParent->m_tasks[bvid_teeter];
      Assert(teeterTask);
      if (balColReactTask->isActive() || fow->isActive() || teeterTask->isActive())
      {
        m_roPacket.m_pos1stContact = balColReactTask->m_pos1stContact;
        m_roPacket.m_normal1stContact = balColReactTask->m_normal1stContact;
        m_roPacket.m_impactOccurred = balColReactTask->m_impactOccurred;
        m_roPacket.m_sideOfPlane = balColReactTask->m_sideOfPlane;
        m_roPacket.m_exclusionZone = balColReactTask->m_parameters.exclusionZone;
        m_roPacket.m_balancerState = balColReactTask->m_balancerState;
        if(balColReactTask->isActive())
          balColReactTask->setHipPitch(timeStep);//mmmmmmWHAT? mmmmHERE
      }
      else//turn off all exclusion zone
      {
        m_roPacket.m_impactOccurred = false;
      }

      //avoid stepping into car (braceForImpact)
		  m_roPacket.m_avoidCar = false;
		  NmRsCBUBraceForImpact* bfiTask = (NmRsCBUBraceForImpact*)m_cbuParent->m_tasks[bvid_braceForImpact];
		  Assert(bfiTask);
		  if (bfiTask->isActive())
		  {
		    int carInstanceIndex = bfiTask->m_parameters.instanceIndex;
        int carInstGenId = bfiTask->getCarInstGenID();
        if (m_character->IsInstValid(carInstanceIndex, carInstGenId))
		    {
			    rage::phInst* pInst = NULL;
			    pInst = m_character->getLevel()->GetInstance(carInstanceIndex);
			    if(pInst)
			    {
			      m_roPacket.m_avoidCar = true;

			      rage::Vector3 objectSize = VEC3V_TO_VECTOR3(pInst->GetArchetype()->GetBound()->GetBoundingBoxSize());
			      objectSize *= 0.5f;
			      static float extra = 0.05f;
			      objectSize.x += extra;
			      objectSize.y += extra;
			      rage::Vector3 offset = VEC3V_TO_VECTOR3(pInst->GetArchetype()->GetBound()->GetCentroidOffset());
			      //bottom i.e. the -objectSize.z(only necessary for graphics)
			      rage::Vector3 corner1L(objectSize.x,objectSize.y,-objectSize.z);
			      rage::Vector3 corner2L(objectSize.x,-objectSize.y,-objectSize.z);
			      rage::Vector3 corner3L(-objectSize.x,-objectSize.y,-objectSize.z);
			      rage::Vector3 corner4L(-objectSize.x,objectSize.y,-objectSize.z);
			      corner1L += offset;
			      corner2L += offset;
			      corner3L += offset;
			      corner4L += offset;
			      m_character->instanceToWorldSpace(&m_roPacket.m_carCorner1, corner1L, carInstanceIndex);
			      m_character->instanceToWorldSpace(&m_roPacket.m_carCorner2, corner2L, carInstanceIndex);
			      m_character->instanceToWorldSpace(&m_roPacket.m_carCorner3, corner3L, carInstanceIndex);
			      m_character->instanceToWorldSpace(&m_roPacket.m_carCorner4, corner4L, carInstanceIndex);
#if ART_ENABLE_BSPY
			      rage::Vector3 col(0.0f, 1.0f, 0.0f);
			      m_character->bspyDrawLine(m_roPacket.m_carCorner1,m_roPacket.m_carCorner2, col);
			      m_character->bspyDrawLine(m_roPacket.m_carCorner2,m_roPacket.m_carCorner3, col);
			      m_character->bspyDrawLine(m_roPacket.m_carCorner3,m_roPacket.m_carCorner4, col);
			      m_character->bspyDrawLine(m_roPacket.m_carCorner4,m_roPacket.m_carCorner1, col);
#endif
			    }//if(pInst)
		    }//if (m_character->IsInstValid(carInstanceIndex, carInstGenId))
		  }//if (bfiTask->isActive())

      if (m_character->getBodyIdentifier() == gtaWilma) // TDL women step a little less far
        m_roPacket.m_stepClampScale *= 0.7f;


      // collect data for body packet
      populateBodyPacket();
      END_PROFILING();
      if (m_roPacket.m_changeStepTime > 0.f /*&& 
                                            (!m_bodyPacket.cd.m_leftFootCollided && !m_bodyPacket.cd.m_rightFootCollided)*/)
                                            m_roPacket.m_timeTakenForStep += timeStep;

      BEGIN_PROFILING("update status");

      // check the status of the balance and
      // bail out early if we just realized we're falling
      if (!updateBalanceStatus())
      {
        END_PROFILING();
        return eCBUTaskComplete;
      }

      // copy lagged values from foot<->pelvis, copy in any custom values (turn dir, etc)
      updateModulePackets(m_footState, m_pelvisState);
      updateRAGETaskParams();
      END_PROFILING();

      // fire ray probes into near-by terrain to find
      // ground heights & normals, results are stored in RO packet
      BEGIN_PROFILING("terrain probe");
      probeForUnevenTerrain();
      END_PROFILING();
      m_footState.m_newStep = false;

      if (m_character->getEngine()->areDistributedTasksEnabled())
      {
        // call balancer tasks - foot placement + pelvis control are concurrent
        BEGIN_PROFILING("task start");
        m_footTaskHandle    = rage::sysTaskManager::Create(TASK_INTERFACE(NmRsCBU_DBMFootPlacement), m_footTaskParams, m_character->getEngine()->getSchedulerIndex());
        m_pelvisTaskHandle  = rage::sysTaskManager::Create(TASK_INTERFACE(NmRsCBU_DBMPelvisControl), m_pelvisTaskParams, m_character->getEngine()->getSchedulerIndex());
        END_PROFILING();
      }

#if ART_ENABLE_BSPY
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]);
#endif

      return eCBUTaskMoreTicksRequired;
    }//if(isActive())
    else
      return eCBUTaskComplete;
  }

  CBUTaskReturn NmRsCBUDynamicBalancer::tickPhase2(float /*timeStep*/)
  {
#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("tickPhase2");
#endif

    if (m_character->getEngine()->areDistributedTasksEnabled())
    {
      Assert(m_footTaskHandle && m_pelvisTaskHandle);

      // wait for the foot/pelvis stuff to finish, then kick off the balance solver
      rage::sysTaskHandle waitHandles[2] = { m_footTaskHandle, m_pelvisTaskHandle };
      rage::sysTaskManager::WaitMultiple(2, waitHandles);

      m_footTaskHandle = 0;
      m_pelvisTaskHandle = 0;

      m_solveTaskHandle   = rage::sysTaskManager::Create(TASK_INTERFACE(NmRsCBU_DBMBalanceSolve), m_solveTaskParams, m_character->getEngine()->getSchedulerIndex());
    }
    else
    {
      BEGIN_PROFILING("dbm-FootPlacement");
#if ART_ENABLE_BSPY
      m_character->setCurrentSubBehaviour("dbm-FootPlacement");
#endif
      dbmFootPlacement(m_roPacket, m_bodyPacket, m_footState);
      END_PROFILING();

      //MMMMM todo this will be a frame behind again if DistributedTasks are Enabled - can we calculate something like it withour footPlacement?
      //  alternatively we minimise the error for moving platforms by calculating everything in local space - urggh
      m_pelvisState.m_centreOfFeet = m_footState.m_centreOfFeet;

      BEGIN_PROFILING("dbm-PelvisControl");
#if ART_ENABLE_BSPY
      m_character->setCurrentSubBehaviour("dbm-FootPlacement");
#endif
      dbmPelvisControl(m_roPacket, m_bodyPacket, m_pelvisState);
      END_PROFILING();

      BEGIN_PROFILING("dbm-BalanceSolve");
#if ART_ENABLE_BSPY
      m_character->setCurrentSubBehaviour("dbm-FootPlacement");
#endif
      dbmBalanceSolve(m_roPacket, m_footState, m_pelvisState, m_bodyPacket);
      END_PROFILING();
    }

#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]);
#endif

    return eCBUTaskMoreTicksRequired;
  }

  CBUTaskReturn NmRsCBUDynamicBalancer::tickPhase3(float timeStep)
  {

#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("tickPhase2");
#endif

    if (m_character->getEngine()->areDistributedTasksEnabled())
    {
      // wait for the solver task to complete
      BEGIN_PROFILING("task wait");
      rage::sysTaskManager::Wait(m_solveTaskHandle);
      m_solveTaskHandle = 0;
      END_PROFILING();
    }

    //numOfSteps starts from the step after any step being taken at activation
    if (m_footState.m_numOfSteps < 0)
      m_footState.m_numOfSteps = 0;
    if (m_footState.m_numOfSteps4Max < 0)
      m_footState.m_numOfSteps4Max = 0;

    // write back to physics from results
    // calculated into the body packet in dbmBalanceSolve()
    BEGIN_PROFILING("process results");
    setPostureData(m_footState);
    readBodyPacketResults();
    END_PROFILING();

#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour("tickPhase2");
#endif

#if defined(LOOSER_ANKLES)
    float ankleStiffness = rage::Clamp(1.f + 4.f * (60.f*m_roPacket.m_timeStep - 1.f), 1.f, 5.f);
    if (m_bodyPacket.cd.m_leftFootCollided)//MMMMhere MMMMnote should be m_leftfootbalance (as can be in contact eg toe brush but don't want strong ankle here)
      getLeftLegInputData()->getAnkle()->setMuscleStiffness(10.f);//mmmmmherebalancerParam went to 3
    else
      getLeftLegInputData()->getAnkle()->setMuscleStiffness(ankleStiffness); 
    if (m_bodyPacket.cd.m_rightFootCollided)
      getRightLegInputData()->getAnkle()->setMuscleStiffness(10.f);
    else
      getRightLegInputData()->getAnkle()->setMuscleStiffness(ankleStiffness); 
#endif

#if defined(LOOSER_TAPERED_KNEE_STRENGTH)
    float kneeStrength = m_kneeStrength * m_leftLegStiffness; // ie the kneeStrength is multiplier on bodyStiffness
    if (m_footState.state.m_leftFootBalance)
    {
      float angle = -(getLeftLeg()->getKnee()->getActualAngle());
      float strength = kneeStrength * (m_taperKneeStrength ? rage::Clamp( (1.f - 2.f*(angle-1.5f)/(PI - 1.5f)), 0.2f, 1.f) : 1.f);
      NM_RS_DBG_LOGF(L"DYNC| kneeTaperStr L : %.3f", strength);

      getLeftLegInputData()->getKnee()->setMuscleStrength(strength*strength);
      getLeftLegInputData()->getKnee()->setMuscleStiffness(1.5f);

      //mmmmSlowerLegTest        
      //set original damping
      float damping = 2.0f * m_leftLegStiffness; /* x damping scale of 1.0 */
      getLeftLegInputData()->getHip()->setMuscleDamping(damping);
    }
    else
    {
      getLeftLegInputData()->getKnee()->setMuscleStrength(kneeStrength*kneeStrength);
      getLeftLegInputData()->getKnee()->setMuscleStiffness(2.f);
      //mmmmSlowerLegTest        
      //set new swing leg damping
      float damping = 2.0f * m_leftLegSwingDamping * m_leftLegStiffness; /* x damping scale of 1.0 */
      getLeftLegInputData()->getHip()->setMuscleDamping(damping);

      //mmmmKneebend2
      //minimum knee bend 
      float kneeMin = -m_minKneeAngle;
      float kneeAng = nmrsGetActualAngle(getLeftLeg()->getKnee());
      if (kneeAng > kneeMin)
      { 
        float kneeAngVel;
        rage::Matrix34 thighMat;
        getLeftLeg()->getThigh()->getMatrix(thighMat);
        kneeAngVel = getLeftLeg()->getKnee()->getJoint()->GetJointAngularVelocityOnAxis(m_character->getArticulatedBody(), RCC_VEC3V(thighMat.a));
#if ART_ENABLE_BSPY
        bspyTaskScratchpadAuto("DynBAl**", kneeAngVel);
#endif
        float bendRate = 0.f;
        if (kneeAngVel > 0.f)
        {
          bendRate = rage::Clamp(kneeAngVel,0.f, 5.f);
        }
        getLeftLegInputData()->getKnee()->setDesiredAngle(rage::Min(kneeAng - bendRate + 20.0f*timeStep * (kneeMin - kneeAng), getLeftLegInputData()->getKnee()->getDesiredAngle()));
      }
    }
    kneeStrength = m_kneeStrength * m_rightLegStiffness;
    if (m_footState.state.m_rightFootBalance)
    {
      float angle = -(getRightLeg()->getKnee()->getActualAngle());
      float strength = kneeStrength * (m_taperKneeStrength ? rage::Clamp( (1.f - 2.f*(angle-1.5f)/(PI - 1.5f)), 0.2f, 1.f) : 1.f);
      NM_RS_DBG_LOGF(L"DYNC| kneeTaperStr R : %.3f", strength);

      getRightLegInputData()->getKnee()->setMuscleStrength(strength*strength);
      getRightLegInputData()->getKnee()->setMuscleStiffness(1.5f);

      //mmmmSlowerLegTest 
      //set original damping
      float damping = 2.0f * m_rightLegStiffness; /* x damping scale of 1.0 */
      getRightLegInputData()->getHip()->setMuscleDamping(damping);
    }
    else
    {
      getRightLegInputData()->getKnee()->setMuscleStrength(kneeStrength*kneeStrength);
      getRightLegInputData()->getKnee()->setMuscleStiffness(2.0f);

      //mmmmSlowerLegTest
      //set swing leg damping
      float damping = 2.0f * m_rightLegSwingDamping * m_rightLegStiffness; /* x damping scale of 1.0 */
      getRightLegInputData()->getHip()->setMuscleDamping(damping);

      //minimum knee bend 
      float kneeMin = -m_minKneeAngle;
      float kneeAng = nmrsGetActualAngle(getRightLeg()->getKnee());
      if (kneeAng > kneeMin)
      { 
        float kneeAngVel;
        rage::Matrix34 thighMat;
        getRightLeg()->getThigh()->getMatrix(thighMat);
        kneeAngVel = getRightLeg()->getKnee()->getJoint()->GetJointAngularVelocityOnAxis(m_character->getArticulatedBody(), RCC_VEC3V(thighMat.a));
#if ART_ENABLE_BSPY
        bspyTaskScratchpadAuto("DynBAl**R", kneeAngVel);
#endif
        float bendRate = 0.f;
        if (kneeAngVel > 0.f)
          bendRate = rage::Clamp(kneeAngVel,0.f, 5.f);

        // limbs todo get desired angle will not behave as expected here. find out what the intent is. can it simply be read from
        // what is already in the right leg data message or does it come from ik or another behaviour?
        getRightLegInputData()->getKnee()->setDesiredAngle(rage::Min(kneeAng - bendRate + 20.0f*timeStep * (kneeMin - kneeAng), nmrsGetDesiredAngle(getLeftLeg()->getKnee())));
      }

    }
#endif // LOOSER_TAPERED_KNEE_STRENGTH
    //mmmmKneebend2 
    //mmmmmtodo minimum knee bend could be applied (as above) to the stance foot but only if there is a large leg split 
    // otherwise standing still gives an oscillation

    //lower the opposeGravity if the foot is not flat to the floor (unless stepping)
    float dot = 1.f;
#if NM_NEW_BALANCER
    if (m_footState.m_footChoice != NmRsCBUDynBal_FootState::kLeftStep)
#endif
      dot = m_roPacket.m_leftFootProbeNormal.Dot(m_bodyPacket.m_leftFoot.m_tm.b);
    float dot2 = dot*dot;

    getLeftLegInputData()->getAnkle()->setOpposeGravity(2.f*m_opposeGravityAnkles*dot2);

    dot = 1.f;
#if NM_NEW_BALANCER
    if (m_footState.m_footChoice != NmRsCBUDynBal_FootState::kRightStep)
#endif
      dot = m_roPacket.m_rightFootProbeNormal.Dot(m_bodyPacket.m_rightFoot.m_tm.b);
    dot2 = dot*dot;

    getRightLegInputData()->getAnkle()->setOpposeGravity(2.f*m_opposeGravityAnkles*dot2);

    if (!m_balanceIndefinitely)
    {
      //m_rampDownBegun so that if maxSteps ever changes on fly will cause balancer to fail
      //need to taker another look at this mmmmhere
      if ((m_timer >= m_maximumBalanceTime || m_footState.m_maxSteps <= m_footState.m_numOfSteps4Max) && !m_rampDownBegun)
      {
        //only do this once
        //once it has begun it cannot be stopped
        m_rampDownBegun = true;
        m_balanceTimeAtRampDownStart = m_timer;
      }

      if (m_timer >= m_balanceTimeAtRampDownStart)
      {
#if DYNBAL_GIVEUP_RAMP
        // if we got here without leaning too much, send "aborted" message
        if(m_distHeightRatio < m_leanToAbort)
        {
          ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
          if (feedback)
          {
            feedback->m_agentID = m_character->getID();
            feedback->m_argsCount = 0;
            strcpy(feedback->m_behaviourName, NMDynamicBalancerAbortFeedbackName);
            feedback->onBehaviourEvent();
          }
        }
#endif
        m_character->m_uprightConstraint.forceActive = false;
        m_character->m_uprightConstraint.torqueActive = false;
        if (m_rampHipPitchOnFail)
        {
          m_hipPitch = rage::Clamp(m_hipPitch - 0.3f*timeStep, -0.6f,0.6f);
          rage::Vector3 levelCom = m_character->m_COMvelRelative;
          m_character->levelVector(levelCom);
          float lean;// = levelCom.Mag();
          lean = -rage::Clamp(4.f*levelCom.Mag(), 0.f,0.7f);
          levelCom.Normalize();
          levelCom *= -1.f;
          autoLeanHipsInDirection(levelCom,lean);
        }

        if (m_fallType == 1)
        {
          ////setStepWithBoth(true);
          //m_legStraightnessModifier = -0.1f*sin(m_timer*7.f);
          setDontChangeStep(true);
        }
        if (m_fallType == 2)
        {
          setForceBalance(true);
        }
        if (m_fallType == 3)
        {
          NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
          Assert(balColReactTask);
          balColReactTask->m_balancerState = bal_Slump;
          m_fallType=4;//HACK mmmmhere
        }

        float stiffnessReductionTime = 1.f/(m_fallMult*0.6f*(m_timer - m_balanceTimeAtRampDownStart)+1.f);

        getRightLegInputData()->getAnkle()->setStiffness(stiffnessReductionTime*m_rightLegStiffness,1.f);
        getRightLegInputData()->getKnee()->setStiffness(stiffnessReductionTime*m_rightLegStiffness,1.f);

        stiffnessReductionTime = 1.f/(m_fallMult*0.4f*(m_timer - m_balanceTimeAtRampDownStart)+1.f);
        getRightLegInputData()->getHip()->setStiffness(stiffnessReductionTime*m_rightLegStiffness,1.f);

        stiffnessReductionTime = 1.f/(m_fallMult*0.4f*(m_timer - m_balanceTimeAtRampDownStart)+1.f);
        getLeftLegInputData()->getAnkle()->setStiffness(stiffnessReductionTime*m_rightLegStiffness,1.f);
        getLeftLegInputData()->getHip()->setStiffness(stiffnessReductionTime*m_rightLegStiffness,1.f);

        stiffnessReductionTime = 1.f/(m_fallMult*0.3f*(m_timer - m_balanceTimeAtRampDownStart)+1.f);
        getLeftLegInputData()->getKnee()->setStiffness(stiffnessReductionTime*m_rightLegStiffness,1.f);

        if (m_fallReduceGravityComp)
        {
          setOpposeGravityLegs(stiffnessReductionTime);
          setOpposeGravityAnkles(stiffnessReductionTime);
          setLowerBodyGravityOpposition(m_body);
        }
      }
    }

#if ART_ENABLE_BSPY
    m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]);
#endif

    return eCBUTaskComplete;
  }


  /**
  * Toggles the collision between left and right leg parts
  */
  void NmRsCBUDynamicBalancer::setLegCollision(bool enable)
  {
#if NM_NEW_LEG_COLLISION_CODE
    if (enable)
    {
      m_character->m_Leg2LegCollisionExclusion.setB(bvmask_None);
    }
    else
    {
      m_character->m_Leg2LegCollisionExclusion.a = bvmask_FootLeft | bvmask_ShinLeft | bvmask_ThighLeft;
      m_character->m_Leg2LegCollisionExclusion.setB(bvmask_FootRight | bvmask_ShinRight | bvmask_ThighRight);
    }
#else
    //Old way of disabling selfCollisions with the legs - leaving here as it may be quicker to:
    //  do this way when disabling and then turn on m_Leg2LegCollisionExclusion method only when trying to enable the collisions again
    Assert(getLeftLeg());

    rage::phArticulatedCollider *collider = m_character->getArticulatedWrapper()->getArticulatedCollider();
    collider->SetPartsCanCollide(getLeftLeg()->getThigh()->getPartIndex(), getRightLeg()->getThigh()->getPartIndex(), enable, false);
    collider->SetPartsCanCollide(getLeftLeg()->getThigh()->getPartIndex(), getRightLeg()->getShin()->getPartIndex(), enable, false);
    collider->SetPartsCanCollide(getLeftLeg()->getShin()->getPartIndex(), getRightLeg()->getThigh()->getPartIndex(), enable, false);
    collider->SetPartsCanCollide(getLeftLeg()->getShin()->getPartIndex(), getRightLeg()->getShin()->getPartIndex(), enable, false);
    collider->SetPartsCanCollide(getLeftLeg()->getShin()->getPartIndex(), getRightLeg()->getFoot()->getPartIndex(), enable, false);
    collider->SetPartsCanCollide(getLeftLeg()->getFoot()->getPartIndex(), getRightLeg()->getShin()->getPartIndex(), enable, false);
    collider->SetPartsCanCollide(getLeftLeg()->getFoot()->getPartIndex(), getRightLeg()->getFoot()->getPartIndex(), enable, false);
    collider->SetPartsCanCollide(getLeftLeg()->getFoot()->getPartIndex(), getRightLeg()->getThigh()->getPartIndex(), enable, false);
    collider->SetPartsCanCollide(getLeftLeg()->getThigh()->getPartIndex(), getRightLeg()->getFoot()->getPartIndex(), enable, false);
    collider->FinalizeSettingPartsCanCollide();
#endif

  }

  void NmRsCBUDynamicBalancer::setLowerBodyGravityOpposition(NmRsHumanBody* body)
  {
#if ART_ENABLE_BSPY
    bspyProfileStart("setLowerBodyGravityOpposition");
#endif

    // this func can be called from outside the balancer. make sure the damn
    // thing is actually running!
    Assert(m_character);
    Assert(m_active);

    body->getLeftLegInputData()->getAnkle()->setOpposeGravity(2.f*m_opposeGravityAnkles);
    body->getLeftLegInputData()->getKnee()->setOpposeGravity(2.f*m_opposeGravityLegs);
    body->getLeftLegInputData()->getHip()->setOpposeGravity(m_opposeGravityLegs);

    body->getRightLegInputData()->getAnkle()->setOpposeGravity(2.f*m_opposeGravityAnkles);
    body->getRightLegInputData()->getKnee()->setOpposeGravity(2.f*m_opposeGravityLegs);
    body->getRightLegInputData()->getHip()->setOpposeGravity(m_opposeGravityLegs);

    body->getSpineInputData()->getSpine0()->setOpposeGravity(m_opposeGravityLegs);

#if ART_ENABLE_BSPY
    bspyProfileEnd("setLowerBodyGravityOpposition");
#endif
  }

  // Depends on the following member variables:
  //  m_character
  //  m_leftLegStiffness
  //  m_rightLegStiffness
  //  
  void NmRsCBUDynamicBalancer::calibrateLowerBodyEffectors(NmRsHumanBody* body)
  {
    // this func can be called from outside the balancer. make sure the damn
    // thing is actually running!
    Assert(m_character);
    if(!isActive())
      return;

#if ART_ENABLE_BSPY
    bspyProfileStart("calibrateLowerBodyEffectors");
#endif

    float stiffness = m_leftLegStiffness;
    float strength = stiffness * stiffness;
    float damping = 2.0f * stiffness; /* x damping scale of 1.0 */

    body->getLeftLegInputData()->getAnkle()->setMuscleStrength(strength);
    body->getLeftLegInputData()->getKnee()->setMuscleStrength(strength);
    body->getLeftLegInputData()->getHip()->setMuscleStrength(strength);
    body->getLeftLegInputData()->getAnkle()->setMuscleDamping(damping);
    body->getLeftLegInputData()->getKnee()->setMuscleDamping(damping);
    body->getLeftLegInputData()->getHip()->setMuscleDamping(damping);

    stiffness = m_rightLegStiffness;
    strength = stiffness * stiffness;
    damping = 2.0f * stiffness; /* x damping scale of 1.0 */

    // todo perhaps a limb-wide setter for this?
    body->getRightLegInputData()->getAnkle()->setMuscleStrength(strength);
    body->getRightLegInputData()->getKnee()->setMuscleStrength(strength);
    body->getRightLegInputData()->getHip()->setMuscleStrength(strength);
    body->getRightLegInputData()->getAnkle()->setMuscleDamping(damping);
    body->getRightLegInputData()->getKnee()->setMuscleDamping(damping);
    body->getRightLegInputData()->getHip()->setMuscleDamping(damping);

    stiffness = rage::Max(m_leftLegStiffness,m_rightLegStiffness);
    strength = stiffness * stiffness;
    damping = 2.0f * stiffness; /* x damping scale of 1.0 */

    body->getSpineInputData()->getSpine0()->setMuscleStrength(strength);
    body->getSpineInputData()->getSpine0()->setMuscleDamping(damping);

    body->getLeftLegInputData()->getKnee()->setMuscleStiffness(2.0f);
    body->getRightLegInputData()->getKnee()->setMuscleStiffness(2.0f);

    body->getLeftLegInputData()->getHip()->setMuscleStiffness(1.5f); // todo what is this asymmetry for?
    body->getRightLegInputData()->getHip()->setMuscleStiffness(1.5f);

#if ART_ENABLE_BSPY
    bspyProfileEnd("calibrateLowerBodyEffectors");
#endif
  }

  void NmRsCBUDynamicBalancer::initializeDefaultDataPackets()
  {
    // NmRsCBUDynBal_ReadOnly
    {
      m_roPacket.m_leftFootProbeHitPos.Set(0,0,0);
      m_roPacket.m_leftFootProbeNormal.Set(0,0,0);
      m_roPacket.m_leftFootHitPosVel.Set(0,0,0);
      m_roPacket.m_rightFootProbeHitPos.Set(0,0,0);
      m_roPacket.m_rightFootProbeNormal.Set(0,0,0);
      m_roPacket.m_rightFootHitPosVel.Set(0,0,0);
      m_roPacket.m_left2right.Set(0,0,0);


      //MMMMbalancerState
      m_roPacket.m_pos1stContact.Set(0,0,0);
      m_roPacket.m_normal1stContact.Set(0,0,0);
      m_roPacket.m_sideOfPlane = 0.f;
      m_roPacket.m_balancerState = bal_Normal;
      m_roPacket.m_impactOccurred = false;

      m_roPacket.m_probeHitLeft = false;
      m_roPacket.m_probeHitRight = false;
      // TDL note the code below is repeated in the step
      m_roPacket.m_stepClampScale = m_stepClampScale;
      m_roPacket.m_stagger = false;
      m_roPacket.m_plantLeg = false;
      m_roPacket.m_airborneStep = true;
      if (m_character->getBodyIdentifier() == gtaWilma) // TDL women step a little less far
        m_roPacket.m_stepClampScale *= 0.7f;

      m_roPacket.m_gUp = m_character->getUpVector();
      m_roPacket.m_gUpReal = m_character->getUpVector();
      m_roPacket.m_leanHipgUp = m_character->getUpVector();

      m_roPacket.m_balanceTime            = 0.2f;
      m_roPacket.m_balanceTimeHip         = 0.3f;
      m_roPacket.m_dragReduction          = 1.0f;
      m_roPacket.m_stepDecisionThreshold  = 0.0f;
      m_roPacket.m_ankleEquilibrium       = 0.f;
      m_roPacket.m_extraFeetApart      = 0.f;
      m_roPacket.m_stepHeight             = m_stepHeight;
      m_roPacket.m_legsApartRestep = 0.2f;
      m_roPacket.m_legsTogetherRestep = 1.0f;
      m_roPacket.m_legsApartMax = 2.0f;
      m_roPacket.m_taperKneeStrength      = m_taperKneeStrength;
      m_roPacket.m_random = m_character->getRandom().GetRanged(0.f,1.f);//set the dominant foot for when the stepper cannot decide.
      m_roPacket.m_useComDirTurnVelThresh = 0.f;

      // cache useful stuff from character config
      const CharacterConfiguration& cc = m_character->getCharacterConfiguration();
      m_roPacket.m_legSeparation = cc.m_legSeparation;
      m_roPacket.m_legStraightness = cc.m_legStraightness + m_legStraightnessModifier;
      m_roPacket.m_hipYaw = cc.m_hipYaw;
      m_roPacket.m_charlieChapliness = cc.m_charlieChapliness;
      m_roPacket.m_defaultHipPitch = cc.m_defaultHipPitch;
      m_roPacket.m_lateralStepOffset = 0.08f + m_roPacket.m_legSeparation * 0.3f;
      m_roPacket.m_hipTwistOffset = 0.f;
      m_roPacket.m_hipLean1Offset = 1.f;
      m_roPacket.m_hipLean2Offset = 1.f;
      //mmmmHEREbalancerParam
      if (m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl)
      {
        m_roPacket.m_hipTwistOffset = 0.3f;//0.33820052f;//Calculated by the difference between the midTwist of the hips for fred(limits not extended) and cowboy(limits extended)
        m_roPacket.m_hipLean1Offset = 1.9f/2.1f;
        m_roPacket.m_hipLean2Offset = 0.86f/1.16f;
      }
      //mmmmHEREbalancer for rdrCowboy m_lateralStepOffset = 0.12 stops him from stepping monkees style 
      //m_roPacket.m_lateralStepOffset = 0.13f;
      // calculate the full length of the characters legs -> buttocks
      rage::Vector3 footMid = getLeftLeg()->getFoot()->getInitialMatrix().d;
      rage::Vector3 pelvis = getSpine()->getPelvisPart()->getInitialMatrix().d;
      footMid.Add(getRightLeg()->getFoot()->getInitialMatrix().d);
      footMid *= 0.5f;
      pelvis.Subtract(footMid); 
      //So Fred/Wilma\Cowboy will work in y or z gravity
      //pelvis is the position of the pelvis in the original/setup co-ord frame therefore is independent of gravity and just depends on the orientation of the setup
      if (m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl)
      {
        rage::Vector3 tempUp(0.f,1.f,0.f);
        //mmmmHEREbalancerParam
        m_roPacket.m_fullLegLength = pelvis.Dot(tempUp);//+0.05f;
        //rage::Vector3 tempUp(0.f,0.f,1.f);
        //m_roPacket.m_fullLegLength = pelvis.Dot(tempUp);
        //
        m_character->m_posture.alternateRoot = getSpine()->getSpine0()->getJointIndex();
      }
      else
      {
        rage::Vector3 tempUp(0.f,0.f,1.f);
        m_roPacket.m_fullLegLength = pelvis.Dot(tempUp);
#if NM_NEW_BALANCER
        //character measurement is wrong - add a bit to it for now
        m_roPacket.m_fullLegLength += 0.07522f*0.4f;
#endif//
      }

      // calculate hip width (buttock - hip jt)
      rage::Matrix34 hipMat;
      rage::Vector3 butPos = getSpine()->getPelvisPart()->getPosition(), hipPos;
      getLeftLeg()->getHip()->getMatrix1(hipMat);
      hipPos.Subtract(hipMat.d, butPos);
      hipPos.Cross(m_roPacket.m_gUpReal); 
      m_roPacket.m_hipWidth = hipPos.Mag();
      //mmmmHEREbalancerParam
      //if (m_character->getBodyIdentifier() == rdrCowboy)
      //  m_roPacket.m_hipWidth = hipPos.Mag()*0.8f;
    }

    // NmRsCBUDynBal_FootState
    {
      m_footState.m_leftFootPos = getLeftLeg()->getFoot()->getPosition();
      m_footState.m_rightFootPos = getRightLeg()->getFoot()->getPosition();
      m_footState.calculateCentreOfFeet();

      m_footState.m_groundHeightLeft = m_roPacket.m_gUpReal.Dot(m_footState.m_leftFootPos); 
      m_footState.m_groundHeightRight = m_roPacket.m_gUpReal.Dot(m_footState.m_rightFootPos); 

      m_footState.m_leftDistance = 0.0f;
      m_footState.m_rightDistance = 0.0f;

      m_footState.m_footChoice = NmRsCBUDynBal_FootState::kNotStepping;

      m_footState.state.m_leftGround = false;
      m_footState.state.m_achievedGoal = false;
      m_footState.state.m_isInsideSupport = true;
      m_footState.state.m_isInsideSupportHonest = true;

      m_footState.state.m_leftFootBalance = false;
      m_footState.state.m_rightFootBalance = false;
      m_footState.m_oldDesiredPos.Set(0,0,0);
      m_footState.m_oldFootPos.Set(0,0,0);

      m_footState.m_stepFootStart.Zero();

      m_footState.m_forceBalance = false;
      m_footState.m_dontChangeStep = false;
      m_footState.m_stepWithBoth = false;
      m_footState.m_numOfSteps = -1;
      m_footState.m_numOfSteps4Max = -1;
      m_footState.m_forceStep = 0;
      m_footState.m_forceStepExtraHeight = 0.07f;
      m_footState.m_numOfStepsAtForceStep = -3;
      m_footState.m_newStep = true;
      m_footState.m_stepIfInSupport = true;
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
      m_footState.m_stepIfInSupport = m_character->stepIfInSupport;
#endif

      m_footState.m_alwaysStepWithFarthest = false;
    }

    // NmRsCBUDynBal_PelvisState
    {
      m_pelvisState.m_useCustomTurnDir = false;
      m_pelvisState.m_customTurnDir.Zero();

      m_pelvisState.m_hipPitch = 0.0f;
      m_pelvisState.m_hipRoll = 0.0f;
      m_pelvisState.m_hipYaw = 0.0f;

      m_pelvisState.m_leftFootBalance = m_footState.state.m_leftFootBalance;
      m_pelvisState.m_rightFootBalance = m_footState.state.m_rightFootBalance;
      m_pelvisState.m_centreOfFeet = m_footState.m_centreOfFeet;

      m_pelvisState.m_totalForwards = m_roPacket.m_ankleEquilibrium;
      m_pelvisState.m_totalRight = 0.0f;

      m_pelvisState.m_waistHeight = 100.0f;

      rage::Matrix34 leftFootMat, rightFootMat, pelvisMat, headMat, thighLeftMat, thighRightMat;
      float dmylegSep, dmyLegStr, hipYaw, charlieChap, headYaw, defaultHipPitch;
      getLeftLeg()->getFoot()->getBoundMatrix(&leftFootMat);
      getRightLeg()->getFoot()->getBoundMatrix(&rightFootMat);
      getSpine()->getPelvisPart()->getBoundMatrix(&pelvisMat);
      getSpine()->getHeadPart()->getBoundMatrix(&headMat);
      getLeftLeg()->getThigh()->getBoundMatrix(&thighLeftMat);
      getRightLeg()->getThigh()->getBoundMatrix(&thighRightMat);


      m_character->measureCharacter(
        leftFootMat, 
        rightFootMat, 
        pelvisMat,
        headMat,
        thighLeftMat,
        thighRightMat,
        &dmylegSep, 
        &dmyLegStr, 
        &charlieChap, 
        &hipYaw,
        &headYaw,
        &defaultHipPitch);

      m_pelvisState.m_twistLeft = hipYaw - charlieChap;
      m_pelvisState.m_twistRight = -hipYaw - charlieChap;
    }
  }

  void NmRsCBUDynamicBalancer::populateBodyPacket()
  {
    // these are calculated by character in ::postStep() before balancer is called,
    // so we can just copy rather than re-calculate them
    m_bodyPacket.m_COM = m_character->m_COM;
    m_bodyPacket.m_COMvel = m_character->m_COMvel;
    m_bodyPacket.m_lvlCOMvelRelative = m_character->m_COMvel - m_character->getFloorVelocity();//probeForUnevenTerrain also updates this
    m_roPacket.levelVector(m_bodyPacket.m_lvlCOMvelRelative);//probeForUnevenTerrain also updates this
    m_bodyPacket.m_COMrotvel = m_character->m_COMrotvel;
    m_bodyPacket.m_COMTM = m_character->m_COMTM;
    m_bodyPacket.m_floorVelocity = m_character->getFloorVelocity();//probeForUnevenTerrain also updates this

    m_body->getLeftArm()->getHand()->saveToShadow(m_bodyPacket.m_leftHand);
    m_body->getRightArm()->getHand()->saveToShadow(m_bodyPacket.m_rightHand);
    // left leg
    getLeftLeg()->getHip()->saveToShadow(m_bodyPacket.m_leftHip);
    getLeftLeg()->getThigh()->saveToShadow(m_bodyPacket.m_leftThigh);
    getLeftLeg()->getKnee()->saveToShadow(m_bodyPacket.m_leftKnee);
    getLeftLeg()->getShin()->saveToShadow(m_bodyPacket.m_leftShin);
    getLeftLeg()->getAnkle()->saveToShadow(m_bodyPacket.m_leftAnkle);
    getLeftLeg()->getFoot()->saveToShadow(m_bodyPacket.m_leftFoot);
    m_bodyPacket.m_leftElbowMat = getLeftLeg()->m_elbowMat;
    m_bodyPacket.cd.m_leftFootCollided = getLeftLeg()->getFoot()->collided();
    m_bodyPacket.cd.m_leftFootCollidedLast = getLeftLeg()->getFoot()->previousCollided();

    // right leg
    getRightLeg()->getHip()->saveToShadow(m_bodyPacket.m_rightHip);
    getRightLeg()->getThigh()->saveToShadow(m_bodyPacket.m_rightThigh);
    getRightLeg()->getKnee()->saveToShadow(m_bodyPacket.m_rightKnee);
    getRightLeg()->getShin()->saveToShadow(m_bodyPacket.m_rightShin);
    getRightLeg()->getAnkle()->saveToShadow(m_bodyPacket.m_rightAnkle);
    getRightLeg()->getFoot()->saveToShadow(m_bodyPacket.m_rightFoot);
    m_bodyPacket.m_rightElbowMat = getRightLeg()->m_elbowMat;
    m_bodyPacket.cd.m_rightFootCollided = getRightLeg()->getFoot()->collidedWithNotOwnCharacter();
    m_bodyPacket.cd.m_rightFootCollidedLast = getRightLeg()->getFoot()->previousCollidedWithNotOwnCharacter();

    NmRsCBUGrab* grabTask = (NmRsCBUGrab*)m_cbuParent->m_tasks[bvid_grab];
    Assert(grabTask);
    m_bodyPacket.cd.m_leftHandCollided = false;
    m_bodyPacket.cd.m_rightHandCollided = false;
    if (grabTask->isActive() && grabTask->getGrabingLeft())
      m_bodyPacket.cd.m_leftHandCollided = true;
    if (grabTask->isActive() && grabTask->getGrabingRight())
      m_bodyPacket.cd.m_rightHandCollided = m_body->getRightArm()->getHand()->collidedWithNotOwnCharacter();
    // lower spine
    getSpine()->getSpine0()->saveToShadow(m_bodyPacket.m_spine0);
    getSpine()->getPelvisPart()->saveToShadow(m_bodyPacket.m_buttocks);

    m_bodyPacket.m_newWaistHeight = 0.0f; // filled during balance solve, fed back into pelvis packet once solve done
  }

  void NmRsCBUDynamicBalancer::readBodyPacketResults()
  {
    getLeftLegInputData()->getHip()->setDesiredAngles(m_bodyPacket.m_leftHip.m_desiredLean1, m_bodyPacket.m_leftHip.m_desiredLean2, m_bodyPacket.m_leftHip.m_desiredTwist);
    getLeftLegInputData()->getKnee()->setDesiredAngle(m_bodyPacket.m_leftKnee.m_desiredAngle);
    getLeftLegInputData()->getAnkle()->setDesiredAngles(m_bodyPacket.m_leftAnkle.m_desiredLean1, m_bodyPacket.m_leftAnkle.m_desiredLean2, m_bodyPacket.m_leftAnkle.m_desiredTwist);

    getRightLegInputData()->getHip()->setDesiredAngles(m_bodyPacket.m_rightHip.m_desiredLean1, m_bodyPacket.m_rightHip.m_desiredLean2, m_bodyPacket.m_rightHip.m_desiredTwist);
    getRightLegInputData()->getKnee()->setDesiredAngle(m_bodyPacket.m_rightKnee.m_desiredAngle);
    getRightLegInputData()->getAnkle()->setDesiredAngles(m_bodyPacket.m_rightAnkle.m_desiredLean1, m_bodyPacket.m_rightAnkle.m_desiredLean2, m_bodyPacket.m_rightAnkle.m_desiredTwist);

    getSpineInputData()->getSpine0()->setDesiredAngles(m_bodyPacket.m_spine0.m_desiredLean1, m_bodyPacket.m_spine0.m_desiredLean2, m_bodyPacket.m_spine0.m_desiredTwist);

    m_pelvisState.m_waistHeight = m_bodyPacket.m_newWaistHeight;
  }

  void NmRsCBUDynamicBalancer::setPostureData(const NmRsCBUDynBal_FootState &foot)
  {
    switch (foot.m_footChoice)
    {
      //Maybe should try out having stance leg not in grounded chain if not in contact
      //Pros: stops ankle pointing when not in contact
      //Cons: Jitter?  Stance leg will not keep contact as long?
    case NmRsCBUDynBal_FootState::kLeftStep:
      m_character->setLeftFootConnected(false);
      m_character->setRightFootConnected(true);
      NM_RS_DBG_LOGF(L"DYNC| STEP LEFT");
      break;

    case NmRsCBUDynBal_FootState::kRightStep:
      m_character->setLeftFootConnected(true);
      m_character->setRightFootConnected(false);
      NM_RS_DBG_LOGF(L"DYNC| STEP RIGHT");
      break;

    case NmRsCBUDynBal_FootState::kNotStepping://handled automatically
      NM_RS_DBG_LOGF(L"DYNC| NOT STEPPING");
      break;
    }

    if (m_roPacket.m_movingFloor)
    {
    m_character->setFloorVelocityFromColliderRefFrameVel();//CatchFall and RollDownStairs can overwrite this value
    m_character->m_floorAcceleration = m_bodyPacket.m_floorAcceleration;
      rage::Vector3 floorAcceleration(m_bodyPacket.m_floorAcceleration);
      if (floorAcceleration.Mag() > 20.0f)//CRASH
        floorAcceleration.Zero();
      if (floorAcceleration.Mag() > m_character->m_uprightConstraint.stayUpAccMax)
      {
        floorAcceleration.Normalize();
        floorAcceleration *= m_character->m_uprightConstraint.stayUpAccMax;
      }
      //mmmm makes no sense for the gravity vector to be leaning for other behaviours
      //except that StayUpright and gravityOpposition use it.
      //mmmmTodo: allow the lean from leanInDirection to be sent through aswell
      m_character->m_gUp = m_character->m_gUpReal + (m_character->m_uprightConstraint.stayUpAcc*floorAcceleration / 9.81f);
      m_character->m_gUp.Normalize();
    }//if (m_roPacket.m_movingFloor)
  }

  void NmRsCBUDynamicBalancer::autoLeanInDirection(const rage::Vector3& dir, float amount)
  {
    m_autoLeanParams.m_mode = autoLeanParams::eALDirection;
    m_autoLeanParams.m_vec.Normalize(dir);
    m_autoLeanParams.m_amount = amount;
  }

  void NmRsCBUDynamicBalancer::autoLeanRandom(float amountMin, float amountMax, float changeTimeMin, float changeTimeMax)
  {
    m_autoLeanParams.m_mode = autoLeanParams::eALRandom;
    m_autoLeanParams.m_amountMin = amountMin;
    m_autoLeanParams.m_amountMax = amountMax;
    m_autoLeanParams.m_changeTimeMin = changeTimeMin;
    m_autoLeanParams.m_changeTimeMax = changeTimeMax;
  }

  void NmRsCBUDynamicBalancer::autoLeanToPosition(const rage::Vector3& pos, float amount)
  {
    m_autoLeanParams.m_mode = autoLeanParams::eALToPosition;
    m_autoLeanParams.m_vec.Set(pos);
    m_autoLeanParams.m_amount = amount;
  }

  void NmRsCBUDynamicBalancer::autoLeanToObject(int objLevelIndex, int objBoundIndex, const rage::Vector3& offset, float amount)
  {
    m_autoLeanParams.m_mode = autoLeanParams::eALToObject;
    m_autoLeanParams.m_vec.Set(offset);
    m_autoLeanParams.m_amount = amount;
    m_autoLeanParams.m_levelIndex = objLevelIndex;
    m_autoLeanParams.m_boundIndex = objBoundIndex;
  }

  void NmRsCBUDynamicBalancer::autoLeanChangeAmountOverTime(float byAmt, float time)//mmmmNote This is currently unused
  {
    Assert(rage::Abs(byAmt) > 1e-10f);
    m_autoLeanParams.m_amountDelta = (time / byAmt);
    m_autoLeanParams.m_amountOverTimeMode = autoLeanParams::eAMModify;
  }

  void NmRsCBUDynamicBalancer::autoLeanCancel()
  {
    m_autoLeanParams.m_mode = autoLeanParams::eALNone;
  }

  void NmRsCBUDynamicBalancer::updateAutoLean(float timeStep)
  {
    Assert(m_autoLeanParams.m_mode < autoLeanParams::eALInvalid && 
      m_autoLeanParams.m_mode >= autoLeanParams::eALNone);
    Assert(m_autoLeanParams.m_amountOverTimeMode < autoLeanParams::eAMInvalid && 
      m_autoLeanParams.m_amountOverTimeMode >= autoLeanParams::eAMNone);

    if (m_autoLeanParams.m_mode == autoLeanParams::eALNone)
    {
      m_upOffset.Zero();
      return;
    }

    switch (m_autoLeanParams.m_mode)
    {
    case autoLeanParams::eALDirection:
      m_upOffset.SetScaled(m_autoLeanParams.m_vec, m_autoLeanParams.m_amount);
      break;

    case autoLeanParams::eALRandom:
      if (m_autoLeanParams.m_timeRemaining <= 0.f)
      {
        float angle = m_character->getRandom().GetRanged(-PI, PI);
        rage::Vector3 xAxis(1.0f,0.f,0.f);
        rage::Vector3 otherHorizontalAxis(1.f,1.f,1.f);
        otherHorizontalAxis -= xAxis;
        otherHorizontalAxis -= m_character->m_gUp;
        xAxis *= rage::Sinf(angle);
        otherHorizontalAxis *= rage::Cosf(angle);
        m_autoLeanParams.m_vec = xAxis;
        m_autoLeanParams.m_vec += otherHorizontalAxis;
        m_autoLeanParams.m_amount = m_character->getRandom().GetRanged(m_autoLeanParams.m_amountMin,m_autoLeanParams.m_amountMax);
        m_autoLeanParams.m_timeRemaining = m_character->getRandom().GetRanged(m_autoLeanParams.m_changeTimeMin,m_autoLeanParams.m_changeTimeMax);
      }
      m_autoLeanParams.m_timeRemaining -= timeStep;
      m_upOffset.SetScaled(m_autoLeanParams.m_vec, m_autoLeanParams.m_amount);
      break;

    case autoLeanParams::eALToPosition:
      m_upOffset.Subtract(m_autoLeanParams.m_vec, m_character->m_COM);

      // have we arrived?
      if (m_upOffset.Mag2() <= 0.1f)
      {
        autoLeanCancel();
        return;
      }

      m_upOffset.Normalize();
      m_upOffset.Scale(m_autoLeanParams.m_amount);
      break;

    case autoLeanParams::eALToObject:
      {
        //mmmmtodo if levelIndex is removed cancel as will then lean to world space m_vec - or take last world good?
        Assert(m_character->getLevel()->IsInLevel(m_autoLeanParams.m_levelIndex) || m_autoLeanParams.m_levelIndex == -1);

        rage::Vector3 ctd;
        m_character->boundToWorldSpace(&ctd, m_autoLeanParams.m_vec, m_autoLeanParams.m_levelIndex, m_autoLeanParams.m_boundIndex);

        m_upOffset.Subtract(ctd, m_character->m_COM);

        if (m_upOffset.Mag2() <= 0.1f)
        {
          autoLeanCancel();
          return;
        }

        m_upOffset.Normalize();
        m_upOffset.Scale(m_autoLeanParams.m_amount);
      }
      break;

    default:
      break;
    }

    if (getLeftLeg()->getFoot()->collided() || getRightLeg()->getFoot()->collided())
    {
      float dot = m_roPacket.m_gUp.Dot(m_upOffset);
      m_roPacket.m_gUp += m_upOffset - m_roPacket.m_gUp*dot;
      m_roPacket.m_gUp.Normalize();
    }


    if (m_autoLeanParams.m_amountOverTimeMode == autoLeanParams::eAMModify)//mmmmNote This is currently unused
    {
      m_autoLeanParams.m_amount += m_autoLeanParams.m_amountDelta;
      m_autoLeanParams.m_timeRemaining -= timeStep;
      if (m_autoLeanParams.m_timeRemaining <= 0.0f)
        m_autoLeanParams.m_amountOverTimeMode = autoLeanParams::eAMNone;
    }
  }

  void NmRsCBUDynamicBalancer::autoLeanHipsInDirection(const rage::Vector3& dir, float amount)
  {
    m_autoLeanHipsParams.m_mode = autoLeanParams::eALDirection;
    m_autoLeanHipsParams.m_vec.Normalize(dir);
    m_autoLeanHipsParams.m_amount = amount;
  }

  void NmRsCBUDynamicBalancer::autoLeanHipsRandom(float amountMin, float amountMax, float changeTimeMin, float changeTimeMax)
  {
    m_autoLeanHipsParams.m_mode = autoLeanParams::eALRandom;
    m_autoLeanHipsParams.m_amountMin = amountMin;
    m_autoLeanHipsParams.m_amountMax = amountMax;
    m_autoLeanHipsParams.m_changeTimeMin = changeTimeMin;
    m_autoLeanHipsParams.m_changeTimeMax = changeTimeMax;
  }

  void NmRsCBUDynamicBalancer::autoLeanHipsToPosition(const rage::Vector3& pos, float amount)
  {
    m_autoLeanHipsParams.m_mode = autoLeanParams::eALToPosition;
    m_autoLeanHipsParams.m_vec.Set(pos);
    m_autoLeanHipsParams.m_amount = amount;
  }

  void NmRsCBUDynamicBalancer::autoLeanHipsToObject(int objLevelIndex, int objBoundIndex, const rage::Vector3& offset, float amount)
  {
    m_autoLeanHipsParams.m_mode = autoLeanParams::eALToObject;
    m_autoLeanHipsParams.m_vec.Set(offset);
    m_autoLeanHipsParams.m_amount = amount;
    m_autoLeanHipsParams.m_levelIndex = objLevelIndex;
    m_autoLeanHipsParams.m_boundIndex = objBoundIndex;
  }

  void NmRsCBUDynamicBalancer::autoLeanHipsChangeAmountOverTime(float byAmt, float time)//mmmmNote This is currently unused
  {
    Assert(rage::Abs(byAmt) > 1e-10f);
    m_autoLeanHipsParams.m_amountDelta = (time / byAmt);
    m_autoLeanHipsParams.m_amountOverTimeMode = autoLeanParams::eAMModify;
  }

  void NmRsCBUDynamicBalancer::autoLeanHipsCancel()
  {
    m_autoLeanHipsParams.m_mode = autoLeanParams::eALNone;
  }

  void NmRsCBUDynamicBalancer::updateAutoLeanHips(float timeStep)
  {
    Assert(m_autoLeanHipsParams.m_mode < autoLeanParams::eALInvalid && 
      m_autoLeanHipsParams.m_mode >= autoLeanParams::eALNone);
    Assert(m_autoLeanHipsParams.m_amountOverTimeMode < autoLeanParams::eAMInvalid && 
      m_autoLeanHipsParams.m_amountOverTimeMode >= autoLeanParams::eAMNone);

    if (m_autoLeanHipsParams.m_mode == autoLeanParams::eALNone)
    {
      m_upOffsetHips.Zero();
      return;
    }

    switch (m_autoLeanHipsParams.m_mode)
    {
    case autoLeanParams::eALDirection:
      m_upOffsetHips.SetScaled(m_autoLeanHipsParams.m_vec, m_autoLeanHipsParams.m_amount);
      break;

    case autoLeanParams::eALRandom:
      if (m_autoLeanHipsParams.m_timeRemaining <= 0.f)
      {
        float angle = m_character->getRandom().GetRanged(-PI, PI);
        rage::Vector3 xAxis(1.0f,0.f,0.f);
        rage::Vector3 otherHorizontalAxis(1.f,1.f,1.f);
        otherHorizontalAxis -= xAxis;
        otherHorizontalAxis -= m_character->m_gUp;
        xAxis *= rage::Sinf(angle);
        otherHorizontalAxis *= rage::Cosf(angle);
        m_autoLeanHipsParams.m_vec = xAxis;
        m_autoLeanHipsParams.m_vec += otherHorizontalAxis;
        m_autoLeanHipsParams.m_amount = m_character->getRandom().GetRanged(m_autoLeanHipsParams.m_amountMin,m_autoLeanHipsParams.m_amountMax);
        m_autoLeanHipsParams.m_timeRemaining = m_character->getRandom().GetRanged(m_autoLeanHipsParams.m_changeTimeMin,m_autoLeanHipsParams.m_changeTimeMax);
      }
      m_autoLeanHipsParams.m_timeRemaining -= timeStep;
      m_upOffsetHips.SetScaled(m_autoLeanHipsParams.m_vec, m_autoLeanHipsParams.m_amount);
      break;

    case autoLeanParams::eALToPosition:
      m_upOffsetHips.Subtract(m_autoLeanHipsParams.m_vec, m_character->m_COM);

      // have we arrived?
      if (m_upOffsetHips.Mag2() <= 0.1f)
      {
        autoLeanHipsCancel();
        return;
      }

      m_upOffsetHips.Normalize();
      m_upOffsetHips.Scale(m_autoLeanHipsParams.m_amount);
      break;

    case autoLeanParams::eALToObject:
      {
        //mmmmtodo if levelIndex is removed cancel as will then lean to world space m_vec - or take last world good?
        Assert(m_character->getLevel()->IsInLevel(m_autoLeanHipsParams.m_levelIndex) || m_autoLeanHipsParams.m_levelIndex == -1);

        rage::Vector3 ctd;
        m_character->boundToWorldSpace(&ctd, m_autoLeanHipsParams.m_vec, m_autoLeanHipsParams.m_levelIndex, m_autoLeanHipsParams.m_boundIndex);

        m_upOffsetHips.Subtract(ctd, m_character->m_COM);

        if (m_upOffsetHips.Mag2() <= 0.1f)
        {
          autoLeanHipsCancel();
          return;
        }

        m_upOffsetHips.Normalize();
        m_upOffsetHips.Scale(m_autoLeanHipsParams.m_amount);
      }
      break;

    default:
      break;
    }

    if (getLeftLeg()->getFoot()->collided() || getRightLeg()->getFoot()->collided())
    {
      rage::Vector3 leanHipgUp = m_roPacket.m_leanHipgUp;
      float dot = leanHipgUp.Dot(m_upOffsetHips);
      leanHipgUp += m_upOffsetHips - leanHipgUp*dot;
      leanHipgUp.Normalize();
      m_roPacket.m_leanHipgUp = leanHipgUp;
      m_roPacket.m_leanHipgUp.Normalize();
    }


    if (m_autoLeanHipsParams.m_amountOverTimeMode == autoLeanParams::eAMModify)//mmmmNote This is currently unused
    {
      m_autoLeanHipsParams.m_amount += m_autoLeanHipsParams.m_amountDelta;
      m_autoLeanHipsParams.m_timeRemaining -= timeStep;
      if (m_autoLeanHipsParams.m_timeRemaining <= 0.0f)
        m_autoLeanHipsParams.m_amountOverTimeMode = autoLeanParams::eAMNone;
    }
  }

  void NmRsCBUDynamicBalancer::autoLeanForceInDirection(const rage::Vector3& dir, float amount, int bodyPart)
  {
    m_autoLeanForceParams.m_mode = autoLeanParams::eALDirection;
    m_autoLeanForceParams.m_vec.Normalize(dir);
    m_autoLeanForceParams.m_amount = amount;
    m_autoLeanForceParams.m_bodyPart = bodyPart;
  }

  void NmRsCBUDynamicBalancer::autoLeanForceRandom(float amountMin, float amountMax, float changeTimeMin, float changeTimeMax, int bodyPart)
  {
    m_autoLeanForceParams.m_mode = autoLeanParams::eALRandom;
    m_autoLeanForceParams.m_amountMin = amountMin;
    m_autoLeanForceParams.m_amountMax = amountMax;
    m_autoLeanForceParams.m_changeTimeMin = changeTimeMin;
    m_autoLeanForceParams.m_changeTimeMax = changeTimeMax;
    m_autoLeanForceParams.m_bodyPart = bodyPart;
  }

  void NmRsCBUDynamicBalancer::autoLeanForceToPosition(const rage::Vector3& pos, float amount, int bodyPart)
  {
    m_autoLeanForceParams.m_mode = autoLeanParams::eALToPosition;
    m_autoLeanForceParams.m_vec.Set(pos);
    m_autoLeanForceParams.m_amount = amount;
    m_autoLeanForceParams.m_bodyPart = bodyPart;
  }

  void NmRsCBUDynamicBalancer::autoLeanForceToObject(int objLevelIndex, int objBoundIndex, const rage::Vector3& offset, float amount, int bodyPart)
  {
    m_autoLeanForceParams.m_mode = autoLeanParams::eALToObject;
    m_autoLeanForceParams.m_vec.Set(offset);
    m_autoLeanForceParams.m_amount = amount;
    m_autoLeanForceParams.m_levelIndex = objLevelIndex;
    m_autoLeanForceParams.m_boundIndex = objBoundIndex;
    m_autoLeanForceParams.m_bodyPart = bodyPart;
  }

  void NmRsCBUDynamicBalancer::autoLeanForceChangeAmountOverTime(float byAmt, float time)//mmmmNote This is currently unused
  {
    Assert(rage::Abs(byAmt) > 1e-10f);
    m_autoLeanForceParams.m_amountDelta = (time / byAmt);
    m_autoLeanForceParams.m_amountOverTimeMode = autoLeanParams::eAMModify;
  }

  void NmRsCBUDynamicBalancer::autoLeanForceCancel()
  {
    m_autoLeanForceParams.m_mode = autoLeanParams::eALNone;
  }

  void NmRsCBUDynamicBalancer::updateAutoLeanForce(float timeStep)
  {
    Assert(m_autoLeanForceParams.m_mode < autoLeanParams::eALInvalid && 
      m_autoLeanForceParams.m_mode >= autoLeanParams::eALNone);
    Assert(m_autoLeanForceParams.m_amountOverTimeMode < autoLeanParams::eAMInvalid && 
      m_autoLeanForceParams.m_amountOverTimeMode >= autoLeanParams::eAMNone);

    if (m_autoLeanForceParams.m_mode == autoLeanParams::eALNone)
    {
      m_upOffsetForce.Zero();
      return;
    }

    switch (m_autoLeanForceParams.m_mode)
    {
    case autoLeanParams::eALDirection:
      m_upOffsetForce.SetScaled(m_autoLeanForceParams.m_vec, m_autoLeanForceParams.m_amount);
      break;

    case autoLeanParams::eALRandom:
      if (m_autoLeanForceParams.m_timeRemaining <= 0.f)
      {
        float angle = m_character->getRandom().GetRanged(-PI, PI);
        rage::Vector3 xAxis(1.0f,0.f,0.f);
        rage::Vector3 otherHorizontalAxis(1.f,1.f,1.f);
        otherHorizontalAxis -= xAxis;
        otherHorizontalAxis -= m_character->m_gUp;
        xAxis *= rage::Sinf(angle);
        otherHorizontalAxis *= rage::Cosf(angle);
        m_autoLeanForceParams.m_vec = xAxis;
        m_autoLeanForceParams.m_vec += otherHorizontalAxis;
        m_autoLeanForceParams.m_amount = m_character->getRandom().GetRanged(m_autoLeanForceParams.m_amountMin,m_autoLeanForceParams.m_amountMax);
        m_autoLeanForceParams.m_timeRemaining = m_character->getRandom().GetRanged(m_autoLeanForceParams.m_changeTimeMin,m_autoLeanForceParams.m_changeTimeMax);
      }
      m_autoLeanForceParams.m_timeRemaining -= timeStep;
      m_upOffsetForce.SetScaled(m_autoLeanForceParams.m_vec, m_autoLeanForceParams.m_amount);
      break;

    case autoLeanParams::eALToPosition:
      m_upOffsetForce.Subtract(m_autoLeanForceParams.m_vec, m_character->m_COM);

      // have we arrived?
      if (m_upOffsetForce.Mag2() <= 0.1f)
      {
        autoLeanForceCancel();
        return;
      }

      m_upOffsetForce.Normalize();
      m_upOffsetForce.Scale(m_autoLeanForceParams.m_amount);
      break;

    case autoLeanParams::eALToObject:
      {
        //mmmmtodo if levelIndex is removed cancel as will then lean to world space m_vec - or take last world good?
        Assert(m_character->getLevel()->IsInLevel(m_autoLeanForceParams.m_levelIndex) || m_autoLeanForceParams.m_levelIndex == -1);

        rage::Vector3 ctd;
        m_character->boundToWorldSpace(&ctd, m_autoLeanForceParams.m_vec, m_autoLeanForceParams.m_levelIndex, m_autoLeanForceParams.m_boundIndex);

        m_upOffsetForce.Subtract(ctd, m_character->m_COM);

        if (m_upOffsetForce.Mag2() <= 0.1f)
        {
          autoLeanForceCancel();
          return;
        }

        m_upOffsetForce.Normalize();
        m_upOffsetForce.Scale(m_autoLeanForceParams.m_amount);
      }
      break;

    default:
      return;
    }

    if ((getLeftLeg()->getFoot()->collided() || getRightLeg()->getFoot()->collided()))// && getSpine()->getPelvisPart()->getLinearVelocity().Mag() < 1.f)
    {
      //apply the force 
      rage::Vector3 leanModifiedgUp4Force = m_character->m_gUp;
      float dot = leanModifiedgUp4Force.Dot(m_upOffsetForce);
      leanModifiedgUp4Force += m_upOffsetForce - leanModifiedgUp4Force*dot;
      leanModifiedgUp4Force.Normalize();

      rage::Vector3 force = rage::phSimulator::GetGravity() * m_character->getTotalMass();
      force -= leanModifiedgUp4Force*force.Dot(leanModifiedgUp4Force);

      leanModifiedgUp4Force = m_character->getEngine()->getUpVector();
      force -= leanModifiedgUp4Force*force.Dot(leanModifiedgUp4Force);

      m_character->getGenericPartByIndex(m_autoLeanForceParams.m_bodyPart)->applyForce(force);
    }

    if (m_autoLeanForceParams.m_amountOverTimeMode == autoLeanParams::eAMModify)//mmmmNote This is currently unused
    {
      m_autoLeanForceParams.m_amount += m_autoLeanForceParams.m_amountDelta;
      m_autoLeanForceParams.m_timeRemaining -= timeStep;
      if (m_autoLeanForceParams.m_timeRemaining <= 0.0f)
        m_autoLeanForceParams.m_amountOverTimeMode = autoLeanParams::eAMNone;
    }
  }

  void NmRsCBUDynamicBalancer::updateModulePackets(NmRsCBUDynBal_FootState& foot, NmRsCBUDynBal_PelvisState& pelvis)
  {
    pelvis.m_centreOfFeet = foot.m_centreOfFeet;
    pelvis.m_leftFootBalance = foot.state.m_leftFootBalance;
    pelvis.m_rightFootBalance = foot.state.m_rightFootBalance;

    const CharacterConfiguration& cc = m_character->getCharacterConfiguration();
    m_roPacket.m_legStraightness = cc.m_legStraightness + m_legStraightnessModifier;
#if NM_STEP_UP
    m_roPacket.m_stepUp = true;
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    m_roPacket.m_stepUp = m_character->stepUp;
#endif
#endif
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    m_roPacket.m_pushOffBackwards = m_character->pushOffBackwards;
#endif

#if NM_NEW_BALANCER
    m_roPacket.m_pushOff = true;
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    m_roPacket.m_pushOff = m_character->pushOff;
#endif

#else 
    m_roPacket.m_pushOff = false;
#endif
    if (m_standUp)
    {
      float higher = 0.f;//mmmmhere
      //if (m_character->m_COMvelMag > 0.3f)
      higher = rage::Min(2.f*m_character->m_COMvelRelativeMag,1.f);
      m_roPacket.m_legStraightness += higher*(1.f - cc.m_legStraightness);
    }

    // copy in custom variables
    pelvis.m_hipPitch = m_hipPitch + cc.m_defaultHipPitch; // Sum to hip pitch measured from the zero pose and the hip pitch specified by behaviours/game. 
    pelvis.m_hipRoll = m_hipRoll;
    pelvis.m_hipYaw = m_hipYaw;
    pelvis.m_useCustomTurnDir = m_useCustomTurnDir;
    pelvis.m_customTurnDir = m_customTurnDir;
  }

  void NmRsCBUDynamicBalancer::initialiseRAGETaskParams()
  {
    memset(&m_footTaskParams, 0, sizeof(m_footTaskParams));
    memset(&m_pelvisTaskParams, 0, sizeof(m_pelvisTaskParams));
    memset(&m_solveTaskParams, 0, sizeof(m_solveTaskParams));

    m_footTaskHandle = 0;
    m_pelvisTaskHandle = 0;
    m_solveTaskHandle = 0;
  }

  void NmRsCBUDynamicBalancer::updateRAGETaskParams()
  {
    m_footTaskParams.ReadOnlyCount = 2;

    m_footTaskParams.ReadOnly[0].Size = sizeof(NmRsCBUDynBal_ReadOnly);
    m_footTaskParams.ReadOnly[0].Data = &m_roPacket;

    m_footTaskParams.ReadOnly[1].Size  = sizeof(NmRsCBUDynBal_BodyPacket);
    m_footTaskParams.ReadOnly[1].Data = &m_bodyPacket;

    m_footTaskParams.Input.Size = sizeof(NmRsCBUDynBal_FootState);
    m_footTaskParams.Input.Data = &m_footState;

    m_footTaskParams.Output.Size = m_footTaskParams.Input.Size;
    m_footTaskParams.Output.Data = m_footTaskParams.Input.Data;

    // --

    m_pelvisTaskParams.ReadOnlyCount = 2;

    m_pelvisTaskParams.ReadOnly[0].Size = sizeof(NmRsCBUDynBal_ReadOnly);
    m_pelvisTaskParams.ReadOnly[0].Data = &m_roPacket;

    m_pelvisTaskParams.ReadOnly[1].Size  = sizeof(NmRsCBUDynBal_BodyPacket);
    m_pelvisTaskParams.ReadOnly[1].Data = &m_bodyPacket;

    m_pelvisTaskParams.Input.Size  = sizeof(NmRsCBUDynBal_PelvisState);
    m_pelvisTaskParams.Input.Data = &m_pelvisState;

    m_pelvisTaskParams.Output.Size = m_pelvisTaskParams.Input.Size;
    m_pelvisTaskParams.Output.Data = m_pelvisTaskParams.Input.Data;

    // --

    m_solveTaskParams.ReadOnly[0].Size = sizeof(NmRsCBUDynBal_ReadOnly);
    m_solveTaskParams.ReadOnly[0].Data = &m_roPacket;

    m_solveTaskParams.ReadOnly[1].Size = sizeof(NmRsCBUDynBal_FootState);
    m_solveTaskParams.ReadOnly[1].Data = &m_footState;

    m_solveTaskParams.ReadOnly[2].Size = sizeof(NmRsCBUDynBal_PelvisState);
    m_solveTaskParams.ReadOnly[2].Data = &m_pelvisState;
    m_solveTaskParams.ReadOnlyCount = 3;

    m_solveTaskParams.Input.Size  = sizeof(NmRsCBUDynBal_BodyPacket);
    m_solveTaskParams.Input.Data = &m_bodyPacket;

    m_solveTaskParams.Output.Size = m_solveTaskParams.Input.Size;
    m_solveTaskParams.Output.Data = m_solveTaskParams.Input.Data;
  }

  void NmRsCBUDynamicBalancer::probeForUnevenTerrain()
  {
    rage::Vector3 leftStart, leftEnd;
    rage::Vector3 rightStart, rightEnd;

    // left and right foot segments are defined as a section of space
    // beginning just above the current foot position and extending down 
    rage::Vector3 lKneePos = getLeftLeg()->getKnee()->getJointPosition();
    rage::Vector3 rKneePos = getRightLeg()->getKnee()->getJointPosition();
    float heightL = lKneePos.Dot(m_roPacket.m_gUp);
    float heightR = rKneePos.Dot(m_roPacket.m_gUp);
    leftStart.Set(m_footState.m_leftFootPos);
    rightStart.Set(m_footState.m_rightFootPos);
#if STOP_TRIPPING_OLD      
    //Stop tripping near edges.
    //The probe start position was the predicted knee position. 
    //Now if the leg is the swing leg and the foot is behind the stance foot (w.r.t direction of travel) 
    //take the probe start as the swing leg ankle. 
    //i.e. the swing leg ignores the vertical drop/slope for the first half of it's swing phase. 
    //The stance leg doesn't therefore start bending to match the hip height to the average height of the 2 probes until after half the swing phase
    //This also makes the down slope walking more upright and causes the character to fall backwards on to the slope more.
    //It also reduces the large straight legged stepping down slopes
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    if (m_character->oldTripping)
#endif
    {
      if (m_footState.state.m_leftFootBalance)
      {
        rage::Vector3 left2right = getRightLeg()->getFoot()->getPosition();
        left2right -= getLeftLeg()->getFoot()->getPosition();
        if (m_character->m_COMvelRelative.Dot(left2right) < 0.f)//relative velocity is one frame behind
          rightStart = getRightLeg()->getAnkle()->getJointPosition();
      }
      if (m_footState.state.m_rightFootBalance)
      {
        rage::Vector3 left2right = getRightLeg()->getFoot()->getPosition();
        left2right -= getLeftLeg()->getFoot()->getPosition();
        if (m_character->m_COMvelRelative.Dot(left2right) > 0.f)//relative velocity is one frame behind
          leftStart = getLeftLeg()->getAnkle()->getJointPosition();
      }
    }
#endif
    //For teeter: Put probes back so they return the edge and don't make the balancer start stepping down  
    if (m_roPacket.m_teeter)
    {
      rage::Vector3 leftStartCopy(leftStart);
      rage::Vector3 rightStartCopy(rightStart);
      m_roPacket.levelVector(leftStartCopy,m_roPacket.m_gUpReal.Dot(m_roPacket.m_pos1stContact));
      m_roPacket.levelVector(rightStartCopy,m_roPacket.m_gUpReal.Dot(m_roPacket.m_pos1stContact));
      float exclusionZone = 0.05f;
      if (m_roPacket.m_normal1stContact.Dot(leftStartCopy-m_roPacket.m_pos1stContact-m_roPacket.m_normal1stContact*exclusionZone)*m_roPacket.m_sideOfPlane > 0.f)//increase offset
      {
        leftStartCopy -= m_roPacket.m_normal1stContact.Dot(leftStartCopy - m_roPacket.m_pos1stContact-m_roPacket.m_normal1stContact*exclusionZone)* m_roPacket.m_normal1stContact;
        leftStart = leftStartCopy;
      }
      if (m_roPacket.m_normal1stContact.Dot(rightStartCopy-m_roPacket.m_pos1stContact-m_roPacket.m_normal1stContact*exclusionZone)*m_roPacket.m_sideOfPlane > 0.f)//increase offset
      {
        rightStartCopy -= m_roPacket.m_normal1stContact.Dot(rightStartCopy - m_roPacket.m_pos1stContact-m_roPacket.m_normal1stContact*exclusionZone)* m_roPacket.m_normal1stContact;
        rightStart = rightStartCopy;
      }
    }

    m_roPacket.levelVector(leftStart, heightL);
    m_roPacket.levelVector(rightStart, heightR);
    leftEnd.AddScaled(leftStart, m_roPacket.m_gUpReal, -cFootProbeRayLower);
    rightEnd.AddScaled(rightStart, m_roPacket.m_gUpReal, -cFootProbeRayLower);

    // generate segments representing our ray probes
    rage::phSegment leftProbe, rightProbe;
    leftProbe.Set(leftStart, leftEnd);
    rightProbe.Set(rightStart, rightEnd);
#if ART_ENABLE_BSPY
    bspyTaskScratchpadAuto("Probe", leftStart);
    bspyTaskScratchpadAuto("Probe", leftEnd);
    bspyTaskScratchpadAuto("Probe", rightStart);
    bspyTaskScratchpadAuto("Probe", rightEnd);
#endif

    rage::u8 stateIncludeFlags = (
      rage::phLevelBase::STATE_FLAG_INACTIVE | 
      rage::phLevelBase::STATE_FLAG_ACTIVE |
      rage::phLevelBase::STATE_FLAG_FIXED);
    bool oldProbeHitLeft = m_roPacket.m_probeHitLeft;
    bool oldProbeHitRight = m_roPacket.m_probeHitRight;
    m_roPacket.m_probeHitLeft = m_character->probeRay(NmRsCharacter::pi_balLeft, leftProbe.A, leftProbe.B, stateIncludeFlags, TYPE_FLAGS_ALL, TYPE_FLAGS_ALL, TYPE_FLAGS_NONE, false);
    m_roPacket.m_probeHitRight = m_character->probeRay(NmRsCharacter::pi_balRight, rightProbe.A, rightProbe.B, stateIncludeFlags, TYPE_FLAGS_ALL, TYPE_FLAGS_ALL, TYPE_FLAGS_NONE, false);


    //mmmmtodo Implement the code below if it saves us any time.
    //if (m_footState.state.m_leftFootBalance)
    //{
    //  if (m_roPacket.m_probeHitLeft)
    //  {
    //    if ((m_character->getLevel()->GetState(intersectionLeft.GetInstance()->GetLevelIndex()) != rage::phLevelBase::OBJECTSTATE_ACTIVE))
    //    {
    //      stateIncludeFlags &= ~rage::phLevelBase::STATE_FLAG_ACTIVE;
    //    }
    //  }

    //}
    //else if (m_footState.state.m_rightFootBalance)
    //{

    //  if (m_roPacket.m_probeHitRight)
    //  {
    //    if ((m_character->getLevel()->GetState(intersectionRight.GetInstance()->GetLevelIndex()) != rage::phLevelBase::OBJECTSTATE_ACTIVE))
    //    {
    //      stateIncludeFlags &= ~rage::phLevelBase::STATE_FLAG_ACTIVE;
    //    }
    //  }

    //}

    if (m_character->m_movingFloor)
    {
      /* Get local velocity of support surface for each foot whose
      probe has hit something. */
      bool doneAVelocityProbe = m_doneAVelocityProbe;
      rage::Vector3 leftFloorVelocity, rightFloorVelocity;
      leftFloorVelocity.Zero();
      rightFloorVelocity.Zero();

      //For better consistency the floor velocity is calculated from the probes if they hit even if that foot is not balancing
      //mmmmtodo: should we zero the velocity/accleration:
      //  1) for times before the character has contacted the moving object?
      //  2) when the character has been airborne for a a certain time?
      if (/*m_footState.state.m_leftFootBalance && */m_roPacket.m_probeHitLeft && m_character->IsInstValid(NmRsCharacter::pi_balLeft))
      {
        //ignore velocity if hit say a gun
        bool allowVelFromProbe = true;
        if (m_character->getDontRegisterProbeVelocityActive())
        {
          rage::Vector3 objectSize = m_character->m_probeHitInstBoundingBoxSize[NmRsCharacter::pi_balLeft];
          float vol = objectSize.x * objectSize.y * objectSize.z; 
          allowVelFromProbe = PHLEVEL->IsFixed(m_character->m_probeHitInstLevelIndex[NmRsCharacter::pi_balLeft]) ||
            ((m_character->m_probeHitInstMass[NmRsCharacter::pi_balLeft] >= m_character->getDontRegisterProbeVelocityMassBelow())
            && (vol >= m_character->getDontRegisterProbeVelocityVolBelow()));
#if ART_ENABLE_BSPY && 0
          if (!allowVelFromProbe)
            bspyTaskScratchpadAuto("Left DontRegisterProbeVelocity", m_character->m_probeHitInstLevelIndex[NmRsCharacter::pi_balLeft]);
#endif
        }

        if (allowVelFromProbe)
        {
          m_doneAVelocityProbe = true;
          m_character->getVelocityOnInstance(m_character->m_probeHitInstLevelIndex[NmRsCharacter::pi_balLeft],m_character->m_probeHitPos[NmRsCharacter::pi_balLeft],&leftFloorVelocity);
#if ART_ENABLE_BSPY && 0
          bspyTaskScratchpadAuto("leftFloorVelocity", leftFloorVelocity);
#endif
        }
      }
      if (/*m_footState.state.m_rightFootBalance && */m_roPacket.m_probeHitRight && m_character->IsInstValid(NmRsCharacter::pi_balRight))
      {
        //ignore velocity if hit say a gun
        bool allowVelFromProbe = true;
        if (m_character->getDontRegisterProbeVelocityActive())
        {
          rage::Vector3 objectSize = m_character->m_probeHitInstBoundingBoxSize[NmRsCharacter::pi_balRight];
          float vol = objectSize.x * objectSize.y * objectSize.z; 
          allowVelFromProbe = PHLEVEL->IsFixed(m_character->m_probeHitInstLevelIndex[NmRsCharacter::pi_balRight]) ||
            ((m_character->m_probeHitInstMass[NmRsCharacter::pi_balRight] >= m_character->getDontRegisterProbeVelocityMassBelow())
            && (vol >= m_character->getDontRegisterProbeVelocityVolBelow()));
#if ART_ENABLE_BSPY && 0
          if (!allowVelFromProbe)
            bspyTaskScratchpadAuto("Right DontRegisterProbeVelocity", m_character->m_probeHitInstLevelIndex[NmRsCharacter::pi_balRight]);
#endif

        }

        if (allowVelFromProbe)
        {
          m_doneAVelocityProbe = true;
          m_character->getVelocityOnInstance(m_character->m_probeHitInstLevelIndex[NmRsCharacter::pi_balRight],m_character->m_probeHitPos[NmRsCharacter::pi_balRight],&rightFloorVelocity);
#if ART_ENABLE_BSPY && 0
          bspyTaskScratchpadAuto("rightFloorVelocity", rightFloorVelocity);
#endif
        }
      }

      /* Register floor velocity of the balancing foot.  Otherwise
      do nothing and use the last known velocity. */
      rage::Vector3 oldVelocity = m_bodyPacket.m_floorVelocity;
      if (m_roPacket.m_probeHitLeft /*&& m_footState.state.m_leftFootBalance*/ && m_roPacket.m_probeHitRight /*&& m_footState.state.m_rightFootBalance*/) 
      {
        //take the nearest velocity to the last one (should be take the velocity that will give the acceleration closest to the last one)
        if ((leftFloorVelocity - m_bodyPacket.m_floorVelocity).Mag() < (rightFloorVelocity - m_bodyPacket.m_floorVelocity).Mag())
          m_bodyPacket.m_floorVelocity = leftFloorVelocity;
        else
          m_bodyPacket.m_floorVelocity = rightFloorVelocity;
      }
      else if (m_roPacket.m_probeHitLeft /*&& m_footState.state.m_leftFootBalance*/) 
      {
        m_bodyPacket.m_floorVelocity = leftFloorVelocity;
      } 
      else if (m_roPacket.m_probeHitRight /*&& m_footState.state.m_rightFootBalance*/) 
      {
        m_bodyPacket.m_floorVelocity = rightFloorVelocity;
      }

      /* Smooth accelerations together. This is a balance between 
      responsiveness and jitter when stationary. */
      if (doneAVelocityProbe != m_doneAVelocityProbe)
        oldVelocity = m_bodyPacket.m_floorVelocity;
#if ART_ENABLE_BSPY && 0
      bspyTaskScratchpadAuto("oldVelocity", oldVelocity);
      bspyTaskScratchpadAuto("doneAVelocityProbe", doneAVelocityProbe);
      bspyTaskScratchpadAuto("m_doneAVelocityProbe", m_doneAVelocityProbe);
#endif

      static float smooth = 0.6f;//mmmTodo  NB: this smoothing is frame rate dependent
      if (m_doneAVelocityProbe)
      {
        rage::Vector3 leveledVelocityDifference; 
        leveledVelocityDifference = m_bodyPacket.m_floorVelocity - oldVelocity;
        m_roPacket.levelVectorReal(leveledVelocityDifference);
        m_bodyPacket.m_floorAcceleration += (leveledVelocityDifference / m_roPacket.m_timeStep - m_bodyPacket.m_floorAcceleration)*smooth;

        // if (m_numAccs < 3)
        // {
        //   m_numAccs ++;
        //   m_bodyPacket.m_floorAcceleration += (leveledVelocityDifference / m_roPacket.m_timeStep - m_bodyPacket.m_floorAcceleration)*0.6f;
        //   m_bodyPacket.m_floorAcceleration_2 = m_bodyPacket.m_floorAcceleration_1;
        //   m_bodyPacket.m_floorAcceleration_1 = leveledVelocityDifference / m_roPacket.m_timeStep;
        // }
        // else
        // {
        //   //butterworth filter
        //   //Make below into functions
        //   //Now smooth and diff the data
        //   float Pi = 3.1415926535897f;//from exel (fro checking only)
        //   float Fs = 30.f;  //Sample Frequency
        //   float Fc = 6.f;  //Cutoff Frequency
        //   float C = 1;      // correction factor;
        //   float Wc = (tan(Pi*Fc/Fs)/C);
        //   float K1 = sqrt(2.f)*Wc;//Butterworth Filter
        //   //float K1 = 2.f*Wc;//Critically damped Filter
        //   float K2 = Wc*Wc;
        //   float a0 = K2/(1+K1+K2);
        //   float a1 = 2.f*a0;
        //   float a2 = a0;
        //   float K3 = 2*a0/K2;
        //   float b1 = -2.f*a0 + K3;
        //   float b2 = 1 -2.f*a0 - K3;

        //   m_bodyPacket.m_floorAcceleration = a0* leveledVelocityDifference / m_roPacket.m_timeStep 
        //   + a1* m_bodyPacket.m_floorAcceleration_1 + a2* m_bodyPacket.m_floorAcceleration_2
        //   + b1* m_bodyPacket.m_floorSAcceleration_1 + b2* m_bodyPacket.m_floorSAcceleration_2;

        //   m_bodyPacket.m_floorAcceleration_2 = m_bodyPacket.m_floorAcceleration_1;
        //   m_bodyPacket.m_floorAcceleration_1 = leveledVelocityDifference / m_roPacket.m_timeStep;
        //   m_bodyPacket.m_floorSAcceleration_2 = m_bodyPacket.m_floorAcceleration_1;
        //   m_bodyPacket.m_floorSAcceleration_1 = m_bodyPacket.m_floorAcceleration;
        //  
        // }
      }
      //Update the relativeVelocities
      m_character->setFloorVelocityFromColliderRefFrameVel();
      m_bodyPacket.m_lvlCOMvelRelative = m_character->m_COMvel - m_character->getFloorVelocity();
      m_roPacket.levelVector(m_bodyPacket.m_lvlCOMvelRelative);
      m_bodyPacket.m_floorVelocity = m_character->getFloorVelocity();

      //Set the velocity based damping on the character to zero - otherwise the character moves slowly against the velocity direction of the moving floor
      if (m_bodyPacket.m_floorVelocity.Mag() > 0.5f)
      {
        m_character->getArticulatedBody()->SetLinearDecayRate(0.0f);
        m_character->getArticulatedBody()->SetAngularDecayRate(0.0f);
        
        //we turn off footSlipCompensation when the character is on a moving floor
        // because at the moment the angle of the foot is in global space not the moving floor space
        // Getting it in moving floor space would mean id-ing the up of the moving floor.
        //Really we should look at angular velocity
        //mmmmtodo re-enable on a fixed object
        if (!m_footSlipCompOnMovingFloor)
          m_character->enableFootSlipCompensationActive(false);
    }
    }//if (m_roPacket.m_movingFloor)

    rage::Vector3 leanOffset(-m_roPacket.m_gUp.x,-m_roPacket.m_gUp.y,m_roPacket.m_gUp.z);
    leanOffset -= m_roPacket.m_gUpReal;
    rage::Vector3 oldHitPosLeft = m_roPacket.m_leftFootProbeHitPos;
    if (oldProbeHitLeft)
      m_roPacket.m_leftFootProbeHitPos -= m_roPacket.m_gUp*0.03f;
    // write results into RO packet      
    if (m_roPacket.m_probeHitLeft)
    {
      m_roPacket.m_leftFootProbeHitPos = m_character->m_probeHitPos[NmRsCharacter::pi_balLeft] + m_roPacket.m_gUp*0.03f;
      m_roPacket.m_leftFootProbeNormal = leanOffset + m_character->m_probeHitNormal[NmRsCharacter::pi_balLeft];
    }
    else // if we hit nothing, make reasonable estimate of height.//mmmmtodo maybe make up a better hitPoint/Normal if probe was late?
    {
      // no hit means that we just leave the lower end of the segment in the 'hit'
      // result and set the normal to face upwards against gravity
      m_roPacket.m_leftFootProbeHitPos = leftStart;
      float mult = 1.f;
      if (m_character->m_underwater)
        mult = 0.8f;
      m_roPacket.m_leftFootProbeHitPos.AddScaled(m_roPacket.m_gUp, -mult*m_roPacket.m_legStraightness*m_roPacket.m_fullLegLength);
      m_roPacket.m_leftFootProbeNormal = leanOffset + m_roPacket.m_gUpReal;
      if (m_roPacket.m_probeHitRight)
        m_roPacket.m_leftFootProbeNormal = leanOffset + m_character->m_probeHitNormal[NmRsCharacter::pi_balRight];

    }
    m_roPacket.m_leftFootProbeNormal.Normalize();
    m_roPacket.m_leftFootHitPosVel = (m_roPacket.m_leftFootProbeHitPos - oldHitPosLeft)/m_roPacket.m_timeStep;

    rage::Vector3 oldHitPosRight = m_roPacket.m_rightFootProbeHitPos;
    if (oldProbeHitRight)
      m_roPacket.m_rightFootProbeHitPos -= m_roPacket.m_gUp*0.03f;
    if (m_roPacket.m_probeHitRight)
    {
      m_roPacket.m_rightFootProbeHitPos = m_character->m_probeHitPos[NmRsCharacter::pi_balRight] + m_roPacket.m_gUp*0.03f;
      m_roPacket.m_rightFootProbeNormal = leanOffset + m_character->m_probeHitNormal[NmRsCharacter::pi_balRight];
    }
    else // if we hit nothing, make reasonable estimate of height.//mmmmtodo maybe make up a better hitPoint/Normal if probe was late?
    {
      m_roPacket.m_rightFootProbeHitPos = rightStart;
      float mult = 1.f;
      if (m_character->m_underwater)
        mult = 0.8f;
      m_roPacket.m_rightFootProbeHitPos.AddScaled(m_roPacket.m_gUp, -mult*m_roPacket.m_legStraightness*m_roPacket.m_fullLegLength);
      m_roPacket.m_rightFootProbeNormal = leanOffset + m_roPacket.m_gUpReal;
      if (m_roPacket.m_probeHitLeft)
        m_roPacket.m_rightFootProbeNormal = leanOffset + m_character->m_probeHitNormal[NmRsCharacter::pi_balLeft];
    }
    m_roPacket.m_rightFootProbeNormal.Normalize();
    m_roPacket.m_rightFootHitPosVel = (m_roPacket.m_rightFootProbeHitPos - oldHitPosRight)/m_roPacket.m_timeStep;
#if NM_STEP_UP
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    if (m_character->stepUp)
#endif//NM_SCRIPTING
    {
      //Don't accept probes from another step up
      if (!m_footState.m_newStep)
      {
        if (m_footState.m_footChoice == NmRsCBUDynBal_FootState::kLeftStep &&
          oldHitPosLeft.z > oldHitPosRight.z + 0.1f && //last was a step              
          m_roPacket.m_leftFootProbeHitPos.z > oldHitPosLeft.z + 0.1f)//gone up another step 
        {
          m_roPacket.m_leftFootProbeHitPos = oldHitPosLeft;
          m_roPacket.m_leftFootHitPosVel.Zero();
#if ART_ENABLE_BSPY
          bool ID = true;
          bspyTaskScratchpadAuto("ID_2StepLeft", ID);
#endif
        }
        if (m_footState.m_footChoice == NmRsCBUDynBal_FootState::kRightStep &&
          oldHitPosRight.z > oldHitPosLeft.z + 0.1f && //last was a step              
          m_roPacket.m_rightFootProbeHitPos.z > oldHitPosRight.z + 0.1f)//gone up another step 
        {
          m_roPacket.m_rightFootProbeHitPos = oldHitPosRight;
          m_roPacket.m_rightFootHitPosVel.Zero();
#if ART_ENABLE_BSPY
          bool ID = true;
          bspyTaskScratchpadAuto("ID_2StepRight", ID);
#endif

        }
      }
    }
#endif //NM_STEP_UP

#if STOP_TRIPPING
    //Don't accept probes from a step down
    //Stop tripping near edges.
    //Causes very high stepping when going down a slope as the highest probe is continually saved
    //What about the NORMALS?
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    if (m_character->newTripping)
#endif
    {
      if (m_footState.m_footChoice == NmRsCBUDynBal_FootState::kRightStep)
      {
        if (m_roPacket.m_rightFootProbeHitPos.z < oldHitPosRight.z - 0.1f) //stepping down
        {
          rage::Vector3 left2right = getRightLeg()->getFoot()->getPosition();
          left2right -= getLeftLeg()->getFoot()->getPosition();
          if (m_character->m_COMvelRelative.Dot(left2right) < 0.f)//step foot behind stance leg
          {
            //put probe position of stepping leg to that of the stance
            m_roPacket.m_rightFootProbeHitPos.z = m_roPacket.m_leftFootProbeHitPos.z;
            if (getLeftLeg()->getFoot()->collidedWithEnvironment())
              m_roPacket.m_rightFootProbeHitPos.z = getLeftLeg()->getFoot()->getPosition().z;
            m_roPacket.m_rightFootHitPosVel.Zero();//mmmtodo

#if ART_ENABLE_BSPY
            bool ID = true;
            bspyTaskScratchpadAuto("ID_TripRight", ID);
#endif
          }
        }
      }
      if (m_footState.m_footChoice == NmRsCBUDynBal_FootState::kLeftStep)
      {
        if (m_roPacket.m_leftFootProbeHitPos.z < oldHitPosLeft.z - 0.1f) //stepping down
        {
          rage::Vector3 left2right = getRightLeg()->getFoot()->getPosition();
          left2right -= getLeftLeg()->getFoot()->getPosition();
          if (m_character->m_COMvelRelative.Dot(left2right) > 0.f)//step foot behind stance leg
          {
            //put probe position of stepping leg to that of the stance
            m_roPacket.m_leftFootProbeHitPos.z = m_roPacket.m_rightFootProbeHitPos.z;
            if (getRightLeg()->getFoot()->collidedWithEnvironment())
              m_roPacket.m_leftFootProbeHitPos.z = getRightLeg()->getFoot()->getPosition().z;
            m_roPacket.m_leftFootHitPosVel.Zero();//mmmtodo

#if ART_ENABLE_BSPY
            bool ID = true;
            bspyTaskScratchpadAuto("ID_TripRight", ID);
#endif
          }
        }
      }

    }
#endif
#if NM_STEP_UP
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    if (m_character->stepUp)
#endif
    {
      bool steppingUp = false;
      if (m_footState.m_footChoice == NmRsCBUDynBal_FootState::kLeftStep)
        steppingUp = m_roPacket.m_leftFootProbeHitPos.z > m_roPacket.m_rightFootProbeHitPos.z + 0.1f;              
      else if (m_footState.m_footChoice == NmRsCBUDynBal_FootState::kRightStep)
        steppingUp = m_roPacket.m_rightFootProbeHitPos.z > m_roPacket.m_leftFootProbeHitPos.z + 0.1f;              
      //increaseStepHeight
      if (steppingUp)
        m_roPacket.m_stepHeight += m_stepHeightInc4Step;
#if ART_ENABLE_BSPY
      bool ID = steppingUp;
      bspyTaskScratchpadAuto("ID_StepUp", ID);
#endif
    }
#endif//NM_STEP_UP


    //mmmmtodo Remove tripping caused by pointing toes near slope edges
    //mmmmtodo test this doesn't look bad while walking on slopes or cause worse up-slope ability
    //if (!m_footState.state.m_leftFootBalance)
    //  m_roPacket.m_leftFootProbeNormal = m_roPacket.m_gUp;
    //if (!m_footState.state.m_rightFootBalance)
    //  m_roPacket.m_rightFootProbeNormal = m_roPacket.m_gUp;

  }

  bool NmRsCBUDynamicBalancer::updateBalanceStatus()
  {
    if (m_crouching)
      return true;

    bool failedAlready = (m_failType != balOK);

    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    //MMMM decideBalancerState
    NmRsCBUBalancerCollisionsReaction* balColReactTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(balColReactTask);

    rage::Vector3 leftFootPos = getLeftLeg()->getFoot()->getPosition();
    rage::Vector3 rightFootPos = getRightLeg()->getFoot()->getPosition();
    rage::Vector3 leftKneePos = getLeftLeg()->getKnee()->getJointPosition();
    rage::Vector3 rightKneePos = getRightLeg()->getKnee()->getJointPosition();
    rage::Vector3 bodyCOM = m_character->m_COM;

    float horizDistLeft = m_roPacket.horizDistanceReal(leftFootPos, bodyCOM);
    float horizDistRight = m_roPacket.horizDistanceReal(rightFootPos, bodyCOM);

    float dist = rage::Min(horizDistLeft, horizDistRight);

    float horizDistLeftKnee = m_roPacket.horizDistanceReal(leftKneePos, bodyCOM);
    float horizDistRightKnee = m_roPacket.horizDistanceReal(rightKneePos, bodyCOM);

    float distKnee = rage::Min(horizDistLeftKnee, horizDistRightKnee);

    // TDL add 3rd point in centre just to get better measure
    // HDD note this is duplicated with stuff in foot control
    rage::Vector3 centrePoint(leftFootPos);
    centrePoint.Add(rightFootPos);
    centrePoint.Multiply(rage::VEC3_HALF);
    rage::Vector3 centrePointKnee(leftKneePos);
    centrePointKnee.Add(rightKneePos);
    centrePointKnee.Multiply(rage::VEC3_HALF);

    float upDotLeft = m_roPacket.m_gUpReal.Dot(leftFootPos);
    float upDotRight = m_roPacket.m_gUpReal.Dot(rightFootPos);

    float upDotLeftKnee = m_roPacket.m_gUpReal.Dot(leftKneePos);
    float upDotRightKnee = m_roPacket.m_gUpReal.Dot(rightKneePos);

    float horizDistCentre = m_roPacket.horizDistanceReal(centrePoint, bodyCOM);
    float horizDistCentreKnee = m_roPacket.horizDistanceReal(centrePointKnee, bodyCOM);
    float height = m_roPacket.m_gUpReal.Dot(bodyCOM) - rage::Min(upDotLeft, upDotRight);
    float heightKnee = m_roPacket.m_gUpReal.Dot(bodyCOM) - rage::Min(upDotLeftKnee, upDotRightKnee);

    dist = rage::Min(dist, horizDistCentre);
    distKnee = rage::Min(distKnee, horizDistCentreKnee);

    NM_RS_DBG_LOGF(L"DYNC| dist: %.2f  height %.2f", dist, height);

    // are we beyond our acceptable stability threshold?
    float giveUpHeight = m_giveUpHeight;
    float giveUpThreshold = m_roPacket.m_giveUpThreshold;

#if DYNBAL_GIVEUP_RAMP
    // check whether we're ramping giveUpHeight up/down
    if (m_giveUpRampDuration > 0)
    {
      // don't bother if start and end values are the same 
      if(fabs(m_giveUpHeightEnd - m_giveUpHeight) > 0.01f)
      {     
        // during ramp
        if (m_timer < m_giveUpRampDuration)
          giveUpHeight = m_giveUpHeight - (m_timer/m_giveUpRampDuration)*(m_giveUpHeight - m_giveUpHeightEnd);
        //after ramp
        else
          giveUpHeight = m_giveUpHeightEnd;
      }

      // don't bother if start and end values are the same 
      if(fabs(m_giveUpThresholdEnd - m_roPacket.m_giveUpThreshold) > 0.01f)
      {     
        // during ramp
        if (m_timer < m_giveUpRampDuration)
          giveUpThreshold = m_roPacket.m_giveUpThreshold - (m_timer/m_giveUpRampDuration)*(m_roPacket.m_giveUpThreshold - m_giveUpThresholdEnd);
        //after ramp
        else
          giveUpThreshold = m_giveUpThresholdEnd;
      }
    }
#endif 

    //keep the balancer on longer for staggerFall
    //Maybe we should remove this as I think it is causing the vertical lower stepping stagger that looks a bit stupid
    //Start catch fall earlier if hands and knees as this keeps the balancer on anyway untill the arms hit the ground
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    if (m_roPacket.m_stagger && !catchFallTask->m_handsAndKnees)
      giveUpHeight = 0.f;//0.2f;

    static float kneeHeightMult = 2.4f;

    // need to save this to check in tickPhase3, when maxSteps is reached, whether lean is enough to send abortMessage
    m_distHeightRatio = dist / rage::Max(0.001f, height);

	// Check whether the back is arched back too far (only when not using staggerFall)
	static float backArchDotLim = 0.75f;
	static float backArchCumulativeAngleLim = -0.5f;
	rage::Vector3 spineDir = getSpine()->getSpine3Part()->getPosition() - getSpine()->getPelvisPart()->getPosition();
	spineDir.NormalizeFast();
	float archDot = spineDir.Dot(m_roPacket.m_gUpReal);
	float backArchCumulativeAngle = getSpine()->getSpine0()->getActualLean1() + getSpine()->getSpine1()->getActualLean1() + getSpine()->getSpine2()->getActualLean1()
		 + getSpine()->getSpine3()->getActualLean1();
	NmRsCBUStaggerFall* staggerFallTask = (NmRsCBUStaggerFall*)m_cbuParent->m_tasks[bvid_staggerFall];
	bool backArchedBack = archDot < backArchDotLim && backArchCumulativeAngle < backArchCumulativeAngleLim 
		&& !(staggerFallTask && staggerFallTask->isActive()); 

#if ART_ENABLE_BSPY
    bspyTaskScratchpadAuto("DynBal", giveUpHeight);
    bspyTaskScratchpadAuto("DynBal", giveUpThreshold);
    m_distKnee = distKnee;
    m_heightKnee = heightKnee;
    if (heightKnee > 1e-10f)
      m_distHeightKneeRatio = distKnee/heightKnee;
    m_dist = dist;
    m_height = height;
#endif
    //get a fail for lastStandMode or any other behaviour that wants to ignore the fail parameters and use the default ones
    if ( (dist > 0.60f*height)           || (distKnee > kneeHeightMult*heightKnee) || (height < 0.5f) || backArchedBack)
      m_failedIfDefaultFailure = true;
    m_ignoringFailure = false;//mmmmmTodo add an ignoredFailure?
    if ( (dist > giveUpThreshold*height) || (distKnee > kneeHeightMult*heightKnee) || (height < giveUpHeight) || backArchedBack)
    {
      if (m_ignoreFailure)
      {
        m_ignoringFailure = true;
      }
      else if ((!m_failMustCollide) || (m_failMustCollide && m_character->hasCollidedWithEnvironment(bvmask_UpperBody)))
      {
        m_failType = balFail_General;//MMMMHandsKneesany type of fail if bSpy not active (or hands and knees)
#if ART_ENABLE_BSPY
        if (distKnee > kneeHeightMult*heightKnee)
          m_failType = balFail_Draped;
        if (dist > giveUpThreshold*height)
          m_failType = balFail_Leaning;
        if (height < giveUpHeight)
          m_failType = balFail_Foot2HipHeight;
        if (dist > giveUpThreshold*height && (height < giveUpHeight))
          m_failType = balFail_LeaningAndHeight;
		if (backArchedBack)
          m_failType = balFail_BackArched;
#endif
        // turn off, mark as failed balance attempt
        if (!catchFallTask->m_handsAndKnees)//balancer is on always if hands and knees is set
        {
          deactivate();
          m_failed = true;
        }

        if (feedback)
        {
          feedback->m_agentID = m_character->getID();
          feedback->m_argsCount = 0;
          strcpy(feedback->m_behaviourName, NMDynamicBalancerFeedbackName);
          feedback->onBehaviourFailure();
        }
      }
    }

    //Turn on collisions for hands and Knees catchFall  //mmmmtodo after a delay? Turn them back on if stands up.
    if (!m_failed && !failedAlready && (m_failType != balOK))
      setLegCollision(true);

    if (!m_failed)
    {
      float speed = m_character->m_COMvelRelativeMag;
      float rotSpeed = m_character->m_COMrotvelMag;

      NM_RS_DBG_LOGF(L"DYNC| success? - (relative) speed : %.4f", speed);
      NM_RS_DBG_LOGF(L"DYNC| success? - rotSpeed : %.4f", rotSpeed);

      if (balColReactTask->isActive())
      {
        if (m_roPacket.m_balancerState == m_roPacket.bal_LeanAgainst && 
          speed < m_stableSuccessMinimumLinSpeed && 
          rotSpeed < m_stableSuccessMinimumRotSpeed)
        {
          balColReactTask->m_balancerState = m_roPacket.bal_LeanAgainstStable;
        }
      }

#if USE_NEW_BALANCE_SUCCESS
      // success is determined from a stable step state and low body velocity
      if (speed < m_stableSuccessMinimumLinSpeed && 
        rotSpeed < m_stableSuccessMinimumRotSpeed && 
        m_footState.m_footChoice == NmRsCBUDynBal_FootState::kNotStepping &&
        m_footState.m_numOfSteps > 0)//mmmmtodo should this be -1 now?
#else
      // success is determined from low body velocity
      if (speed < m_stableSuccessMinimumLinSpeed && 
        rotSpeed < m_stableSuccessMinimumRotSpeed)
#endif
      {
        NM_RS_DBG_LOGF(L"DYNC| success? %d", true);
        if (feedback)
        {
          feedback->m_agentID = m_character->getID();
          feedback->m_argsCount = 0;
          strcpy(feedback->m_behaviourName, NMDynamicBalancerFeedbackName);
          feedback->onBehaviourSuccess();
        }
      }

      // TDL block for predicting stability for an early success feedback message
      float balanceTime = 0.3f;
      rage::Vector3 newLeftFootPos = rage::Vector3(leftFootPos) + getLeftLeg()->getFoot()->getLinearVelocity()*balanceTime;
      rage::Vector3 newRightFootPos = rage::Vector3(rightFootPos) + getRightLeg()->getFoot()->getLinearVelocity()*balanceTime;
      rage::Vector3 newCentrePoint = (rage::Vector3(leftFootPos) + rage::Vector3(rightFootPos)) * 0.5f;
      //rage::Vector3 newBodyCOM = rage::Vector3(bodyCOM) + m_character->m_COMvel*balanceTime;
      rage::Vector3 newBodyCOM = rage::Vector3(bodyCOM) + m_bodyPacket.m_lvlCOMvelRelative*balanceTime;
      float nextDist = rage::Min(m_roPacket.horizDistanceReal(newLeftFootPos, newBodyCOM), m_roPacket.horizDistanceReal(newRightFootPos, newBodyCOM));
      nextDist = rage::Min(nextDist, m_roPacket.horizDistanceReal(newCentrePoint, newBodyCOM));
      float newInstability = nextDist / (0.6f*height*0.5f*0.6f);
      if (newInstability > m_balanceInstability)
        m_balanceInstability = newInstability;
      else
        m_balanceInstability += (newInstability - m_balanceInstability)*0.25f; // smooth to new measure

      if (m_balanceInstability < 1.f && height > 0.7f && (m_balanceIndefinitely || (m_footState.m_maxSteps - m_footState.m_numOfSteps4Max) > 2))
      {
        if (feedback) 
        {
          feedback->m_agentID = m_character->getID();
          feedback->m_argsCount = 1;
          ART::ARTFeedbackInterface::FeedbackUserdata data;
          data.setFloat(m_balanceInstability);
          feedback->m_args[0] = data;
          strcpy(feedback->m_behaviourName, NMDynamicBalancerFeedbackName);
          feedback->onBehaviourEvent();
        }
      }
    }
    if (m_failed && ((m_roPacket.m_balancerState == m_roPacket.bal_Drape) || (m_roPacket.m_balancerState == m_roPacket.bal_DrapeGlancingSpin)))
    {
      //mmmmmtodo undo this
      //if (balColReactTask->isActive())
      //balColReactTask->m_balancerState = m_roPacket.bal_Draped;
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 1;
        ART::ARTFeedbackInterface::FeedbackUserdata data;
        data.setInt(m_roPacket.m_balancerState);
        feedback->m_args[0] = data;
        strcpy(feedback->m_behaviourName, NMBalanceStateFeedbackName);
        feedback->onBehaviourEvent();
      }
    }
    return isActive();
  }


#if ART_ENABLE_BSPY
  void NmRsCBUDynamicBalancer::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    static const char* alp_overTimeModeStrings[] = 
    {
      "eAMNone",
      "eAMModify",
      "eAMInvalid",
    };
    static const char* alp_modeStrings[] = 
    {
      "eALNone",
      "eALDirection",
      "eALToPosition",
      "eALToObject",
      "eALRandom",
      "eALInvalid",
    };
    static const char* footChoiceStrings[] = 
    {
      "kNotStepping",
      "kLeftStep",
      "kRightStep",
    };

    static const char* failTypeStrings[] =
    {
#define BAL_NAME_ACTION(_name) #_name ,
      BAL_STATES(BAL_NAME_ACTION)
#undef BAL_NAME_ACTION
    };

    // only send 'parameters' on the first tick
    if (m_tickPhase == eDynBalTP_2) // note that because the tick phase will be already incremented, we check for eDynBalTP_2 here when we actually want to focus on eDynBalTP_1
    {

    //'parameters' well inputs
    //configureBalance
    bspyTaskVar(m_stepHeight, true);
    bspyTaskVar(m_roPacket.m_legsApartRestep, true);      
    bspyTaskVar(m_roPacket.m_legsTogetherRestep, true);
    bspyTaskVar(m_roPacket.m_legsApartMax, true);
    bspyTaskVar(m_minKneeAngle, true);      
    bspyTaskVar(m_taperKneeStrength, true);
    bspyTaskVar(m_leftLegStiffness, true);
    bspyTaskVar(m_rightLegStiffness, true);
    bspyTaskVar(m_leftLegSwingDamping, true);
    bspyTaskVar(m_rightLegSwingDamping, true);
    bspyTaskVar(m_opposeGravityLegs, true);
    bspyTaskVar(m_opposeGravityAnkles, true);
    bspyTaskVar(m_leanAcc, true);
    bspyTaskVar(m_hipLeanAcc, true);
    bspyTaskVar(m_leanAccMax, true);
    bspyTaskVar(m_resistAcc, true);
    bspyTaskVar(m_resistAccMax, true);
    bspyTaskVar(m_footSlipCompOnMovingFloor, true);
    

    bspyTaskVar(m_stepClampScale, true);
    bspyTaskVar(m_stepClampScaleVariance, true);
    bspyTaskVar(m_balanceTimeVariance, true);
    bspyTaskVar(m_roPacket.m_giveUpThreshold, true);

#if DYNBAL_GIVEUP_RAMP
    bspyTaskVar(m_giveUpHeight, true);
    bspyTaskVar(m_giveUpHeightEnd, true);
    bspyTaskVar(m_giveUpThresholdEnd, true);
    bspyTaskVar(m_giveUpRampDuration, true);
    bspyTaskVar(m_leanToAbort, true);
#endif

    bspyTaskVar(m_roPacket.m_stepClampScale, true);
    bspyTaskVar(m_roPacket.m_balanceTime, true);
    bspyTaskVar(m_roPacket.m_balanceTimeHip, true);
    bspyTaskVar(m_roPacket.m_stepDecisionThreshold, true);

    bspyTaskVar(m_maximumBalanceTime, true);
    bspyTaskVar(m_balanceTimeAtRampDownStart, false);
    bspyTaskVar(m_balanceIndefinitely, true);
    bspyTaskVar(m_rampHipPitchOnFail, true);
    bspyTaskVar(m_character->m_movingFloor, true);      
    bspyTaskVar(m_useCustomTurnDir, true);
    bspyTaskVar(m_customTurnDir, true);
    bspyTaskVar(m_roPacket.m_movingFloor, true);
	  bspyTaskVar(m_roPacket.m_avoidCar, true);//Brace4ImpactDEVEL
    bspyTaskVar(m_roPacket.m_flatterSwingFeet, true);
    bspyTaskVar(m_roPacket.m_flatterStaticFeet, true);

    bspyTaskVar(m_hipPitch, true);
    bspyTaskVar(m_hipRoll, true);
    bspyTaskVar(m_hipYaw, true);
    bspyTaskVar(m_roPacket.m_ankleEquilibrium, true);
    bspyTaskVar(m_kneeStrength, true);
    bspyTaskVar(m_stableSuccessMinimumRotSpeed, true);
    bspyTaskVar(m_stableSuccessMinimumLinSpeed, true);
    // bspyTaskVar(m_legStraightnessModifier, true); HDD: duplicate
    bspyTaskVar(m_leanAgainstVelocity, true);      
    bspyTaskVar(m_footState.m_stepIfInSupport, true);
    bspyTaskVar(m_footState.m_alwaysStepWithFarthest, true);
    bspyTaskVar(m_standUp, true);

    bspyTaskVar(m_character->m_depthFudge, true);      
    bspyTaskVar(m_character->m_depthFudgeStagger, true);      
    bspyTaskVar(m_character->m_footFriction, true);      
    bspyTaskVar(m_character->m_footFrictionStagger, true);      

    bspyTaskVar_StringEnum(m_autoLeanParams.m_mode, alp_modeStrings, true);
    bspyTaskVar_StringEnum(m_autoLeanParams.m_amountOverTimeMode, alp_overTimeModeStrings, true);       
    bspyTaskVar(m_autoLeanParams.m_vec, true);
    bspyTaskVar(m_autoLeanParams.m_levelIndex, true);
    bspyTaskVar(m_autoLeanParams.m_boundIndex, true);
    bspyTaskVar(m_autoLeanParams.m_amount, true);
    bspyTaskVar(m_autoLeanParams.m_amountDelta, true);
    bspyTaskVar(m_autoLeanParams.m_timeRemaining, true);
    bspyTaskVar(m_roPacket.m_useComDirTurnVelThresh, true);
    bspyTaskVar(m_roPacket.m_random, true);

    bspyTaskVar_StringEnum(m_autoLeanHipsParams.m_mode, alp_modeStrings, true);
    bspyTaskVar_StringEnum(m_autoLeanHipsParams.m_amountOverTimeMode, alp_overTimeModeStrings, true);       
    bspyTaskVar(m_autoLeanHipsParams.m_vec, true);
    bspyTaskVar(m_autoLeanHipsParams.m_levelIndex, true);
    bspyTaskVar(m_autoLeanHipsParams.m_boundIndex, true);
    bspyTaskVar(m_autoLeanHipsParams.m_amount, true);
    bspyTaskVar(m_autoLeanHipsParams.m_amountDelta, true);
    bspyTaskVar(m_autoLeanHipsParams.m_timeRemaining, true);

    bspyTaskVar_StringEnum(m_autoLeanForceParams.m_mode, alp_modeStrings, true);
    bspyTaskVar_StringEnum(m_autoLeanForceParams.m_amountOverTimeMode, alp_overTimeModeStrings, true);       
    bspyTaskVar(m_autoLeanForceParams.m_vec, true);
    bspyTaskVar(m_autoLeanForceParams.m_levelIndex, true);
    bspyTaskVar(m_autoLeanForceParams.m_boundIndex, true);
    bspyTaskVar(m_autoLeanForceParams.m_bodyPart, true);
    bspyTaskVar(m_autoLeanForceParams.m_amount, true);
    bspyTaskVar(m_autoLeanForceParams.m_amountDelta, true);
    bspyTaskVar(m_autoLeanForceParams.m_timeRemaining, true);

    bspyTaskVar(m_stagger, true);
    bspyTaskVar(m_roPacket.m_fallToKnees, true);
    bspyTaskVar(m_plantLeg, true);
    bspyTaskVar(m_airborneStep, true);
    bspyTaskVar(m_ignoreFailure, true);     
    bspyTaskVar(m_failMustCollide, true);   
    bspyTaskVar(m_crouching, true);     

    bspyTaskVar(m_character->m_uprightConstraint.forceActive, true);     
    bspyTaskVar(m_character->m_uprightConstraint.torqueActive, true);     
	bspyTaskVar(m_character->m_uprightConstraint.lastStandMode, true); 
	bspyTaskVar(m_character->m_uprightConstraint.lastStandSinkRate, true); 
	bspyTaskVar(m_character->m_uprightConstraint.lastStandHorizDamping, true); 
	bspyTaskVar(m_character->m_uprightConstraint.lastStandMaxTime, true); 
	bspyTaskVar(m_character->m_uprightConstraint.turnTowardsBullets, true);
    bspyTaskVar(m_character->m_uprightConstraint.velocityBased, true);     
    bspyTaskVar(m_character->m_uprightConstraint.torqueOnlyInAir, true);     
    bspyTaskVar(m_character->m_uprightConstraint.forceStrength, true);     
    bspyTaskVar(m_character->m_uprightConstraint.forceDamping, true);     
    bspyTaskVar(m_character->m_uprightConstraint.forceFeetMult, true);     
    bspyTaskVar(m_character->m_uprightConstraint.forceSpine3Share, true);
    bspyTaskVar(m_character->m_uprightConstraint.forceLeanReduction, true);     
    bspyTaskVar(m_character->m_uprightConstraint.forceInAirShare, true);     
    bspyTaskVar(m_character->m_uprightConstraint.forceMin, true);     
    bspyTaskVar(m_character->m_uprightConstraint.forceMax, true);     
    bspyTaskVar(m_character->m_uprightConstraint.forceSaturationVel, true);     
    bspyTaskVar(m_character->m_uprightConstraint.forceThresholdVel, true);     
    bspyTaskVar(m_character->m_uprightConstraint.torqueStrength, true);     
    bspyTaskVar(m_character->m_uprightConstraint.torqueDamping, true);     
    bspyTaskVar(m_character->m_uprightConstraint.torqueSaturationVel, true);     
    bspyTaskVar(m_character->m_uprightConstraint.torqueThresholdVel, true);     
    bspyTaskVar(m_character->m_uprightConstraint.supportPosition, true);     
    bspyTaskVar(m_character->m_uprightConstraint.noSupportForceMult, true);     
    bspyTaskVar(m_character->m_uprightConstraint.stepUpHelp, true);     
    bspyTaskVar(m_character->m_uprightConstraint.stayUpAcc, true);     
    bspyTaskVar(m_character->m_uprightConstraint.stayUpAccMax, true);     

    bspyTaskVar(m_character->m_footSlipMult, true);

    //calculated from zero pose
    const CharacterConfiguration& cc = m_character->getCharacterConfiguration();
    bspyTaskVar(cc.m_legSeparation, true);   
    bspyTaskVar(cc.m_charlieChapliness, true);   
    bspyTaskVar(cc.m_headYaw, true);   
    bspyTaskVar(cc.m_hipYaw, true);   
    bspyTaskVar(cc.m_defaultHipPitch, true);   
    bspyTaskVar(cc.m_legStraightness, true); 
    bspyTaskVar(m_legStraightnessModifier, true); 
    bspyTaskVar(m_roPacket.m_changeStepTime, true); 
    bspyTaskVar(m_roPacket.m_extraFeetApart, true); 
    bspyTaskVar(m_roPacket.m_hipWidth, true); 

    //Output
    //Stability/Failure
    bspyTaskVar(m_giveUpHeight, true);
    bspyTaskVar(m_footState.m_maxSteps, true); 
    bspyTaskVar(m_fallType, true); 
    bspyTaskVar(m_fallMult, true); 
    bspyTaskVar(m_fallReduceGravityComp, true); 

    } // tick phase 1 only


    bspyTaskVar(m_footState.m_forceBalance, false);      
    bspyTaskVar(m_footState.m_dontChangeStep, false);      
    bspyTaskVar(m_footState.m_stepWithBoth, false);      

    bspyTaskVar(m_rampDownBegun, false);


    //Output
    //Stability/Failure
    bspyTaskVar(m_timer, false);
    bspyTaskVar(m_balanceInstability, false);
    bspyTaskVar(m_failed, false);
    bspyTaskVar(m_failedIfDefaultFailure, false);
    bspyTaskVar(m_ignoringFailure, false);
    bspyTaskVar(m_distKnee, false);
    bspyTaskVar(m_heightKnee, false);
    bspyTaskVar(m_distHeightKneeRatio, false);
    bspyTaskVar(m_dist, false);
    bspyTaskVar(m_height, false);
    bspyTaskVar(m_distHeightRatio, false);
    bspyTaskVar_StringEnum(m_failType, failTypeStrings, false);  

    //balancerState
    bspyTaskVar(m_footState.m_numOfSteps, false);
    bspyTaskVar(m_footState.m_numOfSteps4Max, false);
    bspyTaskVar(m_footState.m_forceStep, false);
    bspyTaskVar(m_footState.m_forceStepExtraHeight, false);
    bspyTaskVar(m_footState.m_numOfStepsAtForceStep, false);
    bspyTaskVar(m_footState.m_leftDistance, false);      
    bspyTaskVar(m_footState.m_rightDistance, false);      
    bspyTaskVar(m_footState.state.m_isInsideSupport, false);      
    bspyTaskVar(m_footState.state.m_isInsideSupportHonest, false);      
    bspyTaskVar(m_footState.state.m_leftGround, false);      
    bspyTaskVar(m_footState.state.m_achievedGoal, false);      
    bspyTaskVar(m_footState.state.m_leftFootBalance, false);      
    bspyTaskVar(m_footState.state.m_rightFootBalance, false);      
    bspyTaskVar_StringEnum(m_footState.m_footChoice, footChoiceStrings, false);  

    bspyTaskVar(m_bodyPacket.cd.m_leftFootCollided, false);      
    bspyTaskVar(m_bodyPacket.cd.m_leftFootCollidedLast, false);      
    bspyTaskVar(m_bodyPacket.cd.m_rightFootCollided, false);      
    bspyTaskVar(m_bodyPacket.cd.m_rightFootCollidedLast, false);      
    bspyTaskVar(m_footState.m_groundHeightLeft, false);      
    bspyTaskVar(m_footState.m_groundHeightRight, false);      

    float stepGrad = 0.5f; // set back to 1.0f if we're hitting the ground too much
    bool airborne = false;
    if (m_footState.state.m_leftFootBalance) 
      airborne = !m_bodyPacket.cd.m_leftFootCollided && !m_bodyPacket.cd.m_leftFootCollidedLast; 
    else
      airborne = !m_bodyPacket.cd.m_rightFootCollided && !m_bodyPacket.cd.m_rightFootCollidedLast;
    if (airborne)
      stepGrad = 0.01f;
    bspyTaskVar(airborne, false);      
    bspyTaskVar(stepGrad, false);      

    bspyTaskVar(m_roPacket.m_fullLegLength, false);
    bspyTaskVar(m_roPacket.m_legStraightness, false);      
    bspyTaskVar(m_bodyPacket.m_newWaistHeight, false);     
    bspyTaskVar(m_bodyPacket.m_floorVelocity, false);     
    bspyTaskVar(m_bodyPacket.m_floorAcceleration, false);     
    bspyTaskVar(m_roPacket.m_gUp, false);     
    bspyTaskVar(m_roPacket.m_leanHipgUp, false);     
    bspyTaskVar(m_roPacket.m_timeTakenForStep, false);

    rage::Vector3 progCOM(m_bodyPacket.m_COM);        
    if (m_roPacket.m_stagger)
      progCOM = m_bodyPacket.m_buttocks.m_tm.d;
    progCOM.AddScaled(m_bodyPacket.m_lvlCOMvelRelative, m_roPacket.m_balanceTime);
    m_roPacket.levelVector(progCOM, m_roPacket.m_gUp.Dot(m_footState.m_centreOfFeet));

    //For debugGraphics
    bspyTaskVar(m_roPacket.m_leftFootProbeHitPos, false);
    bspyTaskVar(m_roPacket.m_leftFootProbeNormal, false);
    bspyTaskVar(m_roPacket.m_rightFootProbeHitPos, false);
    bspyTaskVar(m_roPacket.m_rightFootProbeNormal, false);
    bspyTaskVar(progCOM, false);
    bspyTaskVar(m_footState.m_exclusionZonePt1, false);
    bspyTaskVar(m_footState.m_exclusionZonePt2, false);
    bspyTaskVar(m_bodyPacket.m_lvlCOMvelRelative, false);

    bspyTaskVar(m_footState.m_oldDesiredPos, false);
    rage::Matrix34 toeTM;
    float footLength = 0.26448f;
    float footHeight = 0.07522f;
    getLeftLeg()->getFoot()->getBoundMatrix(&toeTM);
    rage::Vector3 ltoe = getLeftLeg()->getFoot()->getPosition();
    ltoe -= toeTM.c*footLength*0.5f;
    ltoe -= toeTM.b*footHeight*0.5f;
    bspyTaskVar(ltoe, false);
    getRightLeg()->getFoot()->getBoundMatrix(&toeTM);
    rage::Vector3 rtoe = getRightLeg()->getFoot()->getPosition();
    rtoe -= toeTM.c*footLength*0.5f;
    rtoe -= toeTM.b*footHeight*0.5f;
    bspyTaskVar(rtoe, false);
    bspyTaskVar(m_pelvisState.m_twistLeft, false);
    bspyTaskVar(m_pelvisState.m_twistRight, false);

    bspyTaskVar(m_pelvisState.m_hipPitch, false);    
    bspyTaskVar(m_pelvisState.m_hipRollCalc, false);    
  }
#endif // ART_ENABLE_BSPY
}

