#ifndef NM_ART_MEMORY_MANAGER_H
#define NM_ART_MEMORY_MANAGER_H

namespace ART
{

  /**
   * \brief ART memory management system
   *
   * The %MemoryManager is used by the ARTContext to allocate and free
   * components in specific managed and tracked heaps.
   */
  class MemoryManager
  {
  public:
    MemoryManager(NMutils::MemoryConfiguration *memConfig);
    virtual ~MemoryManager();

    void *allocate(size_t size, NM_MEMORY_TRACKING_ARGS_DECLDEF);
    void *callocate(size_t size, NM_MEMORY_TRACKING_ARGS_DECLDEF);
    void deallocate(void *mPtr, NM_MEMORY_TRACKING_ARGS_DECLDEF);
    void *reallocate(void* oldptr, size_t size, NM_MEMORY_TRACKING_ARGS_DECLDEF);

    const NMutils::MemoryConfiguration *getConfig() const { return m_config; }

  protected:

    NMutils::MemoryConfiguration *m_config;   // copy of memory config params
  };


}

#endif // NM_ART_MEMORY_MANAGER_H

