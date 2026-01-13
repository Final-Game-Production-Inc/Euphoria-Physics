// 
// grcore/cbset.cpp 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#include "cbset.h"
#include "grcore/channel.h"

#if COMMAND_BUFFER_SUPPORT

namespace rage {


grcCommandBufferSet::Entry* grcCommandBufferSet::sm_InBegin;

void grcCommandBufferSet::Init(int maxHandles,grcCommandBuffer *masterBuffer)
{
	m_Master = masterBuffer;
	m_Entries = rage_new Entry[maxHandles];
	m_MaxHandles = maxHandles;
	m_FirstFree = m_Entries;
	// Fill in the free list (like atPool, but I don't want to be tied to its implementation)
	for (int i=0; i<maxHandles-1; i++) {
		m_Entries[i].Next = &m_Entries[i+1];
		m_Entries[i].Buffer = 0;
	}
	m_Entries[maxHandles-1].Next = 0;		// Last free item
	m_Entries[maxHandles-1].Buffer = 0;
}


grcCommandBufferHandle grcCommandBufferSet::Allocate()
{
	if (!m_FirstFree)
		return 0;
	else {
		Entry *e = m_FirstFree;
		m_FirstFree = m_FirstFree->Next;
		return (grcCommandBufferHandle)((e - m_Entries) + 1);
	}
}


void grcCommandBufferSet::Free(grcCommandBufferHandle h)
{
	if (Verifyf(h > 0 && h <= m_MaxHandles,"Invalid handle %x passed to Free",h)) {
		Entry *e = m_Entries + h - 1;
		// WARNING - Buffer and Next are in a union together.  Buffer is valid if
		// the handle has been allocated, otherwise Next is valid if it's on the free list.
		if (e->Buffer)
			GRCDEVICE.DeleteCommandBuffer(e->Buffer);
		e->Next = m_FirstFree;
		m_FirstFree = e;
	}
}


bool grcCommandBufferSet::BeginInternal(grcCommandBufferHandle handle,bool dirty)
{
	Assert(handle <= m_MaxHandles);
	if (Verifyf(!sm_InBegin,"grcCommandBufferSet::Begin already in progress (did you forget to call End?)")) {
		Entry *e = &m_Entries[handle-1];
		if (dirty && e->Buffer) {
			// crap -- need to put this in a dirty list
			// ... or if the command buffer has been invalidated already, there's a good
			// chance it will be invalidated again later, so just force normal rendering.
			// Once we know we want to get rid of a commandbuffer, we need to count a certain
			// minimum number of frames first so we know the gpu/spu isn't using it.
			GRCDEVICE.DeleteCommandBuffer(e->Buffer);
			e->Buffer = 0;
		}
		if (e->Buffer) {
			e->LastFrameUsed = GRCDEVICE.GetFrameCounter();
			GRCDEVICE.RunCommandBuffer(e->Buffer,0);
			return false;
		}
		else {
			GRCDEVICE.BeginCommandBuffer(m_Master, false);
			sm_InBegin = e;
		}
	}
	// Caller must do a normal render
	return true;
}


void grcCommandBufferSet::EndInternal()
{
	Assert(sm_InBegin);

	Entry *e = sm_InBegin;
	size_t amtUsed = GRCDEVICE.EndCommandBuffer(&e->Buffer);
	e->LastFrameUsed = GRCDEVICE.GetFrameCounter();
	sm_InBegin = NULL;
		if (amtUsed)
		GRCDEVICE.RunCommandBuffer(e->Buffer,0);
	else {
		grcErrorf("Ran out of cmd buf memory, cannot render");
		// TODO: Flag a bunch of stuff for deletion
	}
}


} // namespace rage

#endif		// COMMAND_BUFFER_SUPPORT
