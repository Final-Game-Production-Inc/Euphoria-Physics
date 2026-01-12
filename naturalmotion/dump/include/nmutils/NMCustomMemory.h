#ifndef NM_CUSTOM_MEMORY_H
#define NM_CUSTOM_MEMORY_H

#include "nmutils/NMTypes.h"

#if defined(NM_PLATFORM_CELL_PPU) || defined(NM_PLATFORM_WII)
#include <stdlib.h>
#endif // NM_PLATFORM_CELL_PPU || NM_PLATFORM_WII

namespace NMutils
{
  #define NM_MEMORY_TRACKING_ARGS_DECLDEF	const char* mt_fileName = 0, const int mt_lineNumber = 0, const char* mt_functionName = 0
  //#define NM_MEMORY_TRACKING_ARGS_DECLDEF	const char* mt_fileName, const int mt_lineNumber, const char* mt_functionName
  #define NM_MEMORY_TRACKING_ARGS_DECL	const char* mt_fileName, const int mt_lineNumber, const char* mt_functionName
  #define NM_MEMORY_TRACKING_ARGS_DECL_UNUSED	const char* , const int , const char* 
  #define NM_MEMORY_TRACKING_ARGS_PARAM	mt_fileName, mt_lineNumber, mt_functionName
#if __FINAL
  #define NM_MEMORY_TRACKING_ARGS			NULL, 0, NULL
#else
  #define NM_MEMORY_TRACKING_ARGS			__FILE__, __LINE__, __FUNCTION__
#endif
  typedef void*(*NMCustomMemoryAllocator)(size_t mSize, void* userData, NM_MEMORY_TRACKING_ARGS_DECL);
  typedef void(*NMCustomMemoryDeallocator)(void *mPtr, void* userData, NM_MEMORY_TRACKING_ARGS_DECL);
  typedef void*(*NMCustomMemoryReallocator)(void *oldPtr, size_t mSize, void* userData, NM_MEMORY_TRACKING_ARGS_DECL);

  /**
   * \brief Memory configuration Options
   *
   * %Allows use of custom allocators in the appropriate classes.
   */
  struct MemoryConfiguration
  {
    MemoryConfiguration(
      NMCustomMemoryAllocator alloc,
      NMCustomMemoryAllocator calloc,
      NMCustomMemoryDeallocator dealloc,
      NMCustomMemoryReallocator realloc);

    NMCustomMemoryAllocator          m_allocator;
    NMCustomMemoryAllocator          m_callocator;
    NMCustomMemoryDeallocator        m_deallocator;
    NMCustomMemoryReallocator        m_reallocator;

    // this is passed to each function
    // and can be used for any purpose
    void  *m_userData;
  };
}

#endif // NM_CUSTOM_MEMORY_H
