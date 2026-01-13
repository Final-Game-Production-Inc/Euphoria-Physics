// 
// system/multiallocator.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "multiallocator.h"
#include "diag/errorcodes.h"

namespace rage {

__THREAD int sysMemBuddyFreeRscMem;

sysMemMultiAllocator::sysMemMultiAllocator() { }

sysMemMultiAllocator::~sysMemMultiAllocator() { }

int sysMemMultiAllocator::AddAllocator(sysMemAllocator &allocator)
{
	int result = m_Heaps.GetCount();
	m_Heaps.Append() = &allocator;
	return result;
}

int sysMemMultiAllocator::ReserveAllocator()
{
	int result = m_Heaps.GetCount();
	m_Heaps.Append() = NULL;
	return result;
}

void sysMemMultiAllocator::SetAllocator(int i,sysMemAllocator &a)
{
	m_Heaps[i] = &a;
}

void *sysMemMultiAllocator::Allocate(size_t size,size_t align,int heapIndex)
{
	return m_Heaps[heapIndex]->Allocate(size,align,heapIndex);
}

void *sysMemMultiAllocator::TryAllocate(size_t size,size_t align,int heapIndex)
{
	return m_Heaps[heapIndex]->TryAllocate(size,align,heapIndex);
}

void sysMemMultiAllocator::Resize(const void *ptr,size_t newSmallerSize)
{
	if (ptr) {
		for (int i=0; i<m_Heaps.GetCount(); i++) {
			if (m_Heaps[i]->IsValidPointer(ptr)) {
				m_Heaps[i]->Resize(ptr,newSmallerSize);
				return;
			}
		}
		Quitf(ERR_MEM_MULTIALLOC_RESIZE,"sysMemMultiAllocator::Resize(%p) - Not owned by any known heap?",ptr);
	}
}

void sysMemMultiAllocator::Free(const void *ptr)
{
	if (ptr) {
		for (int i=0; i<m_Heaps.GetCount(); i++) {
			if (m_Heaps[i]->IsValidPointer(ptr)) {
				// If the memory isn't in resource memory, OR the memory isn't part of a resource, go ahead and free it.
				// If sysMemBuddyFreeRscMem is true, we're using the multiallocator to delete memory from the resource heap.
				// This is absolutely not recommended, but PSOs are currently doing it. So we need it.
#if __PS3 || __XENON
				if ((i != MEMTYPE_RESOURCE_PHYSICAL && i != MEMTYPE_RESOURCE_VIRTUAL && i != MEMTYPE_HEADER_VIRTUAL) || sysMemBuddyFreeRscMem)
					m_Heaps[i]->Free(ptr);
#else
				if ((i != MEMTYPE_RESOURCE_PHYSICAL && i != MEMTYPE_RESOURCE_VIRTUAL) || sysMemBuddyFreeRscMem)
					m_Heaps[i]->Free(ptr);
#endif
				return;
			}
		}
#if ENABLE_BUDDY_ALLOCATOR
		Quitf(ERR_MEM_MULTIALLOC_FREE,"sysMemMultiAllocator::Free(%p) - Not owned by any known heap?",ptr);
#endif // ENABLE_BUDDY_ALLOCATOR
	}
}

size_t sysMemMultiAllocator::GetSize(const void *ptr) const
{
	for (int i=0; i<m_Heaps.GetCount(); i++)
		if (m_Heaps[i]->IsValidPointer(ptr))
			return m_Heaps[i]->GetSize(ptr);
	return 0;
}

const void *sysMemMultiAllocator::GetCanonicalBlockPtr(const void *ptr) const
{
	for (int i=0; i<m_Heaps.GetCount(); i++)
		if (m_Heaps[i]->IsValidPointer(ptr))
			return m_Heaps[i]->GetCanonicalBlockPtr(ptr);
	return NULL;
}

bool sysMemMultiAllocator::TryLockBlock(const void *ptr, unsigned lockCount)
{
	for (int i=0; i<m_Heaps.GetCount(); i++)
		if (m_Heaps[i]->IsValidPointer(ptr)) {
			return m_Heaps[i]->TryLockBlock(ptr, lockCount);
		}
	return true;
}

void sysMemMultiAllocator::UnlockBlock(const void *ptr, unsigned unlockCount)
{
	for (int i=0; i<m_Heaps.GetCount(); i++)
		if (m_Heaps[i]->IsValidPointer(ptr)) {
			m_Heaps[i]->UnlockBlock(ptr, unlockCount);
			break;
		}
}

u32 sysMemMultiAllocator::GetUserData(const void *ptr) const
{
	for (int i=0; i<m_Heaps.GetCount(); i++)
		if (m_Heaps[i]->IsValidPointer(ptr))
			return m_Heaps[i]->GetUserData(ptr);
	return ~0U;
}

void sysMemMultiAllocator::SetUserData(const void *ptr, u32 userData)
{
	for (int i=0; i<m_Heaps.GetCount(); i++) {
		if (m_Heaps[i]->IsValidPointer(ptr)) {
			m_Heaps[i]->SetUserData(ptr, userData);
			return;
		}
	}
}

size_t sysMemMultiAllocator::GetSizeWithOverhead(const void *ptr) const
{
	for (int i=0; i<m_Heaps.GetCount(); i++)
		if (m_Heaps[i]->IsValidPointer(ptr))
			return m_Heaps[i]->GetSizeWithOverhead(ptr);
	return 0;
}

size_t sysMemMultiAllocator::GetMemoryUsed(int bucket)
{
	size_t result = 0;
	for (int i=0; i<m_Heaps.GetCount(); i++) {
		if ( ! i || ( i && m_Heaps[i] != m_Heaps[i-1] ) )
			result += m_Heaps[i]->GetMemoryUsed(bucket);
	}
	return result;
}

size_t sysMemMultiAllocator::GetMemoryAvailable()
{
	size_t result = 0;
	for (int i=0; i<m_Heaps.GetCount(); i++) {
		if ( ! i || ( i && m_Heaps[i] != m_Heaps[i-1] ) )
			result += m_Heaps[i]->GetMemoryAvailable();
	}
	return result;
}

size_t sysMemMultiAllocator::GetLargestAvailableBlock()
{
	return m_Heaps[0]->GetLargestAvailableBlock();
}

#if !__FINAL
void sysMemMultiAllocator::SetBreakOnAlloc(int id)
{
	m_Heaps[0]->SetBreakOnAlloc(id);
}
#endif

void sysMemMultiAllocator::SanityCheck()
{
	for (int i=0; i<m_Heaps.GetCount(); i++)
		m_Heaps[i]->SanityCheck();
}

bool sysMemMultiAllocator::IsValidPointer(const void *ptr) const
{
	for (int i=0; i<m_Heaps.GetCount(); i++)
		if (m_Heaps[i]->IsValidPointer(ptr))
			return true;
	return false;
}

void sysMemMultiAllocator::BeginLayer()
{
	m_Heaps[0]->BeginLayer();
}

int sysMemMultiAllocator::EndLayer(const char *layerName,const char *leakfile)
{
	return m_Heaps[0]->EndLayer(layerName,leakfile);
}

void sysMemMultiAllocator::BeginMemoryLog(const char *filename,bool logStackTracebacks)
{
	m_Heaps[0]->BeginMemoryLog(filename,logStackTracebacks);
}

void sysMemMultiAllocator::EndMemoryLog()
{
	m_Heaps[0]->EndMemoryLog();
}

unsigned sysMemMultiAllocator::GetNumAllocators() const
{
	return m_Heaps.GetCount();
}

sysMemAllocator *sysMemMultiAllocator::GetAllocator(int heapIndex)
{
	return m_Heaps[heapIndex];
}

const sysMemAllocator *sysMemMultiAllocator::GetAllocator(int heapIndex) const
{
	return m_Heaps[heapIndex];
}

sysMemAllocator *sysMemMultiAllocator::GetPointerOwner(const void *ptr)
{
	for (int i=0; i<m_Heaps.GetCount(); i++)
		if (m_Heaps[i]->IsValidPointer(ptr))
			return m_Heaps[i];

	return NULL;
}

}	// namespace rage
