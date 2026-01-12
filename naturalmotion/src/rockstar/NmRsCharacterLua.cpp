/*
 * Copyright (c) 2005-2008 NaturalMotion Ltd. All rights reserved. 
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

#ifndef ART_NO_LUA

#include "NmRsCommon.h"
#include "NmRsInclude.h"
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsUtils.h"
#include "NmRsGenericPart.h"
#include "NmRsLuaTools.h"
#include "NmRsCBU_TaskManager.h"

#include "NMmath/MathUtils.h"


/**
 * NM_DOXYGEN_INFO
 * NAME Agent Rockstar API
 *
 * TITLE
 * If the Rockstar %ART plugin is loaded, it makes these functions and values available to 
 * agents created from assets containing a Rockstar segment. See the \ref agentapi
 * page for more agent APIs.
 */

namespace ART
{
  namespace Rockstar
  {
    /**
     * Register all agent functions into a lua table associated with
     * the given <tt>agent</tt>. 
     */
    ARTlua::LuaObject NmRsCharacter::registerAgentLuaBindings(ART::Agent* agent)
    {
      agent->addEngineTable(RockstarAgentTable, this);
      ARTlua::LuaObject agentTable = agent->getLuaState()->GetGlobal(RockstarAgentTable);
      agentTable.SetLightUserData("NULL", (void *)NULL);

      // used to store per-character configuration, such as which hands are free for use
#ifndef NM_RS_REDUCED_LUA_INTERFACE
      agentTable.CreateTable(RockstarCharConfigName);
#endif // NM_RS_REDUCED_LUA_INTERFACE

#define NM_RS_LUAREG_FN(fn) REGISTER_ENGINE_DIRECT_FUNCTION(m_services, this->m_engine, agent, RockstarAgentTable, #fn, NmRsCharacter::fn##_l)
# include "NmRsCharacterLua.inl"
#undef NM_RS_LUAREG_FN

      return agentTable;
    }
    



    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:stopAllBehaviours()
     * 
     * OVERVIEW
     * halts all behaviours in the CBU task manager, resets the effectors and
     * holds the current pose
     *
     */
    int NmRsCharacter::stopAllBehaviours_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC();
      {
        // stop everything!
        m_cbuTaskManager->deactivateAllTasks(agent->getID());

        // and you!
        resetEffectorsToDefaults();

        // and you and you!
        holdPoseAllEffectors();
        setBodyStiffness(5, 0.5,"fb");
      }
      return 0;
    }


    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:configureUprightConstraint(bool enable, float stiffness, float damping)
     * 
     * OVERVIEW
     * enables or disables the upright constraint applied on character postStep
     * to try and keep the dude on his feet
     *
     */
    int NmRsCharacter::configureUprightConstraint_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(3)
      {
        RS_LUA_GET_BOOL_ARG(0, enable);
        RS_LUA_GET_NUMBER_ARG(1, stiffness);
        RS_LUA_GET_NUMBER_ARG(2, damping);

        configureUprightConstraint(enable, stiffness, damping);
      }
      return 0;
    }

    ////EUPHORIA ONLY    
    //int NmRsCharacter::setDraggedPlayerControls_l(ARTlua::LuaState* state)
    //{
    //  RS_LUA_BEGINFUNC_WITH_ARGS();
    //  RS_LUA_CONFIRM_ARGCOUNT(2)
    //  {
    //    RS_LUA_GET_BOOL_ARG(0, grabLeft);
    //    RS_LUA_GET_BOOL_ARG(1, grabRight);
    //    m_grabRight = grabRight;      
    //    m_grabLeft = grabLeft;        
    //  }
    //  return 0;
    //}

    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:enableIncomingTransforms()
     * 
     * OVERVIEW
     * Enables use of incoming animation data
     *
     * DESCRIPTION
     * When enabled, the engine will attempt to use any
     * provided animation data to drive the physics
     */
    int NmRsCharacter::enableIncomingTransforms_l(ARTlua::LuaState*)
    {
      setIncomingTransformApplyMode(ART::IncomingTransformProcessor::kEnabling);
      return 0;
    }
    
    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:disableIncomingTransforms()
     * 
     * OVERVIEW
     * Disables use of incoming animation data
     *
     * DESCRIPTION
     * If disabled, the engine will ignore any
     * animation data present
     */
    int NmRsCharacter::disableIncomingTransforms_l(ARTlua::LuaState*)
    {
      setIncomingTransformApplyMode(ART::IncomingTransformProcessor::kDisabling);
      return 0;
    }
    
    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:enableSingleIncomingTransform()
     * 
     * OVERVIEW
     * enables a single pose of animation data
     *
     * DESCRIPTION
     * Will enable animation data for a single
     * frame, to set the character into the
     * specified pose driven by the animation
     */
    int NmRsCharacter::enableSingleIncomingTransform_l(ARTlua::LuaState*)
    {
      setIncomingTransformApplyMode(ART::IncomingTransformProcessor::kSingleFrame);
      return 0;
    }

    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:activePose(float stiffness, string mask)
     * 
     * OVERVIEW
     * drives all effectors to the current pose defined by the 
     * incoming transforms
     */
    int NmRsCharacter::activePose_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT_RANGE(0, 1)
      {
        RS_LUA_BEGIN_ARGSBLOCK(0)
        {
          activePoseAllEffectors();
          return 0;
        }

        RS_LUA_GET_STRING_ARG(0, ccMask);
        Assert(strlen(ccMask) == 2);

        callMaskedEffectorFunctionNoArgs(
          this, 
          ccMask, 
          &NmRs1DofEffector::activePose, 
          &NmRs3DofEffector::activePose);
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:holdPose(string mask)
     * 
     * OVERVIEW
     * asks all effectors to hold their current pose
     */
    int NmRsCharacter::holdPose_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT_RANGE(0, 1)
      {
        RS_LUA_BEGIN_ARGSBLOCK(0)
        {
          holdPoseAllEffectors();
          return 0;
        }
        
        RS_LUA_GET_STRING_ARG(0, ccMask);
        Assert(strlen(ccMask) == 2);

        callMaskedEffectorFunctionNoArgs(
          this, 
          ccMask, 
          &NmRs1DofEffector::holdPose, 
          &NmRs3DofEffector::holdPose);
      }
      return 0;
    }


    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:setEffectorRelaxation(float pct, string mask)
     * 
     * OVERVIEW
     * relax effectors, value specified between 0 (normal) and 100 (fully relaxed)
     */
    int NmRsCharacter::setEffectorRelaxation_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT_RANGE(1, 2)
      {
        RS_LUA_GET_NUMBER_ARG(0, relaxPct);
        float mult = (100.0f - relaxPct) / 100.0f;

        RS_LUA_BEGIN_ARGSBLOCK(1)
        {
          relaxAllEffectors(mult);
          return 0;
        }

        RS_LUA_GET_STRING_ARG(1, ccMask);
        Assert(strlen(ccMask) == 2);

        callMaskedEffectorFunctionFloatArg(
          this, 
          ccMask, 
          mult, 
          &NmRs1DofEffector::setRelaxation, 
          &NmRs3DofEffector::setRelaxation);
      }
      return 0; 
    }

    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:setBodyStiffness(float stiffness, float damping, char *parts, float muscleStiffness)
     * 
     * OVERVIEW
     * Sets stiffness of body parts
     *
     * DESCRIPTION
     * acc = w^2 * (desiredAngle - angle) + 2*w*d * (0 - angleVel)
     * First parameter is w, next is d
     * Third parameter is the parts id, and the forth is optional muscleStiffness
     */
    int NmRsCharacter::setBodyStiffness_l(ARTlua::LuaState* state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if(numArgs >= 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        float stiffness = ENGINE_DIRECT_FUNC_GET_ARG(0).GetNumber();
        float dampingScale = ENGINE_DIRECT_FUNC_GET_ARG(1).GetNumber();

        if (numArgs == 2)
          setBodyStiffness(stiffness, dampingScale,"fb");
        else
        {
          const char* ccMask = ENGINE_DIRECT_FUNC_GET_ARG(2).GetString();
          Assert(strlen(ccMask) == 2);
          if (numArgs == 3)
            setBodyStiffness(stiffness, dampingScale, ccMask);
          else
          {
            float muscleStiffness = ENGINE_DIRECT_FUNC_GET_ARG(3).GetNumber();
            setBodyStiffness(stiffness, dampingScale, ccMask, &muscleStiffness);
          }
        }
      }
      return 0;
    }


    int NmRsCharacter::defineAttachedObject_l(ARTlua::LuaState* state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      ENGINE_CREATE_ARGS_OBJECT(state);
      LUA_GET_AGENT_AND_CHARACTER();
      if (numArgs == 3)
      {
        int index = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        NMutils::NMVector3Ptr pos = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData(); 
        rage::Vector3 worldPos(pos[0], pos[1], pos[2]);
        float mass = ENGINE_DIRECT_FUNC_GET_ARG(2).GetFloat();
        m_attachedObject.partIndex = index;
        m_attachedObject.worldPos = worldPos;
        m_attachedObject.mass = mass;
      }
      else
        m_attachedObject.partIndex = -1;
      return 0;
    }

#ifndef NM_RS_REDUCED_LUA_INTERFACE



    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:configureIK(string limbID, object rootPart, object threeDofEffector, object upperPart, object oneDofEffector, object lowerPart, object endPartEffector, object endPart)
     * 
     * OVERVIEW
     * Specifies configuration of 2B-IK chain. LimbID may be "LeftLeg", "RightLeg", "LeftArm" or "RightArm"
     *
     * DESCRIPTION
     * Used to specify the various components of the character to use
     * when calculating IK for various limbs. 
     */
    int NmRsCharacter::configureIK_l(ARTlua::LuaState* state)
    {

      if(m_bodyIdent == gtaFred   ||
        m_bodyIdent == gtaWilma  ||
        m_bodyIdent == rdrCowboy ||
        m_bodyIdent == mp3Medium  ||
        m_bodyIdent == mp3Large)
      {
        
        int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
        if (numArgs == 8 || numArgs == 9)
        {
          ENGINE_CREATE_ARGS_OBJECT(state);
          LUA_GET_AGENT_AND_CHARACTER();

          const char* limbID = ENGINE_DIRECT_FUNC_GET_ARG(0).GetString();

          // part indices
          int rootPartIdx = ENGINE_DIRECT_FUNC_GET_ARG(1).GetInteger();
          int upperPartIdx = ENGINE_DIRECT_FUNC_GET_ARG(3).GetInteger();
          int lowerPartIdx = ENGINE_DIRECT_FUNC_GET_ARG(5).GetInteger();
          int endPartIdx = ENGINE_DIRECT_FUNC_GET_ARG(7).GetInteger();

          // resolve to NmRsGenericPart instances
          NmRsGenericPart* rootPart   = getGenericPartByIndex(rootPartIdx);
          NmRsGenericPart* upperPart  = getGenericPartByIndex(upperPartIdx);
          NmRsGenericPart* lowerPart  = getGenericPartByIndex(lowerPartIdx);
          NmRsGenericPart* endPart    = getGenericPartByIndex(endPartIdx);

          ARTlua::LuaObject threeEffTable = ENGINE_DIRECT_FUNC_GET_ARG(2);
          ARTlua::LuaObject oneEffTable = ENGINE_DIRECT_FUNC_GET_ARG(4);
          ARTlua::LuaObject endEffTable = ENGINE_DIRECT_FUNC_GET_ARG(6);

          NmRsEffectorBase* rootEff = 0;
          if (numArgs == 9)
          {
            ARTlua::LuaObject rootEffector = ENGINE_DIRECT_FUNC_GET_ARG(8);
  #if NM_RS_ENABLE_LOGGING
            if (!rootEffector.IsTable() || !rootEffector["__object"].IsLightUserData())
            {
              m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: table passed for root effector is not valid");
              return 0;
            }
  #endif // NM_RS_ENABLE_LOGGING

            rootEff = (NmRsEffectorBase*)rootEffector["__object"].GetLightUserData();

  #if NM_RS_ENABLE_LOGGING
            if (!rootEff->is3DofEffector())
            {
              m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: root effector not valid");
              return 0;
            }
  #endif // NM_RS_ENABLE_LOGGING
          }

          // check that those tables passed for the effectors are infact valid effector references (eg. from rs.effectors)
  #if NM_RS_ENABLE_LOGGING
          if (!threeEffTable.IsTable() || !threeEffTable["__object"].IsLightUserData())
          {
            m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: table passed for 3dof effector is not valid");
            return 0;
          }
          if (!oneEffTable.IsTable() || !oneEffTable["__object"].IsLightUserData())
          {
            m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: table passed for 1dof effector is not valid");
            return 0;
          }
          if (!endEffTable.IsTable() || !endEffTable["__object"].IsLightUserData())
          {
            m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: table passed for end effector is not valid");
            return 0;
          }
  #endif // NM_RS_ENABLE_LOGGING

          // grab effector instances
          NmRsEffectorBase* threeEff  = (NmRsEffectorBase*)threeEffTable["__object"].GetLightUserData();
          NmRsEffectorBase* oneEff    = (NmRsEffectorBase*)oneEffTable["__object"].GetLightUserData();
          NmRsEffectorBase* endEff    = (NmRsEffectorBase*)endEffTable["__object"].GetLightUserData();

          // type/validity checking in debug
  #if NM_RS_ENABLE_LOGGING
          if (!rootPart)
          {
            m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: could not find root part instance");
            return 0;
          }
          if (!upperPart)
          {
            m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: could not find upper part instance");
            return 0;
          }
          if (!lowerPart)
          {
            m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: could not find lower part instance");
            return 0;
          }
          if (!endPart)
          {
            m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: could not find end part instance");
            return 0;
          }

          if (!threeEff->is3DofEffector())
          {
            m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: 3dof effector not valid");
            return 0;
          }
          if (oneEff->is3DofEffector())
          {
            m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: 1dof effector not valid");
            return 0;
          }
          if (!endEff->is3DofEffector())
          {
            m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: end part effector not valid");
            return 0;
          }
  #endif // NM_RS_ENABLE_LOGGING

          // we use string compares for ID - this function is only called during agent init
          // and will only be called 4 times, one for each limb
          if (!strncmp(limbID, "LeftLeg", 7))
            addHumanLimbIKSetup(LimbIKSetup::kLeftLeg, rootEff, rootPart, threeEff, upperPart, oneEff, lowerPart, endEff, endPart);
          else if (!strncmp(limbID, "RightLeg", 8))
            addHumanLimbIKSetup(LimbIKSetup::kRightLeg, rootEff, rootPart, threeEff, upperPart, oneEff, lowerPart, endEff, endPart);
          else if (!strncmp(limbID, "LeftArm", 7))
            addHumanLimbIKSetup(LimbIKSetup::kLeftArm, rootEff, rootPart, threeEff, upperPart, oneEff, lowerPart, endEff, endPart);
          else if (!strncmp(limbID, "RightArm", 8))
            addHumanLimbIKSetup(LimbIKSetup::kRightArm, rootEff, rootPart, threeEff, upperPart, oneEff, lowerPart, endEff, endPart);
  #if NM_RS_ENABLE_LOGGING
          else
          {
            m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: limbID '%i' not recognised", limbID);
          }
  #endif // NM_RS_ENABLE_LOGGING
        }
  #if NM_RS_ENABLE_LOGGING
        else
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureIK: invalid number of arguments: expected 8, got %i", numArgs);
        }
  #endif // NM_RS_ENABLE_LOGGING

      }

      return 0; 
    }

    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:configureSpine(object pelvisPart, object spine0, object spine0Part, object spine1, , object spine1Part, object spine2, object spine2Part, object spine3, object spine3Part, object neckLower,object neckPart, object neckUpper, object headPart)
     * 
     * OVERVIEW
     * Specifies effectors that make up the characters spine
     *
     */
    int NmRsCharacter::configureSpine_l(ARTlua::LuaState* state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 13)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int pelvisPartIdx = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        ARTlua::LuaObject sp0Table = ENGINE_DIRECT_FUNC_GET_ARG(1);
        int spine0PartIdx = ENGINE_DIRECT_FUNC_GET_ARG(2).GetInteger();
        ARTlua::LuaObject sp1Table = ENGINE_DIRECT_FUNC_GET_ARG(3);
        int spine1PartIdx = ENGINE_DIRECT_FUNC_GET_ARG(4).GetInteger();
        ARTlua::LuaObject sp2Table = ENGINE_DIRECT_FUNC_GET_ARG(5);
        int spine2PartIdx = ENGINE_DIRECT_FUNC_GET_ARG(6).GetInteger();
        ARTlua::LuaObject sp3Table = ENGINE_DIRECT_FUNC_GET_ARG(7);
        int spine3PartIdx = ENGINE_DIRECT_FUNC_GET_ARG(8).GetInteger();
        ARTlua::LuaObject neckLowerTable = ENGINE_DIRECT_FUNC_GET_ARG(9);
        int neckPartIdx = ENGINE_DIRECT_FUNC_GET_ARG(10).GetInteger();
        ARTlua::LuaObject neckUpperTable = ENGINE_DIRECT_FUNC_GET_ARG(11);      
        int headPartIdx = ENGINE_DIRECT_FUNC_GET_ARG(12).GetInteger();

#if NM_RS_ENABLE_LOGGING
        if (!sp0Table.IsTable() || !sp0Table["__object"].IsLightUserData())
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: table passed for spine0 effector is not valid");
          return 0;
        }
        if (!sp1Table.IsTable() || !sp1Table["__object"].IsLightUserData())
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: table passed for spine1 effector is not valid");
          return 0;
        }
        if (!sp2Table.IsTable() || !sp2Table["__object"].IsLightUserData())
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: table passed for spine2 effector is not valid");
          return 0;
        }
        if (!sp3Table.IsTable() || !sp3Table["__object"].IsLightUserData())
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: table passed for spine3 effector is not valid");
          return 0;
        }
        if (!neckLowerTable.IsTable() || !neckLowerTable["__object"].IsLightUserData())
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: table passed for neckLower effector is not valid");
          return 0;
        }
        if (!neckUpperTable.IsTable() || !neckUpperTable["__object"].IsLightUserData())
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: table passed for neckUpper effector is not valid");
          return 0;
        }
#endif
    //    configureSpine(pelvisPartIdx, spine0PartIdx, spine1PartIdx, spine2PartIdx, spine3PartIdx, neckPartIdx, headPartIdx);


        NmRsGenericPart* pelvisPart  = getGenericPartByIndex(pelvisPartIdx);
        NmRsGenericPart* spine0Part  = getGenericPartByIndex(spine0PartIdx);
        NmRsGenericPart* spine1Part  = getGenericPartByIndex(spine1PartIdx);
        NmRsGenericPart* spine2Part  = getGenericPartByIndex(spine2PartIdx);
        NmRsGenericPart* spine3Part  = getGenericPartByIndex(spine3PartIdx);
        NmRsGenericPart* neckPart    = getGenericPartByIndex(neckPartIdx);
        NmRsGenericPart* headPart    = getGenericPartByIndex(headPartIdx);

#if NM_RS_ENABLE_LOGGING
        if (!pelvisPart)
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: could not find pelvis part instance");
          return 0;
        }
        if (!spine0Part)
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: could not find spine0 part instance");
          return 0;
        }
        if (!spine1Part)
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: could not find spine1 part instance");
          return 0;
        }
        if (!spine2Part)
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: could not find spine2 part instance");
          return 0;
        }
        if (!spine3Part)
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: could not find spine3 part instance");
          return 0;
        }
        if (!neckPart)
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: could not find neck part instance");
          return 0;
        }
        if (!headPart)
        {
          m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: could not find head part instance");
          return 0;
        }
#endif // NM_RS_ENABLE_LOGGING

        // check that those tables passed for the effectors are infact valid effector references (eg. from rs.effectors)

        // grab effector instances
        NmRsEffectorBase* sp0Eff  = (NmRsEffectorBase*)sp0Table["__object"].GetLightUserData();
        NmRsEffectorBase* sp1Eff  = (NmRsEffectorBase*)sp1Table["__object"].GetLightUserData();
        NmRsEffectorBase* sp2Eff  = (NmRsEffectorBase*)sp2Table["__object"].GetLightUserData();
        NmRsEffectorBase* sp3Eff  = (NmRsEffectorBase*)sp3Table["__object"].GetLightUserData();
        NmRsEffectorBase* lowNeckEff  = (NmRsEffectorBase*)neckLowerTable["__object"].GetLightUserData();
        NmRsEffectorBase* upNeckEff  = (NmRsEffectorBase*)neckUpperTable["__object"].GetLightUserData();

        // log the choices in the character
        doSpineSetup(pelvisPart, sp0Eff, spine0Part, sp1Eff, spine1Part, sp2Eff, spine2Part, sp3Eff, spine3Part, lowNeckEff, neckPart, upNeckEff, headPart);
      }
      else
      {
#if NM_RS_ENABLE_LOGGING
        m_services->logger->logf(NMutils::BasicLogger::kError, L"configureSpine: expected %i arguments, got %i", 13, numArgs);
#endif
      }
        return 0;
    }


    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:configureCharacter(bool leftHandFree, bool rightHandFree, bool storeZeroPose)
     * 
     * OVERVIEW
     * mimics R* C-API function to config character so we can test that functionality in Studio
     *
     */
    int NmRsCharacter::configureCharacter_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(3)
      {
        RS_LUA_GET_BOOL_ARG(0, leftHandFree);
        RS_LUA_GET_BOOL_ARG(1, rightHandFree);
        RS_LUA_GET_BOOL_ARG(2, storeZeroPose);

        // store into the character
        configureCharacter(leftHandFree, rightHandFree, storeZeroPose);
      }
      return 0;
    }

    /**
    * NMDOCS
    * SECTION parts
    * LUA_FUNCTIONS
    * rs:setPostureData(leftFoot, rightFoot, useZMP, clampLimit, damping, alternateRoot)
    * 
    * OVERVIEW
    * optionally set many posture values
    * leftFoot, rightFoot: integer parts, -2 means automatic, -1 means foot disabled
    * useZMP: do you want gravity comp/posture to push up, or against ZMP
    * clampLimit: how much gravity comp torque can an effector of a given strength provide
    * damping: 0 = no damping .. 0.3 is pretty high
    */
    int NmRsCharacter::setPostureData_l(ARTlua::LuaState* state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs <= 6)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        if (ENGINE_DIRECT_FUNC_GET_ARG(0).IsNumber())
          m_posture.leftFoot = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        if (ENGINE_DIRECT_FUNC_GET_ARG(1).IsNumber())
          m_posture.rightFoot = ENGINE_DIRECT_FUNC_GET_ARG(1).GetInteger();
        if (ENGINE_DIRECT_FUNC_GET_ARG(2).IsBoolean())
          m_posture.useZMP = ENGINE_DIRECT_FUNC_GET_ARG(2).GetBoolean();
        if (ENGINE_DIRECT_FUNC_GET_ARG(3).IsNumber())
          m_posture.clampLimit = ENGINE_DIRECT_FUNC_GET_ARG(3).GetNumber();
        if (ENGINE_DIRECT_FUNC_GET_ARG(4).IsNumber())
          m_posture.damping = ENGINE_DIRECT_FUNC_GET_ARG(4).GetNumber();
        if (ENGINE_DIRECT_FUNC_GET_ARG(5).IsNumber())
          m_posture.alternateRoot = ENGINE_DIRECT_FUNC_GET_ARG(5).GetInteger();
      }
      return 0;
    }

    int NmRsCharacter::setMaxAngSpeedMultiplier_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(1)
      {
        RS_LUA_GET_NUMBER_ARG(0, multiplier);
        initMaxAngSpeed(multiplier);
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:getPostureData()
     * 
     * OVERVIEW
     * returns 5 posture values
     * leftFoot, rightFoot: integer parts, -2 means automatic, -1 means foot disabled
     * useZMP: do you want gravity comp/posture to push up, or against ZMP
     * clampLimit: how much gravity comp torque can an effector of a given strength provide
     * damping: 0 = no damping .. 0.3 is pretty high
     */
    int NmRsCharacter::getPostureData_l(ARTlua::LuaState* state)
    {
      ENGINE_CREATE_ARGS_OBJECT(state);
      LUA_GET_AGENT_AND_CHARACTER();
      state->PushInteger((int)m_posture.leftFoot);
      state->PushInteger((int)m_posture.rightFoot);
      state->PushBoolean(m_posture.useZMP);
      state->PushNumber(m_posture.clampLimit);
      state->PushNumber(m_posture.damping);
      state->PushInteger((int)m_posture.alternateRoot);
      return 6;
    }

    int NmRsCharacter::applyZMPPostureControl_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_NUMBER_ARG(0, footWidth);
        RS_LUA_GET_NUMBER_ARG(1, footLength);

        applyZMPPostureControl(footWidth, footLength);
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:pushEffectorMask(string maskCode)
     * 
     * OVERVIEW
     * using the given mask code, disable the specified effectors from taking 'set' commands
     * essentially globally disabling them. this pushes the mask code into a stack so that these
     * masks can be stacked without having to know about each other.
     *
     */
    int NmRsCharacter::pushEffectorMask_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(1)
      {
        RS_LUA_GET_STRING_ARG(0, ccMask);
        Assert(strlen(ccMask) == 2);

        pushMaskCode(ccMask);
        processEffectorMaskCode(ccMask);
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:popEffectorMask()
     * 
     * OVERVIEW
     * re-enable the effectors that were masked off with the last pushEffectorMask
     *
     */
    int NmRsCharacter::popEffectorMask_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(0)
      {
        popEffectorMask();
      }
      return 0;
    }


    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:storeZeroPose()
     * 
     * OVERVIEW
     * asks all effectors to store the current animation frame as a zero pose
     */
    int NmRsCharacter::storeZeroPose_l(ARTlua::LuaState*)
    {
      storeZeroPoseAllEffectors();
      return 0;
    }



    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:blendToZeroPose(float weight, string mask)
     * 
     * OVERVIEW
     * blends to stored zero poses - 0 being no zero pose influence, 1 being full zero pose blend
     */
    int NmRsCharacter::blendToZeroPose_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT_RANGE(1, 2)
      {
        RS_LUA_GET_NUMBER_ARG(0, zpBlend);

        RS_LUA_BEGIN_ARGSBLOCK(1)
        {
          blendToZeroPoseAllEffectors(zpBlend);
          return 0;
        }

        RS_LUA_GET_STRING_ARG(1, ccMask);
        Assert(strlen(ccMask) == 2);
        
        blendToZeroPose(zpBlend, ccMask);
      }
      return 0; 
    }


    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:resetEffectorCalibrationsToDefaults()
     * 
     * OVERVIEW
     * set all effectors to their original muscle calibration defaults (stiffness, strength, etc)
     */
    int NmRsCharacter::resetEffectorCalibrationsToDefaults_l(ARTlua::LuaState*)
    {
      recalibrateAllEffectors();
      return 0;
    }

    /**
     * NMDOCS
     * SECTION general
     * LUA_FUNCTIONS
     * rs:resetEffectorsToDefaults(string mask)
     * 
     * OVERVIEW
     * reset all effector desired angles / leans / twists to 0
     */
    int NmRsCharacter::resetEffectorsToDefaults_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT_RANGE(0, 1)
      {
        RS_LUA_BEGIN_ARGSBLOCK(0)
        {
          resetAllEffectors();
          return 0;
        }

        RS_LUA_GET_STRING_ARG(0, ccMask);
        Assert(strlen(ccMask) == 2);

        resetEffectorsToDefaults(ccMask);
      }
      return 0;
    }

    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * int rs:getPartCount()
     *
     * OVERVIEW
     * Returns number of parts the agent contains
     *
     */
    int NmRsCharacter::getPartCount_l(ARTlua::LuaState* state)
    {
      state->PushInteger(getNumberOfParts());
      return 1;
    }
    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * int rs:getJointCount()
     *
     * OVERVIEW
     * Returns number of joints the agent contains
     *
     */
    int NmRsCharacter::getJointCount_l(ARTlua::LuaState* state)
    {
      state->PushInteger(getArticulatedWrapper()->getNumberOfJoints());
      return 1;
    }
    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * int rs:getPartMass(int index)
     *
     * OVERVIEW
     * Return the mass for the part
     * 
     * DESCRIPTION
     * First parameter is part index (integer, zero-based index).
     *
     */
    int NmRsCharacter::getPartMass_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(1)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        float mass = getPartMass(getArticulatedBody()->GetLink(partIndex));
        state->PushNumber(mass);

        return 1;
      }
    }
    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:getPartTM(int index, matrix4 tm)
     * 
     * OVERVIEW
     * Return the current transform for the part
     * 
     * DESCRIPTION
     * First parameter is part index (integer, zero-based index).
     *
     */
    int NmRsCharacter::getPartTM_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        NmRsGenericPart* part = getGenericPartByIndex(partIndex);

        RS_LUA_GET_MAT4_ARG(1, mat);

        rage::Matrix34 boundMatrix;
        part->getBoundMatrix(&boundMatrix);
        NMutils::NMVector4Set(mat[0], boundMatrix.a.x, boundMatrix.a.y, boundMatrix.a.z, 0.0f);
        NMutils::NMVector4Set(mat[1], boundMatrix.b.x, boundMatrix.b.y, boundMatrix.b.z, 0.0f);
        NMutils::NMVector4Set(mat[2], boundMatrix.c.x, boundMatrix.c.y, boundMatrix.c.z, 0.0f);
        NMutils::NMVector4Set(mat[3], boundMatrix.d.x, boundMatrix.d.y, boundMatrix.d.z, 1.0f); 
      }
      return 0;
    }

    int NmRsCharacter::getPartToBoneTM_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        NmRsGenericPart* part = getGenericPartByIndex(partIndex);

        RS_LUA_GET_MAT4_ARG(1, mat);

        mat[0][0] = part->m_toBoneMatrix.a.x; mat[0][1] = part->m_toBoneMatrix.a.y; mat[0][2] = part->m_toBoneMatrix.a.z;mat[0][3] = 0.f;
        mat[1][0] = part->m_toBoneMatrix.b.x; mat[1][1] = part->m_toBoneMatrix.b.y; mat[1][2] = part->m_toBoneMatrix.b.z;mat[1][3] = 0.f;
        mat[2][0] = part->m_toBoneMatrix.c.x; mat[2][1] = part->m_toBoneMatrix.c.y; mat[2][2] = part->m_toBoneMatrix.c.z;mat[2][3] = 0.f;
        mat[3][0] = part->m_toBoneMatrix.d.x; mat[3][1] = part->m_toBoneMatrix.d.y; mat[3][2] = part->m_toBoneMatrix.d.z;mat[3][3] = 1.f;
      }
      return 0;
    }
    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * [float,float,float] rs:getPartPosition(int index)
     * rs:getPartPosition(int index, vector3 position)
     *
     * OVERVIEW
     * Return the current position for the part
     * 
     * DESCRIPTION
     * Either as 3 return values or into a specified vector
     * First parameter is part index (integer, zero-based index).
     * Second optional parameter is the returned position
     */
    int NmRsCharacter::getPartPosition_l(ARTlua::LuaState* state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if(numArgs == 1 || numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int partIdx = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        if(partIdx < getNumberOfParts())
        {
          NmRsGenericPart* part = getGenericPartByIndex(partIdx);
          
          NMutils::NMVector3 partPos;
          part->getPosition(partPos);
    
          if(numArgs == 1)
          {
            state->PushNumber(partPos[0]);
            state->PushNumber(partPos[1]);
            state->PushNumber(partPos[2]);
            return 3;
          }
          else
          {      
            NMutils::NMVector3Ptr posVec = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData();
            if(agent->isVector3(posVec))
            {
              posVec[0] = partPos[0];
              posVec[1] = partPos[1];
              posVec[2] = partPos[2];
            }
          }
        }
      }
      return 0; 
    }
    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:getPartVelocity(int index, vector3 velocity, vector3 position (optional))
     * 
     * OVERVIEW
     * Return the current linear velocity for the part, optionally at a specified world space point
     * 
     * DESCRIPTION
     * First parameter is part index (integer, zero-based index).
     * Second parameter is the returned vector velocity
     */
    int NmRsCharacter::getPartVelocity_l(ARTlua::LuaState* state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if(numArgs == 2 || numArgs == 3)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int partIdx = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        if(partIdx < getNumberOfParts())
        {
          NmRsGenericPart* part = getGenericPartByIndex(partIdx);
          rage::Vector3 partVel;
          if (numArgs == 3)
          {
            NMutils::NMVector3Ptr vec = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(2).GetLightUserData();
            rage::Vector3 point(vec[0], vec[1], vec[2]);
            partVel = part->getLinearVelocity(&point);
          }
          else
            partVel = part->getLinearVelocity();
    
          NMutils::NMVector3Ptr vec = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData();
          if(agent->isVector3(vec)) 
          {
            vec[0] = partVel.x;
            vec[1] = partVel.y;
            vec[2] = partVel.z;
          }
        }
      }
      return 0;
    }
    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:getPartAngularVelocity(int index, vector3 angVelocity)
     * 
     * OVERVIEW
     * Return the current angular velocity for the part
     * 
     * DESCRIPTION
     * First parameter is part index (integer, zero-based index).
     * Second parameter is the returned vector velocity
     */
    int NmRsCharacter::getPartAngularVelocity_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        NmRsGenericPart* part = getGenericPartByIndex(partIndex);
  
        NMutils::NMVector3 partVel;
        part->getAngularVelocity(partVel);
  
        RS_LUA_GET_VEC3_ARG(1, vec);

        vec[0] = partVel[0];
        vec[1] = partVel[1];
        vec[2] = partVel[2];
      }
      return 0;
    }

    
     /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:getLegLength()
     * 
     * OVERVIEW
     * gets the vertical distance from foot to pelvis in the t-pose
     */
    int NmRsCharacter::getLegLength_l(ARTlua::LuaState* state)
    {
      Assert(getLeftLegSetup()); // must be called after IK setup is called
      
      rage::Vector3 left = getLeftLegSetup()->getFoot()->getInitialMatrix().d;
      rage::Vector3 right = getRightLegSetup()->getFoot()->getInitialMatrix().d;
      rage::Vector3 pelvis = getSpineSetup()->getPelvisPart()->getInitialMatrix().d;
      float height = (pelvis - (left+right)*0.5f).Dot(getUpVector());
      state->PushNumber(height);
      return 1;
    }
    int NmRsCharacter::measureCharacter_l(ARTlua::LuaState* state)
    {
      Assert(getLeftLegSetup()); // must be called after IK setup is called

      rage::Matrix34 leftFootMat, rightFootMat, pelvisMat, headMat, thighLeftMat, thighRightMat;
      getLeftLegSetup()->getFoot()->getBoundMatrix(&leftFootMat);
      getRightLegSetup()->getFoot()->getBoundMatrix(&rightFootMat);
      getSpineSetup()->getPelvisPart()->getBoundMatrix(&pelvisMat);
      getSpineSetup()->getHeadPart()->getBoundMatrix(&headMat);
      getLeftLegSetup()->getThigh()->getBoundMatrix(&thighLeftMat);
      getRightLegSetup()->getThigh()->getBoundMatrix(&thighRightMat);


      float legSeparation, legStraightness, charlieChapliness, hipYaw, headYaw, defaultHipPitch;
      measureCharacter(leftFootMat, rightFootMat, pelvisMat, headMat, thighLeftMat, thighRightMat,
        &legSeparation, &legStraightness, &charlieChapliness, &hipYaw, &headYaw, &defaultHipPitch);

      state->PushNumber(legSeparation);
      state->PushNumber(legStraightness);
      state->PushNumber(charlieChapliness);
      state->PushNumber(hipYaw);
      state->PushNumber(headYaw);
      return 5;
    }

    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:addImpulseToPart(int index, float impulseX, float impulseY, float impulseZ)
     * rs:addImpulseToPart(int index, float impulseX, float impulseY, float impulseZ, float atLocationX, float atLocationY, float atLocationZ)
     * rs:addImpulseToPart(int index, vector impulse)
     * rs:addImpulseToPart(int index, vector impulse, vector atLocationX)
     * 
     * OVERVIEW
     * Applies an impulse to a part.
     *
     * DESCRIPTION
     * First parameter is part index (integer, zero-based index).
     * Next three parameters are impulse vector (x,y,z).
     * Optionally, a further three parameters specify the impulse location vector (x,y,z).
     * Alternatively vec3s can be used in place of the sets of floats.
     */
    int NmRsCharacter::addImpulseToPart_l(ARTlua::LuaState* state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if(numArgs == 4 || numArgs == 7)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int partIdx = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        if(partIdx < getNumberOfParts())
        {
          NmRsGenericPart* part = getGenericPartByIndex(partIdx);

          NMutils::NMVector3 imp, loc;
          NMutils::NMVector3Set(imp, ENGINE_DIRECT_FUNC_GET_ARG(1).GetFloat(), ENGINE_DIRECT_FUNC_GET_ARG(2).GetFloat(), ENGINE_DIRECT_FUNC_GET_ARG(3).GetFloat());

          if(numArgs == 4)
            part->getPosition(loc);
          else
            NMutils::NMVector3Set(loc, ENGINE_DIRECT_FUNC_GET_ARG(4).GetFloat(), ENGINE_DIRECT_FUNC_GET_ARG(5).GetFloat(), ENGINE_DIRECT_FUNC_GET_ARG(6).GetFloat());

          part->applyImpulse(imp, loc);
        }
      }
      else if(numArgs == 2 || numArgs == 3)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int partIdx = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        if(partIdx < getNumberOfParts())
        {
          NmRsGenericPart* part = getGenericPartByIndex(partIdx);

          NMutils::NMVector3Ptr vec = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData();
          if(agent->isVector3(vec))
          {
            NMutils::NMVector3 imp, loc;
            NMutils::NMVector3Set(imp, vec[0],vec[1],vec[2]);

            if(numArgs == 2)
            {
              part->getPosition(loc);
              part->applyImpulse(imp, loc);
            }
            else
            {
              NMutils::NMVector3Ptr pos = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(2).GetLightUserData();
              if(agent->isVector3(pos))
              {
                NMutils::NMVector3Set(loc, pos[0],pos[1],pos[2]);
                part->applyImpulse(imp, loc);
              }
            }
          }
        }
      }

      return 0;
    }

    // TDL this should somehow just call the non-immediate version with FUNC_GET_ARGS calls. It should be identical otherwise
    int NmRsCharacter::addImpulseToPartImmediate_l(ARTlua::LuaState* state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if(numArgs == 4 || numArgs == 7)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int partIdx = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        if(partIdx < getNumberOfParts())
        {
          NmRsGenericPart* part = getGenericPartByIndex(partIdx);

          NMutils::NMVector3 imp, loc;
          NMutils::NMVector3Set(imp, ENGINE_DIRECT_FUNC_GET_ARG(1).GetFloat(), ENGINE_DIRECT_FUNC_GET_ARG(2).GetFloat(), ENGINE_DIRECT_FUNC_GET_ARG(3).GetFloat());

          if(numArgs == 4)
            part->getPosition(loc);
          else
            NMutils::NMVector3Set(loc, ENGINE_DIRECT_FUNC_GET_ARG(4).GetFloat(), ENGINE_DIRECT_FUNC_GET_ARG(5).GetFloat(), ENGINE_DIRECT_FUNC_GET_ARG(6).GetFloat());

          part->applyImpulse(imp, loc);
        }
      }
      else if(numArgs == 2 || numArgs == 3)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int partIdx = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        if(partIdx < getNumberOfParts())
        {
          NmRsGenericPart* part = getGenericPartByIndex(partIdx);

          NMutils::NMVector3Ptr vec = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData();
          if(agent->isVector3(vec))
          {
            NMutils::NMVector3 imp, loc;
            NMutils::NMVector3Set(imp, vec[0],vec[1],vec[2]);

            if(numArgs == 2)
            {
              part->getPosition(loc);
              part->applyImpulse(imp, loc);
            }
            else
            {
              NMutils::NMVector3Ptr pos = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(2).GetLightUserData();
              if(agent->isVector3(pos))
              {
                NMutils::NMVector3Set(loc, pos[0],pos[1],pos[2]);
                part->applyImpulse(imp, loc);
              }
            }
          }
        }
      }

      return 0;
    }


    int NmRsCharacter::calculateInertias_l(ARTlua::LuaState*)
    {
      m_calculateInertias = true;
      return 0;
    }

    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * bool rs:hasCollidedWithWorld(char mask)
     * 
     * OVERVIEW
     * have the parts indicated by the mask collided with the world?
     *
     */
    int NmRsCharacter::hasCollidedWithWorld_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(1)
      {
        RS_LUA_GET_STRING_ARG(0, ccMask);

        Assert(strlen(ccMask) == 2);
        state->PushBoolean(hasCollidedWithWorld(ccMask));
        return 1;
      }
    }

    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * bool rs:hasPartCollidedWithWorld(int partIndex, [bool includeSelfCollision])
     * 
     * OVERVIEW
     * has the part collided with the world? set includeSelfCollision to include collisions with 
     * the part's own character; otherwise this returns true if the part has hit something that
     * is not the part's own character.
     *
     */
    int NmRsCharacter::hasPartCollidedWithWorld_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT_RANGE(1, 2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        bool includeSelfCollision = false;
        RS_LUA_BEGIN_ARGSBLOCK(2)
        {
          RS_LUA_GET_BOOL_ARG(1, selfCol);
          includeSelfCollision = selfCol;
        }
 
        if (includeSelfCollision)
          state->PushBoolean(getGenericPartByIndex(partIndex)->collided());
        else
          state->PushBoolean(getGenericPartByIndex(partIndex)->collidedWithNotOwnCharacter());

        return 1;
      }
    }

    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * type = OwnCharacter=0, OtherCharacters=1, Environment=2, NotOwnCharacter=3
     * float rs:getPartCollisionZMP(int type, int partIndex, vector3 outPos, vector3 outNormal)
     * 
     * OVERVIEW
     * can also use  =>  x, y, z, nX, nY, nZ, d = rs:getPartCollisionZMP(int type, int partIndex)
     * gets the averaged collision position & normal for a part, or 0 0 0 if it has not collided
     *
     */
    int NmRsCharacter::getPartCollisionZMP_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_BEGIN_ARGSBLOCK(4)
      {
        RS_LUA_GET_INTEGER_ARG(0, type);
        RS_LUA_GET_INTEGER_ARG(1, partIndex);
        RS_LUA_GET_VEC3_ARG(2, outPos);
        RS_LUA_GET_VEC3_ARG(3, outNormal);

        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        float zmpDepth = 0;
        rage::Vector3 zmpPos, zmpNormal;
        switch (type)
        {
        case 0:
          getGenericPartByIndex(partIndex)->getCollisionZMPWithOwnCharacter(zmpPos, zmpNormal, &zmpDepth);
          break;
        case 1:
          getGenericPartByIndex(partIndex)->getCollisionZMPWithOtherCharacters(zmpPos, zmpNormal, &zmpDepth);
          break;
        case 2:
          getGenericPartByIndex(partIndex)->getCollisionZMPWithEnvironment(zmpPos, zmpNormal, &zmpDepth);
          break;
        case 3:
          getGenericPartByIndex(partIndex)->getCollisionZMPWithNotOwnCharacter(zmpPos, zmpNormal, &zmpDepth);
          break;
        default:
          Assert(false);
        }

        NMutils::NMVector3Set(outPos, zmpPos.x, zmpPos.y, zmpPos.z);
        NMutils::NMVector3Set(outNormal, zmpNormal.x, zmpNormal.y, zmpNormal.z);

        state->PushNumber(zmpDepth);

        return 1;
      }
      else RS_LUA_BEGIN_ARGSBLOCK(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, type);
        RS_LUA_GET_INTEGER_ARG(1, partIndex);

        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        float zmpDepth = 0;
        rage::Vector3 zmpPos, zmpNormal;
        switch (type)
        {
        case 0:
          getGenericPartByIndex(partIndex)->getCollisionZMPWithOwnCharacter(zmpPos, zmpNormal, &zmpDepth);
          break;
        case 1:
          getGenericPartByIndex(partIndex)->getCollisionZMPWithOtherCharacters(zmpPos, zmpNormal, &zmpDepth);
          break;
        case 2:
          getGenericPartByIndex(partIndex)->getCollisionZMPWithEnvironment(zmpPos, zmpNormal, &zmpDepth);
          break;
        case 3:
          getGenericPartByIndex(partIndex)->getCollisionZMPWithNotOwnCharacter(zmpPos, zmpNormal, &zmpDepth);
          break;
        default:
          Assert(false);
        }

        state->PushNumber(zmpPos.x);
        state->PushNumber(zmpPos.y);
        state->PushNumber(zmpPos.z);

        state->PushNumber(zmpNormal.x);
        state->PushNumber(zmpNormal.y);
        state->PushNumber(zmpNormal.z);

        state->PushNumber(zmpDepth);

        return 7;
      }
      else
      {
        RS_LUA_GENERIC_ARG_MISMATCH();
      }
    }

    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * float rs:getKineticEnergyPerKilo()
     * 
     * OVERVIEW
     * returns the kinetic energy per kilo!
     *
     */
    int NmRsCharacter::getKineticEnergyPerKilo_l(ARTlua::LuaState* state)
    {
      state->PushNumber(getKineticEnergyPerKilo_RelativeVelocity());
      return 1;
    }
    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:addTorqueToPart(int index, vector3 torque)
     * 
     * OVERVIEW
     * adds this additional torque to the part centre of mass
     *
     * DESCRIPTION
     * First parameter is part index 
     * Second parameter is the torque vector
     *
     */
    int NmRsCharacter::addTorqueToPart_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        RS_LUA_GET_VEC3_ARG(1, vec);

        NmRsGenericPart* part = getGenericPartByIndex(partIndex);
        rage::Vector3 torque(vec[0], vec[1], vec[2]);
        part->applyTorque(torque);
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:addForceToPart(int index, vector3 force)
     * 
     * OVERVIEW
     * adds this additional force to the part centre of mass
     *
     * DESCRIPTION
     * First parameter is part index 
     * Second parameter is the force vector
     *
     */
    int NmRsCharacter::addForceToPart_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT_RANGE(2, 3)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        NmRsGenericPart* part = getGenericPartByIndex(partIndex);

        RS_LUA_GET_VEC3_ARG(1, vec);

        rage::Vector3 *position = NULL;
        rage::Vector3 pos;
        RS_LUA_BEGIN_ARGSBLOCK(3)
        {
          RS_LUA_GET_VEC3_ARG(2, p);

          pos.Set(p[0],p[1],p[2]);
          position = &pos;
        }

        rage::Vector3 force(vec[0], vec[1], vec[2]);
        part->applyForce(force, position);
      }
      return 0;
    }

    int NmRsCharacter::addDeltaVelToPart_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT_RANGE(2, 3)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        NmRsGenericPart* part = getGenericPartByIndex(partIndex);
        rage::Vector3 posWorld = part->getPosition();

        RS_LUA_GET_VEC3_ARG(1, vec);

        RS_LUA_BEGIN_ARGSBLOCK_GTE(3)
        {
          RS_LUA_GET_VEC3_ARG(2, position);
          posWorld.Set(position[0], position[1], position[2]);
        }

        {
          rage::Vector3 impulse(vec[0], vec[1], vec[2]);
          rage::Vector3 offset = getArticulatedWrapper()->getArticulatedCollider()->GetMatrix().d;
          rage::Vector3 pos = posWorld - offset;
          float speed = impulse.Mag();
          impulse.Normalize();
          getArticulatedBody()->PrecalcResponsesToForce(partIndex, impulse, pos);
          rage::Vector3 vel;
          getArticulatedBody()->GetLinkResponse(partIndex, pos, &vel);
          impulse *= speed / (vel.Mag() + 0.0001f);
          part->applyImpulse(impulse, posWorld);
        }
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:setPartTMImmediate(int index, matrix4 tm)
     * 
     * OVERVIEW
     * Sets a parts transform immediately. This is like a teleport, no collision sweep to new position
     *
     * DESCRIPTION
     * First parameter is part index (integer, zero-based index).
     * Second parameter is the new transform matrix
     *
     */
    int NmRsCharacter::setPartTMImmediate_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        RS_LUA_GET_MAT4_ARG(1, mat);

        getGenericPartByIndex(partIndex)->teleportMatrix(*(NMutils::NMMatrix4*)mat);
      }
      return 0;
    }
    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:setPartTM(int index, matrix4 tm)
     * 
     * OVERVIEW
     * This function is supposed to move the object and give it the velocity expected from the shift... still in development
     *
     * DESCRIPTION
     * First parameter is part index (integer, zero-based index).
     * Second parameter is the new transform matrix
     *
     */
    int NmRsCharacter::setPartTM_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        RS_LUA_GET_MAT4_ARG(1, mat);

        NmRsGenericPart* part = getGenericPartByIndex(partIndex);
        NMutils::NMMatrix4 oldMat;
        part->getMatrix(oldMat);
        part->setMatrix(*(NMutils::NMMatrix4*)mat);

        rage::Vector3 zero(0,0,0);
        part->setVelocities(zero, zero, false);
      }
      return 0;
    }


    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * [float,float,float] rs:getCOMpos()
     * rs:getCOMpos(vector3 CoM)
     * 
     * OVERVIEW
     * Return the calculated centre of mass for the whole character
     *
     * DESCRIPTION
     * returns 3 values or into a specified vector
     *
     */
    int NmRsCharacter::getCOMpos_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT_RANGE(0, 1)

      RS_LUA_BEGIN_ARGSBLOCK(0)
      {
        state->PushNumber(m_COM.x);
        state->PushNumber(m_COM.y);
        state->PushNumber(m_COM.z);
    
        return 3;
      }
      else RS_LUA_BEGIN_ARGSBLOCK(1)
      {
        RS_LUA_GET_VEC3_ARG(0, lvec);

        lvec[0] = m_COM.x;
        lvec[1] = m_COM.y;
        lvec[2] = m_COM.z;

        return 0;
      }

      RS_LUA_GENERIC_ARG_MISMATCH();
    }
    
    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * [float,float,float] rs:getCOMvel()
     * rs:getCOMvel(vector3 CoMV)
     * 
     * OVERVIEW
     * Return the calculated centre of mass velocity for the whole character
     *
     * DESCRIPTION
     * returns 3 values or into a specified vector
     *
     */
    int NmRsCharacter::getCOMvel_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT_RANGE(0, 1)

      RS_LUA_BEGIN_ARGSBLOCK(0)
      {
        state->PushNumber(m_COMvel.x);
        state->PushNumber(m_COMvel.y);
        state->PushNumber(m_COMvel.z);
    
        return 3;
      }
      else RS_LUA_BEGIN_ARGSBLOCK(1)
      {
        RS_LUA_GET_VEC3_ARG(0, lvec);

        lvec[0] = m_COMvel.x;
        lvec[1] = m_COMvel.y;
        lvec[2] = m_COMvel.z;

        return 0;
      }

      RS_LUA_GENERIC_ARG_MISMATCH();
    }
    
    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * [float,float,float] rs:getCOMrotvel()
     * rs:getCOMrotvel(vector3 CoMV)
     * 
     * OVERVIEW
     * Return the averaged rotational velocity for the whole character
     *
     * DESCRIPTION
     * returns 3 values or into a specified vector
     */
    int NmRsCharacter::getCOMrotvel_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT_RANGE(0, 1)

      RS_LUA_BEGIN_ARGSBLOCK(0)
      {
        state->PushNumber(m_COMrotvel.x);
        state->PushNumber(m_COMrotvel.y);
        state->PushNumber(m_COMrotvel.z);

        return 3;
      }
      else RS_LUA_BEGIN_ARGSBLOCK(1)
      {
        RS_LUA_GET_VEC3_ARG(0, lvec);

        lvec[0] = m_COMrotvel.x;
        lvec[1] = m_COMrotvel.y;
        lvec[2] = m_COMrotvel.z;

        return 0;
      }

      RS_LUA_GENERIC_ARG_MISMATCH();
    }
    
    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * rs:getCOMTM(matrix4 COMTM)
     * 
     * OVERVIEW
     * Return the averaged rotation matrix (position is centre of mass)
     *
     * DESCRIPTION
     * returns into a specified matrix
     */
    int NmRsCharacter::getCOMTM_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(1)
      {
        RS_LUA_GET_MAT4_ARG(0, mat);

        NMutils::NMMatrix4MakeIdentity(mat);
        rage::Matrix34 matrix = m_COMTM;

        // TDL need a cleaner way to copy to a NMMatrix4
        NMutils::NMVector3Set(mat[0], matrix.GetVector(0).x, matrix.GetVector(0).y, matrix.GetVector(0).z);
        NMutils::NMVector3Set(mat[1], matrix.GetVector(1).x, matrix.GetVector(1).y, matrix.GetVector(1).z);
        NMutils::NMVector3Set(mat[2], matrix.GetVector(2).x, matrix.GetVector(2).y, matrix.GetVector(2).z);
        NMutils::NMVector3Set(mat[3], matrix.GetVector(3).x, matrix.GetVector(3).y, matrix.GetVector(3).z);
      }
      return 0;
    }
    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * rs:getBoneTM(int partIndex, matrix4 boneTM)
     * 
     * OVERVIEW
     * Return the bone's transform for a part
     *
     * DESCRIPTION
     * returns into a specified matrix
     */
    int NmRsCharacter::getBoneTM_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        int partIndex = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        NMutils::NMMatrix4Ptr outMat = (NMutils::NMMatrix4Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData();
        if(agent->isMatrix4(outMat))
        {
          NMutils::NMMatrix4 mat;
          NMutils::NMMatrix4MakeIdentity(mat);
          NmRsGenericPart* part = getGenericPartByIndex(partIndex);
          part->getMatrix(mat);
          rage::Matrix34 matrix;
          matrix.a = rage::Vector3(mat[0][0], mat[0][1], mat[0][2]);
          matrix.b = rage::Vector3(mat[1][0], mat[1][1], mat[1][2]);
          matrix.c = rage::Vector3(mat[2][0], mat[2][1], mat[2][2]);
          matrix.d = rage::Vector3(mat[3][0], mat[3][1], mat[3][2]);
          rage::Matrix34 mat2 = matrix;
   /*       mat2.FastInverse();
          mat2.Transform3x3(mat2.d);
          mat2.d *= -1.f);*/
          rage::Matrix34 invToBone = part->m_toBoneMatrix;
          invToBone.Inverse3x3();
          invToBone.d *= -1.f;
          matrix.Dot(part->m_toBoneMatrix, mat2);
          NMutils::NMVector3Set(outMat[0], matrix.a.x, matrix.a.y, matrix.a.z);
          NMutils::NMVector3Set(outMat[1], matrix.b.x, matrix.b.y, matrix.b.z);
          NMutils::NMVector3Set(outMat[2], matrix.c.x, matrix.c.y, matrix.c.z);
          NMutils::NMVector3Set(outMat[3], matrix.d.x, matrix.d.y, matrix.d.z);
        }
      }
      return 0;
    }
    
    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * rs:getInstanceTM(matrix4 instanceTM)
     * 
     * OVERVIEW
     * Return the matrix of the instance (used by the game)
     *
     * DESCRIPTION
     * returns into a specified matrix
     */
    int NmRsCharacter::getInstanceTM_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(1)
      {
        RS_LUA_GET_MAT4_ARG(0, mat);

        NMutils::NMMatrix4MakeIdentity(mat);
        rage::Matrix34(matrix);
        getInstanceTM(&matrix);

        // TDL need a cleaner way to copy to a NMMatrix4
        NMutils::NMVector3Set(mat[0], matrix.GetVector(0).x, matrix.GetVector(0).y, matrix.GetVector(0).z);
        NMutils::NMVector3Set(mat[1], matrix.GetVector(1).x, matrix.GetVector(1).y, matrix.GetVector(1).z);
        NMutils::NMVector3Set(mat[2], matrix.GetVector(2).x, matrix.GetVector(2).y, matrix.GetVector(2).z);
        NMutils::NMVector3Set(mat[3], matrix.GetVector(3).x, matrix.GetVector(3).y, matrix.GetVector(3).z);
      }
      return 0;
    }
    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:getJointMatrix1(int index, matrix4 tm)
     * 
     * OVERVIEW
     * Return the current transform for the joint 
     * 
     * DESCRIPTION
     * First parameter is joint index (integer, zero-based index).
     */
    int NmRsCharacter::getJointMatrix1_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        RS_LUA_GET_MAT4_ARG(1, mat);

        rage::Matrix34 matrix;
        getEffector(partIndex)->getMatrix1(matrix);
        NMutils::NMMatrix4MakeIdentity(mat);

        mat[0][0] = matrix.a.x; mat[0][1] = matrix.a.y; mat[0][2] = matrix.a.z; 
        mat[1][0] = matrix.b.x; mat[1][1] = matrix.b.y; mat[1][2] = matrix.b.z; 
        mat[2][0] = matrix.c.x; mat[2][1] = matrix.c.y; mat[2][2] = matrix.c.z; 
        mat[3][0] = matrix.d.x; mat[3][1] = matrix.d.y; mat[3][2] = matrix.d.z; 
      }
      return 0;
    }
    /**
    * NMDOCS
    * SECTION parts
    * LUA_FUNCTIONS
    * rs:getJointMatrix2(int index, matrix4 tm)
    * 
    * OVERVIEW
    * Return the current transform for the joint that connects the part to its child
    * 
    * DESCRIPTION
    * First parameter is joint index (integer, zero-based index).
    */
    int NmRsCharacter::getJointMatrix2_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        RS_LUA_GET_MAT4_ARG(1, mat);

        rage::Matrix34 matrix;
        getEffector(partIndex)->getMatrix2(matrix);
        NMutils::NMMatrix4MakeIdentity(mat);

        mat[0][0] = matrix.a.x; mat[0][1] = matrix.a.y; mat[0][2] = matrix.a.z; 
        mat[1][0] = matrix.b.x; mat[1][1] = matrix.b.y; mat[1][2] = matrix.b.z; 
        mat[2][0] = matrix.c.x; mat[2][1] = matrix.c.y; mat[2][2] = matrix.c.z; 
        mat[3][0] = matrix.d.x; mat[3][1] = matrix.d.y; mat[3][2] = matrix.d.z; 
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:getJointPosition(int index, vector3 pos)
     * 
     * OVERVIEW
     * Return the current position for the specified joint (pass in a joint)
     * 
     * DESCRIPTION
     * First parameter is joint index (integer, zero-based index).
     */
    int NmRsCharacter::getJointPosition_l(ARTlua::LuaState* state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        RS_LUA_GET_VEC3_ARG(1, point);

        rage::Vector3 pos = getEffector(partIndex)->getJointPosition();
        point[0] = pos.x;
        point[1] = pos.y;
        point[2] = pos.z;
      }
      return 0;
    }

     /**
    * NMDOCS
    * SECTION parts
    * LUA_FUNCTIONS
    * rs:getJointQuaternionFromIncomingTransform(int index, vector4 q)
    * 
    * OVERVIEW
    * Return the quaternion for the incoming transform for the joint
    * 
    * DESCRIPTION
    * First parameter is joint index (integer, zero-based index)
    */
    int NmRsCharacter::getJointQuaternionFromIncomingTransform_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        RS_LUA_GET_VEC4_ARG(1, q);

        bool success = getEffector(partIndex)->getJointQuaternionFromIncomingTransform(q);
        state->PushBoolean(success);

        return 1;
      }
    }

    /**
    * NMDOCS
    * SECTION parts
    * LUA_FUNCTIONS
    * rs:setPostureSupportJointRange(int minIndex, int maxIndex)
    * 
    * OVERVIEW
    * This defines whether the legs are supporting or in the air (it affects the gravity comp)
    * 
    * DESCRIPTION
    * minIndex is lowest index of supporting limbs, max is the highest index
    */
    int NmRsCharacter::setPostureSupportJointRange_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, minIdx);
        RS_LUA_GET_INTEGER_ARG(1, maxIdx);

        m_posture.setSupportJointRange(minIdx, maxIdx);
      }
      return 0;
    }

    int NmRsCharacter::getPostureJointDelta_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(1)
      {
        RS_LUA_GET_INTEGER_ARG(0, jointIndex);

        rage::Vector3 j = getEffector(jointIndex)->getCurrentDelta();
        state->PushNumber(j.x);
        state->PushNumber(j.y);
        state->PushNumber(j.z);
      }
      return 3;
    }

    int NmRsCharacter::setPostureJointDelta_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(4)
      {
        RS_LUA_GET_INTEGER_ARG(0, jointIndex);

        RS_LUA_GET_NUMBER_ARG(1, x);
        RS_LUA_GET_NUMBER_ARG(2, y);
        RS_LUA_GET_NUMBER_ARG(3, z);

        getEffector(jointIndex)->setPostureDelta(rage::Vector3(x,y,z));
      }
      return 0;
    }


    /**
    * NMDOCS
    * SECTION parts
    * LUA_FUNCTIONS
    * rs:applyPostureControl(int clampScale, vector3 ZMP)
    * 
    * OVERVIEW
    * optionally re-postures the character and counters gravity depending on setting in effectors
    * see the setOpposeGravity(scale) and setPostureStrength(value) effector functions
    * DESCRIPTION
    * First parameter is a clamp scale, ie a maximum on the gravity opposition, second is an optional zero moment point
    */
    int NmRsCharacter::applyPostureControl_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      float clampScale = 0.1f;
      rage::Vector3 ZMP, *zmpPtr = 0;
      ENGINE_CREATE_ARGS_OBJECT(state);
      LUA_GET_AGENT_AND_CHARACTER();

      if (numArgs >= 1)
        clampScale = ENGINE_DIRECT_FUNC_GET_ARG(0).GetNumber();

      NMutils::NMVector3Ptr vec;
      if (numArgs >= 2)
      {
        vec = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData();
        if (agent->isVector3(vec))
        {
          ZMP.Set(vec[0], vec[1], vec[2]);
          zmpPtr = &ZMP;
        }
      }

      applyPostureControl(clampScale, zmpPtr);
      return 0;
    }

    int NmRsCharacter::getPostureTorque_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 3)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int index = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        if ((index < getNumberOfParts() - 1) && (index > -1))
        {
          rage::Vector3 torque = getEffector(index)->getPostureTorque();
          NMutils::NMVector3Ptr t = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData();
          if (agent->isVector3(t))
          {    
            t[0] = torque.x / 10.f;
            t[1] = torque.y / 10.f;
            t[2] = torque.z / 10.f;
          }
          torque = getEffector(index)->getPostureTorque();
          t = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(2).GetLightUserData();
          if (agent->isVector3(t))
          {    
            t[0] = torque.x / 10.f;
            t[1] = torque.y / 10.f;
            t[2] = torque.z / 10.f;
          }
        }
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:disableSelfCollision()
     * 
     * OVERVIEW
     * Stops any collision between parts of character
     * 
     * DESCRIPTION
     *
     */
    int NmRsCharacter::disableSelfCollision_l(ARTlua::LuaState*)
    {
      disableSelfCollision();
      return 0;
    }
    
    /*
     * Rockstar-specific. For internal use only.
     * Find twist, swing1, and swing2 parameters from quaternion.
     * Close to singular configurations (q0^2+q3^2 = 0),
     * swing is fully reversed so twist is not defined.
     * In these cases return twist = 0.
     */
    void QuaternionToTwistAndSwing(NMutils::NMVector3Ptr ts, NMutils::NMVector4Ptr q)
    {
      float chs2 = q[0]*q[0] + q[3]*q[3];
    
      if (chs2 > (1.2e-7f)) 
      {
        float chs = rage::Sqrtf(chs2);
        float mul = 1.0f/(chs*(1.0f + chs));
    
        ts[0] = q[0] > 0.0f ? q[3]/(q[0] + chs) : q[3]/(q[0] - chs);
        ts[1] = mul*(q[0]*q[2] + q[1]*q[3]);
        ts[2] = mul*(q[2]*q[3] - q[0]*q[1]);
      }
      else
      {
        float rshs = 1.0f/(rage::Sqrtf(1.0f - chs2));
        ts[0] = 0.0f;
        ts[1] = rshs*q[2];
        ts[2] = -rshs*q[1];
      }
      
      // Convert to radians.
      ts[0] = 4.0f*atanf(ts[0]);
    
      float s2 = ts[1]*ts[1] + ts[2]*ts[2];
      if (s2 > 0.0f)
      {
        float s = rage::Sqrtf(s2);
        float sInv = 1.0f/s;
        
        ts[1] = 4.0f*atanf(s)*sInv*ts[1];
        ts[2] = 4.0f*atanf(s)*sInv*ts[2];
      }
      else
      {
        ts[1] = 0.0f;
        ts[2] = 0.0f;
      }
    }
    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:quaternionToTwistAndSwing(int index, vector4 q, vector3 ts)
     * 
     * OVERVIEW
     * Calculates twist, swing1, and swing2 parameters (used in Rockstar taking
     * into account: reversing motors, drive mode relative, offsets, and limits)
     * from quaternion.
     *
     * DESCRIPTION
     * First parameter is joint index (integer, zero-based index).
     * Second parameter is the vector4 with the quaternion.
     * Third parameter is the vector3 containing the twist and swing parameters.
     */
    int NmRsCharacter::quaternionToTwistAndSwing_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if(numArgs == 3)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int index = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        if (index < getNumberOfParts() - 1 && index > -1)
        {
          NMutils::NMVector4Ptr q = (NMutils::NMVector4Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData();
          NMutils::NMVector3Ptr ts = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(2).GetLightUserData();  
          if (agent->isVector3(ts) && agent->isVector4(q))
          {
            // Calculate the 'raw' twist and swing (in radians) from the quaternion.
            QuaternionToTwistAndSwing(ts, q);
    
            // Convert to values which take into account: reversing motors, drive mode relative
            // offsets, and limits.
            NmRsEffectorBase *effector = getEffector(index);
            if (effector->is3DofEffector())
            {
              NmRs3DofEffector *threeDofEffector = (NmRs3DofEffector *)effector;

              rage::Vector3 tsV(ts[0], ts[1], ts[2]);
              threeDofEffector->getTwistAndSwingFromRawTwistAndSwing(tsV, tsV);
              ts[0] = tsV.x;
              ts[1] = tsV.y;
              ts[2] = tsV.z;
            }
          }
        }
      }
    
      return 0;
    }
    
    /*
     * Rockstar specific. For internal use only.
     * Construct a quaternion from twist and swing parameters.
     * the twist and swing parameters are supplied in a 3-vector ts;
     * twist = ts[0], swing1 = ts[1], swing2 = ts[2].
     */
    void QuaternionFromTwistAndSwing(NMutils::NMVector4Ptr q, NMutils::NMVector3Ptr ts)
    {
      float tsMag2 = ts[0]*ts[0] + ts[1]*ts[1] + ts[2]*ts[2];
      if (tsMag2 > 0.0f)
      {
        // Convert from radians.
        ts[0] = tan(ts[0]/4.0f);
        float s2 = ts[1]*ts[1] + ts[2]*ts[2];
        if (s2 > 0.0f)
        {
          float s = rage::Sqrtf(s2);
          float sInv = 1.0f/s;
          ts[1] = sInv*tan(s/4.0f)*ts[1];
          ts[2] = sInv*tan(s/4.0f)*ts[2];
        }
        else
        {
          ts[1] = 0.0f;
          ts[2] = 0.0f;
        }
    
        float b = 2.0f/(1.0f + ts[1]*ts[1] + ts[2]*ts[2]);
        float c = 2.0f/(1.0f + ts[0]*ts[0]);
    
        q[0] = (b - 1.0f)*(c - 1.0f);
        q[1] = b*(c*ts[0]*ts[1] - (c - 1.0f)*ts[2]);
        q[2] = b*((c - 1.0f)*ts[1] + c*ts[0]*ts[2]);
        q[3] = (b - 1.0f)*c*ts[0];
      }
      else
      {
        q[0] = 1.0f;
        q[1] = 0.0f;
        q[2] = 0.0f;
        q[3] = 0.0f;
      }
    }
    
    /**
     * NMDOCS
     * SECTION parts
     * LUA_FUNCTIONS
     * rs:quaternionFromTwistAndSwing(vector3 ts, vector4 q)
     * 
     * OVERVIEW
     * Constructs a quaternion from twist and swing parameters (used in Rockstar taking
     * into account: reversing motors, drive mode relative, offsets, and limits). 
     *
     * DESCRIPTION
     * First parameter is joint index (integer, zero-based index).
     * Second parameter is the vector3 with the twist and swing parameters.
     * Third parameter is the vector4 containing the quaternion.
     */
    int NmRsCharacter::quaternionFromTwistAndSwing_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(3)
      {
        RS_LUA_GET_INTEGER_ARG(0, effIndex);
        Assert(effIndex >=0 && effIndex < getNumberOfEffectors());

        RS_LUA_GET_VEC3_ARG(1, ts);
        RS_LUA_GET_VEC4_ARG(2, q);

        NmRsEffectorBase *effector = getEffector(effIndex);
        if (effector->is3DofEffector())
        {
          NmRs3DofEffector *threeDofEffector = (NmRs3DofEffector *)effector;

          // Convert the twist and swing to the 'raw' twist and swing.
          rage::Vector3 tsV(ts[0], ts[1], ts[2]);
          threeDofEffector->getTwistAndSwingFromRawTwistAndSwing(tsV, tsV);
          ts[0] = tsV.x;
          ts[1] = tsV.y;
          ts[2] = tsV.z;

          // Calculate the quaternion from the 'raw' twist and swing (in radians).
          QuaternionFromTwistAndSwing(q, ts);
        }
      }
    
      return 0;
    }
    
    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * float rs:getMass()
     * 
     * OVERVIEW
     * Return the mass for the whole character
     *
     * DESCRIPTION
     * returns 1 float value
     *
     */
    int NmRsCharacter::getMass_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(0)
      {    
        state->PushNumber(getTotalMass());
    
        return 1;
      }
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * void rs:setGravity(vec gravity)
     * void rs:setGravity(gravity.x, gravity.y, gravity.z)
     * 
     * OVERVIEW
     * sets the gravity for the whole simulation
     *
     * DESCRIPTION
     * returns nothing
     *
     */
    int NmRsCharacter::setGravity_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      rage::Vector3 gravity;
      ENGINE_CREATE_ARGS_OBJECT(state);
      LUA_GET_AGENT_AND_CHARACTER();
      if(numArgs == 3)
      {
        gravity.x = ENGINE_DIRECT_FUNC_GET_ARG(0).GetFloat();
        gravity.y = ENGINE_DIRECT_FUNC_GET_ARG(1).GetFloat();
        gravity.z = ENGINE_DIRECT_FUNC_GET_ARG(2).GetFloat();
      }
      else if(numArgs == 1)
      {
        NMutils::NMVector3Ptr g = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData();
        ART::Agent *agent = (ART::Agent*)state->GetStateUserData();
        if (agent->isVector3(g))
        {
          gravity.x = g[0];
          gravity.y = g[1];
          gravity.z = g[2];
        }
      }
      rage::phSimulator::SetGravity(gravity);
      m_gUp.Normalize(gravity);
      m_gUp *= -1.0f;
      return 0;
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * float,float,float rs:getGravity()
     * 
     * OVERVIEW
     * gets the gravity for the whole simulation
     *
     * DESCRIPTION
     * returns gravity as 3 floats
     *
     */
    int NmRsCharacter::getGravity_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(0)
      {
        rage::Vector3 gravity = rage::phSimulator::GetGravity();
        state->PushNumber(gravity.x);
        state->PushNumber(gravity.y);
        state->PushNumber(gravity.z);
        return 3;
      }
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * float rs:setPartFriction(int part, float friction)
     * 
     * OVERVIEW
     * set the friction coefficient on a part
     *
     * DESCRIPTION
     * returns the current friction level
     *
     */
    int NmRsCharacter::setPartFriction_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        RS_LUA_GET_NUMBER_ARG(1, friction);

        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        const rage::phMaterial &mat = getBoundByComponentIdx(partIndex)->GetMaterial(0);
        float oldFriction = mat.GetFriction();
        state->PushNumber(oldFriction);
        ((rage::phMaterial &)mat).SetFriction(friction);
        return 1;
      }
    }

    /**
    * NMDOCS
    * SECTION character
    * LUA_FUNCTIONS
    * float rs:enableCollisionOnPart(int part, bool enabled)
    * 
    * OVERVIEW
    * enable or disable collision on a part, this doesn't effect sensor info
    *
    * DESCRIPTION
    * 
    *
    */
    int NmRsCharacter::enableCollisionOnPart_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_INTEGER_ARG(0, partIndex);
        RS_LUA_GET_BOOL_ARG(1, enable);

        Assert(partIndex >=0 && partIndex < getNumberOfParts());

        getGenericPartByIndex(partIndex)->setCollisionEnabled(enable);
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * float rs:setZUpCharacter(bool zUp)
     * 
     * OVERVIEW
     * sets whether the character is exported with z as up or not
     *
     * DESCRIPTION
     * 
     *
     */
    int NmRsCharacter::setZUpCharacter_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(1)
      {
        RS_LUA_GET_BOOL_ARG(0, zUp);
        m_zUp = zUp;
      }
      return 0;
    }


    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * float rs:getRangeRandom(float min, float max)
     * 
     * OVERVIEW
     * gets a pseudo-random value in range min->max; these values are
     * deterministic per agent from the point of scene-insertion.
     *
     * DESCRIPTION
     * ALWAYS USE THIS INSTEAD OF THE LUA RAND FUNCTIONS!
     *
     */      
    int NmRsCharacter::getRangeRandom_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(2)
      {
        RS_LUA_GET_NUMBER_ARG(0, rMin);
        RS_LUA_GET_NUMBER_ARG(1, rMax);

        state->PushNumber(m_random.GetRanged(rMin, rMax));
        return 1;
      }
    }


    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * float rs:getPerlin3(float x, float y, float z)
     * 
     * OVERVIEW
     * gets a perlin noise value at points x,y,z in the range 0.0 .. 1.0
     *
     */      
    int NmRsCharacter::getPerlin3_l(ARTlua::LuaState *state)
    {
      RS_LUA_BEGINFUNC_WITH_ARGS();
      RS_LUA_CONFIRM_ARGCOUNT(3)
      {
        RS_LUA_GET_NUMBER_ARG(0, x);
        RS_LUA_GET_NUMBER_ARG(1, y);
        RS_LUA_GET_NUMBER_ARG(2, z);

        state->PushNumber(getEngine()->perlin3(x, y, z));
        return 1;
      }
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * bool rs:pointInsideFootSupport(vector point, int leftFootPartIndex, int rightFootPartIndex, footWidth, footLength)
     * 
     * OVERVIEW
     * is a point inside the supporting frame of the 2 feet
     *
     * DESCRIPTION
     * returns distance inside foot support
     *
     */
    int NmRsCharacter::pointInsideFootSupport_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 5 || numArgs == 6)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        NMutils::NMVector3Ptr vec = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData();       
        rage::Vector3 point, nearestVec, *nearestVecPtr = 0;
        if(agent->isVector3(vec))
          point.Set(vec[0], vec[1], vec[2]);

        int leftIndex     = ENGINE_DIRECT_FUNC_GET_ARG(1).GetInteger();
        int rightIndex  = ENGINE_DIRECT_FUNC_GET_ARG(2).GetInteger();
        float footWidth = ENGINE_DIRECT_FUNC_GET_ARG(3).GetNumber();
        float footLength = ENGINE_DIRECT_FUNC_GET_ARG(4).GetNumber();
        NMutils::NMVector3Ptr nVec = 0;
        if (numArgs == 6)
        {
          nVec = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(5).GetLightUserData();       
          if (agent->isVector3(nVec))
          {
            nearestVec.Set(nVec[0], nVec[1], nVec[2]);
            nearestVecPtr = &nearestVec;
          }
        }

        float result = pointInsideFootSupport(point, leftIndex, rightIndex, footWidth, footLength, nearestVecPtr);
/*
        rage::Matrix34 leftMat;
        rage::Matrix34 rightMat;
        if (leftIndex != -1)
        {
          NmRsGenericPart* leftFoot   = getGenericPartByIndex(leftIndex);
          leftFoot->getMatrix(leftMat);
        }
        if (rightIndex != -1)
        {
          NmRsGenericPart* rightFoot  = getGenericPartByIndex(rightIndex);
          rightFoot->getMatrix(rightMat);
        }

        float footWidth = ENGINE_DIRECT_FUNC_GET_ARG(3).GetNumber();
        float footLength = ENGINE_DIRECT_FUNC_GET_ARG(4).GetNumber();
        
        NMutils::NMVector3Ptr nVec = 0;
        if (numArgs == 6)
        {
          nVec = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(5).GetLightUserData();       
          if (agent->isVector3(nVec))
          {
            nearestVec.Set(nVec[0], nVec[1], nVec[2]);
            nearestVecPtr = &nearestVec;
          }
        }
        float result = pointInsideFootSupport(point, leftIndex!=-1 ? &leftMat : NULL, rightIndex!=-1 ? &rightMat : NULL, footWidth, footLength, m_gUp, nearestVecPtr);
*/
        if (nearestVecPtr)
        {
          nVec[0] = nearestVec.x;
          nVec[1] = nearestVec.y;
          nVec[2] = nearestVec.z;
        }

        state->PushNumber(result);
        return 1; 
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * float rs:applySpineLean(float lean1, float lean2)
     * 
     * OVERVIEW
     *
     */
    int NmRsCharacter::applySpineLean_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs >= 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        float lean1 = ENGINE_DIRECT_FUNC_GET_ARG(0).GetNumber();
        float lean2 = ENGINE_DIRECT_FUNC_GET_ARG(1).GetNumber();
         
        applySpineLean(lean1,lean2);

      }
      return 0;
    }
    
    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * int rs:clampEffectorToLimits(int jointIndex)
     * 
     * OVERVIEW
     * clamps the desired angles to the limits
     *
     */
    int NmRsCharacter::clampEffectorToLimits_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 1)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();
        int index = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger(); 
        if (getEffector(index)->is3DofEffector())
        {
          NmRs3DofEffector *eff = (NmRs3DofEffector *)getEffector(index);
          float lean1 = eff->getDesiredLean1() - eff->getMidLean1();
          float lean2 = eff->getDesiredLean2() - eff->getMidLean2();
          float l1 = lean1 / eff->getLean1Extent();
          float l2 = lean2 / eff->getLean2Extent();
          float size = l1*l1 + l2*l2;
          if (size > 1.f)
          {
            float scale = 1.f / rage::Sqrtf(size);
            eff->setDesiredLean1(lean1*scale + eff->getMidLean1());
            eff->setDesiredLean2(lean2*scale + eff->getMidLean2());
          }
          float twist = eff->getDesiredTwist();
          if (twist < eff->getMinTwist())
            eff->setDesiredTwist(eff->getMinTwist());
          else if (twist > eff->getMaxTwist())
            eff->setDesiredTwist(eff->getMaxTwist());
        }
        else
        {
          NmRs1DofEffector *eff = (NmRs1DofEffector *)getEffector(index);
          float angle = eff->getDesiredAngle();
          if (angle < eff->getMinAngle())
            eff->setDesiredAngle(eff->getMinAngle());
          else if (angle > eff->getMaxAngle())
            eff->setDesiredAngle(eff->getMaxAngle());
        }
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * int rs:levelVector(vector3 vec [,float height])
     * 
     * OVERVIEW
     * documentation pending
     *
     */
    int NmRsCharacter::levelVector_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 1 || numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();
        float height = 0.0f;

        NMutils::NMVector3Ptr vec = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData(); 
        Assert(agent->isVector3(vec));

        if (numArgs == 2)
        {
          height = ENGINE_DIRECT_FUNC_GET_ARG(1).GetFloat();
        }

        rage::Vector3 rVec;
        rVec.Set(vec[0], vec[1], vec[2]);

        float dot = m_gUp.Dot(rVec);
        rVec.AddScaled(rVec, m_gUp, (height-dot));

        NMutils::NMVector3Set(vec, rVec.x, rVec.y, rVec.z);
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * int rs:horizDistance(vector3 a, vector3 b)
     * 
     * OVERVIEW
     * documentation pending
     *
     */
    int NmRsCharacter::horizDistance_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();
        NMutils::NMVector3Ptr vA = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData(); 
        NMutils::NMVector3Ptr vB = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData(); 
        Assert(agent->isVector3(vA));
        Assert(agent->isVector3(vB));

        NMutils::NMVector3 dif;
        NMutils::NMVector3Subtract(dif, vB, vA);

        rage::Vector3 rVec;
        rVec.Set(dif[0], dif[1], dif[2]);

        float dot = m_gUp.Dot(rVec);
        rVec.AddScaled(rVec, m_gUp, -dot);

        state->PushNumber(rVec.Mag());
        return 1;
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * int rs:vertDistance(vector3 a, vector3 b)
     * 
     * OVERVIEW
     * documentation pending
     *
     */
    int NmRsCharacter::vertDistance_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();
        NMutils::NMVector3Ptr vA = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData(); 
        NMutils::NMVector3Ptr vB = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData(); 
        Assert(agent->isVector3(vA));
        Assert(agent->isVector3(vB));

        rage::Vector3 rVecA;
        rage::Vector3 rVecB;

        rVecA.Set(vA[0], vA[1], vA[2]);
        rVecB.Set(vB[0], vB[1], vB[2]);

        float dotA = m_gUp.Dot(rVecA);
        float dotB = m_gUp.Dot(rVecB);

        state->PushNumber(dotB - dotA);
        return 1;
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * int rs:rotateVectorAxisAngle(vector3 out, vector3 in, float angle [, vector3 axis])
     * 
     * OVERVIEW
     * Defaults to use global up vector if axis not specified
     *
     */
    int NmRsCharacter::rotateVectorAxisAngle_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 3 || numArgs == 4)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();
        float angle = ENGINE_DIRECT_FUNC_GET_ARG(2).GetFloat();
        float cosAngle = rage::Cosf(angle);
        float sinAngle = rage::Sinf(angle);

        NMutils::NMVector3Ptr vOut = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData(); 
        NMutils::NMVector3Ptr vIn = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData(); 
        Assert(agent->isVector3(vOut));
        Assert(agent->isVector3(vIn));

        rage::Vector3 rVecOut;
        rage::Vector3 rVecIn;
        rage::Vector3 rAxis;

        rVecIn.Set(vIn[0], vIn[1], vIn[2]);

        if (numArgs == 4)
        {
          NMutils::NMVector3Ptr vAxis = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(3).GetLightUserData(); 
          rAxis.Set(vAxis[0], vAxis[1], vAxis[2]);
        }
        else
          rAxis.Set(m_gUp);

        float axisDotVecIn = rAxis.Dot(rVecIn);
        rage::Vector3 result;

        result.Cross(rVecIn, rAxis);
        result *= -sinAngle;

        result.AddScaled(rAxis, axisDotVecIn);
        result.AddScaled(rVecIn, cosAngle);
        rVecOut.AddScaled(result, rAxis, -cosAngle*axisDotVecIn);

        NMutils::NMVector3Set(vOut, rVecOut.x, rVecOut.y, rVecOut.z);
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * int rs:buildMatrix(matrix4 out, vector3 forwardsVector)
     * 
     * OVERVIEW
     * Construct a matrix taking the yaw from the specified forwards vector - note this is a 
     * 'sideways' matrix, as the R* buttocks/spine matrices are sideways
     *
     */
    int NmRsCharacter::buildMatrix_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();
        NMutils::NMMatrix4Ptr mOut = (NMutils::NMMatrix4Ptr)ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData(); 
        NMutils::NMVector3Ptr vIn = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData(); 
        Assert(agent->isMatrix4(mOut));
        Assert(agent->isVector3(vIn));

        rage::Vector3 forwardsVec;
        forwardsVec.Set(vIn[0], vIn[1], vIn[2]);

        rage::Matrix34 result;

        result.a.Set(m_gUp);
        result.b.Cross(forwardsVec, result.a);
        result.b.Normalize();
        result.c.Cross(result.a, result.b);

        // bleh!
        NMutils::NMVector4Set(mOut[0], result.a.x, result.a.y, result.a.z, 0.0f);
        NMutils::NMVector4Set(mOut[1], result.b.x, result.b.y, result.b.z, 0.0f);
        NMutils::NMVector4Set(mOut[2], result.c.x, result.c.y, result.c.z, 0.0f);
        NMutils::NMVector4Set(mOut[3], 0.0f, 0.0f, 0.0f, 1.0f);
      }
      return 0;
    }

    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * int rs:applyUprightConstraintForces(float stiffness, float damping)
     * 
     * OVERVIEW
     * Apply some 'cheat forces' to the buttocks and feet to assist in keeping the
     * character balancing / on their feet
     *
     */
    int NmRsCharacter::applyUprightConstraintForces_l(ARTlua::LuaState* state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        float in_stiffness = ENGINE_DIRECT_FUNC_GET_ARG(0).GetNumber();
        float in_damping = ENGINE_DIRECT_FUNC_GET_ARG(1).GetNumber();

        applyUprightConstraintForces(1.f,in_stiffness, in_damping);
      }
      return 0;
    }

    /**
      * NMDOCS
      * SECTION character
      * LUA_FUNCTIONS
      * int rs:fixPart(int index, vector3 position)
      * 
      * OVERVIEW
      * Fixes a part to its current position. First parameter is part index (integer, zero-based index), 
      * second argument is the location of the pin constraint in world space. Optional third arguement allows different attachment point in world
      *
      * DESCRIPTION
      * Returns a handle to the constraint.
      *
      */
    int NmRsCharacter::fixPart_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs >= 1 && numArgs <= 4)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int index = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();

        rage::phConstraintMgr *mgr = getSimulator()->GetConstraintMgr();
        NMutils::NMVector3 p;
        getGenericPartByIndex(index)->getPosition(p);
        rage::Vector3 pos(p[0], p[1], p[2]);
        rage::Vector3 pos2 = pos;
        float partSeparation = 0.f;

        if (numArgs >= 2)
        {
          NMutils::NMVector3Ptr worldPos = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData(); 
          pos.Set(worldPos[0], worldPos[1], worldPos[2]);
          pos2 = pos;
        }
        if (numArgs >= 3)
        {
          NMutils::NMVector3Ptr worldPos = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(2).GetLightUserData(); 
          pos2.Set(worldPos[0], worldPos[1], worldPos[2]);
        }
        if (numArgs == 4)
          partSeparation = ENGINE_DIRECT_FUNC_GET_ARG(3).GetNumber(); 
        rage::phConstraint *con = mgr->AttachObjectToWorld(pos, pos2, getFirstInstance(), index, partSeparation, false);
        state->PushLightUserData((void *)con);
        return 1;
      }
      return 0;
    }

    // TDL check to see whether 1 arm of constraint has disappeared... if so, remove the constraint
    int NmRsCharacter::checkConstraint_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        rage::phConstraint *constraintHandle = (rage::phConstraint *) ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData();
        Assert(constraintHandle);
        int levelIndex = ENGINE_DIRECT_FUNC_GET_ARG(1).GetInteger();
        LUA_GET_AGENT_AND_CHARACTER();
        rage::phConstraintMgr *mgr = getSimulator()->GetConstraintMgr();
        if (levelIndex != -1)
        {
          rage::phLevelNew *level = getEngine()->getLevel();
          if (level->GetState(levelIndex) == rage::phLevelBase::OBJECTSTATE_NONEXISTENT)
          {
            mgr->ReleaseConstraint(constraintHandle);
            state->PushLightUserData(NULL);
            return 1;
          }
        }
        state->PushLightUserData((void *)constraintHandle);
        return 1;
      }
      return 0;
    }

    int NmRsCharacter::getLevelIndex_l(ARTlua::LuaState *state)
    {
      ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      ENGINE_CREATE_ARGS_OBJECT(state);
      LUA_GET_AGENT_AND_CHARACTER();
      state->PushInteger(getFirstInstance()->GetLevelIndex());
      return 1;
    }

    int NmRsCharacter::getVelocityOnInstance_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 3)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();
        int levelIndex = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        Assert(levelIndex >= -1);
        NMutils::NMVector3Ptr position = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData();
        NMutils::NMVector3Ptr velocity = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(2).GetLightUserData();
        rage::Vector3 worldPos(position[0], position[1], position[2]);
        if (levelIndex == -1)
        {
          velocity[0] = 0.f; velocity[1] = 0.f; velocity[2] = 0.f; 
        }
        else
        {
          rage::phCollider *collider = getEngine()->getSimulator()->GetCollider(levelIndex);
          if (collider)
          {
            rage::Vector3 vel;
            collider->GetLocalVelocity(worldPos, RC_VEC3V(vel));
            velocity[0] = vel.x; velocity[1] = vel.y; velocity[2] = vel.z; 
          }
          else
          {
            velocity[0] = 0.f; velocity[1] = 0.f; velocity[2] = 0.f; 
          }
        }
      }
      return 0;
    } 

    int NmRsCharacter::getInstMatrix_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      ENGINE_CREATE_ARGS_OBJECT(state);
      LUA_GET_AGENT_AND_CHARACTER();
      if (numArgs == 2)
      {
        int levelIndex = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        Assert(levelIndex >= -1);
        NMutils::NMMatrix4Ptr mat = (NMutils::NMMatrix4Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData();
        if (levelIndex == -1)
          NMutils::NMMatrix4MakeIdentity(mat);
        else
        {
          rage::phInst *pInst = getEngine()->getLevel()->GetInstance(levelIndex);
          if (pInst)
          {
            rage::Matrix34 instMatrix;
            instMatrix = RCC_MATRIX34(pInst->GetMatrix());
            NMutils::NMVector4Set(mat[0], instMatrix.a.x, instMatrix.a.y, instMatrix.a.z, 0.0f);
            NMutils::NMVector4Set(mat[1], instMatrix.b.x, instMatrix.b.y, instMatrix.b.z, 0.0f);
            NMutils::NMVector4Set(mat[2], instMatrix.c.x, instMatrix.c.y, instMatrix.c.z, 0.0f);
            NMutils::NMVector4Set(mat[3], instMatrix.d.x, instMatrix.d.y, instMatrix.d.z, 1.0f); 
          }
          else
          {
            NMutils::NMMatrix4MakeIdentity(mat);
#if NM_RS_ENABLE_LOGGING
            m_services->logger->logf(NMutils::BasicLogger::kError, L"NmRsCharacter::getInstMatrix_l: invalid level index passed in, phInst == NULL");
#endif // NM_RS_ENABLE_LOGGING
          }
        }
      }
      return 0;
    } 

    /**
    * NMDOCS
    * SECTION character
    * LUA_FUNCTIONS
    * handle rs:fixPartsTogether(int index1, int index2, float separation, vector3 position, int slotIndex)
    * 
    * OVERVIEW
    * Fixes two parts together. First parameter is part 1 index (integer, zero-based index), 
    * second parameter is part 2 index (integer, zero-based index), third parameter 
    * is separation (float), and the fourth parameter is position of pin constraint (optional).
    * slot index is optional, it looks up an instance set in-game.
    *
    * DESCRIPTION
    * Returns a handle to the constraint.
    *
    */
    int NmRsCharacter::fixPartsTogether_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs >= 3 && numArgs <= 5)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int index1 = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        int index2 = ENGINE_DIRECT_FUNC_GET_ARG(1).GetInteger();
        float partSeparation = ENGINE_DIRECT_FUNC_GET_ARG(2).GetFloat();

        rage::phConstraintMgr *mgr = getSimulator()->GetConstraintMgr();
        NMutils::NMVector3 p;
        getGenericPartByIndex(index1)->getPosition(p);
        rage::Vector3 pos(p[0], p[1], p[2]);
        getGenericPartByIndex(index2)->getPosition(p);
        rage::Vector3 pos2(p[0], p[1], p[2]);
        if (numArgs >= 4)
        {
          NMutils::NMVector3Ptr worldPos = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(3).GetLightUserData(); 
          pos.Set(worldPos[0], worldPos[1], worldPos[2]);
          pos2.Set(worldPos[0], worldPos[1], worldPos[2]);
        }
        int levelIndex = -1;
        if (numArgs >= 5)
          levelIndex = ENGINE_DIRECT_FUNC_GET_ARG(4).GetInteger();
        rage::phInst *pInst = getFirstInstance();
        if (levelIndex != -1)
          pInst = getEngine()->getLevel()->GetInstance(levelIndex);
        Assert(pInst);
        rage::phConstraint *con;
        if (getFirstInstance()->GetLevelIndex() <= pInst->GetLevelIndex())
          con = mgr->AttachObjects(pos, pos2, getFirstInstance(), pInst, index1, index2, partSeparation, false);
        else
          con = mgr->AttachObjects(pos2, pos, pInst, getFirstInstance(), index2, index1, partSeparation, false);
        state->PushLightUserData((void *)con);
        return 1;
      }
    
      return 0;
    }

    /**
    * NMDOCS
    * SECTION character
    * LUA_FUNCTIONS
    * handle rs:fixPartsTogether2(int index1, int index2, float separation, vector3 pos1, vector3 pos2, int levelIndex)
    * 
    * OVERVIEW
    * Fixes two parts together. First parameter is part 1 index (integer, zero-based index), 
    * second parameter is part 2 index (integer, zero-based index), third parameter 
    * is separation (float), the fourth parameter is world space part 1 constraint position, 
    * the fifth parameter is world space part 2 constraint position. The 6th param
    * levelIndex is optional, it looks up an instance set in-game.
    *
    * DESCRIPTION
    * Returns a handle to the constraint.
    *
    */
    int NmRsCharacter::fixPartsTogether2_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs >= 5 && numArgs <= 6)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int index1 = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        int index2 = ENGINE_DIRECT_FUNC_GET_ARG(1).GetInteger();
        float partSeparation = ENGINE_DIRECT_FUNC_GET_ARG(2).GetFloat();
        NMutils::NMVector3Ptr p1 = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(3).GetLightUserData(); 
        NMutils::NMVector3Ptr p2 = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(4).GetLightUserData(); 


        rage::phConstraintMgr *mgr = getSimulator()->GetConstraintMgr();
        rage::Vector3 pos(p1[0], p1[1], p1[2]);
        rage::Vector3 pos2(p2[0], p2[1], p2[2]);
        int levelIndex = -1;
        if (numArgs >= 6)
          levelIndex = ENGINE_DIRECT_FUNC_GET_ARG(5).GetInteger();
        rage::phInst *pInst = getFirstInstance();
        if (levelIndex != -1)
          pInst = getEngine()->getLevel()->GetInstance(levelIndex);
        Assert(pInst);
        rage::phConstraint *con;
        if (getFirstInstance()->GetLevelIndex() <= pInst->GetLevelIndex())
          con = mgr->AttachObjects(pos, pos2, getFirstInstance(), pInst, index1, index2, partSeparation, false);
        else
          con = mgr->AttachObjects(pos2, pos, pInst, getFirstInstance(), index2, index1, partSeparation, false);
        state->PushLightUserData((void *)con);
        return 1;
      }

      return 0;
    }

    /**
    * NMDOCS
    * SECTION character
    * LUA_FUNCTIONS
    * void rs:setConstraintBreakingStrength(phConstraint *constraint, float breakingStrength)
    * 
    * OVERVIEW
    * sets the breaking strength on a constraint.. I think this is a force
    *
    */
    int NmRsCharacter::setConstraintBreakingStrength_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        rage::phConstraint *constraintHandle = (rage::phConstraint *) ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData();
        if (constraintHandle)
        {
          LUA_GET_AGENT_AND_CHARACTER();
          float strength = ENGINE_DIRECT_FUNC_GET_ARG(1).GetFloat();
          constraintHandle->SetBreakingStrength(strength);
        }
      }
      return 0;
    }
    /**
    * NMDOCS
    * SECTION character
    * LUA_FUNCTIONS
    * rs:removeConstraint(handle)
    * 
    * OVERVIEW
    * removes the constraint returned by the fix part routines, pass in handle returned from constraint
    * creation functions
    *
    * DESCRIPTION
    * returns nothing
    *
    */
    int NmRsCharacter::removeConstraint_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 1)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        rage::phConstraint *constraintHandle = (rage::phConstraint *) ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData();
        if (constraintHandle)
        {
          LUA_GET_AGENT_AND_CHARACTER();
          rage::phConstraintMgr *mgr = getSimulator()->GetConstraintMgr();
          mgr->ReleaseConstraint(constraintHandle);
        }
        return 0;
      }
      return 0;
    }

    /**
    * NMDOCS
    * SECTION character
    * LUA_FUNCTIONS
    * float rs:updateConstraintSeparation_l(int handle, float separation)
    * 
    * OVERVIEW
    * updates the separation of constraints
    *
    * DESCRIPTION
    * returns nothing
    *
    */
    int NmRsCharacter::updateConstraintSeparation_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        rage::phConstraint *constraintHandle = (rage::phConstraint *) ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData();
        if (constraintHandle)
        {
          float separation = ENGINE_DIRECT_FUNC_GET_ARG(1).GetFloat();
          constraintHandle->SetLength(separation);
        }
      }
      return 0;
    }   

    /**
    * NMDOCS
    * SECTION character
    * LUA_FUNCTIONS
    * float rs:updateConstraintPosition(int handle, vector worldPos)
    * 
    * OVERVIEW
    * updates the position the constraint is fixed to.
    *
    * DESCRIPTION
    * returns nothing
    *
    */
    int NmRsCharacter::updateConstraintPosition_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        rage::phConstraint *constraintHandle = (rage::phConstraint *) ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData();
        if (constraintHandle)
        {
          NMutils::NMVector3Ptr worldPos = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData(); 
          rage::Vector3 pos;
          pos.Set(worldPos[0], worldPos[1], worldPos[2]);
          constraintHandle->SetWorldPosition(pos);
        }
      }
      return 0;
    }   

    /**
    * NMDOCS
    * SECTION character
    * LUA_FUNCTIONS
    * float rs:makeConstraintSoft()
    * 
    * OVERVIEW
    * sets some constraint properties, first is constraint handle (from fixPart), next two are spring and damping constants for rotation, next two optional are for translation
    *
    * DESCRIPTION
    * returns nothing
    *
    */
    int NmRsCharacter::makeConstraintSoft_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 3 || numArgs == 5)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        rage::phConstraint *constraintHandle = (rage::phConstraint *) ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData();
        if (constraintHandle)
        {
          float rotSpring = ENGINE_DIRECT_FUNC_GET_ARG(1).GetFloat();
          float rotDamping = ENGINE_DIRECT_FUNC_GET_ARG(2).GetFloat();
          if (numArgs == 5)
          {
            float spring = ENGINE_DIRECT_FUNC_GET_ARG(3).GetFloat();
            float damping = ENGINE_DIRECT_FUNC_GET_ARG(4).GetFloat();
            constraintHandle->SetForceMode(spring, damping);
          }
          else
            constraintHandle->SetTorqueMode(rotSpring, rotDamping);
        }
      }
      return 0;
    }

    int NmRsCharacter::removeCollidePair_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int part1 = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        int part2 = ENGINE_DIRECT_FUNC_GET_ARG(1).GetInteger();
        rage::phArticulatedCollider *collider = getArticulatedWrapper()->getArticulatedCollider();
        collider->SetPartsCanCollide(part1, part2, false);
      }
      return 0;
    }

    int NmRsCharacter::addCollidePair_l(ARTlua::LuaState *state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        int part1 = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        int part2 = ENGINE_DIRECT_FUNC_GET_ARG(1).GetInteger();
        getArticulatedWrapper()->getArticulatedCollider()->SetPartsCanCollide(part1, part2);
      }
      return 0;
    }


    /**
     * NMDOCS
     * SECTION character
     * LUA_FUNCTIONS
     * float rs:setSkeletonVizMode(string mode)
     * 
     * OVERVIEW
     * chooses the skeleton viz mode (only in Studio, with NMDRAW enabled) - options are "none", "desired" and "actual"
     *
     * DESCRIPTION
     * returns nothing
     *
     */
    int NmRsCharacter::setSkeletonVizMode_l(ARTlua::LuaState* state)
    {
#if NM_RS_ENABLE_DEBUGDRAW

      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 1)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        const char* mode = ENGINE_DIRECT_FUNC_GET_ARG(0).GetString();

        if (!strnicmp(mode, "none", 4))
          setSkeletonVizMode(NmRsCharacter::kSV_None);
        else if (!strnicmp(mode, "desired", 7))
          setSkeletonVizMode(NmRsCharacter::kSV_DesiredAngles);
        else if (!strnicmp(mode, "actual", 6))
          setSkeletonVizMode(NmRsCharacter::kSV_ActualAngles);
#if NM_RS_ENABLE_LOGGING
        else
          m_services->logger->logf(NMutils::BasicLogger::kError, L"setSkeletonVizMode: option passed is not valid (\"none\", \"desired\" and \"actual\")");
#endif // NM_RS_ENABLE_LOGGING
      }

#endif // NM_RS_ENABLE_DEBUGDRAW
      return 0;    
    }

    /**
    * NMDOCS
    * SECTION character
    * LUA_FUNCTIONS
    * bool hit, int component rs:probeRay(vec3 startPos, vec3 endPos, vec3 contactPoint, vec3 contactNormal, bool includeAgent)
    * 
    * OVERVIEW
    * ray probe into the physics world
    *
    * DESCRIPTION
    * returns nothing
    *
    */
    int NmRsCharacter::probeRay_l(ARTlua::LuaState* state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 4 || numArgs == 5)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();

        rage::Vector3 startPos, endPos;

        NMutils::NMVector3Ptr nmstartPos = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData();
        NMutils::NMVector3Ptr nmendPos = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(1).GetLightUserData();
        NMutils::NMVector3Ptr pos = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(2).GetLightUserData(); 
        NMutils::NMVector3Ptr normal = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(3).GetLightUserData(); 
       
        bool includeAgent = true;
        if (numArgs == 5)
          includeAgent = ENGINE_DIRECT_FUNC_GET_ARG(4).GetBoolean(); 

        startPos.Set(nmstartPos[0], nmstartPos[1], nmstartPos[2]);
        endPos.Set(nmendPos[0], nmendPos[1], nmendPos[2]);


        rage::phSegment segment;
        segment.Set(startPos, endPos);
        rage::phIntersection isect;
        bool hit = getLevel()->TestProbe(segment, &isect, includeAgent ? NULL : getFirstInstance(), INCLUDE_FLAGS_ALL, m_probeIncludeFlags) ? true : false;
        if (hit)
        {
          pos[0] = isect.GetPosition().x;pos[1] = isect.GetPosition().y;pos[2] = isect.GetPosition().z;
          normal[0] = isect.GetNormal().x;normal[1] = isect.GetNormal().y;normal[2] = isect.GetNormal().z;
        }
        state->PushBoolean(hit);
        state->PushInteger(hit ? isect.GetComponent() : 0);
        return 2;
      }      
      return 0;
    } 


    int NmRsCharacter::cleverHandIK_l(ARTlua::LuaState* state)
    {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 9)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();
        
        rage::Vector3 target;
        NMutils::NMVector3Ptr nmstartPos = (NMutils::NMVector3Ptr)ENGINE_DIRECT_FUNC_GET_ARG(0).GetLightUserData();
        target.Set(nmstartPos[0], nmstartPos[1], nmstartPos[2]);
  
        const LeftArmSetup   *m_leftArm;
        m_leftArm = getLeftArmSetup();
        
        const RightArmSetup  *m_rightArm;
        m_rightArm = getRightArmSetup(); 

        float direcn = ENGINE_DIRECT_FUNC_GET_ARG(1).GetFloat();
        
        
        bool useHardConstraint = ENGINE_DIRECT_FUNC_GET_ARG(2).GetBoolean();

        float strength = ENGINE_DIRECT_FUNC_GET_ARG(3).GetFloat();


        rage::phConstraint *constraint = (rage::phConstraint *) ENGINE_DIRECT_FUNC_GET_ARG(4).GetLightUserData();
        

        float maxDistance = ENGINE_DIRECT_FUNC_GET_ARG(5).GetFloat();;

        int rootPartIdx = ENGINE_DIRECT_FUNC_GET_ARG(6).GetInteger();
        NmRsGenericPart *woundPart   = getGenericPartByIndex(rootPartIdx);

        int instanceIndex = ENGINE_DIRECT_FUNC_GET_ARG(7).GetInteger();

        float threshold = ENGINE_DIRECT_FUNC_GET_ARG(8).GetFloat();;

        if (direcn==1){
          cleverHandIK(target,m_rightArm,direcn,useHardConstraint,strength,&constraint,maxDistance,woundPart,instanceIndex, threshold,0);
        }
        if ( direcn == -1){
          cleverHandIK(target,m_leftArm,direcn,useHardConstraint,strength,&constraint,maxDistance,woundPart,instanceIndex, threshold,0);
        }
   
        state->PushLightUserData((void *)constraint);
        state->PushNumber(maxDistance);
 //       state->PushNumber(distance);

      return 2;
        }      
      
      return 0;
    } 
   
    
    int NmRsCharacter::matchClavicleToShoulderBetter_l(ARTlua::LuaState* state)
          {
      int numArgs = ENGINE_DIRECT_FUNC_GET_NUM_ARGS(state);
      if (numArgs == 2)
      {
        ENGINE_CREATE_ARGS_OBJECT(state);
        LUA_GET_AGENT_AND_CHARACTER();
        int clavEffint = ENGINE_DIRECT_FUNC_GET_ARG(0).GetInteger();
        int shouldEffint = ENGINE_DIRECT_FUNC_GET_ARG(1).GetInteger();

        NmRs3DofEffector *clavEff   = (NmRs3DofEffector *)getEffector(clavEffint);

        NmRs3DofEffector *shouldEff   = (NmRs3DofEffector *)getEffector(shouldEffint);

        matchClavicleToShoulderBetter(clavEff,shouldEff);
      }
      return 0;
    }
    

#endif // NM_RS_REDUCED_LUA_INTERFACE


  }
}

#endif // ART_NO_LUA


