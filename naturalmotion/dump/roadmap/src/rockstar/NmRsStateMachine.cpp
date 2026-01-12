/*
 * Copyright (c) 2006 NaturalMotion Ltd. All rights reserved. 
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

/*
 * "Portions Copyright (C) Steve Rabin, 2001"
 */

#include "NmRsInclude.h"
#include "NmRsStateMachine.h"

namespace ART
{
     //--------------------------------------------------------------------------------------------
    // A FSM with states as pointers to class instances
    //--------------------------------------------------------------------------------------------
    void FSM::ReturnToPreviousState()
    {
      ChangeState(m_previousState);
    }

    void FSM::ChangeState(State* nextState) 
    { 
      Assert(nextState);
      
      // do nothing if asked to switch to current state
      if(m_currentState == nextState)
        return;

      m_previousState = m_currentState;

      // exit last state if we have one
      if(m_currentState)
      m_currentState->Exit();

      m_currentState = nextState;
      m_currentState->Enter();      
    }

    void FSM::Update(float timeStep) const
    {
      if (m_globalState)   
        m_globalState->Update(timeStep);

      if(m_currentState) 
        m_currentState->Update(timeStep);
    }

    //--------------------------------------------------------------------------------------------
    // A small specialized state class for auto-updating outside of a FSM
    //--------------------------------------------------------------------------------------------
    void AutoState::doEnter()
    {
      m_time = 0.0f;
      enter();
      m_entered = true;
    }

    void AutoState::doExit()
    {
      exit();
      m_exited = true;
      m_entered = false;
    }

    void AutoState::doUpdate(float timeStep)
    {
      update(timeStep);
      m_time += timeStep;
    }

    void AutoState::autoUpdate(float timeStep)
    {
      // if already started, check whether we need to exit
      if(m_entered && exitCondition())
      {
        doExit();
      }

      // check whether we want to start this state off (again)
      bool loopGuard = m_allowLoop || (m_exited == false);
      if(!m_entered && loopGuard && entryCondition())
      {
        doEnter();
      }

      // finally do update tick
      if(m_entered)
      {
        doUpdate(timeStep);
      }
    }


    //--------------------------------------------------------------------------------------------
    // A small FSM with states encapsulated in macro'd if-defs
    //--------------------------------------------------------------------------------------------
    StateMachine::StateMachine( void )
    {
      Reset();
    }

    void StateMachine::Reset( void )
    {
      m_currentState = 0;
      m_stateChange = false;
      m_nextState = false;
    }

    void StateMachine::Initialize( void )
    {
      //Reset();
      Process( EVENT_Enter );
    }

    void StateMachine::Update( void )
    {
      Process( EVENT_Update );
    }

    void StateMachine::Process( StateMachineEvent event )
    {
      States( event, m_currentState ); // virtual function in StateMachine,
                                       // behaviour States function implementation is called here;

      // Check for a state change
      int safetyCount = 10;
      while( m_stateChange && (--safetyCount >= 0) )
      {
        //assert( safetyCount > 0 && "StateMachine::Process - States are flip-flopping in an infinite loop." );

        m_stateChange = false;

        // Let the last state clean-up;
        States( EVENT_Exit, m_currentState );

        // Set the new state;
        m_currentState = m_nextState;

        // Let the new state initialize
        States( EVENT_Enter, m_currentState );
      }
    }

    void StateMachine::SetState( unsigned int newState )
    {
      m_stateChange = true;
      m_nextState = newState;
    }

}
