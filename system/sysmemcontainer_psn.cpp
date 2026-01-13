// 
// system/sysmemcontainer_psn.cpp
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#if __PPU && 0
#include "sysmemcontainer_psn.h"
#include <sys/memory.h>

namespace rage {

CompileTimeAssert(sizeof(rage::u32) == sizeof(sys_memory_container_t));

sysMemContainerMgr SYSMEMCONTAINER;

sysMemContainerMgr::sysMemContainerMgr()
:	m_Preallocated(0)
,	m_PreallocatedSize(0)
,	m_InUseBySystem(false)
,	m_Container(SYS_MEMORY_CONTAINER_ID_INVALID)
{}

sysMemContainerMgr::~sysMemContainerMgr()
{
	if (m_Container != SYS_MEMORY_CONTAINER_ID_INVALID)
	{
		AssertVerify(CELL_OK == sys_memory_container_destroy(m_Container));
	}		
}

void* sysMemContainerMgr::Preallocate(u32 size, const relocateDelegate& relocatefn)
{
	Assert(!m_PreallocatedSize && m_Container == SYS_MEMORY_CONTAINER_ID_INVALID);
	// round up to nearest megabyte
	size = (size + 0xFFFFF) & ~0xFFFFF;
	m_PreallocatedSize = size;
	if (sys_memory_container_create(&m_Container, size) != CELL_OK)
	{
		sys_memory_info_t mem_info;
		sys_memory_get_user_memory_size( &mem_info );
		size_t memRemain = mem_info.available_user_memory >> 10;
		Errorf("sys_memory_container_create failed, only %dk memory remain, wanted %dk",memRemain,size>>10);
	}
	m_RelocateFn = relocatefn;
	sys_addr_t sysaddr = 0;
	AssertVerify(CELL_OK == sys_memory_allocate_from_container(
		size, m_Container, SYS_MEMORY_PAGE_SIZE_1M, &sysaddr));
	m_Preallocated = (void*)sysaddr;
	return m_Preallocated;
}

void sysMemContainerMgr::DestroyPreallocated()
{
	Assert(m_PreallocatedSize && m_Container != SYS_MEMORY_CONTAINER_ID_INVALID);	
	if (!m_InUseBySystem)
	{
		AssertVerify(CELL_OK == sys_memory_free((sys_addr_t)m_Preallocated));	
		m_Preallocated = 0;
		AssertVerify(CELL_OK == sys_memory_container_destroy(m_Container));
		m_Container = SYS_MEMORY_CONTAINER_ID_INVALID;
	}
	m_PreallocatedSize = 0;
}

bool sysMemContainerMgr::IsAvailable() const
{
	return m_Preallocated && !m_InUseBySystem;
}

u32 sysMemContainerMgr::Allocate(u32 size)
{
	Assert(!m_InUseBySystem);
	//Displayf("SONY System utility allocating %i bytes", size);
	if (m_PreallocatedSize)
	{
		// request the memory from the game
		Assert(m_Container != SYS_MEMORY_CONTAINER_ID_INVALID);
		Assert(m_Preallocated);
		m_RelocateFn(true);	
		AssertVerify(CELL_OK == sys_memory_free((sys_addr_t)m_Preallocated));
	}
	else
	{
		Assert(m_Container == SYS_MEMORY_CONTAINER_ID_INVALID);
		Assert(!m_Preallocated);
		if (sys_memory_container_create(&m_Container, size) != CELL_OK)
		{
			sys_memory_info_t mem_info;
			sys_memory_get_user_memory_size( &mem_info );
			size_t memRemain = mem_info.available_user_memory >> 10;
			Errorf("sys_memory_container_create failed, only %dk memory remain, wanted %dk",memRemain,size>>10);
		}
	}	
	m_InUseBySystem = true;
	return m_Container;
}

void sysMemContainerMgr::Destroy(u32 container)
{
	Assert(m_InUseBySystem);
	Assert(container == m_Container);
	m_InUseBySystem = false;
	//Displayf("SONY System utility freed memory");
	if (m_PreallocatedSize)
	{
		// return the memory to the game
		Assert(m_Container != SYS_MEMORY_CONTAINER_ID_INVALID);
		sys_addr_t sysaddr = 0;
		AssertVerify(CELL_OK == sys_memory_allocate_from_container(
			m_PreallocatedSize, m_Container, SYS_MEMORY_PAGE_SIZE_1M, &sysaddr));
		// make sure the memory didn't change address
		Assert((void*)sysaddr == m_Preallocated);
		m_RelocateFn(false);
	}
	else
	{
		AssertVerify(CELL_OK == sys_memory_container_destroy(m_Container));
		m_Container = SYS_MEMORY_CONTAINER_ID_INVALID;
	}	
}

} // namespace rage

#endif // __PPU
