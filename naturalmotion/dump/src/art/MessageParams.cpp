#include "art/ARTInternal.h"
#include "art/MessageParams.h"
#include "math/amath.h"  //Just to get rage::Min

namespace ART
{
  MessageParamsBase::MessageParamsBase(Parameter* const params, int maxParamCount)
    : m_params(params)
    , m_maxParamCount(maxParamCount)
    , m_usedParamCount(0)
    , m_parameterOverflow(false)
  {
    reset();
  }

  MessageParamsBase::MessageParamsBase(const MessageParamsBase& from)
    : m_params(from.m_params)
    , m_maxParamCount(from.m_maxParamCount)
    , m_usedParamCount(from.m_usedParamCount)
    , m_parameterOverflow(from.m_parameterOverflow)
  {
  }

  void MessageParamsBase::setTo(const MessageParamsBase& from)
  {
    m_params = from.m_params;
    m_maxParamCount = from.m_maxParamCount;
    m_usedParamCount = from.m_usedParamCount;
    m_parameterOverflow = from.m_parameterOverflow;
  }

  void MessageParamsBase::reset()
  {
    m_usedParamCount = 0;
    m_parameterOverflow = false;
    memset(m_params, 0, sizeof(Parameter) * m_maxParamCount);
  }

  // this is used in each add...() function to copy in the key name safely and return false
  #define ART_COPY_KEY_VALUE_TO_PARAM size_t keyLength = strlen(key);\
    if (keyLength > ART_PARAMETER_NAME_LENGTH_IN_MESSAGEPARAMS)\
      return false;\
    strncpy(m_params[m_usedParamCount].m_name,\
      key,\
      rage::Min(keyLength, (size_t)ART_PARAMETER_NAME_LENGTH_IN_MESSAGEPARAMS));


  /**
   * Adds an int value to be associated with the given named key.
   * Returns <tt>false</tt> if the key could not be added - the key name
   * could be longer than ART_PARAMETER_NAME_LENGTH_IN_MESSAGEPARAMS or there
   * were already m_maxParamCount parameters added to this MessageParams.
   */
  bool MessageParamsBase::addInt(const char* key, int val)
  {
    if (m_usedParamCount < m_maxParamCount)
    {
      ART_COPY_KEY_VALUE_TO_PARAM

      m_params[m_usedParamCount].m_type = kInt;
      m_params[m_usedParamCount].v.i = val;

      m_usedParamCount ++;
      return true;
    }

    m_parameterOverflow = true;
    return false;
  }

  /**
   * Adds a float value to be associated with the given named key.
   * Returns <tt>false</tt> if the key could not be added - the key name
   * could be longer than ART_PARAMETER_NAME_LENGTH_IN_MESSAGEPARAMS or there
   * were already m_maxParamCount parameters added to this MessageParams.
   */
  bool MessageParamsBase::addFloat(const char* key, float val)
  {
    FastAssert(val == val);
    if (m_usedParamCount < m_maxParamCount)
    {
      ART_COPY_KEY_VALUE_TO_PARAM

      m_params[m_usedParamCount].m_type = kFloat;
      m_params[m_usedParamCount].v.f = val;

      m_usedParamCount ++;
      return true;
    }

    m_parameterOverflow = true;
    return false;
  }

  /**
   * Adds a boolean value to be associated with the given named key.
   * Returns <tt>false</tt> if the key could not be added - the key name
   * could be longer than ART_PARAMETER_NAME_LENGTH_IN_MESSAGEPARAMS or there
   * were already m_maxParamCount parameters added to this MessageParams.
   */
  bool MessageParamsBase::addBool(const char* key, bool val)
  {
    if (m_usedParamCount < m_maxParamCount)
    {
      ART_COPY_KEY_VALUE_TO_PARAM

      m_params[m_usedParamCount].m_type = kBool;
      m_params[m_usedParamCount].v.b = val;

      m_usedParamCount ++;
      return true;
    }

    m_parameterOverflow = true;
    return false;
  }

  /**
   * Adds a string value to be associated with the given named key.
   * Returns <tt>false</tt> if the key could not be added - the key name
   * could be longer than ART_PARAMETER_NAME_LENGTH_IN_MESSAGEPARAMS or there
   * were already m_maxParamCount parameters added to this MessageParams.
   * Will also return <tt>false</tt> if the string value passed in is longer than
   * ART_PARAMETER_STRING_LENGTH_IN_MESSAGEPARAMS
   */
  bool MessageParamsBase::addString(const char* key, const char* val)
  {
    if (m_usedParamCount < m_maxParamCount)
    {
      ART_COPY_KEY_VALUE_TO_PARAM

      size_t stringLength = strlen(val);
      if (stringLength > ART_PARAMETER_STRING_LENGTH_IN_MESSAGEPARAMS)
        return false;
      
      m_params[m_usedParamCount].m_type = kString;
      strncpy(m_params[m_usedParamCount].v.s,
              val,
			  rage::Min(stringLength, (size_t)ART_PARAMETER_STRING_LENGTH_IN_MESSAGEPARAMS));

      m_usedParamCount ++;
      return true;
    }

    m_parameterOverflow = true;
    return false;
  }
  /**
  * Adds a reference value to be associated with the given named key.
  * Returns <tt>false</tt> if the key could not be added - the key name
  * could be longer than ART_PARAMETER_NAME_LENGTH_IN_MESSAGEPARAMS or there
  * were already m_maxParamCount parameters added to this MessageParams.
  */
  bool MessageParamsBase::addReference(const char* key, const void *val)
  {
    if (m_usedParamCount < m_maxParamCount)
    {
      ART_COPY_KEY_VALUE_TO_PARAM
      m_params[m_usedParamCount].m_type = kReference;
      m_params[m_usedParamCount].v.r = val;
      m_usedParamCount ++;
      return true;
    }

    m_parameterOverflow = true;
    return false;
  }

  bool MessageParamsBase::addReference(const char* key, char *val)
  {
    if (m_usedParamCount < m_maxParamCount)
    {
      ART_COPY_KEY_VALUE_TO_PARAM
      m_params[m_usedParamCount].m_type = kReference;
      m_params[m_usedParamCount].v.r = val;
      m_usedParamCount ++;
      return true;
    }

    m_parameterOverflow = true;
    return false;
  }

  /**
   * Syntax sugar for addVector3(char, NMVector3)
   */
  bool MessageParamsBase::addVector3(const char* key, float x, float y, float z)
  {
    FastAssert(x == x);
    FastAssert(y == y);
    FastAssert(z == z);
    if (m_usedParamCount < m_maxParamCount)
    {
      ART_COPY_KEY_VALUE_TO_PARAM

      m_params[m_usedParamCount].m_type = kVector3;
      m_params[m_usedParamCount].v.vec[0] = x;
      m_params[m_usedParamCount].v.vec[1] = y;
      m_params[m_usedParamCount].v.vec[2] = z;

      m_usedParamCount ++;
      return true;
    }

    m_parameterOverflow = true;
    return false;
  }
}

