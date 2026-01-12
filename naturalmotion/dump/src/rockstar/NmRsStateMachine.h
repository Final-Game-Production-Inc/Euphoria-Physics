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

#ifndef NM_RS_STATEMCH_H
#define NM_RS_STATEMCH_H


#include "NmRsCBU_Shared.h"

namespace ART
{
    class CBUTaskBase;

    //--------------------------------------------------------------------------------------------
    // A FSM with states as pointers to class instances
    //--------------------------------------------------------------------------------------------
    class State;
    class FSM
    {
    public:
      FSM() : m_currentState(NULL), m_previousState(NULL), m_globalState(NULL) {};
      ~FSM() {};

      void Update(float timeStep) const;
      void ChangeState(State* nextState);
      void ReturnToPreviousState();

      // use these methods to initialize the FSM
      void SetCurrentState(State* s){ m_currentState = s; };
      void SetGlobalState(State* s){ m_globalState = s; };
      void SetPreviousState(State* s){ m_previousState = s; };

      //accessors
      State* CurrentState() const  { return m_currentState; };
      State* GlobalState() const { return m_globalState; };
      State* PreviousState() const { return m_previousState; };
    
    protected:
      State* m_previousState;
      State* m_currentState;
      State* m_globalState;
    };

    // state class for use with FSM
    //-----------------------------------------------------------------------------------------------------------
    class State
    {
    public:
      State(CBUTaskBase* context = NULL) : m_context(context){};
      virtual ~State(){};

      virtual void Enter(){}; // empty entry by default, overwrite if necessary
      virtual void Exit(){};  // empty exit by default, overwrite if necessary
      virtual void Update(float timeStep) = 0; // enforce abstract class: put stuff here
      
      void setContext(CBUTaskBase* c) { m_context = c; };

    protected:  
      CBUTaskBase* m_context; // pointer to behaviour that "owns" this state for access to its interface
    };

    // specialized state for autonomous update outside of FSM logic
    //-----------------------------------------------------------------------------------------------------------
    class AutoState
    {
    public:
      AutoState(NmRsCharacter* ch = NULL, CBUTaskBase* context = NULL) : m_character(ch), m_context(context), m_entered(false), m_exited(false), m_allowLoop(true), m_time(0){};
      virtual ~AutoState(){};

      void init(NmRsCharacter* c, CBUTaskBase* task = NULL ) { m_character=c; m_context=task; };
      void reset(){ m_entered=false; m_exited=false; m_time=0; resetCustomVariables(); };
      void allowLooping(bool allow) { m_allowLoop = allow; };

      // wrappers for ensuring internal flags are being set appropriately     
      // can be used for manual update at own risk (doesn't check for entering/exiting in right order or multiple times)
      void doEnter(); 
      void doExit();
      void doUpdate(float timeStep);
      
      // checks entry- and exit condition: call instead of do...()
      void autoUpdate(float timeStep);

      // accessors
      bool hasEntered() { return m_entered; };
      bool hasExited() { return m_exited; };     
      bool allowsLooping() { return m_allowLoop; };

    protected:
      // these are internal here, so as to ensure proper usage via public interface
      virtual void enter(){}; // empty entry by default, overwrite if necessary
      virtual void exit(){};  // empty exit by default, overwrite if necessary
      virtual void update(float /*timeStep*/){}; // empty update by default

      // overwrite these in subclass, by default will run indefinitely
      virtual bool entryCondition() { return true; };
      virtual bool exitCondition() { return false; };

      // give derived classes the chance to reset it's own variables when this super-class's reset is called
      virtual void resetCustomVariables(){};

      CBUTaskBase* m_context; // pointer to behaviour that "owns" this state for access to its interface
      NmRsCharacter* m_character; // pointer to character that "owns" this state for access to its interface
      bool m_entered;
      bool m_exited;
      bool m_allowLoop;
      float m_time;
    };

    //--------------------------------------------------------------------------------------------
    // A small FSM with states encapsulated in macro'd if-defs
    //--------------------------------------------------------------------------------------------
    enum StateMachineEvent 
    {
      EVENT_Update,
      EVENT_Enter,
      EVENT_Exit
    };

    #define BeginStateMachine  if( state < 0 ) { if(0) {
    #define EndStateMachine    return( true ); } } else { Assert(0); return( false ); }  return( false );
    #define State(a)           return( true ); } } else if( a == state ) { if(0) {
    #define OnEvent(a)         return( true ); } else if( a == event ) {
    #define OnEnter            OnEvent( EVENT_Enter )
    #define OnUpdate           OnEvent( EVENT_Update )
    #define OnExit             OnEvent( EVENT_Exit )

    class StateMachine
    {
    public:
      StateMachine( void );
      virtual ~StateMachine( void ) {}

      void Reset( void );
      void Initialize( void );
      void Update( void );
      void SetState( unsigned int newState );
      unsigned int GetState(){ return m_currentState; };

    protected:
      unsigned int m_currentState;
      unsigned int m_nextState;
      bool m_stateChange;

      void Process( StateMachineEvent event );
      virtual bool States( StateMachineEvent event, int state ) = 0;
    }; // state machine

} // namespace ART

#endif // defined NM_RS_STATEMCH_H
