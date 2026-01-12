#ifndef NM_ART_FEEDBACK_H
#define NM_ART_FEEDBACK_H

#include "art/ARTBaseDefs.h"
#include "nmutils/NMTypes.h"

namespace ART
{
  class NmRsCharacter;

  #define ART_FEEDBACK_BHNAME_LENGTH    (128)
  #define ART_FEEDBACK_MAX_USERDATA     (32)

  /**
   * \brief %ART Feedback Interface class
   *
   * This class provides hosts with a way to get notifications from
   * inside ART about various behavioural events.
   *
   * As this class will not be explicitly built by ART or any of it's 
   * systems, it does not conform to the ART memory management mechanics.
   * It is envisaged that the host will allocate and assign an instance
   * itself.
   */
  class ARTFeedbackInterface
  {
  public:

    /**
     * simple userdata structure used to pass arbitrary data
     * back from the callback functions
     */
    struct FeedbackUserdata
    {
      enum Type
      {
        kInt,
        kFloat,
        kBool,
        kVoid,
#if ART_ENABLE_BSPY
        kString,
#endif
      };

      union
      {
        int                 m_int;
        float               m_float;
        bool                m_bool;
        void*               m_ptr;
#if ART_ENABLE_BSPY
        char*               m_string;
#endif
      };

      Type      m_type;

      /**
       * some helper functions to set userdata in-place
       */
      void setInt(int value) { m_type = kInt; m_int = value; }
      void setFloat(float value) { m_type = kFloat; m_float = value; }
      void setBool(bool value) { m_type = kBool; m_bool = value; }
      void setVoid(void* value) { m_type = kVoid; m_ptr = value; }
#if ART_ENABLE_BSPY
      void setString(char* value) { m_type = kString; m_string = value; }
#endif
    };

    ARTFeedbackInterface() : m_agentID(INVALID_AGENTID), m_argsCount(0)
    {
      m_behaviourName[0] = '\0';
      m_calledByBehaviourName[0] = '\0';
    }

    virtual ~ARTFeedbackInterface(){};

    /**
     * overload one or more of these functions; check your behaviour
     * documentation for full specs on the arguments that will be present
     */
    virtual int onBehaviourStart()    { return -1; }    // automatically fired when a behaviour is started
    virtual int onBehaviourFailure()  { return -1; }
    virtual int onBehaviourSuccess()  { return -1; }
    virtual int onBehaviourFinish()   { return -1; }    // automatically fired when a behaviour deactivates
    virtual int onBehaviourEvent()    { return -1; }    // allows arbitrary event feedback from behaviours
    virtual int onBehaviourRequest()  { return -1; }    // used to communicate between behaviour -> game -> behaviour


    /**
     * all member variables of this class will be updated 
     * before each callback function is called. during the 
     * overloaded function, the user can then inspect the 
     * contents of the class.
     */
    ART::AgentID          m_agentID;
    ART::NmRsCharacter   *m_agentInstance;
    // phase 2 todo. change these next two members to be something other than
    // character arrays. it is silly to push around strings when they indicate
    // known types.
    char                  m_behaviourName[ART_FEEDBACK_BHNAME_LENGTH];
    char                  m_calledByBehaviourName[ART_FEEDBACK_BHNAME_LENGTH];
    FeedbackUserdata      m_args[ART_FEEDBACK_MAX_USERDATA];
    unsigned int          m_argsCount;
  };
}

#endif // NM_ART_INTERNAL_H
