// 
// data/resourceheader.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef DATA_RESOURCEHEADER_H
#define DATA_RESOURCEHEADER_H

#include <stddef.h>

#include "data/bitfield.h"

#define RESOURCE_HEADER ((__PPU || __XENON) && !__TOOL && !__RESOURCECOMPILER)
#define RESOURCE_HEADER_DEBUG (RESOURCE_HEADER && SUPPORT_DEBUG_MEMORY_FILL)

#if RESOURCE_HEADER
#define RESOURCE_HEADER_ONLY(x) x
#else
#define RESOURCE_HEADER_ONLY(x)
#endif

namespace rage {

struct datResourceMap;

#if RESOURCE_HEADER
const size_t g_rscHeaderLeafSize = 128;
const size_t g_rscHeaderLeafAlignment = 128;
#endif

const size_t g_rscVirtualLeafSize_PS3 = 4096;
const size_t g_rscPhysicalLeafSize_PS3 = 5504;
const size_t g_rscVirtualLeafSize_WIN32 = 8192;
const size_t g_rscPhysicalLeafSize_WIN32 = 8192;

#if __RESOURCECOMPILER
	extern size_t g_rscVirtualLeafSize;
	extern size_t g_rscPhysicalLeafSize;
#elif __PS3
	const size_t g_rscVirtualLeafSize = g_rscVirtualLeafSize_PS3;
	const size_t g_rscPhysicalLeafSize = g_rscPhysicalLeafSize_PS3;
#else
	const size_t g_rscVirtualLeafSize = g_rscVirtualLeafSize_WIN32;
	const size_t g_rscPhysicalLeafSize = g_rscPhysicalLeafSize_WIN32;
#endif

// These five constants need to add up to 20.
#define RESOURCE_16N_BITS	1
#define RESOURCE_8N_BITS	2
#define RESOURCE_4N_BITS	4
#define RESOURCE_2N_BITS	6
#define RESOURCE_N_BITS		7
CompileTimeAssert(4 + RESOURCE_16N_BITS	+ RESOURCE_8N_BITS + RESOURCE_4N_BITS + RESOURCE_2N_BITS + RESOURCE_N_BITS + 4+4 == 32);

struct datResourceInfo {
	struct Sizes {
		u32 DECLARE_BITFIELD_11(
			LeafShift,4,					// How far left to shift leaf size to get base chunk size (up to 512k)
			Head16Count,RESOURCE_16N_BITS,	// How many chunks that are 16* the base chunk size
			Head8Count,RESOURCE_8N_BITS,	// How many chunks that are 8* the base chunk size
			Head4Count,RESOURCE_4N_BITS,	// How many chunks that are 4* the base chunk size
			Head2Count,RESOURCE_2N_BITS,	// How many chunks that are 2* the base chunk size
			BaseCount,RESOURCE_N_BITS,		// How many chunks that are the base chunk size
			HasTail2,1,						// Set if there is a chunk one half the base chunk size afterward
			HasTail4,1,						// Set if there is a chunk one quarter the base chunk size afterward
			HasTail8,1,						// Set if there is a chunk one eighth the base chunk size afterward
			HasTail16,1,					// Set if there is a chunk one sixteenth the base chunk size afterward
			Version,4);						// Version bits are split between virtual and physical

		u32 GetCount() const 
		{
			return Head16Count + Head8Count + Head4Count + Head2Count + BaseCount + HasTail2 + HasTail4 + HasTail8 + HasTail16;
		}
		u32 GetSize(u32 leafSize) const
		{
			leafSize <<= LeafShift;
			return (((Head16Count << 4) + (Head8Count << 3) + (Head4Count << 2) + (Head2Count << 1) + BaseCount) * leafSize)
				+ (HasTail2?(leafSize>>1):0) + (HasTail4?(leafSize>>2):0) + (HasTail8?(leafSize>>3):0) + (HasTail16?(leafSize>>4):0);
		}
	};

#if __WIN32
#pragma warning(push)
#pragma warning(disable: 4201)
#endif

	Sizes Virtual;
	Sizes Physical;

#if __WIN32
#pragma warning(pop)
#endif

	size_t GetVirtualSize() const { return Virtual.GetSize(g_rscVirtualLeafSize); }

	size_t GetPhysicalSize() const { return Physical.GetSize(g_rscPhysicalLeafSize); }

	size_t GetVirtualChunkSize() const { return g_rscVirtualLeafSize << Virtual.LeafShift; }

	size_t GetPhysicalChunkSize() const { return g_rscPhysicalLeafSize << Physical.LeafShift; }

	size_t GetVirtualChunkCount() const { return Virtual.GetCount(); }

	size_t GetPhysicalChunkCount() const { return Physical.GetCount(); }

	int GetVersion() const { return (Virtual.Version << 4) | (Physical.Version); }

	void SetVersion(int v) { Virtual.Version = v >> 4; Physical.Version = v; }

	void GenerateMap(datResourceMap &map) const;

	void Print(char* output, size_t count) const; 
};

CompileTimeAssert(sizeof(datResourceInfo) == 8);

struct datResourceFileHeader {
	static const u32 c_MAGIC = 0x37435352;		// 'RSC7'

	static const u32 c_FIXED_VIRTUAL_BASE = 0x50000000;
	static const u32 c_FIXED_PHYSICAL_BASE = 0x60000000;

	u32 Magic;				// c_MAGIC above (not kept in an archive)
	int Version;			// Application-defined version information.  (only eight lsb's kept in an archive)

	datResourceInfo Info;		// Size information (kept separately in an archive)

	// Returns true if the header represents a valid resource, byte-swapping it if necessary (which is why this isn't const).
	bool IsValidResource() const;
};


// Simple structure describing a single part of a multi-part resource.
struct datResourceChunk {
	static const u32 MAX_CHUNKS = 128;	// Largest number of chunks in a single read.  Cannot raise past 255 without modifying datResource structures.
	void *SrcAddr;		// Address of this chunk in its original address space
	void *DestAddr;		// Address of this chunk in its final address space
	size_t Size;		// Size of this chunk.  All sizes should be the same except for the last one, which may be smaller.

	bool ContainsSrc(void *test) const { return test >= SrcAddr && test < ((char*)SrcAddr + Size); }

	bool ContainsDest(void *test) const { return test >= DestAddr && test < ((char*)DestAddr + Size); }
};

}	// namespace rage

#endif	// DATA_RESOURCEHEADER_H
