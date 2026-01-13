// 
// grcore/cbset.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_CBSET_H 
#define GRCORE_CBSET_H 

#include "grcore/device.h"

#if COMMAND_BUFFER_SUPPORT

namespace rage {

typedef u16 grcCommandBufferHandle;

/*
	Typical usage:

		grcSetWorld(m_Matrix);
		if (sm_CmdBufSet.Begin(m_Handle, m_StateDirty)) {
			DrawModel();
			m_StateDirty = false;
			sm_CmdBufSet.End(m_Handle);
		}

	If Begin returns true, you are expected to do a normal rendering and then call End.
	If Begin returns false, it was able to re-use the existing command buffer, and you
	should not call End (although it's ignored if you do)

	TODO:
	- Should we expect everybody to allocate a handle and then free it when the instance
		is deleted?
	- Should we allow using too many handles to invalidate other people's handles?
		(I don't think so, that's almost never what you want and requires backpointers)
*/

class grcCommandBufferSet {
public:
	// PURPOSE:	Initialize the set
	// PARAMS:	maxHandles - Maximum number of handles we'll allocate.
	//			masterBuffer - Pointer to a master buffer we'll use for recording and cloning.
	void Init(int maxHandles,grcCommandBuffer *masterBuffer);

	grcCommandBufferHandle Allocate();

	void Free(grcCommandBufferHandle);

	// PURPOSE:	Allocate or recycle a command buffer handle.
	// PARAMS:	handle - Reference to handle; should be zeroed out before first use.
	//			dirty - If true, forces the command buffer to be re-recorded.
	// RETURNS:	True if you're expected to do normal rendering and then call End.
	//			False if you can just skip it.
	bool Begin(grcCommandBufferHandle handle,bool dirty = false) {
#if !__ASSERT
		Entry *e = &m_Entries[handle-1];
		if (e->Buffer && !dirty) {
			e->LastFrameUsed = GRCDEVICE.GetFrameCounter();
			GRCDEVICE.RunCommandBuffer(e->Buffer,0);
			return false;
		}
		else
#endif
			return BeginInternal(handle,dirty);
	}

	void End() { if (sm_InBegin) EndInternal(); }

private:
	bool BeginInternal(grcCommandBufferHandle,bool);
	void EndInternal();

	struct Entry {
		union {
			u32 LastFrameUsed;
			Entry *Next;
		};
		grcCommandBuffer *Buffer;
	};
	Entry *m_Entries, *m_FirstFree;
	int m_MaxHandles;
	grcCommandBuffer *m_Master;
	static Entry *sm_InBegin;
};

} // namespace rage

#endif	// COMMAND_BUFFER_SUPPORT

#endif // GRCORE_CBSET_H 
