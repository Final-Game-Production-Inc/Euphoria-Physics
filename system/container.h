// 
// system/container.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_CONTAINER_H 
#define SYSTEM_CONTAINER_H 

#include "system/memory.h"
#include "data/struct.h"
#include "diag/errorcodes.h"

namespace rage {

// This is a simplification of the sysMemContainer class, without all the unnecessary overhead
class sysMemStack : public sysMemAllocator
{
private:
	void* m_base;
	size_t m_size;
	size_t m_offset;
	bool m_owner : 1;
	bool m_ready : 1;

public:
	sysMemStack() : m_base(NULL), m_size(0), m_offset(0), m_owner(false), m_ready(false) { }
	sysMemStack(const size_t size, const size_t align);
	sysMemStack(const void* ptr, const size_t size);
	SYS_MEM_VIRTUAL ~sysMemStack() { Destroy(); }

	// Utility
	void Create(const void* ptr, const size_t size);
	void Create(const size_t size, const size_t align);
	void Destroy();

	inline void Reset() { m_offset = 0; }

	SYS_MEM_VIRTUAL void* Allocate(size_t size, size_t align, int heapIndex = 0);
	SYS_MEM_VIRTUAL void Free(const void* UNUSED_PARAM(ptr)) { Quitf(ERR_MEM_CONT_3,"sysMemStack::Free is not implemented on purpose!"); }

	SYS_MEM_VIRTUAL void* GetHeapBase() const { return m_base; }
	SYS_MEM_VIRTUAL size_t GetHeapSize() const {return m_size; }
	
	SYS_MEM_VIRTUAL size_t GetMemoryUsed(int bucket = -1);
	SYS_MEM_VIRTUAL size_t GetMemoryAvailable();

	SYS_MEM_VIRTUAL bool IsValidPointer(const void *ptr) const { return (ptr >= static_cast<u8*>(m_base) && ptr < (static_cast<u8*>(m_base) + m_size)); }

	// Mutators
	inline void SetReady(bool ready) { m_ready = ready; }

	// Accessors
	inline bool IsReady() const { return m_ready; }
};

class datResource;

struct sysMemContainerData
{
#if !__SPU
	sysMemContainerData() : m_Base(NULL), m_Size(0) { }
	sysMemContainerData(datResource&);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif
#endif

	void *m_Base;
	unsigned m_Size;
};

#if !__SPU && !__TOOL

class sysMemContainer: public sysMemAllocator
{		
	void operator=(const sysMemContainer&) { }
public:
	sysMemContainer(sysMemContainerData &data);

	~sysMemContainer();

	void *Allocate(size_t size,size_t align,int heapIndex);

	void Free(const void *ptr);

	size_t GetMemoryUsed(int);

	size_t GetMemoryAvailable();

	// PURPOSE:	Reserve memory for a container we're going to fill.
	void Init(size_t maxSize);

	// PURPOSE: Finalize a container, preventing further allocations (but the container is still usable)
	void Finalize();

	// PURPOSE:	Reclaim the memory reserved by the container (must be called before object destructs)
	//			(Temporarily changes the current allocator to be this object so we can correctly ignore
	//			deletions within the container itself)
	void BeginShutdown();

	// PURPOSE:	End shutdown process (restores current allocator to default)
	void EndShutdown();

private:
	unsigned m_Offset;	// Current base allocation of container
	sysMemContainerData &m_Data;
	sysMemAllocator *m_Prev;
};

#endif		// !__SPU && !__TOOL

} // namespace rage

#endif // SYSTEM_CONTAINER_H 
