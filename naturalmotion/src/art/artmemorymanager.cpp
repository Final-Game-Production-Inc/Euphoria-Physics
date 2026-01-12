#include "art/ARTInternal.h"

namespace ART
{
  MemoryManager::MemoryManager(NMutils::MemoryConfiguration *memConfig)
    : m_config(memConfig)
  {
  }

  MemoryManager::~MemoryManager()
  {
  }

  void *MemoryManager::allocate(size_t size, NM_MEMORY_TRACKING_ARGS_DECL)
  {
    FastAssert(m_config->m_allocator);
    return m_config->m_allocator(size, m_config->m_userData, NM_MEMORY_TRACKING_ARGS_PARAM);
  }

  void *MemoryManager::callocate(size_t size, NM_MEMORY_TRACKING_ARGS_DECL)
  {
    FastAssert(m_config->m_callocator);
    return m_config->m_callocator(size, m_config->m_userData, NM_MEMORY_TRACKING_ARGS_PARAM);
  }

  void MemoryManager::deallocate(void *mPtr, NM_MEMORY_TRACKING_ARGS_DECL)
  {
    FastAssert(m_config->m_deallocator);
    m_config->m_deallocator(mPtr, m_config->m_userData, NM_MEMORY_TRACKING_ARGS_PARAM);
  }

  void *MemoryManager::reallocate(void* oldptr, size_t size, NM_MEMORY_TRACKING_ARGS_DECL)
  {
    FastAssert(m_config->m_reallocator);
    return m_config->m_reallocator(oldptr, size, m_config->m_userData, NM_MEMORY_TRACKING_ARGS_PARAM);
  }

}
