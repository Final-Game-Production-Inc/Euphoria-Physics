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
 * Designed for when a character jumps off an object. During the
 * fall tries to keep orientated upwards, windmills the arms and
 * pedals the legs. When near the ground will prepare to land 
 * the fall. Attempts to land the highfall and ends up balancing
 * using the BodyBalance if successful or on the ground with 
 * catchfall or rollup. 
 */


#include "NmRsInclude.h"
#include "NmRsCBU_HighFall.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_Pedal.h"
#include "NmRsCBU_ArmsWindmillAdaptive.h"
#include "NmRsCBU_RollUp.h"
#include "NmRsCBU_Catchfall.h"
#include "NmRsCBU_BodyBalance.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_RollDownStairs.h"
#include "NmRsCBU_BodyFoetal.h"
#include "NmRsCBU_BodyWrithe.h"

namespace ART
{
#define highFall_DEVEL 1
  NmRsCBUHighFall::NmRsCBUHighFall(ART::MemoryManager* services) : CBUTaskBase(services, bvid_highFall)
  {
    initialiseCustomVariables();
  }

  NmRsCBUHighFall::~NmRsCBUHighFall(){}

  void NmRsCBUHighFall::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    // reset the variables
    m_timer = 0.0f;
    m_hasCollidedWithWorld = false;
    m_controlOrientation = true;
    m_willLandOnFeet = true;
    m_predImpactTime = 50.0f;
    m_forwardVelMag = 0.0f;
    m_messageSent = false; 
    m_goToCatchFallFlag = false;
    m_rollToBBtimer = 0.0f;
    m_averageComVel = 0.0f;
    m_leftLegLength = 0.0f;
    m_normalDotUp = 0.0f;
    m_probeHit = false;
    m_fallRecoverable = false;
    m_impactVisible = false;
    m_landingNormalOK = false;
    m_orientationState = NmRsCharacter::OS_Back;
    m_orientationStateTimeHalved= NmRsCharacter::OS_Back;
    m_orientationStateCurrent = NmRsCharacter::OS_Back;
    m_horComVel.Zero();

    // reset state machine to initial state
    Reset();
  }

  void NmRsCBUHighFall::onActivate()
  {
    Assert(m_character);
    m_handAnimationType = haNone;
    m_cameraPos = m_character->m_COM;

    m_hulaTime = -0.1f;
    m_hulaTotalTime = 0.f;
    m_rollToBB = false;
    m_impactAnimationTime = 0.0f;

    // Get Agent left leg length.
    m_leftLegLength = getLeftLegLength();

#if HIGHFALL_AUTOMATIC_WEAPON_DETECTION
    m_character->setDontRegisterCollsion(1.0f, 0.05f);//mmmmtodo Note this will be turned off if balancerCollisionsReaction deactivates
#endif
#ifdef NM_RS_CBU_ASYNCH_PROBES
    //This probe can be long and therefore expensive.
    //  We therefore don't use a non-asynchronous probe for the 1st requested probe. 
    //  A frame delay on knowing that the character is going to land on something won't make any difference.
    m_character->InitializeProbe(NmRsCharacter::pi_highFall, true);//This probe can be long and therefore expensive.  We therefore use an asynchronous probe for the 1st requested probe. A frame delay on knowing the character is going to land on something won't make any difference.
#endif //NM_RS_CBU_ASYNCH_PROBES

    // state machine init: will execute first real state's enter function
    Initialize();
  }

  void NmRsCBUHighFall::onDeactivate()
  {
    Assert(m_character);

    // Run the current state's exit routine.
    States( EVENT_Exit, m_currentState );

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
    m_character->deactivateTask(bvid_armsWindmillAdaptive);
    m_character->deactivateTask(bvid_bodyWrithe);
#ifdef NM_RS_CBU_ASYNCH_PROBES
    m_character->ClearAsynchProbe_IfNotInUse(NmRsCharacter::pi_highFall);
#endif
  }

  // tick: not much happening here, most stuff is in the state machine update and individual state's ticks
  // ---------------------------------------------------------------------------------------------------------
  CBUTaskReturn NmRsCBUHighFall::onTick(float UNUSED_PARAM(timeStep))
  {
#if HIGHFALL_AUTOMATIC_WEAPON_DETECTION
    // contact filtering won't affect what we're getting in 1st frame, hence wait:
    if(m_timer > 0.0f)
      m_hasCollidedWithWorld = (m_character->hasCollidedWithWorld(bvmask_Full) == bvmask_None ? false : true);
#else
    m_hasCollidedWithWorld = m_character->hasCollidedWithWorld(bvmask_Full);
#endif

    calculateCommonVariables();

    orientationControllerTick();

    m_timer += m_character->getLastKnownUpdateStep();

    // update state machine
    Update();
    // update feedback
    handAnimationFeedback();
    return eCBUTaskComplete;
  }

  // state machine states and transitions
  // ---------------------------------------------------------------------------------------------------------
  bool NmRsCBUHighFall::States( StateMachineEvent event, int state )
  {
    BeginStateMachine

    // Initial State: Falling  ////////////////////////////////////////////////////
    State( HF_Falling )

      OnEnter
      {
        m_controlOrientation = true;
      }

    OnUpdate
    {
      duringFall();

      float toCloseToGround = (m_predImpactTime - m_parameters.m_catchfalltime - 0.15f);
      bool toLand = (m_timer > toCloseToGround);
      //mmmmtodo parameterized bailout vs prepare?
      bool toBailOut = (toLand && !m_willLandOnFeet) || (toLand && (m_hasCollidedWithWorld && !m_parameters.m_ignorWorldCollisions));

      // Evaluate condition for transitioning to HF_NonRecoverableFall state.
      m_fallRecoverable = true;

      if (toLand)
      {
        // Is impact visible to the character i.e. character is facing the direction of traveling.
        m_fallRecoverable = ((m_character->m_COMvel).Mag() < m_parameters.m_maxSpeedForRecoverableFall);
      }

      // Decide which state should be transitioned to.
      if (!m_fallRecoverable) // Fall is not recoverable i.e. character is moving too fast.
      {
        SetState( HF_NonRecoverableFall );
      }
      else if (toBailOut)
      {
        if (m_impactVisible && (m_orientationState == NmRsCharacter::OS_Front || m_orientationState == NmRsCharacter::OS_Down))
        {
          // Make sure character is going to land on its front.
          if (m_orientationState == NmRsCharacter::OS_Front)
          {
            // Calculate predicted orientation for time to impact halved.
            rage::Matrix34 predCOMTM;
            m_character->getPredictedCOMOrientation((m_predImpactTime - m_timer)*0.5f, &predCOMTM);
            m_orientationStateTimeHalved = m_character->getFacingDirectionFromCOMOrientation(predCOMTM);

            // Current character orientation.
            m_orientationStateCurrent = m_character->getFacingDirectionFromCOMOrientation(m_character->m_COMTM);

            // Allow for catchFall only when both - current and predicted for halved impact time orientations
            // are front i.e. prevent catchFall when character current orientation is up and predicted on impact is front
            // because these cases look worst in Game.
            if (m_orientationStateCurrent == NmRsCharacter::OS_Front && m_orientationStateTimeHalved == NmRsCharacter::OS_Front)
            {
              SetState( HF_CatchFall );
            }
            else
            {
              SetState( HF_BailOut );
            }
          }//if (m_orientationState == NmRsCharacter::OS_Front)
          else // Predicted orientation state is NmRsCharacter::OS_Down and character is facing the impact, allow catch fall in this case.
          {
            SetState( HF_CatchFall );
          }
        }
        else//if (m_impactVisible && (m_orientationState == NmRsCharacter::OS_Front || m_orientationState == NmRsCharacter::OS_Down))
        {
          SetState( HF_BailOut );
        }
      }
      else if (toLand && m_landingNormalOK)//if he hits a slope he will never come out of this state unless he is not standing (quite unlikely I guess)
      {
        SetState( HF_PrepareForLanding );
      }
    }

    OnExit
    {
      m_controlOrientation = false;
      m_character->deactivateTask(bvid_headLook);
      m_character->deactivateTask(bvid_pedalLegs);
      m_character->deactivateTask(bvid_catchFall);
      m_character->deactivateTask(bvid_armsWindmillAdaptive);
      m_character->deactivateTask(bvid_bodyWrithe);
#ifdef NM_RS_CBU_ASYNCH_PROBES
      m_character->ResetRayProbeIndex(NmRsCharacter::pi_highFall, true);
#endif
    }

    // Bail Out ////////////////////////////////////////////////////
    State( HF_BailOut )
    OnEnter
    {
      m_hasCollidedWithWorld = false;
      //Stop Bounce
      m_character->setFrictionPreScale(0.0f, bvmask_FootLeft | bvmask_FootRight);
    }

    OnUpdate
    {
      duringBailOut();
      //soften impact with floor and switch to catchFall
      if (m_hasCollidedWithWorld)
      {
        SetState(HF_CatchFall);
      }
      else if (((m_timer - 0.25f) > m_predImpactTime)) // At this time character should have collided already, if not return to the HF_Falling state.
      {
        SetState(HF_Falling);
      }
    }
    OnExit
    {
      m_character->setFrictionPreScale(1.0f, bvmask_FootLeft | bvmask_FootRight);

      m_character->deactivateTask(bvid_bodyFoetal);
      m_character->deactivateTask(bvid_pedalLegs);
    }

    // Prepare For Landing ////////////////////////////////////////////////////
    State( HF_PrepareForLanding )      
    OnUpdate
    {
      duringPrepareForLanding();
      bool decideToBalance = m_hasCollidedWithWorld;
      bool decideToCatchFall = (m_hasCollidedWithWorld && m_goToCatchFallFlag);
      bool decideToForwardRoll = m_hasCollidedWithWorld && m_parameters.m_forwardRoll;//mmmmtodo only forward roll if near upright?
      if (decideToForwardRoll)
      {
        SetState( HF_ForwardRoll);
      }
      else if (decideToCatchFall)
      {
        SetState( HF_CatchFall);
      }
      else if (decideToBalance)
      {
        SetState(HF_Balance);             
        //m_rollToBB = true; // option for intermediate forward roll into balance?
        //SetState( HF_ForwardRoll);
      }
      else if (((m_timer - 0.25f) > m_predImpactTime) && !m_hasCollidedWithWorld) // At this time character should have collided already, if not return to the falling state.
      {
        SetState(HF_Falling);
      }
    }
    OnExit
    {
      m_character->deactivateTask(bvid_pedalLegs);
      m_character->deactivateTask(bvid_catchFall);
    }

    // Non-recoverable fall ////////////////////////////////////////////////////
    State( HF_NonRecoverableFall )
    OnEnter
    {
      m_character->deactivateTask(bvid_pedalLegs);
    }

    OnUpdate
    {
      // Differentiate between a final collision after which character will come to a rest and potential impact with an airplane fuselage or a building to make sure
      // catch fall state does not get activated for the latter case (HF_BraceForImpact is designed to handle this case).
      // Send a probe out downwards looking for relatively flat surface just below the character.
      rage::Vector3 endOfProbe;
      const float probeLength = 0.6f; // Probe has to be short in order not to pick up any geometry below impact point with the wall for example, but long enough
      // to pick up any surface below when character is on its knees.
      endOfProbe.AddScaled(m_character->m_COMTM.d, -m_character->m_gUp, probeLength);
      const bool probeHit = m_character->probeRay(NmRsCharacter::pi_highFall, m_character->m_COMTM.d, endOfProbe, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);
      bool flatPlaneBelow = false;
      if (probeHit) // Is normal ok?
      {
        const float normalDotUp = m_character->m_probeHitNormal[NmRsCharacter::pi_highFall].Dot(m_character->getUpVector());
        flatPlaneBelow = normalDotUp > 0.7f; // Surface has to be flat enough so character will not slide off from it.
      }
      bool decideToCatchFall = flatPlaneBelow && (m_hasCollidedWithWorld && m_goToCatchFallFlag && m_character->m_COMvelRelativeMag < 1.0f && (m_orientationStateCurrent != NmRsCharacter::OS_Up || m_orientationStateCurrent != NmRsCharacter::OS_Down));
      bool decideToBrace = (!decideToCatchFall) && (m_horComVel.Mag() > m_parameters.m_minSpeedForBrace) && (m_orientationState != NmRsCharacter::OS_Back) && m_impactVisible && !m_landingNormalOK;
      if (decideToCatchFall)
      {
        SetState(HF_CatchFall);
      }
      else if (decideToBrace)
      {
        SetState(HF_BraceForImpact);
      }
      else // Blend bodyFoetal with bodyWrithe.
      {
        duringNonRecoverableFall();
      }

      if (((m_timer - 0.25f) > m_predImpactTime) && !m_hasCollidedWithWorld) // At this time character should have collided already, if not return to the HF_Falling state.
      {
        SetState(HF_Falling);
      }
    }

    OnExit
    {
      m_character->deactivateTask(bvid_bodyWrithe);
      m_character->deactivateTask(bvid_bodyFoetal);
#ifdef NM_RS_CBU_ASYNCH_PROBES
      m_character->ResetRayProbeIndex(NmRsCharacter::pi_highFall, true);
#endif
    }

    // Brace ////////////////////////////////////////////////////
    State( HF_BraceForImpact )
    OnUpdate
    {
      // Brace for impact; it is likely that character is going to hit a building or a fuselage of an airplane.
          if ((m_horComVel.Mag() > m_parameters.m_minSpeedForBrace) && (m_orientationState != NmRsCharacter::OS_Back) && m_impactVisible && !m_landingNormalOK && !((m_timer - 0.25f) > m_predImpactTime))
      {
        duringCatchFall();
      }
      else
      {
        SetState(HF_NonRecoverableFall);
      }
    }

    OnExit
    {
      m_character->deactivateTask(bvid_catchFall);
    }

    // Forward Roll ////////////////////////////////////////////////////
    State( HF_ForwardRoll )
    
    OnUpdate
    {
      duringForwardRoll();

      if (m_rollToBB && (m_rollToBBtimer > 0.1f) )
      {
        SetState( HF_Balance );
      }
      bool decideToFall = m_messageSent;
      if (decideToFall)
      {
        SetState( HF_CatchFall );
      }
    }

    OnExit
    {
      m_character->deactivateTask(bvid_rollDownStairs);
    }
    // Catch Fall ////////////////////////////////////////////////////
    State( HF_CatchFall )      

    OnEnter
    {
      m_impactAnimationTime = 1.0f;
      NmRsCBUBodyFoetal* foetalTask = (NmRsCBUBodyFoetal*)m_cbuParent->m_tasks[bvid_bodyFoetal];
      Assert(foetalTask);
      if ((foetalTask->isActive()))
      {
        foetalTask->deactivate();
      }
      NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
      Assert(pedalTask);
      if ((pedalTask->isActive()))
      {
        pedalTask->deactivate();
      }
    }

    OnUpdate
      if (m_impactAnimationTime > 0.0f)
        m_impactAnimationTime -= m_character->getLastKnownUpdateStep();
      duringCatchFall();

    // Balance ////////////////////////////////////////////////////
    State( HF_Balance )      
    OnUpdate
      duringBalance();
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
      Assert(dynamicBalancerTask);
      if (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK)
        SetState( HF_CatchFall );


    EndStateMachine
  }

  // If dot product between character COM velocity direction and COMTM front direction is greater then facingCone,
  // it is considered that character is facing towards direction of traveling i.e. it is able to see where it is going to hit.
  bool NmRsCBUHighFall::isCharacterFacingWhereItTravels(float facingCone) const
  {
    rage::Vector3 comVelDir = m_character->m_COMvel;
    comVelDir.NormalizeSafeV();
    return ((-m_character->m_COMTM.c).Dot(comVelDir) > facingCone);
  }

  // Blend bodyFoetal with bodyWrithe.
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUHighFall::duringNonRecoverableFall()
  {
    NmRsCBUBodyFoetal* foetalTask = (NmRsCBUBodyFoetal*)m_cbuParent->m_tasks[bvid_bodyFoetal];
    Assert(foetalTask);
    if (!foetalTask->isActive())
    {
      foetalTask->updateBehaviourMessage(NULL); // set parameters to defaults
      foetalTask->activate();
    }

    NmRsCBUBodyWrithe* writheTask = (NmRsCBUBodyWrithe*)m_cbuParent->m_tasks[bvid_bodyWrithe];
    Assert(writheTask);

    writheTask->updateBehaviourMessage(NULL); // set parameters to defaults
    writheTask->m_parameters.m_blendArms = 0.5f; //Blend the writhe arms with the current desired arms (0=don't apply any writhe, 1=only writhe) 
    writheTask->m_parameters.m_blendBack = 0.5f; //Blend the writhe spine and neck with the current desired (0=don't apply any writhe, 1=only writhe)
    writheTask->m_parameters.m_blendLegs = 0.5f; //Blend the writhe legs with the current desired legs (0=don't apply any writhe, 1=only writhe)
    if(!writheTask->isActive())
    {
      writheTask->activate();
    }
  }

  // bail out tick
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUHighFall::duringBailOut()
  {

    static bool stopBounce = true;
    NmRsCBUBodyFoetal* foetalTask = (NmRsCBUBodyFoetal*)m_cbuParent->m_tasks[bvid_bodyFoetal];
    Assert(foetalTask);
    if (!(foetalTask->isActive()))
    {
      foetalTask->updateBehaviourMessage(NULL);
      foetalTask->m_parameters.m_stiffness =(10.f);
      if (stopBounce)
        foetalTask->m_parameters.m_effectorMask = bvmask_UpperBody;
      foetalTask->activate();
    }

    //We use pedal to give a tucked foetal like position for the legs
    //  but if the landing is going to be toe first (which can cause a massive bounce)
    //  then we extend the legs to be straighter
    if (stopBounce)
    {
      NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
      Assert(pedalTask);
      if (!pedalTask->isActive() )
      {
        pedalTask->updateBehaviourMessage(NULL); // reset params
        pedalTask->m_parameters.randomSeed = 100;
        pedalTask->m_parameters.pedalLeftLeg = true;
        pedalTask->m_parameters.pedalRightLeg = true;
        pedalTask->m_parameters.radius = 0.2f;
        pedalTask->m_parameters.angularSpeed = 3.0f;
        pedalTask->m_parameters.backPedal = false;
        pedalTask->m_parameters.legStiffness = 9.0f;
        pedalTask->m_parameters.adaptivePedal4Dragging = false;
        pedalTask->m_parameters.speedAsymmetry = 1.0f;
        pedalTask->m_parameters.pedalOffset = 0.05f;
        pedalTask->m_parameters.centreForwards = 0.1f;
        pedalTask->m_parameters.centreSideways = -0.2f;
        pedalTask->m_parameters.centreUp = 0.3f;//bend legs a little
        pedalTask->m_parameters.ellipse = -0.1f;
        pedalTask->m_parameters.radiusVariance = 0.2f;
        pedalTask->activate();
      }
      float upDotUp = m_character->m_gUp.Dot(m_character->m_COMTM.b);
      float tuck = 0.95f - rage::Clamp(upDotUp, 0.85f, 0.95f);
      pedalTask->m_parameters.centreUp = 0.05f + 3.0f*tuck;//bend legs a little
      pedalTask->m_parameters.centreForwards = -0.1f + 1.5f*tuck;
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
        strcpy(feedback->m_behaviourName, NMHighFallFeedbackName);
        feedback->onBehaviourFailure();
      }
    }
  }

  // get feet in position etc..
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUHighFall::duringPrepareForLanding()
  {
    // The state we want to go into for the landing; Arms up, "prepare for landing" pose or catchFall arms,
    // head forward, knees slightly bent, legs slightly forward and to the side.

    //////////////////////////////////////////////////////////////////////////
    //UPPERBODY AND ARMS REACTION

    // Make a "prepare for landing" pose.
    if (m_parameters.m_armsUp <= -3.0f && m_orientationState == NmRsCharacter::OS_Up)
    {
      getSpineInputData()->getLowerNeck()->setDesiredLean1(0.4f);
      getSpineInputData()->getUpperNeck()->setDesiredLean1(0.3f);
      getSpineInputData()->applySpineLean(0.0f, 0.0f);

      // Calculate reaching targets for ik.
      // Set local offsets in chest space.
      // BBDD This is rig dependent code, side offset should be the multiplier of the distance between the shoulders for example.
      const float leftOffset = 0.38f;
      const float frontOffset = -0.32f;
      const float upOffset = 0.43f;
      rage::Vector3 ikPosLocal(upOffset, leftOffset, frontOffset);
      const float twist = 0.6f;

      rage::Matrix34 chestMat; getSpine()->getSpine3Part()->getMatrix(chestMat);

      // Transform ik reaching targets from local chest space to the world space and commit to the ik solver.
      rage::Vector3 ikPos;
      chestMat.Transform(ikPosLocal, ikPos);
      {
        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);
        NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
        ikInputData->setTarget(ikPos);
        ikInputData->setTwist(twist);
        ikInputData->setDragReduction(0.0f);
        getLeftArm()->postInput(ikInput);
      }

      // Reflect sideOffset component of local target for the right arm.
      ikPosLocal.y *= -1.0f;
      chestMat.Transform(ikPosLocal, ikPos);
      {
        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);
        NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
        ikInputData->setTarget(ikPos);
        ikInputData->setTwist(twist);
        ikInputData->setDragReduction(0.0f);
        getRightArm()->postInput(ikInput);
      }
    }
    // Use catchFall arms.
    else if (m_parameters.m_armsUp <= -2.0f)
    {
      NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
      Assert(catchFallTask);
      if (!catchFallTask->isActive())
      {
        catchFallTask->updateBehaviourMessage(NULL); // set parameters to defaults
        catchFallTask->m_parameters.m_legsStiffness = 0.6f*m_parameters.m_bodyStiffness;
        catchFallTask->m_parameters.m_torsoStiffness = 0.85f*m_parameters.m_bodyStiffness;
        catchFallTask->m_parameters.m_armsStiffness = 1.45f*m_parameters.m_bodyStiffness;
        catchFallTask->m_parameters.m_effectorMask = bvmask_UpperBody;
        catchFallTask->activate();
      }
    }
    else //< Ik arms based on m_parameters.m_armsUp.
    {
      rage::Vector3 ikPos;
      rage::Vector3 gUp = m_character->m_gUp;
      gUp.Normalize();

      getSpineInputData()->getLowerNeck()->setDesiredLean1(0.4f);
      getSpineInputData()->getUpperNeck()->setDesiredLean1(0.3f);
      getSpineInputData()->applySpineLean(0.0f, 0.0f);

      // The Arms:
      rage::Matrix34 headMat;
      rage::Vector3 hSideVec;

      getSpine()->getHeadPart()->getMatrix(headMat);
      hSideVec = headMat.a;
      hSideVec.Normalize();
      ikPos.AddScaled(getSpine()->getHeadPart()->getPosition(),gUp,m_parameters.m_armsUp);     
      ikPos.AddScaled(hSideVec,0.3f);
      {
        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);
        NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
        ikInputData->setTarget(ikPos);
        ikInputData->setTwist(0.0f);
        ikInputData->setDragReduction(0.0f);
        getRightArm()->postInput(ikInput);
      }

      ikPos.AddScaled(hSideVec,-0.6f);
      {
        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);
        NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
        ikInputData->setTarget(ikPos);
        ikInputData->setTwist(0.0f);
        ikInputData->setDragReduction(0.0f);
        getLeftArm()->postInput(ikInput);
      }
    }

    //LEG REACTION
    getLeftLegInputData()->getKnee()->setMuscleStiffness(0.5f);
    getRightLegInputData()->getKnee()->setMuscleStiffness(0.5f);
    getLeftLegInputData()->getHip()->setMuscleStiffness(0.5f);
    getRightLegInputData()->getHip()->setMuscleStiffness(0.5f);

    //We use pedal:
    //  Circles are squished to almost give a horizontal line place to give almost straight legs
    //  Circles centres are offset to the right by the value of m_parameters.m_sideD
    //  Circles centres are offset forwards by the value of m_parameters.m_forwardOffsetOfLegIK
    NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
    Assert(pedalTask);
    if (! pedalTask->isActive() )
    {
      pedalTask->updateBehaviourMessage(NULL); // reset params
      pedalTask->m_parameters.randomSeed=(100);
      pedalTask->m_parameters.pedalLeftLeg=(true);
      pedalTask->m_parameters.pedalRightLeg=(true);
      pedalTask->m_parameters.radius=(m_parameters.m_legRadius);
      pedalTask->m_parameters.angularSpeed=(m_parameters.m_legAngSpeed);
      pedalTask->m_parameters.backPedal=(false);
      pedalTask->m_parameters.legStiffness=(m_parameters.m_bodyStiffness);
      pedalTask->m_parameters.adaptivePedal4Dragging=(false);
      pedalTask->m_parameters.speedAsymmetry=(4.0f);
      pedalTask->m_parameters.pedalOffset=(0.0f);
      pedalTask->m_parameters.centreForwards=m_parameters.m_forwardOffsetOfLegIK;
      pedalTask->m_parameters.centreSideways = m_parameters.m_sideD;//0.2f;
      pedalTask->m_parameters.centreUp= 0.0f;
      pedalTask->m_parameters.ellipse = -0.1f;
      pedalTask->m_parameters.radiusVariance = 0.1f;
      pedalTask->activate();
    }
  }


  // one option if we can't balance....
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUHighFall::duringForwardRoll()
  {
    NmRsCBURollDownStairs* rollDownStairsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
    Assert(rollDownStairsTask);

    m_rollToBBtimer += m_character->getLastKnownUpdateStep();

    getSpineInputData()->getSpine3()->setDesiredLean1(1.0f);
    getSpineInputData()->getSpine2()->setDesiredLean1(1.0f);
    getSpineInputData()->getSpine1()->setDesiredLean1(1.0f);
    getSpineInputData()->getSpine0()->setDesiredLean1(1.0f);

    getRightArmInputData()->getClavicle()->setDesiredLean1(0.0f);
    getRightArmInputData()->getClavicle()->setDesiredLean2(0.0f);
    getRightArmInputData()->getClavicle()->setDesiredTwist(0.0f);
    getRightArmInputData()->getShoulder()->setDesiredTwist(-0.8f);
    getRightArmInputData()->getShoulder()->setDesiredLean2(-2.0f);
    getRightArmInputData()->getShoulder()->setDesiredLean1(2.0f);   
    getRightArmInputData()->getElbow()->setDesiredAngle(1.4f);

    getLeftArmInputData()->getClavicle()->setDesiredLean1(0.0f);
    getLeftArmInputData()->getClavicle()->setDesiredLean2(0.0f);
    getLeftArmInputData()->getClavicle()->setDesiredTwist(0.0f);
    getLeftArmInputData()->getShoulder()->setDesiredTwist(-0.8f);
    getLeftArmInputData()->getShoulder()->setDesiredLean2(-2.0f);
    getLeftArmInputData()->getShoulder()->setDesiredLean1(2.0f);      
    getLeftArmInputData()->getElbow()->setDesiredAngle(1.6f);

    getLeftLegInputData()->getHip()->setDesiredLean1(2.25f);
    getRightLegInputData()->getHip()->setDesiredLean1(2.3f);

    getLeftLegInputData()->getKnee()->setDesiredAngle(-2.0f);
    getRightLegInputData()->getKnee()->setDesiredAngle(-2.0f);
    getSpineInputData()->getLowerNeck()->setDesiredTwist(0.0f);
    getSpineInputData()->getLowerNeck()->setDesiredLean2(0.0f);
    getSpineInputData()->getLowerNeck()->setDesiredLean1(3.0f);
    getSpineInputData()->getUpperNeck()->setDesiredTwist(0.0f);
    getSpineInputData()->getUpperNeck()->setDesiredLean2(0.0f);
    getSpineInputData()->getUpperNeck()->setDesiredLean1(3.0f);

    //-- relax to roll with the reduction of the comRotVel
    float comVM = m_character->m_COMvelRelativeMag;
    m_averageComVel = (9.0f*m_averageComVel+comVM)/(10.0f);
    float stiffness = rage::Clamp(m_parameters.m_bodyStiffness - m_rollToBBtimer*12.0f,8.0f,12.0f);

    m_body->setStiffness(stiffness, m_parameters.m_bodydamping);

    //-- need to push legs based on orientation
    rage::Vector3 theUpVec = m_character->m_gUp;
    rage::Matrix34 chCOMMAT = m_character->m_COMTM;
    rage::Vector3 characterUp = chCOMMAT.b;
    //-- should really project into the up/forward plane !!!!!!

    float upDotG = theUpVec.Dot(characterUp);

    getLeftLegInputData()->getKnee()->setDesiredAngle(upDotG-1.75f);
    getRightLegInputData()->getKnee()->setDesiredAngle(upDotG-1.65f);

    //-- also need to blend with the forward roll..

    if (m_parameters.m_useZeroPose_withForwardRoll)
      m_body->blendToZeroPose(1, bvmask_Full);

    // m_character->keepHeadAwayFromGround(2.70f);

    if ( (!rollDownStairsTask->isActive()) && !m_rollToBB)
    {
      rollDownStairsTask->updateBehaviourMessage(NULL); // sets to defaults.
      rollDownStairsTask->m_parameters.m_ForceMag = 1.0f;
      rollDownStairsTask->m_parameters.m_SpinWhenInAir = false;
      rollDownStairsTask->m_parameters.m_onlyApplyHelperForces = true;
      rollDownStairsTask->activate();
    }

    //success message
    if ((m_rollToBBtimer > 1.6f) && !m_messageSent)
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        m_messageSent = true;
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 0;
        strcpy(feedback->m_behaviourName, NMHighFallFeedbackName);
        feedback->onBehaviourFailure(); // finishing the forwardRoll =  failure. Keep success for ending up balancing.
      }
    }
  }

  // another option if we can't balance
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUHighFall::duringCatchFall()
  {
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    if (!catchFallTask->isActive())
    {
      catchFallTask->updateBehaviourMessage(NULL); // set parameters to defaults
      catchFallTask->m_parameters.m_legsStiffness = 0.6f*m_parameters.m_bodyStiffness;
      catchFallTask->m_parameters.m_torsoStiffness = 0.85f*m_parameters.m_bodyStiffness;
      catchFallTask->m_parameters.m_armsStiffness = 1.45f*m_parameters.m_bodyStiffness;
      //cFParameters.m_forwardMaxArmOffset = .52f;
      catchFallTask->activate();
    }
    getSpine()->keepHeadAwayFromGround(getSpineInput(), 2.0f);//mmmtodo this doesn't work as catchFall calls headlook and overrides this 

    if (!m_parameters.m_balance) //less stiff H+K catchfall?
    {
      float stiff = 0.5f;

      m_body->setStiffness(6.f, 1.f, bvmask_Full, &stiff);

      ////stop bounce if high
      //m_character->setBodyStiffness(4.f,2.5f,"fb",&stiff);
      //m_character->setElasticityMultiplier(0.0f, bvmask_Full);
    }

    // if we've come to rest, fin
    if (!m_messageSent)
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback && m_character->m_COMvelRelativeMag < 0.075f)
      {
        m_messageSent = true;
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 0;
        strcpy(feedback->m_behaviourName, NMHighFallFeedbackName);
        feedback->onBehaviourFailure();
      }
    }
  }

  // during balance
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUHighFall::duringBalance()
  {
    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    Assert(bodyBalanceTask);
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);

    if (!bodyBalanceTask->isActive())
    {
      dynamicBalancerTask->taperKneeStrength(false);
      //tune params to body balance
      bodyBalanceTask->updateBehaviourMessage(NULL);// sets values to defaults
      bodyBalanceTask->m_parameters.m_useHeadLook =  true;
      bodyBalanceTask->m_parameters.m_headLookPos.x = 0.0f;
      bodyBalanceTask->m_parameters.m_headLookPos.y = 0.0f;
#ifdef NM_COWBOY
      if (m_character->getBodyIdentifier() == rdrCowboy || m_character->getBodyIdentifier() == rdrCowgirl)//Up the position, as 0 is at feet
        bodyBalanceTask->m_parameters.m_headLookPos.y = 1.0f;
#endif//#ifdef NM_COWBOY
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
      bodyBalanceTask->m_parameters.m_armsOutOnPushTimeout = 1.0f;//number of seconds before turning off the armsOutOnPush response only for Arms out based on lean2 of the legs (NOT for the angle or angular velocity) 

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
      //  bodyBalanceTask->setGiveUpThreshHold(1.0f);
      //  bodyBalanceTask->setBodyStiffness(m_parameters.m_bodyStiffness*(9.0f/11.0f));
      //  dynamicBalancerTask->setKneeStrength(0.4f);
      m_rollToBBtimer = 0.0f;
    }
    m_rollToBBtimer += m_character->getLastKnownUpdateStep();       

    if (!m_parameters.m_balance) //collapse the bodyBalance
    {
      float stiff = 0.5f;

      m_body->setStiffness(6.f, 1.f, bvmask_Spine | bvmask_ArmLeft | bvmask_ArmRight, &stiff);

      getLeftLegInputData()->getHip()->setStiffness(6.f,1.f,&stiff);
      getRightLegInputData()->getHip()->setStiffness(6.f,1.f,&stiff);
      getLeftLegInputData()->getKnee()->setStiffness(6.f,1.f,&stiff);
      getRightLegInputData()->getKnee()->setStiffness(6.f,1.f,&stiff);
    }
    else
    {
      if (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
      {
        dynamicBalancerTask->taperKneeStrength(false);

        float  hipPitch = rage::Clamp(-0.5f + m_rollToBBtimer, -0.5f, 0.0f);
        NM_RS_DBG_LOGF(L"hipPitch = : %.5f", hipPitch);
        dynamicBalancerTask->setHipPitch(hipPitch);

        float legStr = rage::Clamp(m_parameters.m_legStrength,1.0f,12.0f);
        float kneeStr = rage::Clamp((legStr/12.0f)*(-1.0f+0.5f+(1.0f + m_rollToBBtimer)*(1.0f + m_rollToBBtimer)),0.5f,1.0f);
        NM_RS_DBG_LOGF(L"kneeStr = : %.5f", kneeStr);
        dynamicBalancerTask->setKneeStrength(kneeStr);
      }
      // if (! dynamicBalancerTask->hasFailed()) 
      // {
      //   float legStre = rage::Clamp(m_parameters.m_legStrength + m_rollToBBtimer, 1.0f, 12.0f);
      //   dynamicBalancerTask->setStiffness(legStre);
      // }
    }


    // send feeback
    if (!m_messageSent)
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback && m_character->m_COMvelRelativeMag < 0.15f)
      {
        m_messageSent = true;
        feedback->m_agentID = m_character->getID();
        feedback->m_argsCount = 0;
        strcpy(feedback->m_behaviourName, NMHighFallFeedbackName);
        if (dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK)
          feedback->onBehaviourFailure();
        else
          feedback->onBehaviourSuccess();
      }
    }
  }

  // windmill, pedalling etc...
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUHighFall::duringFall()
  {
    
    probeEnvironmentAndCalcForFall();

    NmRsCBUHeadLook* headLookTask = (NmRsCBUHeadLook*)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
    Assert(pedalTask);
    NmRsCBUArmsWindmillAdaptive* armsWindmillAdaptiveTask = (NmRsCBUArmsWindmillAdaptive*)m_cbuParent->m_tasks[bvid_armsWindmillAdaptive];
    Assert(armsWindmillAdaptiveTask);

    // ACTIVATE: HeadLook, Pedal and ArmsWindmillAdaptive if not already on
    if (! headLookTask->isActive() )
    {
      headLookTask->updateBehaviourMessage(NULL);
      headLookTask->activate();
      headLookTask->m_parameters.m_alwaysLook = true;
    }

    if (! pedalTask->isActive() )
    {
      pedalTask->updateBehaviourMessage(NULL); // reset params
      pedalTask->m_parameters.randomSeed = 100;
      pedalTask->m_parameters.pedalLeftLeg = true;
      pedalTask->m_parameters.pedalRightLeg = true;
      pedalTask->m_parameters.radius = m_parameters.m_legRadius;
      pedalTask->m_parameters.legStiffness = m_parameters.m_bodyStiffness;
      pedalTask->m_parameters.adaptivePedal4Dragging = false;
      pedalTask->m_parameters.pedalOffset = 0.0f;
      pedalTask->m_parameters.backPedal = false;//mmmmSET every tick
      pedalTask->m_parameters.centreForwards = 0.0f;
      pedalTask->activate();
    }

    float armAngle = -m_parameters.m_arms2LegsPhase;//0.0 running, 1.5 opposite, 3.14159 in phase looks bad, 4.65 in phase hands go down as knees go down looks ok.
    if (! armsWindmillAdaptiveTask->isActive() )
    {
      armsWindmillAdaptiveTask->updateBehaviourMessage(NULL); // initialise parameters.
      armsWindmillAdaptiveTask->m_parameters.armDirection = 1;
      armsWindmillAdaptiveTask->activate();
      //synchronize the arms to the legs
      if (m_parameters.m_arms2LegsSync == 2)
      {
        if (pedalTask->isActive())//should be
          armAngle += pedalTask->getAngle();
        armsWindmillAdaptiveTask->setAngle(armAngle);
      }
    }

    //UPDATE Pedal and ArmsWindmillAdaptive
      rage::Matrix34 tmCom(m_character->m_COMTM);
    rage::Vector3 bodyUp = tmCom.b;
      rage::Vector3 bodyBack = tmCom.c;
    if (m_parameters.m_adaptiveCircling)
    {
      rage::Vector3 bodyRight = tmCom.a;

      float somersaultAngMom = bodyRight.Dot(m_character->m_angMom);//+ve is forward
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "HighFall - 1", somersaultAngMom);
      bspyScratchpad(m_character->getBSpyID(), "HighFall - 1", m_character->m_angMom);
#endif
      float qSom;
      float arcsin = bodyBack.z;
      qSom = rage::AsinfSafe(arcsin);//gives forward horizontal = PI/2, upright = 0, lying on back = -PI/2
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "HighFall - 1", qSom);
#endif
      if (bodyUp.z < 0.f && qSom>0.f)
        qSom = PI - qSom;
      else if (bodyUp.z < 0.f && qSom<0.f)
        qSom = -PI - qSom;

      //qSom *= 6.8f;

      static float angleOffset = 0.0f; //mmmmtodo need to add this to target angle for torques 1.0f;
      qSom += angleOffset;

      if (somersaultAngMom>12.f || somersaultAngMom<-12.f || 
        (qSom < 0.f && somersaultAngMom > 0.f) || 
        (qSom > 0.f && somersaultAngMom < 0.f) )
        somersaultAngMom -= qSom;
      else
        somersaultAngMom = -qSom;
#if highFall_DEVEL//Remove quick switching  between backwards and forwards circling
#if ART_ENABLE_BSPY
      float somersaultAngVel = bodyRight.Dot(m_character->m_COMrotvel);//+ve is forward
      bspyScratchpad(m_character->getBSpyID(), "HighFall - 2", somersaultAngVel);
      bspyScratchpad(m_character->getBSpyID(), "HighFall - 2", somersaultAngMom);
      bspyScratchpad(m_character->getBSpyID(), "HighFall - 2", qSom);
#endif
      static float hyster = 2.2f;
      static float hyster2 = 2.2f;
      static float upDownMult = 1.0f;
      if (somersaultAngMom < 0.f && somersaultAngMom < -hyster)
      {
        pedalTask->m_parameters.backPedal = false;             // Falling forwards.
        armsWindmillAdaptiveTask->m_parameters.armDirection = 1;
      }
      else if (somersaultAngMom > 0.f && somersaultAngMom > hyster)
      {
        pedalTask->m_parameters.backPedal = true;            // Falling backwards.
        armsWindmillAdaptiveTask->m_parameters.armDirection = -1;// Falling backwards.
      }        
      float ellipse = 1.0f;
      if (somersaultAngMom < 0.f && somersaultAngMom > -hyster2/*&& pedalTask->m_parameters.backPedal*/)
      {
        ellipse = rage::Clamp(-upDownMult*somersaultAngMom/hyster2, 0.0f, 1.0f);//from circle to ellipse to vertical line
      }
      else if (somersaultAngMom > 0.f && somersaultAngMom < hyster2/*&& !pedalTask->m_parameters.backPedal*/)
      {
        ellipse = rage::Clamp(upDownMult*somersaultAngMom/hyster2, 0.0f, 1.0f);//from circle to ellipse to vertical line
      }        
      pedalTask->m_parameters.ellipse = ellipse;
      armsWindmillAdaptiveTask->m_parameters.lean1mult = ellipse;
#else
      if (somersaultAngMom < 0.f)
        pedalTask->m_parameters.backPedal = false;             // Falling forwards.
      else
        pedalTask->m_parameters.backPedal = true;            // Falling backwards.
#endif//#if highFall_DEVEL
    }//if (m_parameters.m_adaptiveCircling)

    //Hula reaction if can't see floor and not rotating fast.
    //  A horizontal (to the character) circle is followed by both legs
    // (so that if standing up the character would do a hula hoop movement at the hips)
    //  This is the reaction used by gymnasts and cats to twist the body while airborne
    //  especially when no somersault angular momentum is present.
      if (m_parameters.m_hula)
      {
        if (bodyBack.z < -0.2f && m_hulaTime < 0.f && bodyUp.z < 0.f && m_character->m_COMrotvelMag < 7.f)
        {
          m_hulaTime = 1.0f;
        }
        if ((bodyBack.z > 0.2f || bodyUp.z > 0.f) && m_hulaTime < 0.5f)
        {
          m_hulaTime = -0.1f;
          m_hulaTotalTime *= -1.f;
        }
        if (m_hulaTotalTime < 0.f)
        {
          m_hulaTotalTime += m_character->getLastKnownUpdateStep();
          // getSpine()->getPelvisPart()->applyTorque(bodyUp*10.f);
        }
        //float twistAngVel = m_character->m_COMrotvel.Dot(bodyUp);
        //NM_RS_DBG_LOGF(L"twistAngVel: %f", twistAngVel);

    }
    else//Turn off hula reaction
      m_hulaTime = -0.1f;

        if (m_hulaTime > 0.f)
        {
          m_hulaTotalTime += m_character->getLastKnownUpdateStep();
          m_hulaTime -= m_character->getLastKnownUpdateStep();
          if (m_hulaTime <= 0.f)
            m_hulaTotalTime *= -1.f;
      //set the hula pedal parameters
          pedalTask->m_parameters.hula = true;
          pedalTask->m_parameters.radius=1.f;
          pedalTask->m_parameters.angularSpeed=8.f;
          pedalTask->m_parameters.centreUp=-0.25f;
          pedalTask->m_parameters.radiusVariance = 0.f;
          pedalTask->m_parameters.legAngleVariance = 0.f;
          pedalTask->m_parameters.speedAsymmetry = 0.f;
          pedalTask->m_parameters.backPedal = true;
          //float twistAngVel = rage::Clamp(15.f/m_character->m_COMrotvel.Dot(bodyUp),-15.f,15.f);
          //NM_RS_DBG_LOGF(L"twistAngVel: %f", twistAngVel);
          //  
          //getSpine()->getPelvisPart()->applyTorque(bodyUp*twistAngVel);
          //if (twistAngVel>0.f)//when was just m_character->m_COMrotvel.Dot(bodyUp)
          //  getSpine()->getPelvisPart()->applyTorque(bodyUp*12.f);
          //else
          //  getSpine()->getPelvisPart()->applyTorque(-bodyUp*12.f);
          //getSpine()->getPelvisPart()->applyTorque(-bodyUp*10.f);
        }
        else
        {
      //non-hula pedal parameters
      pedalTask->m_parameters.hula = false;
      pedalTask->m_parameters.radius = m_parameters.m_legRadius;
      pedalTask->m_parameters.angularSpeed = m_parameters.m_legAngSpeed;
          pedalTask->m_parameters.centreUp=-0.1f;
      pedalTask->m_parameters.radiusVariance = 0.4f;//default
      pedalTask->m_parameters.legAngleVariance = 0.5f;//default
      pedalTask->m_parameters.speedAsymmetry = m_parameters.m_legAsymmetry;
      }


    //UPDATE: ArmsWindmillAdaptive
    //synchronize the arms to the legs (so that pedalTask->m_parameters.speedAsymmetry can be non zero and the arms/legs will still synch update every step)
    //We synch the arms to the legs because:
    //  Pedal has a de-synching parameter speedAsymmetry and armswindmill doesn't.
    //  Setting the leg angle destroys any de-synching of the leg circles.
    if (m_parameters.m_arms2LegsSync == 1)
    {
      if (pedalTask->isActive())//should be
        armAngle += pedalTask->getAngle();
      armsWindmillAdaptiveTask->setAngle(armAngle);
    }
    // update the armsWindMillTask
    // parameters updated here as teeter is using armswindmill and needs to be overidden by HighFall 
    armsWindmillAdaptiveTask->m_parameters.bodyStiffness = m_parameters.m_bodyStiffness;
    armsWindmillAdaptiveTask->m_parameters.armStiffness = m_parameters.m_bodyStiffness*14.0f/11.0f;
    armsWindmillAdaptiveTask->m_parameters.angSpeed = m_parameters.m_armAngSpeed;
    armsWindmillAdaptiveTask->m_parameters.amplitude = m_parameters.m_armAmplitude;
    armsWindmillAdaptiveTask->m_parameters.phase = m_parameters.m_armPhase;//1.0f;
    armsWindmillAdaptiveTask->m_parameters.disableOnImpact = false;
    armsWindmillAdaptiveTask->m_parameters.bendLeftElbow = m_parameters.m_armBendElbows;
    armsWindmillAdaptiveTask->m_parameters.bendRightElbow = m_parameters.m_armBendElbows;


    //UPDATE: Headlook
    // get headLook position
    getSpineInputData()->applySpineLean(0.0f,0.0f);

    rage::Matrix34 comTM =  m_character->m_COMTM;
    rage::Vector3 currentForwardVec = comTM.c;
    currentForwardVec.Negate();
    m_character->levelVector(currentForwardVec);
    rage::Vector3 comPos = m_character->m_COM;
    rage::Vector3 gUp = m_character->m_gUp;
    currentForwardVec = comPos + currentForwardVec - gUp - gUp;

    headLookTask->m_parameters.m_pos = currentForwardVec;
    headLookTask->m_parameters.m_instanceIndex = -1;
    headLookTask->m_parameters.m_stiffness = m_parameters.m_bodyStiffness;
    headLookTask->m_parameters.m_damping = m_parameters.m_bodydamping;
    headLookTask->m_parameters.m_vel = m_character->m_COMvel;

  }


  // ---------------------------------------------------------------------------------------------------------
/*
  Applies the orientation controller
*/
  void NmRsCBUHighFall::orientationControllerTick()
  {

    // so that BodyBalance (executed before HighFall) can set its own upper body stiffness/damping values
    if (! (m_currentState == HF_Balance || m_currentState == HF_CatchFall || m_currentState == HF_BailOut) ) 
    {
      m_body->setStiffness(m_parameters.m_bodyStiffness,m_parameters.m_bodydamping);
    }

    rage::Vector3 feetCentre = getRightLeg()->getFoot()->getPosition() + getLeftLeg()->getFoot()->getPosition();
    feetCentre.Scale(0.5f);
    
    // the vertical velocity
    rage::Vector3 upVec = m_character->getUpVector();
    rage::Vector3 comVel = m_character->m_COMvel;

    //Apply the orientation control
    if (m_controlOrientation && m_parameters.m_pdStrength > 0.001f)
    {

      rage::Matrix34 currentMat;
      float aimAngle = 0.0f;//Just declared here because of bSpy output
      //Input to orientation controller
      rage::Vector3 torqueVec;
      float rotation;

      if (m_parameters.m_alanRickman)
      {
        //Orientate the body to face the camera (at the moment the the COM position when the behaviour activates)
        rage::Vector3 camera2Com = m_character->m_COM - m_cameraPos;
        camera2Com.Normalize();
        getSpine()->getPelvisPart()->getMatrix(currentMat); // m_character->m_COMTM;
        torqueVec.Cross(currentMat.c, camera2Com);//Rotation axis between the current and desired back of the body
        rotation = rage::AsinfSafe(torqueVec.Mag());
      }
      else
      {
        //Orientate the body to aimAngleBase
    float forwardVelRotation = m_parameters.m_forwardVelRotation;
    float aimAngleBase = m_parameters.m_aimAngleBase;

        //Overwrite orientation parameters for when preparing for a forward roll
        static float frAim = 0.7f;
        if (m_currentState == HF_ForwardRoll && m_parameters.m_forwardRoll)
    {
      forwardVelRotation = 0.0f;
          aimAngleBase = frAim;//slightly forward
    }

    //-- work out the aim matrix to orientate body to falling direction
        rage::Vector3 backAxis = m_character->m_COMTM.c;//Back
        rage::Vector3 leftAxis = m_character->m_COMTM.a;//Right - it is reversed later on

    // here work out the forward Velocity
        rage::Vector3 horizontalVel = comVel;
        m_character->levelVector(horizontalVel);
        m_forwardVelMag = -horizontalVel.Dot(backAxis);

        if ((rage::Abs(m_forwardVelMag) < 1.0f) || (!m_parameters.m_orientateBodyToFallDirection))
        { 
          //When we are falling straight down
          //  backAxis = bodyRight x Up
          //  leftAxis = bodyLeft
          m_character->levelVector(leftAxis);
          backAxis.Cross(leftAxis,upVec);
    }
    else
    {
          //When we are falling with a large forward velocity
          //  backAxis = -horizontalVelocity
          //  leftAxis = Up x horizontalVelocity
          backAxis = m_character->m_COMvel;
          backAxis.Scale(-1.0f);
          m_character->levelVector(backAxis);
          backAxis.Normalize();
          leftAxis.Cross(upVec,backAxis);       
    }

    // setup desired pelvis matrix
        leftAxis.Scale(-1.0f);
        rage::Matrix34 aimMat;
        aimMat.Set(upVec,leftAxis,backAxis,feetCentre);

    // rotate desired pelvis matrix by aim angle
        aimAngle = rage::Clamp(aimAngleBase + forwardVelRotation*m_forwardVelMag, -PI, PI);//,-0.5f, 0.7f);
    aimMat.RotateLocalY(aimAngle);

    // get necessary rotation from current to desired pelvis matrix
        getSpine()->getPelvisPart()->getMatrix(currentMat);
#if ART_ENABLE_BSPY
        m_character->bspyDrawCoordinateFrame(0.5f, aimMat);
        m_character->bspyDrawCoordinateFrame(0.3f, currentMat);
#endif
        if (m_parameters.m_orientateTwist)
        {
    currentMat.Inverse();
          rage::Matrix34 rotationMat;
          rotationMat.Dot(currentMat,aimMat);

    // transform necessary rotation to torque vector
    rage::Quaternion q;
          rotationMat.ToQuaternion(q);
    q.ToRotation(torqueVec,rotation);
          if(rotation > PI)
            rotation -= 2.f*PI;
          else if(rotation < -PI)
            rotation += 2.f*PI;
        }
        else
        {
          //ignore the twist
          getSpine()->getPelvisPart()->getMatrix(currentMat);
          torqueVec.Cross(currentMat.a, aimMat.a);//Rotation axis between the current and desired up of the body
          rotation = rage::AsinfSafe(torqueVec.Mag());
        }
      }

    // if forward velocity too small, only torque sideways
      //if (rage::Abs(m_forwardVelMag) < 0.7f)
      //{
      //  leftAxis = aimMat.b;
      //  float torqueOntoSide = torqueVec.Dot(leftAxis);
      //  torqueVec = leftAxis;
      //  torqueVec.Scale(torqueOntoSide);
      //}

      float velocityOntoTorqueVec = torqueVec.Dot(m_character->m_COMrotvel);
      float P = 260;  // 160 -- maybe need to ramp these up
      float D = 80;   // 60
      //float torqMag = m_parameters.m_pdStrength*(P*rotation - D*velocityOntoTorqueVec);
      float spring = m_parameters.m_pdStrength*P*rotation;
      float damping = m_parameters.m_pdStrength*m_parameters.m_pdDamping*D*velocityOntoTorqueVec;
      float torqMag;
#if ART_ENABLE_BSPY
      torqMag = m_parameters.m_pdStrength*(P*rotation - m_parameters.m_pdDamping*D*velocityOntoTorqueVec);
      bspyScratchpad(m_character->getBSpyID(), "HighFall - orient", aimAngle);
      bspyScratchpad(m_character->getBSpyID(), "HighFall - orient", rotation);
      bspyScratchpad(m_character->getBSpyID(), "HighFall - orient", torqueVec);
      bspyScratchpad(m_character->getBSpyID(), "HighFall", torqMag);
      bspyScratchpad(m_character->getBSpyID(), "HighFall", spring);
      bspyScratchpad(m_character->getBSpyID(), "HighFall", damping);
#endif
      torqMag =rage::Clamp(spring-damping, -m_parameters.m_orientateMax, m_parameters.m_orientateMax);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "HighFall", torqMag);
#endif
      float scale = rage::Clamp(60.f * m_character->getLastKnownUpdateStep(), 1.f, 60.f);//clamp to keep torque same above 60fps and reduce torque from 60fps to 1fps
      torqMag = torqMag/scale;
      torqueVec.Scale(torqMag);
      getSpine()->getPelvisPart()->applyTorque(torqueVec);
    }//if (m_controlOrientation)//Apply the orientation control
    }      

    void NmRsCBUHighFall::calculateCommonVariables()
    {
      // Do we need to do a catch fall?
    m_goToCatchFallFlag = false;
    if (m_character->m_gUp.Dot(m_character->m_COMTM.b) < m_parameters.m_catchFallCutOff)
      {
      m_goToCatchFallFlag = true;
      }

      // Obtain horizontal COM velocity component.
      m_horComVel = m_character->m_COMvel;
      m_character->levelVector(m_horComVel);

      // Is character facing the direction it is moving?
      m_impactVisible = isCharacterFacingWhereItTravels();

      // Get current facing direction.
      m_orientationStateCurrent = m_character->getFacingDirectionFromCOMOrientation(m_character->m_COMTM);
    }      

    // Function updates the following member variables: m_probeHit, m_normalDotUp, m_willLandOnFeet, m_predImpactTime and m_orientationState.
    void NmRsCBUHighFall::probeEnvironmentAndCalcForFall() 
    {
      // Send a probe out in the direction of the fall, from Agent COM position, for the height.
      rage::Vector3 endOfProbe;
      endOfProbe.AddScaled(m_character->m_COMTM.d, m_character->m_COMvel, 5);
      m_probeHit = m_character->probeRay(NmRsCharacter::pi_highFall, m_character->m_COMTM.d, endOfProbe, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);

      // work out the vertical height vector to the ground
      rage::Vector3 hitDirVert = m_character->m_COMTM.d - m_character->m_probeHitPos[NmRsCharacter::pi_highFall];
      rage::Vector3 hitDirHor = hitDirVert;
      m_character->levelVector(hitDirHor);
      hitDirVert = hitDirVert - hitDirHor;

      m_normalDotUp = m_character->m_probeHitNormal[NmRsCharacter::pi_highFall].Dot(m_character->getUpVector());
      m_landingNormalOK = m_normalDotUp > m_parameters.m_landingNormal;

      // Get height above ground.
      float height = hitDirVert.Mag();

      //m_willLandOnFeet is only used on one frame to decide whether to bailout or prepare for landing
      //  don't think it makes sense to have an accuracy blend on it
      //float accuracy = 1.0f-((m_predImpactTime-m_timer)/m_predImpactTime);
      //We could add a prediction to this based on angVel and m_predImpactTime.
      //However the angVel will change due to the circling stopping and based on the decision to bail or prepare
      m_willLandOnFeet = true;
      if (m_character->m_gUp.Dot(m_character->m_COMTM.b) < m_parameters.m_crashOrLandCutOff)
      {
        m_willLandOnFeet = false;
      }

      // If Agent is due to land on its feet, decrease the height of the fall that is measured
      // from the probe pos (currently at the COM) with the length of the leg.
      if (m_willLandOnFeet && height > m_leftLegLength)
      {
        height -= m_leftLegLength;
      }

      const float verticalVelMag = (m_character->m_COMvel).Dot(m_character->getUpVector());

      if (m_probeHit)
      {
        //-- the solution to the quadratic equation is: (time to impact)
        rage::Vector3 gravity = rage::phSimulator::GetGravity();
        float a = gravity.Mag();
        float b = -rage::Abs(verticalVelMag);
        float c = height;
        Assert(b*b+2*a*c >= 0.f && rage::Abs(a)>1e-10f);
        m_predImpactTime = m_timer + (b+sqrtf(b*b+2*a*c))/(a);
      }
      else
      {
        m_predImpactTime = m_timer + 30.f; // default to never reach
      }

      // Get Agent predicted COMTM orientation on impact.
      // NOTE: Predicted impact time is enlarged with m_timer value (see m_predImpactTime calculation).
      // Prediction requires the net time to impact, not the relative time.
      rage::Matrix34 predCOMTM;
      m_character->getPredictedCOMOrientation((m_predImpactTime - m_timer), &predCOMTM);

      // Check which way Agent would be facing if lying on the ground with predicted orientation:
      // Up, Down, Front, Back, Left, Right.
      m_orientationState = m_character->getFacingDirectionFromCOMOrientation(predCOMTM);

      m_willLandOnFeet = (m_orientationState == NmRsCharacter::OS_Up) ? true : false;

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "HighFall - probe", (endOfProbe-m_character->m_COMTM.d));
    bspyScratchpad(m_character->getBSpyID(), "HighFall - probe", m_character->m_probeHitPos[NmRsCharacter::pi_highFall]);
    bspyScratchpad(m_character->getBSpyID(), "HighFall - probe", hitDirVert);
    bspyScratchpad(m_character->getBSpyID(), "HighFall - probe", height);
    // Predicted COMTM.
    m_probeHitPos = m_character->m_probeHitPos[NmRsCharacter::pi_highFall];
    predCOMTMXAxis = predCOMTM.a + m_probeHitPos;
    predCOMTMYAxis = predCOMTM.b + m_probeHitPos;
    predCOMTMZAxis = predCOMTM.c + m_probeHitPos;
#endif
    }

  float NmRsCBUHighFall::getLeftLegLength() const
  {
    // Measure left leg length.
    float legLength = (getLeftLeg()->getKnee()->getJointPosition() - getLeftLeg()->getHip()->getJointPosition()).Mag();
    legLength += (getLeftLeg()->getKnee()->getJointPosition() - getLeftLeg()->getAnkle()->getJointPosition()).Mag();
    return legLength;
  }

  void NmRsCBUHighFall::handAnimationFeedback()
  {
    HandAnimationType handAnimType = haNone;
    
    switch (GetState())
    {
    case HF_Falling:
    case HF_PrepareForLanding:
      handAnimType = haFlail;
      break;
    case HF_ForwardRoll:
    case HF_BraceForImpact:
      handAnimType = haBrace;
      break;
    case HF_BailOut:
    case HF_NonRecoverableFall:
      handAnimType = haLoose;
      break;
    case HF_Balance:
      handAnimType = haNone;
      break;
    case HF_CatchFall:
      handAnimType = haNone;
      if (m_impactAnimationTime > 0.0f)
        handAnimType = haImpact;//only for a second
      break;
    default:
      {
        Assert(0);
      }
      break;
    }
    
    if (m_handAnimationType != handAnimType)//handAnimationType has changed
    {
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();

        feedback->m_argsCount = 2;
        ART::ARTFeedbackInterface::FeedbackUserdata data;
        //left hand
        data.setInt(NmRsCharacter::kLeftHand);
        feedback->m_args[0] = data;
        data.setInt(handAnimType);
        feedback->m_args[1] = data;
        strcpy(feedback->m_behaviourName, NMHandAnimationFeedbackName);
        feedback->onBehaviourEvent();
        //right hand
        data.setInt(NmRsCharacter::kRightHand);
        feedback->m_args[0] = data;
        //data.setInt(handAnimType);
        //feedback->m_args[1] = data;
        //strcpy(feedback->m_behaviourName, NMHandAnimationFeedbackName);
        feedback->onBehaviourEvent();
      }
      m_handAnimationType = handAnimType;
    }

  }

#if ART_ENABLE_BSPY
  void NmRsCBUHighFall::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.m_bodyStiffness, true);
    bspyTaskVar(m_parameters.m_bodydamping, true);
    bspyTaskVar(m_parameters.m_catchfalltime, true);
    bspyTaskVar(m_parameters.m_crashOrLandCutOff, true);
    bspyTaskVar(m_parameters.m_pdStrength, true);
    bspyTaskVar(m_parameters.m_pdDamping, true);
    bspyTaskVar(m_parameters.m_catchFallCutOff, true);
    bspyTaskVar(m_parameters.m_armsUp, true);
    bspyTaskVar(m_parameters.m_legRadius, true);
    bspyTaskVar(m_parameters.m_legAngSpeed, true);
    bspyTaskVar(m_parameters.m_legAsymmetry, true);
    bspyTaskVar(m_parameters.m_arms2LegsPhase, true);
    bspyTaskVar(m_parameters.m_arms2LegsSync, true);
    bspyTaskVar(m_parameters.m_armAngSpeed, true);
    bspyTaskVar(m_parameters.m_armAmplitude, true);
    bspyTaskVar(m_parameters.m_armPhase, true);
    bspyTaskVar(m_parameters.m_armBendElbows, true);
    bspyTaskVar(m_parameters.m_aimAngleBase, true);
    bspyTaskVar(m_parameters.m_forwardVelRotation, true);
    bspyTaskVar(m_parameters.m_footVelCompScale, true);
    bspyTaskVar(m_parameters.m_legL, true);//unused
    bspyTaskVar(m_parameters.m_sideD, true);
    bspyTaskVar(m_parameters.m_forwardOffsetOfLegIK, true);
    bspyTaskVar(m_parameters.m_legStrength, true);
    bspyTaskVar(m_parameters.m_orientateBodyToFallDirection, true);
    bspyTaskVar(m_parameters.m_orientateTwist, true);
    bspyTaskVar(m_parameters.m_orientateMax, true);
    bspyTaskVar(m_parameters.m_alanRickman, true);
    bspyTaskVar(m_parameters.m_forwardRoll, true);
    bspyTaskVar(m_parameters.m_useZeroPose_withForwardRoll, true);
    bspyTaskVar(m_parameters.m_ignorWorldCollisions, true);
    bspyTaskVar(m_parameters.m_adaptiveCircling, true);
    bspyTaskVar(m_parameters.m_hula, true);
    bspyTaskVar(m_parameters.m_maxSpeedForRecoverableFall, true);
    bspyTaskVar(m_parameters.m_landingNormal, true);

    bspyTaskVar(m_timer, false);
    bspyTaskVar(m_predImpactTime, false);
    bspyTaskVar(m_forwardVelMag, false);
    bspyTaskVar(m_rollToBBtimer, false);
    bspyTaskVar(m_averageComVel, false);
    bspyTaskVar(m_hasCollidedWithWorld, false);
    bspyTaskVar(m_controlOrientation, false);
    bspyTaskVar(m_willLandOnFeet, false);
    bspyTaskVar(m_messageSent, false);
    bspyTaskVar(m_goToCatchFallFlag, false);
    bspyTaskVar(m_rollToBB, false);
    bspyTaskVar(m_probeHit, false);
    bspyTaskVar(m_probeHitPos, false);
    bspyTaskVar(predCOMTMXAxis, false);
    bspyTaskVar(predCOMTMYAxis, false);
    bspyTaskVar(predCOMTMZAxis, false);
    bspyTaskVar(m_normalDotUp, false);
    bspyTaskVar(m_fallRecoverable, false);
    bspyTaskVar(m_impactVisible, false);
    bspyTaskVar(m_landingNormalOK, false);
    bspyTaskVar(m_horComVel, false);
    bspyTaskVar(m_impactAnimationTime, false);

    static const char* state_names[] = 
    {
#define HF_NAME_ACTION(_name) #_name ,
      HF_STATES(HF_NAME_ACTION)
#undef HF_NAME_ACTION
    };
    bspyTaskVar_StringEnum((int)GetState(),state_names, false);

    static const char* orientationState_names[] =
    {
#define HF_ORIENTATION_STATE_NAME_ACTION(_name) #_name ,
      ORIENTATION_STATES(HF_ORIENTATION_STATE_NAME_ACTION)
#undef HF_ORIENTATION_STATE_NAME_ACTION
    };
    bspyTaskVar_StringEnum(m_orientationState, orientationState_names, false);
    bspyTaskVar_StringEnum(m_orientationStateTimeHalved, orientationState_names, false);
    bspyTaskVar_StringEnum(m_orientationStateCurrent, orientationState_names, false);

    static const char* ha_state_names[] = 
    {
#define HA_NAME_ACTION(_name) #_name ,
      HA_STATES(HA_NAME_ACTION)
#undef HA_NAME_ACTION
    };
    bspyTaskVar_StringEnum(m_handAnimationType, ha_state_names, true);

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

    float upDotUp = m_character->m_gUp.Dot(m_character->m_COMTM.b);
    bspyTaskVar(upDotUp, false);
    bspyTaskVar(m_character->m_angMom.Dot(m_character->m_COMTM.a), false);
    bspyTaskVar(m_character->m_angMom, false);
#if NM_RIGID_BODY_BULLET
    rage::Vector3 angMomStatic;
    m_character->m_characterInertiaAboutComInN.Transform3x3(m_character->m_COMrotvel, angMomStatic);
    bspyTaskVar(angMomStatic.Dot(m_character->m_COMTM.a), false);
    bspyTaskVar(angMomStatic, false);
#endif //NM_RIGID_BODY_BULLET
    bspyTaskVar(m_character->m_COMrotvel, false);
    //bspyTaskVar(m_character->m_COMrotvel_Diff, false);

    m_character->getEngineNotConst()->setbSpyObject(m_character->m_probeHitInstLevelIndex[NmRsCharacter::pi_highFall]);
  }
#endif // ART_ENABLE_BSPY

} // nms Art

