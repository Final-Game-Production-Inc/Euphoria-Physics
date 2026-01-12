#ifndef NM_ART_BASE_DEFS_H
#define NM_ART_BASE_DEFS_H


#ifndef ART_NO_ERROR_LOGGING
# define ART_ENABLE_ERROR_LOGGING
#endif // ART_NO_ERROR_LOGGING

namespace ART
{
  /**
   * Uniquely identifies an Asset within an ARTContext.
   */
  typedef unsigned int  AssetID;  

  /**
   * Uniquely identifies an Agent within an ARTContext. 
   * Will be 0-based, sequential and no higher than the 'max agents' value
   * passed in during ART ctor.
   */
  typedef unsigned int  AgentID;

  /**
   * AgentID that represents an invalid/nonexistent agent. 
   */
  extern const  AgentID INVALID_AGENTID;
  extern const  int MAX_AGENTS;
  extern const  int MAX_NON_NM_RAGDOLLS;
  extern const  unsigned int MAX_BSPY_AGENTS_OF_EACH_TYPE;

  /**
   * Identifies a type of data within a DataOutputter.
   */
  typedef unsigned int  DataTypeID;

  /**
   * ID code used when using VW invoke() function
   */
  typedef unsigned int  InvokeUID;

  class NmRsCharacter;
}


#endif // NM_ART_BASE_DEFS_H
