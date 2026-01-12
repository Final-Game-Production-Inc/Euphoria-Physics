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
#include "NmRsCBU_TaskManager.h"
#include "NmRsCharacter.h"
#include "NmRsUtils.h"

#include "NmRsCBU_Yanked.h"
#include "NmRsCBU_StaggerFall.h"
#include "NmRsCBU_Dragged.h"
#include "NmRsCBU_ArmsWindmillAdaptive.h"
#include "NmRsCBU_Shared.h"
#include "NmRsCBU_BodyFoetal.h"
#include "NmRsCBU_BodyWrithe.h"
#include "NmRsCBU_BalancerCollisionsReaction.h"
#include "NmRsCBU_DynamicBalancer.h"
#include "NmRsCBU_BodyBalance.h"
#include "NmRsCBU_Teeter.h"
#include "NmRsCBU_Electrocute.h"
#include "NmRsCBU_BraceForImpact.h"
#include "NmRsCBU_AnimPose.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_Pedal.h"
#include "NmRsCBU_RollUp.h"
#include "NmRsCBU_Flinch.h"
#include "NmRsCBU_SpineTwist.h"
#include "NmRsCBU_RollDownStairs.h"
#include "NmRsCBU_InjuredOnGround.h"
#include "NmRsCBU_Carried.h"
#include "NmRsCBU_Dangle.h"
#include "NmRsCBU_CatchFall.h"
#include "NmRsCBU_Grab.h"
#include "NmRsCBU_HighFall.h"
#include "NmRsCBU_Shot.h"
#include "NmRsCBU_PointArm.h"
#include "NmRsCBU_FallOverWall.h"
#include "NmRsCBU_LearnedCrawl.h"
#include "NmRsCBU_PointGun.h"
#include "NmRsCBU_Stumble.h"
#include "NmRsCBU_ArmsWindmill.h"
#include "NmRsCBU_Buoyancy.h"

#if ALLOW_TRAINING_BEHAVIOURS
#include "NmRsCBU_Landing.h"
#endif

#if ALLOW_DEBUG_BEHAVIOURS
//Debug only behaviours
#include "NmRsCBU_DebugRig.h"
#endif //ALLOW_DEBUG_BEHAVIOURS

#include "art/ARTFeedback.h"


#if ART_ENABLE_BSPY

#define NM_RS_CBU_CREATE(lowerCaseName, FullClassName) \
  if (bSendMemoryUsage) bspyMemoryUsageRecord("CBU::" #FullClassName, sizeof(FullClassName)); \
  ARTCustomPlacementNew(m_cbuRecords[id].m_tasks[bvid_##lowerCaseName], FullClassName);

#else

#define NM_RS_CBU_CREATE(lowerCaseName, FullClassName) \
  ARTCustomPlacementNew(m_cbuRecords[id].m_tasks[bvid_##lowerCaseName], FullClassName);

#endif


#define NM_RS_CBU_DELETE(lowerCaseName, FullClassName) \
  ARTCustomPlacementDelete( ((FullClassName*)m_cbuRecords[id].m_tasks[bvid_##lowerCaseName]), FullClassName);


namespace ART
{
#if 0
  // add the task names into the profiler
  // This list should mirror the ordering of enum BehaviourID in naturalmotion\src\rockstar\NmRsCBU_Shared.h
  static char* getBvidNameSafe(bvid_Count) = 
  {
    "buoyancy",
    "dynamicBalancer",
    "bodyBalance",
    "braceForImpact",
    "learnedCrawl",
    "bodyFoetal",
    "shot",
    "staggerFall",
    "teeter",
    "armsWindmill",
    "armsWindmillAdaptive",
    "balancerCollisionsReaction",            
    "spineTwist",
    "catchFall",
    "injuredOnGround",
    "carried",
    "dangle",
    "yanked",
    "dragged",
    "bodyRollUp",
    "upperBodyFlinch",
    "fallOverWall",
    "highFall",
#if ALLOW_TRAINING_BEHAVIOURS
    "landing",
#endif
    "rollDownStairs",
    "pedalLegs",
    "stumble",
    "grab",
    "animPose",
    "bodyWrithe",
    "pointGun",
    "headLook",
    "pointArm",
    "electrocute",

#if ALLOW_DEBUG_BEHAVIOURS
    //Debug only behaviours
    "debugRig",
#endif //ALLOW_DEBUG_BEHAVIOURS
  };
#endif

  NmRsCBUTaskManager::NmRsCBUTaskManager(ART::MemoryManager* services, unsigned int maxCharacters) : m_maxCharacters(maxCharacters), 
    m_topCharacterIndex(0),
    m_cbuRecords(0),
    m_artMemoryManager(services)
  {
    m_cbuRecords = (CBURecord*)m_artMemoryManager->callocate(sizeof(CBURecord) * m_maxCharacters);
  }

  NmRsCBUTaskManager::~NmRsCBUTaskManager()
  {
    m_artMemoryManager->deallocate(m_cbuRecords);
  }


  CBUTaskBase* NmRsCBUTaskManager::getTaskByID(ART::AgentID id, int bvid) const
  {
    Assert(id < m_maxCharacters);

    if (bvid <= bvid_Invalid || bvid >= bvid_Count)
    {
      NM_RS_LOGERROR("[] NmRsCBUTaskManager::getTaskByID : attempt to get invalid task id = %i", bvid);
      return 0;
    }

    return m_cbuRecords[id].m_tasks[bvid];
  }

  bool NmRsCBUTaskManager::getTaskIsActiveByID(ART::AgentID id, int bvid) const
  {
    CBUTaskBase * task = getTaskByID(id, bvid);
    return (task!=NULL && task->isActive());
  }

  void NmRsCBUTaskManager::init(ART::AgentID id, NmRsCharacter* character)
  {
    Assert(id < m_maxCharacters);
    Assert(m_cbuRecords[id].m_character == 0);

    m_cbuRecords[id].m_character = character;
    character->associateCBUTaskManager(this, &m_cbuRecords[id]);

    m_topCharacterIndex = rage::Max(m_topCharacterIndex, (unsigned int)(id + 1));

#if ART_ENABLE_BSPY
    static bool bSendMemoryUsage = true;
#endif // ART_ENABLE_BSPY

    Assert(character->hasValidBodyIdentifier());

    if (character->isBiped())
    {
      NM_RS_CBU_CREATE(balancerCollisionsReaction,          NmRsCBUBalancerCollisionsReaction);       
      NM_RS_CBU_CREATE(dynamicBalancer,      NmRsCBUDynamicBalancer);       
      NM_RS_CBU_CREATE(bodyBalance,          NmRsCBUBodyBalance);
      NM_RS_CBU_CREATE(braceForImpact,       NmRsCBUBraceForImpact);
      NM_RS_CBU_CREATE(dragged,              NmRsCBUDragged);
      NM_RS_CBU_CREATE(learnedCrawl,         NmRsCBULearnedCrawl);
      NM_RS_CBU_CREATE(bodyFoetal,           NmRsCBUBodyFoetal);
      NM_RS_CBU_CREATE(teeter,               NmRsCBUTeeter);
      NM_RS_CBU_CREATE(electrocute,          NmRsCBUElectrocute);
      NM_RS_CBU_CREATE(spineTwist,           NmRsCBUSpineTwist);
      NM_RS_CBU_CREATE(catchFall,            NmRsCBUCatchFall);
      NM_RS_CBU_CREATE(yanked,               NmRsCBUYanked);
      NM_RS_CBU_CREATE(staggerFall,          NmRsCBUStaggerFall);
      NM_RS_CBU_CREATE(bodyRollUp,           NmRsCBURollUp);
      NM_RS_CBU_CREATE(upperBodyFlinch,      NmRsCBUFlinch);
      NM_RS_CBU_CREATE(fallOverWall,         NmRsCBUFallOverWall);
      NM_RS_CBU_CREATE(shot,                 NmRsCBUShot);
      NM_RS_CBU_CREATE(highFall,             NmRsCBUHighFall);
      NM_RS_CBU_CREATE(rollDownStairs,       NmRsCBURollDownStairs);
      NM_RS_CBU_CREATE(bodyWrithe,           NmRsCBUBodyWrithe);
      NM_RS_CBU_CREATE(pedalLegs,            NmRsCBUPedal);
      NM_RS_CBU_CREATE(armsWindmill,         NmRsCBUArmsWindmill);
      NM_RS_CBU_CREATE(injuredOnGround,      NmRsCBUInjuredOnGround);
      NM_RS_CBU_CREATE(carried,      NmRsCBUCarried);
      NM_RS_CBU_CREATE(dangle,         NmRsCBUDangle);
      NM_RS_CBU_CREATE(armsWindmillAdaptive, NmRsCBUArmsWindmillAdaptive);
      NM_RS_CBU_CREATE(grab,                 NmRsCBUGrab);
      NM_RS_CBU_CREATE(animPose,             NmRsCBUAnimPose);
      NM_RS_CBU_CREATE(headLook,             NmRsCBUHeadLook);
      NM_RS_CBU_CREATE(pointArm,             NmRsCBUPointArm);
      NM_RS_CBU_CREATE(pointGun,             NmRsCBUPointGun);
      NM_RS_CBU_CREATE(stumble,              NmRsCBUStumble);

      NM_RS_CBU_CREATE(buoyancy,             NmRsCBUBuoyancy);
#if ALLOW_TRAINING_BEHAVIOURS
      NM_RS_CBU_CREATE(landing,              NmRsCBULanding);
#endif

#if ALLOW_DEBUG_BEHAVIOURS
      //Debug only behaviours
      NM_RS_CBU_CREATE(debugRig,             NmRsCBUDebugRig);
#endif //ALLOW_DEBUG_BEHAVIOURS
    }
    else
      Assert(false); // removed non-biped support for the moment.

#if ART_ENABLE_BSPY
    if (bSendMemoryUsage)
    {
      bspyMemoryUsageRecord("CBU::NmRsCBUTaskManager", sizeof(NmRsCBUTaskManager) + (sizeof(CBURecord) * m_maxCharacters));
      bSendMemoryUsage = false;
    }
#endif // ART_ENABLE_BSPY
  }

  void NmRsCBUTaskManager::term(ART::AgentID id)
  {
    Assert(id < m_maxCharacters);
    Assert(m_cbuRecords[id].m_character != 0);

    m_cbuRecords[id].m_character = 0;


    NM_RS_CBU_DELETE(pointArm,              NmRsCBUPointArm);
    NM_RS_CBU_DELETE(animPose,              NmRsCBUAnimPose);
    NM_RS_CBU_DELETE(headLook,              NmRsCBUHeadLook);
    NM_RS_CBU_DELETE(grab,                  NmRsCBUGrab);
    NM_RS_CBU_DELETE(armsWindmillAdaptive,  NmRsCBUArmsWindmillAdaptive);
    NM_RS_CBU_DELETE(pedalLegs,             NmRsCBUPedal);
    NM_RS_CBU_DELETE(bodyWrithe,            NmRsCBUBodyWrithe);
    NM_RS_CBU_DELETE(rollDownStairs,        NmRsCBURollDownStairs);
    NM_RS_CBU_DELETE(highFall,              NmRsCBUHighFall);
    NM_RS_CBU_DELETE(shot,                  NmRsCBUShot);
    NM_RS_CBU_DELETE(fallOverWall,          NmRsCBUFallOverWall);
    NM_RS_CBU_DELETE(upperBodyFlinch,       NmRsCBUFlinch);
    NM_RS_CBU_DELETE(bodyRollUp,            NmRsCBURollUp);
    NM_RS_CBU_DELETE(catchFall,             NmRsCBUCatchFall);
    NM_RS_CBU_DELETE(spineTwist,            NmRsCBUSpineTwist);
    NM_RS_CBU_DELETE(bodyFoetal,            NmRsCBUBodyFoetal);
    NM_RS_CBU_DELETE(teeter,                NmRsCBUTeeter);
    NM_RS_CBU_DELETE(injuredOnGround,       NmRsCBUInjuredOnGround);
    NM_RS_CBU_DELETE(carried,               NmRsCBUCarried);
    NM_RS_CBU_DELETE(dangle,                NmRsCBUDangle);
    NM_RS_CBU_DELETE(electrocute,           NmRsCBUElectrocute);
    NM_RS_CBU_DELETE(yanked,                NmRsCBUYanked);
    NM_RS_CBU_DELETE(staggerFall,           NmRsCBUStaggerFall);
    NM_RS_CBU_DELETE(dragged,               NmRsCBUDragged);
    NM_RS_CBU_DELETE(learnedCrawl,          NmRsCBULearnedCrawl);
    NM_RS_CBU_DELETE(braceForImpact,        NmRsCBUBraceForImpact);
    NM_RS_CBU_DELETE(bodyBalance,           NmRsCBUBodyBalance);
    NM_RS_CBU_DELETE(balancerCollisionsReaction,          NmRsCBUBalancerCollisionsReaction);
    NM_RS_CBU_DELETE(dynamicBalancer,       NmRsCBUDynamicBalancer);
    NM_RS_CBU_DELETE(pointGun,              NmRsCBUPointGun);
    NM_RS_CBU_DELETE(stumble,               NmRsCBUStumble);
    NM_RS_CBU_DELETE(armsWindmill,          NmRsCBUArmsWindmill);
    NM_RS_CBU_DELETE(buoyancy,              NmRsCBUBuoyancy);
#if ALLOW_TRAINING_BEHAVIOURS
    NM_RS_CBU_DELETE(landing,               NmRsCBULanding);
#endif

#if ALLOW_DEBUG_BEHAVIOURS
    //Debug only behaviours
    NM_RS_CBU_DELETE(debugRig,              NmRsCBUDebugRig);
#endif //ALLOW_DEBUG_BEHAVIOURS
  }

  bool NmRsCBUTaskManager::addToScene(ART::AgentID id, NmRsCharacter* character)
  {
    Assert(id < m_maxCharacters);
    Assert(m_cbuRecords[id].m_character == character);

    for (int t=0; t<bvid_Count; t++)
    {
      if (m_cbuRecords[id].m_tasks[t])
      {
        Assert(!m_cbuRecords[id].m_tasks[t]->isActive());
        m_cbuRecords[id].m_tasks[t]->init(character, &m_cbuRecords[id]);
      }
    }

    return true;
  }

  bool NmRsCBUTaskManager::removeFromScene(ART::AgentID id, NmRsCharacter* character)
  {
    ((void)character);
    Assert(id < m_maxCharacters);
    Assert(m_cbuRecords[id].m_character == character);

    deactivateAllTasks(id);

    for (int t=0; t<bvid_Count; t++)
    {
      if (m_cbuRecords[id].m_tasks[t])
        m_cbuRecords[id].m_tasks[t]->term();
    }

    return true;
  }

  void NmRsCBUTaskManager::tick(float deltaTime)
  {
#if ART_ENABLE_BSPY
    bspyProfileStart("NmRsCBUTaskManager");
#endif // ART_ENABLE_BSPY

    // tick logic, version 3.0!
    // -------------------------------
    // + each character is ticked in turn
    //  + if a task is encountered that requires multiple ticks, the active character
    //    yields and lets the next character begin ticking
    // + once all characters have run out of tasks, we're done
    //
    // this allows for tasks to be left 'in flight' on platforms where we can use
    // distributed processing, plus it still guarantees correct tick ordering per-character
    // ------------------------------

    unsigned int c;

    // copies of active tasks used during step so mid-step activate/deactivates don't interfere
    CBUTaskBase **activeTasks     = (CBUTaskBase**)alloca(sizeof(CBUTaskBase*) * bvid_Count * m_maxCharacters);
    unsigned int *activeTaskCount = (unsigned int*)alloca(sizeof(unsigned int) * bvid_Count * m_maxCharacters);
    unsigned int *activeTaskIndex = (unsigned int*)alloca(sizeof(unsigned int) * bvid_Count * m_maxCharacters);

    unsigned int taskSlots = 0, activeSlots = 0, tickCycle = 1;
    for (c=0; c<m_topCharacterIndex; ++c)
    {
      // character is valid / ready for stepping / has something to do?
      if (!m_cbuRecords[c].m_character ||
        !m_cbuRecords[c].m_character->isInsertedInScene() ||
        m_cbuRecords[c].m_numActiveTasks == 0)
        continue;

#ifdef NM_PLATFORM_X360
      XMemCpy(&activeTasks[taskSlots * bvid_Count], m_cbuRecords[c].m_activeTasks, sizeof(CBUTaskBase*) * bvid_Count);
#else
      memcpy(&activeTasks[taskSlots * bvid_Count], m_cbuRecords[c].m_activeTasks, sizeof(CBUTaskBase*) * bvid_Count);
#endif
      activeTaskCount[taskSlots] = m_cbuRecords[c].m_numActiveTasks;
      activeTaskIndex[taskSlots] = 0;

      ++ taskSlots;
    }

    // loop and tick while characters are active
    activeSlots = taskSlots;
    while (activeSlots > 0)
    {
      BEGIN_PROFILING_TAGGED("cycle:", tickCycle);
      ++ tickCycle;

      for (c=0; c<taskSlots; c++)
      {
        // consumed all executable tasks for this character?
        if (activeTaskIndex[c] >= activeTaskCount[c])
          continue;

        BEGIN_PROFILING_TAGGED("char:", c);
        CBUTaskBase **taskList = &activeTasks[c * bvid_Count];

        Assert(taskList[activeTaskIndex[c]]);
        // tick until we hit a task that requires multiple ticks;
        // at that point, yield to other characters
        while (1)
        {
          BEGIN_PROFILING(getBvidNameSafe(taskList[activeTaskIndex[c)]->getBvID()]);
          CBUTaskReturn ret;
          NmRsCharacter *character = taskList[activeTaskIndex[c]]->getCharacter();

#if ART_ENABLE_BSPY
          if (character->getArticulatedWrapper())
            bSpyServer::inst()->setLastSeenAgent(character->getBSpyID());
#endif // ART_ENABLE_BSPY

          if (character->getArticulatedBody())
          {
            rage::phSleep *sleep = character->getArticulatedWrapper()->getArticulatedCollider()->GetSleep();
            //sleep->Reset();
            if (sleep && sleep->IsAsleep())
              ret = eCBUTaskComplete;
            else
            {
              CBUTaskBase* actTask = taskList[activeTaskIndex[c]];
              Assert(actTask);

#if ART_ENABLE_BSPY
              bool bspy_sendTaskData = ART::bSpyServer::inst() &&
              ART::bSpyServer::inst()->isClientConnected() &&
              ART::bSpyServer::inst()->shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_Tasks);

              const char* taskName = getBvidNameSafe(actTask->getBvID());


              BSPY_ISCLIENTCONNECTED_BEGIN()
              if (actTask->isActive())
              {
                if (bspy_sendTaskData)
                {
                  bSpy::SelectAgentPacket sap((bs_int16)character->getID());
                  bspySendPacket(sap);

                  bSpy::TaskBeginTaskPacket btp((bs_uint8)actTask->getBvID());
                  bspySendPacket(btp);
                }

                bspyProfileStart(taskName);
              }
              BSPY_ISCLIENTCONNECTED_END()
#endif // ART_ENABLE_BSPY

#if ART_ENABLE_BSPY
              if (character->getArticulatedWrapper())
                character->setCurrentBehaviour(actTask->getBvID());
#endif
              ret = actTask->tick(deltaTime);

#if ART_ENABLE_BSPY
              //Output the behaviour's state to bSpy directly after ticking it. 
              //  This gives the correct state.  (If you do it after all the behaviours tick then later ticking behaviours may 
              //  set the parameters of an earlier behaviour leading bSpy to lie about the earlier behaviours state)
              //Going against common sense - Just because it ticks doesn't mean that it is active after the tick:
              //We check for whether it is active as the dynamicBalancer can choose to deactivate at the start of it's tick/OnTick
              //  due to it having a request deactivate structure (to maintain it's state if it is deactivated then reactivated in the same/next frame)
              //
              //NOTE:  bspy doesn't currently allow distributed tasks but if it did...
              //For distributed tasks (only the dynamicBalancer currently - the state is output after the first tickPhase
              //  This means that the 1st phase should load all the state and the next phases should use this state without changing it
              //  otherwise the state output here could be wrong.  
              //  The dynamicBalancer output almost matches this model
              //  TODO: move the changable state to be output after each tickPhase //mmmmtodo
              //  TODO: move the changable state e.g. uprightContraint to after when it is applied//mmmmtodo
              BSPY_ISCLIENTCONNECTED_BEGIN()
              if (actTask->isActive())
              {
                bspyProfileEnd(taskName);
                if (bspy_sendTaskData)
                {
                  NmRsSpy& spy = *character->getEngine()->getNmRsSpy();

                  actTask->sendParameters(spy);

                  bSpy::TaskEndTaskPacket etp((bs_uint8)actTask->getBvID());
                  bspySendPacket(etp);
                }
              }
              BSPY_ISCLIENTCONNECTED_END()
#endif//ART_ENABLE_BSPY

#if defined(NM_RS_VALIDATE_VITAL_VALUES) && ART_ENABLE_BSPY
              if (character->getArticulatedWrapper())
                character->setCurrentBehaviour(bvid_Invalid);
#endif
            }
          }
          else
            ret = eCBUTaskComplete;
          END_PROFILING();

#if ART_ENABLE_BSPY
          if (character->getArticulatedWrapper())
            bSpyServer::inst()->clearLastSeenAgent();
#endif // ART_ENABLE_BSPY

          if (ret == eCBUTaskMoreTicksRequired)
            break;

          ++ activeTaskIndex[c];

          if (activeTaskIndex[c] >= activeTaskCount[c])
          {
            // signal that we're done; when this counter hits 0, the update is finished
            -- activeSlots;
            break;
          }
        }

        END_PROFILING();
      }
      END_PROFILING();
    }

#if ART_ENABLE_BSPY
    bspyProfileEnd("NmRsCBUTaskManager");
#endif // ART_ENABLE_BSPY
  }

  void NmRsCBUTaskManager::deactivateAllTasks(ART::AgentID id)
  {
    Assert(id < m_maxCharacters);
    for (int i=0; i<bvid_Count; i++)
    {
      if (m_cbuRecords[id].m_tasks[i])
        m_cbuRecords[id].m_tasks[i]->deactivate();
    }
  }
#if 0
  void CBUTaskBase::init(NmRsCharacter* character, CBURecord* cbuParent)
  {
    Assert(m_character == 0);
    m_active = false;
    m_character = character;
    m_cbuParent = cbuParent;
    initialiseCustomVariables();
  }

  void CBUTaskBase::activate()
  {
    ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      // fill in feedback structure
      strcpy(feedback->m_behaviourName, getBvidNameSafe(this->getBvID()));
#if ART_ENABLE_BSPY
      char calledByString[ART_FEEDBACK_BHNAME_LENGTH];
      // jrp i can't figure out how to get the bspy string back to print the sub behaviour.
      // todo add feedback bspy packet to replace this shonky nonsense.
      sprintf(calledByString, "%s", getBvidNameSafe(m_character->m_currentBehaviour));
      strcpy(feedback->m_calledByBehaviourName, calledByString);
#endif
      feedback->m_argsCount = 0;
      feedback->m_agentID = m_character->getID();
      feedback->onBehaviourStart();
    }

    if (m_cbuParent->activateTask(getBvID()))
    {
#if ART_ENABLE_BSPY
      BehaviourID currentBehaviour = m_character->m_currentBehaviour;
      m_character->setCurrentBehaviour(getBvID());
      m_updatePhase = kActivate;
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]);
#endif
      if(!m_active)
      {
        m_active = true;
      }
      m_cbuParent->m_tasks[getBvID()]->onActivate();
#if ART_ENABLE_BSPY
      m_character->setCurrentBehaviour(currentBehaviour);
#endif
    }
  }

  CBUTaskReturn CBUTaskBase::tick(float timeStep)
  {
    if(m_active)
    {
#if ART_ENABLE_BSPY
      m_updatePhase = kTick;
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]);
#endif
      return m_cbuParent->m_tasks[getBvID()]->onTick(timeStep);
    }
    return eCBUTaskComplete;
  }

  void CBUTaskBase::deactivate()
  {
    if (m_cbuParent->deactivateTask(getBvID()))
    {
#if ART_ENABLE_BSPY
      BehaviourID currentBehaviour = m_character->m_currentBehaviour;
      m_character->setCurrentBehaviour(getBvID());
      m_updatePhase = kDeactivate;
      m_character->setCurrentSubBehaviour(s_phaseNames[m_updatePhase]);
#endif
      if(m_active)
      {
        m_active = false;//must be set to false before onDeactivate otherwise balancer might not shut down
        m_cbuParent->m_tasks[getBvID()]->onDeactivate();
      }
#if ART_ENABLE_BSPY
      m_character->setCurrentBehaviour(currentBehaviour);
#endif
    }

    ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      // fill in feedback structure
      strcpy(feedback->m_behaviourName, getBvidNameSafe(this->getBvID()));
#if defined(NM_RS_VALIDATE_VITAL_VALUES) || ART_ENABLE_BSPY
      char calledByString[ART_FEEDBACK_BHNAME_LENGTH];
      // todo correctly support sub behaviour feedback?
      sprintf(calledByString, "%s", getBvidNameSafe(m_character->m_currentBehaviour));
      strcpy(feedback->m_calledByBehaviourName, calledByString);
#endif
      feedback->m_argsCount = 0;
      feedback->m_agentID = m_character->getID();
      feedback->onBehaviourFinish();
    }
  }

  void CBUTaskBase::term()
  {
    // ensure the task is deactivated before we terminate
    Assert(!isActive());

    // null out the appropriate pointers to flag
    // ourselves as terminated
    m_character = 0;
    m_cbuParent = 0;
  }

#if ART_ENABLE_BSPY
  void CBUTaskBase::sendParameters(NmRsSpy& /*spy*/)
  {
    bspyTaskVar(m_bvid, false);
    bspyTaskVar(m_active, false);
    bspyTaskVar_Bitfield32(m_mask, false);
  }
#endif // ART_ENABLE_BSPY
#endif

  // !hdd! eliminate function overhead by making m_activeTasks of type BehaviourID...?

  bool CBURecord::activateTask(int bvid)
  {
    Assert(m_numActiveTasks < bvid_Count);

    // easy out on no active tasks, just pop in the front
    if (m_numActiveTasks == 0)
    {
      m_activeTasks[0] = m_tasks[bvid];
      m_numActiveTasks ++;
      return true;
    }

    // go look for insertion point
    unsigned int i = 0;
    while (i < m_numActiveTasks &&
      m_activeTasks[i]->getBvID() < bvid)
    {
      i++;
    }

    // bail if already active
    if (m_activeTasks[i] && m_activeTasks[i]->getBvID() == bvid)
      return false;

    // shuffle list right one entry
    for (unsigned int j=m_numActiveTasks; j>i; j--)
    {
      m_activeTasks[j] = m_activeTasks[j-1];
    }

    // and insert
    m_activeTasks[i] = m_tasks[bvid];

    m_numActiveTasks ++;
    return true;
  }

  bool CBURecord::deactivateTask(int bvid)
  {
    unsigned int i = 0;

    // find which index the task is in
    while (i < m_numActiveTasks &&
      m_activeTasks[i]->getBvID() != bvid)
      i++;

    // cannot find task
    if (i >= m_numActiveTasks)
      return false;

    // copy the rest of the list one entry to the left
    for (; i<m_numActiveTasks; i++)
      m_activeTasks[i] = m_activeTasks[i+1];

    m_numActiveTasks--;
    return true;
  }

#if ART_ENABLE_BSPY

  void NmRsCBUTaskManager::sendDescriptor(NmRsSpy& spy)
  {
    bSpy::TaskDescriptorPacket tmd(bvid_Count);

    // request string tokens for all the task names, get
    // them transmitted to the host
    for (int i=0; i<(int)bvid_Count; i++)
    {
      tmd.m_taskName[i] = spy.getTokenForString(getBvidNameSafe((BehaviourID)i));
    }

    bspySendPacket(tmd);
  }

#endif // ART_ENABLE_BSPY

}
