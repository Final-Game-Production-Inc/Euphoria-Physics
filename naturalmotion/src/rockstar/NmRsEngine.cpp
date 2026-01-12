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


#include "NmRsInclude.h"
#include "NmRsCharacter.h"
#include "NmRsEngine.h"
#include "NmRsCBU_TaskManager.h"
#include "NmRsLimbManager.h"

#if NM_SCRIPTING
#include "NmRsGenericPart.h"//only needed to get local hitpoint when delaying applyBulletImpulse
#include "nmutils/TypeUtils.h"//only to get hashString for NM_SCRIPTING
#endif

#if ART_ENABLE_BSPY
#include "NmRsSpy.h"
#include "phbound/boundbvh.h"
#endif // ART_ENABLE_BSPY

#if __BANK
#include "system/timer.h"
#endif

#ifdef NM_PLATFORM_CELL_PPU
#include <string.h> // for strcmp, strcpy etc
#endif // NM_PLATFORM_CELL_PPU

#if __PFDRAW
PFD_DECLARE_GROUP(NaturalMotion);
PFD_DECLARE_ITEM(GrabBehaviour, rage::Color32(255, 198, 32), NaturalMotion);
#endif // __PFDRAW

namespace ART
{
  // -------------------------------------------------------------------------------------------------------------------
  const AgentID INVALID_AGENTID = 0xffffffff;
  const int MAX_AGENTS = 10;//Maximum number of NM Characters allowed in the game.  (DO NOT EXCEED 24)
  const int MAX_NON_NM_RAGDOLLS = 20;

  float NmRsEngine::ms_StepPhase1TimeUS = 0.0f;
  float NmRsEngine::ms_PreStepTimeUS = 0.0f;
  float NmRsEngine::ms_ApplyTransformsTimeUS = 0.0f;
  float NmRsEngine::ms_StepPhase2TimeUS = 0.0f;
  float NmRsEngine::ms_CalcCOMValsTimeUS = 0.0f;
  float NmRsEngine::ms_BehaviorsTimeUS = 0.0f;
  float NmRsEngine::ms_PostStepTimeUS = 0.0f;

	SelfCollisionSet NmRsEngine::m_DefaultSelfCollisionSet;
	SelfCollisionSet NmRsEngine::m_PointGunSelfCollisionSet;
	SelfCollisionSet NmRsEngine::m_FlinchSelfCollisionSet;

  // -------------------------------------------------------------------------------------------------------------------
  NmRsEngine::NmRsEngine(ART::MemoryManager* services) : 
    m_simulator(0),
    m_level(0),
    m_incomingAnimationVelocityScale(1.0f),
    m_animationMaxSpeed(1e10f),
    m_animationMaxAngSpeed(1e10f),
    m_animationAverageVel(0,0,0),
    m_animationAverageVelSet(false),
    m_lastKnownUpdateStep(1.0f / 60.0f),
    m_lastKnownUpdateStepClamped(m_lastKnownUpdateStep),
    m_schedulerIndex(0),
    m_artMemoryManager(services),
    m_characterTypeData(0),
    m_distributedTasksEnabled(true),
	m_AllowLegInterpenetration(false),
    m_framesSinceDyingAgentRemoved(0),
    m_activeNonNMRagdolls(0)
  {
#if NM_RS_ENABLE_LOGGING || __PPU
    // disable distributed tasks in behaviours if current config doesn't support it
    m_distributedTasksEnabled = false;
#endif // NM_RS_ENABLE_LOGGING


    // fill the ID queue (FIFO), reset character index list
    for (int i=(MaxNumCharacters-1); i>=0; i--)
    {
      m_characterIDs.Push(i);
      m_characterByIndex[i] = 0;
    }

    m_maxAssets = NUM_ASSETS; //mmmmmnoArt mmmmtodo have this taken from one global value

    // Use XML file instead of binary ART file to load NM assets.
    ARTCustomPlacementNew1Arg(m_taskManager, NmRsCBUTaskManager, MaxNumCharacters);

    ARTCustomPlacementNew1Arg(m_limbManager, NmRsLimbManager, MAX_AGENTS);

#if ART_ENABLE_BSPY
    ARTCustomPlacementNew1Arg(m_spy, NmRsSpy, this);
    bspyMemoryUsageRecord("NmRsEngine", 
      sizeof(NmRsEngine) + 
      (sizeof(NmRsCharacter*) * MaxNumCharacters) );

#if NM_ANIM_MATRICES
    int numAnimationParts = 21;
    for (unsigned int i=0; i<NUM_ASSETS; i++)
    {
      m_LeadInAnimationMatrices[i] = rage_new rage::Matrix34[numAnimationParts];
      m_leadInAnimationSent[i] = false;
    }
#endif

#endif // ART_ENABLE_BSPY
  }

  // -------------------------------------------------------------------------------------------------------------------
  NmRsEngine::~NmRsEngine()
  {
#if ART_ENABLE_BSPY
#if NM_ANIM_MATRICES
    for (unsigned int i=0; i<NUM_ASSETS; i++)
    {
      delete[] m_LeadInAnimationMatrices[i];
    }
#endif

    ARTCustomPlacementDelete(m_spy, NmRsSpy);
#endif // ART_ENABLE_BSPY

    ARTCustomPlacementDelete(m_taskManager, NmRsCBUTaskManager);

    ARTCustomPlacementDelete(m_limbManager, NmRsLimbManager);
  }

  // -------------------------------------------------------------------------------------------------------------------
  bool NmRsEngine::initialiseAgent(AssetID asset_ID, AgentID characterID, rage::phArticulatedCollider *collider, unsigned randomSeed)
  {
    // check that the agent ID is within the number maintainable by the engine
    if (characterID >= MaxNumCharacters)
    {
      NM_RS_LOGERROR(L"NmRsEngine::initialiseAgent() Error : Agent %i is > maximum characters managable by ART (%i)", id, MaxNumCharacters);
      return false;
    }

    // make sure the character slot for this agent is empty
    if (m_characterByIndex[characterID] != 0)
    {
      NM_RS_LOGERROR(L"NmRsEngine::initialiseAgent() Error : Agent %i already initialised?", id);
      return false;
    }

    // create a new character manager instance, NmRsCharacter,
    // and initialize it by deserializing from the memory stream.
    NmRsCharacter* charToBuild;
    ARTCustomPlacementNew4Arg(charToBuild, NmRsCharacter, this, asset_ID, characterID, randomSeed);
    charToBuild->setIdentifier(m_characterTypeData[asset_ID].m_Identifier);

    // store the new character instance in the appropriate slot
    m_characterByIndex[characterID] = charToBuild;

    // record the new instance in the hot list
    m_characterVector.push_back(m_characterByIndex[characterID]);

#if ART_ENABLE_BSPY
    AgentState asblock;
    getAgentStateBlock(asblock);
    bSpy::AgentStatePacket asp(asblock);
    m_spy->sendPacket(asp);
#endif // ART_ENABLE_BSPY

    //mmmmTEST charToBuild->setIdentifier("Fred");//as above doesn't seem to be working
    if (!charToBuild->setupCharacter(collider))
    {
      NM_RS_LOGERROR(L"NmRsEngine::initialiseAgent() Error : NmRsCharacter::Deserialize() failed on asset ID %i", agent->getAssetID());

      // failed; throw away character and return
      ARTCustomPlacementDelete(charToBuild, NmRsCharacter);
      return false;
    }

    m_taskManager->init(characterID, charToBuild);


    return true;
  }

  // -------------------------------------------------------------------------------------------------------------------
  bool NmRsEngine::insertAgent(ART::AgentID characterID)
  {
    NmRsCharacter* agent = getCharacterFromAgentID(characterID);

    if (!agent)
    {
      NM_RS_LOGERROR(L"NmRsEngine::insertAgent: character (%d) does not exist", characterID);
      return false;
    }
    if (agent->isInsertedInScene())
    {
      NM_RS_LOGERROR(L"NmRsEngine::insertAgent: character (%d) already inserted in the scene ", characterID);
      return false;
    }

    // Set initial TMs from incoming transforms
    int incomingComponentCount;

    rage::Matrix34* itPtr = 0;
    rage::Matrix34* itPrvPtr = 0;
    IncomingTransformStatus itmStatusFlags = kITSNone;
    agent->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, kITSourceCurrent);
    agent->getIncomingTransforms(&itPrvPtr, itmStatusFlags, incomingComponentCount, kITSourcePrevious);

    if (incomingComponentCount > 0)
    {
      int numTMsToSet = rage::Min(agent->getNumberOfParts(), incomingComponentCount);
     for(int i=0; i<numTMsToSet; ++i)
        setInitialTM(characterID, i, itPtr[i], (itPrvPtr != 0)?(&itPrvPtr[i]):0  );
    }

    // plug the character into the simulation
    Assert(m_characterByIndex[characterID]);
    agent->addToScene(m_simulator, m_level);
    m_taskManager->addToScene(characterID, agent);

    return true;
  }

  // -------------------------------------------------------------------------------------------------------------------
  bool NmRsEngine::removeAgent(ART::AgentID characterID)
  {
    NmRsCharacter* agent = getCharacterFromAgentID(characterID);

    if (!agent)
    {
      NM_RS_LOGERROR(L"NmRsEngine::removeAgent: character (%d) does not exist", characterID);
      return false;
    }
    if (!agent->isInsertedInScene())
    {
      NM_RS_LOGERROR(L"NmRsEngine::removeAgent: character (%d) is not inserted in the scene ", characterID);
      return false;
    }

    Assert(m_characterByIndex[characterID]);
    m_taskManager->removeFromScene(characterID, agent);
    agent->removeFromScene();
    
    return true;
  }

#if ART_ENABLE_BSPY
  void NmRsEngine::setbSpyObject(int levelIndex)
  {
    if (m_level->IsInLevel(levelIndex))
    {
      bool newObject = true;
      for ( int i = m_bSpyObjectsQueue.GetCount()-1 ; i >= 0; i-- )
      {
        //already there?
        if (m_bSpyObjectsQueue[i] == levelIndex)
        {
          newObject = false;
          break;
        }
        //cleanUp queue (doesn't need to be completed as will be done when displaying)
        if (!m_level->IsInLevel(m_bSpyObjectsQueue[i]))
          m_bSpyObjectsQueue.Delete(i);
      }
      if (newObject)
      {
        if(m_bSpyObjectsQueue.IsFull())
          m_bSpyObjectsQueue.Drop();
        m_bSpyObjectsQueue.Push(levelIndex);
      }
    }
  }

  void NmRsEngine::bSpyProcessInstanceOnContact(rage::phInst* inst)
  {     
    rage::phBound* bound = inst->GetArchetype()->GetBound();
    rage::phBound::BoundType boundType = (rage::phBound::BoundType)bound->GetType();
    int instLevelIndex = inst->GetLevelIndex();

    NmRsSpy& spy = *getNmRsSpy();

    if (spy.shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_StaticMeshData))
    {
      // BVH is the prevailing choice for static terrain data
      if (boundType == rage::phBound::BVH && PHLEVEL->IsFixed(instLevelIndex))
      {
        const int polysTransferredPerCall = 160;

        rage::phBoundBVH *bvh = static_cast<rage::phBoundBVH*>(bound);
        unsigned int i = 0, t, numPoly = (unsigned int)bvh->GetNumPolygons();

        Assert(spy.m_staticBoundTxTable != 0);
        if (spy.m_staticBoundTxTable->find(bound, &i))
        {
          // we have seen this object in this session

          // already sent all of its polys, leave
          if (i == 0xFFFFFFFF || i >= (unsigned int)numPoly)
            return;
        }
        else
        {
          spy.m_staticBoundTxTable->insert(bound, 0);
        }

        // choose how many polys to send, record our next index in the table
        t = i + polysTransferredPerCall;
        if (t > numPoly)
        {
          // mark as the last bundle to send, we have < polysTransferredPerCall to post
          t = numPoly;
          spy.m_staticBoundTxTable->replace(bound, 0xFFFFFFFF);
        }
        else
        {
          spy.m_staticBoundTxTable->replace(bound, t);
        }

        rage::Matrix34 instTm = RCC_MATRIX34(inst->GetMatrix());

        for (; i<t; i++)
        {
			// Looks like we're only handling polygons currently.  
			if (bvh->GetPrimitive(i).GetType() == rage::PRIM_TYPE_POLYGON)
		  {
				const rage::phPolygon& poly = bvh->GetPrimitive(i).GetPolygon();
          unsigned int flags = PolyPacket::bSpyPF_IsStaticGeom;

          PolyPacket sgp(flags);

          rage::Vector3 v0, v1, v2, un;
				bvh->GetPolygonVertices(poly, RC_VEC3V(v0), RC_VEC3V(v1), RC_VEC3V(v2));

          instTm.Transform(v0);
          instTm.Transform(v1);
          instTm.Transform(v2);

          sgp.m_vert[0]   = bSpyVec3fromVector3(v0);
          sgp.m_vert[1]   = bSpyVec3fromVector3(v1);
          sgp.m_vert[2]   = bSpyVec3fromVector3(v2);

          bspySendPacket(sgp);
        }
			//else if (bvh->GetPrimitive(i).GetType() == rage::PRIM_TYPE_SPHERE)
			//{
			//}
			//else if (bvh->GetPrimitive(i).GetType() == rage::PRIM_TYPE_CAPSULE)
			//{
			//}
			//else if (bvh->GetPrimitive(i).GetType() == rage::PRIM_TYPE_BOX)
			//{
			//}
			//else if (bvh->GetPrimitive(i).GetType() == rage::PRIM_TYPE_CYLINDER)
			//{
			//}
			//else 
			//{
			//	Assert(0, "Unknown primitive type");
			//}
        }
        return;
      }
    }

    // dynamic object instead?
    if (spy.shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_DynamicCollisionShapes))
    {
      unsigned int frameTicker = spy.getFrameTicker();

      ART::NmRsSpy::DynBoundTracker dbt;
      dbt.frameStamp = 0;
      dbt.sessionStamp = 0;
      Assert(spy.m_dynBoundTxTable != 0);
      if (spy.m_dynBoundTxTable->find(bound, &dbt))
      {
        // already seen this one?
        if (dbt.frameStamp == frameTicker)
          return;
      }
      else
      {
        spy.m_dynBoundTxTable->insert(bound, dbt);
      }

      // update ticker
      dbt.frameStamp = frameTicker;
      spy.m_dynBoundTxTable->replace(bound, dbt);

      rage::Matrix34 instTm = RCC_MATRIX34(inst->GetMatrix());
      rage::Matrix34 partWorld;

      if (boundType == rage::phBound::COMPOSITE)
      {
        rage::phBoundComposite* composite = static_cast<rage::phBoundComposite*>(bound);
        int i, boundCount = composite->GetNumBounds();
        for (i=0; i<boundCount; i++)
        {
          rage::phBound* subBound = composite->GetBound(i);
          if (subBound)
          {
            const rage::Matrix34 &subTm = RCC_MATRIX34(composite->GetCurrentMatrix(i));
            partWorld.Dot(subTm, instTm);

            bSpyProcessDynamicBoundOnContact(inst, subBound, partWorld);
          }
        }
      }
      else
      {
        bSpyProcessDynamicBoundOnContact(inst, bound, instTm);
      }
    }
  }

  void NmRsEngine::bSpyProcessDynamicBoundOnContact(rage::phInst* inst, rage::phBound* bound, const rage::Matrix34& tm)
  {
    rage::phBound::BoundType boundType = (rage::phBound::BoundType)bound->GetType();
    DynamicCollisionShapePacket dcs( (bs_uint32)inst->GetLevelIndex(), (bs_uint32)atDataHash((const char*)&bound, sizeof(bound)), (bs_int16)-1, (bs_int32)inst->GetClassType() );

    if (PHLEVEL->IsFixed(inst->GetLevelIndex()))
      dcs.m_flags |= DynamicCollisionShapePacket::bSpyDO_Fixed;
    if (PHLEVEL->IsInactive(inst->GetLevelIndex()))
      dcs.m_flags |= DynamicCollisionShapePacket::bSpyDO_Inactive;

    switch (boundType)
    {
      // geometry bounds have their mesh sent once and then referenced by 
      // using the bound instance pointer as a UID
    case rage::phBound::GEOMETRY:
      {
        rage::phBoundGeometry* bGeom= static_cast<rage::phBoundGeometry*>(bound);
        int numPoly = bGeom->GetNumPolygons();

        dcs.m_tm = bSpyMat34fromMatrix34(tm);

        dcs.m_shape.m_type = bSpyShapePrimitive::eShapeMeshRef;
        dcs.m_shape.m_data.mesh.m_polyCount = (bs_uint32)(numPoly);
        bspySendPacket(dcs);
#if BSPY_ENDIAN_SWAP
        // unswap this as we re-send it later
        dcs.endianSwap();
        dcs.hdr.endianSwap();
#endif // BSPY_ENDIAN_SWAP

        size_t sessionUID = getNmRsSpy()->getSessionUID();

        NmRsSpy& spy = *getNmRsSpy();
        ART::NmRsSpy::DynBoundTracker dbt;
        dbt.frameStamp = 0;
        dbt.sessionStamp = 0;

        Assert(spy.m_dynBoundTxTable != 0);
        if (!spy.m_dynBoundTxTable->find(bound, &dbt))
        {
          spy.m_dynBoundTxTable->insert(bound, dbt);
        }

        if (dbt.sessionStamp != sessionUID)
        {
          // resend this packet, but redirect to the zero frame.
          // this looks odd, but it is required as the polygon data will end up in the 
          // ZF and the packet must proceed it so that various mesh maps get setup correctly
          dcs.hdr.m_magicB = NM_BSPY_PKT_MAGIC_ZF;
          bspySendPacket(dcs);

          for (int i=0; i<numPoly; i++)
          {
            const rage::phPolygon& poly = bGeom->GetPolygon(i);

            unsigned int flags = PolyPacket::bSpyPF_None;

            PolyPacket sgp(flags);

            rage::Vec3V vec0, vec1, vec2;
            bGeom->GetPolygonVertices(poly, vec0, vec1, vec2);
            rage::Vector3 v0, v1, v2, un;
            v0 = RCC_VECTOR3(vec0);
            v1 = RCC_VECTOR3(vec1);
            v2 = RCC_VECTOR3(vec2);

            sgp.m_vert[0]   = bSpyVec3fromVector3(v0);
            sgp.m_vert[1]   = bSpyVec3fromVector3(v1);
            sgp.m_vert[2]   = bSpyVec3fromVector3(v2);

            bspySendPacket(sgp);
          }

          dbt.sessionStamp = sessionUID;
          spy.m_dynBoundTxTable->replace(bound, dbt);
        }
      }
      break;

    case rage::phBound::SPHERE:
    case rage::phBound::CAPSULE:
    case rage::phBound::BOX:
      {
        rage::Matrix34 bTm;
          tm.Transform(VEC3V_TO_VECTOR3(bound->GetCentroidOffset()), bTm.d);
          bTm.Set3x3(tm);

        dcs.m_tm = bSpyMat34fromMatrix34(bTm);

        phBoundToShapePrimitive(bound, dcs.m_shape);
        bspySendPacket(dcs);
      }
      break;

    default:
      break;
    }
  }

//mmmmBSPY3INTEGRATION Called by the game dll's - remove and just use characterId - NOTE KEEP IN TO BSPY NON-NM CHARACTERS?
  int NmRsEngine::getBSpyIDFromCharacterID(AgentID characterId)
  {
     NmRsCharacter* character = getCharacterFromAgentID(characterId);
    if (character == NULL)
      return -1;
    else
      return character->getBSpyID();
  }

#endif //ART_ENABLE_BSPY



#if (!ART_ENABLE_BSPY) && NM_EMPTY_BSPYSERVER
//mmmmBSPY3INTEGRATION Called by the game dll's - remove and just use characterId - NOTE KEEP IN TO BSPY NON-NM CHARACTERS?
  int NmRsEngine::getBSpyIDFromCharacterID(AgentID /*characterId*/)
  {
    return 0;
  }
#endif//#if (!ART_ENABLE_BSPY) && NM_EMPTY_BSPYSERVER

  bool NmRsEngine::initEngine()
  {

#if ART_ENABLE_BSPY
    if (m_spy->init(true))
      m_spy->startServer();
#endif // ART_ENABLE_BSPY

    return true;
  }

  bool NmRsEngine::termEngine()
  {
#if ART_ENABLE_BSPY
    if (m_spy->isInited())
    {
      m_spy->stopServer();
      m_spy->term();
    }
#endif // ART_ENABLE_BSPY

    return true;
  }

  void NmRsEngine::ResetProfileTimers()
  {
    ms_StepPhase1TimeUS = 0.0f;
    ms_ApplyTransformsTimeUS = 0.0f;
    ms_PreStepTimeUS = 0.0f;
    ms_StepPhase2TimeUS = 0.0f;
    ms_CalcCOMValsTimeUS = 0.0f;
    ms_BehaviorsTimeUS = 0.0f;
    ms_PostStepTimeUS = 0.0f;
  }

  void NmRsEngine::stepPhase1(float deltaTime)
  {
#if __BANK
    rage::utimer_t debugStartTimer1 = rage::sysTimer::GetTicks();
#endif

    if (!m_level)
      return;

    m_framesSinceDyingAgentRemoved = rage::Max(0, m_framesSinceDyingAgentRemoved-1);

    bool haveActiveAgents = m_characterVector.begin() != m_characterVector.end();
    if(haveActiveAgents)
    {
      m_lastKnownUpdateStep = deltaTime;
      m_lastKnownUpdateStepClamped = rage::Max(m_lastKnownUpdateStep, 1.0f/60.f);
      //mmmmmtodo evaluate this for stability at lower framerates than 30fps:
      //m_lastKnownUpdateStepClamped = rage::Clamp(m_lastKnownUpdateStep, 1.0f/60.f, 1.0f/30.f);

      m_gUp = rage::phSimulator::GetGravity();
      m_gUp.Normalize();
      m_gUp.Scale(-1.0f);
    }

#if __BANK
    rage::utimer_t debugStartTimer2 = rage::sysTimer::GetTicks();
#endif
    // update incoming transforms for each agent
    BEGIN_PROFILING("incomingTMs-prefetch");
    if(haveActiveAgents)
    {
      for (NmRsCharacterVector32::iterator it = m_characterVector.begin(), end = m_characterVector.end(); it != end; ++it)
      {
        NmRsCharacter *nmrs = (*it);
        {
#if ART_ENABLE_BSPY
          m_spy->setLastSeenAgent(nmrs->getBSpyID());
#endif // ART_ENABLE_BSPY

          // get character to update incoming transforms if they have been enabled
          nmrs->applyIncomingTransforms(deltaTime);

          // cycle the update mode for the character
          nmrs->updateIncomingTransformApplyMode();

#if ART_ENABLE_BSPY
          m_spy->clearLastSeenAgent();
#endif // ART_ENABLE_BSPY
        }
      }
    }
    END_PROFILING();

#if __BANK
    ms_ApplyTransformsTimeUS += rage::sysTimer::GetTicksToMicroseconds() * (rage::sysTimer::GetTicks() - debugStartTimer2);
#endif

#if __BANK
    rage::utimer_t debugStartTimer3 = rage::sysTimer::GetTicks();
#endif

    // pre-step all the characters
    if(haveActiveAgents)
    {
      for (NmRsCharacterVector32::iterator it = m_characterVector.begin(), end = m_characterVector.end(); it != end; ++it)
      {
        BEGIN_PROFILING("characters");

        NmRsCharacter *nmrs = (*it);
        {
#if ART_ENABLE_BSPY
          m_spy->setLastSeenAgent(nmrs->getBSpyID());
#endif // ART_ENABLE_BSPY

          nmrs->preStep();

#if ART_ENABLE_BSPY
          m_spy->clearLastSeenAgent();
#endif // ART_ENABLE_BSPY
        }

        END_PROFILING();
      }
    }

#if ART_ENABLE_BSPY
    m_spy->sendInfo();
#endif // ART_ENABLE_BSPY


    // Tick the limb manager to free limb request memory
    m_limbManager->tick();

#if __BANK
    ms_PreStepTimeUS += rage::sysTimer::GetTicksToMicroseconds() * (rage::sysTimer::GetTicks() - debugStartTimer3);
#endif

#if ART_ENABLE_BSPY
    if (getNmRsSpy()->isClientConnected())
    {
      for ( int i = m_bSpyObjectsQueue.GetCount()-1 ; i >= 0; i-- )
      {
        if (m_level->IsInLevel(m_bSpyObjectsQueue[i]))
        {
          bool displayObject = true;

          // Don't send information about a character if it is a NMCharacter currently being displayed in bSpy 
          for (NmRsCharacterVector32::iterator it = m_characterVector.begin(), end = m_characterVector.end(); it != end; ++it)
          {
            NmRsCharacter *nmrs = (*it);
            if ( nmrs->getBSpyID() != INVALID_AGENTID &&
                 nmrs->getFirstInstance()->GetLevelIndex() == m_bSpyObjectsQueue[i] )
            {
              displayObject = false;
            }
          }

          if (displayObject)
          {
            rage::phInst* inst = m_level->GetInstance(m_bSpyObjectsQueue[i]);
            if (inst)//mmmmtodo IsInstValid_NoGenIDCheck
              bSpyProcessInstanceOnContact(inst);
          }
        }
        else // Object no longer in scene - cleanUp queue
          m_bSpyObjectsQueue.Delete(i);
      }
    }

    m_spy->endStep(deltaTime);

    m_spy->beginStep(deltaTime);
#endif // ART_ENABLE_BSPY

#if __BANK
    ms_StepPhase1TimeUS += rage::sysTimer::GetTicksToMicroseconds() * (rage::sysTimer::GetTicks() - debugStartTimer1);
#endif
  }

  void NmRsEngine::stepPhase2(float deltaTime)
  {
#if __BANK
    rage::utimer_t debugStartTimer1 = rage::sysTimer::GetTicks();
#endif

    if (!m_level)
      return;

    NmRsCharacter::sm_ApplyForcesImmediately = false;

    bool haveActiveAgents = m_characterVector.begin() != m_characterVector.end();

#if __BANK
    rage::utimer_t debugStartTimer2 = rage::sysTimer::GetTicks();
#endif

    // comCalc all the characters
    if(haveActiveAgents)
    {
      for (NmRsCharacterVector32::iterator it = m_characterVector.begin(), end = m_characterVector.end(); it != end; ++it)
      {
        if((*it)->isInsertedInScene())
        {
          NmRsCharacter *nmrs = (*it);

          nmrs->getArticulatedBody()->EnsureVelocitiesFullyPropagated();

          nmrs->ResetSavedVelocities();

#if ART_ENABLE_BSPY
          m_spy->setLastSeenAgent(nmrs->getBSpyID());
#endif // ART_ENABLE_BSPY

          nmrs->calculateCoMValues();

#if ART_ENABLE_BSPY
          m_spy->clearLastSeenAgent();
#endif // ART_ENABLE_BSPY
        }
      }
    }

#if __BANK
    ms_CalcCOMValsTimeUS += rage::sysTimer::GetTicksToMicroseconds() * (rage::sysTimer::GetTicks() - debugStartTimer2);
#endif

#if __BANK
    rage::utimer_t debugStartTimer3 = rage::sysTimer::GetTicks();
#endif

    // update all CBU tasks
    if(haveActiveAgents)
    {
      BEGIN_PROFILING("CBU");
      m_taskManager->tick(deltaTime);
      END_PROFILING();
    }

#if __BANK
    ms_BehaviorsTimeUS += rage::sysTimer::GetTicksToMicroseconds() * (rage::sysTimer::GetTicks() - debugStartTimer3);
#endif

#if __BANK
    rage::utimer_t debugStartTimer4 = rage::sysTimer::GetTicks();
#endif

    // post-step all the characters
    if(haveActiveAgents)
    {
      for (NmRsCharacterVector32::iterator it = m_characterVector.begin(), end = m_characterVector.end(); it != end; ++it)
      {
        BEGIN_PROFILING("characters");

        NmRsCharacter *nmrs = (*it);
        {
#if ART_ENABLE_BSPY
          m_spy->setLastSeenAgent(nmrs->getBSpyID());
#endif // ART_ENABLE_BSPY

          nmrs->postStep(deltaTime);

          nmrs->ApplyAccumulatedImpulses();

#if ART_ENABLE_BSPY
          m_spy->clearLastSeenAgent();
#endif // ART_ENABLE_BSPY
        }

        END_PROFILING();
      }
    }

    NmRsCharacter::sm_ApplyForcesImmediately = true;

#if __BANK
    ms_PostStepTimeUS += rage::sysTimer::GetTicksToMicroseconds() * (rage::sysTimer::GetTicks() - debugStartTimer4);
#endif

#if __BANK
    ms_StepPhase2TimeUS += rage::sysTimer::GetTicksToMicroseconds() * (rage::sysTimer::GetTicks() - debugStartTimer1);
#endif
  }

  float NmRsEngine::getImpulseModifierUpdate(NmRsCharacter* rsCharacter)
  {
    if (rsCharacter)
    return rsCharacter->m_lastImpulseMultiplier;
	return 0.0f;
  }

  void NmRsEngine::resetImpulseModifierUpdate(NmRsCharacter* rsCharacter)
  {
    Assert(rsCharacter);
    rsCharacter->m_lastImpulseMultiplier = -1.0f;
  }

  /**
  * Retrieves the transformation matrices for the specified agent, copying up to
  * <tt>destLength</tt> of them into <tt>destTMs</tt>. Returns <tt>true</tt> if
  * successful.
  */

  bool NmRsEngine::getComponentTMs(NmRsCharacter* rsCharacter, rage::Matrix34* destTMs, size_t destLength) const
  {
    if (destLength == 0)
      return true; 

    Assert(rsCharacter);

    // ensure the buffer we have been given will comfortably contain all component transforms
    if (destLength < (size_t)rsCharacter->getNumberOfParts())
    {
      NM_RS_LOGERROR(L"ARTContext::getComponentTMs - destination buffer is only %d long, needs to be %d to contain all components", destLength,  agent->getNumberOfParts());
      return false;
    }

    AgentID characterID = rsCharacter->getID();
    return (getComponentTMs(characterID, destTMs) == (int)destLength);
  }

  int NmRsEngine::getComponentTMs(AgentID characterID, rage::Matrix34* dest) const
  {
    if (m_characterByIndex[characterID] == 0)
    {
      NM_RS_LOGERROR(L"NmRsEngine::getComponentTMs() Error : Agent %i not initialized!", agentID);
      return 0;
    }

    NmRsCharacter* character = m_characterByIndex[characterID];

    int numMatrices = character->getNumberOfParts();
    for(int i=0; i < numMatrices; ++i)
    {
      character->getMatrixForPartByComponentIdx(i, dest[i]);
    }

    return numMatrices;
  }

  /**
  * Sets the transformation matrices for the specified agent, so that they may
  * be used by the engines, for example as seed animation data.
  */

  bool NmRsEngine::setComponentTMs(NmRsCharacter* rsCharacter, const rage::Matrix34* src, size_t srcLength, IncomingTransformStatus itmStatusFlags, IncomingTransformSource source)
  {
    if (srcLength == 0)
      return true;

    if(!rsCharacter)
    {
      NM_RS_LOGERROR(L"NmRsEngine::setInitialTM() Error : Agent %i not initialized!", agentID);
      return false;
    }

    rsCharacter->setIncomingTransforms((rage::Matrix34*)src, itmStatusFlags, (int)srcLength, source);
    return true;
  }


  rage::Matrix34* NmRsEngine::GetWorldLastMatrices(ART::AgentID characterID)
  {
    Assert(m_characterByIndex[characterID]);
    return m_characterByIndex[characterID]->GetWorldLastMatrices();
  }

  rage::Matrix34* NmRsEngine::GetWorldCurrentMatrices(ART::AgentID characterID)
  {
    Assert(m_characterByIndex[characterID]);
    return m_characterByIndex[characterID]->GetWorldCurrentMatrices();
  }

#if NM_ANIM_MATRICES
  rage::Matrix34* NmRsEngine::GetBlendOutAnimationMatrices(ART::AgentID characterID)
  {
    Assert(m_characterByIndex[characterID]);
    return m_characterByIndex[characterID]->GetBlendOutAnimationMatrices();
  }

#if ART_ENABLE_BSPY
  rage::Matrix34* NmRsEngine::GetLeadInAnimationMatrices(ART::AssetID assetID)
  {
    Assert(assetID < NUM_ASSETS);
    return m_LeadInAnimationMatrices[assetID];
  }

  /**
  * Sets the animation matrices for use in bSpy.
  */

  void NmRsEngine::setLeadInAnimationTMs(rage::Matrix34* tms, ART::AssetID assetID)
  { 
    Assert(assetID < NUM_ASSETS);
    //Get below from the type
    int numAnimationParts = 21;
    int numChildren = numAnimationParts;//GetTypePhysics()->GetNumChildren();
    // Go through each fragTypeChild/bound component ...
    for (int childIndex = 0; childIndex < numChildren; ++childIndex)
    {
      m_LeadInAnimationMatrices[assetID][childIndex] = tms[childIndex];
    }
  }
#endif
#endif

  /**
  * Sets the current feedback interface for the given agent - NULL is an acceptable value for <tt>iface</tt>
  */
  bool NmRsEngine::setAgentFeedbackInterface(AgentID characterID, ARTFeedbackInterface* iface)
  {
    if(m_characterByIndex[characterID]==0)
    {
      NM_RS_LOGERROR(L"NmRsEngine::setAgentFeedbackInterface: Error - character %i not initialized!", characterID);
      return false;
    }

    m_characterByIndex[characterID]->setFeedbackInterface(iface);
    return true;
  }

  bool NmRsEngine::setInitialTM(ART::AgentID characterID, int componentIdx, const rage::Matrix34 &tm, const rage::Matrix34 *previousFrameTm)
  {
    if (m_characterByIndex[characterID] == 0)
    {
      NM_RS_LOGERROR(L"NmRsEngine::setInitialTM: Error - character %i not initialized!", characterID);
      return false;
    }

    return m_characterByIndex[characterID]->setInitialTM(componentIdx, tm, previousFrameTm);
  }

  NmRsGenericPart *NmRsEngine::getGenericPartForComponentInActor(ART::AgentID characterID, int componentIdx) const
  {
    if(m_characterByIndex[characterID])
    {
      NmRsGenericPart* body = m_characterByIndex[characterID]->getGenericPartByIndex(componentIdx);
      return body;
    }
    return 0;
  }

	void NmRsEngine::setFromAnimationAverageVel(const rage::Vector3 &vel, bool reset) 
	{ 
		if (reset || !m_animationAverageVelSet)
		{
			m_animationAverageVel = vel;  
			m_animationAverageVelSet = !reset; 
		}
  }

  bool NmRsEngine::directInvoke(NmRsCharacter* rsCharacter, InvokeUID iUID, const MessageParamsBase* const params)
  {
#if NM_SCRIPTING
    enum
    {
      mes_activePose,
#if NM_EA
      mes_addPatch,
#endif //NM_EA
      mes_applyImpulse,
      mes_applyBulletImpulse,
      mes_bodyRelax,
      mes_configureBalance,
      mes_configureBalanceReset,
      mes_configureBullets,
      mes_configureExtraBullets,
      mes_configureShotInjuredArm,
      mes_configureShotInjuredLeg,
      mes_defineAttachedObject,
      mes_forceToBodyPart,
      mes_leanInDirection,
      mes_leanRandom,
      mes_leanToPosition,
      mes_leanTowardsObject,
      mes_hipsLeanInDirection,
      mes_hipsLeanRandom,
      mes_hipsLeanToPosition,
      mes_hipsLeanTowardsObject,
      mes_forceLeanInDirection,
      mes_forceLeanRandom,
      mes_forceLeanToPosition,
      mes_forceLeanTowardsObject,
      mes_overrideStartingVelocity,
      mes_setStiffness,
      mes_setMuscleStiffness,
      mes_setWeaponMode,
      mes_registerWeapon,
      mes_shotRelax,
      mes_fireWeapon,
      mes_configureConstraints,
      mes_stayUpright,
      mes_stopAllBehaviours,
      mes_setCharacterStrength,
      mes_setFallingReaction,
      mes_setCharacterUnderwater,
      mes_setCharacterCollisions,
      mes_setCharacterDamping,
      mes_animPose,
      mes_armHang,
      mes_armsWindmill,
      mes_armsWindmillAdaptive,
      mes_balancerCollisionsReaction,
      mes_bodyBalance,
      mes_bodyFoetal,
      mes_bodyRollUp,
      mes_bodyWrithe,
      mes_braceForImpact,
      mes_catchFall,
      mes_dragged,
      mes_electrocute,
      mes_fallOverWall,
      mes_grab,
      mes_hardConstraint,
      mes_headLook,
      mes_highFall,
      mes_incomingTransforms,
      mes_injuredOnGround,
      mes_carried,
      mes_dangle,
      mes_learnedCrawl,
      mes_pedalLegs,
      mes_pointArm,
      mes_relaxUnwind,
      mes_rollDownStairs,
      mes_shootDodge,
      mes_shot,
      mes_shotNewBullet,
      mes_shotSnap,
      mes_shotShockSpin,
      mes_shotFallToKnees,
      mes_shotFromBehind,
      mes_shotInGuts,
      mes_shotHeadLook,
      mes_shotConfigureArms,
      mes_softKeyframe,
      mes_staggerFall,
      mes_teeter,
      mes_upperBodyFlinch,
      mes_yanked,
      mes_pointGun,
      mes_pointGunExtra,
      mes_splitBody,
#if NM_RUNTIME_LIMITS
      mes_configureLimits,
#endif
      mes_lastManStanding,
      mes_buoyancy,
      mes_smartFall,
      numOfMessages
    };
#if ART_ENABLE_BSPY 
    char* m_messageNames[numOfMessages] = 
    {
      "activePose",
#if NM_EA
      "addPatch",
#endif//NM_EA
      "applyImpulse",
      "applyBulletImpulse",
      "bodyRelax",
      "configureBalance",
      "configureBalanceReset",
      "configureBullets",
      "configureExtraBullets",
      "configureShotInjuredArm",
      "configureShotInjuredLeg",
      "defineAttachedObject",
      "forceToBodyPart",
      "leanInDirection",
      "leanRandom",
      "leanToPosition",
      "leanTowardsObject",
      "hipsLeanInDirection",
      "hipsLeanRandom",
      "hipsLeanToPosition",
      "hipsLeanTowardsObject",
      "forceLeanInDirection",
      "forceLeanRandom",
      "forceLeanToPosition",
      "forceLeanTowardsObject",
      "overrideStartingVelocity",
      "setStiffness",
      "setMuscleStiffness",
      "setWeaponMode",
      "registerWeapon",
      "shotRelax",
      "fireWeapon",
      "configureConstraints",
      "stayUpright",
      "stopAllBehaviours",
      "setCharacterStrength",
      "setFallingReaction",
      "setCharacterUnderwater",
      "setCharacterCollisions",
      "setCharacterDamping",
      "animPose",
      "armHang",
      "armsWindmill",
      "armsWindmillAdaptive",
      "balancerCollisionsReaction",
      "bodyBalance",
      "bodyFoetal",
      "bodyRollUp",
      "bodyWrithe",
      "braceForImpact",
      "catchFall",
      "dragged",
      "electrocute",
      "fallOverWall",
      "grab",
      "hardConstraint",
      "headLook",
      "highFall",
      "incomingTransforms",
      "injuredOnGround",
      "carried",
      "dangle",
      "learnedCrawl",
      "pedalLegs",
      "pointArm",
      "relaxUnwind",
      "rollDownStairs",
      "shootDodge",
      "shot",
      "shotNewBullet",
      "shotSnap",
      "shotShockSpin",
      "shotFallToKnees",
      "shotFromBehind",
      "shotInGuts",
      "shotHeadLook",
      "shotConfigureArms",
      "softKeyframe",
      "staggerFall",
      "teeter",
      "upperBodyFlinch",
      "yanked",
      "pointGun",
      "pointGunExtra",
      "splitBody",
#if NM_RUNTIME_LIMITS
      "configureLimits",
#endif
      "lastManStanding",
      "buoyancy",
      "smartFall"
    };
#endif//#if ART_ENABLE_BSPY 
#endif

    if (rsCharacter == 0)
    {
      NM_RS_LOGERROR(L"NmRsEngine::directInvoke() Error : Agent %i not initialized!", agentID);
      return false;
    }

    //mmmmBSPY3INTEGRATION - gives a compiler warning
    //ART::AgentID characterID = rsCharacter->getID();
    //Assert(m_characterByIndex[characterID]);

    NM_RS_DBG_LOGF(L"NmRsEngine::directInvoke - Agent %i | UID %i (0x%08X)", agentID, iUID, iUID);

    if (rsCharacter->hasValidBodyIdentifier())
    {
#if NM_SCRIPTING
      //block messages
      for (int i=0; i<rsCharacter->m_numOfBlockedUIDs; i++)
      {
        if (iUID == rsCharacter->m_blockedUIDs[i])
        {
#if ART_ENABLE_BSPY 
          char str [80];
          sprintf(str,"Unknown");
          //display as blocked in bSpy
          //get behviour name from UID 
          for (int j=0; j<numOfMessages; j++)
          {
            if ((InvokeUID)NMutils::hashString(m_messageNames[j]) == iUID)
              sprintf(str,"#%s",m_messageNames[j]);
          }
          rsCharacter->sendDirectInvoke(str, params);
#endif
          return true;
        }
      }

      //Delay messages
      bool localHitPointInfo = true;
      int partIndex = -1;
      rage::Vector3 hitPoint(0,0,0);
      int hitPointParam = -1, localHitPointInfoParam= -1;
      for (int i=0; i<rsCharacter->m_numOfDelayedUIDs; i++)
      {
        if (iUID == rsCharacter->m_delayedUIDs[i])
        {
#if ART_ENABLE_BSPY 
          char str [80];
          sprintf(str,"Unknown");
          //display as delayed in bSpy
          //get behviour name from UID 
          for (int j=0; j<numOfMessages; j++)
          {
            if ((InvokeUID)NMutils::hashString(m_messageNames[j]) == iUID)
              sprintf(str,"D_%s",m_messageNames[j]);
          }
          rsCharacter->sendDirectInvoke(str, params);
#endif
          //Displayf("Delaying UID...%d", iUID);
          rsCharacter->m_delayedMessageDelay[rsCharacter->m_currentDelayedMessage] = rsCharacter->m_delayForUIDs[i];
          rsCharacter->m_delayedMessageParams[rsCharacter->m_currentDelayedMessage].params.reset();
          for (int j=0; j<params->getUsedParamCount(); j++)
          {

            if (iUID == (InvokeUID)NMutils::hashString("applyBulletImpulse"))
            {
              if (!strcmp(params->getParam(j).m_name,"partIndex"))
              {
                partIndex = params->getParam(j).v.i;
              }
              if (!strcmp(params->getParam(j).m_name,"localHitPointInfo"))
              {
                localHitPointInfo = params->getParam(j).v.b;
                localHitPointInfoParam = j;
              }
              if (!strcmp(params->getParam(j).m_name,"hitPoint"))
              {
                hitPoint.x = params->getParam(j).v.vec[0];
                hitPoint.y = params->getParam(j).v.vec[1];
                hitPoint.z = params->getParam(j).v.vec[2];
                hitPointParam = j;
              }
            }

            //Displayf("Delaying Param...%d od %d", j, params->getUsedParamCount());
            switch (params->getParam(j).m_type)
            {
            case ART::MessageParams::kFloat:
              //Displayf("Delaying Param...Float %s", params->getParam(j).m_name);
              rsCharacter->m_delayedMessageParams[rsCharacter->m_currentDelayedMessage].params.addFloat(params->getParam(j).m_name,params->getParam(j).v.f);
              break;
            case ART::MessageParams::kBool:
              //Displayf("Delaying Param...Bool %s", params->getParam(j).m_name);
              rsCharacter->m_delayedMessageParams[rsCharacter->m_currentDelayedMessage].params.addBool(params->getParam(j).m_name,params->getParam(j).v.b);
              break;
            case ART::MessageParams::kInt:
              //Displayf("Delaying Param...Int %s", params->getParam(j).m_name);
              rsCharacter->m_delayedMessageParams[rsCharacter->m_currentDelayedMessage].params.addInt(params->getParam(j).m_name,params->getParam(j).v.i);
              break;
            case ART::MessageParams::kVector3:
              //Displayf("Delaying Param...Vec %s", params->getParam(j).m_name);
              rsCharacter->m_delayedMessageParams[rsCharacter->m_currentDelayedMessage].params.addVector3(params->getParam(j).m_name,params->getParam(j).v.vec[0],params->getParam(j).v.vec[1],params->getParam(j).v.vec[2]);
              break;
            case ART::MessageParams::kString:
              //Displayf("Delaying Param...String %s", params->getParam(j).m_name);
              rsCharacter->m_delayedMessageParams[rsCharacter->m_currentDelayedMessage].params.addString(params->getParam(j).m_name,params->getParam(j).v.s);
              break;
            case ART::MessageParams::kReference:
            case ART::MessageParams::kUnknown:
            case 0:
              break;
            }
          }          

          //modify global co-ords to local ones if delaying hitpoints
          if (iUID == (InvokeUID)NMutils::hashString("applyBulletImpulse") && !localHitPointInfo)
          {
            rage::Vector3 localHitPoint;
            rage::Matrix34 partMat;
            rsCharacter->getGenericPartByIndex(partIndex)->getMatrix(partMat);
            partMat.UnTransform(hitPoint, localHitPoint);
            rsCharacter->m_delayedMessageParams[rsCharacter->m_currentDelayedMessage].params.getParam(localHitPointInfoParam).v.b = true;
            rsCharacter->m_delayedMessageParams[rsCharacter->m_currentDelayedMessage].params.getParam(hitPointParam).v.vec[0] = localHitPoint.x;
            rsCharacter->m_delayedMessageParams[rsCharacter->m_currentDelayedMessage].params.getParam(hitPointParam).v.vec[1] = localHitPoint.y;
            rsCharacter->m_delayedMessageParams[rsCharacter->m_currentDelayedMessage].params.getParam(hitPointParam).v.vec[2] = localHitPoint.z;
          }

          Displayf("Delaying Param delay...%i",rsCharacter->m_delayedMessageDelay[rsCharacter->m_currentDelayedMessage]);
          rsCharacter->m_delayedMessageUID[rsCharacter->m_currentDelayedMessage] = iUID;
          Displayf("Delaying UID...%d", rsCharacter->m_delayedMessageUID[rsCharacter->m_currentDelayedMessage]);
          rsCharacter->m_currentDelayedMessage++;
          //Displayf("Delaying Param...Done2");
          if (rsCharacter->m_currentDelayedMessage >= 10)
            rsCharacter->m_currentDelayedMessage = 0;
          return false;           
        }
      }
      ////Change message name
      //if (iUID == (InvokeUID)NMutils::hashString("leanInDirection"))//really should check if parameter applyAsForce is true
      //  iUID = (InvokeUID)NMutils::hashString("forceLeanInDirection");
      //if it's got here and is applyBulletImpulse then maybe we want to set ShotNewHit
      if (rsCharacter->m_newHitEachApplyBulletImpulseMessage && iUID == (InvokeUID)NMutils::hashString("applyBulletImpulse"))
      {
        rsCharacter->m_newHitAsApplyBulletImpulseMessage = true;
        rsCharacter->handleDirectInvoke(iUID, params);
        rsCharacter->m_newHitAsApplyBulletImpulseMessage = false;
      }
#endif
      return rsCharacter->handleDirectInvoke(iUID, params);
    }
    else
      return false;
  }

  // -------------------------------------------------------------------------------------------------------------------
  // returns in range 0-1
  float NmRsEngine::perlin3(float x, float y, float z) const
  {
    // !hdd! switched to use RAGE version as it was same as ours, saves
    // having two copies of the perlin input cube
    float rN = 0.5f + rage::PerlinNoise::RawNoise(rage::Vector3(x, y, z));

    // !hdd! when switched to RAGE version we are seeing values returned
    // outside the expected range (by small amounts, perhaps tiny errors
    // introduced by optimisation?) - clamp it so we're sure its 0-1
    return rage::Clamp(rN, 0.0f, 1.0f);
  }

  // -------------------------------------------------------------------------------------------------------------------
  bool NmRsEngine::areDistributedTasksEnabled() const 
  { 
#if ART_ENABLE_BSPY
    // force task threading off for bSpy, so people can instrument functions (eg. IK) 
    return false; 
#else // ART_ENABLE_BSPY
    return m_distributedTasksEnabled; 
#endif // ART_ENABLE_BSPY
  }

  // -------------------------------------------------------------------------------------------------------------------
  void NmRsEngine::setDistributedTasksEnabled(bool onoff) 
  { 
#if ART_ENABLE_BSPY
    // force task threading off for bSpy, so people can instrument functions (eg. IK) 
    onoff = false; 
#endif // ART_ENABLE_BSPY
    m_distributedTasksEnabled = onoff; 
  }

  // -------------------------------------------------------------------------------------------------------------------
  // Creates a new Character based on the specified AssetID. Returns INVALID_AGENTID
  // if unsuccessful. The creation parameters argument, params, can be NULL
  // if there are no parameters.
  //
  NmRsCharacter* NmRsEngine::createCharacter(AssetID assetID, rage::phArticulatedCollider *collider, unsigned randomSeed)
  {
    AgentID characterID;

    // check we have a free slot to plug-in an agent
    if (m_characterIDs.GetCount() == 0)
    {
      NM_RS_LOGERROR(L"ART::AgentBuilder: Cannot create new agent, ran out of agent slots!");
      return 0;
    }

    // fetch a free ID
    characterID = m_characterIDs.Pop();

    // create agent
    if (!initialiseAgent(assetID, characterID, collider, randomSeed))
    {
      NM_RS_LOGERROR(L"NmRsEngine::initialiseAgent() failed");
      return 0;
    }

    return m_characterByIndex[characterID];
  }

  // -------------------------------------------------------------------------------------------------------------------
  // Destroys the specified Character. Returns true< if successful.
  //
  bool NmRsEngine::destroyCharacter(NmRsCharacter* rsCharacter)
  {
    if (!rsCharacter)
      return false;

    AgentID characterID = rsCharacter->getID();

    // check to see if its already been destroyed
    if (m_characterByIndex[characterID] == 0)
    {
      NM_RS_LOGERROR(L"NmRsEngine::destroyCharacter: Error terminating Character %d - already terminated?", characterID);
      return false;
    }

    m_taskManager->term(characterID);

    // put the ID back in the queue for re-use later
    m_characterIDs.Push(characterID);

    bool deletionCheck = false;
    for (NmRsCharacterVector32::iterator it = m_characterVector.begin(), end = m_characterVector.end(); it != end; ++it)
    {
      if (*it == rsCharacter)
      {
        m_characterVector.erase(it);
        deletionCheck = true;
        break;
      }
    }
    if (!deletionCheck)
    {
      NM_RS_LOGERROR(L"NmRsEngine::destroyCharacter: Error deleting character %d - not found in hot list?", characterID);
    }

    // discard the character instance
    ARTCustomPlacementDelete(m_characterByIndex[characterID], NmRsCharacter);
    m_characterByIndex[characterID] = 0;

    return true;
  }

  // -------------------------------------------------------------------------------------------------------------------
  AgentID NmRsEngine::getAgentIDFromCharacter(NmRsCharacter* character) const
  {
    if (character != 0)
      return character->getID();
    else
      return INVALID_AGENTID;
  }

#if ART_ENABLE_BSPY
  // -------------------------------------------------------------------------------------------------------------------
  void NmRsEngine::getAgentStateBlock(AgentState& as) const
  {
    memset(as.m_agentAssetLayout, -1, sizeof(as.m_agentAssetLayout));

    for (NmRsCharacterVector32::const_iterator it = m_characterVector.begin(), end = m_characterVector.end(); it != end; ++it)
    {
      as.m_agentAssetLayout[ (*it)->getID() ] = (bs_int8)(*it)->getAssetID();
    }
  }
#endif // ART_ENABLE_BSPY

}

