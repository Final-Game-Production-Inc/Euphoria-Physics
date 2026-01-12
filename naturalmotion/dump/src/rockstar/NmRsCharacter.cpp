/*
* Copyright (c) 2005-2010 NaturalMotion Ltd. All rights reserved.
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
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsUtils.h"
#include "NmRsGenericPart.h"
#include "NmRsEffectors.h"
#include "NmRsCBU_TaskManager.h"
#include "NmRsPhInstNM.h"

#include "phcore/materialmgr.h"
#include "phbound/boundbvh.h"
#include "system/timer.h"
#include "art/ARTfeedback.h"

#include "NmRsCBU_DynamicBalancer.h" 
#include "system/timer.h"

#include "fragment/cache.h"

#include "fragmentnm/nm_channel.h"

#if NM_SCRIPTING
#include "nmutils/TypeUtils.h" // only to get hashString for NM_SCRIPTING
#endif
 
namespace ART
{

#ifdef NM_RS_CBU_ASYNCH_PROBES
  rage::phAsyncShapeTestMgr*	NmRsCharacter::sm_AsyncShapeTestMgr = NULL;
#endif // NM_RS_CBU_ASYNCH_PROBES

  // forward declaration of global functions so they can be used in this file (they're not defined in a header file that could be included).
  void stayUprightUpdate(const MessageParamsBase* const params, NmRsCharacter *character);
  void setFallingReactionUpdate(const MessageParamsBase* const params, NmRsCharacter *character);
  void setCharacterCollisionsUpdate(const MessageParamsBase* const params, NmRsCharacter *character);
  void setCharacterDampingUpdate(const MessageParamsBase* const params, NmRsCharacter *character);
#if NM_USE_IK_SELF_AVOIDANCE
  void configureSelfAvoidanceUpdate(const MessageParamsBase* const params, NmRsCharacter *character);
#endif //NM_USE_IK_SELF_AVOIDANCE
  void configureBulletsUpdate(const MessageParamsBase* const params, NmRsCharacter *character);
  void configureBulletsExtraUpdate(const MessageParamsBase* const params, NmRsCharacter *character);

#define NM_RS_ALLOC_STORAGE(ptr, type, count) { ptr = (type*)m_artMemoryManager->callocate(sizeof(type) * count, NM_MEMORY_TRACKING_ARGS); }
#define NM_RS_DEALLOC_STORAGE(ptr)            { if (ptr) m_artMemoryManager->deallocate(ptr, NM_MEMORY_TRACKING_ARGS); ptr = 0; }

  int NmRsCharacter::sm_ImpulsePendingForLink[];
  bool NmRsCharacter::sm_ApplyForcesImmediately = true;

#if NM_UNUSED_CODE
  // used to encode a generic part index into a void* userdata value
  // that is then set into phInst or ArticulatedBodyPart instances
  inline void* encodeUserData(int gpIndex)
  {
    Assert(gpIndex >= 0 && gpIndex < 255);

    void* returnPtr = 0;
    unsigned char *perByte = reinterpret_cast<unsigned char*>(&returnPtr);

    perByte[0] = 'N';
    perByte[1] = 'M';
    perByte[2] = static_cast<unsigned char>(gpIndex);

    return returnPtr;
  }

  // decodes the results of encodeUserData, returning true if the data is
  // valid; generic part index is returned as an argument
  inline bool decodeUserData(void* data, int& gpIndex)
  {
    unsigned char *perByte = reinterpret_cast<unsigned char*>(&data);

    if (perByte[0] == 'N' &&
      perByte[1] == 'M')
    {
      gpIndex = static_cast<int>(perByte[2]);
      return true;
    }

    return false;
  }
#endif

  NmRsCharacter::NmRsCharacter(ART::MemoryManager* services, NmRsEngine *rsEngine, AssetID asset, AgentID characterID, unsigned randomSeed) : m_minMuscleDamping(5.f),
    m_genericPartCount(0),
    m_effectorCount(0),
    m_1dofCount(0),
    m_3dofCount(0),
    m_collisionPairCount(0),
    m_asset(asset),
    m_parts(0),
    m_articulatedWrapper(0),
    m_effectors(0),
    m_1dofEffectors(0),
    m_3dofEffectors(0),
    m_collisionPairs(0),
    m_simulator(0),
    m_level(0),
    m_applyMode(kDisabled),
    m_fscLeftTwist(0),
    m_fscRightTwist(0),
    m_maskCodeStackIdx(0),
    m_bodyIdent(notSpecified),
#if ART_ENABLE_BSPY
    m_skeletonVizMode(kSV_None),
#endif // ART_ENABLE_BSPY
    m_agentID(characterID),
    m_rsEngine(rsEngine),
    m_cbuTaskManager(0),
    m_cbuRecord(0),
    m_artMemoryManager(services),
    m_footSlipCompensationActive(true),
    m_footROrientationInitialized(false),
    m_footLOrientationInitialized(false),
    m_ZMPPostureControlActive(true),
    m_isInsertedInScene(false),
    m_incomingAnimationVelocityScale(1.f),
    m_feedbackInterface(0),
    m_body(services)
  {
    memset(m_identifier, 0, sizeof(char) * 8);

    initialiseData();

    m_incomingTm[kITSourceCurrent] = 0;
    m_incomingTm[kITSourcePrevious] = 0;
#if NM_ANIM_MATRICES
    m_incomingTm[kITSourceAnimation] = 0;
#endif
    m_incomingTmStatus = kITSNone;
    m_incomingTmCount = 0;

    for(int i=0; i<kNumNmRsHumanLimbs; i++)
      m_limbs[i] = 0;

    // Replaced Scale(0) with explicit zero assignment as QNAN * 0 = QNAN was causing a crash on PC
    m_uprightConstraint.m_uprightPelvisPosition.Zero();

    // in debug, synch all the random seed generators
    // mmmmtodo Do this for unit tests as well - perhaps seed per behaviour?
#ifdef _DEBUG
    randomSeed = 0xBAD;
#endif // _DEBUG
    if (randomSeed == 0)
      m_random.Reset(rage::sysTimer::GetSystemMsTime());
    else
      m_random.Reset(randomSeed);
  }

  NmRsCharacter::~NmRsCharacter()
  {
    Assert(m_level == 0);
#ifdef NM_RS_CBU_ASYNCH_PROBES
    ClearAllProbes();
    //ShutDown - This shouldn't happen here
    //if (GetAsyncShapeTestMgr())
    //{
    //GetAsyncShapeTestMgr()->Shutdown();
    //delete GetAsyncShapeTestMgr();
    //SetAsyncShapeTestMgr(NULL);   
    //}
#endif // NM_RS_CBU_ASYNCH_PROBES
    freeAllocatedResources();
    deallocateStorage();

    delete[] m_WorldCurrentMatrices;
    delete[] m_WorldLastMatrices;
#if NM_ANIM_MATRICES
    delete[] m_BlendOutAnimationMatrices;
#endif

#if NM_SCRIPTING
    if (m_pNMScriptFile)
      fclose(m_pNMScriptFile);
#endif
  }

  void NmRsCharacter::allocateStorage()
  {
    NM_RS_ALLOC_STORAGE(m_parts, NmRsGenericPart*, m_genericPartCount);

    NM_RS_ALLOC_STORAGE(m_effectors, NmRsEffectorBase*, m_effectorCount);
    NM_RS_ALLOC_STORAGE(m_1dofEffectors, NmRs1DofEffector*, m_1dofCount);
    NM_RS_ALLOC_STORAGE(m_3dofEffectors, NmRs3DofEffector*, m_3dofCount);
  }

  void NmRsCharacter::deallocateStorage()
  {
    NM_RS_DEALLOC_STORAGE(m_collisionPairs);

    NM_RS_DEALLOC_STORAGE(m_3dofEffectors);
    NM_RS_DEALLOC_STORAGE(m_1dofEffectors);
    NM_RS_DEALLOC_STORAGE(m_effectors);

    NM_RS_DEALLOC_STORAGE(m_parts);
  }

  void NmRsCharacter::freeAllocatedResources()
  {
    int i;

    for (i=0; i<m_genericPartCount; i++)
    {
      ARTCustomPlacementDelete(m_parts[i], NmRsGenericPart);
      m_parts[i] = 0;
    }

    if (m_articulatedWrapper)
    {
      ARTCustomPlacementDelete(m_articulatedWrapper, NmRsArticulatedWrapper);
      m_articulatedWrapper = 0;
    }

    for (i=0; i<m_1dofCount; i++)
    {
      ARTCustomPlacementDelete(m_1dofEffectors[i], NmRs1DofEffector);
      m_1dofEffectors[i] = 0;
    }

    for (i=0; i<m_3dofCount; i++)
    {
      ARTCustomPlacementDelete(m_3dofEffectors[i], NmRs3DofEffector);
      m_3dofEffectors[i] = 0;
    }
  }

#if NM_SCRIPTING
  //mmmmteleport
  void NmRsCharacter::readScript()
  {
    char str [80];

    if (!m_pNMScriptFile)
    {
      m_readScript = true;
      // open file that we have just produced in behaviour
#if defined(_XBOX)
      m_pNMScriptFile = fopen("GAME:\\NMBehaviourScript.txt", "rt");
#elif __PS3
      m_pNMScriptFile = fopen("/app_home/NMBehaviourScript.txt", "rt");
#else
      m_pNMScriptFile = fopen("c:\\NMBehaviourScript.txt", "rt");
#endif
      Assertf(m_pNMScriptFile!=NULL, "NMBehaviourScript.txt unable to open ");
      sprintf(str,"OpeningFile");
#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "Script", str);
#endif
      if (!m_pNMScriptFile)
        return;
      //find 1st behaviour block
      //find the correct time chunk to read
      while (std::strcmp("FileEnd", str) != 0) 
      {
        fscanf (m_pNMScriptFile, "%s", &str);
        if (std::strcmp("SIM_TIME", str) == 0)
        {
          fscanf (m_pNMScriptFile, "%f", &m_nextScriptTime);
          break;
        }
      }
      if (std::strcmp("FileEnd", str) == 0)
      {
        m_readScript = false;
      }
    }
    if (!m_readScript)
      return;

    if (m_simTime < m_nextScriptTime)
      return;

    if (m_simTime > m_nextScriptTime && m_nextScriptTime > m_scriptTime)
    {
      m_scriptTime = m_nextScriptTime;
    }

    if (m_pNMScriptFile != NULL)        
    { 
      MMMessage* message = rage_new MMMessage;
      //read behaviour block
      char str2 [80];
      char behaviourName [80];
      char parameterName [80];
      float f,f2,f3;
      sprintf(str,"ReadBehaviourBlock");

      while (std::strcmp("MessagesEnd", str) != 0) 
      {
        fscanf (m_pNMScriptFile, "%s", &str);
#if ART_ENABLE_BSPY
        bspyScratchpad(getBSpyID(), "Script", str);
#endif

        if (std::strcmp("MessagesEnd", str) == 0)
        {
          break;
        }
        message->params.reset();
        while (std::strcmp("behaviourEnd", str) != 0)
        {
          if (std::strcmp("behaviourEnd", str) == 0)
          {
            fscanf (m_pNMScriptFile, "%s", &str);
#if ART_ENABLE_BSPY
            bspyScratchpad(getBSpyID(), "Script", str);
#endif
            break;
          }
          else if (std::strcmp("behaviour", str) == 0) 
          {
            fscanf (m_pNMScriptFile, "%s", &behaviourName);
            fscanf (m_pNMScriptFile, "%s", &str);
#if ART_ENABLE_BSPY
            bspyScratchpad(getBSpyID(), "Script", behaviourName);
            bspyScratchpad(getBSpyID(), "Script", str);
#endif
          }
          else
          {
            f = -99999.f; 
            fscanf (m_pNMScriptFile, "%f", &f);
#if ART_ENABLE_BSPY
            bspyScratchpad(getBSpyID(), "Script", f);
#endif
            if (f == -99999.f)
            {
              fscanf (m_pNMScriptFile, "%s", &str2);
#if ART_ENABLE_BSPY
              bspyScratchpad(getBSpyID(), "Script", str2);
#endif
              {
                if (std::strcmp("true", str2) == 0 || std::strcmp("false", str2) == 0)//is a bool
                {
                  bool bValue = (std::strcmp("true", str2) == 0) ? true : false;
                  message->params.addBool(str, bValue);
                }
                else if (std::strcmp("*", str2) == 0 )//is a pointer
                {
                  int i = 0;//nb won't work on 64bit
                  fscanf (m_pNMScriptFile, "%i", &i);
                  message->params.addReference(str, (const void *) i);
                }
                else
                {
                  message->params.addString(str, str2);
                }
                fscanf (m_pNMScriptFile, "%s", &str);
#if ART_ENABLE_BSPY
                bspyScratchpad(getBSpyID(), "Script", str);
#endif
              }
            }
            else
            {
              sprintf(parameterName,"%s",str);
              fscanf (m_pNMScriptFile, "%s", &str);
#if ART_ENABLE_BSPY
              bspyScratchpad(getBSpyID(), "Script", str);
#endif
              if (std::strcmp("mask", parameterName) == 0)
              {
                int mask;
                mask = (int) f;
                sprintf(str2,"%i", mask);
                message->params.addString(parameterName,str2);
              }
              else if (std::strcmp("%", str) == 0)//integer
              {
                fscanf (m_pNMScriptFile, "%s", &str);
                message->params.addInt(parameterName,(int) f);
              }
              //else if (std::strcmp("&", str) == 0)//Pointer - & used because *  this doesn't work - information loss when converted to float? float to int
              //{
              //  fscanf (m_pNMScriptFile, "%s", &str);
              //  message->params.addReference(parameterName,(const void*) (int)f);
              //}
              //else if (std::strcmp("b", str) == 0)//binary string
              //{
              //  fscanf (m_pNMScriptFile, "%s", &str);
              //  message->params.addInt(parameterName,(int) f);
              //}
              else if (std::strcmp(",", str) == 0)//vector
              {
                fscanf (m_pNMScriptFile, "%f", &f2);
                fscanf (m_pNMScriptFile, "%s", &str);// ,
                fscanf (m_pNMScriptFile, "%f", &f3);
                message->params.addVector3(parameterName,f,f2,f3);
                fscanf (m_pNMScriptFile, "%s", &str);
#if ART_ENABLE_BSPY
                rage::Vector3 vec(f,f2,f3);
                bspyScratchpad(getBSpyID(), "Script", vec);
#endif
              }
              else//float
              {
                message->params.addFloat(parameterName,f);
              }
            }
          }
        }//while (std::strcmp("MessagesEnd", str) != 0)
#if ART_ENABLE_BSPY
        bspyScratchpad(getBSpyID(), "ScriptInvoke", behaviourName);
#endif
        InvokeUID iUID = NMutils::hashString(behaviourName);
        handleDirectInvoke(iUID, &message->params);
      }

      //above needs to look for FileEnd aswell
      //find the next behaviour block
      //find the correct time chunk to read
      while (std::strcmp("FileEnd", str) != 0) 
      {
        fscanf (m_pNMScriptFile, "%s", &str);
        if (std::strcmp("SIM_TIME", str) == 0)
        {
          fscanf (m_pNMScriptFile, "%f", &m_nextScriptTime);
          break;
        }
      }
      if (std::strcmp("FileEnd", str) == 0)
      {
        m_readScript = false;
      }
      delete message;
    }//if (m_pNMScriptFile != NULL) 
  }

  void NmRsCharacter::readBlockedBehaviourMessages()
  {
    m_numOfBlockedUIDs = 0;
    m_numOfDelayedUIDs = 0;
    m_currentDelayedMessage = 0;
    //VARIABLES
    sprintf(ctmAnimation,"");
    ctmCurrent = -1;
    ctmStart = 0;
    ctmEnd = 0;
    ctmTimeStep = -0.1f;
    teleport = false;
    currentTM = rage_new rage::Matrix34[21];
    previousTM = rage_new rage::Matrix34[21];

    m_bulletImpulseMag = -1.f;
    bulletTorqueMult = -1.f;
    impulseTorqueScale = -1.f;
    bulletImpulseMult = 1.f;
	  snapMag = 1.0f;
    rbRatio = 0.f;
    rbLowerShare = 0.5f;
    rbForce = 1.f;
    rbMoment = 1.f;
    rbMaxTwistMomentArm = 0.2f;
    rbMaxBroomMomentArm = 0.5f;
    rbTwistAxis = 0;
    kMultForLoose = 0.1f;
    m_minLegStraightness = 0.f;
    m_minLegSeperation = 0.f;
    m_maxLegSeperation = 1.f;
    m_newHitEachApplyBulletImpulseMessage = false;
    balIgnoreFailure = false;
    balLegCollisions = false;
    oldTripping = true;
    newTripping = false;
    stepUp = false;
    pushOff = true;
    rbPivot = false;

    stepIfInSupport = true;
    pushOffBackwards = false;
    alwaysStepWithFarthest = false;
    m_allowMeasureCharacter = true;
    getZeroPose = true;
    emergencyVelocityClamp = false;
    overideAnimPose = false;
    comAngularClamp = false;
    angularClamp = false;
    oldClamp = false;
    bulletDirection = true;
    m_allowLegShot = true;
    m_alwaysStepWithFarthest = false;
    m_behaviourMasking = true;
    m_standUp = false;
    scaleArmShot = true;
    minArmsLooseness = -0.1f;
    minLegsLooseness = -0.1f;
    allowArmShot = true;
    bulletMomA = true;
    bulletMomB = true;
    oldCOMCalcTime = false;
    newBullet = false;

    bcrWrithe = false;
    m_newHitAsApplyBulletImpulseMessage =false;
#define debug_ReadFile 1
    //store blocked message names
    char behaviourName [80];
#if defined(_XBOX)
    sprintf(behaviourName,"GAME:\\NMBlockAndVars.txt");        
#elif __PS3 
    sprintf(behaviourName,"/app_home/NMBlockAndVars.txt");        
#else
    sprintf(behaviourName,"C:\\NMBlockAndVars.txt");        
#endif

    // open file 
    FILE *pFile = NULL;
    pFile = fopen(behaviourName, "rt");        
    char b [10];
    Assertf(pFile!=NULL, "NMBlockAndVars.txt unable to open ");
    sprintf(behaviourName,"Opening blocked File");
#if ART_ENABLE_BSPY && debug_ReadFile
    bspyScratchpad(bspyNoAgent, "Script", behaviourName);
#endif

    if (pFile != NULL)        
    {   
      sprintf(behaviourName,"OpenedFile");
#if ART_ENABLE_BSPY && debug_ReadFile
      bspyScratchpad(bspyNoAgent, "Script", behaviourName);
#endif
      //BLOCKED
      fscanf (pFile, "%s", &behaviourName);
      if (std::strcmp("BlockBehaviour", behaviourName) == 0)
      {
        fscanf (pFile, "%s", &behaviourName);
        m_numOfBlockedUIDs = 0;
        while (std::strcmp("BlockBehaviourEnd", behaviourName) != 0)
        {
#if ART_ENABLE_BSPY && debug_ReadFile
          bspyScratchpad(bspyNoAgent, "Script", behaviourName);
#endif
          if (behaviourName[0] != '#')
          {
            m_blockedUIDs[m_numOfBlockedUIDs] = NMutils::hashString(behaviourName);
            m_numOfBlockedUIDs++;
#if ART_ENABLE_BSPY && debug_ReadFile
            bspyScratchpad(bspyNoAgent, "Script", behaviourName);
#endif
          }
          fscanf (pFile, "%s", &behaviourName);
        }
      }
      //DELAYED
      m_numOfDelayedUIDs = 0;
      m_currentDelayedMessage = 0;
      for (int i=0; i<10; i++) 
      {
        m_delayedMessageDelay[i] = -1;
      }
      int delay = -1;
      fscanf (pFile, "%s", &behaviourName);
      if (std::strcmp("DelayedBehaviour", behaviourName) == 0)
      {
        fscanf (pFile, "%s", &behaviourName);

        while (std::strcmp("DelayedBehaviourEnd", behaviourName) != 0)
        {
#if ART_ENABLE_BSPY && debug_ReadFile
          bspyScratchpad(bspyNoAgent, "Script", behaviourName);
#endif
          fscanf (pFile, "%i", &delay);
          if (behaviourName[0] != '#')
          {
            m_delayedUIDs[m_numOfDelayedUIDs] = NMutils::hashString(behaviourName);
            m_delayForUIDs[m_numOfDelayedUIDs] = delay;
            m_numOfDelayedUIDs++;
#if ART_ENABLE_BSPY && debug_ReadFile
            bspyScratchpad(bspyNoAgent, "Script", behaviourName);
#endif
          }
          fscanf (pFile, "%s", &behaviourName);
        }
      }


      //VARIABLES
      fscanf (pFile, "%s", &behaviourName);
      if (std::strcmp("Variables", behaviourName) == 0)
      {
        fscanf (pFile, "%s", &behaviourName);

        while (std::strcmp("VariablesEnd", behaviourName) != 0)
        {
#if ART_ENABLE_BSPY && debug_ReadFile
          bspyScratchpad(bspyNoAgent, "Script", behaviourName);
#endif
          float f = -999.f;
          fscanf (pFile, "%f", &f);
          if (f == -999.f)
          {
            fscanf (pFile, "%s", &b);
#if ART_ENABLE_BSPY && debug_ReadFile
            bspyScratchpad(bspyNoAgent, "Script", b);
#endif
          }
          if (behaviourName[0] != '#')
          {
            if (std::strcmp("bulletImpulseMag", behaviourName) == 0)
              m_bulletImpulseMag = f;
            else if (std::strcmp("bulletTorqueMult", behaviourName) == 0)
              bulletTorqueMult = f;
            else if (std::strcmp("impulseTorqueScale", behaviourName) == 0)
              impulseTorqueScale = f;
            else if (std::strcmp("bulletImpulseMult", behaviourName) == 0)
              bulletImpulseMult = f;
            else if (std::strcmp("ctmStart", behaviourName) == 0)
              ctmStart = (int)f;            
            else if (std::strcmp("ctmEnd", behaviourName) == 0)
              ctmEnd = (int)f;            
            else if (std::strcmp("ctmTimeStep", behaviourName) == 0)
              ctmTimeStep = f;            
            else if (std::strcmp("ctmAnimation", behaviourName) == 0)
              sprintf(ctmAnimation, "/app_home/%s", b);
            else if (std::strcmp("teleport", behaviourName) == 0)
              teleport = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("snapMag", behaviourName) == 0)
              snapMag = f;
            else if (std::strcmp("rbRatio", behaviourName) == 0)
              rbRatio = f;            
            else if (std::strcmp("rbLowerShare", behaviourName) == 0)
              rbLowerShare = f;            
            else if (std::strcmp("rbForce", behaviourName) == 0)
              rbForce = f;            
            else if (std::strcmp("rbMoment", behaviourName) == 0)
              rbMoment = f;            
            else if (std::strcmp("rbMaxTwistMomentArm", behaviourName) == 0)
              rbMaxTwistMomentArm = f;            
            else if (std::strcmp("rbTwistAxis", behaviourName) == 0)
              rbTwistAxis = (int)f;            
            else if (std::strcmp("rbMaxBroomMomentArm", behaviourName) == 0)
              rbMaxBroomMomentArm = f;            
            else if (std::strcmp("kMultForLoose", behaviourName) == 0)
              kMultForLoose = f;
            else if (std::strcmp("minLegStraightness", behaviourName) == 0)
              m_minLegStraightness = f;
            else if (std::strcmp("minLegSeperation", behaviourName) == 0)
              m_minLegSeperation = f;
            else if (std::strcmp("maxLegSeperation", behaviourName) == 0)
              m_maxLegSeperation = f;
            else if (std::strcmp("newHitEachApplyBulletImpulseMessage", behaviourName) == 0)
              m_newHitEachApplyBulletImpulseMessage = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("balIgnoreFailure", behaviourName) == 0)
              balIgnoreFailure = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("balLegCollisions", behaviourName) == 0)
              balLegCollisions = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("stepUp", behaviourName) == 0)
              stepUp = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("oldTripping", behaviourName) == 0)
              oldTripping = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("rbPivot", behaviourName) == 0)
              rbPivot = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("newTripping", behaviourName) == 0)
              newTripping = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("pushOff", behaviourName) == 0)
              pushOff = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("pushOffBackwards", behaviourName) == 0)
              pushOffBackwards = (std::strcmp("true", b) == 0)? true:false;         
            else if (std::strcmp("stepIfInSupport", behaviourName) == 0)
              stepIfInSupport = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("alwaysStepWithFarthest", behaviourName) == 0)
              alwaysStepWithFarthest = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("allowMeasureCharacter", behaviourName) == 0)
              m_allowMeasureCharacter = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("getZeroPose", behaviourName) == 0)
              getZeroPose = (std::strcmp("true", b) == 0)? true:false;             
            else if (std::strcmp("emergencyVelocityClamp", behaviourName) == 0)
              emergencyVelocityClamp = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("overideAnimPose", behaviourName) == 0)
              overideAnimPose = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("angularClamp", behaviourName) == 0)
              angularClamp = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("comAngularClamp", behaviourName) == 0)
              comAngularClamp = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("bulletDirection", behaviourName) == 0)
              bulletDirection = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("oldClamp", behaviourName) == 0)
              oldClamp = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("standUp", behaviourName) == 0)
              m_standUp = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("allowLegShot", behaviourName) == 0)
              m_allowLegShot = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("alwaysStepWithFarthest", behaviourName) == 0)
              m_alwaysStepWithFarthest = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("behaviourMasking", behaviourName) == 0)
              m_behaviourMasking = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("bcrWrithe", behaviourName) == 0)
              bcrWrithe = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("scaleArmShot", behaviourName) == 0)
              scaleArmShot = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("minArmsLooseness", behaviourName) == 0)
              minArmsLooseness = f;
            else if (std::strcmp("minLegsLooseness", behaviourName) == 0)
              minLegsLooseness = f;
            else if (std::strcmp("allowArmShot", behaviourName) == 0)
              allowArmShot = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("bulletMomA", behaviourName) == 0)
              bulletMomA = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("bulletMomB", behaviourName) == 0)
              bulletMomB = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("oldCOMCalcTime", behaviourName) == 0)
              oldCOMCalcTime = (std::strcmp("true", b) == 0)? true:false;
            else if (std::strcmp("newBullet", behaviourName) == 0)
              newBullet = (std::strcmp("true", b) == 0)? true:false;

#if ART_ENABLE_BSPY
            if (std::strcmp("bulletImpulseMag", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", m_bulletImpulseMag);
            }
            else if (std::strcmp("bulletTorqueMult", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", bulletTorqueMult);
            }
            else if (std::strcmp("impulseTorqueScale", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", impulseTorqueScale);
            }
            else if (std::strcmp("bulletImpulseMult", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", bulletImpulseMult);
            }
			else if (std::strcmp("snapMag", behaviourName) == 0)
			{
				bspyScratchpad(bspyNoAgent, "Script", snapMag);
			}			
            else if (std::strcmp("newHitEachApplyBulletImpulseMessage", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", m_newHitEachApplyBulletImpulseMessage);
            }
            else if (std::strcmp("balIgnoreFailure", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", balIgnoreFailure);
            }
            else if (std::strcmp("balLegCollisions", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", balLegCollisions);
            }
            else if (std::strcmp("oldTripping", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", oldTripping);
            }
            else if (std::strcmp("rbPivot", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", rbPivot);
            }
            else if (std::strcmp("newTripping", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", newTripping);
            }
            else if (std::strcmp("pushOff", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", pushOff);
            }
            else if (std::strcmp("pushOffBackwards", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", pushOffBackwards);
            }
            else if (std::strcmp("stepUp", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", stepUp);
            }
            else if (std::strcmp("stepIfInSupport", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", stepIfInSupport);
            }
            else if (std::strcmp("alwaysStepWithFarthest", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", alwaysStepWithFarthest);
            }
            else if (std::strcmp("allowMeasureCharacter", behaviourName) == 0)
            {
              bspyScratchpad(bspyNoAgent, "Script", m_allowMeasureCharacter);
            }
#endif
          }
          fscanf (pFile, "%s", &behaviourName);
        }
      }

      fclose(pFile);
    }//if (pFile != NULL) 

  }
#endif
  /**
   * This must be used to reset persistent data every time the agent is inserted into the scene.
   * called in the constructor and in addToScene.
   */
  void NmRsCharacter::initialiseData()
  {
#if NM_SCRIPTING
    m_simTime = 0.f;
    m_scriptTime = -1.1f;
    m_nextScriptTime = FLT_MAX;
    m_pNMScriptFile = NULL;
    m_readScript = false;
#endif
#ifdef NM_RS_CBU_ASYNCH_PROBES
    // Game and RASH should have set sm_AsyncShapeTestMgr already.  As a failsafe create a phAsyncShapeTestMgr if this is not the case.  
    if (!AssertVerify(sm_AsyncShapeTestMgr))
    {
      sm_AsyncShapeTestMgr = rage_new rage::phAsyncShapeTestMgr;
      const int asyncProbeSchedulerIndex = 0;
      sm_AsyncShapeTestMgr->Init(asyncProbeSchedulerIndex);
    }

    for (int i=0; i<api_probeCount; i++) 
    {
      m_AsyncProbeHandles[i] = rage::phAsyncShapeTestMgr::InvalidShapeTestHandle;
      m_AsyncProbeRayProbeIndex[i] = pi_UseNonAsync;
#if ART_ENABLE_BSPY
      m_AsyncProbeSubmitFrame[i] = -1;
#endif
    }
    for (int i=0; i<pi_probeCount; i++) 
    {
      InitializeProbe((rayProbeIndex) i);
    }
    ClearAllProbes();

#endif // NM_RS_CBU_ASYNCH_PROBES

    m_LeftArmMassIncreased = false;
    m_RightArmMassIncreased = false;
#if NM_UPSIDEDOWN_FEET
    m_leftFootUpsideDown = false;
    m_rightFootUpsideDown = false;
    //m_FootSizeCacheLeft should be set to the actual foot size the 1st time the foot goes "upsideDown".  Initialize just for safety.
    m_FootSizeCacheLeft.Set(0.11008f, 0.075008f, 0.3008f);//Fred's foot size with 008 added
    m_FootSizeCacheRight.Set(0.11008f, 0.075008f, 0.3008f);//Fred's foot size with 008 added
#endif

#if NM_STEP_UP
    m_leftFootFrictionMult = 1.0f;
    m_rightFootFrictionMult = 1.0f;
#endif
    m_gsFricScale1 = 1.0f;
    m_gsFricScale2 = 1.0f;
    m_gsFricMask1 = bvmask_None;
    m_gsFricMask2 = bvmask_None;
    m_gsScale1Foot = false;
    m_glancingVehicleHit = false;


    m_COM.Zero();
    m_COMvel.Zero();
    m_COMrotvel.Zero();
    m_COMTM.Identity();
#if NM_RIGID_BODY_BULLET
    m_characterInertiaAboutComInN.Zero();
    m_characterInertiaAboutPivotInN.Zero();
#endif

    m_angMom.Zero();
    m_COMvelMag = m_COMrotvelMag = m_COMvelRelativeMag = 0.0f;
    m_calculateInertias = false;
    m_floorVelocity.Zero();
    m_floorAcceleration.Zero();
    m_footSlipMult = 1.0f;

    //Constraints
    handCuffs = false;
    handCuffsBehindBack = false;
    legCuffs = false;
    rightDominant = false;
    passiveMode = 0;
    bespokeBehaviour = false;
    blend2ZeroPose = 0.f;

    m_rememberSetStiffness = false;
    m_rememberSetMuscleStiffness = false;

    // reset default configs
    m_characterConfig.m_legStraightness = 0.99f;
    m_characterConfig.m_legSeparation = 0.23f;
    m_characterConfig.m_defaultHipPitch = 0.f;
    m_characterConfig.m_leftHandState = CharacterConfiguration::eHS_Free;
    m_characterConfig.m_rightHandState = CharacterConfiguration::eHS_Free;
    m_characterConfig.m_charlieChapliness = 0.0f;
    m_characterConfig.m_hipYaw = 0.0f;
    m_characterConfig.m_headYaw = 0.0f;

    m_gUp = rage::phSimulator::GetGravity();
    m_gUp.Normalize();
    m_gUp.Scale(-1.0f);

    m_gUpReal = m_gUp;

    m_groundNormal = m_gUp;

    m_attachedObject.partIndex = -1;
    m_attachedObject.levelIndex = -1;
    m_attachedObject.mass = 0.0f;
    m_attachedObject.massMultiplier = 1.0f;
    m_attachedObject.worldCOMPos.Zero();

    m_leftHandWeapon.partIndex = -1;
    m_leftHandWeapon.levelIndex = -1;
    m_leftHandWeapon.mass = 0;
    m_leftHandWeapon.massMultiplier = 1.0f;
    m_leftHandWeapon.worldCOMPos.Zero();
    //Unused m_leftHandWeapon.isColliding = false;

    m_rightHandWeapon.partIndex = -1;
    m_rightHandWeapon.levelIndex = -1;
    m_rightHandWeapon.mass = 0;
    m_rightHandWeapon.massMultiplier = 1.0f;
    m_rightHandWeapon.worldCOMPos.Zero();
    //Unused m_rightHandWeapon.isColliding = false;

    m_weaponMode = kNone;

    // set up collision exclusion set defaults
    //Arms for the pointGun.
    //Legs for the dynamicBalancer.
    m_rightHandCollisionExclusion.init();
    m_Leg2LegCollisionExclusion.init();

#if NM_SET_WEAPON_BOUND
    m_handBoundCache[kLeftHand] = NULL;
    m_handBoundCache[kRightHand] = NULL;
#endif
    
    m_registerWeaponCalled = false;
    //Set default transformations between gun and gunHand so that the hand will orientate
    //properly in pointGun if a gunToHand is not sent in. (Identity matrix does not work as gun axes are swapped)
    //PointGun also does this on activate if registerWeapon has not been called (and sets correct rifle ones aswell)
    //kPistolRight
    m_gunToHandAiming[kRightHand].a.Set(0.0836874f, -0.992887f, 0.0846575f);
    m_gunToHandAiming[kRightHand].b.Set(-0.98635f, -0.0946182f, -0.134663f);
    m_gunToHandAiming[kRightHand].c.Set(0.141716f, -0.0722336f, -0.987254f);
    m_gunToHandAiming[kRightHand].d.Set(0.00314653f, -0.0790734f, -0.0385407f);
    m_gunToHandCurrent[kLeftHand].Set(m_gunToHandAiming[kRightHand]);
    m_gunToHandAiming[kLeftHand].Set(m_gunToHandAiming[kRightHand]);
    m_gunToHandCurrent[kRightHand].Set(m_gunToHandAiming[kRightHand]);

    m_gunToHandConstraintHandle[kLeftHand].Reset();
    m_gunToHandConstraintHandle[kRightHand].Reset();
    m_handToHandConstraintHandle.Reset();

#if NM_SET_WEAPON_MASS
    m_handMassCache[kLeftHand] = 0.5f;
    m_handMassCache[kRightHand] = 0.5f;
#endif

    m_posture.init();

    //mmmmmtodo Customize for NORTH
    m_probeTypeIncludeFlags = TYPE_FLAGS_ALL;
    m_probeTypeExcludeFlags = TYPE_FLAGS_NONE;
    //mmmmtodo remove this once the game is sending this info in -START
    //exclude gunBelt etc 
    enum
    {
      BOUNDFLAG_MOVERBOUND = 1,
      BIT_NO_GROUND_PROBE = 23,
      BOUNDFLAG_NO_GROUND_PROBE = 1 << BIT_NO_GROUND_PROBE
    };
    //mmmmtodo remove this once the game is sending this info in - END

    m_probeTypeIncludeFlags = BOUNDFLAG_MOVERBOUND;
    m_probeTypeExcludeFlags = BOUNDFLAG_NO_GROUND_PROBE;
    //mmmmtodo remove this once the game is sending this info in - END

#if ART_ENABLE_BSPY
    m_skeletonVizMode = kSV_DesiredAngles;
    m_skeletonVizRoot = 10;
    m_skeletonVizMask = 0;
    //m_skeletonVizMode = kSV_ActualAngles;
    //m_skeletonVizRoot = 0;//This can anchor the skeleton to a particular joint
    //m_skeletonVizMask = 0xFFFFFFFF;
#endif

#if ART_ENABLE_BSPY
    m_currentBehaviour = bvid_Invalid;
    m_currentSubBehaviour = "";
    m_currentFrame = 1;
#endif
    m_fscLeftTwist = 0.0f;
    m_fscRightTwist = 0.0f;

    // reset incoming transforms flags
    m_applyMode = kDisabled;
    m_applyMask = bvmask_Full;
    m_lastApplyMask = bvmask_Full;

#if 0
    // reset masking
    memset(m_maskCodeStack, 0, sizeof(BehaviourMask) * RS_MASKCODESTACK_SZ);
    m_maskCodeStackIdx = -1;
#endif

    m_dontRegisterCollsionMassBelow = -1.f;//ie register everything
    m_dontRegisterCollsionVolBelow = -1.f;//ie register everything
    m_dontRegisterCollsionActive = false;//ie register everything

    //mmmmtodo configureProbes message?
    m_dontRegisterProbeVelocityMassBelow = 20.0f;//-1.f;//ie register everything 
    m_dontRegisterProbeVelocityVolBelow = 0.1f;//-1.f;//ie register everything
    m_dontRegisterProbeVelocityActive = true;//false;//ie register everything

    m_strength = 1.0f;
    m_health = 1.0f;

    m_depthFudge = 0.01f;
    m_depthFudgeStagger = 0.01f;
    m_footFriction = 1.0f;
    m_footFrictionStagger = 1.0f;
    m_minImpactFriction = 0.0f;
    m_maxImpactFriction = 999999.0f;
    m_applyMinMaxFriction  = false;

    m_viscosity = -1.f;//no viscosity
    m_underwater = false;
    m_stroke = 0.f;
    m_linearStroke = false;

    m_movingFloor = false;


    // calls message to set upright constraint parameters to defaults as specified in NmRsMessageDefinitions
    stayUprightUpdate(0, this);

    // Initialise bullets
    configureBulletsUpdate(0, this); // calls message to set parameters to defaults as specified in NmRsMessageDefinitions
    configureBulletsExtraUpdate(0, this); // calls message to set parameters to defaults as specified in NmRsMessageDefinitions
    for (int i=0; i<NUM_OF_BULLETS; i++)
    {
      m_bulletApplier[i].init(this); // these are the state machine base class functions
      m_bulletApplier[i].reset();
#if ART_ENABLE_BSPY && BulletBSpyScratchPad
      m_bulletApplier[i].setBSpyID(i);
#endif
    }
    m_currentBulletApplier = 0;
    m_impulseLeakage = 1.0f;
    m_lastImpulseMultiplier = -1.0f;

    m_footLOrientationInitialized = false;
    m_footROrientationInitialized = false;

    m_kineticEnergyPerKiloValid = false;

    // copy incoming trasnforms stuff from engine
    const NmRsEngine* engine = getEngine();
    Assert(engine); // do we have one of those yet?
    m_incomingAnimationVelocityScale = engine->getIncomingAnimationVelocityScale();

#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
    m_needToCheckReenableCollisionForGrab = false;
#endif

#if NM_SCRIPTING
    readBlockedBehaviourMessages();
    alreadyHeldPose = false;
#endif

#if NM_EA
    m_currentPatch = -1;
    for (int i = 0; i<NUM_OF_PATCHES; i++)
    {
      m_patches[i].Init();
    }
#endif//#if NM_EA
#if NM_STAGGERSHOT
    m_spineStrengthScale = 1.f;
    m_spineDampingScale = 1.f;
    m_spineMuscleStiffness = 1.5f;
#endif

    m_stopAllBehavioursSent = false;
  }

  bool NmRsCharacter::addToScene(rage::phSimulator *sim, rage::phLevelNew* level)
  {
    if(m_level)
    {
      NM_RS_LOGERROR(L"NmRsCharacter::addToScene() Error : agent %i already added to a scene", getID());
      return false;
    }
    int i;

    Assert(m_cbuTaskManager);

    BEGIN_PROFILING("addToScene");

    initialiseData();

    m_simulator = sim;
    m_level = level;
    m_isInsertedInScene = true;

    m_limbManager = getEngine()->getLimbManager();
    Assert(m_limbManager);

    if (m_articulatedWrapper)
    {
#define NM_MASSES_OVERRIDE 0
#if NM_MASSES_OVERRIDE
      rage::phArticulatedBody *body = getArticulatedWrapper()->getArticulatedBody();
      if (getBodyIdentifier() == gtaWilma || getBodyIdentifier() == gtaWilmaLarge)
      {
        Assert(body->GetLink(0).GetMass().Getf() == 11.f);//pelvis
        Assert(body->GetLink(1).GetMass().Getf() == 7.6f);//thigh
        Assert(body->GetLink(2).GetMass().Getf() == 4.f);//shin
        Assert(body->GetLink(3).GetMass().Getf() == 1.62f);//foot
        Assert(body->GetLink(4).GetMass().Getf() == 7.6f);//thigh
        Assert(body->GetLink(5).GetMass().Getf() == 4.f);//shin
        Assert(body->GetLink(6).GetMass().Getf() == 1.62f);//foot
        Assert(body->GetLink(7).GetMass().Getf() == 7.f);//spine 0
        Assert(body->GetLink(8).GetMass().Getf() == 6.f);//spine 1
        Assert(body->GetLink(9).GetMass().Getf() == 5.f);//spine 2
        Assert(body->GetLink(10).GetMass().Getf() == 4.f);//spine 3
        Assert(body->GetLink(11).GetMass().Getf() == 2.25f);//clavicle
        Assert(body->GetLink(12).GetMass().Getf() == 2.4f);//upper arm
        Assert(body->GetLink(13).GetMass().Getf() == 1.1f);//forearm
        Assert(body->GetLink(14).GetMass().Getf() == 0.4f);//hand
        Assert(body->GetLink(15).GetMass().Getf() == 2.25f);//clavicle
        Assert(body->GetLink(16).GetMass().Getf() == 2.4f);//upper arm
        Assert(body->GetLink(17).GetMass().Getf() == 1.2f);//forearm
        Assert(body->GetLink(18).GetMass().Getf() == 0.4f);//hand
        Assert(body->GetLink(19).GetMass().Getf() == 0.95f);//neck
        Assert(body->GetLink(20).GetMass().Getf() == 5.f);//head
      }
      else if (getBodyIdentifier() == gtaFred || getBodyIdentifier() == gtaFredLarge || getBodyIdentifier() == gtaAlien)
      {
        Assert(body->GetLink(0).GetMass().Getf() == 13.8f);//pelvis
        Assert(body->GetLink(1).GetMass().Getf() == 9.5f);//thigh
        Assert(body->GetLink(2).GetMass().Getf() == 5.f);//shin
        Assert(body->GetLink(3).GetMass().Getf() == 2.f);//foot
        Assert(body->GetLink(4).GetMass().Getf() == 9.5f);//thigh
        Assert(body->GetLink(5).GetMass().Getf() == 5.f);//shin
        Assert(body->GetLink(6).GetMass().Getf() == 2.f);//foot
        Assert(body->GetLink(7).GetMass().Getf() == 8.8f);//spine 0
        Assert(body->GetLink(8).GetMass().Getf() == 7.5f);//spine 1
        Assert(body->GetLink(9).GetMass().Getf() == 6.3f);//spine 2
        Assert(body->GetLink(10).GetMass().Getf() == 5.f);//spine 3
        Assert(body->GetLink(11).GetMass().Getf() == 2.8f);//clavicle
        Assert(body->GetLink(12).GetMass().Getf() == 3.f);//upper arm
        Assert(body->GetLink(13).GetMass().Getf() == 1.3f);//forearm
        Assert(body->GetLink(14).GetMass().Getf() == 0.5f);//hand
        Assert(body->GetLink(15).GetMass().Getf() == 2.8f);//clavicle
        Assert(body->GetLink(16).GetMass().Getf() == 3.f);//upper arm
        Assert(body->GetLink(17).GetMass().Getf() == 1.3f);//forearm
        Assert(body->GetLink(18).GetMass().Getf() == 0.5f);//hand
        Assert(body->GetLink(19).GetMass().Getf() == 1.1f);//neck
        Assert(body->GetLink(20).GetMass().Getf() == 6.3f);//head
      }
#endif

#if NM_RIGID_BODY_BULLET
      m_upperBodyMass = 0.f;
      m_lowerBodyMass = 0.f;

      for (i=0; i<8; i++)//pelvis, legs,spine0
        m_lowerBodyMass += getArticulatedBody()->GetMass(i).Getf();
      for (i=8; i<getNumberOfParts(); i++)//Spine1,2,3 Neck,Head, arms
        m_upperBodyMass += getArticulatedBody()->GetMass(i).Getf();

#endif

      BEGIN_PROFILING("AP collision");

      END_PROFILING();

#if NM_SET_WEAPON_BOUND
      if(m_bodyIdent == mp3Large || m_bodyIdent == mp3Medium || m_bodyIdent == mp3Maxine)
      {
        // cache hand bounds in the even that we overwrite them with weapon bounds
        cacheHandBound(kLeftHand);
        cacheHandBound(kRightHand);
      }
#endif

#if NM_SET_WEAPON_MASS
      // cache hand masses
      cacheHandMass(kLeftHand);
      cacheHandMass(kRightHand);
#endif
    }

    for (i=0; i<m_genericPartCount; i++)
      m_parts[i]->initialisePart(); 

    initMaxAngSpeed();

    // initialize all effectors
    for(i=0; i<m_effectorCount; i++)
      m_effectors[i]->init(this);

    //Apply a default zero pose to the arms - taken from shot idles (used by shot default arms and bodyBalance as the character comes to rest)
    if (getBodyIdentifier() == gtaFred || getBodyIdentifier() == gtaAlien)
    {
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getLeftArmSetup()->getClavicle()->getJointIndex())))->setZeroPoseAngles(0.037f,	-0.024f,	0.021f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getLeftArmSetup()->getShoulder()->getJointIndex())))->setZeroPoseAngles(0.314f,	0.033f,	0.294f);
      (static_cast<NmRs1DofEffector*>(getEffectorDirect(getLeftArmSetup()->getElbow()->getJointIndex())))->setZeroPoseAngle(0.400f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getLeftArmSetup()->getWrist()->getJointIndex())))->setZeroPoseAngles(0.280f,	-0.047f,	0.212f);

      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getRightArmSetup()->getClavicle()->getJointIndex())))->setZeroPoseAngles(0.054f,	0.080f,	0.080f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getRightArmSetup()->getShoulder()->getJointIndex())))->setZeroPoseAngles(0.382f,	0.033f, 0.274f);
      (static_cast<NmRs1DofEffector*>(getEffectorDirect(getRightArmSetup()->getElbow()->getJointIndex())))->setZeroPoseAngle(0.288f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getRightArmSetup()->getWrist()->getJointIndex())))->setZeroPoseAngles(0.271f,	0.104f,	0.072f);
    }
    else if (getBodyIdentifier() == gtaWilma)
    {
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getLeftArmSetup()->getClavicle()->getJointIndex())))->setZeroPoseAngles(0.091f, -0.133f, 0.076f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getLeftArmSetup()->getShoulder()->getJointIndex())))->setZeroPoseAngles(-0.428f, 0.131f, 0.432f);
      (static_cast<NmRs1DofEffector*>(getEffectorDirect(getLeftArmSetup()->getElbow()->getJointIndex())))->setZeroPoseAngle(0.193f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getLeftArmSetup()->getWrist()->getJointIndex())))->setZeroPoseAngles(0.489f, -0.160f, 0.014f);

      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getRightArmSetup()->getClavicle()->getJointIndex())))->setZeroPoseAngles(0.0f, 0.048f, -0.013f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getRightArmSetup()->getShoulder()->getJointIndex())))->setZeroPoseAngles(-0.364f,	-0.016f, 0.477f);
      (static_cast<NmRs1DofEffector*>(getEffectorDirect(getRightArmSetup()->getElbow()->getJointIndex())))->setZeroPoseAngle(0.147f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getRightArmSetup()->getWrist()->getJointIndex())))->setZeroPoseAngles(0.319f,	-0.032f, 0.098f);
    }      
    else if (getBodyIdentifier() == gtaFredLarge)
    {
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getLeftArmSetup()->getClavicle()->getJointIndex())))->setZeroPoseAngles(0.112f,	0.090f,	-0.030f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getLeftArmSetup()->getShoulder()->getJointIndex())))->setZeroPoseAngles(0.112f,	-0.156f,	0.413f);
      (static_cast<NmRs1DofEffector*>(getEffectorDirect(getLeftArmSetup()->getElbow()->getJointIndex())))->setZeroPoseAngle(0.502f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getLeftArmSetup()->getWrist()->getJointIndex())))->setZeroPoseAngles(0.245f,	-0.065f,	0.245f);

      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getRightArmSetup()->getClavicle()->getJointIndex())))->setZeroPoseAngles(0.127f,	-0.047f,	-0.056f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getRightArmSetup()->getShoulder()->getJointIndex())))->setZeroPoseAngles(0.283f,	-0.127f,	0.332f);
      (static_cast<NmRs1DofEffector*>(getEffectorDirect(getRightArmSetup()->getElbow()->getJointIndex())))->setZeroPoseAngle(0.468f);
      (static_cast<NmRs3DofEffector*>(getEffectorDirect(getRightArmSetup()->getWrist()->getJointIndex())))->setZeroPoseAngles(0.206f,	-0.063f,	0.220f);
    }

    m_body.initAllLimbs();
    
    m_frictionHelper.init(this);

    if(isBiped())
    {
#if NM_USE_1DOF_SOFT_LIMITS
      // TODO integrate soft limit controller into the limbs properly.
      m_softLimitCtrls[kLeftArm].init(this,  (NmRs1DofEffector*)getEffectorDirect(gtaJtElbow_Left));
      m_softLimitCtrls[kRightArm].init(this, (NmRs1DofEffector*)getEffectorDirect(gtaJtElbow_Right));
      m_softLimitCtrls[kLeftLeg].init(this,  (NmRs1DofEffector*)getEffectorDirect(gtaJtKnee_Left));
      m_softLimitCtrls[kRightLeg].init(this, (NmRs1DofEffector*)getEffectorDirect(gtaJtKnee_Right));
#endif //NM_USE_1DOF_SOFT_LIMITS
    }

    END_PROFILING();

    setFallingReactionUpdate(0, this); // Set catchFall behaviour falling reaction related parameters to their default values.
    setCharacterCollisionsUpdate(0, this); // Set related parameters to their default values.
    m_collision_withVehicle = false;
    setCharacterDampingUpdate(0, this); // Set related parameters to their default values.
    m_spinDamping.vehicleCollisionTimer = -0.1f;
    m_spinDamping.somersaultDamping = 0.0f;
    m_spinDamping.cartwheelDamping = 0.0f;
#if NM_USE_IK_SELF_AVOIDANCE
    configureSelfAvoidanceUpdate(0, this); // BBDD: Set the self avoidance parameters to default values.
#endif //NM_USE_IK_SELF_AVOIDANCE

    return true;
  }

  bool NmRsCharacter::removeFromScene()
  {
    if(!m_level)
    {
      NM_RS_LOGERROR(L"NmRsCharacter::addToScene() Error : agent %i being removed from a null scene", getID());
      return false;
    }
    int i;

    BEGIN_PROFILING("removeFromScene");

    if(m_bodyIdent == mp3Large || m_bodyIdent == mp3Medium || m_bodyIdent == mp3Maxine)
    {
#if NM_SET_WEAPON_BOUND
      restoreHandBound(kLeftHand);
      restoreHandBound(kRightHand);
#endif
#if NM_SET_WEAPON_MASS
      // cache hand masses
      restoreHandMass(kLeftHand);
      restoreHandMass(kRightHand);
#endif
    }

#if NM_UPSIDEDOWN_FEET
    //Restore Feet to proper size
    if (m_leftFootUpsideDown)
    {
      // left
      int index = getLeftLegSetup()->getFoot()->getPartIndex();
      rage::phBound* bound = getArticulatedWrapper()->getBound(index);
      Assert(bound);
      Assert(bound->GetType() == rage::phBound::BOX);
      rage::phBoundBox* boundBox = (rage::phBoundBox*)bound;
      boundBox->SetBoxSize(VECTOR3_TO_VEC3V(m_FootSizeCacheLeft));
    }
    if (m_rightFootUpsideDown)
    {
      // right
      int index = getRightLegSetup()->getFoot()->getPartIndex();
      rage::phBound* bound = getArticulatedWrapper()->getBound(index);
      Assert(bound);
      Assert(bound->GetType() == rage::phBound::BOX);
      rage::phBoundBox* boundBox = (rage::phBoundBox*)bound;
      boundBox->SetBoxSize(VECTOR3_TO_VEC3V(m_FootSizeCacheRight));
    }
#endif//#if NM_UPSIDEDOWN_FEET

#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
    //Re-enable collisions
    if(m_needToCheckReenableCollisionForGrab)
    {
      getRightArmSetup()->getHand()->setCollisionEnabled(true);
      getRightArmSetup()->getLowerArm()->setCollisionEnabled(true);
      getLeftArmSetup()->getHand()->setCollisionEnabled(true);
      getLeftArmSetup()->getLowerArm()->setCollisionEnabled(true);
    }
#endif

    for(i=(m_effectorCount - 1); i>=0; i--)
    {
#if NM_RUNTIME_LIMITS
	  // Only restore limits if there is still a cache entry.  Not sure why there wouldn't be a cache entry here but it's happening.
	  if (((rage::fragInst*)getFirstInstance())->GetCacheEntry() && ((rage::fragInst*)getFirstInstance())->GetCacheEntry()->GetHierInst())
      m_effectors[i]->restoreLimits();
#endif
      m_effectors[i]->term();
    }

    m_incomingTm[kITSourceCurrent] = 0;
    m_incomingTm[kITSourcePrevious] = 0;
#if NM_ANIM_MATRICES
    m_incomingTm[kITSourceAnimation] = 0;
#endif
    m_incomingTmStatus = kITSNone;
    m_incomingTmCount = 0;


    END_PROFILING();

    m_level = 0;
    m_isInsertedInScene = false;
    return true;
  }

  void NmRsCharacter::prepareForSimulation()
  {
#if ART_ENABLE_BSPY
    ART::bSpyServer::inst()->setLastSeenAgent(getBSpyID());
#endif // ART_ENABLE_BSPY

    calculateCoMValues();

    //Initialise the start twist of the feet (even if airborne) to something realistic 
    //As soon as footSlipCompensation is called (only when foot is in contact) the start twist will be overwritten 
    rage::phArticulatedBody* aBody = getArticulatedBody();
    if(aBody)
    {
      m_footROrientationInitialized = false;
      m_footLOrientationInitialized = false;
      m_fscLeftTwist = twistCompensation(getLeftLegSetup()->getFoot(), m_fscLeftTwist, 0.f, 0.f, 0.f);
      m_fscRightTwist = twistCompensation(getRightLegSetup()->getFoot(), m_fscRightTwist, 0.f, 0.f, 0.f);
#if ART_ENABLE_BSPY
      //Add character levelIndex to pool of bSpy objects
      // ragdoll bounds (therefore animation) will continue to be sent to bSpy when no longer in NM 
      getEngineNotConst()->setbSpyObject(getFirstInstance()->GetLevelIndex());
#endif
    }

#if ART_ENABLE_BSPY
    ART::bSpyServer::inst()->clearLastSeenAgent();
#endif // ART_ENABLE_BSPY
  }

  void NmRsCharacter::preStep()
  {

    if (!m_isInsertedInScene)
      return;

#if NM_SCRIPTING
    if (teleport)
    {
      //Read the script only on 1st frame
      if (/*m_simTime <= 0.00000001f && */isBiped() && ctmCurrent<ctmEnd)
      {
        if (ctmCurrent<ctmStart)
          ctmCurrent = ctmStart-1;
        ctmCurrent ++;

        float deltaTime = 0.016f;
        if (ctmTimeStep > 0.0f)
          deltaTime = ctmTimeStep;
        int numBodies = 21;
        if (ctmStart != -1)
        {
        FILE *CTMFile = NULL;
        CTMFile = fopen(ctmAnimation, "rb");        

        if (CTMFile)
        {
          //CURRENT
          fseek(CTMFile, 0, SEEK_END);
          int fSize = ftell(CTMFile);
          int numOfFrames = fSize/sizeof(rage::Matrix34)/numBodies;
          if (numOfFrames >= ctmCurrent)
          {
            //Move stream pointer to the start of the ctmCurrent frame
            ::fseek(CTMFile, (ctmCurrent-1)*sizeof(rage::Matrix34)*numBodies, SEEK_SET);
            fread(currentTM, sizeof(rage::Matrix34), numBodies, CTMFile);
            if (ctmCurrent == 0)
            previousTM = currentTM;
          
            //Move stream pointer to the start of the ctmCurrent-1 frame
            ::fseek(CTMFile, (ctmCurrent-2)*sizeof(rage::Matrix34)*numBodies, SEEK_SET);
            fread(previousTM, sizeof(rage::Matrix34), numBodies, CTMFile);
          }
          fclose(CTMFile);
        }
        }
        else if (m_simTime <= 0.00000001f)
        {
          FILE *currentTMFile = NULL;
          currentTMFile = fopen("/app_home/current.ctm", "rb");        
          FILE *previousTMFile = NULL;
          previousTMFile = fopen("/app_home/previous.ctm", "rb");        
          if (currentTMFile != NULL && previousTMFile != NULL)
          { 
            //CURRENT
            fseek(currentTMFile, 0, SEEK_END);
            int fSize = ftell(currentTMFile);
            fseek(currentTMFile, 0, SEEK_SET);

            numBodies = fSize / sizeof(rage::Matrix34);
            currentTM = rage_new rage::Matrix34[numBodies];
            fread(currentTM, sizeof(rage::Matrix34), numBodies, currentTMFile);

            //int numberOfFrames = numBodies / 21;
            fclose(currentTMFile);

            //PREVIOUS
            fseek(previousTMFile, 0, SEEK_END);
            fSize = ftell(previousTMFile);
            fseek(previousTMFile, 0, SEEK_SET);

            numBodies = fSize / sizeof(rage::Matrix34);
            previousTM = rage_new rage::Matrix34[numBodies];
            fread(previousTM, sizeof(rage::Matrix34), numBodies, previousTMFile);

            //int numberOfFrames = numBodies / 21;
            fclose(previousTMFile);
          }
        }
        


        //apply the previous and current itms to the character
        if(previousTM && currentTM && !(m_simTime > 0.00000001f && ctmStart == -1))
        {
          setIncomingTransforms(currentTM, ART::kITSNone, numBodies, ART::kITSourceCurrent);
          setIncomingTransforms(previousTM, ART::kITSNone, numBodies, ART::kITSourcePrevious);

          //TELEPORT and apply incoming transforms
          int incomingComponentCount = 0;
          rage::Matrix34 *itPtr = 0;
          ART::IncomingTransformStatus itmStatusFlags = ART::kITSNone;
          getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, ART::kITSourceCurrent);
          if (!(incomingComponentCount == 0 || itPtr == 0))
          {
#if ART_ENABLE_BSPY
            bspyScratchpad(getBSpyID(), "character.teleport", true);
#endif

            rage::Matrix34* teleMatrix = &itPtr[0];
            rage::phArticulatedCollider *collider = getArticulatedWrapper()->getArticulatedCollider();

            rage::Matrix34 offMatrix(*teleMatrix);
            offMatrix.DotFromLeft(collider->GetOffsetMatrix());
            rage::Mat34V oTM = MATRIX34_TO_MAT34V(offMatrix);
            // magic spell
            collider->TeleportCollider(oTM);
            //collider->SetLastInstanceMatrix(m_inst->GetMatrix());
            collider->SetLastInstanceMatrix(getArticulatedWrapper()->getArticulatedPhysInstance()->GetMatrix());
            setIncomingTransformApplyMode(ART::kSingleFrame);
            
            float AnimationMaxSpeed = getEngine()->getAnimationMaxSpeed();
            float AnimationMaxAngSpeed = getEngine()->getAnimationMaxAngSpeed();
            getEngineNotConst()->setFromAnimationMaxSpeed(10000.0f);
            getEngineNotConst()->setFromAnimationMaxAngSpeed(10000.0f);
             applyIncomingTransforms(deltaTime);
            getEngineNotConst()->setFromAnimationMaxSpeed(AnimationMaxSpeed);
            getEngineNotConst()->setFromAnimationMaxAngSpeed(AnimationMaxAngSpeed);
            setIncomingTransformApplyMode(ART::kDisabled);

            collider->UpdateCurrentAndLastMatrices();
            //We definitely need to call prepareForSimulation - it calculates the COM values at the teleported position and initializes footSlipCompensation.
            prepareForSimulation();
            // initialize all effectors
            for(int i=0; i < getNumberOfEffectors(); i++)
              getEffectorDirect(i)->init(this);
         }
          //m_inst->PoseSkeletonFromBounds();
          //m_inst->ReportMovedBySim();

        }


      }
    }

    {
      readScript();
    }
    for (int i=0; i<10; i++) 
    {
      m_delayedMessageDelay[i] -= 1;
      if (m_delayedMessageDelay[i] == 0)
      {
        handleDirectInvoke(m_delayedMessageUID[i], &m_delayedMessageParams[i].params);
      }
    }
    m_simTime += getLastKnownUpdateStep(); 
#endif

#if ART_ENABLE_BSPY
    bspyProfileStart("NmRsCharacter::preStep")
#endif

#if NM_GRAB_DONT_ENABLECOLLISIONS_IF_COLLIDING
    if(m_needToCheckReenableCollisionForGrab)
    {
      reEnableCollision(getRightArmSetup()->getHand());
      reEnableCollision(getRightArmSetup()->getLowerArm());
      reEnableCollision(getLeftArmSetup()->getHand());
      reEnableCollision(getLeftArmSetup()->getLowerArm());
      updateReEnableCollision();
    }
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "character.Grab", m_needToCheckReenableCollisionForGrab);
#endif
#endif

    int i;



#if NM_OBJECTS_IN_COLLISIONS
    m_objectMass = 0.f;
    m_objectSize.Zero();
    m_objectFixed = false;
#endif

    m_gUp = getEngine()->getUpVector();
    m_gUpReal = m_gUp;

    // update attached object positions
    updateAttachedObject(&m_attachedObject);
    updateAttachedObject((AttachedObject*)&m_leftHandWeapon);
    updateAttachedObject((AttachedObject*)&m_rightHandWeapon);

#if ART_ENABLE_BSPY
    bspyProfileStart("limbs")
    bspyProfileStart("body")
#endif

    // Clear flag that keeps multiple stopAllBehaviours calls from flooding
    // limb requests.
    m_stopAllBehavioursSent = false;

    m_body.tick(getLastKnownUpdateStep());

#if ART_ENABLE_BSPY
    bspyProfileEnd("body")
    bspyProfileEnd("limbs")
#endif

    BEGIN_PROFILING("effectors");
#if ART_ENABLE_BSPY
    bspyProfileStart("effectors")
#endif
    if (m_effectorCount != 0)
    {
      float dtClamped = getLastKnownUpdateStepClamped();

#if ART_ENABLE_BSPY
      // output if rage is setting a minimum stiffness on joints (rage currently only sets min stiffness for all joints together)
      float rootMinStiffness = m_articulatedWrapper->getArticulatedCollider()->GetBody()->GetJoint(0).GetMinStiffness();
      bspyScratchpad(getBSpyID(), "character.Minimum joint stiffness", rootMinStiffness);
#endif

      // only preStep the effectors if the ragdoll is stable, otherwise enact a safety mechanism
#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "character.SubduingUnstableRagdoll", m_articulatedWrapper->getArticulatedCollider()->IsRagdollUnstable());
#endif

	  // Continue to let the instability prevention code even after instability is detected, as the prevention code seems to be enough to stop a freak out
	  // without the side-effects that SubdueUnstableRagdoll() has (like being frozen and allowing self collisions).  However, if the prevention code isn't
	  // running then we do seem to benefit from the more extreme safety mechanisms that SubdueUnstableRagdoll() uses.
      if (m_articulatedWrapper->getArticulatedCollider()->IsRagdollUnstable() && !((rage::fragInst*)getFirstInstance())->GetCacheEntry()->IsPreventingInstability())
      {
        m_articulatedWrapper->getArticulatedCollider()->SubdueUnstableRagdoll();
      }
      else
      {
        for(i=0; i<m_effectorCount; i++)
          m_effectors[i]->preStep(dtClamped, m_minMuscleDamping);
      }
    }

#if ART_ENABLE_BSPY
    bspyProfileEnd("effectors")
#endif
    END_PROFILING();

#if ART_ENABLE_BSPY
    bspyProfileStart("parts")
#endif
    BEGIN_PROFILING("parts");

#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "character.underwater", m_underwater);
#endif
    //Apply propulsion from hands and feet //mmmtodo clamp mult2
    if (m_underwater)
    {
#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "character.underwater", m_viscosity);
      bspyScratchpad(getBSpyID(), "character.underwater", m_articulatedWrapper->getArticulatedBody()->GetGravityFactor());
      bspyScratchpad(getBSpyID(), "character.underwater", m_stroke);
      bspyScratchpad(getBSpyID(), "character.underwater", m_linearStroke);
#endif
      rage::Vector3 handVel = getLeftArmSetup()->getHand()->getLinearVelocity();
      rage::Vector3 head2hipDir = getSpineSetup()->getPelvisPart()->getPosition() - getSpineSetup()->getHeadPart()->getPosition();
      head2hipDir.Normalize();
      float mult2;
      if (m_linearStroke)
        mult2 = handVel.Mag();
      else
        mult2 = handVel.Mag2();

      float mult = handVel.Dot(head2hipDir);
      handVel.Normalize();
      if (mult > 0.74f)
        getSpineSetup()->getSpine3Part()->applyForce(-mult*mult2*m_stroke*head2hipDir);
      //getSpineSetup()->getSpine3Part()->applyForce(-mult*mult2*m_stroke*handVel);

      handVel = getRightArmSetup()->getHand()->getLinearVelocity();
      if (m_linearStroke)
        mult2 = handVel.Mag();
      else
        mult2 = handVel.Mag2();
      handVel.Normalize();
      mult = handVel.Dot(head2hipDir);
      if (mult > 0.73f)
        getSpineSetup()->getSpine3Part()->applyForce(-mult*mult2*m_stroke*head2hipDir);
      //getSpineSetup()->getSpine3Part()->applyForce(-mult*mult2*m_stroke*handVel);

      handVel = getLeftLegSetup()->getFoot()->getLinearVelocity();
      if (m_linearStroke)
        mult2 = handVel.Mag();
      else
        mult2 = handVel.Mag2();
      handVel.Normalize();
      mult = handVel.Dot(head2hipDir);
      if (mult > 0.73f)
        getSpineSetup()->getPelvisPart()->applyForce(-mult*mult2*m_stroke*head2hipDir);
      //getSpineSetup()->getPelvisPart()->applyForce(-mult*mult2*m_stroke*handVel);

      handVel = getRightLegSetup()->getFoot()->getLinearVelocity();
      if (m_linearStroke)
        mult2 = handVel.Mag();
      else
        mult2 = handVel.Mag2();
      handVel.Normalize();
      mult = handVel.Dot(head2hipDir);
      if (mult > 0.73f)
        getSpineSetup()->getPelvisPart()->applyForce(-mult*mult2*m_stroke*head2hipDir);
      //getSpineSetup()->getPelvisPart()->applyForce(-mult*mult2*m_stroke*handVel);
    }

    if (m_viscosity > 0.f)
    {
      for (i=0; i<m_genericPartCount; i++)
      {
        m_parts[i]->resetCollided();
        fluidDamping(m_parts[i],m_viscosity);
      }
    }
    else
    {
      for (i=0; i<m_genericPartCount; i++)
        m_parts[i]->resetCollided();
    }

#if NM_FAST_COLLISION_CHECKING
    // clear part collided data
    m_collidedMask = 0;
    m_collidedEnvironmentMask = 0;
    m_collidedOtherCharactersMask = 0;
    m_collidedOwnCharacterMask = 0;
#endif
    m_collision_withVehicle = false;

    if (m_calculateInertias)
    {
      updateArticulatedWrapperInertias();
      m_calculateInertias = false;
    }

    // Invalidate cache
    m_kineticEnergyPerKiloValid = false;

#if ART_ENABLE_BSPY
    bspyProfileEnd("parts")
#endif

#if NM_USE_1DOF_SOFT_LIMITS
    m_softLimitCtrls[kLeftArm].tick();
    m_softLimitCtrls[kRightArm].tick();
    m_softLimitCtrls[kLeftLeg].tick();
    m_softLimitCtrls[kRightLeg].tick();
#endif //NM_USE_1DOF_SOFT_LIMITS

  END_PROFILING();

#if ART_ENABLE_BSPY
    if (getArticulatedBody())
    {
      BEGIN_PROFILING("skeldraw");

      // test to see if we should render the skeleton overlay
      if ((getSkeletonVizMode() != kSV_None) && (m_skeletonVizMask != 0))
        drawSkeleton(getSkeletonVizMode() == kSV_DesiredAngles);

      END_PROFILING();
    }
#endif // ART_ENABLE_BSPY

#if ART_ENABLE_BSPY && NM_BSPY_JOINT_DEBUG
    if (m_effectorCount != 0 && !bspyDebugDrawIsInhibited())
    {
      for(i=0; i<m_effectorCount; i++)
        m_effectors[i]->renderDebugDraw();
    }
#endif // ART_ENABLE_BSPY && NM_BSPY_JOINT_DEBUG

#if ART_ENABLE_BSPY
    bspyProfileEnd("NmRsCharacter::preStep")
#endif
  }

  void NmRsCharacter::setArticulatedPhysInstance(rage::phInst* inst)
  {
    if (m_articulatedWrapper)
      m_articulatedWrapper->setArticulatedPhysInstance(inst);
  }

  rage::phInst * NmRsCharacter::getArticulatedPhysInstance()
  {
    if (m_articulatedWrapper && isInsertedInScene())//NM is keeping stale phInst pointers
      return m_articulatedWrapper->getArticulatedPhysInstance();
    return 0;
  }

  void NmRsCharacter::updateArticulatedWrapperInertias()
  {
    if (m_articulatedWrapper)
      m_articulatedWrapper->getArticulatedBody()->CalculateInertias();
  }

  void NmRsCharacter::postStep(float deltaTime)
  {
    if (!m_isInsertedInScene)
      return;

    int i;

#if ART_ENABLE_BSPY
    bspyProfileStart("NmRsCharacter::postStep")
#endif

#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "Framerate ", getLastKnownUpdateStep());
#endif

    BEGIN_PROFILING("effector");
#if ART_ENABLE_BSPY
    bspyProfileStart("effectors")
#endif
    if (m_effectorCount != 0)
    {
      for(i=0; i<m_effectorCount; i++)
      {
        m_effectors[i]->postStep();
#if ART_ENABLE_BSPY
        bspyScratchpad(getBSpyID(), "Injury", m_effectors[i]->getInjuryAmount());
#endif
      }
    }
    END_PROFILING();
#if ART_ENABLE_BSPY
    bspyProfileEnd("effectors")
#endif

    // Biped specific stuff
    if(isBiped() && (getLeftLegSetup() && getRightLegSetup()))
    {
      BEGIN_PROFILING("misc");
#if ART_ENABLE_BSPY
      bspyProfileStart("misc")
	  bspyProfileStart("footslip")
#endif

      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuTaskManager->getTaskByID(getID(), bvid_dynamicBalancer);
      //Allow the feet to slip round if car collisions are being enhanced 
      if (m_collision_withVehicle && m_collision_footSlip)
      {
        m_footLOrientationInitialized = false; 
        m_footROrientationInitialized = false;
      }
      else if(m_footSlipCompensationActive && dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
        applyFootSlipCompensation(60.0f, 10.0f, m_footSlipMult);

#if ART_ENABLE_BSPY
      bspyProfileEnd("footslip")
      bspyProfileStart("posture")
#endif

      if(m_ZMPPostureControlActive)
        applyZMPPostureControl(0.05f, 0.15f);

#if ART_ENABLE_BSPY
      bspyProfileEnd("posture")
      bspyProfileStart("uprightConstraint")
#endif

#if NM_STEP_UP
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES && 0
      if(stepUp)
#endif
      {
        //Reset the foot friction to what the behaviours have set it to (this can have happened on any previous step)
        //  Needed because friction is not necessarily set every step.
        //  and stepUpHelp sets the stepping foot friction to 0.0 which needs to be switched back to what the behaviours have requested
        setStepUpFriction(1.0f, 1.0f);
        if (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
          stepUpHelp();
      }
#endif//NM_STEP_UP

      m_frictionHelper.tick();

      //Modulate friction when glancing the side of a vehicle
      if (m_glancingVehicleHit)
      {
        for (int i = 0; i<getNumberOfParts(); i++)
        {
          //Don't apply m_gsFricMask1 scalings if it's in the m_gsFricMask2
          if(isPartInMask(m_gsFricMask2, i))
          {
            NmRsGenericPart *part = getGenericPartByIndex(i);
            part->setFrictionMultiplier(part->getFrictionMultiplier()*m_gsFricScale2);
          }
          else if(isPartInMask(m_gsFricMask1, i))
          {
            NmRsGenericPart *part = getGenericPartByIndex(i);
            part->setFrictionMultiplier(part->getFrictionMultiplier()*m_gsFricScale1);
          }
        }
      }

      // check whether to apply torques at all        
      bool doTorques = m_uprightConstraint.torqueActive &&
        (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK) &&
        (!m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_staggerFall));
      if(m_uprightConstraint.torqueOnlyInAir)
        doTorques = doTorques && !(getRightLegSetup()->getFoot()->collidedWithEnvironment() || getLeftLegSetup()->getFoot()->collidedWithEnvironment());

      // check whether to apply forces at all
      bool doForces = m_uprightConstraint.forceActive &&
        (dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK) &&
        (getLeftLegSetup()->getFoot()->collidedWithNotOwnCharacter() ||
        getRightLegSetup()->getFoot()->collidedWithNotOwnCharacter());

      float noSupportForceMult = 1.f;
      if (doForces)//see if a foot could support this upright constraint
      {           
        rage::Vector3 bodyVel(m_COMvelRelative),bodyCom(m_COM);
        rage::Vector3 rightFootP(getRightLegSetup()->getFoot()->getPosition());
        rage::Vector3 leftFootP(getLeftLegSetup()->getFoot()->getPosition());
        levelVector(bodyCom, 0.f); 
        levelVector(bodyVel, 0.f); 
        bodyVel.Normalize();
        float r2ComDotVel = m_uprightConstraint.supportPosition + 1.f;//a fail condition for this foot
        float l2ComDotVel = m_uprightConstraint.supportPosition + 1.f;//a fail condition for this foot
        if (getRightLegSetup()->getFoot()->collidedWithNotOwnCharacter())
        {
          levelVector(rightFootP, 0.f);
          rage::Vector3 r2Com(bodyCom);
          r2Com -= rightFootP;
          //r2Com.Normalize();
          r2ComDotVel = r2Com.Dot(bodyVel);          
        }

        if (getLeftLegSetup()->getFoot()->collidedWithNotOwnCharacter())
        {
          levelVector(leftFootP, 0.f);
          rage::Vector3 l2Com(bodyCom);
          l2Com -= leftFootP;
          //l2Com.Normalize();
          l2ComDotVel = l2Com.Dot(bodyVel);
        }
        if ((r2ComDotVel > m_uprightConstraint.supportPosition) && (l2ComDotVel > m_uprightConstraint.supportPosition))
          noSupportForceMult = m_uprightConstraint.noSupportForceMult;
#if ART_ENABLE_BSPY
        bspyScratchpad(getBSpyID(), "stayUpright", r2ComDotVel);
        bspyScratchpad(getBSpyID(), "stayUpright", l2ComDotVel);
        bspyScratchpad(getBSpyID(), "stayUpright", noSupportForceMult);
#endif
      }

      //mmmmtodo we should probably not allow turnTowardBullets or applyLastStandUprightConstraintForces if the character is airborne
      bool balancerStandingAndActive = (!dynamicBalancerTask->m_failedIfDefaultFailure) && dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK;
      if (balancerStandingAndActive && m_uprightConstraint.turnTowardsBullets)
      {
        turnTowardBullets(deltaTime);
      }
      if(balancerStandingAndActive && m_uprightConstraint.lastStandMode)
      {
        applyLastStandUprightConstraintForces(deltaTime);
      }
      else
      {
        //Force the LastStandMode to initialize
        m_uprightConstraint.m_uprightPelvisPosition.Zero();
        
        // use tanh to get a faster than linear function that saturates at a given value
        // forceStrength K specifies the maximum value that's reached at saturation
        // saturationVel vs specifies the velocity at which saturation (and therefore forceStrength) is reached
        // thresholVel vt specifies the velocity above which the constraint becomes active
        // Equation:  S(v) = K * tanh( (3/(vs-vt)) * (v-vt) ), where S is constraint strength, and v is linear COM velocity magnitude
        //            S = max(0,S), so not less than 0
        // for a plot of this function see http://fooplot.com/index.php?&type0=0&type1=0&type2=0&type3=0&type4=0&y0=tanh((3/(4-1))*(x-1))&y1=&y2=&y3=&y4=&r0=&r1=&r2=&r3=&r4=&px0=&px1=&px2=&px3=&px4=&py0=&py1=&py2=&py3=&py4=&smin0=0&smin1=0&smin2=0&smin3=0&smin4=0&smax0=2pi&smax1=2pi&smax2=2pi&smax3=2pi&smax4=2pi&thetamin0=0&thetamin1=0&thetamin2=0&thetamin3=0&thetamin4=0&thetamax0=2pi&thetamax1=2pi&thetamax2=2pi&thetamax3=2pi&thetamax4=2pi&ipw=0&ixmin=-5&ixmax=5&iymin=-3&iymax=3&igx=1&igy=1&igl=1&igs=0&iax=1&ila=1&xmin=-0.62&xmax=7.78&ymin=-2.52&ymax=2.1199
        // empirically COMvelMag is in ~[0,5]: ~1 when shot while standing, ~5 when shot while running
        if(m_uprightConstraint.velocityBased)
        {          
          if (doForces)
          {
            float velBasedStrength = 1.0f;//apply the full upright constraint i.e. m_uprightConstraint.forceStrength
            float velocity = m_COMvelMag - m_uprightConstraint.forceThresholdVel;
            if ((m_uprightConstraint.forceSaturationVel - m_uprightConstraint.forceThresholdVel) < NM_RS_FLOATEPS)
            {
              if (velocity < m_uprightConstraint.forceThresholdVel)
                velBasedStrength = 0.0f;
            }
            else
            {
              float slope = 3.0f / (m_uprightConstraint.forceSaturationVel - m_uprightConstraint.forceThresholdVel); // 3/K scales so it saturates correctly at m_uprightConstraint.forceSaturationVel
              float velBasedStrength = tanh(slope * velocity); 
              velBasedStrength = rage::Max(0.0f, velBasedStrength); // not less than 0
            }
            velBasedStrength *= m_uprightConstraint.forceStrength;
            if (velBasedStrength > 0.0f)
              applyUprightConstraintForces(noSupportForceMult, velBasedStrength, m_uprightConstraint.forceDamping, m_uprightConstraint.forceFeetMult, m_uprightConstraint.forceLeanReduction, m_uprightConstraint.forceInAirShare, -1, -1); 
          }

          if (doTorques)
          {
            float velBasedStrength = 1.0f;//apply the full upright constraint i.e. m_uprightConstraint.forceStrength
            float velocity = m_COMvelMag - m_uprightConstraint.torqueThresholdVel;
            if ((m_uprightConstraint.torqueSaturationVel - m_uprightConstraint.torqueThresholdVel) < NM_RS_FLOATEPS)
            {
              if (velocity < m_uprightConstraint.torqueThresholdVel)
                velBasedStrength = 0.0f;
            }
            else
            {
              float slope = 3.0f / (m_uprightConstraint.torqueSaturationVel - m_uprightConstraint.torqueThresholdVel); // 3/K scales so it saturates correctly at m_uprightConstraint.forceSaturationVel
              velBasedStrength = tanh(slope * velocity); 
              velBasedStrength = rage::Max(0.0f, velBasedStrength); // not less than 0
            }
            velBasedStrength *= m_uprightConstraint.torqueStrength;
            if (velBasedStrength > 0.0f)
              stayUprightByComTorques(velBasedStrength, velBasedStrength/10.0f);           
          }
        }
        // use specified strength instead of velocity based scaling
        else
        {
          if(doForces)
            applyUprightConstraintForces(noSupportForceMult, m_uprightConstraint.forceStrength, m_uprightConstraint.forceDamping, m_uprightConstraint.forceFeetMult, m_uprightConstraint.forceLeanReduction, m_uprightConstraint.forceInAirShare, m_uprightConstraint.forceMin, m_uprightConstraint.forceMax);

          if(doTorques)
            stayUprightByComTorques(m_uprightConstraint.torqueStrength, m_uprightConstraint.torqueDamping);    
        }
      }
#if ART_ENABLE_BSPY
      bspyProfileEnd("uprightConstraint")
      bspyProfileStart("dampSpin")
#endif

      float angMomMag = m_angMom.Mag();//If using angVel instead multiply angVel by 8.7ish
      //Dampen wild spin
      NmRsGenericPart* pelvis = getSpineSetup()->getPelvisPart();
      m_spinDamping.somersaultDamping = 0.0f;
      m_spinDamping.cartwheelDamping = 0.0f;
      if ((m_spinDamping.vehicleCollisionTime < -0.0f || m_spinDamping.vehicleCollisionTimer > 0.0f) &&  (angMomMag > m_spinDamping.somersaultThresh || angMomMag > m_spinDamping.cartwheelThresh))
      {
        rage::Matrix34 tmCom = m_COMTM;
        rage::Vector3 bodyRight = tmCom.a;
        rage::Vector3 bodyBack = tmCom.c;

        rage::Vector3 comAngMom = m_angMom;    
        if (getBodyIdentifier() == gtaWilma)
          comAngMom *= 1.26f;
        float somMom = comAngMom.Dot(bodyRight);
        float tiltMom = comAngMom.Dot(bodyBack);
        
#if ART_ENABLE_BSPY
        bspyScratchpad(getBSpyID(), "DampSpin", angMomMag);
        bspyScratchpad(getBSpyID(), "DampSpin", somMom);
        bspyScratchpad(getBSpyID(), "DampSpin", tiltMom);
#endif
        float mult = 1.0f;
        //angMom
        rage::Vector3 torque;
        float scale = rage::Clamp(60.f*getLastKnownUpdateStep(), 1.0f, 60.0f);//clamp to keep torque same above 60fps and reduce torque from 60fps to 1fps
        Assert(scale > 0.f);
        if (m_spinDamping.somersaultDamp > 0.0f && rage::Abs(somMom) > m_spinDamping.somersaultThresh)
        {
          somMom += rage::Selectf(somMom, -m_spinDamping.somersaultThresh, m_spinDamping.somersaultThresh);
          torque = bodyRight;
          if (m_spinDamping.v2)
            mult = rage::Abs(somMom);
          m_spinDamping.somersaultDamping = - somMom * mult * m_spinDamping.somersaultDamp;
          torque *=  m_spinDamping.somersaultDamping;
          pelvis->applyTorque(torque / scale);
        }
        if (m_spinDamping.cartwheelDamp > 0.0f && rage::Abs(tiltMom) > m_spinDamping.cartwheelThresh)
        {
          tiltMom += rage::Selectf(tiltMom, -m_spinDamping.cartwheelThresh, m_spinDamping.cartwheelThresh);
          torque = bodyBack;
          mult = 1.0f;
          if (m_spinDamping.v2)
            mult = rage::Abs(tiltMom);
          m_spinDamping.cartwheelDamping = - tiltMom * mult * m_spinDamping.cartwheelDamp;
          torque *=  m_spinDamping.cartwheelDamping;
          pelvis->applyTorque(torque / scale);
        }
       }
      if (m_spinDamping.vehicleCollisionTimer >= 0.0f)
        m_spinDamping.vehicleCollisionTimer -= getLastKnownUpdateStep();
#if ART_ENABLE_BSPY
      bspyProfileEnd("dampSpin")
      bspyProfileStart("rememberSetStiffness")
#endif

      if (m_rememberSetStiffness)
      {
#if ART_ENABLE_BSPY
        setCurrentSubBehaviour("-MsetStiff");
#endif
        NmRsBodyStateHelper helper(
          &m_body, 
          bvid_DirectInvoke,
          m_rememberStiffnessPriority, 0,
          m_rememberStiffnessBlend, m_rememberStiffnessMask
          DEBUG_LIMBS_PARAMETER("setStiffness"));

        m_body.setStiffness(m_rememberStiff, m_rememberDamp, m_rememberStiffnessMask);

        m_rememberSetStiffness = false;

#if ART_ENABLE_BSPY 
        setCurrentSubBehaviour(""); 
#endif
      }

      if (m_rememberSetMuscleStiffness)
      {
#if ART_ENABLE_BSPY
        setCurrentSubBehaviour("-MsetMStiff");
#endif

        NmRsBodyStateHelper helper( 
          &m_body, 
          bvid_DirectInvoke,
          m_rememberMuscleStiffnessPriority, 0,
          m_rememberMuscleStiffnessBlend, m_rememberMuscleStiffnessMask
          DEBUG_LIMBS_PARAMETER("setMuscleStiffness"));

        m_body.callMaskedEffectorDataFunctionFloatArg(
          m_rememberMuscleStiffnessMask,
          m_rememberMuscleStiff,
          &NmRs1DofEffectorInputWrapper::setMuscleStiffness,
          &NmRs3DofEffectorInputWrapper::setMuscleStiffness);

        m_rememberSetMuscleStiffness = false;

#if ART_ENABLE_BSPY 
        setCurrentSubBehaviour("");
#endif
      }
#if ART_ENABLE_BSPY
      bspyProfileEnd("rememberSetStiffness")
#endif

      // limbs todo check that this is actually in use and implement limbs-friendly.
      // intended to overwrite anything that is active, so give a very high
      // priority.
      if (handCuffs)
      {
#if ART_ENABLE_BSPY
        setCurrentSubBehaviour("-Cuffs"); 
#endif
        //PARAMETER(handCuffs, false, bool, false, true);
        //PARAMETER(handCuffsBehindBack, false, bool, false, true);
        //PARAMETER(legCuffs, false, bool, false, true);
        //PARAMETER(rightDominant, false, bool, false, true);
        //PARAMETER(passiveMode, 0, int, 0, 5);//0 setCurrent, 1= IK to dominant, 2=pointGunLikeIK  
        //PARAMETER(bespokeBehaviour, false, bool, false, true);
        //PARAMETER(blend2ZeroPose, 0, float, 0.0, 1.f);//Blend Arms to zero pose
        NmRsBodyStateHelper helper(
          &m_body, 
          bvid_DirectInvoke,
          100, 0,
          1.0f, bvmask_Arms
          DEBUG_LIMBS_PARAMETER("handcuffs"));

        if (rightDominant)
        {
          if (passiveMode == 0)
          {
            m_body.holdPose(bvmask_ArmLeft);
          }
          else if (passiveMode == 1)
          {
            NmRsLimbInput ikInput = m_body.createNmRsLimbInput<NmRsIKInputWrapper>();
            NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
            ikInputData->setTarget(m_body.getRightArm()->getWrist()->getJointPosition());
            ikInputData->setTwist(0.0f);
            m_body.getLeftArm()->postInput(ikInput);
          }
        }
        if (blend2ZeroPose>0.001f)
        {
          m_body.blendToZeroPose(blend2ZeroPose, bvmask_Arms);
        }

#if ART_ENABLE_BSPY
        setCurrentSubBehaviour(""); 
#endif
      }

      END_PROFILING();
#if ART_ENABLE_BSPY
      bspyProfileEnd("misc")
#endif
    }

#if ART_ENABLE_BSPY
    bspyProfileStart("misc2")
    bspyProfileStart("impulseLeakage")
#endif
    // update impulse leakage: TODO: how to do this  so the impulse scalar will recover smoothly even 
    // moved to before impulse applied as want to be able to ensure full impulse for current bullet by specifying
    // impulseRecoveryPerTick = 1 (i.e. m_impulseRecovery*getLastKnownUpdateStep() = 1)
    // when no impulse/torque is currently applied? Entry condition: m_impulseLeakage <= 0.99; !!!!!!!
    // the less healthy/strong the character, the quicker impulses return to full strength
    if (m_impulseLeakage < 0.999f)
    {
      float cStrengthImpRecGain = 100000.0f;
      if (m_strength > 0.00001f)
        cStrengthImpRecGain = m_impulseLeakageStrengthScaled ? 1/m_strength : 1.0f;
      m_impulseLeakage += (1.f-m_impulseLeakage)*m_impulseRecovery*getLastKnownUpdateStep()*cStrengthImpRecGain;
      m_impulseLeakage = rage::Clamp(m_impulseLeakage, 0.f, 1.f);
    }
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "character", m_impulseLeakage);
#endif

#if ART_ENABLE_BSPY
    bspyProfileEnd("impulseLeakage")
    bspyProfileStart("applyBullets")
#endif
    // extended bullet application step
    for (i=0; i<NUM_OF_BULLETS; i++)
    {
      m_bulletApplier[i].autoUpdate(getLastKnownUpdateStep());
    }

    // clear weapon mode change flag
    m_weaponModeChanged = false;

#if ART_ENABLE_BSPY
    bspyProfileEnd("applyBullets")
    bspyProfileStart("collisionExclusion")
#endif

#if ART_ENABLE_BSPY & 0
    m_rightHandCollisionExclusion.debug(getBSpyID(), "ArmCollision");
#if NM_NEW_LEG_COLLISION_CODE
    m_Leg2LegCollisionExclusion.debug(getBSpyID(), "Leg2LegCollisionExclusion");
#endif
#endif
    // update hand collision sets.
    m_rightHandCollisionExclusion.update();
    // update leg collision sets.
#if NM_NEW_LEG_COLLISION_CODE
    m_Leg2LegCollisionExclusion.update();
#endif

#if ART_ENABLE_BSPY
    m_currentFrame++;
#endif

#if ART_ENABLE_BSPY
    bspyProfileEnd("collisionExclusion")
    bspyProfileEnd("misc2")
#endif

    // debug dynamic bounds.
#if ART_ENABLE_BSPY && NM_SET_WEAPON_BOUND && 0//!USE_RAGE_RAGDOLL Will have to get linkAttachement matrices from the ragdoll
    if (m_rsEngine->getNmRsSpy()->isClientConnected())
    {
      NmRsArticulatedWrapper* articulatedWrapper = getArticulatedWrapper();
      Assert(articulatedWrapper);
      NmRsGenericPart* part = getLeftArmSetup()->getHand();
      Assert(part);
      rage::phBound* bound = (rage::phBound*)part->getBound();
      if (bound)
      {
        rage::Matrix34 instTm, tm;
        part->getMatrix(instTm);
        rage::Matrix34* linkMats = articulatedWrapper->getLinkAttachmentMatrices();
        rage::Matrix34 linkMat(linkMats[part->getPartIndex()]);
        tm.Dot(linkMat, instTm);
        bspyDrawCoordinateFrame(0.1f, tm);
        rage::phInst* inst = articulatedWrapper->getArticulatedPhysInstance();
        bSpyProcessDynamicBoundOnContact(inst, bound, instTm, getID());
      }
      part = getRightArmSetup()->getHand();
      Assert(part);
      bound = (rage::phBound*)part->getBound();
      if (bound)
      {
        rage::Matrix34 instTm, tm;
        part->getMatrix(instTm);
        rage::Matrix34* linkMats = articulatedWrapper->getLinkAttachmentMatrices();
        rage::Matrix34 linkMat(linkMats[part->getPartIndex()]);
        tm.Dot(instTm, linkMat);
        rage::phInst* inst = articulatedWrapper->getArticulatedPhysInstance();
        bSpyProcessDynamicBoundOnContact(inst, bound, instTm, getID());
      }
    }
#endif // ART_ENABLE_BSPY && NM_SET_WEAPON_BOUND && !USE_RAGE_RAGDOLL

#if NM_UPSIDEDOWN_FEET
#define NM_UPSIDEDOWN_FEET_SCALE 0.8f
    //1)Stop the character getting stuck on geometry hanging from it's heels upsidedown.
    //2)Help the character not to pivot around feet/get feet stuck when falling over cliff forwards.  
    //  2.1)To stop this affecting falling down slope behaviour friction is only turned off if the rest of the body is not in contact 
    //Reduce the foot length so that the heel is inside the lower leg bound.
    //Set foot friction to 0.0 (this will help stop hanging from feet in general)
    //  Note foot friction is set to 0.0 in handleCollisions
    //  This overrides any footFriction modulation in the behaviours or elsewhere.
    //We could id the collision is on the top of the foot - this would also help to not pivot around feet/get feet stuck when falling over cliff forwards
    rage::Matrix34 footTM;
    bool leftFootUpsideDown = false;
    bool rightFootUpsideDown = false;
    //Character is upSideDown if feet are 76.6cm above the pelvis.  For straight legs this will fire on >60deg slope. (probably less as legs <1m long)
    //Make this value larger if it activates on slopes when not required
    const float verticalDistance = 0.766f;
    //Don't allow foot changes if foot velocity above footVelocity
    const float footVelocity = 1.8f;
    bool upperBodyCollided = hasCollidedWithWorld(bvmask_Full &~ (bvmask_FootRight | bvmask_FootLeft | bvmask_ShinRight | bvmask_ShinLeft));
    //A way to stop this happening so much would be to not do it if the foot speed is high.  Can't be too low otherwise going over the cliff might bite
    if (getLeftLegSetup()->getFoot()->getLinearVelocity().Mag() < footVelocity)
    {
      //Is character hanging upsideDown?
      float leftFoot2Pelvis = (getSpineSetup()->getPelvisPart()->getPosition() - getLeftLegSetup()->getAnkle()->getJointPosition()).Dot(m_gUpReal);
      leftFootUpsideDown = (leftFoot2Pelvis< -verticalDistance);
      //Will character pivot around feet/get feet stuck when falling over cliff forwards.
      if (!upperBodyCollided)
      {
        getLeftLegSetup()->getFoot()->getBoundMatrix(&footTM);
        leftFootUpsideDown |= ((m_gUpReal.Dot(footTM.b) < -0.3f) && (leftFoot2Pelvis < -0.1f));
      }
    }
    if (getRightLegSetup()->getFoot()->getLinearVelocity().Mag() < footVelocity)
    {
      //Is character hanging upsideDown?
      float rightFoot2Pelvis = (getSpineSetup()->getPelvisPart()->getPosition() - getRightLegSetup()->getAnkle()->getJointPosition()).Dot(m_gUpReal);
      rightFootUpsideDown = (rightFoot2Pelvis < -verticalDistance);
      //Will character pivot around feet/get feet stuck when falling over cliff forwards.
      if (!upperBodyCollided)
      {
        getRightLegSetup()->getFoot()->getBoundMatrix(&footTM);
        rightFootUpsideDown |= ((m_gUpReal.Dot(footTM.b) < -0.3f) && (rightFoot2Pelvis < -0.1f));
      }
    }
    if(leftFootUpsideDown != m_leftFootUpsideDown)
    {
      // left
      int index = getLeftLegSetup()->getFoot()->getPartIndex();
      rage::phBound* bound = getArticulatedWrapper()->getBound(index);
      Assert(bound);
      Assert(bound->GetType() == rage::phBound::BOX);
      rage::phBoundBox* boundBox = (rage::phBoundBox*)bound;
      rage::Vector3 boxSize(m_FootSizeCacheLeft);
      if (leftFootUpsideDown)
      {
        boxSize.Set(VEC3V_TO_VECTOR3(boundBox->GetBoxSize()));
        m_FootSizeCacheLeft.Set(boxSize); // cache size for later restore.
        boxSize.z *= NM_UPSIDEDOWN_FEET_SCALE;//foot length
      }
      boundBox->SetBoxSize(VECTOR3_TO_VEC3V(boxSize));
    }
    if(rightFootUpsideDown != m_rightFootUpsideDown)
    {
      // right
      int index = getRightLegSetup()->getFoot()->getPartIndex();
      rage::phBound* bound = getArticulatedWrapper()->getBound(index);
      Assert(bound);
      Assert(bound->GetType() == rage::phBound::BOX);
      rage::phBoundBox* boundBox = (rage::phBoundBox*)bound;
      rage::Vector3 boxSize(m_FootSizeCacheRight);
      if (rightFootUpsideDown)
      {
        boxSize.Set(VEC3V_TO_VECTOR3(boundBox->GetBoxSize()));
        m_FootSizeCacheRight.Set(boxSize);
        boxSize.z *= NM_UPSIDEDOWN_FEET_SCALE;
      }
      boundBox->SetBoxSize(VECTOR3_TO_VEC3V(boxSize));
    }
    m_leftFootUpsideDown = leftFootUpsideDown;
    m_rightFootUpsideDown = rightFootUpsideDown;
#if ART_ENABLE_BSPY
    // inform bspy about changed foot bounds.
    if (m_leftFootUpsideDown)
    {
      //Draw bound but this is difficult to see in bSpy (as it is inside the normal foot bound)
      NmRsGenericPart* part = getLeftLegSetup()->getFoot();
      Assert(part);
      rage::phBound* subBound = (rage::phBound*)part->getBound();
      if (subBound)
      {
        rage::Matrix34 partWorld;
        part->getMatrix(partWorld);
        bSpyProcessDynamicBoundOnContact(getFirstInstance(), subBound, partWorld, getID());
      }
      float footLength = 0.5f*m_FootSizeCacheLeft.z*NM_UPSIDEDOWN_FEET_SCALE;
      float footWidth = 0.5f*m_FootSizeCacheLeft.x;
      float footDepth = 0.5f*m_FootSizeCacheLeft.y;
      getLeftLegSetup()->getFoot()->getBoundMatrix(&footTM);
      rage::Vector3 backLeft = footTM.d; 
      backLeft.AddScaled(footTM.b, footDepth);
      backLeft.AddScaled(footTM.a, -footWidth);
      backLeft.AddScaled(footTM.c,  footLength);
      rage::Vector3 frontLeft = footTM.d;
      frontLeft.AddScaled(footTM.b, footDepth);
      frontLeft.AddScaled(footTM.a, -footWidth);
      frontLeft.AddScaled(footTM.c, -footLength);
      rage::Vector3 frontRight = footTM.d;
      frontRight.AddScaled(footTM.b, footDepth);
      frontRight.AddScaled(footTM.a,  footWidth);
      frontRight.AddScaled(footTM.c, -footLength);
      rage::Vector3 backRight = footTM.d;
      backRight.AddScaled(footTM.b, footDepth);
      backRight.AddScaled(footTM.a,  footWidth);
      backRight.AddScaled(footTM.c,  footLength);
      rage::Vector3 col(1,0,0);
      bspyDrawLine(backLeft, backRight, col);
      bspyDrawLine(backRight, frontRight, col);
      bspyDrawLine(frontRight, frontLeft, col);
      bspyDrawLine(frontLeft, backLeft, col);

    }    // inform bspy about changed foot bounds.
    if (m_rightFootUpsideDown)
    {
      //Draw bound but this is difficult to see in bSpy (as it is inside the normal foot bound)
      NmRsGenericPart* part = getRightLegSetup()->getFoot();
      Assert(part);
      rage::phBound* subBound = (rage::phBound*)part->getBound();
      if (subBound)
      {
        rage::Matrix34 partWorld;
        part->getMatrix(partWorld);
        bSpyProcessDynamicBoundOnContact(getFirstInstance(), subBound, partWorld, getID());
      }
      float footLength = 0.5f*m_FootSizeCacheRight.z*NM_UPSIDEDOWN_FEET_SCALE;
      float footWidth = 0.5f*m_FootSizeCacheRight.x;
      float footDepth = 0.5f*m_FootSizeCacheRight.y;
      getRightLegSetup()->getFoot()->getBoundMatrix(&footTM);
      rage::Vector3 backLeft = footTM.d; 
      backLeft.AddScaled(footTM.b, footDepth);
      backLeft.AddScaled(footTM.a, -footWidth);
      backLeft.AddScaled(footTM.c,  footLength);
      rage::Vector3 frontLeft = footTM.d;
      frontLeft.AddScaled(footTM.b, footDepth);
      frontLeft.AddScaled(footTM.a, -footWidth);
      frontLeft.AddScaled(footTM.c, -footLength);
      rage::Vector3 frontRight = footTM.d;
      frontRight.AddScaled(footTM.b, footDepth);
      frontRight.AddScaled(footTM.a,  footWidth);
      frontRight.AddScaled(footTM.c, -footLength);
      rage::Vector3 backRight = footTM.d;
      backRight.AddScaled(footTM.b, footDepth);
      backRight.AddScaled(footTM.a,  footWidth);
      backRight.AddScaled(footTM.c,  footLength);
      rage::Vector3 col(1,0,0);
      bspyDrawLine(backLeft, backRight, col);
      bspyDrawLine(backRight, frontRight, col);
      bspyDrawLine(frontRight, frontLeft, col);
      bspyDrawLine(frontLeft, backLeft, col);

    }
#endif//#if ART_ENABLE_BSPY
#endif//#if NM_UPSIDEDOWN_FEET

#if ART_ENABLE_BSPY
    bspyProfileEnd("NmRsCharacter::postStep")
#endif
  }

  rage::phMaterial& NmRsCharacter::getMaterial(const char* name)
  {
    PHMaterialManager* mtlMgr = (PHMaterialManager*)(&(MATERIALMGR.GetInstance()));

    rage::phMaterial& partMtl = (rage::phMaterial&)mtlMgr->FindMaterial(name);
    if (&partMtl != &mtlMgr->GetDefaultMaterial())
      return partMtl;
    else
      return mtlMgr->AllocateMaterial(name);
  }

  void NmRsCharacter::setImpactFriction(rage::phContactIterator impact, const float scale, const float min, const float max)
  {
    // NOTE: We can't just call impact.GetFriction(), because it doesn't get refreshed
    // every frame.  As a result, this multiply would accumulate and eventually overflow
    // the float.
    //For new Rage
    //const rage::phMaterial& materialA = MATERIALMGR.GetMaterial(impact.GetContact().GetMaterialIdA());
    //const rage::phMaterial& materialB = MATERIALMGR.GetMaterial(impact.GetContact().GetMaterialIdB());
    //for old rage used by euphoria
    const rage::phMaterial& materialA = MATERIALMGR.GetMaterial(impact.GetMyMaterialId());
    const rage::phMaterial& materialB = MATERIALMGR.GetMaterial(impact.GetOtherMaterialId());

	// Always consider the other material's friction to be constant (no one likes sliding ragdolls on snow, for example)
	static float s_desiredEnvironmentFriction = 1.0f;
	float otherFriction = PHLEVEL->IsFixed(impact.GetOtherInstance()->GetLevelIndex()) ? s_desiredEnvironmentFriction : materialB.GetFriction();

    float fNewFriction = rage::Clamp(materialA.GetFriction() * otherFriction * scale, min, max);
    fNewFriction = rage::Clamp(fNewFriction, min, max);
    impact.SetFriction(fNewFriction);
  }

  void NmRsCharacter::scaleElasticity(rage::phContactIterator impact, const float scale)
  {
    // NOTE: We can't just call impact.GetFriction(), because it doesn't get refreshed
    // every frame.  As a result, this multiply would accumulate and eventually overflow
    // the float.
    //For new Rage
    //const rage::phMaterial& materialA = MATERIALMGR.GetMaterial(impact.GetContact().GetMaterialIdA());
    //const rage::phMaterial& materialB = MATERIALMGR.GetMaterial(impact.GetContact().GetMaterialIdB());
    //for old rage used by euphoria
    const rage::phMaterial& materialA = MATERIALMGR.GetMaterial(impact.GetMyMaterialId());
    const rage::phMaterial& materialB = MATERIALMGR.GetMaterial(impact.GetOtherMaterialId());
    float fNewElasticity = materialA.GetElasticity() * materialB.GetElasticity() * scale;
    impact.SetElasticity(fNewElasticity);
  }

  NmRsGenericPart* NmRsCharacter::addArticulated(rage::phArticulatedBodyPart* artpart, int partIndex)
  {
    NmRsGenericPart *newPart;
    ARTCustomPlacementNew3Arg(newPart, NmRsGenericPart, artpart, partIndex, this);

    m_articulatedWrapper->addPart(newPart);

    Assert(partIndex >= 0 && partIndex < m_genericPartCount);
    m_parts[partIndex] = newPart;

    return newPart;
  }

#if NM_CHECK_VALID_VALUES
  bool NmRsCharacter::CheckValidVector(const rage::Vector3 &v, int ASSERT_ONLY(checkID))
  {
	  float sizeMax = 5000.0f;
	  rage::Vector3 vec = v;
	  bool bInvalid = (vec.x != vec.x) | (vec.y != vec.y) | (vec.z != vec.z) | (v.Mag2() > sizeMax*sizeMax);
#if __ASSERT
	  if (bInvalid)
			{
			nmDebugf3("NmRsCharacter::CheckValidVector - Bad input vector. Vector is (%f, %f, %f). CheckID is %d.", vec.x, vec.y, vec.z, checkID);
			}
#endif
	  return !bInvalid;
		}

  bool NmRsCharacter::CheckValidFloat(const float f, float minSize, float maxSize, int ASSERT_ONLY(checkID))
  {
	  bool bInvalid = (f != f) | (f > maxSize) | (f < minSize);
#if __ASSERT
	  if (bInvalid)
	  {
			nmDebugf3("NmRsCharacter::CheckValidFloat - Bad input float. float is %f. CheckID is %d.", f, checkID);
  }
#endif
	  return !bInvalid;
  }
#endif

  void NmRsCharacter::ResetSavedVelocities()
  {
	  // Reset the saved velocities, since they'll be used to store impulses applied during stepPhase2
	  rage::phArticulatedCollider *collider = getArticulatedWrapper()->getArticulatedCollider();
	  if (collider)
	  {
		  collider->ResetSavedVelocities();
	  }
	  memset(sm_ImpulsePendingForLink, 0, MAX_NUM_LINKS * sizeof(bool));
  }

  void NmRsCharacter::AddDeferredImpulse(int link, rage::Vec3V_In trans, rage::Vec3V_In omega)
  {
	  rage::phArticulatedCollider *collider = getArticulatedWrapper()->getArticulatedCollider();
	  if (collider)
	  {
		  collider->m_SavedLinearVelocities[link] += trans;
		  collider->m_SavedAngularVelocities[link] += omega;
		  sm_ImpulsePendingForLink[link] = 1;
	  }
	  }

  void NmRsCharacter::ApplyAccumulatedImpulses()
  {
		rage::phArticulatedCollider *collider = getArticulatedWrapper()->getArticulatedCollider();
		rage::phArticulatedBody *body = getArticulatedWrapper()->getArticulatedBody();
		if (collider && body)
		{
			Assert(getNumberOfParts() <= MAX_NUM_LINKS);
			int iNumBodyParts = rage::Min(getNumberOfParts(), MAX_NUM_LINKS);
			int numNonZeroEntries = 0;
			int lastNonZeroEntry = 0;
			int iLink = 0;

			// Skip links that didn't get impulses applied
			for (int iLink = 0; iLink < iNumBodyParts; iLink++)
  {
				if (sm_ImpulsePendingForLink[iLink] != 0)
		{
					sm_ImpulsePendingForLink[lastNonZeroEntry++] = iLink;
					numNonZeroEntries++;
				}
		}

			// I counted the number of propUps and propDowns for the case of applying impulse from root to leaves and from leaves to root.
			// The total number for up and down props is the same, but going from leaves to root uses more ups than downs, and downs have a branch.  
			for (int iNonZeroEntry = numNonZeroEntries-1; iNonZeroEntry >= 0; iNonZeroEntry--)
			{
				iLink = sm_ImpulsePendingForLink[iNonZeroEntry];
				body->ApplyPhaseSpaceImpulse(iLink, collider->m_SavedLinearVelocities[iLink], collider->m_SavedAngularVelocities[iLink]);
			}

			collider->ResetSavedVelocities();
  }
  }


  void NmRsCharacter::calculateCoMValues()
  {
#if ART_ENABLE_BSPY
    bspyScopedProfilePoint("calculateCoMValues");
#endif // ART_ENABLE_BSPY

    // !hdd! could be overkill, but we regularly access the links (mass / linvel)
    // inside the articulated body in several of the functions below
    rage::phArticulatedBody* aBody = getArticulatedBody();
    if(aBody)
    {
      aBody->PrefetchForUpdate();

      // !hdd! this now does all the work at once, rather than having multiple
      // runs over the body parts, much more efficient
      calculateCoM(&m_COMTM, &m_COMvel, &m_COMrotvel, &m_angMom);

      m_COM = m_COMTM.d;
      m_COMvelMag = m_COMvel.Mag();
      m_COMrotvelMag = m_COMrotvel.Mag();
      m_COMvelRelative = m_COMvel - m_floorVelocity;
      m_COMvelRelativeMag = m_COMvelRelative.Mag();
    }
  }

  // Updates COMTM orientation using angular velocity over given time duration to calculate predicted COMTM orientation.
  // NOTE: the COMTM position is unaffected.
  void NmRsCharacter::getPredictedCOMOrientation(float predictTime, rage::Matrix34* predTM) const
  {
    Assert(predTM);

    rage::Vector3 angVelCOM;
#if NM_RIGID_BODY_BULLET //m_characterInertiaAboutComInN is only calculated #if NM_RIGID_BODY_BULLET
    // Calculate angular velocity from angular momentum.
    // NOTE: m_COMrotvel could be used, however
    // calculating from angular momentum proved to be more accurate
    // especially when Agent is spinning about its spine axis due to the approximations
    // that were used to calculate m_COMrotvel.
    angVelCOM = m_angMom;
    rage::Matrix34 invCharacterInertiaAboutComInN(m_characterInertiaAboutComInN);
    invCharacterInertiaAboutComInN.Inverse3x3();
    invCharacterInertiaAboutComInN.Transform3x3(angVelCOM);
#else
    angVelCOM = m_COMrotvel;
#endif

    // Make COM orientation quaternion.
    rage::Quaternion qCOM;
    m_COMTM.ToQuaternion(qCOM);
    qCOM.Normalize();

    // NOTE: angular velocity is assumed to be constant
    // throughout the prediction time duration which is only an approximation.
    // Updates COMTM orientation using angular velocity over given time duration to calculate predicted COMTM orientation.
    rage::Quaternion qPredCOM;
    rage::Quaternion deltaQ;
    angVelCOM *= predictTime;
    deltaQ.FromRotation(angVelCOM);//Change in orientation caused by angVel over interval predictTime. Returns identity if angVel is very small.
    qPredCOM.Multiply(deltaQ, qCOM);
    qPredCOM.Normalize();//mmmmtodo without this we sometime get an assert in FromQuaternion below - See B*624182.

    // Convert back to matrix.
    (*predTM).FromQuaternion(qPredCOM);

    // Keep the original COM translation.
    predTM->d = m_COMTM.d;
  }

  // Check which way Agent is facing from specified COM orientation.
  NmRsCharacter::OrientationStates NmRsCharacter::getFacingDirectionFromCOMOrientation(const rage::Matrix34 &COMOrient, const rage::Vector3 * up) const
  {
    if(up == NULL)
      up = &m_gUp;

    const float upDotUp = (COMOrient.b).Dot(*up);
    const float leftDotUp = (-COMOrient.a).Dot(*up);
    const float frontDotUp = (-COMOrient.c).Dot(*up);

    OrientationStates facingDirState = (frontDotUp < 0.25f) ? OS_Front : OS_Back;

    // Agent sideways?
    if ((rage::Abs(leftDotUp) - rage::Abs(frontDotUp)) > 0.15f)
    {
      facingDirState = (leftDotUp < 0.0f) ? OS_Left : OS_Right;
    }

    // Upright instead?
    if ((rage::Abs(upDotUp) > 0.8f) ||
         ((rage::Abs(upDotUp) > rage::Abs(frontDotUp))
         &&(rage::Abs(upDotUp) > rage::Abs(leftDotUp))
         &&(rage::Abs(frontDotUp) < 0.75f)
         &&(rage::Abs(leftDotUp) < 0.75f)
         &&(rage::Abs(upDotUp) > 0.75f)))
    {
      facingDirState = (upDotUp > 0.0f) ? OS_Up : OS_Down;
    }

    return facingDirState;
  }

  NmRsGenericPart* NmRsCharacter::getGenericPartByIndex(int index) const
  {
    Assert(index >= 0 && index < m_genericPartCount);
    // In addition to the assert above, added protection against the crash for gta4
    // because we had seen the assert getting hit, but couldn't figure out why
    if(index < 0 || index >= m_genericPartCount)
      return m_parts[0];
    return m_parts[index];
  }

  const rage::phBound* NmRsCharacter::getBoundByComponentIdx(int idx) const
  {
    NmRsGenericPart* part = getGenericPartByIndex(idx);
    if (!part)
      return 0;

    return part->getBound();
  }
#if NM_UNUSED_CODE
  NmRsGenericPart* NmRsCharacter::lookupPartForInstance(rage::phInst *inst) const
  {
    int gpIndex = -1;
    if (decodeUserData(inst->GetUserData(), gpIndex))
    {
      Assert(gpIndex >= 0 && gpIndex < m_genericPartCount);
      return m_parts[gpIndex];
    }
    return 0;
  }
#endif

  bool NmRsCharacter::getMatrixForPartByComponentIdx(int index, rage::Matrix34& mtm) const
  {
    NmRsGenericPart* part = getGenericPartByIndex(index);
    if (!part)
      return false;

    part->getMatrix(mtm);
    mtm.NormalizeSafe();

    return true;
  }

  float NmRsCharacter::getLastKnownUpdateStep() const
  {
    return m_rsEngine->getLastKnownUpdateStep();
  }

  float NmRsCharacter::getLastKnownUpdateStepClamped() const
  {
    return m_rsEngine->getLastKnownUpdateStepClamped();
  }

#if ART_ENABLE_BSPY

  void NmRsCharacter::bSpyProcessInstanceOnContact(rage::phInst* inst, int collidingNMAgent)
  {     
    if (getBSpyID() == INVALID_AGENTID)
      return;
    rage::phBound* bound = inst->GetArchetype()->GetBound();
    rage::phBound::BoundType boundType = (rage::phBound::BoundType)bound->GetType();
    int instLevelIndex = inst->GetLevelIndex();

    NmRsSpy& spy = *m_rsEngine->getNmRsSpy();

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
#define NM_SEND_CONTACT_DYNAMIC_OBJECTS_LATER 1
#if NM_SEND_CONTACT_DYNAMIC_OBJECTS_LATER
      (void) collidingNMAgent;
      getEngineNotConst()->setbSpyObject(instLevelIndex);
#else
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
        //Add levelIndex of contacted object to pool of bSpy objects
        // Contacted object will continue to be sent to bSpy when no longer in contact 
        getEngineNotConst()->setbSpyObject(instLevelIndex);      
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

            bSpyProcessDynamicBoundOnContact(inst, subBound, partWorld, collidingNMAgent);
          }
        }
      }
      else
      {
        bSpyProcessDynamicBoundOnContact(inst, bound, instTm, collidingNMAgent);
      }
#endif//NM_SEND_CONTACT_DYNAMIC_OBJECTS_LATER
    }//if (spy.shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_DynamicCollisionShapes))
  }

  void NmRsCharacter::bSpyProcessDynamicBoundOnContact(rage::phInst* inst, rage::phBound* bound, const rage::Matrix34& tm, int collidingNMAgent)
  {
    if (collidingNMAgent == (int)INVALID_AGENTID)
      return;

    rage::phBound::BoundType boundType = (rage::phBound::BoundType)bound->GetType();
    DynamicCollisionShapePacket dcs( (bs_uint32)inst->GetLevelIndex(), (bs_uint32)bound, (bs_int16)collidingNMAgent, (bs_int32)inst->GetClassType() );
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

        unsigned int sessionUID = m_rsEngine->getNmRsSpy()->getSessionUID();

        NmRsSpy& spy = *m_rsEngine->getNmRsSpy();
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

            rage::Vector3 v0, v1, v2, un;
            bGeom->GetPolygonVertices(poly, RC_VEC3V(v0), RC_VEC3V(v1), RC_VEC3V(v2));

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

#endif // ART_ENABLE_BSPY

  void NmRsCharacter::handleCollision(rage::phContactIterator impact)
  {

    NmRsGenericPart *gpA = 0, *gpB = 0;
    rage::phInst *a = impact.GetMyInstance();
    rage::phInst *b = impact.GetOtherInstance();
    rage::Vector3 normalA, normalB;

    if(a&&b)
    {
#if 0 //Unused at the moment
      // Get the impulse (from the last frame)
      rage::Vec3V worldImpulse;
      if (a->GetNMAgentID() != -1 || b->GetNMAgentID() != -1)
    		{
          rage::phContact& contact = impact.GetContact();
          rage::Mat33V constraintAxis;
          rage::Vec3V worldNorm = contact.GetWorldNormal();
          worldNorm = NormalizeSafe(worldNorm, rage::Vec3V(rage::V_X_AXIS_WZERO));
          MakeOrthonormals(worldNorm, constraintAxis.GetCol1Ref(), constraintAxis.GetCol2Ref());
          constraintAxis.SetCol0(worldNorm);
          rage::Vec3V totalConstraintImpulse(contact.GetPreviousSolution()); 
          worldImpulse = Multiply(constraintAxis, totalConstraintImpulse);
          worldImpulse = Negate(worldImpulse);
    		}
#endif
      bool aIsNotAFoot = true;
      bool bIsNotAFoot = true;
#if NM_UPSIDEDOWN_FEET
      bool aIsUpsideDownFoot = false;
      bool bIsUpsideDownFoot = false;
#endif
      bool balancing = false;
      bool staggering = false;
      if (a->GetNMAgentID() != -1)
      {
        NmRsCharacter *character = m_rsEngine->getCharacterFromAgentID(a->GetNMAgentID());//mmmmtodo mmmmnoART this can be simplified now?
        NmRsArticulatedWrapper *wrap = character->getArticulatedWrapper();
        if (wrap && isBiped())//NB: There should be no NM agents that are not characters or bipeds
        {
          int leftFootIndex = character->getLeftLegSetup()->getFoot()->getPartIndex();
          int rightFootIndex = character->getRightLegSetup()->getFoot()->getPartIndex();
          gpA = character->getGenericPartByIndex(impact.GetMyComponent());
          if (gpA->m_partIndex == leftFootIndex || gpA->m_partIndex == rightFootIndex)
            aIsNotAFoot = false;

          if (!impact.IsConstraint())
          {
#if NM_UPSIDEDOWN_FEET
            aIsUpsideDownFoot = (gpA->m_partIndex == leftFootIndex && m_leftFootUpsideDown) || (gpA->m_partIndex == rightFootIndex && m_rightFootUpsideDown);
#endif
            NmRsCBUDynamicBalancer* dynamicBalancerTask =(NmRsCBUDynamicBalancer*)m_cbuTaskManager->getTaskByID(getID(), bvid_dynamicBalancer);
            if (((dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK && dynamicBalancerTask->isActive()) 
#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
              || m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_stumble)
#endif
              ) && isBiped() && ((gpA->m_partIndex == leftFootIndex) || (gpA->m_partIndex == rightFootIndex)))
            {
              balancing = true;
              float depthFudge = m_depthFudge;
              if (m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_staggerFall))
              {
                staggering = true;
                depthFudge = m_depthFudgeStagger;
              }
              //Only for feet
              impact.SetDepth(impact.GetDepth() - depthFudge);
            }
          }
        }
        //else
        //  gpA = character->lookupPartForInstance(a);
      }
#if ART_ENABLE_BSPY
      // let bSpy have a poke at any RAGE-side dynamics objects for possible transmission
      else if (m_rsEngine->getNmRsSpy()->isClientConnected())
      {
        bSpyProcessInstanceOnContact(a, b->GetNMAgentID());
      }
#endif // ART_ENABLE_BSPY

      if (b->GetNMAgentID() != -1)
      {
        NmRsCharacter *character = m_rsEngine->getCharacterFromAgentID(b->GetNMAgentID());
        NmRsArticulatedWrapper *wrap = character->getArticulatedWrapper();
        if (wrap  && isBiped())//NB: There should be no NM agents that are not characters or bipeds
        {
          gpB = character->getGenericPartByIndex(impact.GetOtherComponent());
          int leftFootIndex = character->getLeftLegSetup()->getFoot()->getPartIndex();
          int rightFootIndex = character->getRightLegSetup()->getFoot()->getPartIndex();
          if (gpB->m_partIndex == leftFootIndex || gpB->m_partIndex == rightFootIndex)
            bIsNotAFoot = false;
          if (!impact.IsConstraint())
          {
#if NM_UPSIDEDOWN_FEET
            bIsUpsideDownFoot = (gpB->m_partIndex == leftFootIndex && m_leftFootUpsideDown) || (gpB->m_partIndex == rightFootIndex && m_rightFootUpsideDown);
#endif
            NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuTaskManager->getTaskByID(getID(), bvid_dynamicBalancer);
            if (((dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK && dynamicBalancerTask->isActive()) 
#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
              || m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_stumble)
#endif//#if ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V
              ) && isBiped() && ((gpA->m_partIndex == leftFootIndex) || (gpA->m_partIndex == rightFootIndex)))
            {
              balancing = true;
              float depthFudge = m_depthFudge;
              if (m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_staggerFall))
              {
                staggering = true;
                depthFudge = m_depthFudgeStagger;
              }
              //Only for feet
              impact.SetDepth(impact.GetDepth() - depthFudge);
            }
          }
        }
        //else
        //  gpB = character->lookupPartForInstance(b);
      }
#if ART_ENABLE_BSPY
      // let bSpy have a poke at any RAGE-side dynamics objects for possible transmission
      else if (m_rsEngine->getNmRsSpy()->isClientConnected())
      {
        bSpyProcessInstanceOnContact(b, a->GetNMAgentID());
      }
#endif // ART_ENABLE_BSPY

	  rage::Vec3V normA, normB;
	  impact.GetMyNormal(normA);
      impact.GetOtherNormal(normB);
	  normalA = RCC_VECTOR3(normA);
	  normalB = RCC_VECTOR3(normB);
      bool impactWithSelfIgnored = false;
      // update parts with new collision data
      if (gpA)
      {
        if (!impact.IsConstraint())
        {
          // Disable the collision if we wish.
          if (!gpA->isCollisionEnabled())
            impact.DisableImpact();

          // Modify the friction 
          //if ((gpA->m_fictionMultiplier<0.99f))
          float footFrictionMult = 1.0f;
          if (!aIsNotAFoot)// is a foot
          {
            if (balancing)
              footFrictionMult = m_footFriction;
            if (staggering)
              footFrictionMult = m_footFrictionStagger;
#if NM_UPSIDEDOWN_FEET
            if (aIsUpsideDownFoot)
              footFrictionMult = 0.0f;
#endif
          }
          float min = 0.0f;
          float max = 10000.0f;
#define APPLY_MIN_FRICTION_TO_ALL_PARTS 1
#if APPLY_MIN_FRICTION_TO_ALL_PARTS
          if(m_applyMinMaxFriction)
          {
#if NM_UPSIDEDOWN_FEET
            min = (aIsUpsideDownFoot) ? 0.0f:m_minImpactFriction;
#else
            min = m_minImpactFriction;
#endif
            max = m_maxImpactFriction;
          }
#else
          if (m_applyMinMaxFriction && 
              !(gpA->m_partIndex == getLeftLegSetup()->getFoot()->getPartIndex() || gpA->m_partIndex == getRightLegSetup()->getFoot()->getPartIndex() ||
                gpA->m_partIndex == getLeftArmSetup()->getHand()->getPartIndex() || gpA->m_partIndex == getRightArmSetup()->getHand()->getPartIndex() ||
                gpA->m_partIndex == getSpineSetup()->getHeadPart()->getPartIndex()))
          {
#if NM_UPSIDEDOWN_FEET
            min = (aIsUpsideDownFoot) ? 0.0f:m_minImpactFriction;
#else
            min = m_minImpactFriction;
#endif
            max = m_maxImpactFriction;
          }
#endif
          //Modulate friction when glancing the side of a vehicle
          //Make glancing blows from sides of vehicles more spinny
          //If hit a vehicle with the foot ignore the friction scalings completely.
          if (m_glancingVehicleHit && m_gsScale1Foot && (!aIsNotAFoot) && b->GetClassType() == m_collision_vehicleClass)
            setImpactFriction(impact, m_gsFricScale1, min, max);
          else
            setImpactFriction(impact, gpA->m_fictionMultiplier*footFrictionMult, min, max);
          scaleElasticity(impact, gpA->m_elasticityMultiplier);

          bool registerObjectCollision = true;
          if (m_dontRegisterCollsionActive && aIsNotAFoot)
          {
            rage::Vector3 objectSize = VEC3V_TO_VECTOR3(b->GetArchetype()->GetBound()->GetBoundingBoxSize());
            float vol = objectSize.x * objectSize.y * objectSize.z; 
            registerObjectCollision = 
              PHLEVEL->IsFixed(b->GetLevelIndex()) ||
              ((b->GetArchetype()->GetMass() >= m_dontRegisterCollsionMassBelow) &&
              (vol >= m_dontRegisterCollsionVolBelow));
          }

          //// ignore collisions with mp3 shell casings and broken glass.
          //u32 flags = b->GetArchetype()->GetTypeFlags();
          //if(flags & (BIT(10) | BIT(17)))
          //  registerObjectCollision = false;

          if (gpB || registerObjectCollision)
          {
            //Check whether gpB and gpA are from the same character before looking to disable self collision
            //impactWithSelfIgnored should perhaps be handled i.e. the character should know that disabledSelf collisions have "collided" mmmmmtodo
			  bool justSoftenImpact = false;
            if(gpB && a->GetNMAgentID() != -1 && (a->GetNMAgentID() == b->GetNMAgentID()) && (disableLegCollisions(gpA, gpB, impact.GetDepth(), justSoftenImpact) || disableHandCollisions(gpA, gpB)))
            {
				if (justSoftenImpact)
				{
					impact.SetFriction(0.0f);
				}
				else
            {
              impact.DisableImpact();
#if ART_ENABLE_BSPY
              impactWithSelfIgnored = true;
#endif// ART_ENABLE_BSPY
            }
            }
            if (!impactWithSelfIgnored)
            {
				rage::Vector3 myPos = VEC3V_TO_VECTOR3(impact.GetMyPosition());
              gpA->handleCollision(myPos, normalA, impact.GetDepth(), gpB, b);
#if NM_EA_TEST_FROM_IMPACTS
              Patch_Add(b, impact.GetOtherComponent(), false, VEC3V_TO_VECTOR3(impact.GetMyPosition()), normalB);          
#endif//#if NM_EA_TEST_FROM_IMPACTS
            }
          }
#if NM_OBJECTS_IN_COLLISIONS
          else
          {
            gpA->handleCollision(impact.GetMyPosition(), normalA, impact.GetDepth(), gpB, b);
            if (
              (gpB->m_partIndex == getLeftArmSetup()->getHand()->getPartIndex()) ||
              (gpB->m_partIndex == getRightArmSetup()->getHand()->getPartIndex()))
            {
              b->GetArchetype()->GetBound()->GetBoundingBoxSize(m_objectSize);
              m_objectMass = b->GetArchetype()->GetMass();
              m_objectFixed = PHLEVEL->IsFixed(b->GetLevelIndex());
            }
          }
#endif
        }
      }

      if (gpB)
      {
        if (!impact.IsConstraint())
        {
          if (!gpB->isCollisionEnabled())
            impact.DisableImpact();

          if(a->GetLevelIndex() == m_rightHandWeapon.levelIndex)
          {
            if (isPartInMask(bvmask_HighSpine | bvmask_ArmLeft | bvmask_ArmRight, gpB->getPartIndex()))
            {
              impact.DisableImpact();
              //Unused m_rightHandWeapon.isColliding = true;
            }
          }
          else if(a->GetLevelIndex() == m_leftHandWeapon.levelIndex)
          {
            if (isPartInMask(bvmask_HighSpine | bvmask_ArmLeft | bvmask_ArmRight, gpB->getPartIndex()))
            {
              impact.DisableImpact();
              //Unused m_leftHandWeapon.isColliding = true;
            }
          }

          // Modify the friction 
          //if ((gpB->m_fictionMultiplier<0.99f))
          float footFrictionMult = 1.0f;
          if (!bIsNotAFoot)// is a foot
          {
            if (balancing)
              footFrictionMult = m_footFriction;
            if (staggering)
              footFrictionMult = m_footFrictionStagger;
#if NM_UPSIDEDOWN_FEET
            if (bIsUpsideDownFoot)
              footFrictionMult = 0.0f;
#endif
          }
          float min = 0.0f;
          float max = 10000.0f;
#if APPLY_MIN_FRICTION_TO_ALL_PARTS
          if(m_applyMinMaxFriction)
          {
#if NM_UPSIDEDOWN_FEET
            min = (bIsUpsideDownFoot) ? 0.0f:m_minImpactFriction;
#else
            min = m_minImpactFriction;
#endif
            max = m_maxImpactFriction;
          }
#else
          if (m_applyMinMaxFriction && 
              !(gpB->m_partIndex == getLeftLegSetup()->getFoot()->getPartIndex() || gpB->m_partIndex == getRightLegSetup()->getFoot()->getPartIndex() ||
                gpB->m_partIndex == getLeftArmSetup()->getHand()->getPartIndex() || gpB->m_partIndex == getRightArmSetup()->getHand()->getPartIndex() ||
                gpB->m_partIndex == getSpineSetup()->getHeadPart()->getPartIndex()))
          {
#if NM_UPSIDEDOWN_FEET
            min = (bIsUpsideDownFoot) ? 0.0f:m_minImpactFriction;
#else
            min = m_minImpactFriction;
#endif
            max = m_maxImpactFriction;
          }
#endif
          //Modulate friction when glancing the side of a vehicle
          //Make glancing blows from sides of vehicles more spinny
          //If hit a vehicle with the foot ignore the friction scalings completely.
          if (m_glancingVehicleHit && m_gsScale1Foot && (!bIsNotAFoot) && a->GetClassType() == m_collision_vehicleClass)
            setImpactFriction(impact, m_gsFricScale1, min, max);
          else
            setImpactFriction(impact, gpB->m_fictionMultiplier*footFrictionMult, min, max);
          scaleElasticity(impact, gpB->m_elasticityMultiplier);

          //Do I need to ignore here as well?  We don't look at gpB above for the feet fudge
          //Don't register/handleCollision of Gun/Small object collision with NMCharacter
          bool registerObjectCollision = true;
          if (m_dontRegisterCollsionActive && bIsNotAFoot)
          {
            rage::Vector3 objectSize = VEC3V_TO_VECTOR3(a->GetArchetype()->GetBound()->GetBoundingBoxSize());
            float vol = objectSize.x * objectSize.y * objectSize.z; 
            registerObjectCollision = PHLEVEL->IsFixed(a->GetLevelIndex()) || 
              ((a->GetArchetype()->GetMass() >= m_dontRegisterCollsionMassBelow)
              && (vol >= m_dontRegisterCollsionVolBelow));
          }

          //// ignore collisions with mp3 shell casings.
          //u32 flags = a->GetArchetype()->GetTypeFlags();
          //if(flags & (BIT(10) | BIT(17)))
          //  registerObjectCollision = false;

          if (gpA || registerObjectCollision)
          {
            //Check whether gpB and gpA are from the same character before looking to disable self collision
            //impactWithSelfIgnored should perhaps be handled i.e. the character should know that disabledSelf collisions have "collided" mmmmmtodo
			  bool justSoftenImpact = false;
            if(gpA && a->GetNMAgentID() != -1 && (a->GetNMAgentID() == b->GetNMAgentID()) && (disableLegCollisions(gpA, gpB, impact.GetDepth(), justSoftenImpact) || disableHandCollisions(gpA, gpB)))
            {
				if (justSoftenImpact)
				{
					impact.SetFriction(0.0f);
				}
				else
            {
              impact.DisableImpact();
#if ART_ENABLE_BSPY
              impactWithSelfIgnored = true;
#endif// ART_ENABLE_BSPY
            }
            }
            if (!impactWithSelfIgnored)
            {
				rage::Vector3 otherPos = VEC3V_TO_VECTOR3(impact.GetOtherPosition());
              gpB->handleCollision(otherPos, normalB, impact.GetDepth(), gpA, a);
#if NM_EA_TEST_FROM_IMPACTS
              Patch_Add(a, impact.GetMyComponent(), false, VEC3V_TO_VECTOR3(impact.GetMyPosition()), normalB);
#endif//#if NM_EA_TEST_FROM_IMPACTS
            }
          }

#if NM_OBJECTS_IN_COLLISIONS
          else
          {
            if (
              (gpB->m_partIndex == getLeftArmSetup()->getHand()->getPartIndex()) ||
              (gpB->m_partIndex == getRightArmSetup()->getHand()->getPartIndex()))
            {
              gpB->handleCollision(impact.GetOtherPosition(), normalB, impact.GetDepth(), gpA, a);
              a->GetArchetype()->GetBound()->GetBoundingBoxSize(m_objectSize);
              m_objectMass = a->GetArchetype()->GetMass();
              m_objectFixed = PHLEVEL->IsFixed(a->GetLevelIndex());
            }
          }
#endif
        }
      }

#if ART_ENABLE_BSPY
      //impactWithSelfIgnored included here as we want to see the selfCollisions in bSpy at the moment.but only 
      //  when the the self collisions are trying to be turned on
      //  These ignored self impacts should be coloured differently
      impactWithSelfIgnored = impactWithSelfIgnored && (m_Leg2LegCollisionExclusion.b != m_Leg2LegCollisionExclusion.bTarget);
      if ((getBSpyID() != INVALID_AGENTID) && ((!impact.IsDisabled()) || impactWithSelfIgnored) &&
        m_rsEngine->getNmRsSpy()->isClientConnected() &&
        m_rsEngine->getNmRsSpy()->shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_CollisionContactData) && 
        (gpA || gpB))
      {
        ContactPointPacket cp((bs_int32)getBSpyID(), (bs_int32)b->GetNMAgentID());

        //Attempt to get rage Constraint force not the constraint violation - unfortunately Impetus returns zero. 
        //if (impact.IsConstraint())
        //{
        //  if (gpA)
        //  {
        //    impact.GetMyImpetus(normalA);
        //    cp.d.m_depth = normalA.Mag();
        //  }
        //  else
        //  {
        //    impact.GetOtherImpetus(normalB);
        //    cp.d.m_depth = normalB.Mag();
        //  }
        //}
        //else
        cp.d.m_depth = impact.GetDepth();
        cp.d.m_isConstraint = impact.IsConstraint();

        cp.d.m_friction = impact.GetFriction();
        cp.d.m_elasticity = impact.GetElasticity();
        cp.d.m_isForce = impact.IsForce();
        cp.d.m_isDisabled = impact.IsDisabled();
        cp.d.m_agentID2IsLevelIndex = false;

        // HDD: no idea what to set force to
        cp.d.m_force.v[0] = cp.d.m_force.v[1] = cp.d.m_force.v[2] = 0;


        if (gpA)
        {
          cp.d.m_pos        = bSpyVec3fromVector3(VEC3V_TO_VECTOR3(impact.GetMyPosition()));
          cp.d.m_posOther   = bSpyVec3fromVector3(VEC3V_TO_VECTOR3(impact.GetOtherPosition()));
          cp.d.m_norm       = bSpyVec3fromVector3(normalA);

          cp.d.m_part1 = (bs_int8)gpA->getPartIndex();
          cp.d.m_part2 = (bs_int8)(gpB?gpB->getPartIndex():impact.GetOtherComponent());

          if (!gpB)
          {
            cp.d.m_agentID2IsLevelIndex = true;
            cp.d.m_agentID2 = (bs_int32)b->GetLevelIndex();
          }
        }
        else
        {
          cp.d.m_pos        = bSpyVec3fromVector3(VEC3V_TO_VECTOR3(impact.GetOtherPosition()));
          cp.d.m_posOther   = bSpyVec3fromVector3(VEC3V_TO_VECTOR3(impact.GetMyPosition()));
          cp.d.m_norm       = bSpyVec3fromVector3(normalB);

          cp.d.m_part1 = (bs_int8)gpB->getPartIndex();
          cp.d.m_part2 = (bs_int8)(gpA?gpA->getPartIndex():-1);

          bs_int32 tmp = cp.d.m_agentID2;//this is set in the constructor of cp
          cp.d.m_agentID2 = cp.d.m_agentID;
          cp.d.m_agentID = tmp;
        }

        bspySendPacket(cp);
      }
#endif // ART_ENABLE_BSPY
    }
  }

  bool NmRsCharacter::disableHandCollisions(NmRsGenericPart* gpA, NmRsGenericPart* gpB)
  {
    int indexA = gpA->getPartIndex();
    int indexB = gpB->getPartIndex();
    if(m_rightHandCollisionExclusion.b)
    {
      // gpA is right hand, gpB is in exclusion mask.
      if(isPartInMask(m_rightHandCollisionExclusion.a, indexA) &&
        isPartInMask(m_rightHandCollisionExclusion.b, indexB))
      {
        m_rightHandCollisionExclusion.colliding |= partToMask(indexB);
        return true;
      }
      // gpB is right hand, gpA is in exclusion mask.
      if(isPartInMask(m_rightHandCollisionExclusion.a, indexB) &&
        isPartInMask(m_rightHandCollisionExclusion.b, indexA))
      {
        m_rightHandCollisionExclusion.colliding |= partToMask(indexA);
        return true;
      }
    }
    return false;
  }

  bool NmRsCharacter::disableLegCollisions(NmRsGenericPart* gpA, NmRsGenericPart* gpB, float impactDepth, bool &justSoftenImpact)
  {
    //Impacts of depth > impactDepthCalledCollision will be registered with m_Leg2LegCollisionExclusion
    //When trying to turn on a collision between say the 2 feet if we ignore -ve depth collisions, collisions will turn on quicker
    //  if this causes pops then make impactDepthCalledCollision more negative.
    //The safest no pop solution is to have impactDepthCalledCollision = -FLT_MAX
    //If parts are colliding and in the masks we still disable the impact whatever the depth.

	  justSoftenImpact = false;
    const float impactDepthCalledCollision = 0.0f;
    if(m_Leg2LegCollisionExclusion.b)
    {
      int indexA = gpA->getPartIndex();
      int indexB = gpB->getPartIndex();
      // gpA is left leg, gpB is in exclusion mask.
      if(isPartInMask(m_Leg2LegCollisionExclusion.a, indexA) &&
        isPartInMask(m_Leg2LegCollisionExclusion.b, indexB))
      {
        if (impactDepth >= impactDepthCalledCollision)
          m_Leg2LegCollisionExclusion.colliding |= partToMask(indexB);

		// Don't disable the impact is feet aren't involved 
		justSoftenImpact = !getEngine()->GetAllowLegInterpenetration() && !(indexA == 3 || indexA == 6 || indexB == 3 || indexB == 6) && impactDepth >= impactDepthCalledCollision;

        return true;
      }
      // gpB is left leg, gpA is in exclusion mask.
      if(isPartInMask(m_Leg2LegCollisionExclusion.a, indexB) &&
        isPartInMask(m_Leg2LegCollisionExclusion.b, indexA))
      {
        if (impactDepth >= impactDepthCalledCollision)
          m_Leg2LegCollisionExclusion.colliding |= partToMask(indexA);

		// Don't disable the impact is feet aren't involved 
		justSoftenImpact = !getEngine()->GetAllowLegInterpenetration() && !(indexA == 3 || indexA == 6 || indexB == 3 || indexB == 6) && impactDepth >= impactDepthCalledCollision;

        return true;
      }
    }
    return false;
  }

  void NmRsCharacter::updateAttachedObject(AttachedObject* object)
  {
    Assert(object);
    if(object->levelIndex != -1)
    {
      rage::phInst* pInst = NULL;
      rage::phLevelNew* level = getLevel();
      Assert(level);

      int levelIndex = object->levelIndex;
      if(getIsInLevel(levelIndex))
      {
        pInst = level->GetInstance(levelIndex);
        if(pInst)
        {
          object->worldCOMPos = RCC_VECTOR3(pInst->GetPosition());
          //Assert if the attached object is far away from it's part - this will generally mean that the weapon should have been un-registered
          //by calling register weapon with start = false
          //  at the moment can only be a weapon as the attachedObject levelIndex = -1
          //  partIndex of a weapon is always set to be a hand if levelIndex exists
          //  getGenericPartByIndex returns the pelvis if index is out of range
#if __ASSERT
          if (object->worldCOMPos.Dist(getGenericPartByIndex(object->partIndex)->getPosition()) >= 1.0f)
		  {
			  Warningf("NmRsCharacter::updateAttachedObject() - object->worldCOMPos.Dist(getGenericPartByIndex(object->partIndex)->getPosition()) >= 1.0f");
		  }
#endif
          //object->worldCOMPos = RCC_VECTOR3(pInst->GetCenterOfMass());//mmmtodo want comPos but: gives  warning 1322: taking the address of a temporary
          object->mass = pInst->GetArchetype()->GetMass();
        }
        else
        {
          //initialize attached object/gun to be ignored if inst doesn't exist
          object->worldCOMPos.Zero();
          object->mass = 0.f;
          //Unused object->isColliding =false;
          object->levelIndex = -1;
          object->massMultiplier =1.0f;
          object->partIndex = -1;
        }
      }
      else
      {
        //initialize attached object/gun to be ignored if level index no longer in level
        object->worldCOMPos.Zero();
        object->mass = 0.f;
        //Unused object->isColliding =false;
        object->levelIndex = -1;
        object->massMultiplier =1.0f;
        object->partIndex = -1;
      }
    }
  }

  void NmRsCharacter::initMaxAngSpeed(float multiplier)
  {
    Assert(multiplier > 0 && multiplier < 10); // test for valid range
    if (getArticulatedWrapper())
    {
      rage::phArticulatedCollider *collider = getArticulatedWrapper()->getArticulatedCollider();
      if (collider)
        collider->SetMaxAngSpeed(6.0f * PI * multiplier);
    }
  }

  CBUTaskBase *NmRsCharacter::getTask(int bvid)
  { 
    return m_cbuTaskManager->getTaskByID(m_agentID, bvid); 
  }

  void NmRsCharacter::setFloorVelocityFromColliderRefFrameVel()
  {
	  m_floorVelocity = VEC3V_TO_VECTOR3(getArticulatedWrapper()->getArticulatedCollider()->GetReferenceFrameVelocity());
	  if (!m_movingFloor)
		  m_floorVelocity.Zero();
	  m_COMvelRelative = m_COMvel - m_floorVelocity;
	  m_COMvelRelativeMag = m_COMvelRelative.Mag();
  }

  void NmRsCharacter::deactivateTask(int bvid)
  {
    CBUTaskBase* task = m_cbuTaskManager->getTaskByID(m_agentID, bvid); 
    Assert(task);
    if (task->isActive())
      task->deactivate();
  }

  void NmRsCharacter::measureCharacter(rage::Matrix34 &leftFootMat, rage::Matrix34 &rightFootMat,
    rage::Matrix34 &pelvisMat, rage::Matrix34 &headMat, rage::Matrix34 &UNUSED_PARAM(thighLeftMat), rage::Matrix34 &UNUSED_PARAM(thighRightMat),
    float *legSeparation, float *legStraightness, float *charlieChapliness, float *hipYaw,
    float *headYaw, float *defaultHipPitch)
  {
    // Foot separation calculated here from the zero pose foot positions
    rage::Vector3 toRight = rightFootMat.d - leftFootMat.d;
    // level the vector
    toRight -= getUpVector() * (getUpVector().Dot(toRight));

    // HDD; i've scaled this down for now as the balancer seems to take
    // unnecessarily large steps otherwise (I think we tuned it against the
    // Lua version that narrowed leg separation because of a buildMatrix() bug...?)
    *legSeparation = toRight.Mag() * 0.75f;
    //Clamp the legSeparation to what the balancer can handle
    //  if zero pose gives wild values 
    //*legSeparation = rage::Clamp(*legSeparation, 0.3f,0.5f);

    // hipHeight is the height of the zero pose
    rage::Vector3 feetToRoot = pelvisMat.d - (leftFootMat.d + rightFootMat.d)*0.5f;
    float hipHeight = feetToRoot.Dot(getUpVector());

    // height is actual hip height of the character
    rage::Vector3 left = getLeftLegSetup()->getFoot()->getInitialMatrix().d;
    rage::Vector3 right = getRightLegSetup()->getFoot()->getInitialMatrix().d;
    rage::Vector3 pelvis = getSpineSetup()->getPelvisPart()->getInitialMatrix().d;

    float height = (pelvis - (left+right)*0.5f).Dot(getUpVector());

    Assert(rage::Abs(height) > 1e-10f);
    float straightness = hipHeight / height+0.1f; // height should never be near 0
    *legStraightness = straightness < 1.f ? straightness : 1.f;
    //Clamp the legStraightness to what the balancer can handle
    //  if zero pose gives wild values 
    //*legStraightness = rage::Clamp(*legStraightness, 0.85f,1.0f);

    rage::Vector3 cross;
    cross.Cross(rightFootMat.c, leftFootMat.c);

    float sinAngle = cross.Dot(getUpVector());
    float angle = rage::AsinfSafe(sinAngle);
    *charlieChapliness = angle*0.25f; // more accurate scale is * 0.3f, but we're being conservative

    rage::Vector3 footForward = rightFootMat.c + leftFootMat.c;
    footForward.Normalize();
    cross.Cross(footForward, pelvisMat.c);
    sinAngle = cross.Dot(getUpVector());
    angle = rage::AsinfSafe(sinAngle);
    *hipYaw = angle*0.5f; // more accurate scale is * 0.7f, but we're being conservative

    cross.Cross(footForward, headMat.c);
    sinAngle = cross.Dot(getUpVector());
    angle = rage::AsinfSafe(sinAngle);
    *headYaw = angle;

    // So not all default pose's have the back straight about the hips.
    // So lets measure the hipPitch and take account of this in the balancer. 
    // Specifically from/for bug in MP3 shot. KM 13/08/08
#if 0
    rage::Vector3 thighUp = (thighLeftMat.b + thighRightMat.b)*0.5f;
    thighUp.Normalize();
    rage::Vector3 thighForward = -(thighLeftMat.c + thighRightMat.c)*0.5f;
    thighForward.Normalize();
    rage::Vector3 thighSide = -(thighLeftMat.a + thighRightMat.a)*0.5f;
    thighSide.Normalize();
    rage::Vector3 pelUp = pelvisMat.a;

    // Take away to thighForward component of the pelUp. Project it onto the up-forward plane. 
    float pelUpDotThighForward = pelUp.Dot(thighForward);
    thighForward.Scale(pelUpDotThighForward);
    pelUp = pelUp - thighForward;

    float pelUpDotSide = pelUp.Dot(thighSide);
    pelUpDotSide = pelUpDotSide/rage::Abs(pelUpDotSide);

    // work out the angle between pelUp and thighUp
    *defaultHipPitch = 0.5f*pelUpDotSide*rage::AcosfSafe(pelUp.Dot(thighUp));
    bspyScratchpad(getBSpyID(), "measureC", *defaultHipPitch);
#endif

    //mmmm untested for North
    // determined empirically, should lead to stable balancer 
    // clamped, as hipPitch of 0 is just too straight, and beyond -0.5 he can't step forwards well
    //*defaultHipPitch = -10.0f * (1.0f - *legStraightness);
    //*defaultHipPitch = rage::Clamp(*defaultHipPitch, -0.5f, -0.1f);

    //GTA had no default hip pitch
    //We could use the above scheme without the always bend forward clamp
    //A scheme that pitches forward enough to be balanced
    //Just takes the incoming animations hip pitch
    *defaultHipPitch = 0.f;
  }

  void NmRsCharacter::configureCharacter(bool useZeroPose, bool UNUSED_PARAM(leftHandFree), bool UNUSED_PARAM(rightHandFree), float stanceBias, float COMBias)
  {
    // check to see if zero pose needs to be stored
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    if (getZeroPose && useZeroPose)
#else
    if (useZeroPose)
#endif
    {
      int incomingComponentCount = 0;
      rage::Matrix34 *itPtr = 0;
      IncomingTransformStatus itmStatusFlags = kITSNone;
      getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, kITSourceCurrent);

      if (incomingComponentCount != 0 && itPtr)
      {
#if NM_PRE_INTEGRATION
        //This matches configureTheCharacter (and it's intended use by the game) better which will replace this routine
        //i.e. although arm zero poses aren't set here the default ones are pretty close to what the game would set
        //Don't set arm zero poses because this routine is called using a pointGun pose which makes blendToZeroPose 
        //  lift the arms up as the character comes to a rest. 
        BehaviourMask mask = (bvmask_Full & ~bvmask_Arms);
        callMaskedEffectorFunctionNoArgs(this, mask, &NmRs1DofEffector::storeZeroPose, &NmRs3DofEffector::storeZeroPose);
#else
        storeZeroPoseAllEffectors();
#endif
        // Below automatically calculates legSeparation and legStraightness from the current incoming transforms.
        // Will move into a separate function when it grows bigger.
        int leftFootIndex = getLeftLegSetup()->getFoot()->getPartIndex();
        int rightFootIndex = getRightLegSetup()->getFoot()->getPartIndex();
        int headIndex = getSpineSetup()->getHeadPart()->getPartIndex();
        int pelvisIndex = getSpineSetup()->getPelvisPart()->getPartIndex();
        int leftThighIndex = getLeftLegSetup()->getThigh()->getPartIndex();
        int rightThighIndex = getRightLegSetup()->getThigh()->getPartIndex();

#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
        if (m_allowMeasureCharacter)
#endif

        measureCharacter(itPtr[leftFootIndex], itPtr[rightFootIndex], itPtr[pelvisIndex], itPtr[headIndex], itPtr[leftThighIndex], itPtr[rightThighIndex],
        &m_characterConfig.m_legSeparation, &m_characterConfig.m_legStraightness,
        &m_characterConfig.m_charlieChapliness, &m_characterConfig.m_hipYaw,
        &m_characterConfig.m_headYaw, &m_characterConfig.m_defaultHipPitch);

        //mmmmtodo legSeparation may have to be clamped to a sensible range
        //mmmmtodo m_legStraightness should be <= 1.0
        m_characterConfig.m_legSeparation += stanceBias;
        m_characterConfig.m_legStraightness += COMBias;

#if ART_ENABLE_BSPY
        if (getBSpyID() != INVALID_AGENTID)
        {
          rage::Matrix34 mat;
          for(int i=0;i<TotalKnownParts;i++)
          {
            getITMForPart(i, &mat);
            bspyDrawCoordinateFrame(0.1f, mat);
          }
        }
#endif
      }// if (incomingComponentCount != 0 && itPtr)
#if NM_RS_ENABLE_LOGGING
      else
      {
        NM_RS_LOGERROR(L"NmRsCharacter::configureCharacter - no animation set for zero pose");
      }
#endif // NM_RS_ENABLE_LOGGING        
    }// if (useZeroPose)
  }

  void NmRsCharacter::configureTheCharacter(bool setZeroPose, bool setZeroPoseArms, bool configureBalancer, float stanceBias, float COMBias)
  {
        int incomingComponentCount = 0;
        rage::Matrix34 *itPtr = 0;
        IncomingTransformStatus itmStatusFlags = kITSNone;
        getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, kITSourceCurrent);

    if (incomingComponentCount != 0 && itPtr)
    {
      // check to see if zero pose needs to be stored
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
      if ((setZeroPose || setZeroPoseArms) && getZeroPose)
#else
      if (setZeroPose || setZeroPoseArms)
#endif
      {
        BehaviourMask mask = bvmask_None;
        if (setZeroPose)
          mask |= (bvmask_Full & ~bvmask_Arms);
        if (setZeroPoseArms)
          mask |= bvmask_Arms;
        callMaskedEffectorFunctionNoArgs(this, mask, &NmRs1DofEffector::storeZeroPose, &NmRs3DofEffector::storeZeroPose);
      }

      if (configureBalancer)
      {
        // Below automatically calculates legSeparation and legStraightness from the current incoming transforms.
        // Will move into a separate function when it grows bigger.
        int leftFootIndex = getLeftLegSetup()->getFoot()->getPartIndex();
        int rightFootIndex = getRightLegSetup()->getFoot()->getPartIndex();
        int headIndex = getSpineSetup()->getHeadPart()->getPartIndex();
        int pelvisIndex = getSpineSetup()->getPelvisPart()->getPartIndex();
        int leftThighIndex = getLeftLegSetup()->getThigh()->getPartIndex();
        int rightThighIndex = getRightLegSetup()->getThigh()->getPartIndex();

#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
        if (m_allowMeasureCharacter)
#endif

        measureCharacter(itPtr[leftFootIndex], itPtr[rightFootIndex], itPtr[pelvisIndex], itPtr[headIndex], itPtr[leftThighIndex], itPtr[rightThighIndex],
        &m_characterConfig.m_legSeparation, &m_characterConfig.m_legStraightness,
        &m_characterConfig.m_charlieChapliness, &m_characterConfig.m_hipYaw,
        &m_characterConfig.m_headYaw, &m_characterConfig.m_defaultHipPitch);

        //mmmmtodo legSeparation may have to be clamped to a sensible range
        //mmmmtodo m_legStraightness should be <= 1.0
      }
      //although this is strictly measuring the character allow it regardless of configureBalancer
      //as it is assumed the game will expect it to be applied
        m_characterConfig.m_legSeparation += stanceBias;
        m_characterConfig.m_legStraightness += COMBias;


#if ART_ENABLE_BSPY
        if (getBSpyID() != INVALID_AGENTID)
        {
          rage::Matrix34 mat;
          for(int i=0;i<TotalKnownParts;i++)
          {
            getITMForPart(i, &mat);
            bspyDrawCoordinateFrame(0.1f, mat);
          }
        }
#endif
      }// if (incomingComponentCount != 0 && itPtr)
#if NM_RS_ENABLE_LOGGING
      else
      {
        NM_RS_LOGERROR(L"NmRsCharacter::configureCharacter - no animation set for zero pose");
      }
#endif // NM_RS_ENABLE_LOGGING        
  }
  void NmRsCharacter::unregisterWeapon(int hand) 
  {
    m_registerWeaponCalled = false;
    AttachedObject* handWeapon = &m_rightHandWeapon;
    if(hand == kLeftHand)
      handWeapon = &m_leftHandWeapon;

    //Initialize
    handWeapon->partIndex = -1;
    handWeapon->levelIndex = -1;
    handWeapon->mass = 0.0f;
    handWeapon->massMultiplier = 1.0f;
    handWeapon->worldCOMPos.Zero();
    //Unused handWeapon->isColliding = false;
  }

  /*
   * Register attached weapon for collision exception, com calculation and
   * gravity opposition. hand = 0 : left hand, hand = 1 : right hand
   *   Information is also used by pointGun
   *   Sets:
   *	  m_*HandWeapon.levelIndex
   *	  m_*HandWeapon.partIndex
   *	  m_*HandWeapon.worldCOMPos - recalculated later by updateAttachedObject((AttachedObject*)&m_*HandWeapon);
   *	  m_*HandWeapon.mass
   *	  m_gunToHandCurrent[hand]
   *
   *	  m_gunToHandAiming[hand]
   */	
  void NmRsCharacter::registerWeapon(
    int hand, 
    int levelIndex, 
	  rage::phConstraintHandle *UNUSED_PARAM(gunToHandConstraint),
    const rage::Matrix34& gunToHand, 
	  const rage::Vector3& UNUSED_PARAM(gunToMuzzleInGun), 
	  const rage::Vector3& UNUSED_PARAM(gunToButtInGun))
  {
    m_registerWeaponCalled = true;
    NmRsGenericPart* handPart = getRightArmSetup()->getHand();
    AttachedObject* handWeapon = &m_rightHandWeapon;
    if(hand == kLeftHand)
    {
      handWeapon = &m_leftHandWeapon;
      handPart = getLeftArmSetup()->getHand();
    }

    rage::phInst* pInst = NULL;
    rage::phLevelNew* level = getLevel();
    Assert(level);
    //Initialize
    handWeapon->partIndex = -1;
    handWeapon->levelIndex = -1;
    handWeapon->mass = 0.0f;
    handWeapon->massMultiplier = 1.0f;
    handWeapon->worldCOMPos.Zero();
    //Unused handWeapon->isColliding = false;

    m_gunToHandAiming[hand] = gunToHand;

	  // Store the handle in m_gunToHandConstraintHandle
	  //m_gunToHandConstraintHandle[hand].index = constrHandle->index;
	  //m_gunToHandConstraintHandle[hand].generation = constrHandle->generation;

    rage::Matrix34 gunToWorldAiming;
    rage::Matrix34 handToWorld;
    handPart->getMatrix(handToWorld);
    rage::Matrix34 worldToHand;
    handPart->getMatrix(worldToHand);//handToWorld
    worldToHand.Inverse();
    gunToWorldAiming.Dot(m_gunToHandAiming[hand], handToWorld);

    //If the gun doesn't physically exist then use m_gunToHandAiming as m_gunToHandCurrent
    //This means we can test the behaviour without a physical gun
    //If the gun ever disappears this could also be a choice for m_gunToHandCurrent instead of using the last known m_gunToHandCurrent
    m_gunToHandCurrent[hand] = m_gunToHandAiming[hand];
    if(levelIndex != -1 && getIsInLevel(levelIndex))//mmmmCHeck don't think getIsInLevel is working
    {
      pInst = level->GetInstance(levelIndex);
      if(pInst)
      {
#if !NM_TESTING_NEW_REGISTERWEAPON_MESSAGE
        handWeapon->levelIndex = levelIndex;
        //else we go with the levelIndex sent in by the old message
#endif
        handWeapon->worldCOMPos = VEC3V_TO_VECTOR3(pInst->GetCenterOfMass());
        handWeapon->mass = pInst->GetArchetype()->GetMass();
        handWeapon->partIndex = handPart->getPartIndex();
        //set the weaponTransform i.e. orientation of gun to hand
        rage::Matrix34 gunToWorld = RCC_MATRIX34(pInst->GetMatrix());
        m_gunToHandCurrent[hand].Dot(gunToWorld, worldToHand);         


#if 0
        //gunToHand constraint.  
        if (gunToHandConstraint)
          //Set the gunToHand Constraint if it has been sent in by game
          m_gunToHandConstraint[hand] = *gunToHandConstraint;
        else
        {
          //Create the gunToHand Constraint if it has not been sent in by game
          //Remove the gunToHand constraint currently made by the game:
          rage::phConstraintMgr *mgr = getSimulator()->GetConstraintMgr();
          mgr->RemoveActiveConstraints(pInst);
          //Make a constraint (m_gunToHandConstraint[hand])between the gun and the hand using m_gunToHandCurrent[hand]
          //TODO by Brian
          //Code to create the constraint
          //m_gunToHandConstraint[hand] = NULL;

        }

#endif
      }
    }

  }
  /*
   * Register attached weapon for collision exception, com calculation and
   * gravity opposition. hand = 0 : left hand, hand = 1 : right hand
   *   Information is also used by pointGun
   *   Sets:
   *	  m_*HandWeapon.levelIndex
   *	  m_*HandWeapon.partIndex
   *	  m_*HandWeapon.worldCOMPos - recalculated later by updateAttachedObject((AttachedObject*)&m_*HandWeapon);
   *	  m_*HandWeapon.mass
   *	  m_gunToHandCurrent[hand]
   */
  void NmRsCharacter::registerWeapon(int hand, int levelIndex, float /*extraLean1*/, float /*extraLean2*/)
  {

#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "registerWeapon", hand);
    bspyScratchpad(getBSpyID(), "registerWeapon", levelIndex);
#endif

    NmRsGenericPart* handPart = getRightArmSetup()->getHand();
    AttachedObject* handWeapon = &m_rightHandWeapon;
    if(hand == kLeftHand)
    {
      handWeapon = &m_leftHandWeapon;
      handPart = getLeftArmSetup()->getHand();
    }
    //Initialize
    handWeapon->partIndex = -1;
    handWeapon->levelIndex = -1;
    handWeapon->mass = 0.0f;
    handWeapon->massMultiplier = 1.0f;
    handWeapon->worldCOMPos.Zero();
    //Unused handWeapon->isColliding = false;

    rage::phInst* pInst = NULL;
    rage::phLevelNew* level = getLevel();
    Assert(level);

    if(levelIndex != -1)
    {
      pInst = level->GetInstance(levelIndex);
        if(pInst)
        {
        handWeapon->worldCOMPos = VEC3V_TO_VECTOR3(pInst->GetCenterOfMass());
        handWeapon->mass = pInst->GetArchetype()->GetMass();
        handWeapon->levelIndex = levelIndex;
      }
    }

    //set the weaponTransform i.e. orientation of gun to hand
    if(pInst)
    {
      rage::Matrix34 gunToWorld = RCC_MATRIX34(pInst->GetMatrix());
#if ART_ENABLE_BSPY
      getEngineNotConst()->setbSpyObject(levelIndex);
#endif
      rage::Matrix34 worldToHand;
      handPart->getMatrix(worldToHand);//handToWorld
      worldToHand.Inverse();
      m_gunToHandCurrent[hand].Dot(gunToWorld, worldToHand);
      //PointGun making the m_supportHandToGunHand using m_supportHandToGun will still work
      // with this old message because m_supportHandToGun is made from m_gunToHandAiming and the two hands itms if... 
      m_gunToHandAiming[hand] = m_gunToHandCurrent[hand];
    }

  }

#if NM_SET_WEAPON_BOUND
#if NM_SET_WEAPON_MASS
  void NmRsCharacter::setHandBound(int hand, rage::phBound* bound, rage::Matrix34* gunToWorld, float weaponMass /* = 0 */)
#else
  void NmRsCharacter::setHandBound(int hand, rage::phBound* bound, rage::Matrix34* gunToWorld, float /* weaponMass = 0 */)
#endif
  {
    Assert(bound);

#if ART_ENABLE_BSPY
    bspyLogf(info, L"Setting hand bound for %d", hand);
#endif

    NmRsGenericPart* handPart;
    if(hand == (NmRsHand)kLeftHand)
      handPart = getLeftArmSetup()->getHand();
    else
      handPart = getRightArmSetup()->getHand();

    rage::phBound* boundToSet = 0;
    rage::Matrix34 boundOffset; boundOffset.Identity();

    if (bound->GetType() == rage::phBound::COMPOSITE)
    {
      rage::phBoundComposite* phComp = (rage::phBoundComposite*)bound;
      int i, numBounds = phComp->GetNumBounds();
      for (i=0; i<numBounds; i++)
      {
        rage::phBound* sub = phComp->GetBound(i);
        if(sub->GetType() == rage::phBound::GEOMETRY ||
          sub->GetType() == rage::phBound::SPHERE ||
          sub->GetType() == rage::phBound::CAPSULE ||
          sub->GetType() == rage::phBound::BOX)
        {
          boundOffset = phComp->GetCurrentMatrix(i);
          boundToSet = sub;
          break;
        }
      }
    }
    else if(bound->GetType() == rage::phBound::GEOMETRY ||
      bound->GetType() == rage::phBound::SPHERE ||
      bound->GetType() == rage::phBound::CAPSULE ||
      bound->GetType() == rage::phBound::BOX)
    {
      boundToSet = bound;
    }

    if (boundToSet)
    {
#if ART_ENABLE_BSPY & 0

      NmRsSpy& spy = *m_rsEngine->getNmRsSpy();

      // kill the cached session ID to force re-send of mesh data
      ART::NmRsSpy::DynBoundTracker dbt;
      Assert(spy.m_dynBoundTxTable != 0);
      if (spy.m_dynBoundTxTable->find(boundToSet, &dbt))
      {
        dbt.sessionStamp = 0xFFFFFFFF;
        spy.m_dynBoundTxTable->replace(boundToSet, dbt);
      }

#endif // ART_ENABLE_BSPY
      getArticulatedWrapper()->setBound(handPart->getPartIndex(), boundToSet);
    }
#if ART_ENABLE_BSPY
    else
    {
      bspyLogf(info, L"Bound type is not geometry/sphere/capsule/box");
    }
#endif

    // compute weapon extra leans from gunToWorld, if provided
    if(gunToWorld)
    {
      rage::Matrix34 worldToHand;
      handPart->getMatrix(worldToHand);//handToWorld
      worldToHand.Inverse();
      m_gunToHandCurrent[hand].Dot(gunToWorld, worldToHand);
      //mmmmtodo set? m_gunToHandAiming[hand] = m_gunToHandCurrent[hand] 	
    }
    else
    {
      m_gunToHandCurrent[hand].Identity();
      //mmmmtodo set? m_gunToHandAiming[hand] = m_gunToHandCurrent[hand] 	
    }

#if NM_SET_WEAPON_MASS
    /*
     *  set mass and center of mass.
     */

    // get weapon center of mass from weapon transform.
    rage::Vector3 COMoffset(m_gunToHandCurrent[hand].d);

#define USE_DEFAULT_WEAPON_MASSES 1
#if USE_DEFAULT_WEAPON_MASSES
    // mass defaults until hooked up to game.
    if(hand == kLeftHand)
      if(m_weaponMode == kPistolLeft || m_weaponMode == kDual)
        weaponMass = 2.5f;
      else
        weaponMass = m_handMassCache[hand];
    else // right
      if(m_weaponMode == kPistolRight || m_weaponMode == kPistol || m_weaponMode == kDual)
        weaponMass = 5.f;
      else if(m_weaponMode == kRifle)
        weaponMass = 12.0f;
      else
        weaponMass = m_handMassCache[hand];
#endif

      // hand center of mass is always the part position.
      float handMass = m_handMassCache[hand];
      Assert(handMass >= 0.5f);

      // find the COM of the combined hand and weapon.
      COMoffset.Scale(weaponMass/(weaponMass+handMass));

      // set the mass and COM offset.
      setHandMass(hand, weaponMass+handMass, &COMoffset);
#endif

#else
  void NmRsCharacter::setHandBound(int /* hand */, rage::phBound* /* bound */)
  {
#endif // NM_SET_WEAPON_BOUND
  }

#if NM_SET_WEAPON_BOUND
  void NmRsCharacter::cacheHandBound(NmRsHand hand)
  {
    int index;
    if(hand == kLeftHand)
      index = getLeftArmSetup()->getHand()->getPartIndex();
    else
      index = getRightArmSetup()->getHand()->getPartIndex();
    rage::phBound* bound = getArticulatedWrapper()->getBound(index);
    bound->AddRef(); // keep from disappearing while not in use
    m_handBoundCache[(int)hand] = bound;

#if NM_SET_WEAPON_MASS
    cacheHandMass(hand);
#endif
  }

  void NmRsCharacter::restoreHandBound(NmRsHand hand)
  {
#if ART_ENABLE_BSPY
    bspyLogf(info, L"Restoring hand bound for %d", hand);
#endif
    int index;
    if(hand == kLeftHand)
      index = getLeftArmSetup()->getHand()->getPartIndex();
    else
      index = getRightArmSetup()->getHand()->getPartIndex();

    if(m_handBoundCache[(int)hand])
    {
      getArticulatedWrapper()->setBound(index, m_handBoundCache[(int)hand]);
      m_handBoundCache[(int)hand]->Release(); // decrement reference count
    }

    m_gunToHandCurrent[hand].Identity();

#if NM_SET_WEAPON_MASS
    restoreHandMass(hand);
#endif
  }
#endif//NM_SET_WEAPON_BOUND

#if NM_SET_WEAPON_MASS
  void NmRsCharacter::cacheHandMass(NmRsHand hand)
  {
    NmRsGenericPart* part = ((hand == kLeftHand) ? getLeftArmSetup()->getHand() : getRightArmSetup()->getHand());
    rage::phArticulatedBodyPart *bodyPart = (rage::phArticulatedBodyPart *)(part->getDataPtr());
    m_handMassCache[hand] = bodyPart->GetMass().Getf();
  }

  void NmRsCharacter::setHandMass(int hand, float mass, rage::Vector3* comOffset /* = 0 */)
  {
    NmRsGenericPart* part = ((hand == kLeftHand) ? getLeftArmSetup()->getHand() : getRightArmSetup()->getHand());
    rage::phArticulatedBodyPart *bodyPart = (rage::phArticulatedBodyPart *)(part->getDataPtr());

    // set mass
    Assert(bodyPart);
    Assert(mass > 0.f);
    bodyPart->SetMassOnly(mass);

    // set com offset
    rage::Vector3 offset;
    offset.Zero();
    if(comOffset)
      offset.Set(*comOffset);
    rage::phBound* bound = part->getBound();
    bound->SetCGOffset(offset);
  }

  void NmRsCharacter::restoreHandMass(NmRsHand hand)
  {
    Assert(m_handMassCache[hand] >= 0.5f);
    setHandMass(hand, m_handMassCache[hand]);
  }
#endif

  void NmRsCharacter::setWeaponMode(int weaponMode)
  {
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "setWeaponMode", weaponMode);
#endif

    WeaponMode weaponModeNew = (WeaponMode)(weaponMode + 1);//the 1 offset is because the game sends in kNone as -1
    if (weaponModeNew >= kNumWeaponModes)
      weaponModeNew = kNone;
    m_weaponModeChanged = weaponModeNew != m_weaponMode;
    m_weaponMode = weaponModeNew;

    //NB: This needs to be revisited.
    //  Not all behaviours limit the arm movement if the armState == eHS_HoldingItem
    //  An arm with armState == eHS_HoldingItem can be left set to (0,0,0) or doing very little. 
    //  Only a rifle in the right hand looks bad if other behaviours move it. 
    m_characterConfig.m_leftHandState = CharacterConfiguration::eHS_Free;
    m_characterConfig.m_rightHandState = CharacterConfiguration::eHS_Free;
    switch(m_weaponMode)
    {
    case kRifle:
        m_characterConfig.m_rightHandState = CharacterConfiguration::eHS_Rifle;
      break;
    case kPistol:
      case kPistolRight:
        m_characterConfig.m_rightHandState = CharacterConfiguration::eHS_Pistol;
        break;
      case kSidearm:
        m_characterConfig.m_leftHandState = CharacterConfiguration::eHS_Rifle;
        m_characterConfig.m_rightHandState = CharacterConfiguration::eHS_Pistol;
        break;
      case kPistolLeft:
        m_characterConfig.m_leftHandState = CharacterConfiguration::eHS_Pistol;
        break;
      case kDual:
        m_characterConfig.m_leftHandState = CharacterConfiguration::eHS_Pistol;
        m_characterConfig.m_rightHandState = CharacterConfiguration::eHS_Pistol;
      break;
    default:
        break;
    }
  }

  NmRsEffectorBase* NmRsCharacter::add1DofEffector(rage::phJoint1Dof* joint, int jointIndex, int jointTypeIndex, NmRs1DofEffectorParams &info )
  {
    NmRs1DofEffector* effector;
    ARTCustomPlacementNew4Arg(effector, NmRs1DofEffector, joint, jointIndex, jointTypeIndex, info );

    m_effectors[jointIndex] = effector;
    m_1dofEffectors[jointTypeIndex] = effector;

    return effector;
  }

  NmRsEffectorBase* NmRsCharacter::add3DofEffector(rage::phJoint3Dof* joint, int jointIndex, int jointTypeIndex, NmRs3DofEffectorParams &info )
  {
    NmRs3DofEffector* effector;
    ARTCustomPlacementNew4Arg(effector, NmRs3DofEffector, joint, jointIndex, jointTypeIndex, info );

    m_effectors[jointIndex] = effector;
    m_3dofEffectors[jointTypeIndex] = effector;

    return effector;
  }

  void NmRsCharacter:: setBodyDriveState(rage::phJoint::driveState state)
  {
    for(int i=0; i<m_effectorCount; i++)
      m_effectors[i]->setDriveState(state);
  }

  void NmRsCharacter::storeZeroPoseAllEffectors()//mmmmtodo can be removed when configureCharacter has been removed
  {
    for(int i=0; i<m_effectorCount; i++)
      m_effectors[i]->storeZeroPose();
  }

  void NmRsCharacter::recalibrateAllEffectors()
  {
    for(int i=0; i<m_effectorCount; i++)
      m_effectors[i]->resetEffectorCalibrations();
  }

  void NmRsCharacter::resetAllEffectors()
  {
    for(int i=0; i<m_effectorCount; i++)
    {
      m_effectors[i]->resetEffectorCalibrations();
      m_effectors[i]->resetAngles();
    }
  }

  rage::phBound* NmRsCharacter::getArticulatedBound() const
  {
    if (getArticulatedWrapper())
      return getArticulatedWrapper()->getArchetype()->GetBound();
    else
      return 0;
  }

  rage::phArticulatedBody* NmRsCharacter::getArticulatedBody() const
  {
    if (getArticulatedWrapper())
      return getArticulatedWrapper()->getArticulatedBody();
    else
      return 0;
  }

  rage::phInst *NmRsCharacter::getFirstInstance() const
  {
    if (getArticulatedWrapper())
      return getArticulatedWrapper()->getArticulatedPhysInstance();
    else
    {
      NmRsGenericPart *part = getGenericPartByIndex(0);
      if (part)
        return part->getInstance();
    }
    return 0;
  }


  // NOTES: COMrotationalVelocity
  // it is an approximation. A better approximation would be:
  // sum(momentOfInertia*rotationalVelocity + mass*velocity X vectorFromCharacterCOM) / sum(momentOfInertia + mass*sqr(distanceFromCharacterCOM)) see function below this
  // for all bodies in the character. Where momentOfInertia can be approximated as the average of the tensor diagonals
  // The correct solution requires full use of the tensor and part matrices, but is not much more accurate.
  //
  // NOTES: AngMom - doesn't include inertias of the parts
  // We want angmom about com:
  //
  //  AngMom_about_P = AngMom_of_Com_about_P + AngMom_about_com
  //  L_P = L_COM_P + L*
  //
  //  L_P = Sum_i[Ri x (mi.Vi)]
  //  L_COM_P = R* x (M.V*)
  //  L* = Sum_i[ri x mi.vi)]
  //  We could calculate with the above equation after knowing the com position but this would mean looping over the parts twice.
  //    Ri = displacement of ith part from P
  //    mi = mass of ith part
  //    Vi = velocity of ith part
  //    R* = displacement of com from P = 1/M * Sum_i[mi.Ri]
  //    M = total mass of the parts
  //    V* = velocity of com = 1/M * Sum_i[mi.Vi]
  //    ri = displacement of ith part from com
  //    vi = velocity of ith part wrt com
  //
  //  L* = L_P - L_COM_P 
  //  = Sum_i[Ri x (mi.Vi)] - R* x (M.V*)
  //  = Sum_i[Ri x (mi.Vi)] – 1/M * Sum_i[mi.Ri] x Sum_i[mi.Vi]  
  //
  //Therefore we can calculate the Sum_i terms above as we loop over the parts for the com calculations and get the angmom about the com

#if NM_RIGID_BODY_BULLET
  void NmRsCharacter::calculateCoM(
    rage::Matrix34* comResult,
    rage::Vector3* comVelResult,
    rage::Vector3* comAngVelResult,
    rage::Vector3* angMomResult) 
#else
  void NmRsCharacter::calculateCoM(
    rage::Matrix34* comResult,
    rage::Vector3* comVelResult,
    rage::Vector3* comAngVelResult,
    rage::Vector3* angMomResult) const
#endif
  {
    float totalMass = 0.0f;
    rage::Matrix34 mat;
    rage::Vector3 axis0, axis1, axis2, bpMass,
      vCom, vComVel, vComAngVel,
      angMom, sum_Ri_x_Vi, sum_miRi, sum_miVi,
      angMomOfPartAboutP;//P will be the ArticulatedCollider position i.e. offset
    NmRsGenericPart* part;

    axis0.Zero(); axis1.Zero(); axis2.Zero();
    vCom.Zero(); vComVel.Zero(); vComAngVel.Zero();
    angMom.Zero(); sum_Ri_x_Vi.Zero(); sum_miRi.Zero(); sum_miVi.Zero(); 

    if (getArticulatedWrapper())
    {
      rage::Vector3 offset = RCC_VECTOR3(getArticulatedWrapper()->getArticulatedCollider()->GetMatrix().GetCol3ConstRef());

      rage::phArticulatedBody *body = getArticulatedWrapper()->getArticulatedBody();
      int numLinks = body->GetNumJoints() + 1;

#if NM_RIGID_BODY_BULLET
      rage::Vector3 bpInertiaInBp;
      rage::Matrix34 bpInertiaAboutInstance0InN;
      m_characterInertiaAboutComInN.Zero();
      m_characterInertiaAboutPivotInN.Zero();
#endif        

      for (int i = 0; i<numLinks; i++)
      {
        part = getGenericPartByIndex(i);

        rage::phArticulatedBodyPart& bp = body->GetLink(i);
        bpMass = SCALARV_TO_VECTOR3(body->GetMass(i));

        rage::Matrix34 mat1 = MAT34V_TO_MATRIX34(bp.GetMatrix());
        rage::Matrix34 mat0 = part->getInitialMatrix(); // not transposed as stored in generic part (transposed are the ones in the bounds)

        mat0.Inverse3x3();
        mat.Dot3x3(mat1, mat0);

        axis0.AddScaled(mat.GetVector(0), bpMass);
        axis1.AddScaled(mat.GetVector(1), bpMass);
        axis2.AddScaled(mat.GetVector(2), bpMass);

        vCom.AddScaled(bp.GetPosition(), bpMass);
        vComVel.AddScaled(VEC3V_TO_VECTOR3(getArticulatedBody()->GetLinearVelocityNoProp(i)), bpMass); // NoProp
        vComAngVel.AddScaled(VEC3V_TO_VECTOR3(getArticulatedBody()->GetAngularVelocityNoProp(i)), bpMass); // NoProp

        totalMass += bpMass.x;
        //Calcs for the AngMom
        angMomOfPartAboutP = bp.GetPosition();
        angMomOfPartAboutP.Cross(bpMass.x*VEC3V_TO_VECTOR3(getArticulatedBody()->GetLinearVelocityNoProp(i))); // NoProp
        sum_Ri_x_Vi += angMomOfPartAboutP;
        sum_miRi += bpMass.x*bp.GetPosition();
        sum_miVi += bpMass.x*VEC3V_TO_VECTOR3(getArticulatedBody()->GetLinearVelocityNoProp(i)); // NoProp
#if NM_RIGID_BODY_BULLET
        bpInertiaInBp = VEC3V_TO_VECTOR3(body->GetAngInertia(i));
        //bpInertiaMatrixAboutBpInBp
        m_bpInertiaMatrixAboutBpInN[i].Zero();
        m_bpInertiaMatrixAboutBpInN[i].a.x = bpInertiaInBp.x;
        m_bpInertiaMatrixAboutBpInN[i].b.y = bpInertiaInBp.y;
        m_bpInertiaMatrixAboutBpInN[i].c.z = bpInertiaInBp.z;
        //Convert bpInertiaMatrixAboutBpInBp to m_bpInertiaMatrixAboutBpInN
        rage::Matrix34 Tb2N = MAT34V_TO_MATRIX34(bp.GetMatrix());
        rage::Matrix34 Tb2NTranspose = Tb2N;
        Tb2NTranspose.Transpose();
        m_bpInertiaMatrixAboutBpInN[i].Dot3x3(Tb2NTranspose);
        m_bpInertiaMatrixAboutBpInN[i].Dot3x3FromLeft(Tb2N);
        //Move axes to change m_bpInertiaMatrixAboutBpInN to bpInertiaAboutInstance0InN using parallel axis theorem
        rage::Vector3 posBpInInstance = bp.GetPosition();
        bpInertiaAboutInstance0InN = m_bpInertiaMatrixAboutBpInN[i];
        bpInertiaAboutInstance0InN.a.x += bpMass.x*(posBpInInstance.y*posBpInInstance.y + posBpInInstance.z*posBpInInstance.z);
        bpInertiaAboutInstance0InN.b.y += bpMass.x*(posBpInInstance.x*posBpInInstance.x + posBpInInstance.z*posBpInInstance.z);
        bpInertiaAboutInstance0InN.c.z += bpMass.x*(posBpInInstance.x*posBpInInstance.x + posBpInInstance.y*posBpInInstance.y);
        bpInertiaAboutInstance0InN.a.y += bpMass.x*(-posBpInInstance.x*posBpInInstance.y);
        bpInertiaAboutInstance0InN.b.x += bpMass.x*(-posBpInInstance.x*posBpInInstance.y);
        bpInertiaAboutInstance0InN.a.z += bpMass.x*(-posBpInInstance.x*posBpInInstance.z);
        bpInertiaAboutInstance0InN.c.x += bpMass.x*(-posBpInInstance.x*posBpInInstance.z);
        bpInertiaAboutInstance0InN.b.z += bpMass.x*(-posBpInInstance.y*posBpInInstance.z);
        bpInertiaAboutInstance0InN.c.y += bpMass.x*(-posBpInInstance.y*posBpInInstance.z);
        //add result to character I matrix
        //at the moment we are calculating characterInertiaAboutInstance0InN 
        m_characterInertiaAboutComInN.Add(bpInertiaAboutInstance0InN);
#endif
      }

      axis0.Normalize();
      axis1.Normalize();
      axis2.Normalize();

      if (m_attachedObject.partIndex >= 0)
      {
        vCom += (m_attachedObject.worldCOMPos - offset) * m_attachedObject.mass;//NB massMultiplier not applied here that's just to give a heavier effect for gravityCompensation - com should use the real mass
        totalMass += m_attachedObject.mass;
      }

      if (m_leftHandWeapon.partIndex >= 0)
      {
        vCom += (m_leftHandWeapon.worldCOMPos - offset) * m_leftHandWeapon.mass;
        totalMass += m_leftHandWeapon.mass;
      }
      if (m_rightHandWeapon.partIndex >= 0)
      {
        vCom += (m_rightHandWeapon.worldCOMPos - offset) * m_rightHandWeapon.mass;
        totalMass += m_rightHandWeapon.mass;
      }
      rage::Vector3 vecInvTotalMass;
      Assert(totalMass > 0.f);
      vecInvTotalMass.Set(1.0f / totalMass);

      vCom.Multiply(vecInvTotalMass);
      vComAngVel.Multiply(vecInvTotalMass);
      vComVel.Multiply(vecInvTotalMass);

      // add on the position of the instance (since body part positions are local to the instance)
      vCom += offset;

      comResult->Set(axis0, axis1, axis2, vCom);
      comResult->Transpose();

      comVelResult->Set(vComVel);

      if (m_zUp) // rotate the matrix the right way
      {
        rage::Vector3 up = comResult->c;
        comResult->c = -comResult->b;
        comResult->b = up;
      }

      comAngVelResult->Set(vComAngVel);

#if NM_RS_VALIDATE_VITAL_VALUES
      Assert(vCom.x == vCom.x && vCom.y == vCom.y && vCom.z == vCom.z);
      Assert(vComVel.x == vComVel.x && vComVel.y == vComVel.y && vComVel.z == vComVel.z);
      Assert(vComAngVel.x == vComAngVel.x && vComAngVel.y == vComAngVel.y && vComAngVel.z == vComAngVel.z);
      Assert(comResult->IsEqual(*comResult));
#endif // NM_RS_VALIDATE_VITAL_VALUES

      comResult->Normalize();

      //Angmom
      angMom.Cross(sum_miRi,sum_miVi);
      angMom *= -1/totalMass;
      angMom += sum_Ri_x_Vi;
      angMomResult->Set(angMom);

#if NM_RIGID_BODY_BULLET
      //Move axes to change characterInertiaAboutInstance0InN to m_characterInertiaAboutComInN using parallel axis theorem
      rage::Vector3 instance2com = comResult->d - offset;
      m_characterInertiaAboutComInN.a.x += totalMass*(instance2com.y*instance2com.y + instance2com.z*instance2com.z);
      m_characterInertiaAboutComInN.b.y += totalMass*(instance2com.x*instance2com.x + instance2com.z*instance2com.z);
      m_characterInertiaAboutComInN.c.z += totalMass*(instance2com.x*instance2com.x + instance2com.y*instance2com.y);
      m_characterInertiaAboutComInN.a.y += totalMass*(-instance2com.x*instance2com.y);
      m_characterInertiaAboutComInN.b.x += totalMass*(-instance2com.x*instance2com.y);
      m_characterInertiaAboutComInN.a.z += totalMass*(-instance2com.x*instance2com.z);
      m_characterInertiaAboutComInN.c.x += totalMass*(-instance2com.x*instance2com.z);
      m_characterInertiaAboutComInN.b.z += totalMass*(-instance2com.y*instance2com.z);
      m_characterInertiaAboutComInN.c.y += totalMass*(-instance2com.y*instance2com.z);
      m_characterInertiaAboutPivotInN.Set(m_characterInertiaAboutComInN);
#endif
    }
  }

#if NM_RIGID_BODY_BULLET
  void NmRsCharacter::getCharacterInertiaAboutPivot(rage::Matrix34* characterInertiaAboutPivotInN, rage::Vector3* pivotPoint)
  {
    //We only want to do this once every step if requested
    //Move axes to change m_characterInertiaAboutComInN to m_characterInertiaAboutPivotInN using parallel axis theorem
    //Pivot can be COM if airborne or pivot not set or centreOfFeet if both feet on ground or foot position if only 1 foot on ground
    rage::Vector3 pivotPointLocal;
    bool pivotNotCom = false;
    if (getLeftLegSetup()->getFoot()->collidedWithEnvironment() && getRightLegSetup()->getFoot()->collidedWithEnvironment())
    {
      //pivot around centre of feet
      pivotPointLocal = 0.5f*(getLeftLegSetup()->getFoot()->getPosition() + getRightLegSetup()->getFoot()->getPosition());
      pivotNotCom = true;
    }
    else if (getLeftLegSetup()->getFoot()->collidedWithEnvironment())
    {
      //pivot around left foot
      pivotPointLocal = getLeftLegSetup()->getFoot()->getPosition();
      pivotNotCom = true;
    }
    else if (getRightLegSetup()->getFoot()->collidedWithEnvironment())
    {
      //pivot around right foot
      pivotPointLocal = getRightLegSetup()->getFoot()->getPosition();
      pivotNotCom = true;
    }
    if (pivotNotCom)
    {
      rage::Vector3 offset = RCC_VECTOR3(getArticulatedWrapper()->getArticulatedCollider()->GetMatrix().GetCol3ConstRef());
      rage::Vector3 com2pivot = pivotPointLocal - m_COM;
      float totalMass = getTotalMass();
      m_characterInertiaAboutPivotInN.a.x += totalMass*(com2pivot.y*com2pivot.y + com2pivot.z*com2pivot.z);
      m_characterInertiaAboutPivotInN.b.y += totalMass*(com2pivot.x*com2pivot.x + com2pivot.z*com2pivot.z);
      m_characterInertiaAboutPivotInN.c.z += totalMass*(com2pivot.x*com2pivot.x + com2pivot.y*com2pivot.y);
      m_characterInertiaAboutPivotInN.a.y += totalMass*(-com2pivot.x*com2pivot.y);
      m_characterInertiaAboutPivotInN.b.x += totalMass*(-com2pivot.x*com2pivot.y);
      m_characterInertiaAboutPivotInN.a.z += totalMass*(-com2pivot.x*com2pivot.z);
      m_characterInertiaAboutPivotInN.c.x += totalMass*(-com2pivot.x*com2pivot.z);
      m_characterInertiaAboutPivotInN.b.z += totalMass*(-com2pivot.y*com2pivot.z);
      m_characterInertiaAboutPivotInN.c.y += totalMass*(-com2pivot.y*com2pivot.z);
      pivotPoint->Set(pivotPointLocal);
      characterInertiaAboutPivotInN->Set(m_characterInertiaAboutPivotInN);
    }
  }
#endif

  float NmRsCharacter::pointInsideFootSupport(const rage::Vector3 &point, int leftIndex, int rightIndex, float footWidth, float footLength, rage::Vector3 *nearestPoint)
  {
    rage::Matrix34 leftMat;
    rage::Matrix34 rightMat;
    if (leftIndex != -1)
    {
      NmRsGenericPart* leftFoot = getGenericPartByIndex(leftIndex);
      leftFoot->getMatrix(leftMat);
    }
    if (rightIndex != -1)
    {
      NmRsGenericPart* rightFoot  = getGenericPartByIndex(rightIndex);
      rightFoot->getMatrix(rightMat);
    }
    return pointInsideSupport(point, leftIndex!=-1 ? &leftMat : NULL, rightIndex!=-1 ? &rightMat : NULL, footWidth, footLength, getUpVector(), nearestPoint);
  }

  bool NmRsCharacter::noBehavioursUsingDynBalance()
  {
    int numActive = 0;
    if (m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_bodyBalance))
      numActive++;
    if (m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_braceForImpact))
      numActive++;
    //if (m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_highFall))//this calls bodyBalance
    //  numActive++;
#if ALLOW_TRAINING_BEHAVIOURS & 0//Needs changing to limb system
    if (m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_landing))
      numActive++;
#endif
    if (m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_shot))
      numActive++;
    if (m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_staggerFall))
      numActive++;
    if (m_cbuTaskManager->getTaskIsActiveByID(getID(), bvid_yanked))
      numActive++;

    return (numActive == 0);
  }
#if NM_UNUSED_CODE
  void NmRsCharacter::getInstanceTM(rage::Matrix34 *matrix) const
  {
    Assert(getArticulatedWrapper());
    *matrix = RCC_MATRIX34(getArticulatedWrapper()->getArticulatedPhysInstance()->GetMatrix());
  }
#endif

#if CRAWL_LEARNING // At present, only the Crawl learner uses this function
  void NmRsCharacter::disableSelfCollision()
  {
    Assert(getArticulatedWrapper());

	  // The best way to handle this is by disabling the impacts in handleCollision, since the selfCollision arrays are shared now.
  }
#endif

  float NmRsCharacter::twistCompensation(NmRsGenericPart* foot, float startTwist, float twistStrength, float twistDamping, float mult)
  {
    rage::Matrix34 footTm;
    rage::Vector3 xAxis(1, 0, 0), otherHorizontalAxis, upVec(m_gUpReal);
    //should torque around normal to floor but it makes no sense to do it round a leaned up so get the realUp


    foot->getMatrix(footTm);
    otherHorizontalAxis.Cross(upVec, xAxis);

    float height = rage::Max(0.0f, upVec.Dot(footTm.b));

    float x = footTm.c.Dot(xAxis);
    float y = footTm.c.Dot(otherHorizontalAxis);

    float twist = atan2f(x, y);

    rage::Vector3 rotVel = foot->getAngularVelocity();
    float twistVel = rotVel.Dot(upVec);

    bool isRightFoot = foot == getRightLegSetup()->getFoot();
    bool doInitialize = !foot->previousCollided() || (!m_footROrientationInitialized && isRightFoot) || (!m_footLOrientationInitialized && !isRightFoot);
    if(doInitialize)
    {
      startTwist = twist;
      if (isRightFoot)
        m_footROrientationInitialized = true;
      else
        m_footLOrientationInitialized = true;
    }
#if ART_ENABLE_BSPY && 0
    bspyScratchpad(getBSpyID(), "character.twistCompensation", m_footROrientationInitialized);
    bspyScratchpad(getBSpyID(), "character.twistCompensation", m_footLOrientationInitialized);
#endif
    rage::Vector3 torque(upVec);
    float dif = twist - startTwist;
    if (dif > PI)
      dif -= 2.0f * PI;
    if (dif < -PI)
      dif += 2.0f * PI;

    // height ensures less compensation when feet not level
    torque *=  (dif * twistStrength - twistVel * twistDamping) * height;
    float scale = rage::Clamp(60.f*getLastKnownUpdateStep(), 1.0f, 60.0f);//clamp to keep torque same above 60fps and reduce torque from 60fps to 1fps
    Assert(scale > 0.f);
    foot->applyTorque(mult * torque / scale);

    return startTwist;
  }

  void NmRsCharacter::stayUprightByComTorques(float stiffness, float damping)
  {
    float somK = stiffness*500.f/3.f;
    float somD = damping*4.f;
    float tiltK = stiffness*500.f/3.f;
    float tiltD = damping*4.f;;

    rage::Matrix34 tmCom = m_COMTM;
    rage::Vector3 bodyRight = tmCom.a;
    rage::Vector3 bodyUp = tmCom.b;
    rage::Vector3 bodyBack = tmCom.c;
    float qTilt;
    float qSom;

    if (m_gUp.y > 0.9f) //y_Up
    {
      float arcsin = bodyBack.y;
      qSom = rage::AsinfSafe(arcsin);
      float c2 = rage::Cosf(qSom);
      Assert(rage::Abs(c2) > 1e-10f);
      arcsin = -bodyRight.y / c2;
      qTilt = rage::AsinfSafe(arcsin);
    }
    else
    {
      float arcsin = bodyBack.z;
      qSom = rage::AsinfSafe(arcsin);
      float c2 = rage::Cosf(qSom);
      Assert(rage::Abs(c2) > 1e-10f);
      arcsin = -bodyRight.z / c2;
      qTilt = rage::AsinfSafe(arcsin);
    }   
    rage::Vector3 comAngVel = m_COMrotvel;    
    float somVel = comAngVel.Dot(bodyRight);
    float tiltVel = comAngVel.Dot(bodyBack);

    NmRsGenericPart* pelvis = getSpineSetup()->getPelvisPart();

    rage::Vector3 torque;
    torque = bodyRight;
    float scale = rage::Clamp(60.f*getLastKnownUpdateStep(), 1.0f, 60.0f);//clamp to keep torque same above 60fps and reduce torque from 60fps to 1fps
    Assert(scale > 0.f);
    torque *=  (qSom * somK - somVel * somD);
    pelvis->applyTorque(torque / scale);

#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "Upright torques", stiffness);
    bspyScratchpad(getBSpyID(), "Upright torques", damping);
    bspyScratchpad(getBSpyID(), "Upright torques", somK);
    bspyScratchpad(getBSpyID(), "Upright torques", somD);
    bspyScratchpad(getBSpyID(), "Upright torques", tiltK);
    bspyScratchpad(getBSpyID(), "Upright torques", tiltD);
    bspyScratchpad(getBSpyID(), "Upright torques - som", torque.Mag());
#endif

    torque = bodyBack;
    torque *=  (qTilt * tiltK - tiltVel * tiltD);
    pelvis->applyTorque(torque / scale);

#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "Upright torques - tilt", torque.Mag());
#endif
  }

  void NmRsCharacter::applyFootSlipCompensation(float twistStrength, float twistDamping, float mult)
  {
    NmRsGenericPart* leftFoot = getLeftLegSetup()->getFoot();
    NmRsGenericPart* rightFoot = getRightLegSetup()->getFoot();

    Assert(leftFoot && rightFoot);

    if (leftFoot->collidedWithNotOwnCharacter() )
      m_fscLeftTwist = twistCompensation(leftFoot, m_fscLeftTwist, twistStrength, twistDamping, mult);

    if (rightFoot->collidedWithNotOwnCharacter())
      m_fscRightTwist = twistCompensation(rightFoot, m_fscRightTwist, twistStrength, twistDamping, mult);
  }

  void NmRsCharacter::setIncomingTransforms(rage::Matrix34* ptr, IncomingTransformStatus statFlag, int tmcount, IncomingTransformSource source)
  {
    m_incomingTm[source] = ptr;
    m_incomingTmStatus = statFlag;
    m_incomingTmCount = tmcount;
  }

  void NmRsCharacter::getIncomingTransforms(rage::Matrix34 **ptr, IncomingTransformStatus &statFlag, int &tmcount, IncomingTransformSource source) const
  {
    *ptr = m_incomingTm[source];
    statFlag = m_incomingTmStatus;
    tmcount = m_incomingTmCount;
  }

  rage::Matrix34* NmRsCharacter::GetWorldLastMatrices()
  {
    return m_WorldLastMatrices;
  }

  rage::Matrix34* NmRsCharacter::GetWorldCurrentMatrices()
  {
    return m_WorldCurrentMatrices;
  }

#if NM_ANIM_MATRICES
  rage::Matrix34* NmRsCharacter::GetBlendOutAnimationMatrices()
  {
    return m_BlendOutAnimationMatrices;
  }
#endif

  void NmRsCharacter::applyIncomingTransforms(float deltaTime)
  {
    if (m_applyMode == kDisabled)
      return;

    BehaviourMask disablingMask = (m_lastApplyMask^m_applyMask)&~m_applyMask;
#if ART_ENABLE_BSPY && NM_RS_MASKING_DEBUG
    bspyScratchpad(getBSpyID(), "applyIncomingTransforms", deltaTime);
    bspyScratchpad(getBSpyID(), "applyIncomingTransforms", getIncomingTransformApplyMode());
    bspyScratchpad_Bitfield32(getBSpyID(), "applyIncomingTransforms", m_applyMask);
    bspyScratchpad_Bitfield32(getBSpyID(), "applyIncomingTransforms", m_lastApplyMask);
    bspyScratchpad_Bitfield32(getBSpyID(), "applyIncomingTransforms", disablingMask);
    bspyScratchpad(getBSpyID(), "applyIncomingTransforms", m_incomingAnimationVelocityScale);
#endif

    // go get the transforms from the agent
    int incomingComponentCount = 0;
    rage::Matrix34 *itPtr = 0;
    rage::Matrix34 *prevItPtr = 0;
    IncomingTransformStatus itmStatusFlags = kITSNone;
    getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, kITSourceCurrent);
    getIncomingTransforms(&prevItPtr, itmStatusFlags, incomingComponentCount, kITSourcePrevious);

    Assert(deltaTime > 0.f);
    float invDeltaTime = 1.0f / deltaTime;

    // no transforms?
    if (itPtr == 0 || incomingComponentCount == 0)
      return;

    int compCount = rage::Min(incomingComponentCount, m_genericPartCount);

    for (int i=0; i<compCount; i++)
    {
      NmRsGenericPart *part = getGenericPartByIndex(i);
      if(part)
      {
        if ((m_applyMode == kSingleFrame ||
          m_applyMode == kEnabling) &&
          isPartInMask(m_applyMask, part->getPartIndex()))
        {
          rage::Matrix34& toTM = itPtr[i];
          part->teleportMatrix(toTM);

          if (prevItPtr)
          {
            rage::Matrix34& fromTM = prevItPtr[i];

            part->applyVelocitiesToPart(
              fromTM,
              toTM,
              invDeltaTime);
          }
          else
          {
            part->setLinearVelocity(0, 0, 0);
            part->setAngularVelocity(0, 0, 0);
          }
#if ART_ENABLE_BSPY && NM_RS_MASKING_DEBUG
          bspyLogf(info, L"enabling %d", i);
#endif
        }
        // if we are about to be disabled, set physics components back to their original state
        else if ( (m_applyMode == kDisabling &&
          isPartInMask(m_applyMask, part->getPartIndex())) 
          || (m_applyMode == kContinuous &&
          isPartInMask(disablingMask, part->getPartIndex()))
          )
        {
#if ART_ENABLE_BSPY && NM_RS_MASKING_DEBUG
          bspyLogf(info, L"disabling %d", i);
#endif

          rage::Matrix34& toTM = itPtr[i];
          part->setMatrix(toTM);

          if (prevItPtr)
          {
            rage::Matrix34& fromTM = prevItPtr[i];

            part->applyVelocitiesToPart(
              fromTM,
              toTM,
              invDeltaTime);
          }
          else
          {
            part->setLinearVelocity(0, 0, 0);
            part->setAngularVelocity(0, 0, 0);
          }
        }
        else if (m_applyMode == kContinuous &&
          isPartInMask(m_applyMask, part->getPartIndex()))
        {
          rage::Matrix34& toTM = itPtr[i];
          part->setMatrix(toTM);
          part->setLinearVelocity(0, 0, 0);
          part->setAngularVelocity(0, 0, 0);
#if ART_ENABLE_BSPY && NM_RS_MASKING_DEBUG
          bspyLogf(info, L"ticking %d", i);
#endif
        }
      }
    }

    m_lastApplyMask = m_applyMask;
  }

  void NmRsCharacter::setIncomingTransformApplyMode(const IncomingTransformApplyMode mode)
  {
    // don't set kDisabling if we're already disabled. this causes unnecessary velocity
    // override to occur.
    if(!(mode == kDisabling && m_applyMode == kDisabled))
      m_applyMode = mode;
  }

  void NmRsCharacter::setIncomingTransformMask(BehaviourMask mask)
  {
    m_lastApplyMask = m_applyMask;
    m_applyMask = mask;
  }

#if ART_ENABLE_BSPY && NM_RS_MASKING_DEBUG
  IncomingTransformApplyMode NmRsCharacter::getIncomingTransformApplyMode() const
  {
    return m_applyMode;
  }
#endif

  void NmRsCharacter::updateIncomingTransformApplyMode()
  {
    IncomingTransformApplyMode newMode = m_applyMode;
    if (newMode == kEnabling)
      newMode = kContinuous;
    else if (newMode == kSingleFrame)
      newMode = kDisabled;
    else if (newMode == kDisabling)
      newMode = kDisabled;

    m_applyMode = newMode;
  }

  bool NmRsCharacter::setInitialTM(int componentIdx, const rage::Matrix34& newTm, const rage::Matrix34 *previousFrameTm)
  {
    Assert(componentIdx >= 0 && componentIdx < m_genericPartCount);
    NmRsGenericPart *part = getGenericPartByIndex(componentIdx);

    rage::Matrix34 curTm(newTm);
    curTm.NormalizeSafe();

    if (previousFrameTm && (uintptr_t)previousFrameTm != (uintptr_t)&newTm)
    {
      part->teleportMatrix(curTm, true);

      part->applyVelocitiesToPart(
        *previousFrameTm,
        curTm,
        1.0f / getLastKnownUpdateStep());
    }
    else
    {
      part->teleportMatrix(curTm, true);
      part->setLinearVelocity(0, 0, 0);
      part->setAngularVelocity(0, 0, 0);
    }
    return true;
  }

  ART::ARTFeedbackInterface *NmRsCharacter::simpleFeedback(const char *name)
  {
    ART::ARTFeedbackInterface *feedback = getFeedbackInterface();
    Assert(feedback);
    feedback->m_agentID = getID();
    feedback->m_argsCount = 0;
    strcpy(feedback->m_behaviourName, name);
    return feedback;
  }

  void NmRsCharacter::sendFeedbackSuccess(const char *name){ if (!getFeedbackInterface()) return; simpleFeedback(name)->onBehaviourSuccess(); }
  void NmRsCharacter::sendFeedbackFailure(const char *name){ if (!getFeedbackInterface()) return; simpleFeedback(name)->onBehaviourFailure(); }
  void NmRsCharacter::sendFeedbackFinish(const char *name) { if (!getFeedbackInterface()) return; simpleFeedback(name)->onBehaviourFinish(); }
#if NM_UNUSED_CODE
  void NmRsCharacter::sendFeedbackEvent(const char *name) { if (!getFeedbackInterface()) return; simpleFeedback(name)->onBehaviourEvent(); }
#endif
  /**
   * this function sets up the userdata and feedback information as above
   * but as a straight C call, designed to cover the event case that passes only a string and an id
   */
  bool NmRsCharacter::prepareFeedback(const char* name, int id)
  {
    // return false if no feedback interface was set for this agent
    ARTFeedbackInterface* feedback = getFeedbackInterface();
    if (!feedback)
    {
      return false;
    }

    feedback->m_agentID = m_agentID;
    feedback->m_agentInstance = this;

    // stick the passed behaviour name into the feedback function
    feedback->m_behaviourName[0] = '\0';
    feedback->m_calledByBehaviourName[0] = '\0';
    strncpy(feedback->m_behaviourName, name, ART_FEEDBACK_BHNAME_LENGTH - 1);

    // loop through the arguments passed and convert them
    // into arguments stored in the feedback interface
    feedback->m_argsCount = 0;
    feedback->m_args[feedback->m_argsCount++].setFloat((float)id);

    return true;
  }
#if NM_UNUSED_CODE
  int NmRsCharacter::feedbackBehaviourEvent(const char* name, int id)
  {
    if (!prepareFeedback(name, id))
      return -1;

    // call and return the result of the feedback function
    // if it was not overloaded, it will return -1
    return getFeedbackInterface()->onBehaviourEvent();
  }
#endif

  ARTFeedbackInterface* NmRsCharacter::getFeedbackInterface()
  {
#if ART_ENABLE_BSPY
    return this;
#else // ART_ENABLE_BSPY
    return m_feedbackInterface;
#endif // ART_ENABLE_BSPY
  }

  /**
   * Returns the AssetID of this character. 
   */
  AssetID NmRsCharacter::getAssetID() const
  {
    return m_asset;
  }

  void NmRsCharacter::setIdentifier(const char* ident)
  {
    strncpy(m_identifier, ident, rage::Min(strlen(ident), size_t(7)));
  }

  const char* NmRsCharacter::getIdentifier() const
  {
    return m_identifier;
  }


#if ART_ENABLE_BSPY
  void NmRsCharacter::copyFeedbackDataToLiveInstance()
  {
    if(m_feedbackInterface)
    {
      // replicate our setup to the real, live interface
      m_feedbackInterface->m_agentID = m_agentID;
      m_feedbackInterface->m_agentInstance = m_agentInstance;
      strcpy(m_feedbackInterface->m_behaviourName, m_behaviourName);
      strcpy(m_feedbackInterface->m_calledByBehaviourName, m_calledByBehaviourName);
      m_feedbackInterface->m_argsCount = m_argsCount;
      memcpy(m_feedbackInterface->m_args, m_args, sizeof(ARTFeedbackInterface::FeedbackUserdata) * m_argsCount);
    }
  }

#define bspyFeedbackStub(type, v_call)\
  int NmRsCharacter::v_call()\
  {\
  if (!m_feedbackInterface) return 0;\
  copyFeedbackDataToLiveInstance();\
  if (bSpyServer::inst() == 0 || !bSpyServer::inst()->isClientConnected() || m_agentID == INVALID_AGENTID) return m_feedbackInterface->v_call();\
  bSpy::FeedbackPacket fp(type, (bSpy::bs_uint8)m_agentID, (bSpy::bs_uint8)m_argsCount);\
  fp.m_behaviourName = bSpyServer::inst()->getTokenForString(m_behaviourName);\
  fp.m_calledByBehaviourName = bSpyServer::inst()->getTokenForString(m_calledByBehaviourName);\
  for (int i=0; i<fp.m_argsNum; i++)\
  {\
  switch (m_args[i].m_type)\
  {\
    case ARTFeedbackInterface::FeedbackUserdata::kInt:\
    fp.m_args[i].setInt(m_args[i].m_int);\
    break;\
    case ARTFeedbackInterface::FeedbackUserdata::kFloat:\
    fp.m_args[i].setFloat(m_args[i].m_float);\
    break;\
    case ARTFeedbackInterface::FeedbackUserdata::kBool:\
    fp.m_args[i].setBool(m_args[i].m_bool);\
    break;\
    case ARTFeedbackInterface::FeedbackUserdata::kVoid:\
    fp.m_args[i].setVoid();\
    break;\
    case ARTFeedbackInterface::FeedbackUserdata::kString:\
    bSpy::bSpyStringToken stringToken;\
    stringToken = bSpyServer::inst()->getTokenForString(m_args[i].m_string);\
    fp.m_args[i].setStringToken(stringToken);\
    break;\
  }\
  }\
  int retVal = m_feedbackInterface->v_call();\
  fp.m_retVal = (bSpy::bs_uint32)retVal;\
  bspySendPacket(fp);\
  return retVal;\
  }

  bspyFeedbackStub(bSpy::FeedbackPacket::ft_start, onBehaviourStart);
  bspyFeedbackStub(bSpy::FeedbackPacket::ft_failure, onBehaviourFailure);
  bspyFeedbackStub(bSpy::FeedbackPacket::ft_success, onBehaviourSuccess);
  bspyFeedbackStub(bSpy::FeedbackPacket::ft_finish, onBehaviourFinish);
  bspyFeedbackStub(bSpy::FeedbackPacket::ft_event, onBehaviourEvent);
  bspyFeedbackStub(bSpy::FeedbackPacket::ft_request, onBehaviourRequest);

  void NmRsCharacter::sendDescriptor()
  {
    Assert(m_agentID != INVALID_AGENTID);

    // send asset description
    {
      AssetDescriptorPacket adp;

      memset(adp.m_identity, 0, sizeof(char) * 8);
      strcpy(adp.m_identity, getIdentifier());

      adp.m_assetID = (bs_int16)getAssetID();
      adp.m_bodyIdent = (bs_int16)getBodyIdentifier();

      adp.m_genericPartCount = getNumberOfParts();
      adp.m_1dofCount = getNumberOf1Dofs();
      adp.m_3dofCount = getNumberOf3Dofs();

      bspySendPacket(adp);
    }

    // select which asset ID we will be describing
    SelectAssetPacket sap((bs_int16)getAssetID());
    bspySendPacket(sap);

    int i;

    // describe each part ..
    for (i=0; i<getNumberOfParts(); i++)
      m_parts[i]->sendDescriptor();

    // .. and now effectors
    for (i=0; i<getNumberOfEffectors(); i++)
      m_effectors[i]->sendDescriptor();

    // send memory usage report to bSpy log
    {
      size_t nmUsage = sizeof(NmRsCharacter);
      nmUsage += sizeof(NmRsGenericPart) * m_genericPartCount;
      nmUsage += sizeof(NmRs1DofEffector) * m_1dofCount;
      nmUsage += sizeof(NmRs3DofEffector) * m_3dofCount;
      nmUsage += sizeof(NmRsArticulatedWrapper);

      nmUsage += sizeof(NmRsGenericPart*) * 2 * m_genericPartCount;

      nmUsage += sizeof(NmRsEffectorBase*) * m_effectorCount;
      nmUsage += sizeof(NmRs1DofEffector*) * m_1dofCount;
      nmUsage += sizeof(NmRs3DofEffector*) * m_3dofCount;

      nmUsage += sizeof(rage::phJoint1Dof*) * m_1dofCount;
      nmUsage += sizeof(rage::phJoint3Dof*) * m_3dofCount;


      size_t rageUsage = sizeof(rage::phArticulatedCollider);
      rageUsage += sizeof(rage::phJoint1Dof) * m_1dofCount;
      rageUsage += sizeof(rage::phJoint3Dof) * m_3dofCount;
      rageUsage += sizeof(rage::phBoundCapsule) * m_genericPartCount; // worst-case overestimate!

      char usageRecord[128];
      sprintf(usageRecord, "NmRsCharacter AssetID(%i) - NM", getAssetID());
      bspyMemoryUsageRecord(usageRecord, nmUsage);

      sprintf(usageRecord, "NmRsCharacter AssetID(%i) - RAGE", getAssetID());
      bspyMemoryUsageRecord(usageRecord, rageUsage);
    }
  }

  void NmRsCharacter::sendUpdate()
  {
    Assert(m_agentID != INVALID_AGENTID);

    bspyProfileStart("NmRsCharacter::sendUpdate")

#if NM_EA
    NmRsSpy& spy = *m_rsEngine->getNmRsSpy();
#endif

    int i;

    // select the agent ID we will be updating
    SelectAgentPacket sap((bs_int16)getBSpyID());
    bspySendPacket(sap);

    // update character-level state
    {
      CharacterUpdatePacket cup;
      cup.d.m_gUp       = bSpyVec3fromVector3(m_gUp);
      cup.d.m_COMtm     = bSpyMat34fromMatrix34(m_COMTM);
      cup.d.m_COMvel    = bSpyVec3fromVector3(m_COMvel);
      cup.d.m_COMangvel = bSpyVec3fromVector3(m_COMrotvel);

      bspySendPacket(cup);
    }
    if (isBiped())
    {
      NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_cbuTaskManager->getTaskByID(getID(), bvid_dynamicBalancer);
      Assert(dynamicBalancerTask);

      if (dynamicBalancerTask->isActive())
      {
        const Vector3& leanHipUp = dynamicBalancerTask->getLeanHipUpVector();
        bspyCharacterVar(leanHipUp);
      }
    }


    bspyCharacterVar(m_angMom);
    bspyCharacterVar(getEngine()->getAnimationMaxSpeed());
    bspyCharacterVar(getEngine()->getAnimationMaxAngSpeed());
      bspyCharacterVar(m_strength);
      bspyCharacterVar(m_health);
      bspyCharacterVar(m_uprightConstraint.forceActive);
      bspyCharacterVar(m_footSlipCompensationActive);
      bspyCharacterVar(m_ZMPPostureControlActive);
      bspyCharacterVar(m_floorAcceleration);
      bspyCharacterVar(m_floorVelocity);
      bspyCharacterVar(m_groundNormal);
    // debug weapon mode
    static const char* state_names[] = 
    {
#define WM_NAME_ACTION(_name) #_name ,
      WM_STATES(WM_NAME_ACTION)
#undef WM_NAME_ACTION
    };

    if (m_weaponMode < kNumWeaponModes && m_weaponMode >= 0){
      bspyCharacterVar(state_names[m_weaponMode]);}
    else{
      bspyCharacterVar(m_weaponMode);}

    bspyCharacterVar(m_weaponModeChanged);
    bspyCharacterVar(m_rightHandWeapon.levelIndex);
    bspyCharacterVar(m_leftHandWeapon.levelIndex);

    static const char* hand_state_names[] = 
    {
#define HS_NAME_ACTION(_name) #_name ,
      HS_STATES(HS_NAME_ACTION)
#undef HS_NAME_ACTION
    };
    if (m_characterConfig.m_leftHandState < CharacterConfiguration::eHS_NumOfHandSates && m_characterConfig.m_leftHandState >= 0){
      bspyCharacterVar(hand_state_names[m_characterConfig.m_leftHandState]);}
    else{
      bspyCharacterVar(m_characterConfig.m_leftHandState);}
    if (m_characterConfig.m_rightHandState < CharacterConfiguration::eHS_NumOfHandSates && m_characterConfig.m_rightHandState >= 0){
      bspyCharacterVar(hand_state_names[m_characterConfig.m_rightHandState]);}
    else{
      bspyCharacterVar(m_characterConfig.m_rightHandState);}

    bspyCharacterVar(m_minImpactFriction);
    bspyCharacterVar(m_maxImpactFriction);
    bspyCharacterVar(m_applyMinMaxFriction);
#if NM_UPSIDEDOWN_FEET
    bspyCharacterVar(m_leftFootUpsideDown);
    bspyCharacterVar(m_rightFootUpsideDown);
#endif
    bspyCharacterVar(m_spinDamping.somersaultDamp);
    bspyCharacterVar(m_spinDamping.somersaultThresh);
    bspyCharacterVar(m_spinDamping.cartwheelDamp);
    bspyCharacterVar(m_spinDamping.cartwheelThresh);
    bspyCharacterVar(m_spinDamping.vehicleCollisionTime);
    bspyCharacterVar(m_spinDamping.v2);
    bspyCharacterVar(m_spinDamping.vehicleCollisionTimer);
    bspyCharacterVar(m_spinDamping.somersaultDamping);
    bspyCharacterVar(m_spinDamping.cartwheelDamping);

    bspyCharacterVar(m_collision_withVehicle);
    bspyCharacterVar(m_glancingVehicleHit);     
    bspyCharacterVar(m_collision_vehicleClass);
    bspyCharacterVar(m_collision_spin);
    bspyCharacterVar(m_collision_footSlip);
    if (m_collision_spin > 0.0f)
    {
      bspyCharacterVar(m_collision_maxVelocity);
      bspyCharacterVar(m_collision_applyToAll);
      bspyCharacterVar(m_collision_applyToSpine);
      bspyCharacterVar(m_collision_applyToThighs);
      bspyCharacterVar(m_collision_applyToClavicles);
      bspyCharacterVar(m_collision_applyToUpperArms);
    }
 
    // get each part to send update packets
    for (i=0; i<getNumberOfParts(); i++)
      m_parts[i]->sendUpdate();

    // .. and now effectors
    for (i=0; i<getNumberOfEffectors(); i++)
      m_effectors[i]->sendUpdate();

#if NM_EA
    //update patches
    for (i=0; i<NUM_OF_PATCHES; i++)
      PatchSendUpdate(i, spy);
#endif//#if NM_EA
    ////doesn't updateDescription correctly without sending the 1dofs again
    //if (updateDescription)
    //{
    //  for (i=0; i<getNumberOf1Dofs(); i++)
    //    m_1dofEffectors[i]->sendUpdate(i, spy, false);
    //}

    bspyProfileEnd("NmRsCharacter::sendUpdate")
  }
#endif // ART_ENABLE_BSPY

}
