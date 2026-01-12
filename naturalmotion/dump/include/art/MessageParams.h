#ifndef NM_MESSAGE_PARAMS_H
#define NM_MESSAGE_PARAMS_H

namespace ART
{
  // Define upfront how many parameters we can send through in a single MessageParams instance.
  // MessageParamsList<> can be used alternatively to reduce stack usage or use more parameters for
  // just a few messages.
  #define ART_MAXIMUM_PARAMETERS_IN_MESSAGEPARAMS       (64)

  // How long a parameter name key can be (not including the terminator)
  #define ART_PARAMETER_NAME_LENGTH_IN_MESSAGEPARAMS    (31)

  // How long a parameter string can be (not including the terminator))
  #define ART_PARAMETER_STRING_LENGTH_IN_MESSAGEPARAMS  (31)

  /**
  * \brief Message parameter list descriptor.
  *
  * %MessageParamsBase is used to pass parameters to message handling functions. No actual storage
  * is provided in this base class, MessageParamsList or MessageParams provide storage. Classes
  * derived from %MessageParamsBase are designed to be allocated on the stack therefore after
  * modifying this class DLL clients must be re-build!
  */
  class MessageParamsBase
  {
  public:

    enum ParamType
    {
      kInvalid = 0,
      kInt,
      kFloat,
      kBool,
      kString,
      kVector3,
      kReference,
      kUnknown
    };

    struct Parameter
    {
      ParamType   m_type;
      char        m_name[ART_PARAMETER_NAME_LENGTH_IN_MESSAGEPARAMS + 1];

      union Value
      {
        int i;
        float f;
        bool b;
        char s[ART_PARAMETER_STRING_LENGTH_IN_MESSAGEPARAMS + 1];
        float vec[3];
        const void *r;
      } v;
    };

    /**
    * Call reset() if you wish to re-use a MessageParams structure; note that
    * you need add every parameter from scratch if you do so, nothing will be kept
    */
    void reset();

    bool addInt(const char* key, int val);
    bool addFloat(const char* key, float val);
    bool addBool(const char* key, bool val);
    bool addString(const char* key, const char* val);
    bool addVector3(const char* key, float x, float y, float z);
    bool addReference(const char* key, char* val);
    bool addReference(const char* key, const void* val);

    /// If more parameters have been added than there is space this will return
    /// true. This will be checked and asserted on when passing messages to ART.
    bool getParameterOverflow() const { return m_parameterOverflow; }

    int getMaxParamCount() const { return m_maxParamCount; }
    int getUsedParamCount() const { return m_usedParamCount; }

    Parameter& getParam(int index) { FastAssert(index >=0 && index < m_maxParamCount); return m_params[index]; }
    const Parameter& getParam(int index) const { FastAssert(index >=0 && index < m_maxParamCount); return m_params[index]; }

  protected:

    MessageParamsBase(Parameter* const params, int maxParamCount);
    MessageParamsBase(const MessageParamsBase& from);

    void setTo(const MessageParamsBase& from);

  private:

    Parameter* m_params;
    int        m_maxParamCount;
    int        m_usedParamCount;
    bool       m_parameterOverflow;

    /// The copy operator is not available use setTo() instead!
    MessageParamsBase& operator=(const MessageParamsBase& from);
  };

  /**
  * \brief Variable parameters for PostMessage
  *
  * %MessageParamsList is used to pass a configurable maximum number of
  * parameters to message handling functions. It is designed to be allocated
  * on the stack, have parameters added to it and then be sent to an agent to
  * activate a behaviour. This class should not be derived from.
  */
  template<int maxParamCount>
  class MessageParamsList
    : public MessageParamsBase
  {
  public:

    MessageParamsList()
      : MessageParamsBase(m_params, maxParamCount)
    {
    }

    MessageParamsList(const MessageParamsList& from)
      : MessageParamsBase(from)
    {
      copyParams(from);
    }

    void setTo(const MessageParamsList& from)
    {
      MessageParamsBase::setTo(from);
      copyParams(from);
    }

  private:

    Parameter m_params[maxParamCount];

    /// The copy operator is not available use setTo() instead!
    MessageParamsList& operator=(const MessageParamsList& from);

    void copyParams(const MessageParamsList& from)
    {
      for (int i = 0; i < maxParamCount; ++i) { m_params[i] = from.m_params[i]; }
    }
  };

  /**
  * \brief Parameters for PostMessage
  *
  * %MessageParams is used to pass values to message handling functions.
  * It is designed to be allocated on the stack, have parameters added to it 
  * and then be sent to an agent to activate a behaviour. 
  */
  typedef MessageParamsList<ART_MAXIMUM_PARAMETERS_IN_MESSAGEPARAMS> MessageParams;
}

#endif // NM_MESSAGE_PARAMS_H
