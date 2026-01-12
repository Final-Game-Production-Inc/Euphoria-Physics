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
#include "NmRsCBU_SmartFall.h" 
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
#include "NmRsCBU_Teeter.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"

#ifdef GET_TASK
#undef GET_TASK
#endif
#define GET_TASK(taskName, TaskName)\
  NmRsCBU##TaskName* taskName##Task = (NmRsCBU##TaskName*)m_cbuParent->m_tasks[bvid_##taskName];\
  Assert(taskName##Task);

namespace ART
{
#define highFall_DEVEL 1
  NmRsCBUSmartFall::NmRsCBUSmartFall(ART::MemoryManager* services) : CBUTaskBase(services, bvid_smartFall)
  {
    initialiseCustomVariables();
  }

  NmRsCBUSmartFall::~NmRsCBUSmartFall(){}

  void NmRsCBUSmartFall::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    // reset the variables
    m_timer = 0.0f;
    m_hasCollidedWithWorld = false;
    m_controlOrientation = true;
    m_willLandOnFeet = true;
    m_predImpactTime = 30.0f;
    m_forwardVelMag = 0.0f;
    m_feedbackSent = false; 
    m_goToCatchFallFlag = false;
    m_rollToBBtimer = 0.0f;
    m_averageComVel = 0.0f;
    m_leftLegLength = 0.0f;
    m_normalDotUp = 0.0f;
    m_probeHit = false;
    m_probeDownHit = false;
    m_impactVisible = false;
    m_landingNormalOK = false;
    m_dead = false;
    m_orientationState = NmRsCharacter::OS_Back;
    m_orientationStateTimeHalved= NmRsCharacter::OS_Back;
    m_orientationStateCurrent = NmRsCharacter::OS_Back;
    m_horComVel.Zero();

    m_sinkRatio = 0.0f;
    m_sinkRate = 0.0f;
    m_sinkAccel = 0.0f;
    m_height = 0.0f;
    m_slope = 0.0f;

    m_smoothCollidedWithWorld = 0.0f;
    m_supported = false;

    // reset state machine to initial state
    Reset();
  }

  void NmRsCBUSmartFall::onActivate()
  {
    Assert(m_character);
    m_cameraPos = m_character->m_COM;

    m_hulaTime = -0.1f;
    m_hulaTotalTime = 0.0f;
    m_rollToBB = false;
    m_catchFallTime = 0.0f;
    m_slowEnoughToRecover = true;

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

    Assert(m_parameters.initialState >= 0 && m_parameters.initialState <= SF_Balance);
    m_currentState = m_parameters.initialState;

    // Randomize values for arms brace
    // TODO make brace state struct do this on init
    if (m_character->getBodyIdentifier() == gtaWilma)
    {
      //hands together to little more than shoulder width
      m_leftHandSeparation = m_character->getRandom().GetRanged(0.075f, 0.23f);
      m_rightHandSeparation = m_character->getRandom().GetRanged(0.075f, 0.23f);
    }
    else //(m_character->getBodyIdentifier() == gtaFred || m_character->getBodyIdentifier() == rdrCowboy )
    {
      //little less than shoulder width to wide
      m_leftHandSeparation = m_character->getRandom().GetRanged(0.17f, 0.40f);
      m_rightHandSeparation = m_character->getRandom().GetRanged(0.17f, 0.40f);
    } 

    // state machine init: will execute first real state's enter function
    Initialize();

    // REBOUND ASSISTANCE: Init.
    //
    m_reboundVelocity = m_character->m_COMvel;
    m_reboundTime = -1.0f;
  }

  void NmRsCBUSmartFall::onDeactivate()
  {
    Assert(m_character);

    // Run the current state's exit routine.
    States( EVENT_Exit, m_currentState );

    initialiseCustomVariables();

    //Turn off not Registering certain collisions
    m_character->setDontRegisterCollsion(-1.f, -1.f); //mmmmtodo note this may reset balancerCollisionsReaction's settings

    // turn off all possible sub behaviours (new wrapper)
    // TODO may not be necessary if state exits are all correct.
    m_character->deactivateTask(bvid_bodyRollUp);
    m_character->deactivateTask(bvid_rollDownStairs);
    m_character->deactivateTask(bvid_catchFall);
    m_character->deactivateTask(bvid_bodyBalance);
    m_character->deactivateTask(bvid_balancerCollisionsReaction);
    m_character->deactivateTask(bvid_headLook);
    m_character->deactivateTask(bvid_pedalLegs);
    m_character->deactivateTask(bvid_armsWindmillAdaptive);
    m_character->deactivateTask(bvid_bodyWrithe);

    // Deactivate self-avoidance, in case we used it.
    m_character->m_selfAvoidance.useSelfAvoidance = false;

    //SmartFall is the only behaviour that sets m_character->m_groundNormal
    // so restoring it to it's old default of up is not a problem (used by keepHeadAwayFromGround in headlook)
    m_character->m_groundNormal = m_character->m_gUpReal;

#ifdef NM_RS_CBU_ASYNCH_PROBES
    m_character->ClearAsynchProbe_IfNotInUse(NmRsCharacter::pi_highFall);
#endif
  }

  // tick: not much happening here, most stuff is in the state machine update and individual state's ticks
  // ---------------------------------------------------------------------------------------------------------
  CBUTaskReturn NmRsCBUSmartFall::onTick(float UNUSED_PARAM(timeStep))
  {
    // so that BodyBalance (executed before SmartFall) can set its own upper body stiffness/damping values
    // TODO re-prioritize BodyBalance activation.
    if (! (m_currentState == SF_Balance || m_currentState == SF_CatchFall || m_currentState == SF_BailOut || m_currentState == SF_Splat) ) 
    {
      // Simple piecewise linear fn. For health > 1/k, stiffness = bodyStiffness.  For health < 1/k, stiffness tapers linearly to zero.
      const float k = 2.0f; // range: 1..whatever
      float stiffness = rage::Clamp(m_character->m_health * m_parameters.bodyStiffness * k, 5.0f, m_parameters.bodyStiffness);
      m_body->setStiffness(stiffness, m_parameters.bodydamping);
    }

    probeEnvironmentAndCalcForFall();

    calculateCommonVariables();

    orientationControllerTick();

    m_timer += m_character->getLastKnownUpdateStep();

    // update state machine
    Update();

    return eCBUTaskComplete;
  }

  // state machine states and transitions
  // ---------------------------------------------------------------------------------------------------------
  bool NmRsCBUSmartFall::States( StateMachineEvent event, int state )
  {
    BeginStateMachine

    // Initial State: Falling  //////////////////////////////////////////////////// 
    State( SF_Falling )

    OnEnter
    {
      m_controlOrientation = true;

      // REBOUND ASSISTANCE: Reset.
      //
      m_reboundTime = -1.0;
      m_reboundVelocity.Zero();

      duringFall();
    }

    OnUpdate
    {
      SCOPED_BODY_CONTEXT(SF_Falling, 0)

      duringFall();

      doBrace();

      // Available transitions from this state, in this order:

      // SF_Splat
      if(m_toExitFall && m_dead)
      {
        SetState(SF_Splat);
        return true;
      }

      // SF_NonRecoverableFall
      {
        if(m_aboutToLand && !m_slowEnoughToRecover)
        {
          SetState(SF_NonRecoverableFall);
          return true;
        }
      }

      // SF_CatchFall
      {
        if(m_toExitFall && m_impactVisible && (m_impactDown || (m_impactFront && m_impactPredictedFront)))
        {
          SetState(SF_CatchFall);
          return true;
        }
      }

      // SF_BailOut
      {
        if(m_toExitFall && (!m_impactVisible || (m_impactFront && !m_impactPredictedFront)))
        {
          SetState(SF_BailOut);
          return true;
        }
      }

      // SF_PrepareForLanding
      {
        if(m_aboutToLand && m_landingNormalOK)
        {
          SetState(SF_PrepareForLanding);
          return true;
        }
      }

      // SF_Glide
      {
        if(!m_fallingDown)
        {
          SetState(SF_Glide);
          return true;
        }
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
    }

    // Glide  ////////////////////////////////////////////////////
    State( SF_Glide )

    OnEnter
    {
      m_controlOrientation = false;

      duringGlide();
    }

    OnUpdate
    {
      SCOPED_BODY_CONTEXT(SF_Glide, 0)

      duringGlide();

      doBrace();

      // Available transitions from this state, in this order:

      // SF_Splat
      if(m_toExitFall && m_dead)
      {
        SetState(SF_Splat);
        return true;
      }

      // SF_NonRecoverableFall
      {
        if(m_aboutToLand && !m_slowEnoughToRecover)
        {
          SetState(SF_NonRecoverableFall);
          return true;
        }
      }

      // SF_CatchFall
      {
        if(m_toExitFall && !m_dead && m_impactVisible && (m_impactDown || (m_impactFront && m_impactPredictedFront)))
        {
          SetState(SF_CatchFall);
          return true;
        }
      }

      // SF_BailOut
      {
        if(m_toExitFall && (!m_impactVisible || (m_impactFront && !m_impactPredictedFront)))
        {
          SetState(SF_BailOut);
          return true;
        }
      }

      // SF_PrepareForLanding
      {
        if(m_aboutToLand && !m_dead && m_landingNormalOK)
        {
          SetState(SF_PrepareForLanding);
          return true;
        }
      }

      // SF_Falling
      {
        if(m_fallingDown)
        {
          SetState(SF_Falling);
          return true;
        }
      }
    }

    OnExit
    {
      m_character->deactivateTask(bvid_bodyWrithe);
      m_character->deactivateTask(bvid_bodyFoetal);
    }

    // Bail Out ////////////////////////////////////////////////////
    State( SF_BailOut )
    OnEnter
    {
      SCOPED_BODY_CONTEXT(SF_BailOut, 0)

      m_hasCollidedWithWorld = false;

      //Stop Bounce
      m_character->setFrictionPreScale(0.0f, bvmask_FootLeft | bvmask_FootRight);

      duringBailOut();

    }

    OnUpdate
    {
      SCOPED_BODY_CONTEXT(SF_BailOut, 0)

      duringBailOut();

      doBrace();

      // Available transitions from this state, in this order:

      // SF_Balance
      if(m_hasCollidedWithWorld && m_willLandOnFeet)
      {
        SetState(SF_Balance);
        return true;
      }

      // SF_CatchFall
      {
        if (m_hasCollidedWithWorld || m_impactPredictedFront)
        {
          SetState(SF_CatchFall);
          return true;
        }
      }

      // SF_NonRecoverableFall
      if(!m_slowEnoughToRecover)
      {
        SetState(SF_NonRecoverableFall);
        return true;
      }

      // SF_Falling
      {
        bool impactSoon = (m_predImpactTime < 0.4f);
        bool highEnough = (m_height > 1.5f);
        if(!impactSoon && !m_hasCollidedWithWorld && highEnough && m_fallingDown)
        {
          SetState(SF_Falling);
          return true;
        }
      }

      // SF_Glide
      {
        bool impactSoon = (m_predImpactTime < 0.4f);
        bool highEnough = (m_height > 1.5f);
        bool fallingDown = (m_sinkRatio > 0.3f);
        if(!impactSoon && !m_hasCollidedWithWorld && highEnough && !fallingDown)
        {
          SetState(SF_Glide);
          return true;
        }
      }
    }
    OnExit
    {
      m_character->setFrictionPreScale(1.0f, bvmask_FootLeft | bvmask_FootRight);

      m_character->deactivateTask(bvid_bodyFoetal);
      m_character->deactivateTask(bvid_pedalLegs);
    }

    // Prepare For Landing ////////////////////////////////////////////////////
    State( SF_PrepareForLanding )

    OnEnter
    {
      duringPrepareForLanding();
    }

    OnUpdate
    {
      SCOPED_BODY_CONTEXT(SF_PrepareForLanding, 0)

      duringPrepareForLanding();

      // Available transitions from this state, in this order:

      // SF_Falling
      {
        if(m_restartFall && m_fallingDown)
        {
          SetState(SF_Falling);
          return true;
        }
      }

      // SF_Glide
      {
        if(m_restartFall && !m_fallingDown)
        {
          SetState(SF_Glide);
          return true;
        }
      }

      // SF_NonRecoverableFall
      if(!m_slowEnoughToRecover)
      {
        SetState(SF_NonRecoverableFall);
        return true;
      }

      // SF_Splat
      if(m_hasCollidedWithWorld && m_dead)
      {
        SetState(SF_Splat);
        return true;
      }

      // SF_ForwardRoll
      if(m_hasCollidedWithWorld && m_parameters.forwardRoll)
      {
        SetState(SF_ForwardRoll);
        return true;
      }

      // SF_Balance
      if(m_hasCollidedWithWorld && m_willLandOnFeet)
      {
        SetState(SF_Balance);
        return true;
      }

      // SF_CatchFall
      if(m_hasCollidedWithWorld && !m_willLandOnFeet)
      {
        SetState(SF_CatchFall);
        return true;
      }
    }
    OnExit
    {
      m_character->deactivateTask(bvid_pedalLegs);
      m_character->deactivateTask(bvid_catchFall);
    }

    // Non-recoverable fall ////////////////////////////////////////////////////
    State( SF_NonRecoverableFall )
    OnEnter
    {
      m_character->deactivateTask(bvid_pedalLegs);

      duringNonRecoverableFall();
    }

    OnUpdate
    {
      SCOPED_BODY_CONTEXT(SF_NonRecoverableFall, 0)

      duringNonRecoverableFall();

      // Available transitions from this state, in this order:

      // SF_Falling
      {
        if(m_restartFall && m_fallingDown)
        {
          SetState(SF_Falling);
          return true;
        }
      }

      // SF_Glide
      {
        if(m_restartFall && !m_fallingDown)
        {
          SetState(SF_Glide);
          return true;
        }
      }

      // SF_Splat
      if(m_hasCollidedWithWorld && (m_dead || !m_slowEnoughToRecover))
      {
        SetState(SF_Splat);
        return true;
      }

      // SF_Balance
      if(m_hasCollidedWithWorld && m_willLandOnFeet)
      {
        SetState(SF_Balance);
        return true;
      }

      // SF_CatchFall
      if(m_hasCollidedWithWorld && !m_willLandOnFeet)
      {
        SetState(SF_CatchFall);
        return true;
      }

    }

    OnExit
    {
      m_character->deactivateTask(bvid_bodyWrithe);
      m_character->deactivateTask(bvid_bodyFoetal);
    }

    // Forward Roll ////////////////////////////////////////////////////
    State( SF_ForwardRoll )
    
    OnUpdate
    {
      SCOPED_BODY_CONTEXT(SF_ForwardRoll, 0)

      duringForwardRoll();

      // Available transitions from this state, in this order:

      // SF_Balance
      // Appears m_rollToBB is never true.
      if (m_rollToBB && (m_rollToBBtimer > 0.1f) )
      {
        SetState( SF_Balance );
        return true;
      }

      // SF_CatchFall
      if (m_feedbackSent)
      {
        SetState( SF_CatchFall );
        return true;
      }

      // SF_Falling
      {
        if(m_restartFall && m_fallingDown)
        {
          SetState(SF_Falling);
          return true;
        }
      }

      // SF_Glide
      {
        if(m_restartFall && !m_fallingDown)
        {
          SetState(SF_Glide);
          return true;
        }
      }
    }

    OnExit
    {
      m_character->deactivateTask(bvid_rollDownStairs);
    }
    // Catch Fall ////////////////////////////////////////////////////
    State( SF_CatchFall )

    OnEnter
    {
      SCOPED_BODY_CONTEXT(SF_CatchFall, 0)

      m_catchFallTime = 0.0f;

      m_character->deactivateTask(bvid_bodyWrithe);
      m_character->deactivateTask(bvid_bodyFoetal);
      m_character->deactivateTask(bvid_pedalLegs);

      // Minimize initial bounce for car bail-out.
      if(m_parameters.rdsUseStartingFriction)
      {
        BehaviourMask handsAndFeet = bvmask_HandLeft | bvmask_HandRight | bvmask_FootLeft | bvmask_FootRight;
        BehaviourMask neck = bvmask_Head | bvmask_Neck;
        m_character->setFrictionPreScale(m_parameters.rdsStartingFriction, bvmask_Full & ~(handsAndFeet | neck));
        if(m_parameters.changeExtremityFriction)
        {
          m_character->setFrictionPreScale(m_parameters.rdsStartingFriction, neck);
          m_character->setFrictionPreScale(m_parameters.rdsStartingFriction / 3.0f, handsAndFeet);
        }

        if(m_parameters.rdsStartingFrictionMin > 0.0f)
        {
          m_character->m_minImpactFriction = m_parameters.rdsStartingFrictionMin;
          m_character->m_applyMinMaxFriction = true;
        }
        else
        {
          m_character->m_applyMinMaxFriction = false;
        }
      }

      NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
      Assert(catchFallTask);
      catchFallTask->m_allowFrictionChangeToExtremities = m_parameters.changeExtremityFriction;

      // Set target velocity from current velocity (doesn't work yet - has dropped too much by the time we arrive here).
      NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
      Assert(rdsTask);
      if(m_parameters.rdsTargetLinearVelocity < 0.0f)
        rdsTask->m_parameters.m_targetLinearVelocity = m_character->m_COMvelMag;
    }

    OnUpdate
    {
      SCOPED_BODY_CONTEXT(SF_CatchFall, 0)

      m_catchFallTime += m_character->getLastKnownUpdateStep();

      duringCatchFall();

      // Available transitions from this state, in this order:

      // SF_Falling
      {
        if(m_restartFall && m_fallingDown)
        {
          SetState(SF_Falling);
          return true;
        }
      }

      // SF_Glide
      {
        if(m_restartFall && !m_fallingDown)
        {
          SetState(SF_Glide);
          return true;
        }
      }

      // SF_Splat
      // 
      // Disabled SF_Splat transition on death when on a steep slope.
      //   
      if(((m_parameters.splatWhenStopped > 0.0f) && (m_character->m_COMvelMag < m_parameters.splatWhenStopped)) || m_dead || (m_hasCollidedWithWorld && !m_slowEnoughToRecover))
      {
        SetState(SF_Splat);
        return true;
      }
    }

    OnExit
    {
      m_catchFallTime = 0.0f;

      m_character->m_applyMinMaxFriction = false;

      m_character->deactivateTask(bvid_catchFall);
    }

    // Balance ////////////////////////////////////////////////////
    State( SF_Balance )

    OnUpdate
    {
      SCOPED_BODY_CONTEXT(SF_Balance, 0)

      duringBalance();

      doBrace();

      // Available transitions from this state, in this order:

      // SF_Falling
      {
        if((m_restartFall || m_teeterFailed) && m_fallingDown)
        {
          SetState(SF_Falling);
          return true;
        }
      }

      // SF_Glide
      {
        if((m_restartFall || m_teeterFailed) && !m_fallingDown)
        {
          SetState(SF_Glide);
          return true;
        }
      }

      if(m_balanceFailed && (m_dead || !m_slowEnoughToRecover))
      {
        SetState(SF_Splat);
        return true;
      }

      // SF_CatchFall
      {
        if(m_balanceFailed)
        {
          SetState(SF_CatchFall);
          return true;
        }
      }
    }

    OnExit
    {
      m_character->deactivateTask(bvid_teeter);
      m_character->deactivateTask(bvid_bodyBalance);
      m_character->deactivateTask(bvid_balancerCollisionsReaction);
    }

    // Splat ////////////////////////////////////////////////////
    State( SF_Splat )

    OnEnter
    {
      SCOPED_BODY_CONTEXT(SF_Splat, 0) 

      duringSplat();
    }

    OnUpdate
    {
      SCOPED_BODY_CONTEXT(SF_Splat, 0)

      duringSplat();

      // Available transitions from this state, in this order:

      // SF_Falling
      {
        if((m_restartFall || m_teeterFailed) && m_fallingDown)
        {
          SetState(SF_Falling);
          return true;
        }
      }

      // SF_Glide
      {
        if((m_restartFall || m_teeterFailed) && !m_fallingDown)
        {
          SetState(SF_Glide);
          return true;
        }
      }
    }

    OnExit
    {
    }

    EndStateMachine
  }

  void NmRsCBUSmartFall::sendFeedback(bool success /* = false */)
  {
    if(m_feedbackSent)
      return;

    m_feedbackSent = true;
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      feedback->m_agentID = m_character->getID();
      feedback->m_argsCount = 0;
      strcpy(feedback->m_behaviourName, NMSmartFallFeedbackName);
      if(success)
        feedback->onBehaviourSuccess();
      else
        feedback->onBehaviourFailure();
    }
  }

  // Individual state DURING...

  // Blend bodyFoetal with bodyWrithe.
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUSmartFall::duringNonRecoverableFall()
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
  void NmRsCBUSmartFall::duringBailOut()
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
    if(m_character->m_COMvelRelativeMag < 0.05f)
      sendFeedback();
  }

  // get feet in position etc..
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUSmartFall::duringPrepareForLanding()
  {
    // The state we want to go into for the landing; Arms up, "prepare for landing" pose or catchFall arms,
    // head forward, knees slightly bent, legs slightly forward and to the side.

    //////////////////////////////////////////////////////////////////////////
    //UPPERBODY AND ARMS REACTION

    // Make a "prepare for landing" pose.
    if (m_parameters.armsUp <= -3.0f && m_orientationState == NmRsCharacter::OS_Up)
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
    else if (m_parameters.armsUp <= -2.0f)
    {
      NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
      Assert(catchFallTask);
      if (!catchFallTask->isActive())
      {
        catchFallTask->updateBehaviourMessage(NULL); // set parameters to defaults
        catchFallTask->m_parameters.m_legsStiffness = 0.6f*m_parameters.bodyStiffness;
        catchFallTask->m_parameters.m_torsoStiffness = 0.85f*m_parameters.bodyStiffness;
        catchFallTask->m_parameters.m_armsStiffness = 1.45f*m_parameters.bodyStiffness;
        catchFallTask->m_parameters.m_effectorMask = bvmask_UpperBody;
        catchFallTask->activate();
      }
    }
    else //< Ik arms based on m_parameters.armsUp.
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
      ikPos.AddScaled(getSpine()->getHeadPart()->getPosition(),gUp,m_parameters.armsUp);     
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
    //  Circles centres are offset to the right by the value of m_parameters.sideD
    //  Circles centres are offset forwards by the value of m_parameters.forwardOffsetOfLegIK
    NmRsCBUPedal* pedalTask = (NmRsCBUPedal*)m_cbuParent->m_tasks[bvid_pedalLegs];
    Assert(pedalTask);
    if (! pedalTask->isActive() )
    {
      pedalTask->updateBehaviourMessage(NULL); // reset params
      pedalTask->m_parameters.randomSeed=(100);
      pedalTask->m_parameters.pedalLeftLeg=(true);
      pedalTask->m_parameters.pedalRightLeg=(true);
      pedalTask->m_parameters.radius=(m_parameters.legRadius);
      pedalTask->m_parameters.angularSpeed=(m_parameters.legAngSpeed);
      pedalTask->m_parameters.backPedal=(false);
      pedalTask->m_parameters.legStiffness=(m_parameters.bodyStiffness);
      pedalTask->m_parameters.adaptivePedal4Dragging=(false);
      pedalTask->m_parameters.speedAsymmetry=(4.0f);
      pedalTask->m_parameters.pedalOffset=(0.0f);
      pedalTask->m_parameters.centreForwards=m_parameters.forwardOffsetOfLegIK;
      pedalTask->m_parameters.centreSideways = m_parameters.sideD;//0.2f;
      pedalTask->m_parameters.centreUp= 0.0f;
      pedalTask->m_parameters.ellipse = -0.1f;
      pedalTask->m_parameters.radiusVariance = 0.1f;
      pedalTask->activate();
    }
  }


  // one option if we can't balance....
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUSmartFall::duringForwardRoll()
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
    float stiffness = rage::Clamp(m_parameters.bodyStiffness - m_rollToBBtimer*12.0f,8.0f,12.0f);

    m_body->setStiffness(stiffness, m_parameters.bodydamping);

    //-- need to push legs based on orientation
    rage::Vector3 theUpVec = m_character->m_gUp;
    rage::Matrix34 chCOMMAT = m_character->m_COMTM;
    rage::Vector3 characterUp = chCOMMAT.b;
    //-- should really project into the up/forward plane !!!!!!

    float upDotG = theUpVec.Dot(characterUp);

    getLeftLegInputData()->getKnee()->setDesiredAngle(upDotG-1.75f);
    getRightLegInputData()->getKnee()->setDesiredAngle(upDotG-1.65f);

    //-- also need to blend with the forward roll..

    if (m_parameters.useZeroPose_withForwardRoll)
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
    if ((m_rollToBBtimer > 1.6f))
      sendFeedback();
  }

  void NmRsCBUSmartFall::duringSplat()
  {
    NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
    Assert(rollUpTask);

    // When moving fast, Roll Up.
    const float splatThresholdSpeed = 5.0f;
    if(m_character->m_COMvelMag > splatThresholdSpeed)
    {
      if(!rollUpTask->isActive())
      {
        rollUpTask->updateBehaviourMessage(NULL); // sets to defaults.
        rollUpTask->activate();
      }
    }
    else
    {
      if(rollUpTask->isActive())
      {
        rollUpTask->deactivate();
      }

      // Poached from Catch Fall slope work...

      // slopeAlignment - Detect sloped ground and the body's alignment relative to the slope -1..1 (
      // When positive, the left side is down-slope.
      //
      rage::Vector3 bodyUp = m_character->m_COMTM.b;
      rage::Vector3 slopeAxis;
      slopeAxis.Cross(m_character->m_groundNormal, m_character->m_gUp);
      slopeAxis.Normalize();
      float slopeAlignment = slopeAxis.Dot(bodyUp);

      // faceUpness
      //
      float faceUpness = -m_character->m_COMTM.c.Dot(m_character->m_groundNormal);

      // fallSpeed
      const float maxSpeed = 3.0f;
      const float fallSpeed = rage::Min(m_character->m_COMvelMag, maxSpeed) / maxSpeed;

      // combinedSlopeResponse
      //
      const float combinedSlopeResponse = (1.0f - fallSpeed) * m_slope * rage::Abs(slopeAlignment);

  #if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "SmartFall-Splat", combinedSlopeResponse);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall-Splat", fallSpeed);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall-Splat", slopeAxis);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall-Splat", slopeAlignment);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall-Splat", faceUpness);
  #endif

      // Ensure soft limits are not too severe (otherwise limbs snap to bent
      // when ground collision can cause abrupt changes in rotational velocity)
      const float minLimitAngle = 0.4f;
      for(int i = 0; i < (kNumNmRsHumanLimbs-1); ++i)
      {
        SoftLimitController *softLimitCtrl = &(m_character->m_softLimitCtrls[i]);
        Assert(softLimitCtrl);
        softLimitCtrl->setLimitAngle(rage::Min(softLimitCtrl->getLimit(), minLimitAngle));
      }

      //
      m_character->deactivateTask(bvid_bodyWrithe);
      m_character->deactivateTask(bvid_bodyFoetal);
      m_character->deactivateTask(bvid_pedalLegs);
      m_character->deactivateTask(bvid_catchFall);

      // Unwind
      m_body->resetEffectors(kResetAngles);

      // Bend me some elbows
      //
      getLeftArmInputData()->getElbow()->setDesiredAngle(1.0f);
      getRightArmInputData()->getElbow()->setDesiredAngle(1.0f);

      // Spread legs to stop rolling.
      //
      {
        NmRsBodyStateHelper helper(m_body, m_bvid, m_priority, 1, combinedSlopeResponse, bvmask_Full DEBUG_LIMBS_PARAMETER("SplatSlope"));
        getLeftLegInputData()->getHip()->setDesiredLean2(-0.7f);
        getRightLegInputData()->getHip()->setDesiredLean2(-0.7f);
      }

      // CatchFall normally handles setting friction values based on 
      // setFallingReaction.  If CF hasn't timed-out before we get here,
      // we need to make sure friction is switched correctly.
      //
      // Code comes from NmRsCBU_Catchfall.cpp tick around line 317. The
      // friction management should probably be moved out into the character
      // once the current project is complete.
      //
      NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
      Assert(catchFallTask);
      //
      // Allow hand and foot friction to be uniform (material value is 3x higher than other parts) when rolling or moving fast.
      BehaviourMask handsAndFeet = bvmask_HandLeft | bvmask_HandRight | bvmask_FootLeft | bvmask_FootRight;   
      m_character->setFrictionPreScale(catchFallTask->m_groundFriction, bvmask_Full);
      if(m_parameters.changeExtremityFriction)
        m_character->setFrictionPreScale(catchFallTask->m_groundFriction / 3.0f, handsAndFeet);
      m_character->m_minImpactFriction = catchFallTask->m_groundFrictionMin;
      m_character->m_applyMinMaxFriction = true;

      // BODY RELAX equivalent.
      const float mult = 0.6f;
      float multDamping = 0.3f;
      m_body->setRelaxation(mult, bvmask_Full, &multDamping);

      // Keep down-slope shoulders strong enough to resist rolling and raise down-slope arm.
      //
      const float stiffnessMin = 6.0f;
      const float damping = 0.5f;
      float downslopeStiffness = stiffnessMin + m_parameters.bodyStiffness * combinedSlopeResponse;
      BehaviourMask downslopeMask = bvmask_None;
      if(slopeAlignment * faceUpness < 0.0f)
      {
        downslopeMask = bvmask_UpperArmLeft | bvmask_ClavicleLeft | bvmask_ThighLeft;
        NmRsBodyStateHelper helper(m_body, m_bvid, m_priority, 1, m_slope * rage::Abs(slopeAlignment), downslopeMask DEBUG_LIMBS_PARAMETER("SplatSlope"));
        getLeftArmInputData()->getClavicle()->setDesiredLean2(-0.2f);
        getLeftArmInputData()->getShoulder()->setDesiredLean2(-0.5f);
        getLeftArmInputData()->getElbow()->setDesiredAngle(0.0f);
      }
      else
      {
        downslopeMask = bvmask_UpperArmRight | bvmask_ClavicleRight | bvmask_ThighRight;
        NmRsBodyStateHelper helper(m_body, m_bvid, m_priority, 1, m_slope * rage::Abs(slopeAlignment), downslopeMask DEBUG_LIMBS_PARAMETER("SplatSlope"));
        getRightArmInputData()->getClavicle()->setDesiredLean2(-0.2f);
        getRightArmInputData()->getShoulder()->setDesiredLean2(-0.5f);
        getRightArmInputData()->getElbow()->setDesiredAngle(0.0f);
      }
      m_body->setStiffness(downslopeStiffness, damping, downslopeMask);

  #if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "SmartFall-Splat", downslopeStiffness);
  #endif
    }
  }

  // another option if we can't balance

  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUSmartFall::duringCatchFall()
  {
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)m_cbuParent->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    if (!catchFallTask->isActive())
    {
      catchFallTask->updateBehaviourMessage(NULL); // set parameters to defaults
      catchFallTask->m_parameters.m_legsStiffness = 0.85f*m_parameters.bodyStiffness;
      catchFallTask->m_parameters.m_torsoStiffness = 0.85f*m_parameters.bodyStiffness;
      catchFallTask->m_parameters.m_armsStiffness = 1.45f*m_parameters.bodyStiffness;
      catchFallTask->m_parameters.extraSit = 0.0f;
      catchFallTask->m_parameters.m_zAxisSpinReduction = m_parameters.cfZAxisSpinReduction;
      if(m_parameters.forceHeadAvoid)
      {
        catchFallTask->setHeadAvoidActive(true);
      }
      catchFallTask->activate();
    }

    // Reduce hand friction for the first part of Catch Fall.  Does not affect
    // roll portion of behaviour and is limted to the period of time from first
    // ground contact to when the inhibit rolling timer expires.
    //
    if(catchFallTask->isActive() && catchFallTask->isOnGround() && catchFallTask->m_inhibitRollingTime > 0.0f)
    {
      BehaviourMask hands = bvmask_HandLeft | bvmask_HandRight;
      m_character->setFrictionPreScale(0.05f, hands);
    }

    // Blend to neutral head pose as we come to a stop. 
    //
    if(m_parameters.blendHeadWhenStopped > 0.0f)
    {
      const float velRange = m_parameters.blendHeadWhenStopped;
      float blend = 1.0f - (rage::Min(m_character->m_COMvelMag, velRange)/velRange);
      NmRsHumanBody *body = m_character->getBody();
      Assert(body);
      NmRsBodyStateHelper helper(body, m_bvid, m_priority, 1, blend, bvmask_CervicalSpine DEBUG_LIMBS_PARAMETER("Neutral Head"));
      body->getSpineInputData()->getLowerNeck()->setDesiredAngles(0.0f, 0.0f, 0.0f);
      body->getSpineInputData()->getUpperNeck()->setDesiredAngles(0.0f, 0.0f, 0.0f);
  #if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "SmartFall-Head", blend);
  #endif
    }

    if(m_character->m_health < 0.5f)
    {
      catchFallTask->m_handsAndKnees = false;
    }

    if (!m_parameters.balance) //less stiff H+K catchfall?
    {
      float stiff = 0.5f;
 
      m_body->setStiffness(6.f, 1.f, bvmask_Full, &stiff);

      ////stop bounce if high
      //m_character->setBodyStiffness(4.f,2.5f,"fb",&stiff);
      //m_character->setElasticityMultiplier(0.0f, bvmask_Full);
    }

    // Tweak some RDS parameters.
    //
    NmRsCBURollDownStairs* rdsTask = (NmRsCBURollDownStairs*)m_cbuParent->m_tasks[bvid_rollDownStairs];
    Assert(rdsTask);
    rdsTask->m_parameters.m_Stiffness = m_parameters.bodyStiffness;
    rdsTask->m_parameters.limitSpinReduction = true;
    if(m_character->m_COMvelMag < m_parameters.rdsForceVelThreshold)
    {
      rdsTask->m_parameters.m_ForceMag = m_parameters.rdsForceMag;
    }
    else
    {
      rdsTask->m_parameters.m_ForceMag = 0.0f;
    }
    rdsTask->m_parameters.m_targetLinearVelocityDecayTime = m_parameters.rdsTargetLinearVelocityDecayTime;
    if(m_parameters.rdsTargetLinearVelocity >= 0.0f)
    {
      rdsTask->m_parameters.m_targetLinearVelocity = m_parameters.rdsTargetLinearVelocity;
    }
    rdsTask->m_parameters.m_SpinWhenInAir = false;

    // Tweak some Roll Up parameters.
    //
    NmRsCBURollUp* rollUpTask = (NmRsCBURollUp*)m_cbuParent->m_tasks[bvid_bodyRollUp];
    Assert(rollUpTask);
    rollUpTask->m_parameters.velocityScale = 0.1f;
    rollUpTask->m_parameters.velocityOffset = 0.0f;

#define RDS_INCREASE_STIFFNESS 0
#if RDS_INCREASE_STIFFNESS
    // JRP new...
    rdsTask->m_parameters.m_Stiffness = 11.0f;
    rdsTask->m_parameters.m_StiffnessDecayTime = m_parameters.rdsTargetLinearVelocityDecayTime;
    rdsTask->m_parameters.m_StiffnessDecayTarget = 7.0f;
#endif

    // if we've been rolling a while, actively try to slow down.
    if(m_catchFallTime > m_parameters.stopRollingTime)
    {
      rdsTask->m_parameters.m_UseArmsToSlowDown = 1.0f;
      rdsTask->m_parameters.m_ForceMag = 0.0f;
      catchFallTask->m_resistRolling = true;
      catchFallTask->m_stopOnSlopes = true;
    }

    // if we've come to rest, fin
    if(m_character->m_COMvelRelativeMag < 0.075f)
      sendFeedback();
  }

  // during balance
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUSmartFall::duringBalance()
  {
    NmRsCBUBodyBalance* bodyBalanceTask = (NmRsCBUBodyBalance*)m_cbuParent->m_tasks[bvid_bodyBalance];
    Assert(bodyBalanceTask);
    NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    NmRsCBUTeeter* teeterTask = (NmRsCBUTeeter*)m_cbuParent->m_tasks[bvid_teeter];
    Assert(teeterTask);
    NmRsCBUBalancerCollisionsReaction* bcrTask = (NmRsCBUBalancerCollisionsReaction*)m_cbuParent->m_tasks[bvid_balancerCollisionsReaction];
    Assert(bcrTask);

    if (m_parameters.teeter && (m_parameters.initialState == SF_Balance) && !teeterTask->isActive())
    {
      // TEETER
      //
      rage::Vector3 fwd = m_character->m_COMvel;
      fwd.Normalize();
      m_character->levelVector(fwd);

      rage::Vector3 ctr;
      if(m_probeDownHit)
      {
        // Start from the down probe hit.
        ctr = m_character->m_probeHitPos[NmRsCharacter::pi_highFallDown];
      }
      else
      {
        // Start from the COM position leveled to the average foot height.
        ctr = m_character->m_COM;
        ctr.z = m_body->getLeftLeg()->getFoot()->getPosition().z;
        ctr.z += m_body->getRightLeg()->getFoot()->getPosition().z;
        ctr.z *= 0.5f;
      }

      rage::Vector3 lft;
      lft.Cross(fwd, -m_character->m_gUp);
      lft.Normalize();

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "SmartFall-Teeter", ctr);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall-Teeter", fwd);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall-Teeter", lft);
#endif

      teeterTask->m_parameters.edgeLeft = ctr + m_parameters.teeterOffset * fwd + lft;
      teeterTask->m_parameters.edgeRight = ctr + m_parameters.teeterOffset * fwd - lft;

      teeterTask->m_parameters.useExclusionZone = true;
      teeterTask->m_parameters.preTeeterTime = 1.0f;
      teeterTask->m_parameters.leanAwayTime = 1.0f;
      teeterTask->m_parameters.leanAwayScale = 0.5f;
      teeterTask->m_parameters.teeterTime = 0.5f;

      teeterTask->m_parameters.callHighFall = false;

      teeterTask->activate();
    }
    else
    {
      // No stay upright unless we're teetering
      m_character->m_uprightConstraint.forceActive = false;
      m_character->m_uprightConstraint.torqueActive = false;

      // Mild lean in direction of travel.
      rage::Vector3 dir = m_horComVel;
      dir.Normalize();
      const float leanAmount = 0.25f;
      dynamicBalancerTask->autoLeanInDirection(dir, leanAmount);
    }

    if(teeterTask->isActive() && teeterTask->hasFallen())
    {
      m_teeterFailed = true;
    }

    if (!bodyBalanceTask->isActive())
    {
      const float scale = rage::Max(0.7f, m_character->m_health);

      dynamicBalancerTask->taperKneeStrength(false);
      //tune params to body balance
      bodyBalanceTask->updateBehaviourMessage(NULL);// sets values to defaults
      bodyBalanceTask->m_parameters.m_useHeadLook =  true;
      bodyBalanceTask->m_parameters.m_headLookPos.x = 0.0f;
      bodyBalanceTask->m_parameters.m_headLookPos.y = 4.0f;
      bodyBalanceTask->m_parameters.m_headLookPos.z = 0.0f;

      bodyBalanceTask->m_parameters.m_headLookInstanceIndex = m_character->getFirstInstance()->GetLevelIndex();

      bodyBalanceTask->m_parameters.m_spineStiffness = scale * 10.0f;
      bodyBalanceTask->m_parameters.m_spineDamping = 1.0f;

      bodyBalanceTask->m_parameters.m_useBodyTurn = m_character->getRandom().GetBool();

      bodyBalanceTask->m_parameters.m_somersaultAngleThreshold = 0.25f;//Amount of somersault 'angle' before bodyBalanceTask->m_parameters.m_somersaultAngle is used for ArmsOut
      bodyBalanceTask->m_parameters.m_sideSomersaultAngleThreshold = 0.25f;
      bodyBalanceTask->m_parameters.m_somersaultAngVelThreshold = 1.2f;
      bodyBalanceTask->m_parameters.m_twistAngVelThreshold = 3.0f;
      bodyBalanceTask->m_parameters.m_sideSomersaultAngVelThreshold = 1.2f;

      bodyBalanceTask->m_parameters.m_armStiffness = m_parameters.bodyStiffness*(9.0f/11.0f);
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
      //  bodyBalanceTask->setBodyStiffness(m_parameters.bodyStiffness*(9.0f/11.0f));
      //  dynamicBalancerTask->setKneeStrength(0.4f);
      m_rollToBBtimer = 0.0f;
    }

    // Ensure dead/exhausted characters fall down.
    // TODO keep an eye on this. won't work as desired if the balancer is already running...
    const float balanceTime = 1.0f + 2.0f * m_character->m_health;
    dynamicBalancerTask->setMaximumBalanceTime(balanceTime);

    if(m_character->m_health < 0.3f)
    {
      dynamicBalancerTask->taperKneeStrength(true);
    }

    // Balancer COLLISION REACTION
    if(!bcrTask->isActive())
    {
      bcrTask->updateBehaviourMessage(NULL);
      bcrTask->activate();
    }

    m_rollToBBtimer += m_character->getLastKnownUpdateStep();

    // This is the only place muscle stiffness is set. Will leave these values in place after state is exited.
    //
    // TODO 
    //
    if (!m_parameters.balance) //collapse the bodyBalance
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

    }

    m_balanceFailed = dynamicBalancerTask->m_failType != dynamicBalancerTask->balOK;

    // send feeback
    if(m_character->m_COMvelRelativeMag < 0.15f)
      sendFeedback(m_balanceFailed);
  }


  // windmill, pedalling etc...
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUSmartFall::duringFall()
  {
    // Reset feedback flag.
    m_feedbackSent = false;

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
      pedalTask->m_parameters.radius = m_parameters.legRadius;
      pedalTask->m_parameters.legStiffness = m_parameters.bodyStiffness;
      pedalTask->m_parameters.adaptivePedal4Dragging = false;
      pedalTask->m_parameters.pedalOffset = 0.0f;
      pedalTask->m_parameters.backPedal = false;//mmmmSET every tick
      pedalTask->m_parameters.centreForwards = 0.0f;
      pedalTask->activate();
    }

    float armAngle = -m_parameters.arms2LegsPhase;//0.0 running, 1.5 opposite, 3.14159 in phase looks bad, 4.65 in phase hands go down as knees go down looks ok.
    if (! armsWindmillAdaptiveTask->isActive() )
    {
      armsWindmillAdaptiveTask->updateBehaviourMessage(NULL); // initialise parameters.
      armsWindmillAdaptiveTask->m_parameters.armDirection = 1;
      armsWindmillAdaptiveTask->activate();
      //synchronize the arms to the legs
      if (m_parameters.arms2LegsSync == 2)
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
    if (m_parameters.adaptiveCircling)
    {
      rage::Vector3 bodyRight = tmCom.a;

      float somersaultAngMom = bodyRight.Dot(m_character->m_angMom);//+ve is forward

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "SmartFall - 1", somersaultAngMom);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall - 1", m_character->m_angMom);
#endif
      float qSom;
      float arcsin = bodyBack.z;
      qSom = rage::AsinfSafe(arcsin);//gives forward horizontal = PI/2, upright = 0, lying on back = -PI/2
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "SmartFall - 1", qSom);
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
      bspyScratchpad(m_character->getBSpyID(), "SmartFall - 2", somersaultAngVel);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall - 2", somersaultAngMom);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall - 2", qSom);
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
    }//if (m_parameters.adaptiveCircling)

    //Hula reaction if can't see floor and not rotating fast.
    //  A horizontal (to the character) circle is followed by both legs
    // (so that if standing up the character would do a hula hoop movement at the hips)
    //  This is the reaction used by gymnasts and cats to twist the body while airborne
    //  especially when no somersault angular momentum is present.
    if (m_parameters.hula)
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
    {
      m_hulaTime = -0.1f;
    }

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
      pedalTask->m_parameters.radius = m_parameters.legRadius;
      pedalTask->m_parameters.angularSpeed = m_parameters.legAngSpeed;
      pedalTask->m_parameters.centreUp=-0.1f;
      pedalTask->m_parameters.radiusVariance = 0.4f;//default
      pedalTask->m_parameters.legAngleVariance = 0.5f;//default
      pedalTask->m_parameters.speedAsymmetry = m_parameters.legAsymmetry;
    }
    pedalTask->m_parameters.spread = m_parameters.spreadLegs;

    //UPDATE: ArmsWindmillAdaptive
    //synchronize the arms to the legs (so that pedalTask->m_parameters.speedAsymmetry can be non zero and the arms/legs will still synch update every step)
    //We synch the arms to the legs because:
    //  Pedal has a de-synching parameter speedAsymmetry and armswindmill doesn't.
    //  Setting the leg angle destroys any de-synching of the leg circles.
    if (m_parameters.arms2LegsSync == 1)
    {
      if (pedalTask->isActive())//should be
        armAngle += pedalTask->getAngle();
      armsWindmillAdaptiveTask->setAngle(armAngle);
    }
    // update the armsWindMillTask
    // parameters updated here as teeter is using armswindmill and needs to be overidden by SmartFall 
    armsWindmillAdaptiveTask->m_parameters.bodyStiffness = m_parameters.bodyStiffness;
    armsWindmillAdaptiveTask->m_parameters.armStiffness = m_parameters.bodyStiffness*14.0f/11.0f;
    armsWindmillAdaptiveTask->m_parameters.angSpeed = m_parameters.armAngSpeed;
    armsWindmillAdaptiveTask->m_parameters.amplitude = m_parameters.armAmplitude;
    armsWindmillAdaptiveTask->m_parameters.phase = m_parameters.armPhase;//1.0f;
    armsWindmillAdaptiveTask->m_parameters.disableOnImpact = false;
    armsWindmillAdaptiveTask->m_parameters.bendLeftElbow = m_parameters.armBendElbows;
    armsWindmillAdaptiveTask->m_parameters.bendRightElbow = m_parameters.armBendElbows;


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
    headLookTask->m_parameters.m_stiffness = m_parameters.bodyStiffness;
    headLookTask->m_parameters.m_damping = m_parameters.bodydamping;
    headLookTask->m_parameters.m_vel = m_character->m_COMvel;

  }

  // Writhe over Foetal FTW!
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUSmartFall::duringGlide()
  {
    // Reset feedback flag.
    m_feedbackSent = false;

    GET_TASK(bodyFoetal, BodyFoetal)
    GET_TASK(bodyWrithe, BodyWrithe)

    if(!bodyFoetalTask->isActive())
    {
      bodyFoetalTask->updateBehaviourMessage(NULL);
      bodyFoetalTask->setPriority(10);
      bodyFoetalTask->activate();
    }

    if(!bodyWritheTask->isActive())
    {
      bodyWritheTask->updateBehaviourMessage(NULL);

      bodyWritheTask->m_parameters.m_armStiffness = 15.0f;

      const float basePeriod = 0.1f;
      bodyWritheTask->m_parameters.m_armPeriod = basePeriod + 0.5f;
      bodyWritheTask->m_parameters.m_legPeriod = basePeriod + 0.5f;
      bodyWritheTask->m_parameters.m_backPeriod = basePeriod + 0.5f;

      const float blend = 1.0f;
      bodyWritheTask->m_parameters.m_blendArms = blend;
      bodyWritheTask->m_parameters.m_blendBack = blend;
      bodyWritheTask->m_parameters.m_blendLegs = 0.5f;

      bodyWritheTask->m_parameters.m_armAmplitude = 3.f;

      bodyWritheTask->setPriority(11);
      bodyWritheTask->activate();
    }
  }

  // ---------------------------------------------------------------------------------------------------------
/*
  Applies the orientation controller
*/
  void NmRsCBUSmartFall::orientationControllerTick()
  {

    rage::Vector3 feetCentre = getRightLeg()->getFoot()->getPosition() + getLeftLeg()->getFoot()->getPosition();
    feetCentre.Scale(0.5f);
    
    // the vertical velocity
    rage::Vector3 upVec = m_character->getUpVector();
    rage::Vector3 comVel = m_character->m_COMvel;

    //Apply the orientation control
    if (m_controlOrientation && m_parameters.pdStrength > 0.001f)
    {
      rage::Matrix34 currentMat;
      float aimAngle = 0.0f;//Just declared here because of bSpy output
      //Input to orientation controller
      rage::Vector3 torqueVec;
      float rotation;

      if (m_parameters.alanRickman)
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
        float forwardVelRotation = m_parameters.forwardVelRotation;
        float aimAngleBase = m_parameters.aimAngleBase;

        //Overwrite orientation parameters for when preparing for a forward roll
        static float frAim = 0.7f;
        if (m_currentState == SF_ForwardRoll && m_parameters.forwardRoll)
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

        if ((rage::Abs(m_forwardVelMag) < 1.0f) || (!m_parameters.orientateBodyToFallDirection))
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
        if (m_parameters.orientateTwist)
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

      float velocityOntoTorqueVec = torqueVec.Dot(m_character->m_COMrotvel);
      float P = 260;  // 160 -- maybe need to ramp these up
      float D = 80;   // 60
      //float torqMag = m_parameters.pdStrength*(P*rotation - D*velocityOntoTorqueVec);
      float spring = m_parameters.pdStrength*P*rotation;
      float damping = m_parameters.pdStrength*m_parameters.pdDamping*D*velocityOntoTorqueVec;
      float torqMag;
#if ART_ENABLE_BSPY
      torqMag = m_parameters.pdStrength*(P*rotation - m_parameters.pdDamping*D*velocityOntoTorqueVec);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall - orient", aimAngle);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall - orient", rotation);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall - orient", torqueVec);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall", torqMag);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall", spring);
      bspyScratchpad(m_character->getBSpyID(), "SmartFall", damping);
#endif
      torqMag =rage::Clamp(spring-damping, -m_parameters.orientateMax, m_parameters.orientateMax);
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "SmartFall", torqMag);
#endif
      float scale = rage::Clamp(60.f * m_character->getLastKnownUpdateStep(), 1.f, 60.f);//clamp to keep torque same above 60fps and reduce torque from 60fps to 1fps
      torqMag = torqMag/scale;
      torqueVec.Scale(torqMag);
      getSpine()->getPelvisPart()->applyTorque(torqueVec);
    }//if (m_controlOrientation)//Apply the orientation control
  }

  void NmRsCBUSmartFall::calculateCommonVariables()
  {
    // Are we uprightish? If so we need to do a catch fall?
    m_goToCatchFallFlag = (m_character->m_gUp.Dot(m_character->m_COMTM.b) < m_parameters.catchFallCutOff);

    // Obtain horizontal COM velocity component.
    m_horComVel = m_character->m_COMvel;
    m_character->levelVector(m_horComVel);

    // Sink ratio: downward speed / total speed
    float newSinkRate = -m_character->m_gUp.Dot(m_character->m_COMvel);
    m_sinkAccel = newSinkRate - m_sinkRate;
    m_sinkRate = newSinkRate;
    float fHorizVelMag = m_horComVel.Mag();
    m_sinkRatio = m_sinkRate / (rage::Abs(m_sinkRate) + fHorizVelMag);

    // Is character facing the direction it is moving?
    rage::Vector3 comVelDir = m_character->m_COMvel;
    comVelDir.NormalizeSafeV();
    m_impactVisible = ((-m_character->m_COMTM.c).Dot(comVelDir) > 0.2f);

    // Get current facing direction.
    m_orientationStateCurrent = m_character->getFacingDirectionFromCOMOrientation(m_character->m_COMTM, (m_character->m_probeHit[NmRsCharacter::pi_highFall] ? &m_character->m_probeHitNormal[NmRsCharacter::pi_highFall] : NULL));

    // Calculate predicted orientation for time to impact halved.
    rage::Matrix34 predCOMTM;
    m_character->getPredictedCOMOrientation(m_predImpactTime * 0.5f, &predCOMTM);
    m_orientationStateTimeHalved = m_character->getFacingDirectionFromCOMOrientation(predCOMTM, (m_character->m_probeHit[NmRsCharacter::pi_highFall] ? &m_character->m_probeHitNormal[NmRsCharacter::pi_highFall] : NULL));
#if ART_ENABLE_BSPY
    m_character->bspyDrawCoordinateFrame(0.5f, predCOMTM);
#endif

    // REBOUND ASSISTANCE: Compute some common values.
    //
    float bounceScale = 10000.0f * m_parameters.reboundScale;
    const float maxBounceDuration = 0.2f;
    bool canBounce = (m_parameters.reboundScale > 0.0f) && (m_reboundTime >= 0.0f);
    float reboundVelMag = m_reboundVelocity.Mag();
    if(reboundVelMag < m_character->m_COMvelMag)
    {
      m_reboundVelocity = m_character->m_COMvel;
      reboundVelMag = m_character->m_COMvelMag;
    }
    bool doBounce = canBounce && (m_timer < m_reboundTime + maxBounceDuration);  
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "SmartFall", canBounce);
    bspyScratchpad(m_character->getBSpyID(), "SmartFall", doBounce);
#endif

    // Wait a bit to filter initial contacts with weapons and ground.
    //
    if(m_timer > 0.1f)
    {
      // Average contact normals to avoid thinking we're in contact
      // while sliding down walls and such.
      //
      BehaviourMask hitMask = m_character->collidedWithWorld(bvmask_Full);
      m_hasCollidedWithWorld = (hitMask == bvmask_None ? false : true);
      if(hitMask)
      {
        m_character->m_groundNormal.Zero();
        rage::Vector3 pos, normal;
        float depth = 0;
        rage::phInst *inst = NULL;
        int instGenID = -1;
        for(int i = 0; i < m_character->getNumberOfParts(); i++)
        {
          if(m_character->isPartInMask(hitMask, i))
          {
            NmRsGenericPart* part = m_character->getGenericPartByIndex(i);
            part->getCollisionZMPWithNotOwnCharacter(pos, normal, &depth, &inst, &instGenID);
            m_character->m_groundNormal.Add(normal);

            // REBOUND ASSISTANCE: Apply impulse to parts.
            //
            if(doBounce && m_character->isPartInMask(m_parameters.reboundMask, i))
            {
              float partMass = m_character->getArticulatedBody()->GetMass(i).Getf();
              float scale = rage::Clamp(reboundVelMag, 0.0f, 15.0f) / 15.0f; // make dead zone?
              scale *= bounceScale;
              normal.Scale(partMass * scale * rage::Max(depth, 0.0f));
              part->applyForce(normal);
            }
          }
        }
        m_character->m_groundNormal.Normalize();
        m_supported = m_character->m_groundNormal.Dot(m_character->m_gUp) > 0.7f;

        // REBOUND ASSISTANCE: Start rebound!
        //
        if(m_supported && m_currentState != SF_Balance && m_reboundTime < 0.0f)
        {
          m_reboundTime = m_timer;
        }
      }
      else
      {
        m_supported = false;
      }
    }

    // State transition variables - computed to simplify state transition code.
    m_aboutToLand = (m_predImpactTime < m_parameters.catchfalltime + 0.15f);
    m_hasCollided = (m_hasCollidedWithWorld && !m_parameters.ignorWorldCollisions);
    m_landingUpright = (m_willLandOnFeet && m_normalDotUp > 0.7);
    m_impactDown = (m_orientationState == NmRsCharacter::OS_Down);
    m_impactFront = (m_orientationState == NmRsCharacter::OS_Front);
    m_impactPredictedFront = (m_orientationStateCurrent == NmRsCharacter::OS_Front && m_orientationStateTimeHalved == NmRsCharacter::OS_Front);
    m_impactVisible = m_impactVisible && (m_impactFront || m_impactDown);
    m_toExitFall = m_hasCollided || (m_aboutToLand && !m_landingUpright);

    // slope - Scale steepness to emphasize the first 15 degrees or so and get a range from 0..1 (flat..~25degrees)
    //
    const float maxSteepness = 0.1f;
    m_slope = rage::Clamp(1.0f - m_character->m_groundNormal.Dot(m_character->m_gUp), 0.0f, maxSteepness) / maxSteepness;
    const float vertComVel = m_character->m_COMvel.Dot(m_character->m_gUp);
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "SmartFall-SlowEnoughToRecover", vertComVel);
    float steepness = 1.0f - m_character->m_groundNormal.Dot(m_character->m_gUp);
    bspyScratchpad(m_character->getBSpyID(), "SmartFall-SlowEnoughToRecover", steepness);
    bspyScratchpad(m_character->getBSpyID(), "SmartFall-SlowEnoughToRecover", maxSteepness);
#endif
    // Scale perceived vertical COM velocity magnitude by inverse of slope
    // measure to effectively increase maxSpeedForRecoverableFall value on slopes.
    m_slowEnoughToRecover = m_slowEnoughToRecover && (vertComVel * (1.0f - m_slope) > -m_parameters.maxSpeedForRecoverableFall);
    m_restartFall = !m_aboutToLand && (m_predImpactTime > 0.4f) && !m_hasCollidedWithWorld && (m_height > 1.5f);
    m_fallingDown = (m_sinkRatio > 0.3f);
    m_dead = (m_character->m_health == 0.0f);
    m_teeterFailed = false;
  }

  // Function updates the following member variables: m_probeHit, m_normalDotUp, m_willLandOnFeet, m_predImpactTime and m_orientationState.
  void NmRsCBUSmartFall::probeEnvironmentAndCalcForFall()
  {
    // PROBE AHEAD of the falling character.  Ideally, this would be a piecewise
    // linear approximation of a parabolic path, but we don't have the free probes.
    // 
    // JRP TODO consider doing simple approximation with two probes? 1st probe
    // should be the length of time required to get hands out for a brace.
    //
    const float probeTime = 1.0f;
    rage::Vector3 endOfProbe, startOfProbe;
    startOfProbe.Set(m_character->m_COMTM.d);
    endOfProbe.AddScaled(m_character->m_COMTM.d, m_character->m_COMvel, probeTime);

    // This works best if we're allowed a sphere and a swept sphere test per
    // frame (static sphere needed to account for swept sphere missing contacts
    // within the initial sphere.
    //
    // If this is OK to do (we've only used ray probes to date), this set of
    // tests will need to be better integrated into the NM probe managmeent.
    // At the moment, non-ray tests are supported only for synchronous probes.
#define USE_SWEPT_SPHERE 0
#if USE_SWEPT_SPHERE
    const float probeRadius = 0.5f;
    m_probeHit = m_character->probeRay(
      NmRsCharacter::pi_highFall,
      startOfProbe, endOfProbe,
      rage::phLevelBase::STATE_FLAGS_ALL,
      TYPE_FLAGS_ALL,
      m_character->m_probeTypeIncludeFlags,
      m_character->m_probeTypeExcludeFlags,
      false,
      &probeRadius);
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "SmartFall - probe", startOfProbe);
    bspyScratchpad(m_character->getBSpyID(), "SmartFall - probe", endOfProbe);
    bspyScratchpad(m_character->getBSpyID(), "SmartFall - probe", probeRadius);
#endif
#else
    // If we are using ray probes, we need to compensate a bit for acceleration
    // due to gravity in order to make sure we get hits while sliding on slopes.
    //endOfProbe.AddScaled(m_character->getSimulator()->GetGravity(), probeTime / 2.5f);

    m_probeHit = m_character->probeRay(
      NmRsCharacter::pi_highFall,
      startOfProbe, endOfProbe,
      rage::phLevelBase::STATE_FLAGS_ALL,
      TYPE_FLAGS_ALL,
      m_character->m_probeTypeIncludeFlags,
      m_character->m_probeTypeExcludeFlags,
      false);
#endif

    m_normalDotUp = m_character->m_probeHitNormal[NmRsCharacter::pi_highFall].Dot(m_character->getUpVector());
    m_landingNormalOK = m_normalDotUp > m_parameters.landingNormal;

    // work out the vertical height vector to the ground
    rage::Vector3 hitDirVert = m_character->m_COMTM.d - m_character->m_probeHitPos[NmRsCharacter::pi_highFall];
    rage::Vector3 hitDirHor = hitDirVert;
    m_character->levelVector(hitDirHor);
    hitDirVert = hitDirVert - hitDirHor;

    m_normalDotUp = m_character->m_probeHitNormal[NmRsCharacter::pi_highFall].Dot(m_character->getUpVector());
    m_landingNormalOK = m_normalDotUp > m_parameters.landingNormal;

    // Get height above ground.
    float height = hitDirVert.Mag();

    //m_willLandOnFeet is only used on one frame to decide whether to bailout or prepare for landing
    //  don't think it makes sense to have an accuracy blend on it
    //float accuracy = 1.0f-((m_predImpactTime-m_timer)/m_predImpactTime);
    //We could add a prediction to this based on angVel and m_predImpactTime.
    //However the angVel will change due to the circling stopping and based on the decision to bail or prepare
    m_willLandOnFeet = m_character->m_gUp.Dot(m_character->m_COMTM.b) > m_parameters.crashOrLandCutOff;

    // If Agent is due to land on its feet, decrease the height of the fall that is measured
    // from the probe pos (currently at the COM) with the length of the leg.
    if (m_willLandOnFeet && height > m_leftLegLength)
    {
      height -= m_leftLegLength;
    }

    const float verticalVelMag = (m_character->m_COMvel).Dot(m_character->getUpVector());

    // JRP this metric may not work so well for horizontal flight and will
    // completely fail when verticalVelMag is positive.
    //
    //-- the solution to the quadratic equation is: (time to impact)
    rage::Vector3 gravity = rage::phSimulator::GetGravity();
    float a = gravity.Mag();
    float b = -rage::Abs(verticalVelMag); // !!!
    float c = height;
    if (m_probeHit)
    {
      Assert(b*b+2*a*c >= 0.f && rage::Abs(a)>1e-10f);
      m_predImpactTime = (b+sqrtf(b*b+2*a*c))/(a);
    }
    else
    {
      m_predImpactTime = 30.f; // default to never reach
    }

    // Get Agent predicted COMTM orientation on impact.
    rage::Matrix34 predCOMTM;
    m_character->getPredictedCOMOrientation(m_predImpactTime, &predCOMTM);

    // Check which way Agent would be facing if lying on the ground with
    // predicted orientation: Up, Down, Front, Back, Left, Right.
    m_orientationState = m_character->getFacingDirectionFromCOMOrientation(predCOMTM, (m_character->m_probeHit[NmRsCharacter::pi_highFall] ? &m_character->m_probeHitNormal[NmRsCharacter::pi_highFall] : NULL));

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "SmartFall - probe ahead", endOfProbe);
    bspyScratchpad(m_character->getBSpyID(), "SmartFall - probe ahead", m_character->m_probeHitPos[NmRsCharacter::pi_highFall]);
    bspyScratchpad(m_character->getBSpyID(), "SmartFall - probe ahead", hitDirVert);

    // Predicted COMTM.
    m_probeHitPos = m_character->m_probeHitPos[NmRsCharacter::pi_highFall];
    predCOMTMXAxis = predCOMTM.a + m_probeHitPos;
    predCOMTMYAxis = predCOMTM.b + m_probeHitPos;
    predCOMTMZAxis = predCOMTM.c + m_probeHitPos;
#endif

    // PROBE DOWN to determine if we're close to the ground (so things like RDS
    // are not interrupted by bouncing.
    //
    const float heightProbeLength = 5.0f;
    endOfProbe = startOfProbe;
    endOfProbe.AddScaled(m_character->m_gUp, -heightProbeLength);
    m_probeDownHit = m_character->probeRay(
      NmRsCharacter::pi_highFallDown,
      startOfProbe, endOfProbe,
      rage::phLevelBase::STATE_FLAGS_ALL,
      TYPE_FLAGS_ALL,
      m_character->m_probeTypeIncludeFlags,
      m_character->m_probeTypeExcludeFlags,
      false);
    if(m_probeDownHit)
    {
      rage::Vector3 toGround = m_character->m_COMTM.d - m_character->m_probeHitPos[NmRsCharacter::pi_highFallDown];
      m_height = toGround.Mag();
      m_character->m_groundNormal = m_character->m_probeHitNormal[NmRsCharacter::pi_highFallDown];
    }
    else
    {
      m_height = heightProbeLength; // we are at least this high...
    }

#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "SmartFall - probe down", endOfProbe);
    bspyScratchpad(m_character->getBSpyID(), "SmartFall - probe down", m_character->m_probeHitPos[NmRsCharacter::pi_highFallDown]);
#endif

  }

  void NmRsCBUSmartFall::doBrace()
  {
    // TODO parameterize...
    if(m_predImpactTime > 0.5f)
      return;

    const float timeStep = m_character->getLastKnownUpdateStep();

    const float braceDistance = 0.6f;
    const float targetPredictionTime = 0.45f; // time epected to get arms up from idle
    const float minBraceTime = 0.3f;
    const float timeToBackwardsBrace = 0.5f;
    const float handsDelayMin = 0.0f; // 0.3f; // min delay to break up 2-handed symmetry
    const float handsDelayMax = 0.0f; // 0.7f; // max delay to break up 2-handed symmetry
    const float reachAbsorbtionTime = 0.15f; // larger values and he absorbs the impact more
    const float braceStiffness = 12.0f;

    const rage::Vector3 target(m_character->m_COM + m_character->m_COMvel);
    const rage::Vector3 targetVel(0.0f, 0.0f, 0.0f);

    float distanceToTarget;
    float braceTime;
    float backwardsBraceTimer;

    bool shouldBrace;
    bool doBrace;

    m_character->DecideToBrace(
      timeStep,
      target,
      targetVel,
      braceDistance,
      targetPredictionTime,
      minBraceTime,
      timeToBackwardsBrace,
      distanceToTarget,
      braceTime,
      backwardsBraceTimer,
      shouldBrace,
      doBrace);

    bool braceLeft;
    bool braceRight;
    float handsDelay; 
    bool delayLeftHand;

    m_character->DecideBraceHands(
      timeStep,
      target,
      doBrace,
      braceLeft,
      braceRight,
      braceTime,
      handsDelay,
      handsDelayMin,
      handsDelayMax,
      delayLeftHand,
      m_leftHandSeparation,
      m_rightHandSeparation);

    if (doBrace)
    {
      rage::Vector3 leftHandPos;
      rage::Vector3 rightHandPos;

      m_character->ArmsBrace(
        target,
        targetVel,
        reachAbsorbtionTime,
        braceDistance,
        braceStiffness,
        braceLeft,
        braceRight,
        m_leftHandSeparation,
        m_rightHandSeparation,
        m_body,
        leftHandPos, 
        rightHandPos);
    }
  }

  float NmRsCBUSmartFall::getLeftLegLength() const
  {
    // Measure left leg length.
    float legLength = (getLeftLeg()->getKnee()->getJointPosition() - getLeftLeg()->getHip()->getJointPosition()).Mag();
    legLength += (getLeftLeg()->getKnee()->getJointPosition() - getLeftLeg()->getAnkle()->getJointPosition()).Mag();
    return legLength;
  }

#if ART_ENABLE_BSPY
  void NmRsCBUSmartFall::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.bodyStiffness, true);
    bspyTaskVar(m_parameters.bodydamping, true);
    bspyTaskVar(m_parameters.catchfalltime, true);
    bspyTaskVar(m_parameters.crashOrLandCutOff, true);
    bspyTaskVar(m_parameters.pdStrength, true);
    bspyTaskVar(m_parameters.pdDamping, true);
    bspyTaskVar(m_parameters.catchFallCutOff, true);
    bspyTaskVar(m_parameters.armsUp, true);
    bspyTaskVar(m_parameters.legRadius, true);
    bspyTaskVar(m_parameters.legAngSpeed, true);
    bspyTaskVar(m_parameters.legAsymmetry, true);
    bspyTaskVar(m_parameters.arms2LegsPhase, true);
    bspyTaskVar(m_parameters.arms2LegsSync, true);
    bspyTaskVar(m_parameters.armAngSpeed, true);
    bspyTaskVar(m_parameters.armAmplitude, true);
    bspyTaskVar(m_parameters.armPhase, true);
    bspyTaskVar(m_parameters.armBendElbows, true);
    bspyTaskVar(m_parameters.aimAngleBase, true);
    bspyTaskVar(m_parameters.forwardVelRotation, true);
    bspyTaskVar(m_parameters.footVelCompScale, true);
    bspyTaskVar(m_parameters.legL, true);//unused
    bspyTaskVar(m_parameters.sideD, true);
    bspyTaskVar(m_parameters.forwardOffsetOfLegIK, true);
    bspyTaskVar(m_parameters.legStrength, true);
    bspyTaskVar(m_parameters.orientateBodyToFallDirection, true);
    bspyTaskVar(m_parameters.orientateTwist, true);
    bspyTaskVar(m_parameters.orientateMax, true);
    bspyTaskVar(m_parameters.alanRickman, true);
    bspyTaskVar(m_parameters.forwardRoll, true);
    bspyTaskVar(m_parameters.useZeroPose_withForwardRoll, true);
    bspyTaskVar(m_parameters.ignorWorldCollisions, true);
    bspyTaskVar(m_parameters.adaptiveCircling, true);
    bspyTaskVar(m_parameters.hula, true);
    bspyTaskVar(m_parameters.maxSpeedForRecoverableFall, true);
    bspyTaskVar(m_parameters.landingNormal, true);
    bspyTaskVar(m_parameters.rdsForceMag, true);
    bspyTaskVar(m_parameters.rdsTargetLinearVelocityDecayTime, true);
    bspyTaskVar(m_parameters.rdsTargetLinearVelocity, true);
    bspyTaskVar(m_parameters.rdsUseStartingFriction, true);
    bspyTaskVar(m_parameters.rdsStartingFriction, true);
    bspyTaskVar(m_parameters.rdsStartingFrictionMin, true);
    bspyTaskVar(m_parameters.rdsForceVelThreshold, true);
    bspyTaskVar(m_parameters.initialState, true);
    bspyTaskVar(m_parameters.teeter, true);
    bspyTaskVar(m_parameters.teeterOffset, true);
    bspyTaskVar(m_parameters.reboundScale, true);
    bspyTaskVar(m_parameters.stopRollingTime, true);
    bspyTaskVar_Bitfield32(m_parameters.reboundMask, true);
    bspyTaskVar(m_parameters.splatWhenStopped, true);
    bspyTaskVar(m_parameters.blendHeadWhenStopped, true);
    bspyTaskVar(m_parameters.spreadLegs, true);

    bspyTaskVar(m_timer, false);
    bspyTaskVar(m_sinkRatio, false);
    bspyTaskVar(m_sinkRate, false);
    bspyTaskVar(m_sinkAccel, false);
    bspyTaskVar(m_height, false);
    bspyTaskVar(m_slope, false);
    bspyTaskVar(m_predImpactTime, false);
    bspyTaskVar(m_forwardVelMag, false);
    bspyTaskVar(m_rollToBBtimer, false);
    bspyTaskVar(m_averageComVel, false);
    bspyTaskVar(m_hasCollidedWithWorld, false);
    bspyTaskVar(m_controlOrientation, false);
    bspyTaskVar(m_willLandOnFeet, false);
    bspyTaskVar(m_feedbackSent, false);
    bspyTaskVar(m_goToCatchFallFlag, false);
    bspyTaskVar(m_rollToBB, false);
    bspyTaskVar(m_probeHit, false);
    bspyTaskVar(m_probeHitPos, false);
    bspyTaskVar(predCOMTMXAxis, false);
    bspyTaskVar(predCOMTMYAxis, false);
    bspyTaskVar(predCOMTMZAxis, false);
    bspyTaskVar(m_normalDotUp, false);
    bspyTaskVar(m_impactVisible, false);
    bspyTaskVar(m_landingNormalOK, false);
    bspyTaskVar(m_horComVel, false);
    bspyTaskVar(m_smoothCollidedWithWorld, false);
    bspyTaskVar(m_supported, false);
    bspyTaskVar(m_dead, false);
    bspyTaskVar(m_restartFall, false);
    bspyTaskVar(m_fallingDown, false);
    bspyTaskVar(m_headOrFeetFirst, false);
    bspyTaskVar(m_aboutToLand, false);
    bspyTaskVar(m_hasCollided, false);
    bspyTaskVar(m_landingUpright, false);
    bspyTaskVar(m_impactDown, false);
    bspyTaskVar(m_impactFront, false);
    bspyTaskVar(m_impactPredictedFront, false);
    bspyTaskVar(m_toExitFall, false);
    bspyTaskVar(m_slowEnoughToRecover, false);
    bspyTaskVar(m_balanceFailed, false);
    bspyTaskVar(m_teeterFailed, false);
    bspyTaskVar(m_catchFallTime, false);
    bspyTaskVar(m_reboundVelocity, false);
    bspyTaskVar(m_reboundTime, false);

    static const char* state_names[] = 
    {
#define SF_NAME_ACTION(_name) #_name ,
      SF_STATES(SF_NAME_ACTION)
#undef SF_NAME_ACTION
    };
    bspyTaskVar_StringEnum((int)GetState(),state_names, false);

    static const char* orientationState_names[] =
    {
#define SF_ORIENTATION_STATE_NAME_ACTION(_name) #_name ,
      ORIENTATION_STATES(SF_ORIENTATION_STATE_NAME_ACTION)
#undef SF_ORIENTATION_STATE_NAME_ACTION
    };
    bspyTaskVar_StringEnum(m_orientationState, orientationState_names, false);
    bspyTaskVar_StringEnum(m_orientationStateTimeHalved, orientationState_names, false);
    bspyTaskVar_StringEnum(m_orientationStateCurrent, orientationState_names, false);

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
    static int objectLevel = 1;
    m_character->getEngineNotConst()->setbSpyObject(objectLevel);
  }
#endif // ART_ENABLE_BSPY

} // nms Art
