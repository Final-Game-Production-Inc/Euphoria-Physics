// 
// grcore/effect_internal.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_EFFECT_INTERNAL_H 
#define GRCORE_EFFECT_INTERNAL_H 

// Set to nonzero to allow vertex programs below this size to be inlined instead of called.
// If program is above a certain size, it's better to still call out to it.
// No inlining, camera detached, 89.3ms gpu. (PS3 Release)
// Inline limit of 2k: 89.2ms.  Really not a whole lot to write hope about.
// GPAD claims a 1.85ms savings though, annoying.  (55.6ms baseline)
// 4k limit - GPAD still removed 1169 calls, 0.9ms potential savings.  (same for 8000 byte limit)
// 2k limit - GPAD removed 1169 calls, 1.0ms potential savings.
// 1k limit - GPAD removed 1219 calls, 0.9ms potential savings.
// 512b limit - GPAD removed 1415 calls, 0.9ms potential savings.
// Disabled - GPAD removed 1979 calls, 0.9ms potential savings.  I call bullshit.
#define INLINE_SMALL_VERTEX_PROGRAM_LIMIT 2048

#include "grcore/effect_typedefs.h"

namespace rage {

template <class _T,int size> class TinySharedHeap {
	u16 EncodeHandle(u32 offset,int count) {
		Assert(offset < 1024);
		Assert(count < 16);
		return u16(offset | (count << 10));
	}
public:
	// Intentionally no ctor, Occupancy is assumed to be initialized to zero at global scope!

	// PURPOSE:	Registers a string of words in the heap, sharing duplicates when possible
	// PARAMS:	data - Pointer to (potentially shared) data
	//			wordCount - count of data, in words
	// NOTES:	Matching algorithm is naive O(N) implementation, so this isn't intended to
	//			scale well.  Heaps are intended to be small enough to use a single u16 to
	//			store both the offset and payload count in higher-level code.
#if !__SPU
	u16 Register(const _T *data,int count);
	int GetOccupancy() const { return (Occupancy+15)&~15; }		// Round up for DMA
	const _T* GetHeap() const { return Heap; }
#endif

#if __ASSERT
	void ValidateHandle(u16 handle) const {
		int count = (handle >> 10) & 15;
		int offset = (handle & 1023);
		Assert(offset < Occupancy);
		Assert(offset + count <= Occupancy);
	}
#endif
	int GetCount(u16 handle) const { ASSERT_ONLY(ValidateHandle(handle)); return (handle >> 10) & 15; }
	const _T* GetData(u16 handle) const { ASSERT_ONLY(ValidateHandle(handle)); return &Heap[handle & 1023]; }

#if __SPU
	_T *Heap;
#else
private:
	_T Heap[size];
#endif
	int Occupancy;
} ;

struct grcRenderState {
	grceRenderState State;
	union {
		int Int;
		float Float;
	};
	bool operator==(const grcRenderState &that) const { return State==that.State && Int==that.Int; }
	bool operator<(const grcRenderState &that) const { return State<that.State || (State==that.State && Int<that.Int); }
};

const unsigned grcRenderStateHeapSize = 384+16+16+32+64;
extern TinySharedHeap<grcRenderState,grcRenderStateHeapSize> g_RenderStates;

}

#if __XENON
struct D3DVertexDeclaration;
namespace rage {

struct VertexShaderSecretInfo {
	D3DVertexDeclaration *Decl;
	u32 HashCode;
	int Stride;
	char *Streamable;
};

} // namespace rage
#endif // __XENON


#endif // GRCORE_EFFECT_INTERNAL_H 
