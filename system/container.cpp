// 
// system/container.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#include "container.h"
#include "diag/output.h"
#include "system/new.h"

namespace rage {

// Stack
sysMemStack::sysMemStack(const void* ptr, const size_t size) : m_base(const_cast<void*>(ptr)), m_size(size), m_offset(0), m_owner(false), m_ready(false)
{ 
	Assert(ptr && size); 
}

sysMemStack::sysMemStack(const size_t size, const size_t align) : m_size(size), m_offset(0), m_owner(true), m_ready(false)
{
	Assert(size && align);

	m_base = sysMemAllocator::GetCurrent().RAGE_LOG_ALLOCATE(size, align);
}

void sysMemStack::Create(const void* ptr, const size_t size)
{
	Assert(ptr && size);

	Destroy();

	m_base = const_cast<void*>(ptr); 
	m_size = size; 
	m_offset = 0;
	m_owner = false;
}

void sysMemStack::Create(const size_t size, const size_t align)
{
	Assert(size && align);

	Destroy();

	m_base = sysMemAllocator::GetCurrent().RAGE_LOG_ALLOCATE(size, align);
	m_size = size;
	m_offset = 0;
	m_owner = true;
}

void sysMemStack::Destroy()
{
	if (m_owner)
	{
		sysMemAllocator::GetCurrent().Free(m_base);
		m_owner = false;
	}

	m_base = NULL;
	m_size = 0;
	m_offset = 0;	
}

void* sysMemStack::Allocate(size_t size, size_t align, int UNUSED_PARAM(heapIndex /*= 0*/))
{
	Assert(size && align);

	if (!size)
		return NULL;
	
#if !__SPU
	// If the size is smaller than the alignment, only respect the size.
	if (size <= 4)
		align = 4;
	else if (size <= 8)
		align = 8;
#endif

	u8* ptr = reinterpret_cast<u8*>(m_base) + m_offset;
	u8* aligned_ptr = reinterpret_cast<u8*>((reinterpret_cast<size_t>(ptr) + (align - 1)) & ~(align - 1));
	m_offset = (aligned_ptr - reinterpret_cast<u8*>(m_base)) + size;

	if (m_offset > m_size)
		Quitf(ERR_MEM_CONT_1,"sysMemStack::Allocate(%" SIZETFMT "u) overflowed, give larger number to Init call", size);

	return aligned_ptr;
}

size_t sysMemStack::GetMemoryUsed(int UNUSED_PARAM(bucket /*= -1*/))
{
	return m_offset;
}

size_t sysMemStack::GetMemoryAvailable()
{
	return m_size - m_offset;
}

// Data
sysMemContainerData::sysMemContainerData(datResource &rsc) {
	 rsc.PointerFixup(m_Base);
}

#if __DECLARESTRUCT
void sysMemContainerData::DeclareStruct(datTypeStruct &s) {
	STRUCT_BEGIN(sysMemContainerData);
	STRUCT_FIELD_VP(m_Base);
	STRUCT_FIELD(m_Size);
	STRUCT_END();
}
#endif

#if !__SPU && !__TOOL

// Container
sysMemContainer::sysMemContainer(sysMemContainerData &data) : m_Offset(0), m_Data(data), m_Prev(NULL)
{
}

sysMemContainer::~sysMemContainer()
{
	AssertMsg(!m_Prev,"Missing Finalize/Shutdown call.");
}

void *sysMemContainer::Allocate(size_t size,size_t align,int /*heapIndex*/)
{
	Assertf(size, "Tried to allocate 0 bytes in sysMemContainer::Allocate! Open a B* to Eric J Anderson to investigate.");
	size += ! size;
	if (!align) align=16;

	// Assert(align == 16); this can now happen legitimately with atArray<T, __alignof(T)>
	// If the size is smaller than the alignment, only respect the size.
	if (size <= 4)
		align = 4;
	else if (size <= 8)
		align = 8;

	m_Offset = (unsigned)((m_Offset + (align-1)) & ~(align-1));
	void *result = (char*)m_Data.m_Base + m_Offset;
	m_Offset += (unsigned)size;
	if (m_Offset > m_Data.m_Size)
		Quitf(ERR_MEM_CONT_2,"sysMemContainer(%" SIZETFMT "u) overflowed, give larger number to Init call",size);
	// Displayf("Allocate(%u) returns %p (offset now %u)",size,result,m_Offset);
	return result;
}

void sysMemContainer::Free(const void * ptr)
{
	if (ptr && (ptr < m_Data.m_Base || ptr >= (char*)m_Data.m_Base+m_Data.m_Size))
	{
		m_Prev->Free(ptr);
		// Quitf("sysMemContainer::Free(%p) not within this container.",ptr);
	}
}

size_t sysMemContainer::GetMemoryUsed(int)
{
	return m_Offset;
}

size_t sysMemContainer::GetMemoryAvailable()
{
	return m_Data.m_Size - m_Offset;
}

void sysMemContainer::Init(size_t maxSize)
{
	AssertMsg(!m_Prev,"Missing Finalize/EndShutdown before Init");
	AssertMsg(!m_Data.m_Base,"Memory container is being re-used, this is really bad.");
	m_Data.m_Base = sysMemAllocator::GetCurrent().RAGE_LOG_ALLOCATE(m_Data.m_Size = (unsigned) maxSize, 16);
	m_Prev = &SetContainer(*this);
}

void sysMemContainer::Finalize()
{
	AssertMsg(m_Prev,"Missing Init call before Finalize");
	m_Offset = (m_Offset + 15) & ~15;
	sysMemAllocator::GetCurrent().Resize(m_Data.m_Base,m_Offset);
	if (!m_Offset)
		m_Data.m_Base = NULL;
	m_Data.m_Size = m_Offset;
	SetContainer(*m_Prev);
	m_Prev = NULL;
}

void sysMemContainer::BeginShutdown()
{
	AssertMsg(!m_Prev,"Missing Finalize/EndShutdown before BeginShutdown");
	m_Prev = &GetCurrent();
	SetCurrent(*this);
}

void sysMemContainer::EndShutdown()
{
	AssertMsg(m_Prev,"Missing BeginShutdown call before EndShutdown");
	m_Prev->Free(m_Data.m_Base);
	m_Offset = m_Data.m_Size = 0;
	m_Data.m_Base = NULL;
	SetCurrent(*m_Prev);
	m_Prev = NULL;
}

#endif	// !__SPU

} // namespace rage
