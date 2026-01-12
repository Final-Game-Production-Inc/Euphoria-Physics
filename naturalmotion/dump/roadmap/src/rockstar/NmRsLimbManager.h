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

#ifndef NM_RS_LIMB_MANAGER_H
#define NM_RS_LIMB_MANAGER_H

#include "NmRsLimbInputWrapper.h"

#define MAX_INPUTS_PER_LIMB 12
#define MAX_INPUTS (MAX_INPUTS_PER_LIMB * 5)

// Determines the pool size for limb input messages. Tune such that we use as
// little as possible witout overrunning into reserved memory.
#define MAX_INPUT_MEMORY_PER_AGENT 3000

namespace ART
{
  //
  // NmRsLimbInput
  // Effectively a packet header for limb input messages.that indicates the type
  // of message carried as well as blend and sort priority information. m_data
  // points to a data structure of the appropriate type.
  //---------------------------------------------------------------------------
  struct NmRsLimbInput
  {
    NmRsLimbInput() {
      valid = false;
    }

    template <typename T>
    T* getData()
    {
      Assert(T::TYPE == type);
      return (T*)data;
    }

    NmRsInputWrapperBase* getBase()
    {
      return (NmRsInputWrapperBase*)data;
    }

    NmRsLimbInputType type;        // Type of input request - IK, pose, etc..
    float             weight;      // Blend weight
    int               priority;    // Input priority / blend weight
    int               subPriority; // Used to control layering of inputs *within* behaviours.
    BehaviourMask     mask;

#if ART_ENABLE_BSPY_LIMBS
    BehaviourID       task;   // Task that sent this message
    const char*       subTask;// Text to differentiate multiple messages from the same task.
#endif

    void *            data;   // Pointer to data particular to input type
    unsigned int      dataSize;

    bool              valid;    // Should this message be posted?
    bool              dataValid;// false if data for this message was pulled from the overrun failsafe area of the pool.

    // Supports sorting of vectors of limb inputs in order of *descending* priority.
    int operator<(const NmRsLimbInput &rhs) const
    {
      if(this->priority < rhs.priority)
      {
        return 1;
      }
      else if(this->priority == rhs.priority && this->subPriority < rhs.subPriority)
      {
        return 1;
      }
      return 0;
    }
  };

  class NmRsLimbInputQueue
  {
  public:

    NmRsLimbInputQueue();
    
    void clear();
    void add(const NmRsLimbInput& v);

    inline const unsigned int size() const { return m_index; }
    inline NmRsLimbInput* get(unsigned int index) { Assert(m_ptrs[index]); return m_ptrs[index]; }

    void sort();

  protected:

    NmRsLimbInput  m_data[MAX_INPUTS_PER_LIMB];
    NmRsLimbInput* m_ptrs[MAX_INPUTS_PER_LIMB];

    unsigned int m_index;
  };


  //
  // NmRsLimbManager
  // 
  // Manages memory for input data.
  //--------------------------------------------------------------------------

#if ART_ENABLE_BSPY_LIMBS
#define DEBUG_SETBY(setBy, setBySub) , setBy, setBySub
#else
#define DEBUG_SETBY
#endif

  class NmRsLimbManager
  {
  public:

    NmRsLimbManager(ART::MemoryManager* manager, int maxNumCharacters);
    ~NmRsLimbManager();

    template <typename T>
#if ART_ENABLE_BSPY_LIMBS
    inline NmRsLimbInput createNmRsLimbInput(int priority, int subPriority, BehaviourMask mask = bvmask_Full, float weight = 1.0f, BehaviourID task = bvid_Invalid, const char* subTask = 0)
#else
    inline NmRsLimbInput createNmRsLimbInput(int priority, int subPriority, BehaviourMask mask = bvmask_Full, float weight = 1.0f)
#endif
    {
#if __WIN32
      // we need, NAY DEMAND, that T is derived from NmRsInputWrapperBase
      CompileTimeAssert(__is_base_of(NmRsInputWrapperBase, T));
#endif // __WIN32

      NmRsLimbInput input;

      input.priority = priority;
      input.subPriority = subPriority;
      input.mask = mask;
      input.weight = weight;
      input.type = T::TYPE;
      input.weight = weight;
      input.dataSize = sizeof(T);
#if ART_ENABLE_BSPY_LIMBS
      input.task = task;
      input.subTask = subTask;
#endif

      T* newData = (T*)m_pCurrent;
      new(newData) T();
      m_pCurrent = alignPtr(m_pCurrent + sizeof(T));
      Assert(m_pCurrent <= m_pEnd);

      input.data = newData;

      // If current has exceeded our failsafe boundary, mark the resulting
      // input as invalid. We are potentially going to overwrite its data.
      input.dataValid = (m_pCurrent < m_pReserved);

      input.valid = true;


      if(!input.dataValid)
      {
        Displayf("limbs memory budget exceeded by (at least) %u/%u", m_pCurrent - m_pReserved, m_pEnd - m_pReserved);
#if ART_ENABLE_BSPY_LIMBS
        bspyLogf(warning, L"limbs memory budget exceeded by (at least) %u/%u", m_pCurrent - m_pReserved, m_pEnd - m_pReserved);
#endif
    }

      return input;
    }

    // We reserve some of the memory pool as a failsafe against overrunning.
    // When a message is requested that passes the boundary marked by m_pReserved,
    // we grant the placement, but mark the input as invalid, preventing posting.
    // After the body has attempted to post its inputs, this function should be
    // called to reset m_pCurrent to m_pReserved so the next ticking task will
    // use the failsafe memory as well. Once the preStep is complete, and the pool
    // reset, message data placement should proceed as normal.
    void checkForOverflow();

    void tick();

  protected:
    inline char* alignPtr(char* ptr)
    {
      const ptrdiff_t ALIGNMENT = 16;
      const ptrdiff_t align = ALIGNMENT - 1;
      return (char*)(((ptrdiff_t)ptr + align) & ~align);
    }

    MemoryManager * m_memoryManager;

    char* m_pStart;
    char* m_pCurrent;
    char* m_pEnd;

    char* m_pReserved;

#if ART_ENABLE_BSPY
    unsigned int m_memUsed;
    unsigned int m_memAvailable;
#endif
  };

} // namespace ART

#endif // NM_RS_LIMB_MANAGER_H
