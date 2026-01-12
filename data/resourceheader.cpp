//
// data/resourceheader.cpp
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//

#include "resourceheader.h"
#include "resource.h"

#include "diag/trap.h"

namespace rage {

static u32 GenerateHelper(datResourceInfo::Sizes info,size_t chunkSize,datResourceMap &map,u32 ctr,char *srcAddr)
{
	size_t counts[9] = { info.Head16Count, info.Head8Count, info.Head4Count, info.Head2Count,
		info.BaseCount, info.HasTail2, info.HasTail4, info.HasTail8, info.HasTail16 };
	chunkSize <<= (4 + info.LeafShift);

	// Make sure we didn't receive an impossible size progression
	Assert(info.LeafShift > 0 || (!info.HasTail2 && !info.HasTail4 && !info.HasTail8 && !info.HasTail16));
	Assert(info.LeafShift > 1 || (!info.HasTail4 && !info.HasTail8 && !info.HasTail16));
	Assert(info.LeafShift > 2 || (!info.HasTail8 && !info.HasTail16));
	Assert(info.LeafShift > 3 || !info.HasTail16);

	for (u32 c=0; c<9; c++)
	{
		while (counts[c])
		{
			TrapGE(ctr,datResourceChunk::MAX_CHUNKS);
			map.Chunks[ctr].Size = chunkSize;
			map.Chunks[ctr].SrcAddr = srcAddr;
			map.Chunks[ctr].DestAddr = NULL;
			srcAddr += chunkSize;
			++ctr;
			--counts[c];
		}
		chunkSize >>= 1;
	}
	return ctr;
}

void datResourceInfo::GenerateMap(datResourceMap &map) const {
	// The order is virtual extended (8N, 4N, 2N, all optional), then virtual normal (always at least one of these)
	// Followed by physical extended (8N, 4N, 2N, all optional), then physical normal (may be zero of these).
	// The root virtual chunk is always the first virtual normal page.
	Assign(map.VirtualCount,GetVirtualChunkCount());
	Assign(map.PhysicalCount,GetPhysicalChunkCount());
	map.RootVirtualChunk = 0;
	u32 i = GenerateHelper(Virtual,g_rscVirtualLeafSize,map,0,(char*) datResourceFileHeader::c_FIXED_VIRTUAL_BASE);
	Assert(i == map.VirtualCount);
	i = GenerateHelper(Physical,g_rscPhysicalLeafSize,map,i,(char*) datResourceFileHeader::c_FIXED_PHYSICAL_BASE);
	Assert(i == (size_t)(map.VirtualCount + map.PhysicalCount));

	// Zero out the rest.
	map.VirtualBase = NULL;
	map.LastSrc = map.LastDest = 0;
#if RESOURCE_HEADER	
	map.HeaderType = RESOURCE_TYPE_NONE;
#endif
}

#if __NO_OUTPUT
void datResourceInfo::Print(char* /*output*/, size_t /*count*/) const {}
#else
void datResourceInfo::Print(char* output, size_t count) const
{
	datResourceMap map;
	GenerateMap(map);
	map.Print(output, count);
}
#endif // __NO_OUTPUT

} // namespace rage - KS
