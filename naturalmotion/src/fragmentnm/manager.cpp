// 
// fragmentnm/manager.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "diag/tracker.h"
#include "phbound/bound.h"
#include "phbound/boundcomposite.h"
#include "physics/simulator.h"
#include "profile/profiler.h"
#include "system/memory.h"
#include "system/alloca.h"
#include "fragment/cache.h"
#include "system/memory.h"
#include "system/memops.h"
#include "parser/manager.h"
#include "physics/levelnew.h"

#include <map>
#include <vector>
#include <list>

#include <art/artrockstar.h>
#include <art/art.h>
#include <art/MessageParams.h>
#include <nmutils/NMCustomMemory.h>
#include <nmutils/NMCustomMemory.h>
#include <rockstar/NmRsCommon.h>
#include <rockstar/NmRsInclude.h>
#include <rockstar/NmRsEngine.h>
#include <rockstar/NmRsCharacter.h>
#include <rockstar/NmRsGenericPart.h>
#include <rockstar/NmRsEffectors.h>
#include <rockstar/NmRsBodyLayout.h>
#include "manager.h"
#include "instance.h"

#if HACK_GTA4 
#define XMLFILE_DB_VERSION          "10"
#endif // HACK_GTA4

#define MAX_NAME_LENGTH 8

#if HACK_GTA4
#if __BANK || NM_TEST_ENVIRONMENT // NM Test Environment build always loads from other place (otherwise the game crashes for non __DEV bankRelease)
// This exists so that we can load the NM data files from a special folder (set with -nmfolder on the command line)
// which is visible to the guys at Natural Motion - i.e. outside of common.rpf. Used below during NM XML asset loading.
PARAM(nmfolder, "Folder to find natural motion ART files");
#endif // __DEV
#endif // HACK_GTA4

namespace rage {

  namespace phNaturalMotionStats
  {
    PF_PAGE(NaturalMotion,"ph NaturalMotion");

    PF_GROUP(Update);
    PF_LINK(NaturalMotion,Update);
    PF_TIMER(StepPhase1,Update);
    PF_TIMER(StepPhase2,Update);
    PF_TIMER(DynamicBalancer,Update);
    PF_TIMER(BalanceProbes,Update);
	PF_TIMER(FixStuckInGeometry,Update);
  };

  using namespace phNaturalMotionStats;

  fragNMAssetManager* fragNMAssetManager::sm_Instance = NULL;

  void fragNMAssetManager::Create()
  {
    sm_Instance = rage_new fragNMAssetManager;
  }

  void fragNMAssetManager::Destroy()
  {
    delete sm_Instance;
  }

  static void* rageNMCustomMemoryAllocator(size_t mSize, void*, NM_MEMORY_TRACKING_ARGS_DECL_UNUSED)
  {
	RAGE_TRACK(NaturalMotion);
    void* result = sysMemAllocator::GetMaster().Allocate(mSize, 16, MEMTYPE_GAME_VIRTUAL);

    // No longer necessary, done within sysMemAllocator::Allocate(): /FF
    //	RAGE_TRACKING_TALLY( result, MEMTYPE_GAME_VIRTUAL );

    return result;
    // return rage_new char[mSize];
  }

  static void* rageNMCustomMemoryCAllocator(size_t mSize, void*, NM_MEMORY_TRACKING_ARGS_DECL_UNUSED)
  {
    RAGE_TRACK(NaturalMotion);
    // char* newData = rage_new char[mSize];
    void* newData = sysMemAllocator::GetMaster().Allocate(mSize, 16, MEMTYPE_GAME_VIRTUAL);

    // No longer necessary, done within sysMemAllocator::Allocate(): /FF
    //	RAGE_TRACKING_TALLY( newData, MEMTYPE_GAME_VIRTUAL );

    memset(newData,0,mSize);
    return newData;
  }

  static void rageNMCustomMemoryDeallocator(void *mPtr, void*, NM_MEMORY_TRACKING_ARGS_DECL_UNUSED)
  {
    // No longer necessary, done within sysMemAllocator::Free(): /FF
    //	RAGE_TRACKING_UNTALLY( mPtr );

    sysMemAllocator::GetMaster().Free(mPtr);
    //delete [] ((char*)mPtr);
  }

  static void* rageNMCustomMemoryReallocator(void *oldPtr, size_t mSize, void*, NM_MEMORY_TRACKING_ARGS_DECL_UNUSED)
  {
    RAGE_TRACK(NaturalMotion);
    //char* newData=new char[mSize];
    void* newData = sysMemAllocator::GetMaster().Allocate(mSize, 16, MEMTYPE_GAME_VIRTUAL);

    // No longer necessary, done within sysMemAllocator::Allocate(): /FF
    //	RAGE_TRACKING_TALLY( newData, MEMTYPE_GAME_VIRTUAL );

    if (oldPtr!=NULL)
    {
      size_t oldSize = sysMemAllocator::GetMaster().GetSize(oldPtr);
      memcpy(newData,oldPtr,Min(oldSize, mSize));
      //delete [] ((char*)oldPtr);

      // No longer necessary, done within sysMemAllocator::Free(): /FF
      //	RAGE_TRACKING_UNTALLY( oldPtr );

      sysMemAllocator::GetCurrent().Free(oldPtr);
    }

    return newData;
  }

#if 0 //mmmmMP3
  // TG - June 14th 2010 MP3 Integration - Moved bottom 2 function implementations from inlined header 
  // as we don't want to include NmRsEngine.h in the header file as it causes compilation issues.
  Matrix34* fragNMAssetManager::GetWorldLastMatrices(ART::AgentID agent)
  {
    return ART::gRockstarARTInstance->GetWorldLastMatrices(agent);
  }

  Matrix34* fragNMAssetManager::GetWorldCurrentMatrices(ART::AgentID agent)
  {
    return ART::gRockstarARTInstance->GetWorldCurrentMatrices(agent);
  }
#endif

  fragNMAssetManager::fragNMAssetManager()
  {
    m_MemConfig = rage_new NMutils::MemoryConfiguration(
      rageNMCustomMemoryAllocator,
      rageNMCustomMemoryCAllocator,
      rageNMCustomMemoryDeallocator,
      rageNMCustomMemoryReallocator);

    sm_nAssetsLoaded = 0;

    // build memory manager
    m_artMemoryManager = (ART::MemoryManager*)m_MemConfig->m_allocator(sizeof(ART::MemoryManager), m_MemConfig->m_userData, NM_MEMORY_TRACKING_ARGS);
    new (m_artMemoryManager) ART::MemoryManager(m_MemConfig);

    m_NmRsEngineInstance = rage_new ART::NmRsEngine(m_artMemoryManager);
    ART::gRockstarARTInstance = m_NmRsEngineInstance;

    ART::setRockstarEnvironment(PHSIM,PHLEVEL);
    ART::gRockstarARTInstance->initEngine();
    for (int asset = 0; asset < ART::NUM_ASSETS; ++asset)
    {
      m_SuccessfullyLoaded[asset] = false;
    }

    m_AgentPools.Resize(ART::NUM_ASSETS + 1);
    for (int asset = 0; asset < m_AgentPools.GetCount(); ++asset)
    {
      m_AgentPools[asset].Reserve(10);//mmmmhere in mmmmMP3 m_AgentPools[asset].Reserve(ART::NUM_ASSETS);
    }

#if __PFDRAW
	PHLEVEL->sm_ColorChoiceFunc = phLevelNew::ColorChoiceFunc().ResetC<fragNMAssetManager, &fragNMAssetManager::FragNMManagerColorChoice>(this);
#endif
  }

  fragNMAssetManager::~fragNMAssetManager()
  {
    int numAgentTypes = ART::NUM_ASSETS;
    ART::NmRsEngine *engine = ART::gRockstarARTInstance;
    if (engine->m_characterTypeData)
    {
      for (int i=0; i<numAgentTypes; i++)
      {
        m_artMemoryManager->deallocate(engine->m_characterTypeData[i].m_InitialMatrices, NM_MEMORY_TRACKING_ARGS);
        m_artMemoryManager->deallocate(engine->m_characterTypeData[i].m_ComponentToBoneMatrices, NM_MEMORY_TRACKING_ARGS);
        m_artMemoryManager->deallocate(engine->m_characterTypeData[i].m_1DofEffectorParams, NM_MEMORY_TRACKING_ARGS);
        m_artMemoryManager->deallocate(engine->m_characterTypeData[i].m_3DofEffectorParams, NM_MEMORY_TRACKING_ARGS);
#if ART_ENABLE_BSPY
        m_artMemoryManager->deallocate(engine->m_characterTypeData[i].m_JointTokens, NM_MEMORY_TRACKING_ARGS);
        m_artMemoryManager->deallocate(engine->m_characterTypeData[i].m_PartTokens, NM_MEMORY_TRACKING_ARGS);
#endif
      }
      m_artMemoryManager->deallocate(engine->m_characterTypeData, NM_MEMORY_TRACKING_ARGS);
      engine->m_characterTypeData = NULL;
    }

    for (int asset = 0; asset < m_AgentPools.GetCount(); ++asset)
    {
      //	    Assertf(m_AgentPools[asset].GetCapacity() == m_AgentPools[asset].GetCount(), "fragNMAssetManager being destroyed, but %d agents of asset %d still in use", m_AgentPools[asset].GetCapacity() - m_AgentPools[asset].GetCount(), asset);

      for (int agent = 0; agent < m_AgentPools[asset].GetCount(); ++agent)
      {
        ART::gRockstarARTInstance->removeAgent(m_AgentPools[asset][agent]);     
      }

      ART::gRockstarARTInstance->termEngine();

      for (int agent = 0; agent < m_AgentPools[asset].GetCount(); ++agent)
      {
        ART::gRockstarARTInstance->destroyCharacter(ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentPools[asset][agent]));
      }
    }

    delete m_NmRsEngineInstance;
    ART::gRockstarARTInstance = 0;

    ART::MemoryManager *savedMemMgrPtr = m_artMemoryManager;//mmmmmtodo why do this?
    memset((void*)m_artMemoryManager, 0, sizeof(ART::MemoryManager));
    m_artMemoryManager = savedMemMgrPtr;//mmmmmtodo why do this? Surely not good

    delete m_MemConfig;
  }

  void fragNMAssetManager::Load(const char* artPath)//mmmmmRename to loadAssetFile
  {
    // Load the asset data from an XML file
    Displayf("[fragNMAssetManager::Load] Loading %s.xml", artPath );

    // Create the structures that we're going to load into. TODO - number of types and quantities of parts and effectors should be read in first
    int numAgentTypes = ART::NUM_ASSETS;
    int numParts = 21;
    int numJoints = 20;
    ART::NmRsEngine *engine = ART::gRockstarARTInstance;
    if (!engine->m_characterTypeData)
    {
      engine->m_characterTypeData = 
        (ART::NmRsCharacterTypeData*)m_artMemoryManager->callocate(sizeof(ART::NmRsCharacterTypeData) * numAgentTypes, NM_MEMORY_TRACKING_ARGS);
      for (int agentID=0; agentID<numAgentTypes; agentID++)
      {
        engine->m_characterTypeData[agentID].m_Num1DofJoints = 4;
        engine->m_characterTypeData[agentID].m_Num3DofJoints = 16;
        engine->m_characterTypeData[agentID].m_InitialMatrices = 
          (rage::Matrix34*)m_artMemoryManager->callocate(sizeof(rage::Matrix34) * numParts, NM_MEMORY_TRACKING_ARGS);
        engine->m_characterTypeData[agentID].m_1DofEffectorParams = 
          (ART::NmRs1DofEffectorParams*)m_artMemoryManager->callocate(sizeof(ART::NmRs1DofEffectorParams) * 
          engine->m_characterTypeData[agentID].m_Num1DofJoints, NM_MEMORY_TRACKING_ARGS);
        engine->m_characterTypeData[agentID].m_3DofEffectorParams = 
          (ART::NmRs3DofEffectorParams*)m_artMemoryManager->callocate(sizeof(ART::NmRs3DofEffectorParams) * 
          engine->m_characterTypeData[agentID].m_Num3DofJoints, NM_MEMORY_TRACKING_ARGS);
#if ART_ENABLE_BSPY
        engine->m_characterTypeData[agentID].m_PartTokens = 
          (bSpy::bSpyStringToken*)m_artMemoryManager->callocate(sizeof(bSpy::bSpyStringToken) * numParts, NM_MEMORY_TRACKING_ARGS);
        engine->m_characterTypeData[agentID].m_JointTokens = 
          (bSpy::bSpyStringToken*)m_artMemoryManager->callocate(sizeof(bSpy::bSpyStringToken) * numJoints, NM_MEMORY_TRACKING_ARGS);
#endif
        engine->m_characterTypeData[agentID].m_ComponentToBoneMatrices = NULL;
        memset(engine->m_characterTypeData[agentID].m_Identifier, 0, sizeof(char) * MAX_NAME_LENGTH);
      }
    }

    // Load XML asset data
    parTree* pXmlTree = NULL;
    atString assetFileName(artPath);
    assetFileName += ".xml";

    INIT_PARSER;

#if HACK_GTA4
    const char* pNmPath = "common:/data/naturalmotion";
#if __BANK || NM_TEST_ENVIRONMENT // NM Test Environment build always loads from other place (otherwise the game crashes for non __DEV bankRelease)
    // This difference exists so that we can load the NM data files from a special folder (set with -nmfolder on the command line)
    // which is visible to the guys at Natural Motion - i.e. outside of common.rpf.
    PARAM_nmfolder.Get(pNmPath);
#endif // __DEV
    ASSET.PushFolder(pNmPath);
#else // HACK_GTA4
    ASSET.PushFolder("common:/data/naturalmotion");
#endif // HACK_GTA4
    pXmlTree = PARSER.LoadTree(assetFileName.c_str(), "xml");
    Assert(pXmlTree);
    parTreeNode* pNode = pXmlTree->GetRoot();

    // Check the name and ID
    int assetID = atoi(pNode->GetChild()->GetSibling()->GetData());
    Assert(assetID < numAgentTypes);
    Assert( (strcmp(pNode->GetChild()->GetData(), "Fred") == 0 && assetID == 0) ||
      (strcmp(pNode->GetChild()->GetData(), "Wilma") == 0 && assetID == 1) ||
	  (strcmp(pNode->GetChild()->GetData(), "FrLarge") == 0 && assetID == 2) ||
	  (strcmp(pNode->GetChild()->GetData(), "WiLarge") == 0 && assetID == 3) ||
	  (strcmp(pNode->GetChild()->GetData(), "Alien") == 0 && assetID == 4) );
    safecpy(engine->m_characterTypeData[assetID].m_Identifier, pNode->GetChild()->GetData(), MAX_NAME_LENGTH);

    // Load some buoyancy modifiers (part modifiers handled in part loop)
    pNode = pNode->GetChild()->GetSibling()->GetSibling();
    engine->m_characterTypeData[assetID].m_BodyBuoyancyMultiplier = (float)atof(pNode->GetData()); 
    pNode = pNode->GetSibling();
    engine->m_characterTypeData[assetID].m_DragMultiplier = (float)atof(pNode->GetData()); 
    pNode = pNode->GetSibling();
    engine->m_characterTypeData[assetID].m_WeightBeltMultiplier = (float)atof(pNode->GetData()); 

    // Load joints
    int curr1DofIndex = 0, curr3DofIndex = 0;
    pNode = pNode->GetSibling();
    parTreeNode* pSubNode = NULL;
#if ART_ENABLE_BSPY
    char buf[50]; 
#endif
    for (int i=0; i<numJoints; i++)
    {	
      Assert(i == (int)atoi(pNode->GetChild()->GetData()));
      pSubNode = pNode->GetChild()->GetSibling();  
#if ART_ENABLE_BSPY
      engine->m_characterTypeData[assetID].m_JointTokens[i] = engine->getNmRsSpy()->getTokenForString(
        pNode->GetElement().FindAttributeStringValue("name", "not_found", buf, 50));
#endif
      if (strcmp(pNode->GetElement().GetName(), "threedof") == 0)
      {
        ART::NmRs3DofEffectorParams &info = engine->m_characterTypeData[assetID].m_3DofEffectorParams[curr3DofIndex++];
        info.childIndex = i+1;
        info.parentIndex = (int)atoi(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.minFirstLeanAngle = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.maxFirstLeanAngle = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.minSecondLeanAngle = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.maxSecondLeanAngle = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.minTwistAngle = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.maxTwistAngle = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.reverseFirstLeanMotor = strcmp(pSubNode->GetData(), "true") == 0; pSubNode=pSubNode->GetSibling();
        info.reverseSecondLeanMotor = strcmp(pSubNode->GetData(), "true") == 0; pSubNode=pSubNode->GetSibling();
        info.reverseTwistMotor = strcmp(pSubNode->GetData(), "true") == 0; pSubNode=pSubNode->GetSibling();
        info.softLimitFirstLeanMultiplier = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.softLimitSecondLeanMultiplier = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.softLimitTwistMultiplier = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.m_defaultLeanForceCap = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.m_defaultTwistForceCap = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.m_defaultMuscleStiffness = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.m_defaultMuscleStrength = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.m_defaultMuscleDamping = (float)atof(pSubNode->GetData()); 
      }
      else if (strcmp(pNode->GetElement().GetName(), "onedof") == 0)
      {
        ART::NmRs1DofEffectorParams &info = engine->m_characterTypeData[assetID].m_1DofEffectorParams[curr1DofIndex++];
        info.childIndex = i+1;
        info.parentIndex = (int)atoi(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.minAngle = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.maxAngle = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.m_defaultForceCap = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.m_defaultMuscleStiffness = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.m_defaultMuscleStrength = (float)atof(pSubNode->GetData()); pSubNode=pSubNode->GetSibling();
        info.m_defaultMuscleDamping = (float)atof(pSubNode->GetData()); 
      }
      else
        AssertMsg(0, "fragNMAssetManager::Load - unrecognized joint type");

      pNode = pNode->GetSibling();
    }

    // Load parts
    for (int currPart=0; currPart<numParts; currPart++)
    {
      Assert(currPart == (int)atoi(pNode->GetChild()->GetData()));
#if ART_ENABLE_BSPY
      engine->m_characterTypeData[assetID].m_PartTokens[currPart] = engine->getNmRsSpy()->getTokenForString(
        pNode->GetElement().FindAttributeStringValue("name", "not_found", buf, 50));
#endif
      engine->m_characterTypeData[assetID].m_PartBuoyancyMultipliers[currPart] = (float)atof(pNode->GetChild()->GetSibling()->GetData()); 
      pSubNode = pNode->GetChild()->GetSibling()->GetSibling()->GetChild();  
      rage::Matrix34& matrix = engine->m_characterTypeData[assetID].m_InitialMatrices[currPart];
      matrix.a.x = pSubNode->GetElement().FindAttributeFloatValue("c0", -7.7f);
      matrix.a.y = pSubNode->GetElement().FindAttributeFloatValue("c1", -7.7f);
      matrix.a.z = pSubNode->GetElement().FindAttributeFloatValue("c2", -7.7f);
      matrix.a.w = 0.0f;
      pSubNode= pSubNode->GetSibling();
      matrix.b.x = pSubNode->GetElement().FindAttributeFloatValue("c0", -7.7f);
      matrix.b.y = pSubNode->GetElement().FindAttributeFloatValue("c1", -7.7f);
      matrix.b.z = pSubNode->GetElement().FindAttributeFloatValue("c2", -7.7f);
      matrix.b.w = 0.0f;
      pSubNode= pSubNode->GetSibling();
      matrix.c.x = pSubNode->GetElement().FindAttributeFloatValue("c0", -7.7f);
      matrix.c.y = pSubNode->GetElement().FindAttributeFloatValue("c1", -7.7f);
      matrix.c.z = pSubNode->GetElement().FindAttributeFloatValue("c2", -7.7f);
      matrix.c.w = 0.0f;
      pSubNode= pSubNode->GetSibling();
      matrix.d.x = pSubNode->GetElement().FindAttributeFloatValue("c0", -7.7f);
      matrix.d.y = pSubNode->GetElement().FindAttributeFloatValue("c1", -7.7f);
      matrix.d.z = pSubNode->GetElement().FindAttributeFloatValue("c2", -7.7f);
      matrix.d.w = 0.0f;
      matrix.Normalize();
      pNode = pNode->GetSibling();
    }

    m_SuccessfullyLoaded[assetID] = true;

    // We are done with the XML tree.
    delete pXmlTree;
    pXmlTree = 0;

    ASSET.PopFolder();

    SHUTDOWN_PARSER;
    /**********************************************************************************************************************/

    // Register the character instance with NmRsEngine singleton. This management class will be responsible
    // for deleting the character instances on destruction.
    //Assert(assetId < ART::gRockstarARTInstance->getMaxAssets());
    //Assertf(assetId == sm_nAssetsLoaded, "Assigned asset ID doesn't match expectations.");
    sm_nAssetsLoaded++;
  }

  void fragNMAssetManager::InitSelfCollisionsGroups(rage::fragPhysicsLOD *physicsLOD)
  {
	  // Init the default collision set
	  const int collisionGroupSize = 256;
	  ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideA.Reserve(collisionGroupSize);
	  ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideB.Reserve(collisionGroupSize);
	  for (int sc = 0; sc < physicsLOD->GetNumSelfCollisions(); ++sc)
	  {
		  if (physicsLOD->GetSelfCollisionA(sc) < physicsLOD->GetNumChildGroups() &&
			  physicsLOD->GetSelfCollisionB(sc) < physicsLOD->GetNumChildGroups())
		  {
			  fragTypeGroup* groupA = physicsLOD->GetGroup(physicsLOD->GetSelfCollisionA(sc));
			  fragTypeGroup* groupB = physicsLOD->GetGroup(physicsLOD->GetSelfCollisionB(sc));

			  if (groupA != groupB)
			  {
				  // Loop over all the groups of each self collision, and set every child of
				  // each to collide with every child of the other
				  for (u8 gai = 0; gai < groupA->GetNumChildren(); ++gai)
				  {
					  for (u8 gbi = 0; gbi < groupB->GetNumChildren(); ++gbi)
					  {
						  rage::phArticulatedCollider::SetPartsCanCollide(
							  ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideA, ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideB, 
							  groupA->GetChildFragmentIndex() + gai, groupB->GetChildFragmentIndex() + gbi, true, false);
					  }
				  }
			  }
		  }
	  }
	  rage::phArticulatedCollider::FinalizeSettingPartsCanCollide(ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideA, ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideB);

	  // Create the collision set used in point gun
	  ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideA.Resize(ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideA.GetCount());
	  ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideB.Resize(ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideB.GetCount());
	  sysMemCpy(&(ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideA[0]), &(ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideA[0]), sizeof(rage::u8) * ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideA.GetCount());
	  sysMemCpy(&(ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideB[0]), &(ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideB[0]), sizeof(rage::u8) * ART::NmRsEngine::m_DefaultSelfCollisionSet.m_PartCanCollideB.GetCount());
	  rage::phArticulatedCollider::SetPartsCanCollide(
		  ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideA, ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideB, 
		  ART::gtaHand_Left, ART::gtaHand_Right, false, false);

	  // Create the collision set used in flinch
	  ART::NmRsEngine::m_FlinchSelfCollisionSet.m_PartCanCollideA.Resize(ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideA.GetCount());
	  ART::NmRsEngine::m_FlinchSelfCollisionSet.m_PartCanCollideB.Resize(ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideB.GetCount());
	  sysMemCpy(&(ART::NmRsEngine::m_FlinchSelfCollisionSet.m_PartCanCollideA[0]), &(ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideA[0]), sizeof(rage::u8) * ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideA.GetCount());
	  sysMemCpy(&(ART::NmRsEngine::m_FlinchSelfCollisionSet.m_PartCanCollideB[0]), &(ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideB[0]), sizeof(rage::u8) * ART::NmRsEngine::m_PointGunSelfCollisionSet.m_PartCanCollideB.GetCount());
	  rage::phArticulatedCollider::SetPartsCanCollide(
		  ART::NmRsEngine::m_FlinchSelfCollisionSet.m_PartCanCollideA, ART::NmRsEngine::m_FlinchSelfCollisionSet.m_PartCanCollideB, 
		  ART::gtaHand_Left, ART::gtaLower_Arm_Right, false, false);
	  rage::phArticulatedCollider::SetPartsCanCollide(
		  ART::NmRsEngine::m_FlinchSelfCollisionSet.m_PartCanCollideA, ART::NmRsEngine::m_FlinchSelfCollisionSet.m_PartCanCollideB, 
		  ART::gtaHand_Right, ART::gtaLower_Arm_Left, false, false);
  }

  void fragNMAssetManager::AssignSelfCollisionSet(rage::phArticulatedCollider *artCollider, ART::SelfCollisionSet *selfCollisionSet)
  {
		ART::NmRsEngine::AssignSelfCollisionSet(artCollider, selfCollisionSet);
  }

  void fragNMAssetManager::DestroyCharacter(ART::AssetID assetID)
  {
    int numOfCharacters = m_AgentPools[assetID].GetCount();
    if (numOfCharacters > 0)
    {
      ART::gRockstarARTInstance->destroyCharacter(ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentPools[assetID][numOfCharacters-1]));
      m_AgentPools[assetID].Delete(numOfCharacters-1);
    }
  }

  void fragNMAssetManager::EnableAPILogging(bool /*enable*/ /*= true*/)//mmmmnoART mmmmtodo cannot remove called by PedDebugVisualiser.obj
  {
  }

  void fragNMAssetManager::SetIncomingAnimVelocityScale(float fScale)
  {
    ART::setIncomingAnimationVelocityScale(fScale);
  }

  ART::AgentID fragNMAssetManager::GetAgent(ART::AssetID assetID, phArticulatedCollider *collider, unsigned randomSeed)
  {
#ifdef ON_DEMAND_AGENT       
    ART::NmRsCharacter* agent = ART::gRockstarARTInstance->createCharacter(assetID, collider, randomSeed);
    return ART::gRockstarARTInstance->getAgentIDFromCharacter(agent);
#else //ON_DEMAND_AGENT 
    (void) collider;
    (void) randomSeed;
    //From pool
    if (m_AgentPools[assetID].GetCount() > 0)
    {
      return m_AgentPools[assetID].Pop();
    }
    else
    {
      return ART::INVALID_AGENTID;
    }
#endif //not ON_DEMAND_AGENT 
  }

  //Get the amount of agents of type AssetID that could be inserted now
  int fragNMAssetManager::GetAgentCount(ART::AssetID assetId)
  {
    //CTaskNMBehaviour::CanUSeRagdoll calls GetAgentCapacity and GetAgentCount more than once to work out whether it can use an NMCharacter
    // I can't see this code so will have to liase with R* to change this. 
#ifdef ON_DEMAND_AGENT
    (void)assetId; //mmmmtodo compiler hack
    return ART::MAX_AGENTS - ART::gRockstarARTInstance->GetNumInsertedCharacters();
#else
    return m_AgentPools[assetId].GetCount();
#endif
  }

  // Inform that a dying ragdoll has been removed this frame
  void fragNMAssetManager::SetDyingAgentRemoved()
  {
    static int waitFrames = 40;
    ART::gRockstarARTInstance->m_framesSinceDyingAgentRemoved = waitFrames;
  }

  bool fragNMAssetManager::GetDyingAgentRemovedRecently() const
  {
    return ART::gRockstarARTInstance->m_framesSinceDyingAgentRemoved > 0;
  }

  //Get the total amount of agents of type AssetID available
    //CTaskNMBehaviour::CanUSeRagdoll calls GetAgentCapacity and GetAgentCount more than once to work out whether it can use an NMCharacter
    // I can't see this code so will have to liase with R* to change this. 
  int fragNMAssetManager::GetAgentCapacity(ART::AssetID UNUSED_PARAM(assetId))
  {
    //CTaskNMBehaviour::CanUSeRagdoll calls GetAgentCapacity and GetAgentCount more than once to work out whether it can use an NMCharacter
    // I can't see this code so will have to liase with R* to change this. 
#ifdef ON_DEMAND_AGENT
    return ART::MAX_AGENTS;//HACK so that shot works with on_demand
#else
    return m_AgentPools[assetId].GetCount();//m_AgentPools[assetId].GetCapacity();//mmmmHACK to get resizable/oversize character pools to work
#endif
  }

  int fragNMAssetManager::GetNonNMRagdollCapacity()
  {
    return ART::MAX_NON_NM_RAGDOLLS;
  }

  int fragNMAssetManager::GetNumActiveNonNMRagdolls()
  {
    return ART::gRockstarARTInstance->GetActiveNonNMRagdolls();
  }

  void fragNMAssetManager::IncrementNonNMRagdollCount()
  {
    ART::gRockstarARTInstance->IncrementNonNMRagdoll();
  }

  void fragNMAssetManager::DecrementNonNMRagdollCount()
  {
    ART::gRockstarARTInstance->DecrementNonNMRagdoll();
  }

  bool fragNMAssetManager::IsOnGround(int agentId)
  {
    ART::NmRsCharacter* character = ART::gRockstarARTInstance->getCharacterFromAgentID(agentId);
    if (character)
      return character->IsOnGround();

    return false;
  }

  float fragNMAssetManager::GetPartBuoyancyMultiplier(int assetID, int part)
  {
    return ART::gRockstarARTInstance->m_characterTypeData[assetID].m_PartBuoyancyMultipliers[part];
  }
  float fragNMAssetManager::GetBodyBuoyancyMultiplier(int assetID)
  {
    return ART::gRockstarARTInstance->m_characterTypeData[assetID].m_BodyBuoyancyMultiplier;
  }
  float fragNMAssetManager::GetDragMultiplier(int assetID)
  {
    return ART::gRockstarARTInstance->m_characterTypeData[assetID].m_DragMultiplier;
  }
  float fragNMAssetManager::GetWeightBeltMultiplier(int assetID)
  {
    return ART::gRockstarARTInstance->m_characterTypeData[assetID].m_WeightBeltMultiplier;
  }
  void fragNMAssetManager::SetPartBuoyancyMultiplier(int assetID, int part, float set)
  {
    ART::gRockstarARTInstance->m_characterTypeData[assetID].m_PartBuoyancyMultipliers[part] = set;
  }
  void fragNMAssetManager::SetBodyBuoyancyMultiplier(int assetID, float set)
  {
    ART::gRockstarARTInstance->m_characterTypeData[assetID].m_BodyBuoyancyMultiplier = set;
  }
  void fragNMAssetManager::SetDragMultiplier(int assetID, float set)
  {
    ART::gRockstarARTInstance->m_characterTypeData[assetID].m_DragMultiplier = set;
  }
  void fragNMAssetManager::SetWeightBeltMultiplier(int assetID, float set)
  {
    ART::gRockstarARTInstance->m_characterTypeData[assetID].m_WeightBeltMultiplier = set;
  }

#ifdef ON_DEMAND_AGENT
  void fragNMAssetManager::RecycleAgent(ART::AssetID UNUSED_PARAM(assetId), ART::AgentID UNUSED_PARAM(agentId))
  {
  }
#else
  void fragNMAssetManager::RecycleAgent(ART::AssetID UNUSED_PARAM(assetId), ART::AgentID UNUSED_PARAM(agentId))
  {
    if (agentId != ART::INVALID_AGENTID)
    {
      m_AgentPools[assetId].Append() = agentId;
    }
    //mmmmhere in mmmmMP3
    //#else
    //	assetId = assetId;
    //	agentId = agentId;
  }
#endif//ON_DEMAND_AGENT

  bool fragNMAssetManager::InsertAgent(int agentId)
  {
    return ART::gRockstarARTInstance->insertAgent(agentId);
  }

  bool fragNMAssetManager::RemoveAgent(int agentId)
  {
#ifdef ON_DEMAND_AGENT       
    ART::gRockstarARTInstance->removeAgent(agentId);
    return ART::gRockstarARTInstance->destroyCharacter(ART::gRockstarARTInstance->getCharacterFromAgentID(agentId));
#else //ON_DEMAND_AGENT 
    //remove from scene (leave in pool)
    return ART::gRockstarARTInstance->removeAgent(agentId);
#endif //not ON_DEMAND_AGENT 
  }

  void fragNMAssetManager::ResetProfileTimers()
  {
    ART::gRockstarARTInstance->ResetProfileTimers();
  }

  void fragNMAssetManager::StepPhase1(float timeStep)
  {
    PF_FUNC(StepPhase1);
    ART::gRockstarARTInstance->stepPhase1(timeStep);
  }

  void fragNMAssetManager::StepPhase2(float timeStep)
  {
    PF_FUNC(StepPhase2);
    ART::gRockstarARTInstance->stepPhase2(timeStep);
  }

  void fragNMAssetManager::SetDistributedTasksEnabled(bool enable /*= true*/)
  {
    ART::setDistributedTasksEnabled(enable);
  }

  bool fragNMAssetManager::AreDistributedTasksEnabled() const 
  { 
    return ART::areDistributedTasksEnabled(); 
  }

  void fragNMAssetManager::setbSpyObject(int levelIndex)
  {
    ART::setbSpyObject(levelIndex);
  }

  bool fragNMAssetManager::SetFromAnimationMaxSpeed(float speed) const 
  { 
    return ART::setFromAnimationMaxSpeed(speed);
  }

  bool fragNMAssetManager::SetFromAnimationMaxAngSpeed(float angSpeed) const 
  { 
    return ART::setFromAnimationMaxAngSpeed(angSpeed);
  }

  bool fragNMAssetManager::SetFromAnimationAverageVel(const rage::Vector3 &vel, bool reset) const
  { 
    return ART::setFromAnimationAverageVel(vel, reset);
  }

#if __PFDRAW
  void fragNMAssetManager::FragNMManagerColorChoice(const phInst* inst) const
  {
	  switch(PHLEVEL->GetState(inst->GetLevelIndex()))
	  {
	  case phLevelBase::OBJECTSTATE_ACTIVE:
		  {
			  phCollider* collider = PHSIM->GetActiveCollider(inst->GetLevelIndex());

			  phSleep* sleep = PHSIM->ColliderIsPermanentlyActive(collider) ? NULL : collider->GetSleep();
			  float sleepPercent = sleep ? Min(1.0f, sleep->GetPercentTotal()) : 0.0f;

			  if (collider->IsArticulated())
			  {
				  const fragInstNM *instNM = dynamic_cast<const fragInstNM*>(inst);
				  if (instNM && instNM->GetNMAgentID() >= 0)
				  {
					grcColor(Lerp(sleepPercent, Color_LightBlue1, Color_yellow));
				  }
				  else
				  {
					grcColor(Lerp(sleepPercent, Color_blue, Color_yellow));
				  }
			  }
			  else if (collider->CanBeArticulated())
			  {
				  grcColor(Lerp(sleepPercent, Color_purple, Color_yellow));
			  }
			  else
			  {
				  grcColor(Lerp(sleepPercent, Color_red, Color_yellow));
			  }

			  break;
		  }
	  case phLevelBase::OBJECTSTATE_INACTIVE:
		  {
			  grcColor(Color_green);
			  break;
		  }
	  default:
		  {
			  grcColor(Color_grey);
			  break;
		  }
	  }
  }
#endif // __PFDRAW

} // namespace rage
