//
// system/memory.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_MEMORY_H
#define SYSTEM_MEMORY_H

#include "system/tls.h"
#include "system/new_config.h"
#include "system/memops.h"
#include "system/debugmemoryfill.h"
#include "system/buddyallocator_config.h"

#if RSG_ORBIS
#include <sdk_version.h>
#endif

#define COMMERCE_CONTAINER (__PPU && !__TOOL)
#if COMMERCE_CONTAINER
#define COMMERCE_CONTAINER_ONLY(x) x

// PS3 Commerce Memory
#include <sys/types.h>
#define COMMERCE_IO_SIZE (32 << 20)		//  32 M
#define COMMERCE_HEAP_SIZE (18 << 20)	//  18 M
#define COMMERCE_PRX_SIZE (1 << 20)		//   1 M
CompileTimeAssert(0 == (COMMERCE_HEAP_SIZE & 0xFFFF));	// Make sure we are 64KB aligned
#else
#define COMMERCE_CONTAINER_ONLY(x)
#endif

// NOTE: TO hook system allocs on PC, you need to link to MinHook.x64.lib
#define API_HOOKER (0 && RSG_PC && !__FINAL && !RSG_TOOL)

#if API_HOOKER
#pragma comment (lib, "MinHook.x64.lib")
#endif

// Frag Cache Memory
#if defined(__RGSC_DLL) && __RGSC_DLL
#define FRAG_HEAP_SIZE (0)
#elif __XENON || __PS3
#define FRAG_HEAP_SIZE (4 << 20)		//  4 M
#elif RSG_ORBIS || RSG_DURANGO
#define FRAG_HEAP_SIZE (8 << 20)		// 40 M
#else
#define FRAG_HEAP_SIZE (8 << 20)		// 40 M
#endif

#if RSG_DURANGO
// DON'T FUCK WITH THIS UNLESS YOU KNOW WHAT YOU'RE DOING!
// We have optimized the heap sizes to reduce TLB misses. Change this and you could fuck up performance (by as much as 1+ ms/frame)
CompileTimeAssert(FRAG_HEAP_SIZE % (4 << 20) == 0);
#endif

// EJ: Special preprocessors for memory debugging
#define RAGE_MEMORY_DEBUG ((!__NO_OUTPUT || !__FINAL) && !__TOOL && !__RESOURCECOMPILER)
#if RAGE_MEMORY_DEBUG
#define RAGE_MEMORY_DEBUG_ONLY(x) x
#define RAGE_NOT_MEMORY_DEBUG_ONLY(x)
#define Memoryf Printf
extern void RAGE_LOG_NEW(const void*,size_t,const char*,int);
extern void RAGE_LOG_DELETE(const void*);
#else
#define RAGE_MEMORY_DEBUG_ONLY(x)
#define RAGE_NOT_MEMORY_DEBUG_ONLY(x) x
#define Memoryf Printf
#define RAGE_LOG_NEW(ptr,size,file,line)
#define RAGE_LOG_DELETE(ptr)
#endif

#if RSG_PC || RSG_ORBIS || RSG_DURANGO
#define NETWORK_HEAP_SIZE (13312 << 10)			// 13 M
#elif RSG_ORBIS 
#define NETWORK_HEAP_SIZE (8704 << 10)			// 
#elif __XENON
	#define NETWORK_HEAP_SIZE (3950 << 10)			// 3.85 M
#else
#if __PS3 
	#if __BANK 
		#define NETWORK_HEAP_SIZE (3594 << 10)			// 3.5 M
	#elif !__FINAL   
		#define NETWORK_HEAP_SIZE (3050 << 10)			// Release - from a reading in final (29/05/2013)
	#else
		#define NETWORK_HEAP_SIZE (2910 << 10)			// Final - from a reading in final (29/05/2013)
	#endif
#else
	#define NETWORK_HEAP_SIZE (3584 << 10)			// 3.5 M
#endif 
#endif // __XENON

#if RSG_PC || RSG_ORBIS || RSG_DURANGO
	#define MOVEPED_HEAP_SIZE (4096 << 10)
#else
	#define MOVEPED_HEAP_SIZE (1280 << 10)
#endif

#if defined(__RGSC_DLL) && __RGSC_DLL
#define MEMORY_TRACKER 0
#else
#define MEMORY_TRACKER (1 && (__XENON || RSG_DURANGO || RSG_PC) && !__FINAL && !RSG_TOOL)
#endif

#define MEM_VALIDATE_USERDATA	((__DEV || __BANK) && (__PS3 || __XENON))

#define MEM_INVALID_USERDATA		(0)				// Should be 0 because sysMemCurrentUserData is TLS, so it defaults to MEM_INVALID_USERDATA
#define MEM_LAST_CUSTOM_USERDATA	(0xfffffe)

#if RAGE_ENABLE_RAGE_NEW

// PURPOSE: Use in place of normal call to Allocate() to have the allocation logged for you.
#	define RAGE_LOG_ALLOCATE(size,align) LoggedAllocate(size, align, 0, __FILE__, __LINE__)

// PURPOSE: Use in place of normal call to Allocate() to have the allocation logged for you.
#	define RAGE_LOG_ALLOCATE_HEAP(size,align,heapIndex) LoggedAllocate(size, align, heapIndex, __FILE__, __LINE__)

#else

// PURPOSE: Use in place of normal call to Allocate() to have the allocation logged for you.
#	define RAGE_LOG_ALLOCATE(size,align) Allocate(size, align, 0)

// PURPOSE: Use in place of normal call to Allocate() to have the allocation logged for you.
#	define RAGE_LOG_ALLOCATE_HEAP(size,align,heapIndex) Allocate(size, align, heapIndex)

#endif

namespace rage {

class bkGroup;
struct datResourceMap;

// PURPOSE: Current memory bucket; all memory allocations get tagged with
//			this bucket number, allowing us to determine memory usage by type.
//			The bucket number is currently a small non-negative integer.
// SEE ALSO:	MEMBUCKET_MAXBUCKETS
extern __THREAD int sysMemCurrentMemoryBucket;

#if MEM_VALIDATE_USERDATA
extern __THREAD u32 sysMemCurrentUserData;
#endif // MEM_VALIDATE_USERDATA

extern __THREAD int sysMemAllowResourceAlloc;

#if __ASSERT
extern __THREAD int sysMemStreamingCount;
#endif

// Don't use virtual interface on SPU
#if __SPU
#define SYS_MEM_VIRTUAL
#define SYS_MEM_PURE_VIRTUAL
#else
#define SYS_MEM_VIRTUAL virtual
#define SYS_MEM_PURE_VIRTUAL = 0
#endif

enum MemoryBucketIds
{
	MEMBUCKET_INVALID = -1,
	MEMBUCKET_DEFAULT =  0,
	MEMBUCKET_ANIMATION,
	MEMBUCKET_STREAMING,
	MEMBUCKET_WORLD,
	MEMBUCKET_GAMEPLAY,
	MEMBUCKET_FX,
	MEMBUCKET_RENDER,
	MEMBUCKET_PHYSICS,
	MEMBUCKET_AUDIO,
	MEMBUCKET_NETWORK,
	MEMBUCKET_SYSTEM,
	MEMBUCKET_SCALEFORM,
	MEMBUCKET_SCRIPT,
	MEMBUCKET_RESOURCE,
	MEMBUCKET_UI,
	MEMBUCKET_DEBUG = 15
};

enum MemoryCustomUserData
{
	MEMUSERDATA_DRAWPAGE			= MEM_LAST_CUSTOM_USERDATA - 5, // This will include +0, +1, +2, +3, +4 - see eDrawListPageType in drawlist.h
	MEMUSERDATA_SCALEFORM			= MEM_LAST_CUSTOM_USERDATA - 6, // Scaleform-related allocations
	MEMUSERDATA_TEST_ALLOC			= MEM_LAST_CUSTOM_USERDATA - 7, // These are done by the streaming system to test the waters and see whether or not we have enough memory available
	MEMUSERDATA_PLANTMGR			= MEM_LAST_CUSTOM_USERDATA - 8,
	MEMUSERDATA_BINK				= MEM_LAST_CUSTOM_USERDATA - 9,
	MEMUSERDATA_MESH_ALLOC			= MEM_LAST_CUSTOM_USERDATA - 10,
	MEMUSERDATA_CLOTH				= MEM_LAST_CUSTOM_USERDATA - 11,
	MEMUSERDATA_PEDHEADSHOT			= MEM_LAST_CUSTOM_USERDATA - 12,
	MEMUSERDATA_SAVEGAME			= MEM_LAST_CUSTOM_USERDATA - 13,
	MEMUSERDATA_VEHGLASS			= MEM_LAST_CUSTOM_USERDATA - 14,
	MEMUSERDATA_XGRAPHICS			= MEM_LAST_CUSTOM_USERDATA - 15,
	MEMUSERDATA_BGGLASS				= MEM_LAST_CUSTOM_USERDATA - 16,
	MEMUSERDATA_DL_MANAGER			= MEM_LAST_CUSTOM_USERDATA - 17,
	MEMUSERDATA_MESHBLEND			= MEM_LAST_CUSTOM_USERDATA - 18,
	MEMUSERDATA_MESHBLENDX			= MEM_LAST_CUSTOM_USERDATA - 19,
	MEMUSERDATA_VISEME				= MEM_LAST_CUSTOM_USERDATA - 20,
	MEMUSERDATA_TEXLITE				= MEM_LAST_CUSTOM_USERDATA - 21,
	MEMUSERDATA_POPULATION			= MEM_LAST_CUSTOM_USERDATA - 22,
	MEMUSERDATA_SCREENSHOT			= MEM_LAST_CUSTOM_USERDATA - 23,
	MEMUSERDATA_DEBUGSTEAL			= MEM_LAST_CUSTOM_USERDATA - 24,
	MEMUSERDATA_ASSETSTORE			= MEM_LAST_CUSTOM_USERDATA - 25,	// This will include -0 through -31
	MEMUSERDATA_STRINFO				= MEM_LAST_CUSTOM_USERDATA - 57,
	MEMUSERDATA_NETWORKING			= MEM_LAST_CUSTOM_USERDATA - 58,
	MEMUSERDATA_BREAKABLEGLASS		= MEM_LAST_CUSTOM_USERDATA - 59,
	MEMUSERDATA_PARSER				= MEM_LAST_CUSTOM_USERDATA - 60,
	MEMUSERDATA_CACHE_LOADER		= MEM_LAST_CUSTOM_USERDATA - 61,
	MEMUSERDATA_VIDEO_TRANSCODING	= MEM_LAST_CUSTOM_USERDATA - 62,

	MEM_FIRST_CUSTOM_USER_DATA		= MEM_LAST_CUSTOM_USERDATA - 63
};

#if RSG_DURANGO
enum PhysicalMemoryType
{
	// Writeback is the standard CPU caching model. The CPU can hold modified
	// cache lines until some other device needs to access them, at which time
	// they are written back to memory.

	// Writecombine is somewhat similar to non-cached on the CPU, except that
	// the CPU can combine together writes before committing them to memory.
	// Writecombining memory is slow for the CPU to read.

	// The GPU onion bus is coherent between CPU and GPU, but has lower
	// bandwidth for GPU access.

	// The GPU garlic bus has higher GPU bandwidth, but is not automatically
	// coherent with the CPU caches.
	//
	// When garlic is used with CPU writecombine, then there is no coherency
	// problem, the CPU is not caching the data. GPU caches may still need to
	// be manually invalidated or flushed.
	//
	// When garlic is used with writeback, then code must explicitly handle both
	// CPU and GPU cache flushes. Additionally there is no way to prevent
	// memory corruption to garlic/writeback if the GPU writes to this memory;
	// the CPU's speculative execution can cause overwrites of the data written
	// by the GPU. Because of this memory corruption issue, GARLIC_WRITEBACK is
	// read only for the GPU.
	//
	PHYS_MEM_ONION_WRITEBACK,
	PHYS_MEM_ONION_WRITECOMBINE,
	PHYS_MEM_GARLIC_WRITEBACK, // GPU cannot write to this memory otherwise the CPU can corrupt it)
	PHYS_MEM_GARLIC_WRITECOMBINE,
	//
	PHYS_MEM_DEFAULT = PHYS_MEM_ONION_WRITEBACK,
	PHYS_MEM_MIN = PHYS_MEM_ONION_WRITEBACK,
	PHYS_MEM_MAX = PHYS_MEM_GARLIC_WRITECOMBINE	
};
#endif

#define USE_MEMBUCKET(bucket)	sysMemUseMemoryBucket mem_##bucket(bucket);

#define USE_DEBUG_MEMORY()		sysMemUseMemoryBucket mem_debug(MEMBUCKET_DEBUG); sysMemAutoUseDebugMemory auto_debug

#if MEM_VALIDATE_USERDATA
	#define MEM_USE_USERDATA(userData)	sysMemSetUserData memUserData(userData);
#else // MEM_VALIDATE_USERDATA
	#define MEM_USE_USERDATA(userData)
#endif // MEM_VALIDATE_USERDATA

#define ENABLE_DEBUG_HEAP		(!__FINAL && !__TOOL && !__GAMETOOL && !__RESOURCECOMPILER)
#define ENABLE_DEBUG_POOL		((__BANK || (__XENON && !__FINAL)) && !__TOOL && !__RESOURCECOMPILER)

#ifndef ENABLE_DEBUG_HEAP_ONLY
#if ENABLE_DEBUG_HEAP
#define ENABLE_DEBUG_HEAP_ONLY(x) x
#else
#define ENABLE_DEBUG_HEAP_ONLY(x)
#endif
#endif

enum eMemoryType {
	MEMTYPE_GAME_VIRTUAL,
	MEMTYPE_RESOURCE_VIRTUAL,
	MEMTYPE_GAME_PHYSICAL,
	MEMTYPE_RESOURCE_PHYSICAL,
#if __PS3 || __XENON
	MEMTYPE_HEADER_VIRTUAL,
#endif
	MEMTYPE_DEBUG_VIRTUAL,		// will be an alias for MEMTYPE_GAME_VIRTUAL if debug heap is not available.

	MEMTYPE_COUNT				// Total POSSIBLE number of heaps (some of them might not be available)
};

#define IS_RESOURCE_MEMORY_TYPE(A) ((A)==MEMTYPE_RESOURCE_PHYSICAL || (A)==MEMTYPE_RESOURCE_VIRTUAL)
/*
	sysUseMemoryBucket is a convenience class which simply makes a particular memory
	bucket active and then restores the previously active bucket when this object
	falls out of scope; useful to avoid incorrectly-billed allocations due to an exit
	path of a function not properly restoring things.
*/
class sysMemUseMemoryBucket
{
#if !__FINAL
public:
	// PURPOSE: Constructor
	// PARAMS:	bucket - New bucket to bill memory allocations to, or -1 to keep the current one
	sysMemUseMemoryBucket(int bucket)	{
		m_OldBucket=sysMemCurrentMemoryBucket;
		if (bucket != -1)
			sysMemCurrentMemoryBucket=bucket;
	}

	static void SwitchTo(int bucket) {
		sysMemCurrentMemoryBucket=bucket;
	}

	// PURPOSE: Destructor; restores previous memory bucket
	~sysMemUseMemoryBucket()			{sysMemCurrentMemoryBucket=m_OldBucket;}
private:
	int m_OldBucket;
#else
public:
	sysMemUseMemoryBucket(int UNUSED_PARAM(bucket))	{}
	~sysMemUseMemoryBucket()						{}
#endif // !__FINAL
};

#if MEM_VALIDATE_USERDATA
class sysMemSetUserData
{
public:
	// PURPOSE: Constructor
	// PARAMS:	userData - New userData to use to check memory frees
	sysMemSetUserData(u32 userData)	{
		m_OldUserData=sysMemCurrentUserData;
		sysMemCurrentUserData = userData;
	}

	// PURPOSE: Destructor; restores previous memory bucket
	~sysMemSetUserData() {
		sysMemCurrentUserData = m_OldUserData;
	}
private:
	u32 m_OldUserData;
};
#endif // MEM_VALIDATE_USERDATA


class sysTinyHeap;
class sysMemAllocator;
COMMERCE_CONTAINER_ONLY(class VirtualMemHeap;)
extern __THREAD sysMemAllocator* sysMemAllocator_sm_Current;
extern __THREAD sysMemAllocator* sysMemAllocator_sm_Master;
extern __THREAD sysMemAllocator* sysMemAllocator_sm_Container;
extern __THREAD int sysMemAllocator_sm_LockHeap;

extern void (*sysMemOutOfMemoryDisplay)();

// Global
#if __ASSERT
typedef void (*VerifyMainThread)();
extern VerifyMainThread sysMemVerifyMainThread;
#endif

struct sysMemDistribution {
	u32 UsedBySize[32];		// Number of blocks no larger than 1 << N that are in use
	u32 FreeBySize[32];		// Number of blocks no larger than 1 << N that are free
};

struct datResourceMap;

struct sysMemDefragmentation {
	static const int c_MAX_TO_MOVE = 32;		// Keep in sync with sysBuddyHeapDefragInfo
	int Count;				// Number of nodes that need to be moved
	struct {
		void *From,			// Source of the node (original address)
				*To;		// Destination for node (final address)
		u32 Size;			// Size of the node, in bytes
	} Nodes[c_MAX_TO_MOVE];
};

struct sysMemDefragmentationFree 
{
	static const int c_MAX_TO_FREE = 4;		// Keep in sync with sysBuddyHeapDefragInfo
	int Count;				// Number of nodes that need to be moved
	void* Nodes[c_MAX_TO_FREE];
};

struct sysMemoryWatermark
{
	sysMemoryWatermark() : HeapSize(0), LowWatermark(0x7FFFFFFF), MemoryAvailable(0), LargestBlock(0), Fragmentation(0), Dirty(false) {}

	size_t HeapSize;		// Size of the heap
	size_t LowWatermark;	// Lowest memory available - see GetLowWaterMark
	size_t MemoryAvailable; // Current memory available
	size_t LargestBlock;	// Largest block of memory
	size_t Fragmentation;	// Fragmentation % (0-100) - see GetFragmentation
	bool   Dirty;			// New low watermark has been hit but not consumed by tracking code.
};

/*
	This class provides an abstract interface for all memory allocation.
	It replaces several ad-hoc function pointers in the previous revision
	of this code and is much cleaner and easier to extend.  By default on
	all non-__TOOL builds, data/main.h includes definitions for operator
	rage_new and operator delete which are routed through to this class.
	The system always provides a default allocator object that is valid
	at all times, so it is currently safe to allocate memory even in
	global constructors.  This default allocator simply uses malloc(3)
	and free(3).
*/
class sysMemAllocator {
public:
    SYS_MEM_VIRTUAL ~sysMemAllocator();

    //PURPOSE
    //  Configures how the allocator behaves when an allocation fails.
    //PARAMS
    //  quitOnFail  - If true, an allocation failure will Quitf().
    //                If false, an allocation failure will return NULL
    //                from Allocate().
    //RETURNS
    //  Old quit-on-fail value.
    SYS_MEM_VIRTUAL bool SetQuitOnFail(const bool quitOnFail);

	// PURPOSE:	Allocate memory with a specified alignment
	// PARAMS:	size - Amount of memory to allocate.  
	//				Zero bytes is treated as one byte for compliance with
	//				some microsoft code even though the C++ standard is
	//				unclear on this point.
	//			align - Memory alignment to enforce.  Ignored by stock allocator.
	//			heapIndex - Which subheap (if any) to allocate the memory from.
	// RETURNS:	Pointer to newly allocated memory.  Will never return NULL.
	SYS_MEM_VIRTUAL void* Allocate(size_t size,size_t align,int heapIndex = 0) SYS_MEM_PURE_VIRTUAL;

    // PURPOSE:	Try to allocate memory with a specified alignment.
    //          Return NULL on failure.
	// PARAMS:	size - Amount of memory to allocate.  
	//				Zero bytes is treated as one byte for compliance with
	//				some microsoft code even though the C++ standard is
	//				unclear on this point.
	//			align - Memory alignment to enforce.  Ignored by stock allocator.
	//			heapIndex - Which subheap (if any) to allocate the memory from.
	// RETURNS:	Pointer to newly allocated memory.  Can return NULL.
	SYS_MEM_VIRTUAL void* TryAllocate(size_t size,size_t align,int heapIndex = 0);

	// PURPPOSE: Wrapper around allocate to facilitate allocation logging.
	// PARAMS:	size - Amount of memory to allocate.  
	//				Zero bytes is treated as one byte for compliance with
	//				some microsoft code even though the C++ standard is
	//				unclear on this point.
	//			align - Memory alignment to enforce.  Ignored by stock allocator.
	//			heapIndex - Which subheap (if any) to allocate the memory from.
	//			fileName - File from where the allocation was requested (usually provided by the RAGE_LOG_ALLOCATE macro)
	//          lineNumber - Line number from where the allocation was requested (usually provided by the RAGE_LOG_ALLOCATE macro)
	// RETURNS:	Pointer to newly allocated memory.  Will never return NULL.
	inline void* LoggedAllocate(size_t size,size_t align,int heapIndex,const char* fileName,int lineNumber);

	// PURPPOSE: Wrapper around TryAllocate to facilitate allocation logging.
	// PARAMS:	size - Amount of memory to allocate.  
	//				Zero bytes is treated as one byte for compliance with
	//				some microsoft code even though the C++ standard is
	//				unclear on this point.
	//			align - Memory alignment to enforce.  Ignored by stock allocator.
	//			heapIndex - Which subheap (if any) to allocate the memory from.
	//			fileName - File from where the allocation was requested (usually provided by the RAGE_LOG_ALLOCATE macro)
	//          lineNumber - Line number from where the allocation was requested (usually provided by the RAGE_LOG_ALLOCATE macro)
	// RETURNS:	Pointer to newly allocated memory.  Can return NULL.
	inline void* TryLoggedAllocate(size_t size,size_t align,int heapIndex,const char* fileName,int lineNumber);

	// PURPOSE:	Free memory allocated via Allocate.
	// PARAMS:	ptr - Pointer to memory to free.
	SYS_MEM_VIRTUAL void Free(const void *ptr) SYS_MEM_PURE_VIRTUAL;
	SYS_MEM_VIRTUAL void DeferredFree(const void *ptr);

	// PURPOSE:	Change the amount of memory associated with this allocation.  Must be same
	//			size or smaller than original allocation.  Memory never moves.
	SYS_MEM_VIRTUAL void Resize(const void * /*ptr*/,size_t /*newSmallerSize*/) {
		AssertMsg(false,"sysMemAllocator::Resize not implemented.");
	}

	// PURPOSE: Return the subheap of this allocator. If this particular implementation
	// of the allocator (including the default implementation) does not have the concept
	// of "subheaps", this function will return itself.
	//
	// PARAMS: heapIndex - This is the subheap - the index of the heap inside
	// this allocator. If this allocator does not have subheaps, this value is ignored.
	SYS_MEM_VIRTUAL const sysMemAllocator *GetAllocator(int UNUSED_PARAM(heapIndex)) const
	{
		return this;
	}

	SYS_MEM_VIRTUAL sysMemAllocator *GetAllocator(int UNUSED_PARAM(heapIndex))
	{
		return this;
	}
	SYS_MEM_VIRTUAL sysMemAllocator *GetPointerOwner(const void *ptr)
	{
		if (IsValidPointer(ptr))
			return this;

		return NULL;
	}

	// PURPOSE:	Returns actual amount of memory (sans node overhead) associated with the allocation.
	// PARAMS:	ptr - Pointer to check data size on
	// RETURNS:	0 if feature is not supported, or ptr is NULL or otherwise invalid;
	//			otherwise, actual usable amount of memory in the block.
	SYS_MEM_VIRTUAL size_t GetSize(const void *ptr) const;

	// PURPOSE:	Returns amount of memory used in a particular bucket
	// PARAMS:	bucket - Bucket index (eMemoryBucket above), or -1
	//				for all buckets combined.  If HasMemoryBuckets
	//				returns false, the parameter to this function is
	//				ignored and you will always receive the total
	//				amount of memory used (if that is available).
	// RETURNS:	Amount of memory used, or zero if unknown
	// NOTES:	The "all bucket" total will generally be higher than
	//			the cumulative sum of the "per bucket" values because
	//			there is typically allocation overhead.
	SYS_MEM_VIRTUAL size_t GetMemoryUsed(int bucket = -1 /*all buckets*/) SYS_MEM_PURE_VIRTUAL;

	// PURPOSE:	Returns amount of memory remaining in the system
	// RETURNS:	Amount of memory available, in bytes, or zero if unknown
	SYS_MEM_VIRTUAL size_t GetMemoryAvailable() SYS_MEM_PURE_VIRTUAL;

	// PURPOSE:	Returns largest amount of memory that can be allocated in a single contiguous block
	// RETURNS:	Amount of memory available in a contiguous block, in bytes, or zero if unknown
	SYS_MEM_VIRTUAL size_t GetLargestAvailableBlock() { return 0; };

	SYS_MEM_VIRTUAL size_t GetLowWaterMark(bool /*reset*/) { return 0; }

	SYS_MEM_VIRTUAL size_t GetHighWaterMark(bool /*reset*/) { return 0; }

	SYS_MEM_VIRTUAL void UpdateMemorySnapshot() { }

	SYS_MEM_VIRTUAL size_t GetMemorySnapshot(int /*bucket*/) {return 0;}

#if !__FINAL
	// PURPOSE:	Specify the allocation number to stop execution on
	// PARAMS:	id - Allocation number to stop at, or -1 to disable feature
	// NOTES:	This is for the -breakonalloc command line option. To use it, run without it (or set it to 0) and get the allocation numbers of
	//			any leaks from the output when exiting. Then run with -breakonalloc followed by the allocation number of a leak.
	static void SetBreakOnAlloc(int id) { sm_BreakOnAlloc = id; }

	// PURPOSE:	Specify the allocation address to stop execution on
	// PARAMS:	addr - Allocation address to stop at, or (void*)~0U to disable feature
	// NOTES:	This is for the -breakonadrr command line option.
	static void SetBreakOnAddr(void *addr) { sm_BreakOnAddr = addr; }
#endif

	// PURPOSE: Disable all allocations for this thread.  Will nest.
	static void LockHeap() { ++sysMemAllocator_sm_LockHeap; }

	// PURPOSE: Re-enable allocations for this thread.
	static void UnlockHeap() { --sysMemAllocator_sm_LockHeap; }

	// PURPOSE: Determine whether heap is locked.
	static bool IsHeapLocked() { return sysMemAllocator_sm_LockHeap != 0; }

	// PURPOSE: Determine whether heap needs tallied.
	SYS_MEM_VIRTUAL bool IsTallied() { return true; } // KS

	// PURPOSE:	Begin a memory layer; any memory allocations made
	//			while this layer is active will be flagged if they
	//			are left unfreed when EndLayer is called.
	SYS_MEM_VIRTUAL void BeginLayer() { }

	// PURPOSE:	End a memory layer, reporting any unfreed allocations.
	// PARAMS:	layerName - human-readable layer name, for annotating leaks.
	//			leakfile - If non-null, every leak gets written to this file
	// RETURNS:	Nonzero if any leaks were found; if possible, it's the actual
	//			number of leaks if that information is available.
	SYS_MEM_VIRTUAL int EndLayer(const char * /*layerName*/,const char * /*leakfile*/) { return 0; }

	// PURPOSE:	Begins memory logging to specified filename
	// PARAMS:	filename - Name of log file to create
	//			logStackTracebacks - True to enable stack tracebacks with
	//				every memory allocation.
	// NOTES:	This logs the allocation id, size and active bucket to the file, optionally
	//			along with a stack traceback (which is currently very slow, so use sparingly)
	SYS_MEM_VIRTUAL void BeginMemoryLog(const char * /*filename*/,bool /*logStackTracebacks*/) { }

	// PURPOSE:	Close memory log.  
	SYS_MEM_VIRTUAL void EndMemoryLog() { }

	// RETURNS:	True if a resource heap is currently being built
	SYS_MEM_VIRTUAL bool IsBuildingResource() const { return false; }

	// RETURNS:	True if this allocator supports memory buckets
	SYS_MEM_VIRTUAL bool HasMemoryBuckets() const { return false; }

	// PURPOSE: Run a sanity check on the heap and halt the application
	//			if any inconsistencies are found.
	SYS_MEM_VIRTUAL void SanityCheck() { }

	// RETURNS:	True if this is a valid heap pointer owned by this heap.
	SYS_MEM_VIRTUAL bool IsValidPointer(const void * /*ptr*/) const { return true; }

	virtual bool SupportsAllocateMap() const { return false; }
	
	virtual bool AllocateMap(datResourceMap &) { return false; }
	virtual void FreeMap(const datResourceMap &) { }

    // PURPOSE:	Change the currently active allocator object
	// PARAMS:	current - New allocator to make current
	static void SetCurrent(sysMemAllocator &current) {
		sysMemAllocator_sm_Current = &current;
	}

    // RETURNS:	True if the currently active heap allocator is already set
    static bool IsCurrentSet() {
        return sysMemAllocator_sm_Current != 0;
    }

	// RETURNS:	Currently active allocator object
	static sysMemAllocator& GetCurrent() {
		return *sysMemAllocator_sm_Current;
	}

	// RETURNS:	Reference to the master allocator, which is the memory
	//			allocator we first attach in InitGameHeap (in system/main.h)
	static sysMemAllocator& GetMaster() {
		return *sysMemAllocator_sm_Master;
	}

	// RETURNS:	Sets the master allocator, which is the memory
	//			allocator we first attach in InitGameHeap (in system/main.h)
	static void SetMaster(sysMemAllocator& m) {
		sysMemAllocator_sm_Master = &m;
	}

	static sysMemAllocator& GetContainer() {
		return *sysMemAllocator_sm_Container;
	}

	static sysMemAllocator &SetContainer(sysMemAllocator& m) {
		sysMemAllocator &prev = *sysMemAllocator_sm_Container;
		sysMemAllocator_sm_Container = &m;
		return prev;
	}

#if !__FINAL
	static void SetBucketName(s32 bucket, const char * name) {
		sm_MemoryBucketNames[bucket] = name;
	}

	static const char *GetBucketName(s32 bucket) {
		return sm_MemoryBucketNames[bucket];
	}

#endif

	// PURPOSE:	Returns actual amount of memory (WITH node overhead) associated with the allocation.
	// PARAMS:	ptr - Pointer to check data size on
	// RETURNS:	0 if feature is not supported, or ptr is NULL or otherwise invalid;
	//			otherwise, actual amount of memory consumed by the block
	SYS_MEM_VIRTUAL size_t GetSizeWithOverhead(const void *ptr) const { return GetSize(ptr); }

	// RETURNS:	Total size of this heap, if available
	SYS_MEM_VIRTUAL size_t GetHeapSize() const { FastAssert(false); return 0; }

	// RETURNS:	Base address of the heap
	SYS_MEM_VIRTUAL void *GetHeapBase() const { FastAssert(false); return NULL; }

	// PURPOSE:	Sets the heap base address.  FOR INTERNAL USE ONLY
	SYS_MEM_VIRTUAL void SetHeapBase(void*) { FastAssert(false); }

	// PURPOSE: Determine if this right after the root allocation of a resource heap.
	SYS_MEM_VIRTUAL bool IsRootResourceAllocation() const { return false; }

	// PURPOSE: Given a pointer, get a canonical pointer to the block the allocation is from.
	//          If the allocator does not support defragmenting, will just return NULL.
	SYS_MEM_VIRTUAL const void *GetCanonicalBlockPtr(const void * /*ptr*/) const { return NULL; }

	// PURPOSE: Given a pointer, get a canonical pointer to the block the allocation is from.
	//          If the allocator does not support defragmenting, will just return NULL.
	void *GetCanonicalBlockPtr(void *ptr) const { return const_cast<void*>(GetCanonicalBlockPtr(const_cast<const void*>(ptr))); }

	// PURPOSE: Attempt to lock a memory block from being moved by memory defragmentation.
	//          This will fail if the block is currently being defragged.
	//          Allocators that don't support defragmentation will always succeed.
	SYS_MEM_VIRTUAL bool TryLockBlock(const void * /*block*/, unsigned lockCount=1) { (void)lockCount; return true; }

	SYS_MEM_VIRTUAL void UnlockBlock(const void * /*block*/, unsigned unlockCount=1) { (void)unlockCount; }

#if __TOOL || __RESOURCECOMPILER || __WIN32PC || RSG_ORBIS
	enum { DEFAULT_ALIGNMENT = 16 };
#else
	// EJ: Memory optimization that saves between 450K - 750K
	enum { DEFAULT_ALIGNMENT = sizeof(void*) };
#endif	

// If older than VS2010, C++11 enum types generate a warning:
// "nonstandard extension used: specifying underlying type for enum ''"
// VS2008, sort of supports typed enums, but not very well, often requires an additional cast when using the enum in a numerical expression.
// GTA is still using the ancient VS2008 for ragebuilder.
#if defined(_MSC_VER) && (_MSC_VER <= 1600 || RSG_TOOL)
	#pragma warning(push)
	#pragma warning(disable:4480)
#endif

	enum : u32 { INVALID_USER_DATA = ~0u, DEFAULT_ALIGNMENT_MASK = (DEFAULT_ALIGNMENT - 1) };

#if defined(_MSC_VER) && (_MSC_VER <= 1600 || RSG_TOOL)
	#pragma warning(pop)
#endif

	// PURPOSE: Get the user data associated with an allocation
	SYS_MEM_VIRTUAL u32 GetUserData(const void *) const { FastAssert(false); return (u32)INVALID_USER_DATA; }

	SYS_MEM_VIRTUAL void SetUserData(const void *, u32 /*userData*/) { FastAssert(false);  }

	SYS_MEM_VIRTUAL bool GetMemoryDistribution(sysMemDistribution & /*outDist*/) { return false; }

	SYS_MEM_VIRTUAL bool Defragment(sysMemDefragmentation& /*outDefrag*/, sysMemDefragmentationFree& /*outDefragFree*/, size_t /*maxSize*/) { return false; }

	// RETURNS: Amount of fragmentation, as a percentage (0-100).  Returns -1 if unknown.
	// NOTES:	Typical implementation is 1.0f - (largestAvailable / totalAvailable)
	SYS_MEM_VIRTUAL size_t GetFragmentation();

#if __BANK
	SYS_MEM_VIRTUAL void AddWidgets(bkGroup& /*grp*/) { }
#endif

#if !__FINAL
	// One memory breakpoint for all allocators and all threads.
	static int sm_BreakOnAlloc;
	static void *sm_BreakOnAddr;
#endif

#if !__NO_OUTPUT
	// Return value is to allow callbacks to be chained.  Callback should return
	// true if it knows how to display debug info for the pointer.  Other
	// callbacks in the chain may then choose to silently print nothing once a
	// previous callback has returned true.
	static bool (*sm_DebugPrintAllocInfo)(const void *ptr);
	static void DebugPrintAllocInfo(const void *ptr) { if (sm_DebugPrintAllocInfo) sm_DebugPrintAllocInfo(ptr); }
#else
	static void DebugPrintAllocInfo(const void*) {}
#endif

#if !__FINAL
	static const char * sm_MemoryBucketNames[16];
#endif
};

#if RAGE_MEMORY_DEBUG

inline void* sysMemAllocator::LoggedAllocate(size_t size,size_t align,int heapIndex,const char* fileName,int lineNumber)
{
	void* result = Allocate(size,align,heapIndex);
	if (result && fileName)
		RAGE_LOG_NEW(result, size, fileName, lineNumber);
	return result;
}

inline void* sysMemAllocator::TryLoggedAllocate(size_t size,size_t align,int heapIndex,const char* fileName,int lineNumber)
{
	void* result = TryAllocate(size,align,heapIndex);
	if (result && fileName)
		RAGE_LOG_NEW(result, size, fileName, lineNumber);
	return result;
}

#else

inline void* sysMemAllocator::LoggedAllocate(size_t size,size_t align,int heapIndex,const char*,int)
{
	return Allocate(size,align,heapIndex);
}

inline void* sysMemAllocator::TryLoggedAllocate(size_t size,size_t align,int heapIndex,const char*,int)
{
	return TryAllocate(size,align,heapIndex);
}

#endif

// PURPOSE:	Convenience wrapper for sysMemAllocator::GetCurrent().GetMemoryUsed(bucket)
inline size_t sysMemGetMemoryUsed(int bucket = -1) {
	return sysMemAllocator::GetCurrent().GetMemoryUsed(bucket);
}

// PURPOSE:	Convenience wrapper for sysMemAllocator::GetCurrent().GetMemoryAvailable()
inline size_t sysMemGetMemoryAvailable(bool /*ignored*/ = false) {
	return sysMemAllocator::GetCurrent().GetMemoryAvailable();
}


// PURPOSE:	Switches from any resource heap to the main heap; useful for temporary allocations.
//			Calls will properly nest
extern void sysMemStartTemp();

// PURPOSE:	Switches back from main heap to resource heap.
extern void sysMemEndTemp();

// PURPOSE: Switches to the "pool heap" - there's limits in place on it to prevent excessive
//			usage!
extern void sysMemStartPool();

// PURPOSE: Switches back to the original heap from pool
extern void sysMemEndPool();

#if ENABLE_DEBUG_HEAP
extern void sysMemStartDebug();
extern void sysMemEndDebug();
extern bool sysMemDebugIsActive();
extern bool g_sysHasDebugHeap;
extern int g_sysDirectDebugEnabled;
extern int g_sysDirectVirtualEnabled;
extern int g_sysDirectFlexEnabled;

extern void sysMemStartRecorder();
extern void sysMemEndRecorder();
extern bool sysMemRecorderIsActive();
#else
#define sysMemStartDebug()
#define sysMemEndDebug()
#define sysMemDebugIsActive() (false)

#define sysMemStartRecorder()
#define sysMemEndRecorder()
#define sysMemRecorderIsActive() (false)

// Intentionally leave g_sysHasDebugHeap undefined, code should be protected with ENABLE_DEBUG_HEAP
#endif

struct sysMemAutoUseTempMemory
{
	sysMemAutoUseTempMemory() {sysMemStartTemp();}
	~sysMemAutoUseTempMemory() {sysMemEndTemp();}
};

struct sysMemAutoUseDebugMemory
{
	sysMemAutoUseDebugMemory() {sysMemStartDebug();}
	~sysMemAutoUseDebugMemory() {sysMemEndDebug();}
};

struct sysMemAutoUseAllocator
{
	sysMemAutoUseAllocator(sysMemAllocator& alloc) : m_OldAlloc(sysMemAllocator::GetCurrent()) { sysMemAllocator::SetCurrent(alloc); }
	~sysMemAutoUseAllocator() { sysMemAllocator::SetCurrent(m_OldAlloc); }
private:
	sysMemAllocator& m_OldAlloc;
};

// PURPOSE:	Current fill value used by debug_memory_fill
extern unsigned sysMemoryFillWord;

// PURPOSE:	If true, fill allocated memory with sysMemoryFillWord value
extern bool sysEnableMemoryFill;

// PURPOSE: Allocate an aligned block of physically contiguous memory
// PARAMS
//   size - The size of the needed block
//   alignSize - The granularity (in bytes) to which the memory block needs to be aligned, MUST be a power of two
// RETURNS: A pointer to the newly allocated block of memory, or NULL if insufficient memory.
void* physical_new_(size_t size,size_t align RAGE_NEW_EXTRA_ARGS);

#if RAGE_ENABLE_RAGE_NEW
#define physical_new(s,a)	physical_new_((s),(a),__FILE__,__LINE__)
#else
#define physical_new(s,a)	physical_new_((s),(a))
#endif

// PURPOSE: Delete a block of memory allocated with physical_new
// PARAMS
//   ptr - The pointer returned by physical_new
void physical_delete(void *ptr);

// PURPOSE: fill memory with sysMemoryFillWord
// PARAMS
//   ptr - A pointer to the block of memory to be filled
//   bytes - The number of bytes to fill
#if SUPPORT_DEBUG_MEMORY_FILL
void debug_memory_fill(void *ptr,size_t bytes);
#endif

// PURPOSE:	Allocate virtual memory from the underlying OS.
// PARAMS:	size - Amount of memory to allocate (typically rounded to multiple of 64k)
// RETURNS:	Pointer to memory, or NULL if not available
// NOTES:	These allocations intentionally bypass the rage tracker, because they are
//			typically used to provide the "backing store" of our game heaps.
#if __WIN32PC
void* sysMemVirtualAllocate(size_t size, bool bTracked=false);
#else
void* sysMemVirtualAllocate(size_t size);
#endif

#if RSG_DURANGO && !__FINAL
void* VirtualAllocTracked(void* lpAddress, size_t dwSize, unsigned long flAllocationType, unsigned long flProtect);
int VirtualFreeTracked(void* lpAddress, size_t dwSize, unsigned long dwFreeType);
#else
#define VirtualAllocTracked ::VirtualAlloc
#define VirtualFreeTracked ::VirtualFree
#endif

#if API_HOOKER
void* __stdcall VirtualAllocHooked(void* lpAddress, size_t dwSize, unsigned long flAllocationType, unsigned long flProtect);
int __stdcall VirtualFreeHooked(void* lpAddress, size_t dwSize, unsigned long dwFreeType);

void* __stdcall HeapAllocHooked(void* hHeap, unsigned long dwFlags, size_t dwSize);
void* __stdcall HeapReAllocHooked(void* hHeap, unsigned long dwFlags, void* lpMem, size_t dwSize);
int __stdcall HeapFreeHooked(void* hHeap, unsigned long dwFlags, void* lpMem);

void* __stdcall LocalAllocHooked(unsigned int dwFlags, size_t dwSize);
void* __stdcall LocalReAllocHooked(void* lpMem, size_t dwSize, unsigned int dwFlags);
void* __stdcall LocalFreeHooked(void* lpMem);
void* __stdcall GlobalFreeHooked(void* lpMem);
#endif

// PURPOSE:	Return virtual memory to the underlying OS.
// PARAMS:	ptr - Pointer returned by sysMemVirtualAllocate
// NOTES:	These allocations intentionally bypass the rage tracker, because they are
//			typically used to provide the "backing store" of our game heaps.
void sysMemVirtualFree(void *ptr);

// PURPOSE:	Allocate virtual memory from the underlying OS.
// PARAMS:	size - Amount of memory to allocate (typically rounded to multiple of 64k)
// RETURNS:	Pointer to memory, or NULL if not available
// NOTES:	These allocations intentionally bypass the rage tracker, because they are
//			typically used to provide the "backing store" of our game heaps.
void* sysMemFlexAllocate(size_t size);

// PURPOSE:	Return virtual memory to the underlying OS.
// PARAMS:	ptr - Pointer returned by sysMemVirtualAllocate
// NOTES:	These allocations intentionally bypass the rage tracker, because they are
//			typically used to provide the "backing store" of our game heaps.
int sysMemFlexFree(void *ptr, size_t size);

#if RSG_DURANGO
// PURPOSE:	Allocate virtual memory from the underlying OS.
// PARAMS:	size - Amount of memory to allocate (typically rounded to multiple of 64k)
//			alignment - Required alignment.
// RETURNS:	Pointer to memory, or NULL if not available
// NOTES:	These allocations intentionally bypass the rage tracker, because they are
//			typically used to provide the "backing store" of our game heaps.
void* sysMemPhysicalAllocate(size_t size, size_t alignment, PhysicalMemoryType memType = PhysicalMemoryType::PHYS_MEM_DEFAULT);

inline void* sysMemPhysicalAllocate(size_t size, PhysicalMemoryType memType = PhysicalMemoryType::PHYS_MEM_DEFAULT)
{ 
	return sysMemPhysicalAllocate(size, 4096, memType); 
}

#else

// PURPOSE:	Allocate virtual memory from the underlying OS.
// PARAMS:	size - Amount of memory to allocate (typically rounded to multiple of 64k)
// RETURNS:	Pointer to memory, or NULL if not available
// NOTES:	These allocations intentionally bypass the rage tracker, because they are
//			typically used to provide the "backing store" of our game heaps.
void* sysMemPhysicalAllocate(size_t size);

#endif // RSG_DURANGO

// PURPOSE:	Return virtual memory to the underlying OS.
// PARAMS:	ptr - Pointer returned by sysMemVirtualAllocate
// NOTES:	These allocations intentionally bypass the rage tracker, because they are
//			typically used to provide the "backing store" of our game heaps.
void sysMemPhysicalFree(void *ptr);

// PURPOSE:	Allocate virtual memory from the underlying OS.
// PARAMS:	size - Amount of memory to allocate (typically rounded to multiple of 64k)
// RETURNS:	Pointer to memory, or NULL if not available
// NOTES:	These allocations intentionally bypass the rage tracker, because they are
//			typically used to provide the "backing store" of our game heaps.
void* sysMemHeapAllocate(size_t size);

// PURPOSE:	Return virtual memory to the underlying OS.
// PARAMS:	ptr - Pointer returned by sysMemVirtualAllocate
// NOTES:	These allocations intentionally bypass the rage tracker, because they are
//			typically used to provide the "backing store" of our game heaps.
bool sysMemHeapFree(void *ptr);

#if ENABLE_DEBUG_HEAP
// PURPOSE:	Allocate virtual memory from the underlying OS.
// PARAMS:	size - Amount of memory to allocate (typically rounded to multiple of 64k)
// RETURNS:	Pointer to memory, or NULL if not available
// NOTES:	These allocations intentionally bypass the rage tracker, because they are
//			typically used to provide the "backing store" of our debug heaps.  This
//			memory can only ever be used by the main processor, not the GPU
void* sysMemDebugAllocate(size_t size);

// PURPOSE:	Return virtual memory to the underlying OS.
// PARAMS:	ptr - Pointer returned by sysMemDebugAllocate
// NOTES:	These allocations intentionally bypass the rage tracker, because they are
//			typically used to provide the "backing store" of our debug heaps.
void sysMemDebugFree(void *ptr);
#endif

// PURPOSE:	Return the total free memory available in the OS.
// PARAMS:	None
// NOTES:
u64 sysMemTotalFreeMemory();

// PURPOSE:	Return the total available memory of the underlying OS.
// PARAMS:	None
// NOTES:
u64 sysMemTotalMemory();

// PURPOSE:	Return the largest free block of memory available in the OS.
// PARAMS:	None
// NOTES:
u64 sysMemLargestFreeBlock();

// PURPOSE:	Return the total free memory in application space
// PARAMS:	None
// NOTES:
u64 sysMemTotalFreeAppMemory();

// PURPOSE:	Return the total available kernel memory of the underlying OS.
// PARAMS:	None
// NOTES:
u64 sysMemTotalKernelMemory();

// PURPOSE:	Return the total free kernel memory of the underlying OS.
// PARAMS:	None
// NOTES:
u64 sysMemTotalFreeKernelMemory();

// PURPOSE:	Return the name of the bucket.
// PARAMS:	Id of the bucket.
// NOTES:
const char * sysMemGetBucketName(MemoryBucketIds bucket);

// PURPOSE: Callback for Rockstar Target Manager to get the list of physical
//          memory allocations (allocated with sysMemPhysicalAllocate).  This is
//          required since IXboxDebugTarget.MemoryRegions does not list them.
#define LIST_PHYSICAL_ALLOCS_CALLBACK   (!__FINAL && !__NO_OUTPUT && RSG_XENON)
#if LIST_PHYSICAL_ALLOCS_CALLBACK
	struct SysMemPhysicalAlloc
	{
		void   *base;
		size_t  size;
	};
	const SysMemPhysicalAlloc *sysMemListPhysicalAllocs();
#endif // LIST_PHYSICAL_ALLOCS_CALLBACK

//PURPOSE
//  Delete an object allocated from an allocator by calling its
//  destructor and then freeing the memory in the allocator.
template<typename T>
inline
	void Delete(T* t, sysMemAllocator& a)
{
	if(t)
	{
		t->~T();
		a.Free(t);
	}
}

//PURPOSE
//  Deletes an array of objects allocated from an allocator by calling
//  each of their destructor and then freeing the memory in the allocator.
template<typename T>
inline
	void Delete(T* t, const size_t len, sysMemAllocator& a)
{
	if(t)
	{
		for(size_t i = 0; i < len; ++i)
		{
			t[i].~T();
		}

		a.Free(t);
	}
}

const size_t sysMemOsPageSize = RSG_ORBIS? 16384 : 4096;
const size_t sysMemOsPageSizeMask = sysMemOsPageSize - 1;


static __forceinline uptr AlignMem(uptr x, uptr y)
{
	return (x+y-1) & ~(y-1);
}


#if RSG_DURANGO

	// PURPOSE: Hook function for startup code to install using XMemSetAllocationHooks
	void *sysMemXMemAlloc(size_t size, u64 attributes);

	// PURPOSE: Hook function for startup code to install using XMemSetAllocationHooks
	void sysMemXMemFree(void *address, u64 attributes);

	// PURPOSE: User level, per allocator id, hooks
	typedef void *sysMemXMemAllocHook(size_t size, u64 attributes);
	typedef void sysMemXMemFreeHook(void *address, u64 attributes);
	void sysMemSetXMemHooks(u8 allocatorId, sysMemXMemAllocHook *allocHook, sysMemXMemFreeHook *freeHook);

	size_t sysMemXMemCalculateAlignment( u64 const alignmentFlags );

#endif // RSG_DURANGO

#if __WIN32PC
	// PURPOSE:	Set a region of memory to be write protected
	// PARAMS:	size - Amount of memory to protect
	// RETURNS:	true if successful
	// NOTES:	For debug tracking
	bool sysMemLockMemory(void * memory, size_t size);

	// PURPOSE:	Set a region of memory to be read/write enabled
	// PARAMS:	size - Amount of memory to protect
	// RETURNS:	true if successful
	// NOTES:	For debug tracking
	bool sysMemUnlockMemory(void * memory, size_t size);
#endif


}   //namespace rage

#endif // SYSTEM_MEMORY_H

