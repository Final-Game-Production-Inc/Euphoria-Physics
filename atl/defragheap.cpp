// 
// atl/defragheap.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#if 0
#include "defragheap.h"

namespace rage {

void atDefragmentableHeap::InitInternal(void *handleStorage,size_t handleStorageSize,void *nodeStorage,size_t nodeStorageSize) {
	m_LockCount = 0;

	// Init handle table
	m_HandleCount = handleStorageSize / sizeof(Entry);
	m_Handles = (Entry*) handleStorage;
	sysMemSet(m_Handles, 0, sizeof(handleStorageSize));

	// Init node storage with a single large free node
	Assert((nodeStorageSize & 3) == 0);
	m_Start = (Node*) nodeStorage;
	m_End = (Node*) ((char*)nodeStorage + nodeStorageSize);
	sysMemSet(m_Start, 0, nodeStorageSize);
	m_Start->Free = 1;
	m_Start->Size = nodeStorageSize>>2;
	m_FirstFreeNode = m_Start;
}

void atDefragmentableHeap::BeginGarbageCollect()
{
	Assert(!m_LockCount);
	// Nothing to do here, Mark fields are already kept cleared
}

void atDefragmentableHeap::EndGarbageCollect()
{
	Assert(!m_LockCount);
	// Invalidate all non-marked handles
	for (int i=1; i<m_HandleCount; i++) {
		if (m_Handles[i].Marked)
			m_Handles[i].Marked = 0;
		else if (!m_Handles[i].Free)
			Free(i);
	}
	Defragment();
}

atDefragmentableHeap::Handle atDefragmentableHeap::Allocate(size_t size) {
	// Out of handles?
	if (!m_FirstFreeHandle)
		return 0;

	// Round size up to next multiple of four, and convert to word count
	if (!size)
		++size;
	size = (size+3) >> 2;



}

void atDefragmentableHeap::Free(atDefragmentableHeap::Handle handle_) {
	if (!handle_)
		return;
	u32 handle = (u32) handle_;

	Assert(handle < m_HandleCount);
	Assert(!m_Handles[i].Free);

	// Find the heap node and mark it free
	Node *cur = m_Start + m_Handle[i].Offset;
	cur->Free = 1;
	// Merge the next node now if it was already free
	Node *next = cur + cur->Size;
	if (next != m_End && next->Free)
		cur->Size += next->Size;
	// Mark the handle free, add it to free handle list
	m_Handles[handle].Free = 1;
	m_Handles[handle].Offset = m_FirstFreeHandle;
	m_Handles[handle].Magic = 0;
	m_FirstFreeHandle = i;
}

} // namespace rage

#endif
