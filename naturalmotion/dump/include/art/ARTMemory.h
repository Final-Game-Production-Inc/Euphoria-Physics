#ifndef NM_ART_MEMORY_H
#define NM_ART_MEMORY_H

#include "art/ARTMemoryManager.h"
#include <stddef.h>

#define ARTCustomPlacementNew(loc, type) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(m_artMemoryManager);}

#define ARTCustomPlacementDelete(loc, type) do { if (loc) {\
  loc->~type(); \
  m_artMemoryManager->deallocate(loc, NM_MEMORY_TRACKING_ARGS);} } while (0)

#define ARTCustomPlacementNew1Arg(loc, type, arg1) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(m_artMemoryManager, arg1);}

#define ARTCustomPlacementNew2Arg(loc, type, arg1, arg2) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(m_artMemoryManager, arg1, arg2);}

#define ARTCustomPlacementNew3Arg(loc, type, arg1, arg2, arg3) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(m_artMemoryManager, arg1, arg2, arg3);}

#define ARTCustomPlacementNew4Arg(loc, type, arg1, arg2, arg3, arg4) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(m_artMemoryManager, arg1, arg2, arg3, arg4);}

#define ARTCustomPlacementNew5Arg(loc, type, arg1, arg2, arg3, arg4, arg5) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(m_artMemoryManager, arg1, arg2, arg3, arg4, arg5);}

#define ARTCustomPlacementNew6Arg(loc, type, arg1, arg2, arg3, arg4, arg5, arg6) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(m_artMemoryManager, arg1, arg2, arg3, arg4, arg5, arg6);}

#define ARTCustomPlacementNew7Arg(loc, type, arg1, arg2, arg3, arg4, arg5, arg6, arg7) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(m_artMemoryManager, arg1, arg2, arg3, arg4, arg5, arg6, arg7);}

#define ARTCustomPlacementNew8Arg(loc, type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(m_artMemoryManager, arg1, arg2, arg3, arg4, arg5, arg6, arg7,arg8);}

#define ARTCustomPlacementNew1ArgMConfig(loc, type, arg1) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(arg1, m_artMemoryManager->getConfig());}

#define ARTCustomPlacementNew2ArgMConfig(loc, type, arg1, arg2) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(arg1, arg2, m_artMemoryManager->getConfig());}

#define ARTCustomPlacementNew4ArgMConfig(loc, type, arg1, arg2, arg3, arg4) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(arg1, arg2, arg3, arg4, m_artMemoryManager->getConfig());}

#define ARTCustomPlacementNewNoService(loc, type) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type;}

#define ARTCustomPlacementNew1ArgNoService(loc, type, arg1) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(arg1);}

#define ARTCustomPlacementNew2ArgNoService(loc, type, arg1, arg2) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(arg1, arg2);}

#define ARTCustomPlacementNew3ArgNoService(loc, type, arg1, arg2, arg3) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(arg1, arg2, arg3);}

#define ARTCustomPlacementNew4ArgNoService(loc, type, arg1, arg2, arg3, arg4) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(arg1, arg2, arg3, arg4);}

#define ARTCustomPlacementNew5ArgNoService(loc, type, arg1, arg2, arg3, arg4, arg5) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(arg1, arg2, arg3, arg4, arg5);}

#define ARTCustomPlacementNew6ArgNoService(loc, type, arg1, arg2, arg3, arg4, arg5, arg6) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(arg1, arg2, arg3, arg4, arg5, arg6);}

#define ARTCustomPlacementNew7ArgNoService(loc, type, arg1, arg2, arg3, arg4, arg5, arg6, arg7) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7);}

#define ARTCustomPlacementNew8ArgNoService(loc, type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) {\
  void* const _at_ = m_artMemoryManager->allocate(sizeof(type), NM_MEMORY_TRACKING_ARGS);\
  loc = new (_at_) type(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);}


#endif // NM_ART_MEMORY_H

