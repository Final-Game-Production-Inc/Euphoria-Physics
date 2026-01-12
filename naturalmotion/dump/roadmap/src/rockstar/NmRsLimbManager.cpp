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
#include "NmRsLimbManager.h"
#include "NmRsCharacter.h"

namespace ART
{
  //
  // NmRsLimbInputQueue
  //
  // Sortable array to accumulate message inputs
  //---------------------------------------------------------------------------

  NmRsLimbInputQueue::NmRsLimbInputQueue()
  {
    clear();
  }

  void NmRsLimbInputQueue::clear()
  {
    m_index = 0;
    for(int i = 0; i < MAX_INPUTS_PER_LIMB; ++i)
    {
      m_data[i].valid = false;
      m_ptrs[i] = &m_data[i];
    }
  }

  void NmRsLimbInputQueue::add(const NmRsLimbInput& v)
  {
    Assert(m_index < MAX_INPUTS_PER_LIMB);

    if(m_index < MAX_INPUTS_PER_LIMB)
    {
      Assert(v.valid && v.dataValid);
      m_data[m_index++] = v;
    }
  }

  void NmRsLimbInputQueue::sort()
  {
    // Simple insertion sort. Optimal here because the most common case
    // is an already sorted array and no swaps are necessary.
    int i, j;
    NmRsLimbInput* key;
    for(j = 1; j < (int)m_index; j++)
    {
      key = m_ptrs[j];
      for(i = j - 1; (i >= 0) && (*m_ptrs[i] < *key); i--)
      {
        m_ptrs[i+1] = m_ptrs[i];
      }
      m_ptrs[i+1] = key;
    }
  }

  //
  // NmRsLimbManager
  //
  // Manages memory for input data...
  //---------------------------------------------------------------------------
  NmRsLimbManager::NmRsLimbManager(MemoryManager* manager, int maxNumCharacters)
    : m_memoryManager(manager)
  {
    // allocate some shared memory for limb input data
    const unsigned int size = MAX_INPUT_MEMORY_PER_AGENT * maxNumCharacters;

    // This defines the space reserved for potential pool overflow.
    // I have observed a maximum overrun of 1200 to date.  3000 seems safe for now.
    const unsigned int reservedSize = 3000;

    m_pStart = (char *)m_memoryManager->callocate(size + reservedSize, NM_MEMORY_TRACKING_ARGS);
    m_pEnd = m_pStart + size + reservedSize;

    // Note: do not re-align m_pStart, only adjust m_pCurrent.  The start ptr must be kept as-is for deallocation.
    m_pCurrent = alignPtr(m_pStart);

    m_pReserved = alignPtr(m_pEnd - reservedSize);

#if ART_ENABLE_BSPY
    m_memUsed = 0;
    m_memAvailable = size;
#endif
  }

  NmRsLimbManager::~NmRsLimbManager()
  {
    m_memoryManager->deallocate(m_pStart, NM_MEMORY_TRACKING_ARGS);
  }

  void NmRsLimbManager::tick()
  {
#if ART_ENABLE_BSPY
    m_memUsed = rage::Max((int)m_pCurrent - (int)m_pStart, (int)m_memUsed);
    bspyLogf(info, L"limb manager memory used %d/%d", (int)m_memUsed, (int)m_memAvailable);
#endif

#if 0
    Displayf("limb manager memory used %d/%d", (int)(m_pCurrent-m_pStart), (int)(m_pReserved-m_pStart));
#endif

    m_pCurrent = alignPtr(m_pStart);
  }

  void NmRsLimbManager::checkForOverflow()
  {
    // We should have already invalidated any limb inputs using this memory.
    if(m_pCurrent >= m_pReserved)
    {
#if ART_ENABLE_BSPY
      bspyLogf(error, L"limbs memory budget exceeded by (at least) %u/%u", m_pCurrent - m_pReserved, m_pEnd - m_pReserved);
#endif
      Errorf("limbs memory budget exceeded by (at least) %u/%u", m_pCurrent - m_pReserved, m_pEnd - m_pReserved);

      // If this assert is blowing, either 1) increase MAX_INPUT_MEMORY_PER_AGENT
      // or 2) review your behaviour useage for ways to reduce messaging.
      Assert(false);

      m_pCurrent = alignPtr(m_pReserved);
    }
  }

} // namespace ART
