#include "nmutils/NMCustomMemory.h"

namespace NMutils
{
  MemoryConfiguration::MemoryConfiguration(
    NMCustomMemoryAllocator alloc,
    NMCustomMemoryAllocator calloc,
    NMCustomMemoryDeallocator dealloc,
    NMCustomMemoryReallocator realloc) : 
  m_allocator(alloc), m_callocator(calloc), m_deallocator(dealloc), m_reallocator(realloc), m_userData(0)
  {
  }
}
