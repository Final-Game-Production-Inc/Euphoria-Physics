//
// data/resource.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "resource.h"
#include "data/resourceheader.h"
#include "paging/rscbuilder.h"
#include "system/endian.h"
#include "system/memory.h"
#include "string/string.h"

namespace rage {

#if !__SPU
__THREAD datResource* datResource_sm_Current;

#if ENABLE_DEFRAGMENTATION
__THREAD bool datResource_IsDefragmentation;
#endif

#if RSG_PC || !__NO_OUTPUT
void datResource::ResourceFailure(const char *msg) const
#else
void datResource::ResourceFailure(const char *) const
#endif // RSG_PC || !__NO_OUTPUT
{
	Quitf(ERR_SYS_INVALIDRESOURCE_1,"Resource '%s': %s",m_DebugName,msg);
}

#if RESOURCE_HEADER
bool datResourceMap::IsOptimized() const
{ 
	return sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_HEADER_VIRTUAL)->IsValidPointer(Chunks[0].DestAddr); 
}
#endif

#if __NO_OUTPUT
void datResourceMap::Print(char* /*output*/, size_t /*count*/) const {}
#else
void datResourceMap::Print(char* output, size_t count) const {
	const char* memory[4] = {"b", "Kb", "Mb", "Gb"};
	const char* memoryTypePrefix[2] = { "M:", "V:" };

	// We have two arrays here, one for virtual (0) and one for physical (1).
	u32 UsedBySize[2][32] = {0};
	size_t LeafSize[2];
	bool anyAllocation[2] = {0};

	size_t virtualCount = VirtualCount;
	size_t physicalCount = PhysicalCount;

	size_t totalCount = virtualCount + physicalCount;

	LeafSize[0] = g_rscVirtualLeafSize;
	LeafSize[1] = g_rscPhysicalLeafSize;

	for (size_t i=0; i<totalCount; i++) {
		bool isPhysical = i >= virtualCount;
		int memoryTypeIndex  = (isPhysical) ? 1 : 0;

		size_t size = pgRscBuilder::ComputeLeafSize(Chunks[i].Size, isPhysical);
		size_t leafSize = LeafSize[memoryTypeIndex];

		size_t level = 0;
		while (size > leafSize) {
			size >>= 1;
			++level;
		}

		UsedBySize[memoryTypeIndex][level]++;
		anyAllocation[memoryTypeIndex] = true;
	}

	char tmp[32];
	output[0] = '\0';

	for (int x=0; x<2; x++)
	{
		if (anyAllocation[x])
		{
			strncat(output, memoryTypePrefix[x], count);
			for (int i = 0; i < 26; ++i)
			{
				if (UsedBySize[x][i])
				{
					size_t size = LeafSize[x] << i;

					int memoryIndex = 0;
					while (size > 1024)
					{
						memoryIndex++;

						size += 1023;
						size >>= 10;
					}

					formatf(tmp, "%dx%d%s ", UsedBySize[x][i], size, memory[memoryIndex]);
					strncat(output, tmp, count);
				}
			}
		}
	}
}
#endif // __NO_OUTPUT

bool datResourceFileHeader::IsValidResource() const
{
#if __WIN32PC	// should be __RESOURCECOMPILER -- not until we get rid of rage gem
	if (Magic == datResourceFileHeader::c_MAGIC)
		return true;
	else if (Magic == sysEndian::Swap(datResourceFileHeader::c_MAGIC))
	{
		u32 *tmp = (u32*) this;
		tmp[0] = sysEndian::Swap(tmp[0]);
		tmp[1] = sysEndian::Swap(tmp[1]);
		tmp[2] = sysEndian::Swap(tmp[2]);
		tmp[3] = sysEndian::Swap(tmp[3]);
		return true;
	}
	else
		return false;
#else
	return Magic == datResourceFileHeader::c_MAGIC;
#endif
}

/*
	TODO list for defragmentation:

	The datResourceMap needs to stick around indefinitely, not just during the streaming process.
	I think the right answer here is to allocate it at the start of the resource heap itself, like we
	used to do with the resource header.  It will be filled in with address range translation information
	after streamin is completed.

	There's also the issue where the toplevel memory allocation won't necessarily be first in the resource
	file any longer.  We could either guarantee that it must be, or we can store the offset to it in
	the datResourceMap as well.

	When constructing a resource heap, if it's done in a single pass we need to do a best fit based on
	whatever leaves are available.  We'll allocate a new leaf that is the next-largest-available 
	power-of-two-multiple of the root page size for that memory type.  Not sure how well this will
	work, though, since we can't merge two leaves smaller than our max allocation size because we
	don't even know it yet.  We may need to use a two-pass method all the time.

	If doing a resource heap in two passes, during the first pass we simply record the size of the object
	and allocate it normally with no buckets.  Then, between the first pass and second pass, sort all
	of the allocations by descending size and then allocate buckets on demand.  If we ever get more than
	one bucket smaller than our max page size, do a (recursive) merge with the next-largest bucket.

	We can deduce the final bucket arrangement solely from its final size (in multiples of a root page size)
	and the size of the largest bucket; while the total size is larger than the largest bucket, allocate
	a bucket the sized of the largest bucket.  Once we fall below the largest bucket size, every set bit
	in the remaining size corresponds to a single bucket of its matching size.

	During defragmentation, how do we get a backpointer to the toplevel resource given any node associated
	with that resource?  (It's necessary in order to reinvoke the rsc ctor).

	When a resource is locked, how do we quickly manage to lock all of the leaves associated with the resource?
	Do we need a special resource handle class in addition to the "internal allocation" handle class?
*/

#endif	// __SPU

} // namespace rage
