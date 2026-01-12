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

#define MAX_INPUTS_PER_LIMB  18

// Change this when new body types are added.
#define MAX_LIMBS 5

// Increase if assert in pushBodyState() is triggering.
#define MAX_BODY_INPUTS 5

// Determines the pool size for limb input messages. Tune such that we use as
// little as possible without overrunning into reserved memory.
#define MAX_INPUT_MEMORY_PER_AGENT 3200

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
      init();
    }

    void init() {
      type = kLimbInputInvalid;
      weight = 0.0f;
      priority = 0;
      subPriority = 0;
      mask = bvmask_None;
#if ART_ENABLE_BSPY_LIMBS
      task = bvid_Invalid;
      subTask = "invalid";
#endif
      data = 0;
      dataSize = 0;
      valid = false;
      dataValid = false;
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

  // Separate structure contains set of default inputs for a given behaviour or
  // process.  Moved from NmRsBody so we can put it in a stack and handle nested
  // behaviour states.
  struct NmRsBodyState
  {
    int            m_priority;
    int            m_subPriority;
    BehaviourID    m_bvid;
    BehaviourMask  m_mask;
    float          m_blend;
    const char*    m_subTask;

    NmRsLimbInput  m_allLimbInputs[MAX_LIMBS];
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

    friend class NmRsLimb;

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

      if(alignPtr(m_pCurrent + sizeof(T)) <= m_pEnd)
      {
        // Normal operation. Place input data in managed memory.
        //
        T* newData = (T*)m_pCurrent;
        new(newData) T();
        m_pCurrent = alignPtr(m_pCurrent + sizeof(T));
        Assert(m_pCurrent <= m_pEnd);
#if ART_ENABLE_BSPY & 0
        bspyLogf(info, L"limb manager create input %d/%d", (int)m_pCurrent - (int)m_pStart, (int)m_memAvailable);
#endif
        input.data = newData;
        input.dataValid = true;
      }
      else
      {
        // Overflow. Assign address of appropriate dummy data and flag as invalid
        //
        switch(input.type)
        {
        case kIk:
          input.data = &m_dummyIK;
          break;
        case kArmPose:
          input.data = &m_dummyArm;
          break;
        case kLegPose:
          input.data = &m_dummyLeg;
          break;
        case kSpinePose:
          input.data = &m_dummySpine;
          break;
        case kSetStiffness:
          input.data = &m_dummyStiffness;
          break;
        case kStopAll:
          input.data = &m_dummyStopAll;
          break;
        case kLimbInputInvalid:
        default:
          input.data = 0;
          Assert(false);
          break;
        }
        input.dataValid = false;
#if ART_ENABLE_BSPY
        bspyLogf(info, L"limb manager overflow. dummy data assigned for type %u", input.type);
#endif
        Displayf("limb manager overflow. dummy data assigned for type %u", input.type);
      }

      input.valid = true;

      return input;
    }

    void tick();

    void reset();

    int getActiveBodyInput() { return m_activeBodyInput; }

    NmRsBodyState* pushBodyState()
    {
      ++m_activeBodyInput;

      // We should not normally be exceeding MAX_BODY_INPUTS. If this is
      // happening regularly, it need to be increased (or excessive nested
      // activation may need to be addressed).
      Assert(m_activeBodyInput >= 0 && m_activeBodyInput < MAX_BODY_INPUTS);
      if(m_activeBodyInput >= MAX_BODY_INPUTS)
      {
        NmRsBodyState input;
        m_bodyInputs.PushAndGrow(input);
#if ART_ENABLE_BSPY
        bspyLogf(warning, L"getNextBodyInput m_activeBodyInput >= MAX_BODY_INPUTS");
#endif
      }
#if ART_ENABLE_BSPY & 0
      bspyLogf(info, L"push(%d)", m_activeBodyInput);
#endif

      return &m_bodyInputs[m_activeBodyInput];
    }

    NmRsBodyState* popBodyState()
    {
      m_activeBodyInput = rage::Max(--m_activeBodyInput, -1);
#if ART_ENABLE_BSPY & 0
      bspyLogf(info, L"pop(%d)", m_activeBodyInput);
#endif
      if(m_activeBodyInput < 0)
        return NULL;
      return &m_bodyInputs[m_activeBodyInput];
    }

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

    int m_activeBodyInput;
    rage::atArray<NmRsBodyState> m_bodyInputs;

#if ART_ENABLE_BSPY
    unsigned int m_memUsed;
    unsigned int m_memAvailable;
#endif

  private:

    // A dummy instance of each limb input type to be used as reserved memory
    //
    NmRsArmInputWrapper           m_dummyArm;
    NmRsLegInputWrapper           m_dummyLeg;
    NmRsSpineInputWrapper         m_dummySpine;
    NmRsIKInputWrapper            m_dummyIK;
    NmRsSetStiffnessInputWrapper  m_dummyStiffness;
    NmRsStopAllInputWrapper       m_dummyStopAll;

  };

} // namespace ART

#endif // NM_RS_LIMB_MANAGER_H
