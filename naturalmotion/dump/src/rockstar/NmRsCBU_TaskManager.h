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

#ifndef NM_RS_CBU_TASKMANAGER_H
#define NM_RS_CBU_TASKMANAGER_H

#include "NmRsCBU_Shared.h"
#include "NmRsEngine.h"


#if ART_ENABLE_BSPY
#include "system/timer.h"
#endif // ART_ENABLE_BSPY


namespace ART
{
     class NmRsCharacter;
     class CBUTaskBase;

    /**
     * one CBURecord exists per NmRsCharacter, holding the instances of 
     * the compiled behaviour manager classes
     */
    MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
    NM_ALIGN_PREFIX(16) struct CBURecord
    {
      inline CBURecord() : m_character(0), m_numActiveTasks(0)
      {
        memset(m_tasks, 0, sizeof(CBUTaskBase*) * bvid_Count);
      }

      // character record for this block
      NmRsCharacter  *m_character;

      // list of all the CBU classes available, in same order
      // as the BehaviourEnum - these are the original instances of the task classes
      CBUTaskBase    *m_tasks[bvid_Count];

      // list of active tasks, still in step-order but packed together
      // this 'hot list' is what gets ticked every frame
      CBUTaskBase    *m_activeTasks[bvid_Count + 1];
      unsigned int    m_numActiveTasks;

      // helper functions used to insert and remove tasks from 
      // the m_activeTasks list
      bool activateTask(int bvid);
      bool deactivateTask(int bvid);

    } NM_ALIGN_SUFFIX(16);
    MSVCEndWarningMacroBlock()

    /**
     * The Task Manager is the interface to the C++ ported behaviours, or Compiled Behaviour Units (CBUs)
     * It is responsible for preparing, activating, executing and configuring all
     * the ported behaviours available. One instance exists per instance of NmRsEngine, and the task
     * manager holds records for each character setup in the world.
     */
    class NmRsCBUTaskManager
    {
    public:
      NmRsCBUTaskManager(ART::MemoryManager* services, unsigned int maxCharacters);
      ~NmRsCBUTaskManager();

      /**
       * called by NmRsEngine during initAgent
       */
      void init(ART::AgentID id, NmRsCharacter* character);
      void term(ART::AgentID id);

      void tick(float deltaTime);

      // called when a character is added to/removed from scene
      // so that behaviours can be reset appropriately
      bool addToScene(ART::AgentID id, NmRsCharacter* character);
      bool removeFromScene(ART::AgentID id, NmRsCharacter* character);

      /**
       * bvid from behaviour enum in NmRsCBU_Common.h
       */
      CBUTaskBase* getTaskByID(ART::AgentID id, int bvid) const;
      bool getTaskIsActiveByID(ART::AgentID id, int bvid) const;

      // as the name suggests, call deactivate() on all task entries
      void deactivateAllTasks(ART::AgentID id);

#if ART_ENABLE_BSPY
      void sendDescriptor(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    protected:

      unsigned int                  m_maxCharacters,
                                    m_topCharacterIndex;
      CBURecord                    *m_cbuRecords;

#if ART_ENABLE_BSPY
      rage::sysTimer                m_cbuTimer;
#endif // ART_ENABLE_BSPY

      ART::MemoryManager           *m_artMemoryManager;
    };
}


#endif // NM_RS_CBU_TASKMANAGER_H


