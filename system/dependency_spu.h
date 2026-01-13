// 
// system/dependency_spu.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_DEPENDENCY_SPU_H
#define SYSTEM_DEPENDENCY_SPU_H

#include "dependency_config.h"

#include "math/amath.h"
#include "system/criticalsection.h"
#include "system/dma.h"
#include "system/ipc.h"
#include "system/memops.h"
#include "system/new.h"
#include "system/tinyheap.h"

#if __PS3
#include <cell/sheap.h>
#endif

#if __PPU
#pragma comment(lib,"sheap_stub")
#endif

namespace rage
{

// PURPOSE: Define cross-platform memory transfer macros
#if __SPU
#define sysMemGet sysDmaGet
#define sysMemLargeGet sysDmaLargeGet
#define sysMemGetAndWait sysDmaGetAndWait
#define sysMemLargeGetAndWait sysDmaLargeGetAndWait
#define sysMemPut sysDmaPut
#define sysMemLargePut sysDmaLargePut
#define sysMemPutAndWait sysDmaPutAndWait
#define sysMemLargePutAndWait sysDmaLargePutAndWait
#define sysMemGetU32 sysDmaGetUInt32
#define sysMemPutU32 sysDmaPutUInt32
#define sysMemPutPtr sysMemPutU32
#define sysMemGetPtr sysMemGetU32
#define sysMemWait sysDmaWait
#else
#define sysMemLargeGet sysMemGet
#define sysMemGetAndWait sysMemGet
#define sysMemLargeGetAndWait sysMemGet
#define sysMemLargePut sysMemPut
#define sysMemPutAndWait sysMemPut
#define sysMemLargePutAndWait sysMemPut
inline void* sysMemGetPtr(uptr ea, u32) { return *(void**)ea; }
inline u32 sysMemGetU32(uptr ea, u32) { return *(u32*)ea; }
inline void sysMemPutU32(u32 val, uptr ea, u32) { *(u32*)ea=val; }
inline void sysMemPutPtr(uptr val, uptr ea, u32) { *(uptr*)ea=val; }
inline void sysMemGet(void* ls, uptr ea, u32 size, u32) { sysMemCpy(ls, (void*)ea, size); }
inline void sysMemPut(void* ls, uptr ea, u32 size, u32) { sysMemCpy((void*)ea, ls, size); }
inline void sysMemWait(u32) {}
#endif

///////////////////////////////////////////////////////////////////////

enum
{
	SYS_DEPENDENCY_NUM_PIPELINE_STAGES      = 5,
};

// PURPOSE: Central location for defining dma tags used on the SPUs
enum
{
	SYS_DEPENDENCY_PIPELINE_0_DMA_TAG       = 0,
	SYS_DEPENDENCY_LOCK_FREE_RING_DMA_TAG   = SYS_DEPENDENCY_PIPELINE_0_DMA_TAG + SYS_DEPENDENCY_NUM_PIPELINE_STAGES,
};

///////////////////////////////////////////////////////////////////////

class crHeap
{
public:
	crHeap(u32 size);
	~crHeap();
	void* Allocate(u32 size);
	void Free(void* ptr);

private:
#if !__PS3
	sysCriticalSectionToken m_CsToken;
	sysTinyHeap m_Allocator;
#endif
	u8* m_Buffer;
};

///////////////////////////////////////////////////////////////////////

inline crHeap::crHeap(u32 size)
{
	Assert((size&127)==0);
	m_Buffer = rage_aligned_new(128) u8[size];
#if __PPU
	cellSheapInitialize((void*)m_Buffer, size, 0);
#elif __SPU
	cellSheapInitialize(uptr(m_Buffer), size, 0);
#else
	m_Allocator.Init(m_Buffer, size);
#endif
}

///////////////////////////////////////////////////////////////////////

inline crHeap::~crHeap()
{
	delete [] m_Buffer;
	m_Buffer = NULL;
}

///////////////////////////////////////////////////////////////////////

inline void* crHeap::Allocate(u32 size)
{
#if __PPU
	return reinterpret_cast<void*>(cellSheapAllocate((void*)m_Buffer, size));
#elif __SPU
	uptr buffer = sysDmaGetUInt32(uptr(&m_Buffer), 0);
	return reinterpret_cast<void*>(cellSheapAllocate(buffer, size));
#else
	sysCriticalSection cs(m_CsToken);
	return m_Allocator.Allocate(size);
#endif
}

///////////////////////////////////////////////////////////////////////

inline void crHeap::Free(void* ptr)
{
#if __PPU
	cellSheapFree((void*)m_Buffer, ptr);
#elif __SPU
	uptr buffer = sysDmaGetUInt32(uptr(&m_Buffer), 0);
	cellSheapFree(buffer, uptr(ptr));
#else
	sysCriticalSection cs(m_CsToken);
	m_Allocator.Free(ptr);
#endif
}

///////////////////////////////////////////////////////////////////////

// Memory buffer for DMA transfer
class crScratch
{
public:
	void Init(u8* buffer, u32 size);
	void* Alloc(u32 size);
	void Wait() const;
	void* Get(const void* src, u32 size);
	void* Push(const void* src, u32 size);
	void Pop(u32 size);

private:
	static const u32 sm_Tag = 8;
	u8* m_Buffer;
	u32 m_Size;
	u32 m_Reserve;
};

///////////////////////////////////////////////////////////////////////

inline void crScratch::Init(u8* buffer, u32 size)
{
	m_Buffer = buffer;
	m_Reserve = size;
	m_Size = 0;
}

///////////////////////////////////////////////////////////////////////

inline void* crScratch::Alloc(u32 size)
{
	FastAssert(m_Reserve >= m_Size+size);
	u32 pos = m_Size;
	u8* dest = m_Buffer + pos;
	m_Size = pos + size;
	return dest;
}

///////////////////////////////////////////////////////////////////////

inline void crScratch::Wait() const
{
	sysMemWait(1<<sm_Tag);
}

///////////////////////////////////////////////////////////////////////

#if SYS_USE_SPU_DEPENDENCY
inline void* crScratch::Get(const void* src, u32 size)
{
	u32 alignedSize = RAGE_ALIGN(size, 4);
	FastAssert(m_Reserve >= m_Size+alignedSize);
	u8* dest = m_Buffer + m_Size;
	sysMemGetAndWait(dest, uptr(src), alignedSize, sm_Tag);
	return dest;
}

///////////////////////////////////////////////////////////////////////

inline void* crScratch::Push(const void* src, u32 size)
{
	FastAssert(m_Reserve >= m_Size+size);
	u8* dest = m_Buffer + m_Size;
	sysMemGet(dest, uptr(src), size, sm_Tag);
	m_Size += size;
	return dest;
}

///////////////////////////////////////////////////////////////////////

inline void crScratch::Pop(u32 size)
{
	FastAssert(m_Size >= size);
	m_Size -= size;
}

///////////////////////////////////////////////////////////////////////

#else // SYS_USE_SPU_DEPENDENCY
inline void* crScratch::Get(const void* src, u32) { return const_cast<void*>(src); }
inline void* crScratch::Push(const void* src, u32) { return const_cast<void*>(src); }
inline void crScratch::Pop(u32) {}
#endif

} // namespace rage

#endif  // SYSTEM_DEPENDENCY_SPU_H
