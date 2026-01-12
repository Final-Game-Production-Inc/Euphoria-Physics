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

// NOTE: this file is used to automatically define the appropriate
//        macro functions used to declare, implement and register
//        effector parameters
//
// define NM_RS_EFFECTOR_CLASS_BODY when including in class body declaration
// define NM_RS_EFFECTOR_CLASS_IMPLEMENTATION when including in construct() fn

#ifdef NM_RS_EFFECTOR_PARAMETER
#undef NM_RS_EFFECTOR_PARAMETER
#endif // NM_RS_EFFECTOR_PARAMETER

#ifdef NM_RS_RO_PARAMETER
#undef NM_RS_RO_PARAMETER
#endif // NM_RS_RO_PARAMETER

#ifdef NM_RS_RO_PARAMETER_ACTUAL
#undef NM_RS_RO_PARAMETER_ACTUAL
#endif // NM_RS_RO_PARAMETER_ACTUAL

#ifdef NM_RS_PARAMETER_DIRECT
#undef NM_RS_PARAMETER_DIRECT
#endif

#ifdef NM_RS_PARAMETER
#undef NM_RS_PARAMETER
#endif // NM_RS_PARAMETER

#ifdef NM_RS_PARAMETER_DIRECT
#undef NM_RS_PARAMETER_DIRECT
#endif // NM_RS_PARAMETER_DIRECT

// effector accessors declaration macros
#ifdef NM_RS_EFFECTOR_CLASS_BODY

    // macro used to add get/set accessors for effector values
    // note: get##_l cannot be const, the LuaPlus dispatcher templates don't like const functions much
    #define NM_RS_EFFECTOR_PARAMETER(classname, type, name) \
      public:\
      inline type get##name() const { return m_##name; } \
      inline void set##name(type _v) { m_##name = _v; }\
      protected:\
      type m_##name;

    // read-only version of the above
    #define NM_RS_RO_PARAMETER(classname, type, name) \
      public:\
      inline type get##name() const { return m_##name; } \
      protected:\
      type m_##name;

    // actual angles and velocities are computed on-demand
#define NM_RS_RO_PARAMETER_ACTUAL(classname, type, name) \
      public:\
      inline type get##name() const { if(!state.m_actualAnglesValid) updateCurrentAngles(); return m_##name; } \
      protected:\
      type m_##name;

#if ART_ENABLE_BSPY
#if ART_ENABLE_BSPY_EFFECTOR_SETBY
#define NM_RS_PARAMETER(classname, type, name, _min, _max, _default) \
      public:\
      inline type get##name() const { return m_##name; } \
      inline void set##name(type _v, const char * setBy = 0) {\
        NM_RS_VALIDATE_VAL_WARN(_v, _min, _max, getJointIndex(), setBy ? setBy : s_bvIDNames[currentBehaviour()+1]);\
        if (!isMasked()) {m_##name = _v;\
          if(setBy) {\
            m_##name##SetBy = bSpyServer::inst()->getTokenForString(setBy);\
          }\
          else if(currentSubBehaviour() != 0){\
            char buffer[1024];\
            sprintf(buffer, "nl %s | %s", getBvidNameSafe(currentBehaviour()), currentSubBehaviour());\
            m_##name##SetBy = bSpyServer::inst()->getTokenForString(buffer);\
          }\
          else{\
            char buffer[1024];\
            sprintf(buffer, "nl %s", getBvidNameSafe(currentBehaviour()));\
            m_##name##SetBy = bSpyServer::inst()->getTokenForString(buffer);\
          }\
          m_##name##SetByFrame = currentFrame();\
        }\
      }\
      protected:\
      type m_##name;\
      bSpyStringToken m_##name##SetBy;\
      int m_##name##SetByFrame;
#else
#define NM_RS_PARAMETER(classname, type, name, _min, _max, _default) \
      public:\
      inline type get##name() const { return m_##name; } \
      inline void set##name(type _v, const char * setBy = 0) {\
        setBy = setBy;\
        NM_RS_VALIDATE_VAL_WARN(_v, _min, _max, getJointIndex(), currentBehaviour());\
        m_##name = _v;\
        m_##name##SetBy = BSPY_INVALID_MSGTOKEN;\
        m_##name##SetByFrame = currentFrame();\
      }\
      protected:\
      type m_##name;\
      bSpyStringToken m_##name##SetBy;\
      int m_##name##SetByFrame;


#endif
#else
#define NM_RS_PARAMETER(classname, type, name, _min, _max, _default) \
      public:\
      inline type get##name() const { return m_##name; } \
      inline type get##name##_l() { return m_##name; } \
      inline void set##name(type _v) {\
        NM_RS_VALIDATE_VAL_WARN(_v, _min, _max, 0, "");\
        if (!isMasked())\
          m_##name = _v;\
      }\
      protected:\
      type m_##name;
#define NM_RS_EFFECTOR_PARAMETER_MASKABLE_VALIDATE_WARN(classname, type, name, _min, _max) \
      public:\
      inline type get##name() const { return m_##name; } \
      inline void set##name(type _v) { NM_RS_VALIDATE_VAL_WARN(_v, _min, _max, 0, 0); if (!isMasked()) m_##name = _v; }\
      protected:\
      type m_##name;
#endif

#define NM_RS_PARAMETER_DIRECT(classname, type, name, _min, _max, _default) NM_RS_PARAMETER(classname, type, name, _min, _max, _default)

#endif

// effector accessors exposure to Lua written included class construction
#ifdef NM_RS_EFFECTOR_CLASS_IMPLEMENTATION

    #define NM_RS_EFFECTOR_PARAMETER(classname, type, name) \
      m_luaTable.RegisterObjectDirect("set"#name, (classname*)0, &classname::set##name); \
      m_luaTable.RegisterObjectDirect("get"#name, (classname*)0, &classname::get##name##_l);
    
    #define NM_RS_RO_PARAMETER(classname, type, name) \
      m_luaTable.RegisterObjectDirect("get"#name, (classname*)0, &classname::get##name##_l);

    #define NM_RS_RO_PARAMETER_ACTUAL(classname, type, name) \
      m_luaTable.RegisterObjectDirect("get"#name, (classname*)0, &classname::get##name##_l);

    #define NM_RS_PARAMETER(classname, type, name, _min, _max) \
      m_luaTable.RegisterObjectDirect("set"#name, (classname*)0, &classname::set##name); \
      m_luaTable.RegisterObjectDirect("get"#name, (classname*)0, &classname::get##name##_l);

#endif // NM_RS_EFFECTOR_CLASS_IMPLEMENTATION

