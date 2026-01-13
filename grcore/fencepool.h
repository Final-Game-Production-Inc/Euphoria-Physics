//
// grcore/fencepool.h
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_FENCEPOOL_H
#define GRCORE_FENCEPOOL_H

#include "channel.h"
#include "system/criticalsection.h"

namespace rage {

#if RSG_DURANGO || RSG_ORBIS

	struct ALIGNAS(8) __grcFenceHandle {
		// state values
		enum {
			// The 0xfe578 prefix is just something to make it more humanly
			// recognizable looking at memory.  Really bad 1337 speak for "FE(nce)
			// state" :/
			STATE_PENDING           = 0xfe578001,
			STATE_DONE              = 0xfe578002,
			STATE_FREE              = 0xfe578003,
		};

		typedef u32 StateType;
		volatile StateType      state;

		// flag values
		enum {
			FLAG_OVERFLOW           = 0x00000001u,
		};
		u32                     flags;

		__grcFenceHandle       *next;
	};

#elif RSG_PC

	struct __grcFenceHandle {
		// state values
		enum {
			STATE_MARKED_PENDING    = 0x0001,
			STATE_QUERY_ISSUED      = 0x0002,
			STATE_DONE              = 0x0003,
			STATE_FREE              = 0x0004,
		};

		typedef u16 StateType;
		StateType               state;

		// flag values
		enum {
			FLAG_OVERFLOW           = 0x0001u,
		};
		u16                     flags;

		s32                     queryIndex;

		__grcFenceHandle       *next;
	};

#else
#	error "platform not yet supported"
#endif


class FencePool {
public:

#	if __ASSERT
#		define FENCE_ALLOCATED ((__grcFenceHandle*)0xdecafbaddecafbad)
#	endif

	explicit FencePool(unsigned size) {
		grcAssert(size);

		// rage_new is allocating ONION/WB which is important for CPU polling of
		// fences.
		m_Fences = rage_new __grcFenceHandle[size];

		// Put all fences into the free list.  Don't really need to be
		// initializing 'state' here, but initializing 'flags' is important
		// (otherwise FLAG_OVERFLOW could be set).
		for (unsigned i=0; i<size-1; ++i) {
			m_Fences[i].state = (__grcFenceHandle::StateType)__grcFenceHandle::STATE_FREE;
			m_Fences[i].flags = 0;
			m_Fences[i].next  = m_Fences+i+1;
		}
		m_Fences[size-1].state = (__grcFenceHandle::StateType)__grcFenceHandle::STATE_FREE;
		m_Fences[size-1].flags = 0;
		m_Fences[size-1].next  = NULL;
		m_Free = m_Fences;

		m_PendingGpuFreePool = NULL;
		m_PendingGpuFreeOverflow = NULL;

		ASSERT_ONLY(m_Constructed = true;)
	}

	~FencePool() {
		delete[] m_Fences;
	}

	__grcFenceHandle *Alloc() {
#		if __ASSERT
			// If this "assert" fires, then it will be while executing static
			// constructors.  Problem is that Assert doesn't actually work at
			// that point, since the channels have not been constructed either.
			// So use a hardcoded breakpoint instead.
			if (Unlikely(!m_Constructed)) __debugbreak(); // Don't allocate fences from static constructors!
#		endif

		// This could be made lock free (provided m_Free!=NULL) (be careful of
		// ABA if doing this!), but unlikely to be necissary, so just keep
		// simple mutex for now unless this ever shows up in profiling.
		SYS_CS_SYNC(m_Mutex);

		// Check free list
		__grcFenceHandle *free = m_Free;
		if (Likely(free)) {
			m_Free = free->next;
			ASSERT_ONLY(free->next = FENCE_ALLOCATED;)
			return free;
		}

		// Free list is empty, check for anything that has been freed by the
		// GPU.
		__grcFenceHandle **prev = &m_PendingGpuFreePool;
		__grcFenceHandle *curr = *prev;
		while (curr) {
			__grcFenceHandle *const next = curr->next;
			if (curr->state == __grcFenceHandle::STATE_FREE) {
				*prev = next;
				curr->next = free;
				free = curr;
			}
			else
				prev = &(curr->next);
			curr = next;
		}
		if (free) {
			m_Free = free->next;
			ASSERT_ONLY(free->next = FENCE_ALLOCATED;)
			return free;
		}

		// Nothing available in pool.  Emergency allocation from "game virtual".
		grcAssertf(0, "GPU handle pool exhausted.  Performing emergency allocation, which will probably work.  But pool size needs to be increased.");
		free = rage_new __grcFenceHandle;
		free->flags = __grcFenceHandle::FLAG_OVERFLOW;
		ASSERT_ONLY(free->next = FENCE_ALLOCATED;)
		return free;
	}

	void ImmediateFree(__grcFenceHandle *fence) {
		FatalAssert(fence->next == FENCE_ALLOCATED);
		if (Unlikely(fence->flags & __grcFenceHandle::FLAG_OVERFLOW))
			delete fence;
		else {
			SYS_CS_SYNC(m_Mutex);
			fence->next = m_Free;
			m_Free = fence;
		}
	}

	void PendingFree(__grcFenceHandle *fence) {
		FatalAssert(fence->next == FENCE_ALLOCATED);
		if (fence->state == __grcFenceHandle::STATE_FREE)
			ImmediateFree(fence);
		else {
			SYS_CS_SYNC(m_Mutex);
			if (Unlikely(fence->flags & __grcFenceHandle::FLAG_OVERFLOW)) {
				fence->next = m_PendingGpuFreeOverflow;
				m_PendingGpuFreeOverflow = fence;
			}
			else {
				fence->next = m_PendingGpuFreePool;
				m_PendingGpuFreePool = fence;
			}
		}
	}

	void CleanUpOverflow() {
		SYS_CS_SYNC(m_Mutex);
		__grcFenceHandle **prev = &m_PendingGpuFreeOverflow;
		__grcFenceHandle *curr = m_PendingGpuFreeOverflow;
		while (curr) {
			__grcFenceHandle *const next = curr->next;
			if (curr->state == __grcFenceHandle::STATE_FREE) {
				*prev = next;
				delete curr;
			}
			else
				prev = &(curr->next);
			curr = next;
		}
	}


private:

	ASSERT_ONLY(bool            m_Constructed;)
	sysCriticalSectionToken     m_Mutex;
	__grcFenceHandle           *m_Fences;
	__grcFenceHandle           *m_Free;
	__grcFenceHandle           *m_PendingGpuFreePool;
	__grcFenceHandle           *m_PendingGpuFreeOverflow;

#	if __ASSERT
#		undef FENCE_ALLOCATED
#	endif
};

}
// namespace rage

#endif // GRCORE_FENCEPOOL_H
